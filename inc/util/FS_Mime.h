#ifndef _FS_MIME_H_
#define _FS_MIME_H_

#include "inc\FS_Config.h"
#include "inc\util\FS_List.h"

#define FS_MIME_TYPE_NAME_LEN	64
/* mime header field max len */
#define FS_MIME_HEAD_FIELD_MAX_LEN		(8 * 1024)

/* wsp content type */
#define FS_WCT_TEXT_HTML			0x02
#define FS_WCT_TEXT_PLAIN			0x03
#define FS_WCT_TEXT_VCALENDAR		0x06
#define FS_WCT_TEXT_VCARD			0x07
#define FS_WCT_TEXT_WML 			0x08
#define FS_WCT_TEXT_WMLSCRIPT		0x09

#define FS_WCT_APP_WMLC 			0x14
#define FS_WCT_APP_WMLSCRIPTC		0x15
#define FS_WCT_APP_OCTET			0x10
#define FS_WCT_APP_FORM_URLENCODE	0x12

#define FS_WCT_IMAGE_GIF			0x1D
#define FS_WCT_IMAGE_JPEG			0x1E
#define FS_WCT_IMAGE_PNG			0x20
#define FS_WCT_IMAGE_WBMP			0x21

#define FS_WCT_TEXT_VND_WAP_SI		0x2D
#define FS_WCT_APP_VND_WAP_SIC		0x2E
#define FS_WCT_TEXT_VND_WAP_SL		0x2F
#define FS_WCT_APP_VND_WAP_SLC		0x30

#define FS_WCT_APP_VND_WAP_MMS						0x3E
#define FS_WCT_APP_VND_WAP_MULTIPART_RELATED		0x33
#define FS_WCT_APP_VND_WAP_MULTIPART_MIXED			0x23

typedef struct FS_HeadField_Tag
{
	FS_CHAR *name;
	FS_CHAR *value;
}FS_HeadField;


typedef enum FS_MimeMediaType_Tag
{
	FS_MIME_UNKNOW = 0,
	FS_MIME_TEXT = 0x01,
	FS_MIME_IMAGE = 0x02,
	FS_MIME_AUDIO = 0x04,
	FS_MIME_VIDEO = 0x08,
	FS_MIME_APPLICATION = 0x10,
	FS_MIME_MESSAGE = 0x20,
	FS_MIME_MULTIPART = 0x40
}FS_MimeMediaType;

typedef enum FS_EncodingType_Tag
{
	FS_ENC_UNKNOW = 0,
	FS_ENC_7BIT,
	FS_ENC_8BIT,
	FS_ENC_BINARY,
	FS_ENC_BASE64,
	FS_ENC_QUOTED_PRINTABLE
}FS_EncodeType;

typedef enum FS_DispositionType_Tag
{
	FS_DPS_UNKNOW = 0,
	FS_DPS_INLINE,
	FS_DPS_ATTACHMENT
}FS_DispositionType;

typedef struct FS_MimeParam_Tag
{
	FS_List			list;
	FS_CHAR *		name;
	FS_CHAR *		val;
}FS_MimeParam;

typedef struct FS_MimeEntry_Tag
{
	FS_List							list;
	FS_CHAR *						disp_name;
	FS_CHAR *						file_name;
	FS_BOOL							temp_file;
	FS_SINT4						offset;
	FS_SINT4						length;
	FS_MimeMediaType				type;
	FS_CHAR *						subtype;
	
	FS_EncodeType					encode;
	FS_DispositionType				disposition;
	FS_List							params;		// store FS_MimeParam structure
}FS_MimeEntry;

FS_SINT4 FS_GetTypeFromMime( FS_CHAR *mime );

FS_SINT4 FS_GetTypeFromMimeCode( FS_UINT2 code );

FS_CHAR * FS_GetMimeFromExt( FS_CHAR *filename );

FS_CHAR * FS_GetExtFromMime( FS_CHAR *mime );

FS_CHAR * FS_GetMimeFromCode( FS_UINT2 code );

FS_UINT2 FS_GetMimeCodeFromMime( FS_CHAR *mime );

FS_UINT2 FS_GetMimeCodeFromExt( FS_CHAR *ext );

FS_CHAR * FS_GetExtFromMimeCode( FS_UINT2 code );

FS_MimeMediaType FS_GetMimeMediaType( FS_CHAR *str );

FS_EncodeType FS_GetEncodeType( FS_CHAR *str );

void FS_FreeMimeEntry( FS_MimeEntry *entry );

FS_MimeEntry * FS_NewMimeEntry( void );

FS_CHAR * FS_GetParam( FS_List *params, FS_CHAR *name );

FS_CHAR * FS_GenMimeTypeName( FS_CHAR * mime, FS_MimeMediaType type, FS_CHAR * subtype );

/*---------------------------------MIME parser utilities---------------------------------------*/
FS_SINT4 FS_GetOneField( FS_CHAR *buf, FS_SINT4 len, FS_CHAR ** data, FS_HeadField *hentry );

void FS_ProcessParams( FS_List *paramList, FS_CHAR * str );

void FS_ProcessContentType( FS_MimeEntry *entry, FS_CHAR * str );

void FS_InsertParams( FS_List *paramList, FS_MimeParam *params );

#define FS_IS_BOUNDARY(s, bnd, len) \
	(bnd && s[0] == '-' && s[1] == '-' && ! IFS_Strncmp(s + 2, bnd, len))

#endif
