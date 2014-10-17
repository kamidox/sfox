#ifndef _FS_MMS_NET_H_
#define _FS_MMS_NET_H_
#include "inc/FS_Config.h"

#define FS_MMS_NET_ERR_FILE		-1
#define FS_MMS_NET_ERR_BUSY		-2
#define FS_MMS_NET_ERR_NET		-3
#define FS_MMS_NET_ERR_MEMORY	-4
#define FS_MMS_NET_ERR_UNKNOW	-5
#define FS_MMS_NET_ERR_CONN 	-6

#define FS_MMS_NET_OK			0
#define FS_MMS_NET_CONNECTED	1
#define FS_MMS_NET_SENDING		2
#define FS_MMS_NET_RECVING		3
#define FS_MMS_NET_CONN_OK		4

typedef void ( * FS_MmsNetFunc ) ( void *user_data, FS_SINT4 ev, FS_UINT4 param );

void FS_MmsNetCancel( void );

FS_SINT4 FS_MmsSend( FS_CHAR *file, FS_MmsNetFunc callback, void *user_data );

FS_SINT4 FS_MmsRecv( FS_CHAR *url, FS_MmsNetFunc callback, void * user_data );

void FS_MmsSendReadReportMms( FS_CHAR *to, FS_CHAR *subject, FS_CHAR *content );

void FS_MmsSendReadReportPdu( FS_CHAR *msg_id, FS_CHAR *to, FS_BOOL bRead );


#endif

