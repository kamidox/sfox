#include "inc/FS_Config.h"

#ifdef FS_MODULE_WEB

#include "inc/web/FS_Wbxml.h"
#include "inc/util/FS_Util.h"
#include "inc/inte/FS_Inte.h"
#include "inc/util/FS_Charset.h"
#include "inc/util/FS_MemDebug.h"

#define FS_WBXML_TOKEN_SWITCH_PAGE		0x00
#define FS_WBXML_TOKEN_END				0x01
#define FS_WBXML_TOKEN_ENTITY			0x02
#define FS_WBXML_TOKEN_STR_I			0x03
#define FS_WBXML_TOKEN_LITERAL			0x04

#define FS_WBXML_TOKEN_EXT_I0			0x40
#define FS_WBXML_TOKEN_EXT_I1			0x41
#define FS_WBXML_TOKEN_EXT_I2			0x42
#define FS_WBXML_TOKEN_PI				0x43
#define FS_WBXML_TOKEN_LITERAL_C		0x44

#define FS_WBXML_TOKEN_EXT_T0			0x80
#define FS_WBXML_TOKEN_EXT_T1			0x81
#define FS_WBXML_TOKEN_EXT_T2			0x82
#define FS_WBXML_TOKEN_STR_T			0x83
#define FS_WBXML_TOKEN_LITERAL_A		0x84

#define FS_WBXML_TOKEN_EXT0				0xC0
#define FS_WBXML_TOKEN_EXT1				0xC1
#define FS_WBXML_TOKEN_EXT2				0xC2
#define FS_WBXML_TOKEN_OPAQUE			0xC3
#define FS_WBXML_TOKEN_LITERAL_AC		0xC4

#define FS_WBXML_STAG( byteVal )		((byteVal) & 0x3F)
#define FS_WBXML_AFLAG( byteVal )		((byteVal) & 0x80)
#define FS_WBXML_CFLAG( byteVal )		((byteVal) & 0x40)

#define FS_WBXML_PARSER_DATA_PTR( parser )				((parser)->data + (parser)->offset)
#define FS_WBXML_PARSER_CALLBACK( parser, ev, param )	((parser)->handler((parser)->user_data, (ev), (FS_UINT4)(param)))
#define FS_WBXML_PARSER_DATA_BYTE( parser )				((parser)->data[(parser)->offset])

#define FS_WBXML_IS_EXTENTION( byteVal )	\
	( (byteVal) == FS_WBXML_TOKEN_EXT0 || (byteVal) == FS_WBXML_TOKEN_EXT1 \
	|| (byteVal) == FS_WBXML_TOKEN_EXT2 || (byteVal) == FS_WBXML_TOKEN_EXT_I0 \
	|| (byteVal) == FS_WBXML_TOKEN_EXT_I1 || (byteVal) == FS_WBXML_TOKEN_EXT_I2 \
	|| (byteVal) == FS_WBXML_TOKEN_EXT_T0 || (byteVal) == FS_WBXML_TOKEN_EXT_T1 \
	|| (byteVal) == FS_WBXML_TOKEN_EXT_T2 )

/* attribute value is large than 128 or encode to string or extention or entity */
#define FS_WBXML_IS_ATTR_VALUE( byteVal )	\
	( (byteVal) >= 128 || FS_WBXML_IS_EXTENTION(byteVal) \
	|| (byteVal) == FS_WBXML_TOKEN_STR_I || (byteVal) == FS_WBXML_TOKEN_STR_T || (byteVal) == FS_WBXML_TOKEN_ENTITY )

/*
	content = element | string | extension | entity | pi | opaque
	we this ignore the element and pi. just to lookup other content value
*/
#define FS_WBXML_IS_TAG_CONTENT( byteVal )	\
	( FS_WBXML_IS_EXTENTION(byteVal) || (byteVal) == FS_WBXML_TOKEN_STR_I \
	|| (byteVal) == FS_WBXML_TOKEN_STR_T || (byteVal) == FS_WBXML_TOKEN_ENTITY || (byteVal) == FS_WBXML_TOKEN_OPAQUE )
	
typedef struct FS_WbxmlTag_Tag
{
	FS_List			list;
	FS_UINT4		stag;
}FS_WbxmlTag;

typedef struct FS_WbxmlParser_Tag
{
	FS_UINT1 *			data;
	FS_SINT4			len;
	FS_SINT4			offset;

	FS_BYTE *			str_table;
	FS_SINT4			table_len;
	FS_WbxmlHandler		handler;
	void *				user_data;

	FS_List				tag_list;
	FS_UINT4			stag;		/* may unknow tag LITERAL index */
	FS_BOOL				a_flag;		/* this tag contain attribute */
	FS_BOOL				c_flag;		/* this tag contain content */
}FS_WbxmlParser;

static FS_BOOL FS_WbxmlParserReadByte( FS_WbxmlParser *parser, FS_BYTE *out )
{
	if( parser->offset < parser->len )
	{
		*out = parser->data[parser->offset];
		parser->offset ++;
		return FS_TRUE;
	}
	else
	{
		return FS_FALSE;
	}
}

static void FS_WbxmlFreeTokenList( FS_List *head )
{
	FS_List *node;
	FS_WbxmlToken *pTok;

	node = head->next;
	while( node != head )
	{
		pTok = FS_ListEntry( node, FS_WbxmlToken, list );
		node = node->next;

		FS_ListDel( &pTok->list );
		FS_SAFE_FREE( pTok->data );
		IFS_Free( pTok );
	}
}

static FS_BOOL FS_WbxmlAttrValue( FS_WbxmlParser *parser, FS_BYTE preByte, FS_WbxmlToken *pTok )
{
	FS_UINT4 uval;
	FS_CHAR *str;
	FS_BOOL ret = FS_TRUE;
	
	if( preByte == FS_WBXML_TOKEN_STR_I
		|| preByte == FS_WBXML_TOKEN_EXT_I0
		|| preByte == FS_WBXML_TOKEN_EXT_I1 
		|| preByte == FS_WBXML_TOKEN_EXT_I2 )
	{
		/* inline and extension I */
		str = FS_WBXML_PARSER_DATA_PTR(parser);
		pTok->value_type = FS_WBXML_TOK_STRING;
		pTok->data = IFS_Strdup( str );
		pTok->value = IFS_Strlen( str );
		parser->offset += pTok->value + 1;	/* +1 to skip terminate char */
	}
	else if( preByte == FS_WBXML_TOKEN_STR_T
		|| preByte == FS_WBXML_TOKEN_EXT_T0 
		|| preByte == FS_WBXML_TOKEN_EXT_T1 
		|| preByte == FS_WBXML_TOKEN_EXT_T2 )
	{
		/* table ref */
		parser->offset += FS_UIntVarToUInt4( &uval, FS_WBXML_PARSER_DATA_PTR(parser) );
		if( preByte == FS_WBXML_TOKEN_STR_T && (FS_SINT4)uval < parser->table_len )
		{
			pTok->value_type = FS_WBXML_TOK_STRING;
			str = parser->str_table + uval;
			pTok->data = IFS_Strdup( str );
		}
		else if( (FS_SINT4)uval < parser->table_len )
		{
			pTok->value_type = preByte;
			str = parser->str_table + uval;
			pTok->value = IFS_Strlen( str );
			pTok->data = IFS_Strdup( str );
		}
	}
	else if( preByte == FS_WBXML_TOKEN_EXT0 
		|| preByte == FS_WBXML_TOKEN_EXT1 
		|| preByte == FS_WBXML_TOKEN_EXT2 )
	{
		/* EXT */
		pTok->value_type = preByte;
		pTok->value = FS_WBXML_PARSER_DATA_BYTE( parser );
		parser->offset ++;
	}
	else if( preByte == FS_WBXML_TOKEN_ENTITY )
	{
		/* entity = ENTITY entcode */
		parser->offset += FS_UIntVarToUInt4( &pTok->value, FS_WBXML_PARSER_DATA_PTR(parser) );
		pTok->value_type = FS_WBXML_TOK_ENTITY;
	}
	else
	{
		/* something else. may error */
		ret = FS_FALSE;
	}
	return ret;
}

/* content = element | string | extension | entity | pi | opaque */
static FS_BOOL FS_WbxmlContentValue( FS_WbxmlParser *parser, FS_BYTE preByte, FS_WbxmlToken *pTok )
{
	FS_BOOL ret;

	/* FS_WbxmlAttrValue process string | extension | entity */
	ret = FS_WbxmlAttrValue( parser, preByte, pTok );
	if( ! ret )
	{
		if( preByte == FS_WBXML_TOKEN_OPAQUE )
		{
			/* opaque = OPAQUE length *byte */
			parser->offset += FS_UIntVarToUInt4( &pTok->value, FS_WBXML_PARSER_DATA_PTR(parser) );
			if( pTok->value > 0 && (FS_SINT4)(pTok->value) < (parser->len - parser->offset) )
			{
				pTok->data = IFS_Malloc( pTok->value );
				if( pTok->data )
				{
					IFS_Memcpy( pTok->data, FS_WBXML_PARSER_DATA_PTR(parser), pTok->value );
				}
			}
			pTok->value_type = FS_WBXML_TOK_OPAQUE;
			ret = FS_TRUE;
		}
		else
		{
			/* may end of content or some nest element */
			ret = FS_FALSE;
		}
	}
	return ret;
}

/* if reach to END TAG ( attribute end ). return FS_TRUE */
static FS_BOOL FS_WbxmlTagAttribute( FS_WbxmlParser *parser, FS_WbxmlAttr *pAttr )
{
	FS_BYTE byteVal;
	FS_WbxmlToken *pTok;
	FS_BOOL ret = FS_FALSE;
	
	byteVal = FS_WBXML_PARSER_DATA_BYTE( parser );
	
	if( byteVal < 128 )
	{
		/* tokens with a value less than 128 indicate the start of an attribute. */
		parser->offset ++;
		pAttr->attr_start = byteVal;
		
		while( FS_WbxmlParserReadByte( parser, &byteVal ) )
		{
			if( byteVal == FS_WBXML_TOKEN_END )
			{
				ret = FS_TRUE;
				break;
			}
			else if( FS_WBXML_IS_ATTR_VALUE( byteVal ) )
			{
				pTok = FS_NEW( FS_WbxmlToken );
				if( pTok )
				{
					IFS_Memset( pTok, 0, sizeof(FS_WbxmlToken) );
					/* attrValue = ATTRVALUE | string | extension | entity */
					if( byteVal >= 128 && ! FS_WBXML_IS_EXTENTION(byteVal) )
					{
						/* tokens with a value of 128 or greater represent a well-known string present in an attribute value. */
						pTok->value_type = FS_WBXML_TOK_ATTR_VALUE;
						pTok->value = byteVal;
						FS_ListAddTail( &pAttr->attr_value, &pTok->list );
					}
					else 
					{
						if( FS_WbxmlAttrValue( parser, byteVal, pTok ) )
						{
							FS_ListAddTail( &pAttr->attr_value, &pTok->list );
						}
						else
						{
							/* some thing unsupport token. may end of attribute value */
							parser->offset --;
							IFS_Free( pTok );
							break;	/* break out to return */
						}
					}
				}
			}
			else
			{
				/* not a token value. just contain attr start token. break out to return */
				parser->offset --;
				break;
			}
		}
	}
	else
	{
		ret = FS_TRUE;	/* document error */
	}

	return ret;
}

static FS_BOOL FS_WbxmlTagContent( FS_WbxmlParser *parser, FS_WbxmlContent *pCnt )
{
	FS_BYTE byteVal;
	FS_WbxmlToken *pTok;
	FS_BOOL ret = FS_FALSE;
	
	while( FS_WbxmlParserReadByte( parser, &byteVal ) )
	{
		if( byteVal == FS_WBXML_TOKEN_END )
		{
			ret = FS_TRUE;
			break;	/* content end */
		}
		else
		{
			pTok = FS_NEW( FS_WbxmlToken );
			if( pTok )
			{
				IFS_Memset( pTok, 0, sizeof(FS_WbxmlToken) );
				if( FS_WbxmlContentValue( parser, byteVal, pTok ) )
				{
					FS_ListAddTail( &pCnt->content, &pTok->list );
				}
				else
				{
					/* some thing unsupport token. may end of content value or some nest element */
					parser->offset --;
					IFS_Free( pTok );
					break;
				}
			}
		}
	}

	return ret;
}

static void FS_WbxmlProcessPI( FS_WbxmlParser *parser )
{
	FS_BYTE byteVal;
	FS_WbxmlAttr attr;
	
	IFS_Memset( &attr, 0, sizeof(FS_WbxmlAttr) );
	FS_ListInit( &attr.attr_value );
	
	/* PI: PI attrStart *attrValue END */
	byteVal = FS_WBXML_PARSER_DATA_BYTE( parser );
	if( byteVal < 128 )
	{
		/* tokens with a value less than 128 indicate the start of an attribute. */
		FS_WbxmlTagAttribute( parser, &attr );
		FS_WBXML_PARSER_CALLBACK( parser, FS_WBXML_EV_PI, &attr );
		FS_WbxmlFreeTokenList( &attr.attr_value );
	}
}

static void FS_WbxmlPushTag( FS_WbxmlParser *parser )
{
	FS_WbxmlTag *pTag = FS_NEW( FS_WbxmlTag );
	if( pTag )
	{
		pTag->stag = parser->stag;
		if( parser->a_flag )
			pTag->stag = pTag->stag | 0x80;
		if( parser->c_flag )
			pTag->stag = pTag->stag | 0x40;
		FS_ListAdd( &parser->tag_list, &pTag->list );
	}
}

static FS_BOOL FS_WbxmlPopTag( FS_WbxmlParser *parser )
{
	FS_WbxmlTag *pTag;
	FS_List *node;
	FS_BOOL ret = FS_FALSE;
	
	node = parser->tag_list.next;
	if( node != &parser->tag_list )
	{
		pTag = FS_ListEntry( node, FS_WbxmlTag, list );
		FS_ListDel( &pTag->list );

		FS_WBXML_PARSER_CALLBACK( parser, FS_WBXML_EV_TAGEND, FS_WBXML_STAG(pTag->stag) );
		
		ret = FS_TRUE;
		IFS_Free( pTag );
	}
	return ret;
}

static FS_UINT4 FS_WbxmlTopTag( FS_WbxmlParser *parser )
{
	FS_WbxmlTag *pTag;
	FS_List *node;
	FS_UINT4 ret = 0;
	
	node = parser->tag_list.next;
	if( node != &parser->tag_list )
	{
		pTag = FS_ListEntry( node, FS_WbxmlTag, list );
		ret = pTag->stag;
	}
	return ret;
}

void FS_WbxmlProcessData( FS_BYTE *data, FS_SINT4 len, FS_WbxmlHandler handler, void *user_data )
{
	FS_BYTE byteVal;
	FS_UINT4 uval;
	FS_WbxmlParser parser;
	FS_WbxmlAttr attr;
	FS_WbxmlContent cnt;
	FS_BOOL bTokEnd;
	
	IFS_Memset( &parser, 0, sizeof(FS_WbxmlParser) );
	
	parser.data = data;
	parser.len = len;
	parser.offset = 0;
	parser.handler = handler;
	parser.user_data = user_data;
	FS_ListInit( &parser.tag_list );
	
	/* Wbxml version */
	byteVal = FS_WBXML_PARSER_DATA_BYTE( &parser );
	parser.offset ++;
	FS_WBXML_PARSER_CALLBACK( &parser, FS_WBXML_EV_VER, byteVal );
	
	/* Document Public Identifier: uintvar */
	parser.offset += FS_UIntVarToUInt4( &uval, FS_WBXML_PARSER_DATA_PTR(&parser) );
	FS_WBXML_PARSER_CALLBACK( &parser, FS_WBXML_EV_PUBID, uval );

	/* Charset: uintvar */
	parser.offset += FS_UIntVarToUInt4( &uval, FS_WBXML_PARSER_DATA_PTR(&parser) );
	FS_WBXML_PARSER_CALLBACK( &parser, FS_WBXML_EV_CHARSET, uval );

	/* String Table: length *byte */
	parser.offset += FS_UIntVarToUInt4( &uval, FS_WBXML_PARSER_DATA_PTR(&parser) );
	if( uval > 0 && (FS_SINT4)uval < parser.len )
	{
		parser.str_table = IFS_Malloc( uval );
		if( parser.str_table )
		{
			parser.table_len = (FS_SINT4)uval;
			IFS_Memcpy( parser.str_table, FS_WBXML_PARSER_DATA_PTR(&parser), uval );
		}
		parser.offset += uval;
	}

	/* Body: *pi element *pi */
	while( FS_WbxmlParserReadByte( &parser, &byteVal ) )
	{
		if( byteVal == FS_WBXML_TOKEN_PI )
		{
			FS_WbxmlProcessPI( &parser );
		}
		if( byteVal == FS_WBXML_TOKEN_END )
		{
			/* encounter END token. */
			FS_WbxmlPopTag( &parser );
		}
		else if( FS_WBXML_IS_TAG_CONTENT( byteVal ) )
		{
			/* tag content */
			uval = FS_WbxmlTopTag( &parser );
			while( (parser.c_flag || FS_WBXML_CFLAG(uval)) && FS_WBXML_IS_TAG_CONTENT(byteVal))
			{
				IFS_Memset( &cnt, 0, sizeof(FS_WbxmlContent) );
				FS_ListInit( &cnt.content );

				parser.offset --;	/* roll back one byte of the content */
				cnt.stag = FS_WbxmlTopTag( &parser );
				cnt.stag = FS_WBXML_STAG( cnt.stag );
				bTokEnd = FS_WbxmlTagContent( &parser, &cnt );
				if( ! FS_ListIsEmpty(&cnt.content) )
				{
					FS_WBXML_PARSER_CALLBACK( &parser, FS_WBXML_EV_CONTENT, &cnt );
					FS_WbxmlFreeTokenList( &cnt.content );
				}
			
				if( bTokEnd )
				{
					/* encounter END token. */
					FS_WbxmlPopTag( &parser );
				}
				uval = FS_WbxmlTopTag( &parser );
				byteVal = FS_WBXML_PARSER_DATA_BYTE( &parser );
			}
		}
		else
		{
			/* here, must a STag: stag = TAG | ( LITERAL index ) */
			if( byteVal == FS_WBXML_TOKEN_LITERAL 
				|| byteVal == FS_WBXML_TOKEN_LITERAL_A
				|| byteVal == FS_WBXML_TOKEN_LITERAL_C
				|| byteVal == FS_WBXML_TOKEN_LITERAL_AC )
			{
				parser.offset += FS_UIntVarToUInt4( &parser.stag, FS_WBXML_PARSER_DATA_PTR(&parser) );
			}
			else
			{
				parser.stag = FS_WBXML_STAG( byteVal );
			}
			parser.a_flag = FS_WBXML_AFLAG( byteVal );
			parser.c_flag = FS_WBXML_CFLAG( byteVal );

			FS_WbxmlPushTag( &parser );
			
			FS_WBXML_PARSER_CALLBACK( &parser, FS_WBXML_EV_STAG, parser.stag );
			/* Tags containing both attributes and content always encode the attributes before the content. */
			if( parser.a_flag )
			{
				/*
					All tokenised attributes must begin with a single attribute start token 
					and may be followed by zero or more attribute value, string, entity or extension 
					tokens. An attribute start token, a LITERAL token or the END token indicates the 
					end of an attribute value.				
				*/
				do {
					IFS_Memset( &attr, 0, sizeof(FS_WbxmlAttr) );
					FS_ListInit( &attr.attr_value );

					attr.stag = parser.stag;
					bTokEnd = FS_WbxmlTagAttribute( &parser, &attr );
					FS_WBXML_PARSER_CALLBACK( &parser, FS_WBXML_EV_ATTR, &attr );
					FS_WbxmlFreeTokenList( &attr.attr_value );

					byteVal = FS_WBXML_PARSER_DATA_BYTE( &parser );
				} while( ! bTokEnd && ( byteVal < 128 || FS_WBXML_STAG(byteVal) == FS_WBXML_TOKEN_LITERAL) );
			}
			
			if( ! parser.c_flag )
			{
				/* no content. end tag */
				FS_WbxmlPopTag( &parser );
			}

			uval = FS_WbxmlTopTag( &parser );
			byteVal = FS_WBXML_PARSER_DATA_BYTE( &parser );
			while( (parser.c_flag || FS_WBXML_CFLAG(uval)) && FS_WBXML_IS_TAG_CONTENT(byteVal))
			{
				IFS_Memset( &cnt, 0, sizeof(FS_WbxmlContent) );
				FS_ListInit( &cnt.content );

				cnt.stag = FS_WbxmlTopTag( &parser );
				cnt.stag = FS_WBXML_STAG( cnt.stag );
				bTokEnd = FS_WbxmlTagContent( &parser, &cnt );
				if( ! FS_ListIsEmpty(&cnt.content) )
				{
					FS_WBXML_PARSER_CALLBACK( &parser, FS_WBXML_EV_CONTENT, &cnt );
					FS_WbxmlFreeTokenList( &cnt.content );
				}

				if( bTokEnd )
				{
					/* encounter END token. */
					FS_WbxmlPopTag( &parser );
				}
				uval = FS_WbxmlTopTag( &parser );
				byteVal = FS_WBXML_PARSER_DATA_BYTE( &parser );
			}
		}
	}

	while( FS_WbxmlPopTag( &parser ) )
		;	/* use this to pop all tag */

	FS_SAFE_FREE( parser.str_table );
}

void FS_WbxmlProcessFile( FS_SINT4 dir, FS_CHAR *file, FS_WbxmlHandler handler, void *user_data )
{
	FS_SINT4 size;
	FS_BYTE *buf;
	
	size = FS_FileGetSize( dir, file );
	if( size > 0 )
	{
		buf = IFS_Malloc( size );
		if( buf )
		{
			FS_FileRead( dir, file, 0, buf, size );
			FS_WbxmlProcessData( buf, size, handler, user_data );
			IFS_Free( buf );
		}
	}
}

/* utility for wbxml attribute token value */
void FS_WbxmlAttrTokenValue( FS_BYTE *out, FS_SINT4 len, FS_List *pTokList, FS_WbxmlAttrValueMapFunc attrValueFunc )
{
	FS_List *node;
	FS_WbxmlToken *pTok;
	FS_CHAR *str;
	FS_SINT4 slen, offset;

	offset = 0;
	node = pTokList->next;
	len --;
	while( node != pTokList )
	{
		pTok = FS_ListEntry( node, FS_WbxmlToken, list );
		node = node->next;

		if( pTok->value_type == FS_WBXML_TOK_ATTR_VALUE && (str = attrValueFunc((FS_UINT1)pTok->value)) )
		{
			slen = IFS_Strlen( str );
			if( slen > (len - offset) )
				slen = len - offset;
			IFS_Memcpy( out + offset, str, slen );
			offset += slen;
		}
		else if( pTok->data && (pTok->value_type == FS_WBXML_TOK_STRING || pTok->value_type == FS_WBXML_TOK_OPAQUE) )
		{
			if( (FS_SINT4)pTok->value > (len - offset) )
				slen = len - offset;
			else
				slen = pTok->value;
			IFS_Memcpy( out + offset, pTok->data, slen );
			offset += slen;
		}
		else if( pTok->value_type == FS_WBXML_TOK_ENTITY )
		{
			/* assume as UTF-8 */
			offset += FS_CnvtUcs2ToUtf8Char( (FS_WCHAR)pTok->value, (FS_CHAR *)out + offset );
		}
		else if( FS_WBXML_IS_EXTENTION(pTok->value_type) )
		{
			/* extention here EXT*, EXT_I*, EXT_T* */
			if( pTok->data && pTok->value > 0 )
			{
				/* if data is not NULL. means that must be EXT_I*, EXT_T* */
				/* base on binary format or WML. this is a var. so we add a var flag */
				IFS_Memcpy( out + offset, "$(", 2 );
				offset += 2;
				IFS_Memcpy( out + offset, pTok->data, pTok->value );
				offset += pTok->value;
				out[offset ++] = ')';
			}
		}
		/* ignore other type value */
	}

	out[offset] = 0;
}


#ifdef FS_DEBUG_

#include "inc/util/FS_File.h"

#define FS_WBXML_TEST_FILE	"test.wbxml"
#define FS_WBXML_OUT_FILE	"wbxml_out.txt"

static FS_SINT4 GFS_WbxmlOutOffset = 0;

void FS_WbxmlTestDumpTokens( FS_List *head, FS_CHAR *buf )
{
	FS_WbxmlToken *pTok;
	FS_List *node;
	FS_SINT4 len = 0;
	
	buf[0] = 0;
	node = head->next;
	while( node != head )
	{
		pTok = FS_ListEntry( node, FS_WbxmlToken, list );
		node = node->next;

		if( pTok->value_type == FS_WBXML_TOK_ATTR_VALUE )
		{
			len = IFS_Strlen( buf );
			IFS_Sprintf( buf + len, "ATTRVAL %u ", pTok->value );
		}
		else if( pTok->value_type == FS_WBXML_TOK_ENTITY )
		{
			len = IFS_Strlen( buf );
			IFS_Sprintf( buf + len, "ENTITY %u ", pTok->value );
		}
		else if( pTok->value_type == FS_WBXML_TOK_STRING )
		{
			len = IFS_Strlen( buf );
			IFS_Sprintf( buf + len, "STRING %s ", pTok->data );
		}
		else if( pTok->value_type == FS_WBXML_TOK_OPAQUE )
		{
			len = IFS_Strlen( buf );
			IFS_Sprintf( buf + len, "OPAQUE len = %u, data_ptr = 0x%X ", pTok->value, pTok->data );			
		}
		else
		{
			len = IFS_Strlen( buf );
			IFS_Sprintf( buf + len, "EXT %d, %u ", pTok->value_type, pTok->value );
		}			
	}
}

void FS_WbxmlTestHandler( void *user_data, FS_UINT1 event, FS_UINT4 param )
{
	static FS_CHAR str[2048];
	FS_SINT4 len;
	FS_WbxmlAttr *pAttr;
 	FS_WbxmlContent *pCnt;
 	
	if( event == FS_WBXML_EV_VER )
	{
		IFS_Sprintf( str, "VERSION: %u\r\n", param );
		len = IFS_Strlen( str );
		FS_FileWrite( FS_DIR_ROOT, FS_WBXML_OUT_FILE, GFS_WbxmlOutOffset, str, len );
		GFS_WbxmlOutOffset += len;
	}
	else if( event == FS_WBXML_EV_PUBID )
	{
		IFS_Sprintf( str, "PUBLICID: %u\r\n", param );
		len = IFS_Strlen( str );
		FS_FileWrite( FS_DIR_ROOT, FS_WBXML_OUT_FILE, GFS_WbxmlOutOffset, str, len );
		GFS_WbxmlOutOffset += len;
	}
	else if( event == FS_WBXML_EV_CHARSET )
	{
		IFS_Sprintf( str, "CHARSET: %u\r\n", param );
		len = IFS_Strlen( str );
		FS_FileWrite( FS_DIR_ROOT, FS_WBXML_OUT_FILE, GFS_WbxmlOutOffset, str, len );
		GFS_WbxmlOutOffset += len;
	}
	else if( event == FS_WBXML_EV_PI )
	{
		pAttr = (FS_WbxmlAttr *)param;
		IFS_Sprintf( str, "PI: attr_start = %u ", pAttr->attr_start );
		len = IFS_Strlen( str );
		FS_WbxmlTestDumpTokens( &pAttr->attr_value, str + len );
		IFS_Strcat( str, "\r\n" );
		len = IFS_Strlen( str );
		FS_FileWrite( FS_DIR_ROOT, FS_WBXML_OUT_FILE, GFS_WbxmlOutOffset, str, len );
		GFS_WbxmlOutOffset += len;
	}
	else if( event == FS_WBXML_EV_STAG )
	{
		IFS_Sprintf( str, "STAG: %u\r\n", param );
		len = IFS_Strlen( str );
		FS_FileWrite( FS_DIR_ROOT, FS_WBXML_OUT_FILE, GFS_WbxmlOutOffset, str, len );
		GFS_WbxmlOutOffset += len;
	}
	else if( event == FS_WBXML_EV_ATTR )
	{
		pAttr = (FS_WbxmlAttr *)param;
		IFS_Sprintf( str, "ATTR: stag = %u attr_start = %u ", pAttr->stag, pAttr->attr_start );
		len = IFS_Strlen( str );
		FS_WbxmlTestDumpTokens( &pAttr->attr_value, str + len );
		IFS_Strcat( str, "\r\n" );
		len = IFS_Strlen( str );
		FS_FileWrite( FS_DIR_ROOT, FS_WBXML_OUT_FILE, GFS_WbxmlOutOffset, str, len );
		GFS_WbxmlOutOffset += len;
	}
	else if( event == FS_WBXML_EV_CONTENT )
	{
		pCnt = (FS_WbxmlContent *)param;
		IFS_Sprintf( str, "CONTENT: stag = %u ", pCnt->stag );
		len = IFS_Strlen( str );
		FS_WbxmlTestDumpTokens( &pCnt->content, str + len );
		IFS_Strcat( str, "\r\n" );
		len = IFS_Strlen( str );
		FS_FileWrite( FS_DIR_ROOT, FS_WBXML_OUT_FILE, GFS_WbxmlOutOffset, str, len );
		GFS_WbxmlOutOffset += len;
	}
	
}

void FS_WbxmlTest( void )
{
	GFS_WbxmlOutOffset = 0;
	FS_WbxmlProcessFile( FS_DIR_ROOT, FS_WBXML_TEST_FILE, FS_WbxmlTestHandler, FS_NULL );
}

#endif

#endif	//FS_MODULE_WEB


