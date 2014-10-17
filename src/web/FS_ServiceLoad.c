#include "inc/FS_Config.h"

#ifdef FS_MODULE_WEB

#include "inc/FS_Config.h"
#include "inc/web/FS_Wbxml.h"
#include "inc/web/FS_Push.h"
#include "inc/util/FS_Charset.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_MemDebug.h"

#define FS_SLTAG_SL 							0x05

#define FS_SLATTR_ACTION_EXCUTE_LOW 			0x05
#define FS_SLATTR_ACTION_EXCUTE_HIGH			0x06
#define FS_SLATTR_ACTION_CACHE					0x07
#define FS_SLATTR_HREF							0x08
#define FS_SLATTR_HREF_HTTP 					0x09
#define FS_SLATTR_HREF_HTTP_WWW 				0x0A
#define FS_SLATTR_HREF_HTTPS					0x0B
#define FS_SLATTR_HREF_HTTPS_WWW				0x0C

extern FS_CHAR * FS_SIAttrValue( FS_UINT1 tok );

static FS_SLWbxmlHandler( void *user_data, FS_UINT1 event, FS_UINT4 param )
{
	static FS_UINT4 s_charset;
	FS_WbxmlAttr *pAttr;
	FS_WbxmlContent *pCnt;
	FS_CHAR *str = FS_NULL, subject[256];
	FS_PushMsg *pMsg = (FS_PushMsg *)user_data;
	
	if( event == FS_WBXML_EV_ATTR && param )
	{
		pAttr = ( FS_WbxmlAttr * )param;
		if( pAttr->stag == FS_SLTAG_SL )
		{
			/* priority */
			if( pAttr->attr_start == FS_SLATTR_ACTION_EXCUTE_LOW )
			{
				FS_PUSHMSG_SET_FLAG( pMsg, FS_PUSHMSG_PRI_LOW );
			}
			else if( pAttr->attr_start == FS_SLATTR_ACTION_EXCUTE_HIGH )
			{
				FS_PUSHMSG_SET_FLAG( pMsg, FS_PUSHMSG_PRI_HIGH );
			}
			else if( pAttr->attr_start == FS_SLATTR_ACTION_CACHE )
			{
				FS_PUSHMSG_SET_FLAG( pMsg, FS_PUSHMSG_PRI_NONE );
			}
			/* href */
			else if( pAttr->attr_start == FS_SLATTR_HREF )
			{
				FS_WbxmlAttrTokenValue( pMsg->url, sizeof(pMsg->url), &pAttr->attr_value, FS_SIAttrValue );
			}
			else if( pAttr->attr_start == FS_SLATTR_HREF_HTTP )
			{
				IFS_Strcpy( pMsg->url, "http://" );
				FS_WbxmlAttrTokenValue( pMsg->url + 7, sizeof(pMsg->url) - 7, &pAttr->attr_value, FS_SIAttrValue );
			}
			else if( pAttr->attr_start == FS_SLATTR_HREF_HTTP_WWW )
			{
				IFS_Strcpy( pMsg->url, "http://www." );
				FS_WbxmlAttrTokenValue( pMsg->url + 11, sizeof(pMsg->url) - 11, &pAttr->attr_value, FS_SIAttrValue );
			}
			else if( pAttr->attr_start == FS_SLATTR_HREF_HTTPS )
			{
				IFS_Strcpy( pMsg->url, "https://" );
				FS_WbxmlAttrTokenValue( pMsg->url + 8, sizeof(pMsg->url) - 8, &pAttr->attr_value, FS_SIAttrValue );
			}
			else if( pAttr->attr_start == FS_SLATTR_HREF_HTTPS_WWW )
			{
				IFS_Strcpy( pMsg->url, "https://www." );
				FS_WbxmlAttrTokenValue( pMsg->url + 12, sizeof(pMsg->url) - 12, &pAttr->attr_value, FS_SIAttrValue );
			}
		}
	}
	else if( event == FS_WBXML_EV_CONTENT && param )
	{
		pCnt = (FS_WbxmlContent *)param;
		if( pCnt->stag == FS_SLTAG_SL )
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

void FS_SLProcessData( FS_BYTE *data, FS_SINT4 len )
{
	FS_PushMsg *pMsg = FS_NEW( FS_PushMsg );
	if( pMsg )
	{
		IFS_Memset( pMsg, 0, sizeof(FS_PushMsg) );
		FS_WbxmlProcessData( data, len, FS_SLWbxmlHandler, pMsg );
		if( pMsg->url[0] )
		{
			FS_PUSHMSG_SET_FLAG( pMsg, FS_PUSHMSG_TYPE_SL );
			IFS_GetDateTime( &pMsg->date );
			FS_PushAddItem( pMsg );
		}
		else
		{
			IFS_Free( pMsg );
		}
		pMsg = FS_NULL;
	}
}

#endif	//FS_MODULE_WEB


