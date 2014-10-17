#include "inc\inte\FS_ISocket.h"

#ifdef FS_PLT_WIN
#include <winsock2.h>
extern HWND GFS_HWND;
#define WM_SOCKET	(WM_USER + 1)
#define WM_SOCKET_SENDOK	(WM_USER + 3)

static int IFS_HostIsIPAddress( char *host );

#endif

static FS_ISockEvHandler GIFS_SockCb = FS_NULL;

#ifdef FS_PLT_VIENNA
#include "..\system\portab.h"
#include "..\system\sysprim.h"
#include "..\global\gsmerror.h"
#include "..\global\types.h"
#include "..\system\syslib.h"
#include "..\ui\uif\inet\socket.h"
#include "..\ui\uif\inet\dns.h"
#include "..\system\sysnet.h"
#include "..\ui\uif5\win\uiftimer.h"   /* timer support */
#include "..\amoi\mobileweb\AmoiGprs.h"

static UINT32 GIFS_LocalAddr = 0;
static UINT16 GIFS_Port = 0;
static FS_UINT4 GIFS_SockId = 0;
static BOOLEAN GIFS_Tcp = FALSE;

static void IFS_GprsCallBack( INT8 id , AmoiConnEvent event , UINT32 info)
{
	BOOLEAN ret = FALSE;
	switch(event)
	{
		case amoi_conn_ok:
			if(info != 0)
				GIFS_LocalAddr = info;
			ret = TRUE;
			break;
		case amoi_conn_error:
			GIFS_LocalAddr = 0;
			break;
		default:
			break;
	}
	FS_NetConnResultInd( ret );
}

static void IFS_SocketErrorHandler(INT16 sid, SOCKET_ERROR err)
{
	GIFS_SockCb( sid, FS_ISOCK_ERROR, (FS_UINT4)err );
}
//-------------------------------------------------------------------------------------------------
static void IFS_DataRecvHandler( INT16 sid, UINT16 len )
{
	GIFS_SockCb( sid, FS_ISOCK_READ, 0 );
}

static void IFS_DataSendHandler( INT16 sid, UINT16 len )
{
	GIFS_SockCb( sid, FS_ISOCK_SENDOK, 0 );
	GIFS_SockCb( sid, FS_ISOCK_WRITE, 0 );
}
//-------------------------------------------------------------------------------------------------
void IFS_SocketEventHandler( INT16 sock_id, SOCKET_EVENT env )
{
	switch( env )
	{
		case SOCK_EV_OPEN:
		{
			struct SockAddr_In sockaddr;
			sockaddr.sin_family = AF_INET;
			if( GIFS_Tcp )
				sockaddr.sin_port = GSMhtons( 0 );
			else
				sockaddr.sin_port = GSMhtons( GIFS_Port );
			sockaddr.sin_addr.s_addr = GSMhtonl(0); 
			if( ! SocketIsBound(sock_id) )
			{
				SocketBind( sock_id, (struct SockAddr *)(&sockaddr), sizeof( sockaddr ) );
			}
			break;
		}
		case SOCK_EV_BIND:
		{
			if( ! GIFS_Tcp )
			{
				GIFS_SockCb( sock_id, FS_ISOCK_CONNECT, 0 );
			}
			break;
		}
		case SOCK_EV_CONNECT:
			GIFS_SockCb( sock_id, FS_ISOCK_CONNECT, 0 );
			break;
		case SOCK_EV_CLOSE:
			GIFS_SockCb( sock_id, FS_ISOCK_CLOSE, 0 );
			break;
		default:
			break;
	}
}

void IFS_DNSCallBack( INT16 qid, INT16 status, struct HOSTENT * hp )
{
	if(0 == status) //²éÑ¯³É¹¦
	{
		struct SockAddr_In sock;
		sock.sin_family = AF_INET;
		sock.sin_port = GSMhtons( GIFS_Port );
		sock.sin_addr.s_addr = *((UINT32*)(hp->h_addr));
		SocketConnect( GIFS_SockId, (struct SockAddr *)(&sock), sizeof(sock));
	}
	else
	{
		GIFS_SockCb( GIFS_SockId, FS_ISOCK_ERROR, 0 );
	}
	
	if(hp != NULL)
		DNS_FreeHostent( hp );	
}

void IFS_DeferConnect( void *host )
{
	DNS_GetHostByName( host, IFS_DNSCallBack );
	GSMFree( host );
}

#endif

FS_BOOL IFS_NetConnect(   FS_CHAR *apn, FS_CHAR *user, FS_CHAR *pass  )
{
#ifdef FS_PLT_WIN
	WSADATA wsaData;
	FS_BOOL bRet;
	int ret;
	// Initialize Winsock version 2.2
	ret = WSAStartup( MAKEWORD(2,2), &wsaData );
	bRet = (ret == 0 ? FS_TRUE : FS_FALSE);
	FS_NetConnResultInd( bRet );
	return bRet;
#endif
#ifdef FS_PLT_VIENNA
	FS_BOOL mode;
	if( GSMstricmp(apn, "cmnet") == 0 )
		mode = FS_FALSE;
	else
		mode = FS_TRUE;
	AmoiCheckConnect( mode, IFS_GprsCallBack);
	return FS_TRUE;
#endif
}

void IFS_NetDisconnect( void )
{
#ifdef FS_PLT_WIN
	WSACleanup();
#endif
#ifdef FS_PLT_VIENNA
	AmoiDisconnect();
#endif
}

FS_BOOL IFS_SocketCreate( FS_UINT4 *sockid, FS_BOOL bTCP, FS_ISockEvHandler handler )
{
#ifdef FS_PLT_WIN
	FS_BOOL ret = FS_FALSE;
	SOCKET s;
	if( bTCP )
		s = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	else
		s = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	
	if( (FS_UINT4)s != 0xFFFFFFFF )
	{
		ret = FS_TRUE;
		*sockid = (FS_UINT4)s;
		WSAAsyncSelect( s, GFS_HWND, WM_SOCKET, FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE );
		GIFS_SockCb = handler;
	}
	else
	{
		int errno = WSAGetLastError( );
	}
	return ret;
#endif
#ifdef FS_PLT_VIENNA
	FS_BOOL ret = FS_FALSE;
	INT16 sid;

	GIFS_Tcp = bTCP;
	if( bTCP )
		sid = SocketOpen(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	else
		sid = SocketOpen(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if( sid != -1 )
	{
		*sockid = sid;
		SocketSetErrorHandler( sid, IFS_SocketErrorHandler );
		SocketSetEventHandler( sid, IFS_SocketEventHandler );
		SocketSetRecvDataHandler( sid, IFS_DataRecvHandler );
		SocketSetSendDataHandler( sid, IFS_DataSendHandler );
		ret = FS_TRUE;
		GIFS_SockCb = handler;
	}
	return ret;
#endif
}

FS_BOOL IFS_SocketConnect( FS_UINT4 sockid, FS_CHAR *host, FS_UINT2 port )
{
#ifdef FS_PLT_WIN
	SOCKET s = (SOCKET)sockid ;
	SOCKADDR_IN sAddr;
	int ret = 0;
	int noblock = 1;
	int err;
	
	struct hostent* hent;
	if( host )
	{
		if( ! IFS_HostIsIPAddress( host ) )
		{
			hent = gethostbyname( host );
			if( hent )
				memcpy( &sAddr.sin_addr.s_addr, hent->h_addr, sizeof(int) );
			else
				return FS_FALSE;
		}
		else
		{
			sAddr.sin_addr.s_addr = inet_addr( host );
		}
		sAddr.sin_family = AF_INET;
		sAddr.sin_port = htons( port );    

		ret = connect( s, (SOCKADDR *)&sAddr, sizeof(sAddr) );
		err = WSAGetLastError( );
		if( ret == SOCKET_ERROR && err == WSAEWOULDBLOCK )
			return FS_TRUE;
		else
			return FS_FALSE;
	}
	else
	{
		sAddr.sin_family = AF_INET;
		sAddr.sin_port = htons( port );
		sAddr.sin_addr.s_addr = 0;
		bind( s,  (SOCKADDR *)&sAddr, sizeof(sAddr) );
		GIFS_SockCb( s, FS_ISOCK_CONNECT, 0 );
		return FS_TRUE;
	}
#endif	
#ifdef FS_PLT_VIENNA
	GIFS_SockId = sockid;
	GIFS_Port = port;

#ifdef SIMULATION
	{
		UINT32 dns;
		GSMIPAddrDotFormatToByteArray("10.100.151.17", (UINT8 *)&dns);
		DNS_SetServerIPAddress(dns);
	}
#else
	{
		UINT32 dnsip = 0;
		DNS_GetServerIPAddress(&dnsip);
		GSMprintf("IFS_SocketConnect DNS Server Address %d.%d.%d.%d\n",
			(UINT8)(dnsip >> 24),
			(UINT8)(dnsip >> 16),
			(UINT8)(dnsip >> 8),
			(UINT8)dnsip);
		if( dnsip == 0 )
		{
			UINT32 dns;
			GSMIPAddrDotFormatToByteArray("211.138.151.161", (UINT8 *)&dns);
			DNS_SetServerIPAddress(dns);
		}
	}
#endif

	if( GIFS_Tcp )
	{
		/* go TCP */
		if( SocketIsBound(sockid) )
		{
			INT16 dnsRet = DNS_GetHostByName( host, IFS_DNSCallBack );
			GSMprintf("IFS_SocketConnect DNS_GetHostByName return %d\n", dnsRet);
		}
		else
		{
			char *hn = GSMstrdup( host );
			ALStartTimer( 8888, MSECS(500), IFS_DeferConnect, hn );
		}
	}
	else
	{
		/* go UDP */
		struct SockAddr_In sockaddr;
		sockaddr.sin_family = AF_INET;
		sockaddr.sin_port = GSMhtons( port );
		sockaddr.sin_addr.s_addr = GSMhtonl(GIFS_LocalAddr);;
		if( ! SocketIsBound(sockid) )
		{
			SocketBind( sockid, (struct SockAddr *)(&sockaddr), sizeof( sockaddr ) );
		}
		else
		{
			GIFS_SockCb( sockid, FS_ISOCK_CONNECT, 0 );
		}
	}
	return FS_TRUE;
#endif
}

FS_SINT4 IFS_SocketSend( FS_UINT4 sockid, FS_BYTE *buf, FS_SINT4 len )
{
#ifdef FS_PLT_WIN
	int ret;
	int noblock = 0;
	SOCKET s = (SOCKET)sockid;

	if ( len > 2048 ) len = 2048;

	ret = WSAAsyncSelect( s, GFS_HWND, WM_SOCKET, 0 );
	if( ret != SOCKET_ERROR )
		ret = ioctlsocket( s, FIONBIO, &noblock );
	if( ret != SOCKET_ERROR )
		ret = send( (SOCKET)sockid, buf, len, 0 );
	if( ret == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK )
	{
		ret = -1;
	}
	else
	{
		PostMessage( GFS_HWND, WM_SOCKET_SENDOK, s, FS_ISOCK_SENDOK );
		WSAAsyncSelect( s, GFS_HWND, WM_SOCKET, FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE );
	}
	return ret;
#endif	
#ifdef FS_PLT_VIENNA
	INT16 rlen = SocketSend( sockid, buf, len );
	return rlen;
#endif
}

FS_SINT4 IFS_SocketSendTo( FS_UINT4 sockid, FS_BYTE *buf, FS_SINT4 len, FS_SockAddr *to )
{
#ifdef FS_PLT_WIN
	int ret;
	int noblock = 0;
	struct hostent* hent;
	struct sockaddr_in to_addr;
	SOCKET s = (SOCKET)sockid;

	if( ! IFS_HostIsIPAddress( to->host ) )
	{
		hent = gethostbyname( to->host );
		if( hent )
			memcpy( &to_addr.sin_addr.s_addr, hent->h_addr, sizeof(int) );
		else
			return -1;
	}
	else
	{
		to_addr.sin_addr.s_addr = inet_addr( to->host );
	}
	to_addr.sin_family = AF_INET;
	to_addr.sin_port = htons( to->port );    
	
	ret = sendto( (SOCKET)sockid, buf, len, 0, (SOCKADDR *)&to_addr, sizeof(to_addr) );
	return ret;
#endif
#ifdef FS_PLT_VIENNA
	struct SockAddr_In sock;
	BOOLEAN bret;
	INT16 rlen = -1;

	sock.sin_family = AF_INET;
	sock.sin_port = GSMhtons( to->port );
	bret = GSMIPAddrDotFormatToByteArray( to->host, (UINT8 *)&sock.sin_addr.s_addr );

	if( bret )
	{
		rlen = SocketSendTo( sockid, buf, len, (struct SockAddr *)&sock, sizeof(sock) );
	}
	return rlen;
#endif
}

FS_SINT4 IFS_SocketRecv( FS_UINT4 sockid, FS_BYTE *buf, FS_SINT4 len )
{
#ifdef FS_PLT_WIN
	int ret;
	ret = recv( (SOCKET)sockid, buf, len, 0 );
	if( ret == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK )
	{
		ret = -1;
	}
	return ret;
#endif
#ifdef FS_PLT_VIENNA
	 return SocketRecv( sockid, buf, len );
#endif
}

FS_SINT4 IFS_SocketRecvFrom( FS_UINT4 sockid, FS_BYTE *buf, FS_SINT4 len, FS_SockAddr *from )
{
#ifdef FS_PLT_WIN
	int ret;
	struct sockaddr_in from_addr;
	int from_len = sizeof(struct sockaddr_in);
	
	memset( &from_addr, 0, sizeof(struct sockaddr_in));
	ret = recvfrom( (SOCKET)sockid, buf, len, 0, (SOCKADDR *)&from_addr, &from_len );
	if( ret == SOCKET_ERROR ){
		from_len = WSAGetLastError( );
		ret = -1;
	}
	return ret;
#endif	
#ifdef FS_PLT_VIENNA
	struct SockAddr_In sock;
	UINT8 addrlen;
	INT16 rlen = -1;

	rlen = SocketRecvFrom( sockid, buf, len, (struct SockAddr *)&sock, &addrlen );
	return rlen;
#endif
}

FS_BOOL IFS_SocketClose( FS_UINT4 sockid )
{
#ifdef FS_PLT_WIN
	closesocket( (SOCKET)sockid );
	return FS_TRUE;
#endif
#ifdef FS_PLT_VIENNA
	 SocketClose( sockid );
	 return FS_TRUE;
#endif
}

#ifdef FS_PLT_WIN
static int IFS_HostIsIPAddress( char *host )
{
	int i = 0;
	while( host[i] )
	{
		if( !( host[i] == '.' || (host[i] >= '0' && host[i] <= '9') ) )
			return 0;

		i ++;
	}
	return 1;
}

void ProcessSocketSendOK( FS_UINT4 wParam, FS_UINT4 lParam )
{
	SOCKET s = (SOCKET)wParam;
	GIFS_SockCb( s, lParam, 0 );
}

void ProcessSocketEvent( FS_UINT4 wParam, FS_UINT4 lParam )
{
	SOCKET s = (SOCKET)wParam;
	
	if( WSAGETSELECTERROR(lParam) )
	{
		GIFS_SockCb( wParam, FS_ISOCK_ERROR, WSAGetLastError() );
		return;
	}

	// Determine what event occurred on the
	// socket

	switch( WSAGETSELECTEVENT(lParam) )
	{
		case FD_CONNECT:
			GIFS_SockCb( s, FS_ISOCK_CONNECT, 0 );
			break;
			
		case FD_READ:
			GIFS_SockCb( s, FS_ISOCK_READ, 0 );
			break;

		case FD_WRITE:
			GIFS_SockCb( s, FS_ISOCK_WRITE, 0 );
			break;

		case FD_CLOSE:
			GIFS_SockCb( s, FS_ISOCK_CLOSE, 0 );
			break;
	}
}
#endif
