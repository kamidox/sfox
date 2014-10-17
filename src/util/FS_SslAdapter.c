#include "inc/FS_Config.h"

#ifdef FS_MODULE_SSL

#include "inc/util/FS_SslAdapter.h"
#include "inc/inte/FS_Inte.h"
#include "inc/inte/FS_ISocket.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_MemDebug.h"

#include "Havege.h"
#include "ssl.h"
#include "net.h"

#define FS_SSL_CTX( ssl )			(&ssl->ssl_ctx)

#define FS_SSL_HANDSHAKE_OVER( ssl )	( ssl->ssl_ctx.state == SSL_HANDSHAKE_OVER )

typedef struct FS_SslNetBio_Tag
{
	FS_BYTE *				data;
	FS_SINT4				len;
	FS_SINT4				max_len;
}FS_SslNetBio;

typedef struct FS_SslSession_Tag
{
	ssl_context				ssl_ctx;
	
	FS_SslEvHandler 		callback;

	FS_UINT4				socket;
	FS_SslNetBio			r_bio;		/* read bio */
	FS_SslNetBio			w_bio;		/* write bio */
	FS_BOOL					in_use;
}FS_SslSession;

static FS_SslSession GFS_Ssl;

static FS_BOOL FS_SslNetBioWrite( FS_SslNetBio *bio, FS_BYTE *data, FS_SINT4 len )
{
	FS_BYTE *p;
	if( bio->max_len < bio->len + len )
	{
		/* buffer not enough large to hold data */
		p = bio->data;
		bio->max_len = bio->len + len;
		bio->data = IFS_Malloc( bio->max_len );
		if( bio->data == FS_NULL )
		{
			FS_SAFE_FREE( p );
			IFS_Memset( bio, 0, sizeof(FS_SslNetBio) );
			return FS_FALSE;
		}
		if( p )
		{
			IFS_Memcpy( bio->data, p, bio->len );
			FS_SAFE_FREE( p );
		}
		IFS_Memcpy( bio->data + bio->len, data, len );
		bio->len += len;
	}
	else
	{
		/* just copy data */
		IFS_Memcpy( bio->data + bio->len, data, len );
		bio->len += len;
	}
	return FS_TRUE;
}

static FS_SINT4 FS_SslNetBioRead( FS_SslNetBio *bio, FS_BYTE *buf, FS_SINT4 len, FS_BOOL bUpdate )
{
	FS_SINT4 rlen;

	rlen = FS_MIN( bio->len, len );
	if( rlen > 0 )
	{
		IFS_Memcpy( buf, bio->data, rlen );
		if( bUpdate )
		{
			if( bio->len > rlen )
				IFS_Memmove( bio->data, bio->data + rlen, bio->len - rlen );
			bio->len -= rlen;
			if( bio->len == 0 )
			{
				IFS_Free( bio->data );
				IFS_Memset( bio, 0, sizeof(FS_SslNetBio) );
			}
		}
	}
	return rlen;
}

static void FS_SslErrorReport( FS_SslSession *ssl, FS_SINT4 errno )
{
	ssl->callback( ssl, FS_SSL_EV_ERROR, errno );
}

static void FS_SslNetDataRead( FS_SslSession *ssl )
{
	FS_SINT4 rlen;
	FS_BYTE *buf = IFS_Malloc( FS_SOCKET_BUF_LEN );
	if( buf )
	{
		rlen = IFS_SocketRecv( ssl->socket, buf, FS_SOCKET_BUF_LEN );
		FS_TRACE1( "SSL recv data. len = %d\r\n", rlen );
		if( rlen < 0 )
		{
			IFS_Free( buf );
			FS_SslErrorReport( ssl, FS_SSL_ERR_NETWORK );
			return;
		}
		/* write data buffer to ssl read BIO */
		FS_SslNetBioWrite( &ssl->r_bio, buf, rlen );
		IFS_Free( buf );
	}
	else
	{
		FS_SslErrorReport( ssl, FS_SSL_ERR_MEMORY );
		return;
	}
}

static void FS_SslNetFlushWriteBio( FS_SslSession *ssl )
{
	FS_BYTE *buf;
	FS_SINT4 len;
	
	FS_SslNetBio *bio = &ssl->w_bio;
	if( bio->len > 0 )
	{
		buf = IFS_Malloc( FS_SOCKET_PKG_LEN + 50 );
		if( buf )
		{
			len = FS_SslNetBioRead( bio, buf, FS_SOCKET_PKG_LEN + 50, FS_TRUE );
			IFS_SocketSend( ssl->socket, buf, len );
			IFS_Free( buf );
		}
	}
}

static void FS_SslHandshake( FS_SslSession *ssl )
{
	int ret;
	if( (ret = ssl_client_start( FS_SSL_CTX(ssl) )) != 0 )
	{
		if( ret != ERR_NET_WOULD_BLOCK ) 
		{
			FS_SslErrorReport( ssl, ret );
			return;
		}
	}
	FS_SslNetFlushWriteBio( ssl );

	if( FS_SSL_HANDSHAKE_OVER(ssl) )
	{
		ssl->callback( ssl, FS_SSL_EV_CONNECTED, 0 );
	}
}

static void FS_SslApplicationDataArrived( FS_SslSession *ssl )
{
	FS_BYTE *buf;
	FS_SINT4 len, ret;
	FS_SslBuf sslBuf;
	
	buf = IFS_Malloc( SSL_MAX_CONTENT_LEN );
	if( buf )
	{
		len = SSL_MAX_CONTENT_LEN;
		if( (ret = ssl_read(FS_SSL_CTX(ssl), buf, &len) ) != 0 )
		{
			if( ret != ERR_NET_WOULD_BLOCK && ret != ERR_SSL_PEER_CLOSE_NOTIFY )
				FS_SslErrorReport( ssl, ret );
		}
		else
		{
			sslBuf.buf = buf;
			sslBuf.len = len;
			FS_TRACE1( "SSL APPLICATION DATA len = %d\r\n", len );
			ssl->callback( ssl, FS_SSL_EV_DATA_ARRIVE, (FS_UINT4)&sslBuf );
		}
		IFS_Free( buf );
	}
	else
	{
		FS_SslErrorReport( ssl, FS_SSL_ERR_MEMORY );
	}
}

static void FS_SslSocketEvent_CB( FS_UINT4 socket, FS_ISockEvent ev, FS_UINT4 param )
{
	FS_SslSession *ssl = &GFS_Ssl;
	
	if( ! ssl->in_use || ssl->socket != socket ) return;

	switch( ev )
	{
		case FS_ISOCK_CONNECT:
			FS_SslHandshake( ssl );
			break;
		case FS_ISOCK_SENDOK:
			if( ssl->w_bio.len > 0 )
			{
				FS_SslNetFlushWriteBio( ssl );
				return;
			}

			if( FS_SSL_HANDSHAKE_OVER(ssl) && ssl->w_bio.len == 0 )
				ssl->callback( ssl, FS_SSL_EV_SEND_OK, 0 );
			else
				FS_SslHandshake( ssl );
			break;
		case FS_ISOCK_READ:
			FS_SslNetDataRead( ssl );
			/* when SSL Record is all recived. we continue do handshake */
			if( FS_SSL_HANDSHAKE_OVER(ssl) )
			{
				/* handle application data */
				FS_SslApplicationDataArrived( ssl );
			}
			else
			{
				/* do handshake */
				FS_SslHandshake( ssl );
			}
			break;
		case FS_ISOCK_CLOSE:
		case FS_ISOCK_ERROR:
			if( ssl->in_use )
			{
				if( ssl->socket )
				{
					IFS_SocketClose( ssl->socket );
					ssl->socket = 0;
					if( FS_SSL_HANDSHAKE_OVER(ssl) )
					{
						ssl->callback( ssl, FS_SSL_EV_CLOSE, 0 );
					}
					else
					{
						ssl->callback( ssl, FS_SSL_EV_ERROR, 0 );
					}
				}
			}
			break;
		default:
			break;
	}
}

FS_SslHandle FS_SslCreate( FS_SslEvHandler callback )
{	
	FS_SslSession *ssl = &GFS_Ssl;
	
	if( ssl->in_use ) return FS_NULL;
	
	IFS_Memset( ssl, 0, sizeof(FS_SslSession) );
	ssl->in_use = FS_TRUE;
	ssl->callback = callback;
	
	return ssl;
}

/* act a ssl handshake. caller must connect to server */
FS_BOOL FS_SslConnect( FS_SslHandle hSsl, FS_CHAR *host, FS_UINT2 port )
{
	static havege_state s_hs;
	FS_SslSession *ssl = &GFS_Ssl;
	
	if( hSsl != ssl || !ssl->in_use )
		return FS_FALSE;
		
	havege_init( &s_hs );
	if( ssl_init( FS_SSL_CTX(ssl), 1 ) != 0 )
		return FS_FALSE;
	
	ssl_set_endpoint( FS_SSL_CTX(ssl), SSL_IS_CLIENT );
	ssl_set_authmode( FS_SSL_CTX(ssl), SSL_VERIFY_NONE );
	ssl_set_rng_func( FS_SSL_CTX(ssl), havege_rand, &s_hs );
	ssl_set_ciphlist( FS_SSL_CTX(ssl), ssl_default_ciphers );
	
	if( ! IFS_SocketCreate(&ssl->socket, FS_TRUE, FS_SslSocketEvent_CB ) )
		return FS_FALSE;
	if( ! IFS_SocketConnect(ssl->socket, host, port) )
		return FS_FALSE;
	return FS_TRUE;
}

FS_SINT4 FS_SslWrite( FS_SslHandle hSsl, FS_BYTE *buf, FS_SINT4 len )
{
	FS_SslSession *ssl = &GFS_Ssl;
	
	if( hSsl != ssl || !ssl->in_use )
		return -1;

	if( ! FS_SSL_HANDSHAKE_OVER(ssl) )
		return -1;
	
	if( ssl_write( FS_SSL_CTX(ssl), buf, len ) == 0 )
	{
		FS_SslNetFlushWriteBio( ssl );
		return len;
	}
	else
	{
		return 0;
	}
}

void FS_SslDestroy( FS_SslHandle hSsl )
{
	FS_SslSession *ssl = &GFS_Ssl;
	
	if( hSsl != ssl || !ssl->in_use ) return;

	ssl_free( FS_SSL_CTX(ssl) );
	FS_SAFE_FREE( ssl->r_bio.data );
	FS_SAFE_FREE( ssl->w_bio.data );
	IFS_Memset( ssl, 0, sizeof(FS_SslSession) );
}

void FS_SslClose( FS_SslHandle hSsl )
{
	FS_SslSession *ssl = &GFS_Ssl;
	
	if( hSsl != ssl || !ssl->in_use ) return;
	
	FS_SAFE_FREE( ssl->r_bio.data );
	FS_SAFE_FREE( ssl->w_bio.data );
	IFS_Memset( &ssl->r_bio, 0, sizeof(FS_SslNetBio) );
	IFS_Memset( &ssl->w_bio, 0, sizeof(FS_SslNetBio) );
	IFS_SocketClose( ssl->socket );
	ssl->socket = 0;
}

/*----------------------- Interface for xyssl lib net ----------------------------------*/
/**
 * \brief		   Loop until "len" characters have been read
 */
int net_recv( int fd, unsigned char *buf, int *len )
{
	int ret = ERR_NET_RECV_FAILED, rlen;
	FS_SslSession *ssl = &GFS_Ssl;
	if( ssl->in_use )
	{
		rlen = FS_SslNetBioRead( &ssl->r_bio, buf, *len, FS_TRUE );
		FS_TRACE2( "net_recv len = %d, rlen = %d\r\n", *len, rlen );
		if( *len == rlen )
		{
			ret = 0;
		}
		else
		{
			*len = rlen;
			ret = ERR_NET_WOULD_BLOCK;
		}
	}
	return ret;
}

/**
 * \brief		   Loop until "len" characters have been written
 */
int net_send( int fd, unsigned char *buf, int *len )
{
	int ret = ERR_NET_SEND_FAILED;
	FS_SslSession *ssl = &GFS_Ssl;
	if( ssl->in_use )
	{
		if( FS_SslNetBioWrite( &ssl->w_bio, buf, *len ) )
		{
			ret = 0;
		}
	}
	return ret;
}

/**
 * \brief		   Gracefully shutdown the connection
 */
void net_close( int sock_fd )
{
	FS_SslSession *ssl = &GFS_Ssl;
	if( ssl->in_use )
	{
		if( ssl->socket )
		{
			IFS_SocketClose( ssl->socket );
			ssl->socket = 0;
		}
	}
}

#else //FS_MODULE_SSL
#include "inc/util/FS_SslAdapter.h"

/* dummy function to make complier happy */
FS_SslHandle FS_SslCreate( FS_SslEvHandler callback ){return FS_NULL;}

/* act a ssl handshake. caller must connect to server */
FS_BOOL FS_SslConnect( FS_SslHandle hSsl, FS_CHAR *host, FS_UINT2 port ){return FS_FALSE;}

FS_SINT4 FS_SslWrite( FS_SslHandle hSsl, FS_BYTE *buf, FS_SINT4 len ) {return 0;}

void FS_SslClose( FS_SslHandle hSsl ){}

void FS_SslDestroy( FS_SslHandle hSsl ){}

#endif //FS_MODULE_SSL
