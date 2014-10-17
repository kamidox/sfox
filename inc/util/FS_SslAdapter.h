#ifndef _FS_I_SSL_H_
#define _FS_I_SSL_H_
#include "inc/FS_Config.h"

typedef enum FS_SslEvent_Tag
{
	FS_SSL_EV_HANDSHAKE,		// do handshake
	FS_SSL_EV_CONNECTED, 		// connected. handshake finish
	FS_SSL_EV_SEND_OK,			// data send complete
	FS_SSL_EV_DATA_ARRIVE,		// data arrive inform( FS_SslBuf )
	FS_SSL_EV_CLOSE, 			// ssl is closed, or remote host is close the connection
	FS_SSL_EV_ERROR				// ssl error
}FS_SslEvent;

#define FS_SSL_ERR_NETWORK		1
#define FS_SSL_ERR_MEMORY		2
#define FS_SSL_ERR_UNKNOW		3

typedef struct FS_SslBuf_Tag
{
	FS_BYTE *		buf;
	FS_SINT4		len;
}FS_SslBuf;

typedef void * FS_SslHandle;

typedef void ( *FS_SslEvHandler)( FS_SslHandle ssl, FS_SslEvent ev, FS_UINT4 param );

FS_SslHandle FS_SslCreate( FS_SslEvHandler callback );

/* act a ssl handshake. caller must connect to server */
FS_BOOL FS_SslConnect( FS_SslHandle hSsl, FS_CHAR *host, FS_UINT2 port );

FS_SINT4 FS_SslWrite( FS_SslHandle hSsl, FS_BYTE *buf, FS_SINT4 len );

void FS_SslClose( FS_SslHandle hSsl );

void FS_SslDestroy( FS_SslHandle hSsl );

#endif //_FS_I_SSL_H_
