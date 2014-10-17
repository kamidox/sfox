#ifndef _FS_HTTP_H_
#define _FS_HTTP_H_

#include "inc/FS_Config.h"
#include "inc/util/FS_Mime.h"
#include "inc/inte/FS_ISocket.h" 

#define		FS_HTTP_ERR_OK				0
#define		FS_HTTP_ERR_NET				1
#define 	FS_HTTP_ERR_MEMORY			2
#define 	FS_HTTP_ERR_USER_CANCEL		3
#define		FS_HTTP_ERR_UNKNOW			4

#define		FS_HTTP_COUNTINUE					100
#define		FS_HTTP_SWICH_PROTOCOL				101

#define		FS_HTTP_OK							200

#define		FS_HTTP_MOVED						301
#define		FS_HTTP_FOUND						302
#define		FS_HTTP_SEE_OTHER					303
#define		FS_HTTP_NOT_MODIFIED				304

#define		FS_HTTP_INTERNAL_SERVER_ERROR		500

typedef void * FS_HttpHandle;

typedef struct FS_HttpHeader_Tag
{
	FS_CHAR *		content_type;
	FS_SINT4		content_length;

	FS_CHAR *		location;
	FS_CHAR *		cookies;
	FS_CHAR *		x_next_url;
	FS_List			params;		// store FS_MimeParam structure
}FS_HttpHeader;

typedef struct FS_HttpPostDataStruct_Tag
{
	FS_CHAR *		content_type;
	FS_CHAR *		data;
	FS_SINT4		data_len;
	FS_SINT4		offset;					/* inner use */
	FS_BOOL 		data_is_file;			/* is true, data is a abs file name */
}FS_HttpPostDataStruct;

typedef void ( * FS_HttpResponseStartFunc)( void *user_data, FS_HttpHandle hHttp, FS_SINT4 status, FS_HttpHeader *headers );

typedef void ( * FS_HttpResponseDataFunc)( void *user_data, FS_HttpHandle hHttp, FS_BYTE *data, FS_SINT4 data_len );

typedef void ( * FS_HttpResponseEndFunc)( void *user_data, FS_HttpHandle hHttp, FS_SINT4 error_code );

typedef void ( * FS_HttpRequestProgFunc)( void *user_data, FS_HttpHandle hHttp, FS_SINT4 offset );

FS_HttpHandle FS_HttpCreateHandle( void *userData, 
	FS_HttpResponseStartFunc start, FS_HttpResponseDataFunc data, FS_HttpResponseEndFunc end );

void FS_HttpDestroyHandle( FS_HttpHandle hHttp );

void FS_HttpRequest(FS_HttpHandle hHttp, FS_SockAddr *addr, FS_CHAR *method, 
	FS_CHAR *url, FS_HttpPostDataStruct *post_data, FS_CHAR *add_headers );

void FS_HttpRequestCancel(FS_HttpHandle hHttp, FS_BOOL callback );

void FS_HttpSetRequestProgFunc( FS_HttpHandle hHttp, FS_HttpRequestProgFunc progFunc );

#endif
