#include "inc/inte/FS_Inte.h"
#include "inc/util/FS_Sax.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_List.h"
#include "inc/util/FS_MemDebug.h"

#define FS_SAX_E_NONE		0
#define FS_SAX_E_HEAD		1
#define FS_SAX_E_BODY		2

typedef struct FS_SaxElement_Tag
{
	FS_List				list;
	FS_CHAR *			name;
}FS_SaxElement;

typedef struct FS_SaxXml_Tag
{
	void *				user_data;
	FS_BYTE *			buf;			/* current process stream. alloc */
	FS_SINT4			buf_len;
	FS_BYTE *			data;			/* current read pos */
	FS_BYTE *			link_pos;		/* when data is provide in multitime, we must save a pos the keep parser element integrate */
	FS_BYTE				state;
	FS_List				element_list;	/* store FS_SaxElement */
	
	FS_BOOL							data_finish;
	FS_BOOL							running;		/* sax is running */
	/* 
		flag to control access to buf. when new feed data, call FS_SaxReadByte, it only return data when process 
		return to main loop : FS_SaxProcXmlDoc function, or, it will return 0. when main loop call FS_SaxReadByte, 
		it must set this flag to FALSE.
	*/
	FS_BOOL							can_access;
	FS_BOOL							pend_request;

	FS_SaxDataRequest				data_request;
	FS_SaxDataRequest				complete;
	
	FS_SaxTextHandler				start_element;
	FS_SaxTextHandler				start_element_end;
	FS_SaxTextHandler				end_element;
	FS_SaxElementTextHandler		element_text;
	FS_SaxElementTextHandler		comment;

	FS_SaxAttrHandler				attribute;

	FS_SaxTextHandler				xml_instruction;
	FS_SaxXmlNoteHandler			xml_note;
}FS_SaxXml;

/*-----------------------------------------------------------------------------------
		local function implement
-----------------------------------------------------------------------------------*/
#if 1
#define FS_IsTokenChar( b )			\
	(((b) != 32 /* ' ' */ && (b) != 34 /* '"' */ && b != '='	\
	&& (b) != 39 /* ''' */ && (b) != 60 /* '<' */ && (b) != 62 /* '>' */)	\
	|| (b) > 0x80 )
#else
#define FS_IsTokenChar( b ) 		\
( ((b) >= 'a' && (b) <= 'z') 		\
|| ((b) >= 63 && (b) <= 'Z') 		\
|| ((b) >= 35 && (b) <= 58 && (b) != 39 )		\
|| ((b) == 95 ) || ((b) == 124 ) || (b) >= 0xA1	 )	/* GB2312 */
#endif

static FS_BYTE FS_SaxReadByte( FS_SaxXml *sax )
{
	FS_BYTE b = 0;
	if( sax->can_access && sax->buf && ((sax->data - sax->buf) < sax->buf_len) )
	{
		b = *sax->data;
		sax->data ++;
	}
	else
	{
		sax->can_access = FS_FALSE;
		if( ! sax->data_finish )
		{
			if( ! sax->pend_request ) 
			{
				sax->pend_request = FS_TRUE;
				sax->data_request( sax->user_data, sax );
			}
		}
	}
	return b;
}

/* will not increment the sax->data point, and will not call data_request */
static FS_BYTE FS_SaxGetByte( FS_SaxXml *sax )
{
	if( sax->buf && (sax->data - sax->buf) < sax->buf_len )
		return *sax->data;
	else
		return 0;
}

static void FS_SaxSkipWhiteSpace( FS_SaxXml *sax )
{
	FS_BYTE b;

	do 
	{
		b = FS_SaxGetByte( sax );
		if( FS_IsWhiteSpace(b) )
			sax->data ++;
		else
			break;
	} while( b );
}

static void FS_SaxLinkPos( FS_SaxXml *sax, FS_SINT4 offset )
{
	sax->link_pos = sax->data + offset;
}

static FS_CHAR *FS_SaxToken( FS_SaxXml *sax, FS_SINT4 * len )
{
	FS_BYTE b, quote = 0;
	FS_BYTE *token;
	FS_SaxSkipWhiteSpace( sax );
	token = sax->data;
	b = FS_SaxGetByte( sax );
	if( b == '"' || b == '\'' )
	{
		token ++;
		sax->data ++;
		quote = b;
	}

	*len = 0;
	if( quote != 0 )
	{
		while( (b = FS_SaxReadByte(sax)) && b != quote )
			;	/* empty here */
	}
	else	
	{
		while( (b = FS_SaxReadByte(sax)) && FS_IsTokenChar( b ) )
			;	/* empty here */
	}
	
	if( b != '"' && b != '\'' && b != 0 )
	{
		sax->data --;	/* rollback one byte */
	}
	else if( b == '"' || b == '\'' )
	{
		*len = -1;		/* exclude the quote char */
	}
	else if( b == 0 )	/* run out buffer */
	{
		return FS_NULL;
	}
	
	*len += (sax->data - token);
	return token;
}

static FS_CHAR *FS_SaxAttrName( FS_SaxXml *sax, FS_SINT4 * len )
{
	FS_BYTE b;
	FS_BYTE *token;
	FS_SaxSkipWhiteSpace( sax );
	token = sax->data;

	*len = 0;
	while( (b = FS_SaxReadByte(sax)) && (FS_IsTokenChar(b) && b != '=' && b != '/') )
		;	/* empty here */
	
	if( b == '/' || b == '=' || b == '>' )
	{
		sax->data --;	/* rollback one byte */
	}
	else if( b == 0 )	/* run out buffer */
	{
		return FS_NULL;
	}
	
	*len += (sax->data - token);
	return token;
}

static FS_CHAR *FS_SaxElementName( FS_SaxXml *sax, FS_SINT4 * len )
{
	FS_BYTE b;
	FS_BYTE *token;
	FS_SaxSkipWhiteSpace( sax );
	token = sax->data;

	*len = 0;
	while( (b = FS_SaxReadByte(sax)) && (FS_IsTokenChar(b) && b != '=' && b != '/' && b > 32) )
		;	/* empty here */
	
	if( b == '/' || b == '=' || b == '>' || b <= 32 )
	{
		sax->data --;	/* rollback one byte */
	}
	else if( b == 0 )	/* run out buffer */
	{
		return FS_NULL;
	}
	
	*len += (sax->data - token);
	return token;
}


static void FS_SaxPushElement( FS_SaxXml *sax, FS_CHAR *ename )
{
	FS_SaxElement *element = IFS_Malloc( sizeof(FS_SaxElement) );
	if( element )
	{
		element->name = ename;
		FS_ListAdd( &sax->element_list, &element->list );
	}
	else
	{
		IFS_Free( ename );		/* avoid memory leak when memory run out */
	}
}

/* will remove the ename and its child element(if it's clile element has no end part) */
static void FS_SaxPopElement( FS_SaxXml *sax, FS_CHAR *ename )
{
	FS_BOOL bFinish = FS_FALSE;
	FS_SaxElement *element;
	FS_List *node = sax->element_list.next;
	while( node != &sax->element_list )
	{
		element = FS_ListEntry( node, FS_SaxElement, list);
		node = node->next;
		FS_ListDel( &element->list );
		if( element->name )
		{
			if( IFS_Stricmp( element->name, ename) == 0 )
				bFinish = FS_TRUE;
			IFS_Free( element->name );
		}
		IFS_Free( element );
		
		if( bFinish ) break;
	}
}

static FS_CHAR* FS_SaxTopElement( FS_SaxXml *sax )
{
	FS_SaxElement *element;
	if( ! FS_ListIsEmpty( &sax->element_list ) )
	{
		element = FS_ListEntry( sax->element_list.next, FS_SaxElement, list);
		return element->name;
	}
	else
	{
		return FS_NULL;
	}
}

static void FS_SaxRunOutElement( FS_SaxXml *sax )
{
	FS_BYTE b;
	while( (b = FS_SaxReadByte(sax)) && b != '>' )
		;	/* empty here */
}

static void FS_SaxXmlNote( FS_SaxXml *sax )
{
	FS_CHAR *token, *version = FS_NULL, *encoding = FS_NULL, b = 0;
	FS_SINT4 len, vlen, clen;
	do
	{
		token = FS_SaxToken( sax, &len );
		FS_SaxSkipWhiteSpace( sax );
		b = FS_SaxReadByte( sax );

		if( token == FS_NULL )
			break;	/* we have run out all buf */
		
		/* if b != '=', format error. we ignore it */
		if( b == '=' && IFS_Strnicmp( token, "version", 7 ) == 0 )
		{
			version = FS_SaxToken( sax, &vlen );
		}
		else if( b == '=' && IFS_Strnicmp( token, "encoding", 8 ) == 0 )
		{
			encoding = FS_SaxToken( sax, &clen );
		}
		else if( b == '=' )
		{
			token = FS_SaxToken( sax, &len );	/* run out unknow attr */
		}
		
		if( encoding != FS_NULL && version != FS_NULL )
		{
			FS_SaxRunOutElement( sax );
			break;
		}
	} while( b != '>' && b != 0 );

	if( version && vlen > 0 )
		version = FS_Strndup( version, vlen );
	else
		version = FS_NULL;
	
	if( encoding && clen > 0 )
		encoding = FS_Strndup( encoding, clen );
	else
		encoding = FS_NULL;
	
	if( b != 0 )
		sax->xml_note( sax->user_data, version, encoding );
	if( version ) IFS_Free( version );
	if( encoding ) IFS_Free( encoding );
}

static void FS_SaxXmlInstruction( FS_SaxXml *sax )
{
	/* TODO */
	FS_SaxRunOutElement( sax );
}

static void FS_SaxXmlComment( FS_SaxXml *sax )
{
	FS_BOOL bHtmlScript = FS_FALSE;
	FS_BYTE b, *cmt;
	FS_SINT4 clen;
	FS_CHAR *ename;
	
	FS_SaxSkipWhiteSpace( sax );
	cmt = sax->data;
	ename = FS_SaxTopElement( sax );
	if( ename && (ename[0] == 's' || ename[0] == 'S') )
		bHtmlScript = ( IFS_Stricmp( ename, "script" ) == 0 );

	/* script comment may have nested comment. so, cannot use "-->" */
	if( bHtmlScript )
		sax->data = FS_StrStrI( (FS_CHAR *)sax->data, "</script" );
	else
		sax->data = IFS_Strstr( (FS_CHAR *)sax->data, "-->" );
	
	if( sax->data == FS_NULL )
	{
		sax->data = sax->buf  + sax->buf_len;	/* comment not end, we run out buffer */
	}
	else
	{
		if( bHtmlScript )
		{
			while( sax->data > cmt && *(sax->data) != '>' )
				sax->data --;
		}
		else
		{
			sax->data += 2;
		}
	}
	
	b = FS_SaxReadByte( sax );
	if( b == '>' )
	{
		sax->data --;
		clen = sax->data - cmt - 2;
		if( clen > 0 )
		{
			sax->comment( sax->user_data, FS_SaxTopElement(sax), cmt, clen );
			FS_SaxLinkPos( sax, 0 );
		}
	}
}

static void FS_SaxXmlCData( FS_SaxXml *sax )
{
	/* TODO */
	FS_SaxRunOutElement( sax );
}

static void FS_SaxXmlDocType( FS_SaxXml *sax )
{
	/* TODO */
	FS_SaxRunOutElement( sax );
}

static void FS_SaxXmlStartElement( FS_SaxXml *sax )
{
	FS_CHAR *token;
	FS_SINT4 tlen;

	token = FS_SaxElementName( sax, &tlen );
	if( token && tlen > 0 )
	{
		if( token[tlen - 1] == '/' ) tlen --;
		
		token = FS_Strndup( token, tlen );
		if( token )
		{
			FS_SaxPushElement( sax, token );
			sax->start_element( sax->user_data, token );
			FS_SaxLinkPos( sax, 0 );
		}
	}
}

static void FS_SaxXmlStartElementEnd( FS_SaxXml *sax )
{
	FS_SaxLinkPos( sax, 0 );
	sax->start_element_end( sax->user_data, FS_SaxTopElement(sax) );
}

static void FS_SaxXmlEndElement( FS_SaxXml *sax )
{
	FS_CHAR *token;
	FS_SINT4 tlen;
	FS_BYTE b;
	FS_BOOL bOK = FS_TRUE;
	
	token = FS_SaxToken( sax, &tlen );
	if( token )
	{
		if( tlen != 0 )
		{
			token = FS_Strndup( token, tlen );
		}
		else
		{
			token = FS_SaxTopElement( sax );
			sax->start_element_end( sax->user_data, token );
		}
		
		FS_SaxSkipWhiteSpace( sax );
		b = FS_SaxReadByte( sax );
		if( b != '>' )
			bOK = FS_FALSE;
		
		if( bOK )
		{
			sax->end_element( sax->user_data, token );
			FS_SaxPopElement( sax, token );
			FS_SaxLinkPos( sax, 0 );
		}
		if( tlen != 0 && token )
			IFS_Free( token );

	}
}

static void FS_SaxXmlAttribute( FS_SaxXml *sax )
{
	FS_BYTE b;
	FS_CHAR *name, *value;
	FS_SINT4 nlen, vlen;

	name = FS_SaxAttrName( sax, &nlen );
	if( name )
	{
		if( nlen > 0 )
			name = FS_Strndup( name, nlen );
		else
			name = FS_NULL;

		FS_SaxSkipWhiteSpace( sax );
		b = FS_SaxReadByte( sax );
		if( b == '=' )
		{
			value = FS_SaxToken( sax, &vlen );
			if( value )
			{
				if( vlen > 0 )
					value = FS_Strndup( value, vlen );
				else
					value = FS_NULL;
				
				FS_TrimBlankSpace( name, nlen );
				FS_TrimBlankSpace( value, vlen );
				sax->attribute( sax->user_data, FS_SaxTopElement(sax), name, value );
				if( value ) IFS_Free( value );
				FS_SaxLinkPos( sax, 0 );
			}
		}
		else	/* no value attribute */
		{
			sax->data --;
			sax->attribute( sax->user_data, FS_SaxTopElement(sax), name, FS_NULL );
			FS_SaxLinkPos( sax, 0 );
		}
		if( name ) IFS_Free( name );
	}
}

static void FS_SaxXmlElementText( FS_SaxXml *sax )
{
	FS_BYTE b, *text;
	FS_CHAR *ename;
	FS_SINT4 tlen;
	FS_BOOL bHtmlScript = FS_FALSE;
	
	FS_SaxSkipWhiteSpace( sax );
	text = sax->data;
	ename = FS_SaxTopElement(sax);
	if( ename && (ename[0] == 's' || ename[0] == 'S') )
	{
		bHtmlScript = ( IFS_Stricmp( ename, "script" ) == 0 );
	}

	if( ! bHtmlScript )
	{
		while( (b = FS_SaxReadByte(sax)) && b != '<' )
			;	/* empty here */
	}
	else		/* here, support to HTML script */
	{
		sax->data = FS_StrStrI( (FS_CHAR *)sax->data, "</script>" );
		if( sax->data == FS_NULL )
			sax->data = sax->buf  + sax->buf_len;	/* script not end, we run out buffer */
		b = FS_SaxReadByte( sax );
	}
	
	if( b == '<' )
	{
		sax->data --;	/* rollback one char */
		tlen = sax->data - text;
		/* trim the right white space */
		while( tlen > 0 && FS_IsWhiteSpace( text[tlen - 1] ) )
			tlen --;
		
		if( tlen > 0 )	/* valid element body text */
		{
			sax->element_text( sax->user_data, ename, text, tlen );
			FS_SaxLinkPos( sax, 0 );
		}
	}
}

/* dummy functon */
static void FS_SaxDummyTextHandler( void * userData, FS_CHAR *str )
{
}
static void FS_SaxDummyElementTextHandler( void * userData,  FS_CHAR *element, FS_CHAR *str, FS_SINT4 slen )
{
}
static void FS_SaxDummyAttrHandler( void * userData, FS_CHAR *element, FS_CHAR *name, FS_CHAR *value )
{
}
static void FS_SaxDummyXmlNoteHandler( void * userData, FS_CHAR *version, FS_CHAR *encoding )
{
}

/*-----------------------------------------------------------------------------------
		export function implement
-----------------------------------------------------------------------------------*/
FS_SaxHandle FS_CreateSaxHandler( void *userData )
{
	FS_SaxXml *sax = IFS_Malloc( sizeof(FS_SaxXml) );
	if( sax )
	{
		IFS_Memset( sax, 0, sizeof(FS_SaxXml) );
		FS_ListInit( &sax->element_list );
		sax->user_data = userData;
		sax->start_element = FS_SaxDummyTextHandler;
		sax->start_element_end = FS_SaxDummyTextHandler;
		sax->end_element = FS_SaxDummyTextHandler;
		sax->element_text = FS_SaxDummyElementTextHandler;
		sax->comment = FS_SaxDummyElementTextHandler;
		sax->attribute = FS_SaxDummyAttrHandler;
		sax->xml_instruction = FS_SaxDummyTextHandler;
		sax->xml_note = FS_SaxDummyXmlNoteHandler;
	}
	return sax;
}

void FS_FreeSaxHandler( FS_SaxHandle hsax )
{
	FS_List *node;
	FS_SaxElement *element;
	FS_SaxXml *sax = (FS_SaxXml *)hsax;
	if( sax->buf )
		IFS_Free( sax->buf );
	
	node = sax->element_list.next;
	while( node != &sax->element_list )
	{
		element = FS_ListEntry( node, FS_SaxElement, list );
		node = node->next;
		FS_ListDel( &element->list );
		IFS_Free( element->name );
		IFS_Free( element );
	}
	IFS_Free( sax );
}

void FS_SaxSetDataRequest( FS_SaxHandle hsax, FS_SaxDataRequest handler )
{
	FS_SaxXml *sax = (FS_SaxXml *)hsax;
	sax->data_request = handler;
}

void FS_SaxSetCompleteHandler( FS_SaxHandle hsax, FS_SaxDataRequest handler )
{
	FS_SaxXml *sax = (FS_SaxXml *)hsax;
	sax->complete = handler;
}


void FS_SaxDataFeed( FS_SaxHandle hsax, FS_BYTE *data, FS_SINT4 dlen, FS_BOOL finish )
{	
	FS_SaxXml *sax = (FS_SaxXml *)hsax;
	FS_SINT4 linklen = 0;
	FS_SINT4 datalen;
	FS_BYTE *tmp;

	sax->data_finish = finish;
	if( data == FS_NULL || dlen <= 0 )
	{
		if( finish && ! sax->running )
			FS_SaxProcXmlDoc( hsax );
		return;
	}

	sax->pend_request = FS_FALSE;
	if( sax->link_pos )
		linklen = sax->buf_len - (sax->link_pos - sax->buf);
	datalen = dlen + linklen;
	tmp = IFS_Malloc( datalen + 1 );
	if( tmp )
	{
		if( linklen > 0 )
			IFS_Memcpy( tmp, sax->link_pos, linklen );
		IFS_Memcpy( tmp + linklen, data, dlen );
		tmp[datalen] = 0;
		
		if( sax->buf )
			IFS_Free( sax->buf );
		sax->buf = tmp;
		sax->data = tmp;
		sax->link_pos = tmp;
		sax->buf_len = datalen;
		/* restart process xml document */
		if( ! sax->running )
			FS_SaxProcXmlDoc( hsax );
	}
	else
	{
		if( sax->buf )
			IFS_Free( sax->buf );
		sax->buf = FS_NULL;
		sax->buf_len = 0;
		sax->data = FS_NULL;
		sax->link_pos = FS_NULL;
	}
}

void FS_SaxSetStartElementHandler( FS_SaxHandle hsax, FS_SaxTextHandler handler )
{
	FS_SaxXml *sax = (FS_SaxXml *)hsax;
	sax->start_element = handler;
}

void FS_SaxSetStartElementEndHandler( FS_SaxHandle hsax, FS_SaxTextHandler handler )
{
	FS_SaxXml *sax = (FS_SaxXml *)hsax;
	sax->start_element_end = handler;
}

void FS_SaxSetEndElementHandler( FS_SaxHandle hsax, FS_SaxTextHandler handler )
{
	FS_SaxXml *sax = (FS_SaxXml *)hsax;
	sax->end_element = handler;
}

void FS_SaxSetElementTextHandler( FS_SaxHandle hsax, FS_SaxElementTextHandler handler )
{
	FS_SaxXml *sax = (FS_SaxXml *)hsax;
	sax->element_text = handler;
}

void FS_SaxSetAttributeHandler( FS_SaxHandle hsax, FS_SaxAttrHandler handler )
{
	FS_SaxXml *sax = (FS_SaxXml *)hsax;
	sax->attribute = handler;
}

void FS_SaxSetXmlNoteHandler( FS_SaxHandle hsax, FS_SaxXmlNoteHandler handler )
{
	FS_SaxXml *sax = (FS_SaxXml *)hsax;
	sax->xml_note = handler;
}

void FS_SaxSetXmlInstructionHandler( FS_SaxHandle hsax, FS_SaxTextHandler handler )
{
	FS_SaxXml *sax = (FS_SaxXml *)hsax;
	sax->xml_instruction = handler;
}

void FS_SaxSetCommentHandler( FS_SaxHandle hsax, FS_SaxElementTextHandler handler )
{
	FS_SaxXml *sax = (FS_SaxXml *)hsax;
	sax->comment = handler;
}

void FS_SaxProcXmlDoc( FS_SaxHandle hsax )
{
	FS_SaxXml *sax = (FS_SaxXml *)hsax;
	FS_BYTE b, flag;
	
	sax->running = FS_TRUE;
	sax->can_access = FS_TRUE;
	b = FS_SaxReadByte( sax );
	if( b == 0 )
	{
		sax->can_access = FS_TRUE;
		b = FS_SaxReadByte( sax );
		if( b == 0 )
		{
			if( sax->data_finish && sax->complete )
			{
				sax->complete( sax->user_data, sax );
				return;
			}
		}
	}
	
	while( b )
	{
		if( b == '<' )
		{
			FS_SaxLinkPos( sax, -1 );
			b = FS_SaxReadByte( sax );
			if( b == '?' )
			{
				if( IFS_Strnicmp( sax->data, "xml", 3 ) == 0 )	/* xml note */
				{
					sax->data += 3;
					FS_SaxXmlNote( sax );
				}
				else	/* xml instruction */
				{
					FS_SaxXmlInstruction( sax );
				}
			}
			else if( b == '!' )
			{
				if( IFS_Strnicmp( sax->data, "--", 2 ) == 0 )	/* comment */
				{
					sax->data += 2;
					FS_SaxXmlComment( sax );
				}
				else if( IFS_Strnicmp( sax->data, "[CDATA[", 7 ) == 0 )		/* cdata */
				{
					sax->data += 7;
					FS_SaxXmlCData( sax );
				}
				else if( IFS_Strnicmp( sax->data, "DOCTYPE", 7 ) == 0 )
				{
					sax->data += 7;
					FS_SaxXmlDocType( sax );
				}
				else	/* ignore unknow element */
					FS_SaxRunOutElement( sax );
			}
			else if( b == '/' )		/* element end */
			{
				sax->state = FS_SAX_E_NONE;
				FS_SaxXmlEndElement( sax );
				if( FS_SaxTopElement(sax) )
					sax->state = FS_SAX_E_BODY;
			}
			else if( b != 0 )		/* xml element start */
			{
				sax->data --;		/* rollback one byte */
				sax->state = FS_SAX_E_HEAD;
				FS_SaxXmlStartElement( sax );
			}
		}
		else if( b == '/' && sax->state != FS_SAX_E_BODY )			/* element end */
		{
			sax->state = FS_SAX_E_NONE;
			FS_SaxXmlEndElement( sax );
			if( FS_SaxTopElement(sax) )
				sax->state = FS_SAX_E_BODY;
		}
		else if( b == '>' )			/* element head end */
		{
			FS_SaxXmlStartElementEnd( sax );
			sax->state = FS_SAX_E_BODY;
		}
		else
		{
			if( FS_SAX_E_HEAD == sax->state )	/* attribute or element end*/
			{
				sax->data --;
				FS_SaxSkipWhiteSpace( sax );
				flag = FS_SaxGetByte( sax );
				if( flag != '/' && flag != '>' && flag != 0 )	/* attribute */
				{
					FS_SaxXmlAttribute( sax );
				}
				else if( flag == '/' )			/* element end */
				{
					sax->state = FS_SAX_E_NONE;
					sax->data ++;
					FS_SaxXmlEndElement( sax );
					if( FS_SaxTopElement(sax) )
						sax->state = FS_SAX_E_BODY;
				}
			}
			else if( FS_SAX_E_BODY == sax->state )	/* may be element body text or child element */
			{
				sax->data --;
				FS_SaxSkipWhiteSpace( sax );
				flag = FS_SaxGetByte( sax );
				if( flag != '<' && flag != 0 )		/* body text */
				{
					FS_SaxXmlElementText( sax );
				}
			}
			else
			{
				/* sax run to element start position */
				while( (b = FS_SaxReadByte( sax )) && b != '<' )
					;	/* empty here */
				if( b == '<' )
					sax->data --;
			}
		}

		sax->can_access = FS_TRUE;
		b = FS_SaxReadByte( sax );
		if( b == 0 )
		{
			sax->can_access = FS_TRUE;
			b = FS_SaxReadByte( sax );
			if( b == 0 )
			{
				if( sax->data_finish && sax->complete )
				{
					sax->complete( sax->user_data, sax );
					return;
				}
			}
		}
	}
	
	sax->running = FS_FALSE;
}

#ifdef FS_DEBUG_
#define FS_SAX_TEST_BLOCK

#include "inc/util/FS_File.h"

#define FS_SAX_TR	"sax_out.xml"
static FS_CHAR GFS_XmlFile[64];
static FS_SINT4 GFS_Offset = 0;
static FS_SINT4 GFS_FileOffset = 0;

/* test functon */
static void FS_SaxTestStartElementHandler( void * userData, FS_CHAR *str )
{
	static FS_CHAR text[1024];
	FS_SINT4 tlen;
	IFS_Sprintf( text, "<%s", str );
	tlen = IFS_Strlen( text );
	FS_FileWrite( FS_DIR_ROOT, FS_SAX_TR, GFS_Offset, text, tlen );
	GFS_Offset += tlen;
}

static void FS_SaxTestElementStartEndHandler( void * userData, FS_CHAR *str )
{
	static FS_CHAR text[1024];
	FS_SINT4 tlen;
	IFS_Sprintf( text, ">\r\n", str );
	tlen = IFS_Strlen( text );
	FS_FileWrite( FS_DIR_ROOT, FS_SAX_TR, GFS_Offset, text, tlen );
	GFS_Offset += tlen;
}

static void FS_SaxTestElementEndHandler( void * userData, FS_CHAR *str )
{
	static FS_CHAR text[1024];
	FS_SINT4 tlen;
	IFS_Sprintf( text, "</%s>\r\n", str );
	tlen = IFS_Strlen( text );
	FS_FileWrite( FS_DIR_ROOT, FS_SAX_TR, GFS_Offset, text, tlen );
	GFS_Offset += tlen;
}

static void FS_SaxTestElementTextHandler( void * userData,  FS_CHAR *element, FS_CHAR *str, FS_SINT4 slen )
{
	FS_FileWrite( FS_DIR_ROOT, FS_SAX_TR, GFS_Offset, str, slen );
	GFS_Offset += slen;
	FS_FileWrite( FS_DIR_ROOT, FS_SAX_TR, GFS_Offset, "\r\n", 2 );
	GFS_Offset += 2;
}

static void FS_SaxTestCommentHandler( void * userData,	FS_CHAR *element, FS_CHAR *str, FS_SINT4 slen )
{
	static FS_CHAR text[1024];
	FS_SINT4 tlen;
	IFS_Sprintf( text, "<!--\r\n", element );
	tlen = IFS_Strlen( text );
	FS_FileWrite( FS_DIR_ROOT, FS_SAX_TR, GFS_Offset, text, tlen );
	GFS_Offset += tlen;
	FS_FileWrite( FS_DIR_ROOT, FS_SAX_TR, GFS_Offset, str, slen );
	GFS_Offset += slen;
	FS_FileWrite( FS_DIR_ROOT, FS_SAX_TR, GFS_Offset, "\r\n--", 4 );
	GFS_Offset += 4;
}

static void FS_SaxTestAttrHandler( void * userData, FS_CHAR *element, FS_CHAR *name, FS_CHAR *value )
{
	static FS_CHAR text[1024];
	FS_SINT4 tlen;
	IFS_Sprintf( text, " %s = \"%s\"", name, value );
	tlen = IFS_Strlen( text );
	FS_FileWrite( FS_DIR_ROOT, FS_SAX_TR, GFS_Offset, text, tlen );
	GFS_Offset += tlen;
}

static void FS_SaxTestXmlNoteHandler( void * userData, FS_CHAR *version, FS_CHAR *encoding )
{
	static FS_CHAR text[1024];
	FS_SINT4 tlen;
	if( version == FS_NULL )
		version = "1.0";
	if( encoding == FS_NULL )
		encoding = "US-ASCII";
	
	IFS_Sprintf( text, "<?xml version = '%s' encoding = '%s' ?>\r\n", version, encoding );
	tlen = IFS_Strlen( text );
	FS_FileWrite( FS_DIR_ROOT, FS_SAX_TR, GFS_Offset, text, tlen );
	GFS_Offset += tlen;
}

void FS_SaxTestDataRequest( void *userData, FS_SaxHandle hsax )
{
	FS_SINT4 size, rlen;
	FS_BYTE *buf;
	FS_BOOL bDone = FS_FALSE;
	IFS_Strcpy( GFS_XmlFile, "test.xml" );
	size = FS_FileGetSize( FS_DIR_ROOT, GFS_XmlFile );
	if( size < 0 )
	{
		IFS_Strcpy( GFS_XmlFile, "test.html" );
		size = FS_FileGetSize( FS_DIR_ROOT, GFS_XmlFile );
	}
	
	if( size > 0 )
	{
		buf = IFS_Malloc( size );
		#ifdef FS_SAX_TEST_BLOCK
		rlen = FS_FileRead( FS_DIR_ROOT, GFS_XmlFile, GFS_FileOffset, buf, 2048 );
		#else
		rlen = FS_FileRead( FS_DIR_ROOT, GFS_XmlFile, 0, buf, size );
		#endif
		GFS_FileOffset += rlen;
		if( GFS_FileOffset >= size )
			bDone = FS_TRUE;
		FS_SaxDataFeed( hsax, buf, rlen, bDone );
		IFS_Free( buf );
	}
}

void FS_SaxTestComplete( void *userData, FS_SaxHandle hsax )
{
	FS_FreeSaxHandler( hsax );
}

void FS_SaxTest( void )
{
	FS_SaxHandle hsax;	
	hsax = FS_CreateSaxHandler( FS_NULL );
	FS_SaxSetDataRequest( hsax, FS_SaxTestDataRequest );
	FS_SaxSetStartElementHandler( hsax, FS_SaxTestStartElementHandler );
	FS_SaxSetStartElementEndHandler( hsax, FS_SaxTestElementStartEndHandler );
	FS_SaxSetEndElementHandler( hsax, FS_SaxTestElementEndHandler );
	FS_SaxSetElementTextHandler( hsax, FS_SaxTestElementTextHandler );
	FS_SaxSetCommentHandler( hsax, FS_SaxTestCommentHandler );
	FS_SaxSetAttributeHandler( hsax, FS_SaxTestAttrHandler );
	FS_SaxSetXmlNoteHandler( hsax, FS_SaxTestXmlNoteHandler );
	FS_SaxSetCompleteHandler( hsax, FS_SaxTestComplete );
	GFS_Offset = 0;
	GFS_FileOffset = 0;
	FS_SaxProcXmlDoc( hsax );
}

#endif
