#ifndef _FS_NET_CONN_H_
#define _FS_NET_CONN_H_
#include "inc/FS_Config.h"

typedef void (* FS_NetConnCallback )( void *user_data, FS_BOOL ok );

void FS_NetDisconnect( FS_UINT4 app );

void FS_NetConnect( FS_CHAR *apn, FS_CHAR *user, FS_CHAR *pass, FS_NetConnCallback callback, 
	FS_UINT4 app, FS_BOOL highPri, void *user_data );

#endif
