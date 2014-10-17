#include "inc/FS_Config.h"
#include "inc/inte/FS_ISocket.h"

/* wsp session event */
#define FS_WSP_EV_CONNECT_CNF		1
#define FS_WSP_EV_RESULT_IND		2	/* param is FS_WspResultData struct pointer */
#define FS_WSP_EV_DISCONNECT_IND	3
#define FS_WSP_EV_SUSPEND_IND		4
#define FS_WSP_EV_RESUME_IND		5
#define FS_WSP_EV_PUSH_IND			6	/* param is FS_WspPushData struct pointer */
#define FS_WSP_EV_INVOKE_IND		7

#define FS_WSP_ERR_NET				-1
#define FS_WSP_ERR_MEMORY			-2
#define FS_WSP_ERR_UNKNOW			-3
#define FS_WSP_ERR_USER_ABORT		-4

/* wsp status code */
#define FS_WSP_STATUS_OK			0x20
#define FS_WSP_STATUS_MOVED_PERM	0x31
#define FS_WSP_STATUS_MOVED_TEMP	0x32
#define FS_WSP_STATUS_SEE_OTHER		0x33

typedef void * FS_WspHandle;

typedef void ( * FS_WspEventFunc)( void *user_data, FS_WspHandle hWsp, FS_SINT4 event, FS_UINT4 param );

typedef struct FS_WspResultData_Tag
{
	FS_BYTE *		data;
	FS_SINT4		len;
	FS_BYTE			status;
	FS_UINT2		content_type;	/* encoding format content type */
	FS_CHAR *		mime_content;	/* string format content type */
	FS_CHAR *		location;
	FS_BOOL 		done;
}FS_WspResultData;

typedef struct FS_WspPostDataStruct_Tag
{
	FS_CHAR *		content_type;
	FS_UINT1		int_enc_content;		/* integer encoding content type */
	FS_CHAR *		data;
	FS_SINT4		data_len;
	FS_BOOL			data_is_file;			/* is true, data is a abs file name */
	FS_CHAR *		referer;				/* if not NULL. will sned referer header */
}FS_WspPostDataStruct;

typedef struct FS_WspPushData_Tag
{
	FS_UINT1		tid;
	FS_UINT1		app_id_code;
	FS_CHAR *		app_id_value;
	FS_UINT2		content_type;	/* encoding format content type */
	FS_CHAR *		mime_content;	/* string format content type */
	FS_BYTE *		data;
	FS_SINT4		len;
}FS_WspPushData;

FS_WspHandle FS_WspCreateHandle( FS_SockAddr *addr, void *user_data, FS_WspEventFunc handler );

void FS_WspPushInd( FS_BYTE *pdu, FS_SINT4 len, FS_WspEventFunc evFunc );

void FS_WspGetReq( FS_WspHandle hWsp, FS_CHAR *url );

void FS_WspPostReq( FS_WspHandle hWsp, FS_CHAR *url, FS_WspPostDataStruct *postData );

void FS_WspAbortReq( FS_WspHandle hWsp, FS_BOOL callback );

void FS_WspResumeReq( FS_WspHandle hWsp );

void FS_WspSuspendReq( FS_WspHandle hWsp );

void FS_WspDestroyHandle( FS_WspHandle hWsp );

