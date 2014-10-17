#ifndef _FS_I_SOCKET_H_
#define _FS_I_SOCKET_H_

#include "inc\FS_Config.h"

/* defined to socket buffer len */
#define FS_SOCKET_BUF_LEN 		16384
#define FS_SOCKET_PKG_LEN		1400

typedef enum FS_ISockEvent_Tag
{
	FS_ISOCK_CONNECT,		// connected to host
	FS_ISOCK_WRITE,			// can send the data
	FS_ISOCK_SENDOK,		// data send confirm
	FS_ISOCK_READ,			// data arrive inform
	FS_ISOCK_CLOSE,			// socket is closed, or remote host is close the connection
	FS_ISOCK_ERROR			// socket error
}FS_ISockEvent;

typedef void ( *FS_ISockEvHandler)( FS_UINT4 sockid, FS_ISockEvent ev, FS_UINT4 param );

typedef struct FS_SockAddr_Tag
{
	FS_CHAR *		host;			/* may be dot format ip address or host name */
	FS_UINT2		port;
}FS_SockAddr;

/*
	init , can place dial up here
	@param bNeedProxy	TRUE, then dial up to proxy(CMWAP), or direct connect(CMNET)

	@remark
		when dial up success or failed, device must call FS_NetConnResultInd 
		to tell client the result of dail.
*/	
FS_BOOL IFS_NetConnect(   FS_CHAR *apn, FS_CHAR *user, FS_CHAR *pass  );

/*
	clean up, can place close dial up code here
*/	
void IFS_NetDisconnect( void );

/*
	create a socket return socket id
*/
FS_BOOL IFS_SocketCreate( FS_UINT4 *sockid, FS_BOOL bTCP, FS_ISockEvHandler handler );

/*
	host		remote host URL, may need DNS query
	port		port to connect
	handler		event handler, all this socket event will send to this handler
	NOTE:
	connected to host
	return socket id, to return -1 on error
*/	
FS_BOOL IFS_SocketConnect( FS_UINT4 sockid, FS_CHAR *host, FS_UINT2 port );

/*
	send data from socket, will be call after FS_ISOCK_CONNECT event is send
	after data is sended to network, must send FS_ISOCK_SENDOK to event handler
*/
FS_SINT4 IFS_SocketSend( FS_UINT4 sockid, FS_BYTE *buf, FS_SINT4 len );

/*
	send data throw UDP
*/
FS_SINT4 IFS_SocketSendTo( FS_UINT4 sockid, FS_BYTE *buf, FS_SINT4 len, FS_SockAddr *to );

/*
	recv data from socket, will be call after FS_ISOCK_READ event is send
	return readed bytes
*/	
FS_SINT4 IFS_SocketRecv( FS_UINT4 sockid, FS_BYTE *buf, FS_SINT4 len );

/*
	recv data from socket, will be call after FS_ISOCK_READ event is send
	return readed bytes
*/
FS_SINT4 IFS_SocketRecvFrom( FS_UINT4 sockid, FS_BYTE *buf, FS_SINT4 len, FS_SockAddr *from );

/*
	close a socket
*/	
FS_BOOL IFS_SocketClose( FS_UINT4 sockid );

#endif
