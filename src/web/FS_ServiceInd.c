#include "inc/FS_Config.h"

#ifdef FS_MODULE_WEB

#include "inc/FS_Config.h"
#include "inc/web/FS_Wbxml.h"
#include "inc/web/FS_Push.h"
#include "inc/util/FS_Charset.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_MemDebug.h"

/* Tag Tokens */
#define FS_SITAG_SI				0x05
#define FS_SITAG_INDICATION		0x06
#define FS_SITAG_INFO			0x07
#define FS_SITAG_ITEM			0x08

/* Attribute Start Tokens */
#define FS_SIATTR_ACTION_SIGNAL_NONE			0x05
#define FS_SIATTR_ACTION_SIGNAL_LOW				0x06
#define FS_SIATTR_ACTION_SIGNAL_MEDIUM			0x07
#define FS_SIATTR_ACTION_SIGNAL_HIGH			0x08
#define FS_SIATTR_ACTION_DELETE					0x09
#define FS_SIATTR_CREATED						0x0A
#define FS_SIATTR_HREF							0x0B
#define FS_SIATTR_HREF_HTTP						0x0C
#define FS_SIATTR_HREF_HTTP_WWW					0x0D
#define FS_SIATTR_HREF_HTTPS 					0x0E
#define FS_SIATTR_HREF_HTTPS_WWW				0x0F
#define FS_SIATTR_SI_EXPIRES 					0x10
#define FS_SIATTR_SI_ID		 					0x11
#define FS_SIATTR_CLASS		 					0x12

/* Attribute Value Tokens */
#define FS_SIATTRVAL_COM						0x85	/* .com/ */
#define FS_SIATTRVAL_EDU 						0x86	/* .edu/ */
#define FS_SIATTRVAL_NET 						0x87	/* .net/ */
#define FS_SIATTRVAL_ORG						0x88	/* .org/ */

static FS_WbxmlAttrVal GFS_SIAttrValTable [] =
{
	{ FS_SIATTRVAL_COM, ".com/" },
	{ FS_SIATTRVAL_EDU, ".edu/" },
	{ FS_SIATTRVAL_NET, ".net/" },
	{ FS_SIATTRVAL_ORG, ".org/" },
	{ 0, FS_NULL }
};

FS_CHAR *FS_SIAttrValue( FS_UINT1 tok )
{
	FS_UINT1 i = 0;
	
	while( GFS_SIAttrValTable[i].value )
	{
		if( GFS_SIAttrValTable[i].value == tok )
			return GFS_SIAttrValTable[i].string;
		i ++;
	}
	return FS_NULL;
}

#if 0
void FS_SIAttrTokenValue( FS_BYTE *out, FS_SINT4 len, FS_List *pTokList )
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

		if( pTok->value_type == FS_WBXML_TOK_ATTR_VALUE && (str = FS_SIAttrValue((FS_UINT1)pTok->value)) )
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
		/* ignore other type value */
	}

	out[offset] = 0;
}
#endif

static FS_SIWbxmlHandler( void *user_data, FS_UINT1 event, FS_UINT4 param )
{
	static FS_UINT4 s_charset;
	FS_PushMsg *pMsg = (FS_PushMsg *)user_data;
	FS_WbxmlAttr *pAttr;
	FS_WbxmlContent *pCnt;
	FS_CHAR *str = FS_NULL, subject[256];
	
	if( event == FS_WBXML_EV_ATTR && param )
	{
		pAttr = ( FS_WbxmlAttr * )param;
		if( pAttr->stag == FS_SITAG_INDICATION )
		{
			/* priority */
			if( pAttr->attr_start == FS_SIATTR_ACTION_SIGNAL_LOW )
			{
				FS_PUSHMSG_SET_FLAG( pMsg, FS_PUSHMSG_PRI_LOW );
			}
			else if( pAttr->attr_start == FS_SIATTR_ACTION_SIGNAL_MEDIUM )
			{
				FS_PUSHMSG_SET_FLAG( pMsg, FS_PUSHMSG_PRI_MEDIUM );
			}
			else if( pAttr->attr_start == FS_SIATTR_ACTION_SIGNAL_HIGH )
			{
				FS_PUSHMSG_SET_FLAG( pMsg, FS_PUSHMSG_PRI_HIGH );
			}
			/* href */
			else if( pAttr->attr_start == FS_SIATTR_HREF )
			{
				FS_WbxmlAttrTokenValue( pMsg->url, sizeof(pMsg->url), &pAttr->attr_value, FS_SIAttrValue );
			}
			else if( pAttr->attr_start == FS_SIATTR_HREF_HTTP )
			{
				IFS_Strcpy( pMsg->url, "http://" );
				FS_WbxmlAttrTokenValue( pMsg->url + 7, sizeof(pMsg->url) - 7, &pAttr->attr_value, FS_SIAttrValue );
			}
			else if( pAttr->attr_start == FS_SIATTR_HREF_HTTP_WWW )
			{
				IFS_Strcpy( pMsg->url, "http://www." );
				FS_WbxmlAttrTokenValue( pMsg->url + 11, sizeof(pMsg->url) - 11, &pAttr->attr_value, FS_SIAttrValue );
			}
			else if( pAttr->attr_start == FS_SIATTR_HREF_HTTPS )
			{
				IFS_Strcpy( pMsg->url, "https://" );
				FS_WbxmlAttrTokenValue( pMsg->url + 8, sizeof(pMsg->url) - 8, &pAttr->attr_value, FS_SIAttrValue );
			}
			else if( pAttr->attr_start == FS_SIATTR_HREF_HTTPS_WWW )
			{
				IFS_Strcpy( pMsg->url, "https://www." );
				FS_WbxmlAttrTokenValue( pMsg->url + 12, sizeof(pMsg->url) - 12, &pAttr->attr_value, FS_SIAttrValue );
			}			
		}
	}
	else if( event == FS_WBXML_EV_CONTENT && param )
	{
		pCnt = (FS_WbxmlContent *)param;
		if( pCnt->stag == FS_SITAG_INDICATION )
		{
			subject[0] = 0;
			FS_WbxmlAttrTokenValue( subject, sizeof(subject), &pCnt->content, FS_SIAttrValue );
			if( s_charset == 106 )
				str = FS_ProcessCharset( subject, -1, "UTF-8", FS_NULL );
			if( str == FS_NULL ) str = subject;
			IFS_Strncpy( pMsg->subject, str, sizeof(pMsg->subject) - 1 );
			if( str != subject ) IFS_Free( str );
		}
	}
	else if( event == FS_WBXML_EV_CHARSET )
	{
		s_charset = param;
	}
}

void FS_SIProcessData( FS_BYTE *data, FS_SINT4 len )
{
	FS_PushMsg *pMsg = FS_NEW( FS_PushMsg );
	if( pMsg )
	{
		IFS_Memset( pMsg, 0, sizeof(FS_PushMsg) );
		FS_WbxmlProcessData( data, len, FS_SIWbxmlHandler, pMsg );
		if( pMsg->url[0] )
		{
			FS_PUSHMSG_SET_FLAG( pMsg, FS_PUSHMSG_TYPE_SI );
			IFS_GetDateTime( &pMsg->date );
			FS_PushAddItem( pMsg );
		}
		else
		{
			IFS_Free( pMsg );
		}
	}
}

#endif

