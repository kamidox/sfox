#ifndef _FS_WBXML_H_
#define _FS_WBXML_H_

#include "inc/FS_Config.h"
#include "inc/util/FS_List.h"

#define FS_WBXML_EV_VER 				0x01	
#define FS_WBXML_EV_PUBID				0x02	
#define FS_WBXML_EV_CHARSET				0x03	/* param is FS_UINT4, charser code */
#define FS_WBXML_EV_PI		 			0x04	/* param is FS_WbxmlAttr */
#define FS_WBXML_EV_STAG	 			0x05	/* param is FS_UINT4, tag name. may LITERAL tag */
#define FS_WBXML_EV_ATTR	 			0x06	/* param is FS_WbxmlAttr, tag name. may LITERAL tag */
#define FS_WBXML_EV_CONTENT	 			0x07	/* param is FS_WbxmlContent, tag name is nearest. may LITERAL tag */
#define FS_WBXML_EV_TAGEND				0x08	/* param is FS_UINT4, tag name. may LITERAL tag. tag end */

typedef void ( * FS_WbxmlHandler )( void *user_data, FS_UINT1 event, FS_UINT4 param );

#define FS_WBXML_TOK_UNDEF				0
#define FS_WBXML_TOK_ATTR_VALUE			1
#define FS_WBXML_TOK_STRING				2
#define FS_WBXML_TOK_ENTITY				3
#define FS_WBXML_TOK_OPAQUE				4
/* extention use its original value as token type. see WBXML-SPEC */
/* like EXT* EXT_I* EXT_T* */

/* 
	when value type is string, data is a string 
	when value type is a opaque, data is a opaque bytes, and value is opaque length
*/
typedef struct FS_WbxmlToken_Tag
{
	FS_List			list;
	FS_UINT1		value_type;
	FS_UINT4		value;
	FS_BYTE *		data;
}FS_WbxmlToken;

typedef struct FS_WbxmlAttr_Tag
{
	FS_UINT4		stag;
	FS_BYTE			attr_start;
	FS_List			attr_value;	/* FS_WbxmlToken */
}FS_WbxmlAttr;

typedef struct FS_WbxmlContent_Tag
{
	FS_UINT4		stag;
	FS_List			content;	/* FS_WbxmlToken */
}FS_WbxmlContent;

typedef struct FS_WbxmlAttrVal_Tag
{
	FS_UINT1			value;
	FS_CHAR *			string;
}FS_WbxmlAttrVal;

typedef FS_CHAR *( * FS_WbxmlAttrValueMapFunc )( FS_UINT1 code );

void FS_WbxmlAttrTokenValue( FS_BYTE *out, FS_SINT4 len, FS_List *pTokList, FS_WbxmlAttrValueMapFunc attrValueFunc );

void FS_WbxmlProcessData( FS_UINT1 *data, FS_SINT4 len, FS_WbxmlHandler handler, void *user_data );

void FS_WbxmlProcessFile( FS_SINT4 dir, FS_CHAR *file, FS_WbxmlHandler handler, void *user_data );

#endif
