#include "inc/web/FS_Http.h"
#include "inc/inte/FS_Inte.h" 
#include "inc/util/FS_Mime.h" 
#include "inc/util/FS_File.h" 
#include "inc/util/FS_Util.h" 
#include "inc/res/FS_TimerID.h" 
#include "inc/util/FS_SslAdapter.h"
#include "inc/util/FS_MemDebug.h"

#define _HTTP_DEBUG

#ifdef _HTTP_DEBUG
#define FS_HTTP_TRACE0(a)				FS_TRACE0( "[HTTP]" a "\r\n")
#define FS_HTTP_TRACE1(a,b)				FS_TRACE1( "[HTTP]" a "\r\n", b)
#define FS_HTTP_TRACE2(a,b,c)			FS_TRACE2( "[HTTP]" a "\r\n", b, c)
#define FS_HTTP_TRACE3(a,b,c,d)			FS_TRACE3( "[HTTP]" a "\r\n", b, c, d)
#else
#define FS_HTTP_TRACE0(a)
#define FS_HTTP_TRACE1(a,b)
#define FS_HTTP_TRACE2(a,b,c)
#define FS_HTTP_TRACE3(a,b,c,d)
#endif

#define FS_HTTP_HEADER_LEN			16384
#ifdef FS_PLT_WIN
#define FS_HTTP_WAIT_RESPONSE		10000	/* 10 second */
#else
#define FS_HTTP_WAIT_RESPONSE		120000	/* 120 second */
#endif

typedef enum FS_HttpState_Tag
{
	FS_HS_NONE = 0,
	FS_HS_IDLE,
	FS_HS_REQUEST,
	FS_HS_RESEND_REQUEST,
	FS_HS_POST_DATA,
	FS_HS_WAIT_RESPONSE
}FS_HttpState;

typedef struct FS_HttpSession_Tag
{
	FS_List						list;
	
	FS_CHAR *					method;
	FS_CHAR *					url;
	FS_CHAR *					addtional_headers;
	FS_HttpPostDataStruct *		post_data;

	FS_BOOL						is_security;
	FS_UINT4					socket;
	FS_SslHandle				hSsl;
	
	FS_SINT4					status;		/* http status code */
	FS_BOOL						conn_close;
	FS_HttpHeader *				header;
	FS_BYTE *					buf;		/* for cache http headers response from server */
	FS_SINT4					buf_len;
	FS_CHAR *					host;
	FS_UINT2					port;
	FS_SockAddr					addr;
	FS_CHAR *					request_data;	/* for cache http request header to be send */
	FS_SINT4					request_data_len;
	FS_SINT4					request_data_offset;
	
	FS_HttpState				state;
	FS_BOOL						can_retry;
	FS_SINT4					rsp_data_len;
	FS_UINT4					rsp_timer_id;
	
	FS_CHAR *					accept;
	FS_CHAR *					accept_charset;
	FS_CHAR *					accept_encoding;
	FS_CHAR *					accept_language;
	FS_CHAR *					user_agent;
	FS_CHAR *					ua_profile;
	
	void *							user_data;
	FS_HttpResponseStartFunc		response_start;
	FS_HttpResponseDataFunc			response_data;
	FS_HttpResponseEndFunc			response_end;
	FS_HttpRequestProgFunc			request_prog;

	FS_BOOL						in_use;

	/* http chunked encoding support */
	FS_BOOL						chunked_encoding;
	FS_SINT4					chunk_size;
}FS_HttpSession;

static FS_List GFS_HttpSessionList = {&GFS_HttpSessionList, &GFS_HttpSessionList};

static void FS_HttpSockEvent_CB( FS_UINT4 socket, FS_ISockEvent ev, FS_UINT4 param );
static void FS_HttpSslEvent_CB( FS_SslHandle hSsl, FS_SslEvent ev, FS_UINT4 param );

static FS_BOOL FS_HttpSessionExist( FS_HttpSession *http )
{
	FS_List *node;
	FS_HttpSession *httpSes;
	
	FS_ListForEach( node, &GFS_HttpSessionList ){
		httpSes = FS_ListEntry( node, FS_HttpSession, list );
		if( httpSes == http ){
			return FS_TRUE;
		}
	}
	return FS_FALSE;
}

static FS_HttpSession *FS_HttpGetSessionBySocketId( FS_UINT4 socketid )
{
	FS_List *node;
	FS_HttpSession *httpSes;
	
	FS_ListForEach( node, &GFS_HttpSessionList ){
		httpSes = FS_ListEntry( node, FS_HttpSession, list );
		if( httpSes->socket == socketid ){
			return httpSes;
		}
	}
	return FS_NULL;
}

static FS_HttpSession *FS_HttpGetSessionBySslHandler( FS_SslHandle hSsl )
{
	FS_List *node;
	FS_HttpSession *httpSes;
	
	FS_ListForEach( node, &GFS_HttpSessionList ){
		httpSes = FS_ListEntry( node, FS_HttpSession, list );
		if( httpSes->hSsl == hSsl ){
			return httpSes;
		}
	}
	return FS_NULL;
}

static void FS_HttpFreeUselessSession( void )
{
	FS_List *node;
	FS_HttpSession *httpSes;

	node = GFS_HttpSessionList.next;
	while( node != &GFS_HttpSessionList ){
		httpSes = FS_ListEntry( node, FS_HttpSession, list );
		node = node->next;
		
		if( ! httpSes->in_use ){
			FS_ListDel( &httpSes->list );
			IFS_Free( httpSes );
		}
	}
}

static void FS_HttpProcessUrl( FS_CHAR * url, FS_HttpSession *http )
{
	FS_CHAR *p, *origHost;
	FS_SINT4 len = 0;
	
	FS_COPY_TEXT( http->url, url );
	origHost = http->host;
	
	http->is_security = FS_FALSE;
	if( IFS_Strnicmp( url, "http://", 7 ) == 0 )
	{
		len = 7;
		http->port = 80;
	}
	else if( IFS_Strnicmp( url, "https://", 8 ) == 0 )
	{
		len = 8;
		http->is_security = FS_TRUE;
		http->port = 443;
	}
	url += len;

	/* url & host */
	len = IFS_Strlen( url );
	p = IFS_Strchr( url, '/' );
	if( p )
	{
		http->host = FS_Strndup( url, p - url );
		
		/* not use proxy. we need relative url */
		if( http->addr.host == FS_NULL )
		{
			FS_COPY_TEXT( http->url, p );
		}
	}
	else
	{
		http->host = IFS_Strdup( url );
		
		/* not use proxy. we need relative url */
		if( http->addr.host == FS_NULL )
		{
			FS_COPY_TEXT( http->url, "/" );
		}
	}

	/* port */
	p = IFS_Strchr( http->host, ':' );
	if( p )
	{
		*p = '\0';
		p ++;
		http->port = IFS_Atoi( p );
	}

 	/* set state to FS_HS_NONE to force reconnect to server */
	if( origHost && http->host && IFS_Stricmp(origHost, http->host) != 0 ) {
		FS_HTTP_TRACE0( "FS_HttpProcessUrl different host, force to reconnect to server" );
		http->state = FS_HS_NONE;
	}

	FS_SAFE_FREE( origHost );
}

static void FS_HttpFreePostData( FS_HttpSession *http )
{
	if( http->post_data )
	{
		FS_SAFE_FREE( http->post_data->content_type );
		FS_SAFE_FREE( http->post_data->data );
		IFS_Free( http->post_data );
		http->post_data = FS_NULL;
	}
}

static void FS_HttpSavePostData( FS_HttpSession *http, FS_HttpPostDataStruct *post_data )
{
	if( http->post_data )
		FS_HttpFreePostData( http );
	http->post_data = IFS_Malloc( sizeof(FS_HttpPostDataStruct) );
	if( http->post_data )
	{
		IFS_Memset( http->post_data, 0, sizeof(FS_HttpPostDataStruct) );
		http->post_data->content_type = IFS_Strdup( post_data->content_type );
		http->post_data->data_len = post_data->data_len;
		http->post_data->data_is_file = post_data->data_is_file;
		if( post_data->data_is_file )
		{
			FS_COPY_TEXT( http->post_data->data, post_data->data );
		}
		else
		{
			http->post_data->data = IFS_Malloc( post_data->data_len );
			if( http->post_data->data ) IFS_Memcpy( http->post_data->data, post_data->data, post_data->data_len );
		}
	}
}

static void FS_HttpFreeHeaders( FS_HttpSession *http )
{
	FS_List *node;
	FS_MimeParam *param;
	if( http->header )
	{
		FS_SAFE_FREE( http->header->content_type );
		FS_SAFE_FREE( http->header->location );
		FS_SAFE_FREE( http->header->cookies );
		FS_SAFE_FREE( http->header->x_next_url );
		node = http->header->params.next;
		while( node != &http->header->params )
		{
			param = FS_ListEntry( node, FS_MimeParam, list );
			node = node->next;
			FS_ListDel( &param->list );
			if( param->name )
				IFS_Free( param->name );
			if( param->val )
				IFS_Free( param->val );
			IFS_Free( param );
		}
		IFS_Free( http->header );
		http->header = FS_NULL;
	}
}

static void FS_HttpResponseFinish( FS_HttpSession *http, FS_SINT4 end_code, FS_BOOL callback )
{
	FS_HTTP_TRACE3( "FS_HttpResponseFinish state = %d, end_code = %d, callback = %d", http->state, end_code, callback );

	if( http->state == FS_HS_NONE ) {
		return;
	}

	if ( http->state == FS_HS_IDLE && end_code != FS_HTTP_OK ) {
		http->state = FS_HS_NONE;
		if ( http->socket ) {
			IFS_SocketClose( http->socket );
			http->socket = 0;
		}
		return;		
	}
		
	if( http->socket && end_code == FS_HTTP_ERR_OK ) {
		http->state = FS_HS_IDLE;
	} else {
		http->state = FS_HS_NONE;
		if ( http->socket ) {
			IFS_SocketClose( http->socket );
			http->socket = 0;
		}
	}
	FS_HTTP_TRACE1( "FS_HttpResponseFinish state = %d", http->state );

	http->rsp_data_len = 0;
	http->chunked_encoding = FS_FALSE;
	http->chunk_size = 0;
	FS_SAFE_FREE( http->addr.host );
	http->addr.host = FS_NULL;
	http->addr.port = 0;
	FS_HttpFreeHeaders( http );
	FS_HttpFreePostData( http );
	FS_SAFE_FREE( http->addtional_headers );
	FS_SAFE_FREE( http->buf );
	http->buf_len = 0;
	http->addtional_headers = FS_NULL;
	http->request_data_offset = 0;
	http->request_data_len = 0;
	FS_SAFE_FREE( http->request_data );
	if( http->rsp_timer_id )
	{
		IFS_StopTimer( http->rsp_timer_id );
		http->rsp_timer_id = 0;
	}
	
	if( http->hSsl )
	{
		/* close socket. act resume ssl session */
		http->state = FS_HS_NONE;
		FS_SslClose( http->hSsl );
	}
	
	if( callback )
	{
		http->response_end( http->user_data, http, end_code );
	}
}

static void FS_HttpConnectServer( FS_HttpSession *http )
{
	FS_CHAR *host;
	FS_UINT2 port;

	if( http->socket )
	{
		IFS_SocketClose( http->socket );
		http->socket = 0;
	}
	
	/* here, client may use http proxy */
	if( http->addr.host )
	{
		host = http->addr.host;
		port = http->addr.port;
	}
	else
	{
		host = http->host;
		port = http->port;
	}

	FS_HTTP_TRACE2( "FS_HttpConnectServer host = %s, port = %d", host, port );

	if( http->is_security )
	{
		if( http->hSsl )
		{
			if( ! FS_SslConnect( http->hSsl, host, port ) )
			{
				FS_SslDestroy( http->hSsl );
				http->hSsl = FS_NULL;
				FS_HttpResponseFinish( http, FS_HTTP_ERR_NET, FS_TRUE );
			}
		}
		else
		{
			if( ( http->hSsl = FS_SslCreate(FS_HttpSslEvent_CB) ) == FS_NULL
				|| ! FS_SslConnect( http->hSsl, host, port ) )
			{
				FS_HttpResponseFinish( http, FS_HTTP_ERR_NET, FS_TRUE );
			}
		}
	}
	else
	{
		if( ! IFS_SocketCreate( &http->socket, FS_TRUE, FS_HttpSockEvent_CB )
			|| ! IFS_SocketConnect( http->socket, host, port ) )
		{
			FS_HttpResponseFinish( http, FS_HTTP_ERR_NET, FS_TRUE );
		}
	}
}

static void FS_FreeHttpSession( FS_HttpSession *http )
{	
	FS_SAFE_FREE( http->method );
	FS_SAFE_FREE( http->url );
	FS_SAFE_FREE( http->addtional_headers );
	if( http->post_data )
		FS_HttpFreePostData( http );
	if( http->socket ) {
		IFS_SocketClose( http->socket );
		http->socket = 0;
	}
	if( http->hSsl ){
		FS_SslDestroy( http->hSsl );
		http->hSsl = FS_NULL;
	}
	FS_SAFE_FREE( http->host );
	FS_SAFE_FREE( http->accept );
	FS_SAFE_FREE( http->accept_charset );
	FS_SAFE_FREE( http->accept_encoding );
	FS_SAFE_FREE( http->accept_language );
	FS_SAFE_FREE( http->user_agent );
	FS_SAFE_FREE( http->ua_profile );
	FS_SAFE_FREE( http->buf );
	FS_SAFE_FREE( http->addr.host );
	FS_SAFE_FREE( http->request_data );
	FS_HttpFreeHeaders( http );
	if( http->rsp_timer_id )
	{
		IFS_StopTimer( http->rsp_timer_id );
		http->rsp_timer_id = 0;
	}
}

static FS_SINT4 FS_HttpFormatRequest( FS_HttpSession *http, FS_CHAR *out, FS_SINT4 buflen )
{
	FS_SINT4 ret = 0;
	FS_SINT4 len = 0;
	/* method */
	IFS_Snprintf( out + ret, buflen - 1, "%s %s HTTP/1.1\r\n", http->method, http->url );
	ret = IFS_Strlen( out );

	/* Host */
	IFS_Snprintf( out + ret, buflen - ret - 1, "Host: %s\r\n", http->host );
	ret += IFS_Strlen( out + ret );
	
	if( http->addtional_headers )
	{
		/* client addtional headers */
		IFS_Strncpy( out + ret, http->addtional_headers, buflen - ret - 1 );
		ret += IFS_Strlen( out + ret );
	}
	if( http->accept_encoding )
	{
		/* Accept-Encoding */
		IFS_Snprintf( out + ret, buflen - ret - 1, "Accept-Encoding: %s\r\n", http->accept_encoding );
		ret += IFS_Strlen( out + ret );
	}
	if( http->accept_charset )
	{
		/* Accept-Charset */
		IFS_Snprintf( out + ret, buflen - ret - 1, "Accept-Charset: %s\r\n", http->accept_charset );
		ret += IFS_Strlen( out + ret );
	}
	if( http->accept_language )
	{
		/* Accept-Language */
		IFS_Snprintf( out + ret, buflen - ret - 1, "Accept-Language: %s\r\n", http->accept_language );
		ret += IFS_Strlen( out + ret );
	}
	if( IFS_Strnicmp(http->method, "POST", 4) == 0 && http->post_data )
	{		
		/* Content-Type */
		IFS_Snprintf( out + ret, buflen - ret - 1, "Content-Type: %s\r\n", http->post_data->content_type );
		ret += IFS_Strlen( out + ret );
		
		/* Content-Length */
		IFS_Snprintf( out + ret, buflen - ret - 1, "Content-Length: %d\r\n", http->post_data->data_len );
		ret += IFS_Strlen( out + ret );
	}

	/* Accept */
	IFS_Snprintf( out + ret, buflen - ret - 1, "Accept: %s\r\n", http->accept );
	ret += IFS_Strlen( out + ret );

	if( http->user_agent )
	{
		/* User-Agent */
		IFS_Snprintf( out + ret, buflen - ret - 1, "User-Agent: %s\r\n", http->user_agent );
		ret += IFS_Strlen( out + ret );
	}
	
	if( http->ua_profile )
	{
		/* x-wap-profile */
		IFS_Snprintf( out + ret, buflen - ret - 1, "X-Wap-Profile: %s\r\n", http->ua_profile );
		ret += IFS_Strlen( out + ret );
	}
	
	/* Connection */
	IFS_Strncpy( out + ret, "Connection: Keep-Alive\r\n", buflen - ret - 1 );
	ret += IFS_Strlen( out + ret );
	
	IFS_Strncpy( out + ret, "\r\n", buflen - ret - 1 );
	ret += IFS_Strlen( out + ret );
	
	if( http->post_data && http->post_data->data )
	{
		/* post body if not a file */
		if( ! http->post_data->data_is_file )
		{
			len = FS_MIN( buflen - ret - 1, http->post_data->data_len );
			IFS_Memcpy( out + ret, http->post_data->data, len );
			ret += len;
			out[ret] = 0;
			http->post_data->offset = http->post_data->data_len;
		}
		else
		{
			http->post_data->offset = 0;
		}
	}

	return ret;
}

static FS_BYTE * FS_HttpReadNetData( FS_HttpSession *http, FS_SINT4 *olen )
{
	FS_SINT4 rlen = -1;
	FS_BYTE *buf = IFS_Malloc( FS_SOCKET_BUF_LEN );
	if( buf )
	{
		rlen = IFS_SocketRecv( http->socket, buf, FS_SOCKET_BUF_LEN - 1 );
		FS_HTTP_TRACE1( "FS_HttpReadNetData rlen = %d", rlen );
		if( rlen > 0 )
		{
			buf[rlen] = '\0';
		}
		else
		{
			IFS_Free( buf );
			buf = FS_NULL;
			
			if( http->state == FS_HS_WAIT_RESPONSE )
			{
				if( http->rsp_data_len <= 0 )
					FS_HttpResponseFinish( http, FS_HTTP_ERR_NET, FS_TRUE );
				else
					FS_HttpResponseFinish( http, FS_HTTP_ERR_OK, FS_TRUE );
			}
			else if ( http->state == FS_HS_NONE || http->state == FS_HS_IDLE )
			{
				FS_HttpResponseFinish( http, FS_HTTP_ERR_NET, FS_FALSE );
			}
			else
			{
				FS_HttpResponseFinish( http, FS_HTTP_ERR_NET, FS_TRUE );
			}
		}
	}
	else
	{
		FS_HttpResponseFinish( http, FS_HTTP_ERR_MEMORY, FS_TRUE );
	}
	*olen = rlen;
	return buf;
}

static void FS_HttpSendRequest( FS_HttpSession *http, FS_BOOL bFirstTime )
{
	FS_SINT4 len;

	if ( bFirstTime ) {
		http->request_data_offset = 0;
		http->request_data_len = 0;
		FS_SAFE_FREE( http->request_data );

		http->request_data = IFS_Malloc( FS_HTTP_HEADER_LEN );
		if( http->request_data ) {
			http->request_data_len = FS_HttpFormatRequest( http, http->request_data, FS_HTTP_HEADER_LEN );
			if ( http->request_data_len >= FS_HTTP_HEADER_LEN - 1 ) {
				FS_HTTP_TRACE1( "FS_HttpSendRequest ERROR. request data may exceed length limit!!!\r\n%s", http->request_data );
			}
		}
	}

	if ( http->request_data == FS_NULL || http->request_data_len == 0 ) {
		FS_HTTP_TRACE0( "FS_HttpSendRequest ERROR. http->request_data is NULL" );
		return;
	}

	if( http->is_security ) {
		len = FS_SslWrite( http->hSsl, http->request_data + http->request_data_offset, 
			FS_MIN( FS_SOCKET_PKG_LEN, http->request_data_len - http->request_data_offset) );
	} else {
		len = IFS_SocketSend( http->socket, http->request_data + http->request_data_offset, 
			FS_MIN( FS_SOCKET_PKG_LEN, http->request_data_len - http->request_data_offset) );
	}

	http->request_data_offset += len;
	FS_HTTP_TRACE3( "FS_HttpSendRequest len = %d, offset = %d, http state = %d", 
		http->request_data_len, http->request_data_offset, http->state );

	if( len <= 0 ) {
		if( http->state == FS_HS_REQUEST ) {
			/* server may close the connection. we connect server and retry */
			http->state = FS_HS_RESEND_REQUEST;
			FS_HttpConnectServer( http );
		} else {
			FS_HttpResponseFinish( http, FS_HTTP_ERR_NET, FS_TRUE );
		}
	}

	if ( http->request_data_offset >= http->request_data_len ) {
		http->request_data_offset = 0;
		http->request_data_len = 0;
		FS_SAFE_FREE( http->request_data );

		if( http->post_data && http->post_data->data_is_file ) {
			http->state = FS_HS_POST_DATA;
		}

		if( http->request_prog ) {
			http->request_prog( http->user_data, http, 0 );
		}
	}
}

static void FS_HttpSendPostData( FS_HttpSession *http )
{
	FS_SINT4 len;
	FS_BYTE *buf = IFS_Malloc( FS_SOCKET_PKG_LEN );

	if( buf )
	{
		len = FS_FileRead( -1, http->post_data->data, http->post_data->offset, buf, FS_SOCKET_PKG_LEN );

		if( len <= 0 )
		{
			FS_HttpResponseFinish( http, FS_HTTP_ERR_UNKNOW, FS_TRUE );
			IFS_Free( buf );
			return;
		}
		
		if( http->is_security )
			len = FS_SslWrite( http->hSsl, buf, len );
		else
			len = IFS_SocketSend( http->socket, buf, len );
		
		IFS_Free( buf );
		if( len <= 0 )
		{
			FS_HttpResponseFinish( http, FS_HTTP_ERR_NET, FS_TRUE );
		}
		else
		{
			http->post_data->offset += len;
			if( http->request_prog )
			{
				http->request_prog( http->user_data, http, http->post_data->offset );
			}
		}
	}
	else
	{
		FS_HttpResponseFinish( http, FS_HTTP_ERR_MEMORY, FS_TRUE );
	}
}

static void FS_HttpProcessStatus( FS_HttpSession *http, FS_CHAR *buf )
{
	FS_CHAR *p = IFS_Strchr( buf, ' ' );
	if( p )
	{
		p ++;
		http->status = IFS_Atoi( p );
	}
	else
	{
		http->status = FS_HTTP_INTERNAL_SERVER_ERROR;
	}
}

static FS_HttpProcessContentType( FS_HttpSession *http, FS_CHAR *str )
{
	FS_CHAR *p;
	FS_SINT4 len;
	p = IFS_Strchr( str, ';' );
	if( p )
		len = p - str;
	else
		len = IFS_Strlen( str );

	http->header->content_type = FS_Strndup( str, len );
	if( p )
		FS_ProcessParams( &http->header->params, p + 1 );
}

static void FS_HttpWaitResponseHandler( FS_HttpSession *http )
{
	if( http->in_use && http->state == FS_HS_WAIT_RESPONSE )
	{
		FS_HttpResponseFinish( http, FS_HTTP_ERR_OK, FS_TRUE );
	}
}

static void FS_HttpJudgeResponseFinish( FS_HttpSession *http )
{
	FS_BOOL finished = FS_FALSE;
	
	if( http->header )
	{
		FS_HTTP_TRACE3( "FS_HttpJudgeResponseFinish rsp_data_len = %d, content-length = %d, chunked-encoding = %d", 
			http->rsp_data_len, http->header->content_length, http->chunked_encoding );
		if( !http->chunked_encoding && http->header->content_length > 0 )
		{
			if( http->rsp_data_len >= http->header->content_length )
			{
				FS_HttpResponseFinish( http, FS_HTTP_ERR_OK, FS_TRUE );
				finished = FS_TRUE;
			}
		}

		if( ! finished )
		{
			if( http->rsp_timer_id )
			{
				IFS_StopTimer( http->rsp_timer_id );
			}
			http->rsp_timer_id = IFS_StartTimer( FS_TIMER_ID_HTTP, FS_HTTP_WAIT_RESPONSE, FS_HttpWaitResponseHandler, http );
		}
	}
}

static void FS_HttpProcessHeaders( FS_HttpSession *http )
{
	FS_SINT4 hnum = 0;
	FS_CHAR * buf, *hp, *data;
	
	typedef enum FS_HttpHeadField_Tag
	{
		FS_H_CONTENT_LENGTH = 0,
		FS_H_CONTENT_TYPE,
		FS_H_LOCATION,
		FS_H_CONNECTION,
		FS_H_COOKIE,
		FS_H_X_DP_NEXT_URL,
		FS_H_TRANSFER_ENCODING
	}FS_HttpHeadField;
	
	FS_HeadField hentry[] = 
	{
		{ "Content-Length:",			FS_NULL },
		{ "Content-Type:",				FS_NULL },
		{ "Location:",					FS_NULL },
		{ "Connection:",				FS_NULL },
		{ "Set-Cookie:",				FS_NULL },
		{ "X-DP-nextURI:",				FS_NULL },
		{ "Transfer-Encoding:",			FS_NULL },
		// last item
		{ FS_NULL,						FS_NULL }
	};

	FS_HTTP_TRACE0( "FS_HttpProcessHeaders" );
	http->header = IFS_Malloc( sizeof(FS_HttpHeader) );
	if( http->header )
	{
		IFS_Memset( http->header, 0, sizeof(FS_HttpHeader) );
		http->header->content_length = -1;
		FS_ListInit( &http->header->params );
		data = http->buf;
		
		buf = IFS_Malloc( FS_MIME_HEAD_FIELD_MAX_LEN );
		if( buf )
		{
			FS_GetLine( buf, FS_MIME_HEAD_FIELD_MAX_LEN, &data );
			FS_HttpProcessStatus( http, buf );
			
			while(( hnum = FS_GetOneField( buf, FS_MIME_HEAD_FIELD_MAX_LEN, &data, hentry )) != -1 )
			{
				hp = buf + IFS_Strlen( hentry[hnum].name );
				while (*hp == ' ' || *hp == '\t')
					hp++;
	
				switch (hnum)
				{
				case FS_H_CONTENT_LENGTH:
					http->header->content_length = IFS_Atoi( hp );
					break;
				case FS_H_CONTENT_TYPE:
					FS_HttpProcessContentType( http, hp );
					break;
				case FS_H_LOCATION:
					if( ! http->header->location )
						http->header->location = IFS_Strdup( hp );
					break;
				case FS_H_COOKIE:
					if( http->header->cookies == FS_NULL )
					{
						http->header->cookies = IFS_Strdup( hp );
					}
					else
					{
						FS_CHAR *tck = http->header->cookies;
						http->header->cookies = FS_StrConCat( tck, ",", hp, FS_NULL );
						IFS_Free( tck );
					}
					break;
				case FS_H_CONNECTION:
					if( IFS_Strnicmp( hp, "close", 5 ) == 0 )
						http->conn_close = FS_TRUE;	/* connection closed */
					break;
				case FS_H_X_DP_NEXT_URL:
					if( ! http->header->x_next_url )
						http->header->x_next_url = IFS_Strdup( hp );
					break;
				case FS_H_TRANSFER_ENCODING:
					if( IFS_Strnicmp( hp, "chunked", 7 ) == 0 )
					{
						http->chunked_encoding = FS_TRUE;	/* chunked encoding */
						http->header->content_length = 0;
					}
					break;
				default:
					break;
				}
			}
			IFS_Free( buf );	
		}
	}

}

/*
	Chunked-Body   = *chunk
					last-chunk
					trailer
					CRLF

	chunk          = chunk-size [ chunk-extension ] CRLF
					chunk-data CRLF
	chunk-size     = 1*HEX
	last-chunk     = 1*("0") [ chunk-extension ] CRLF

	chunk-extension= *( ";" chunk-ext-name [ "=" chunk-ext-val ] )
	chunk-ext-name = token
	chunk-ext-val  = token | quoted-string
	chunk-data     = chunk-size(OCTET)
	trailer        = *(entity-header CRLF)
*/
static FS_SINT4 FS_HttpProcessChunkSize( FS_HttpSession *http, FS_CHAR *buf, FS_SINT4 blen )
{
	FS_SINT4 i, len = 0, slen = 0;
	FS_CHAR *pChunkEnd = IFS_Strstr( buf, "\r\n" );

	if( blen < 3 ) return 0;
	if( pChunkEnd == FS_NULL ) return 0;
	
	for( i = 1; i < 16; i ++ )
	{
		if( buf[i] == 0x0D || buf[i] == 0x0A || buf[i] == ';' )
		{
			len = i;
			break;
		}
		else if ( buf[i] == 0x20 )
		{
			slen ++;
		}
	}

	if( len == 0 ) return 0;
	http->header->content_length += http->chunk_size;
	http->chunk_size = 0;
	for( i = 0; i < len - slen; i ++ )
	{
		if ( buf[i] >= 'a' )
		{
			http->chunk_size |= ((buf[i] - 'a' + 10) << ((len - slen - i - 1) * 4));
		}
		else if ( buf[i] >= 'A' )
		{
			http->chunk_size |= ((buf[i] - 'A' + 10) << ((len - slen - i - 1) * 4));
		}
		else
		{
			http->chunk_size |= ((buf[i] - '0') << ((len - slen - i - 1) * 4));
		}
	}
	FS_HTTP_TRACE2( "FS_HttpProcessChunkSize content-length = %d, chunk-size = %d", 
		http->header->content_length, http->chunk_size );
	if ( http->chunk_size == 0 )
	{
		/* last chunk */
		http->chunked_encoding = FS_FALSE;
		pChunkEnd = IFS_Strstr( pChunkEnd + 2, "\r\n" );
		if ( pChunkEnd == FS_NULL )
		{
			FS_HTTP_TRACE0( "FS_HttpProcessChunkSize ERROR. Invalid chunked transfer encoding." );
			return 0;
		}
	}
	return (FS_SINT4)(pChunkEnd - buf) + 2;
}

/*
	length := 0
	read chunk-size, chunk-extension (if any) and CRLF
	while (chunk-size > 0) {
		read chunk-data and CRLF
		append chunk-data to entity-body
		length := length + chunk-size
		read chunk-size and CRLF
	}
	read entity-header
	while (entity-header not empty) {
		append entity-header to existing header fields
		read entity-header
	}
	Content-Length := length
	Remove "chunked" from Transfer-Encoding
*/
static void FS_HttpProcessChunkedEncoding( FS_HttpSession *http, FS_BYTE *buf, FS_SINT4 rlen )
{
	FS_SINT4 hlen = 0;
	FS_SINT4 dlen = rlen;
	FS_SINT4 blen = http->rsp_data_len + rlen;
	FS_BYTE *data = buf;
	
	FS_HTTP_TRACE3("FS_HttpProcessChunkedEncoding blen=%d, content-length=%d, chunk-size=%d", 
		blen, http->header->content_length, http->chunk_size );
	if ( http->rsp_data_len != 0 
		&& http->chunk_size > 0
		&& blen <= http->header->content_length + http->chunk_size )
	{
		FS_HTTP_TRACE2("chunked data. Fragment chunk data. rsp_data_len=%d, dlen=%d", http->rsp_data_len, dlen );
		http->response_data( http->user_data, http, data, dlen );
		http->rsp_data_len += dlen;
		return;
	}
	
	if ( http->rsp_data_len != 0 
		&& http->chunk_size > 0
		&& blen >= http->header->content_length + http->chunk_size + 2
		&& http->chunk_size + http->header->content_length > http->rsp_data_len)
	{
		hlen = http->chunk_size + http->header->content_length - http->rsp_data_len;
		FS_HTTP_TRACE3("chunked data. Tailed chunk data. rsp_data_len=%d, dlen=%d, remain buffer len=%d",
			http->rsp_data_len, hlen, dlen - hlen - 2 );
		http->response_data( http->user_data, http, data, hlen );
		http->rsp_data_len += hlen;

		data = data + hlen + 2; /* including CRLF */
		dlen = dlen - hlen - 2;
	}

	hlen = FS_HttpProcessChunkSize( http, data, dlen );
	while( hlen > 0 && http->chunk_size > 0 ) {
		data += hlen;
		dlen -= hlen;
		if ( blen > http->header->content_length + http->chunk_size ) {
			FS_HTTP_TRACE2("chunked data. Full chunk data. rsp_data_len=%d, dlen=%d", http->rsp_data_len, http->chunk_size );
			http->response_data( http->user_data, http, data, http->chunk_size );
			http->rsp_data_len += http->chunk_size;

			data = data + http->chunk_size + 2;	/* including CRLF */
			dlen = dlen - http->chunk_size - 2;
		} else {
			FS_HTTP_TRACE2("chunked data. Fragment chunk data. rsp_data_len=%d, dlen=%d", http->rsp_data_len, dlen );
			http->response_data( http->user_data, http, data, dlen );
			http->rsp_data_len += dlen;
			break;
		}
		hlen = FS_HttpProcessChunkSize( http, data, dlen );
	}
}

static void FS_HttpProcessResponse( FS_HttpSession *http, FS_BYTE *buf, FS_SINT4 rlen )
{
	FS_SINT4 llen;
	FS_CHAR *p;
	FS_BYTE *data, *tmp;

	FS_HTTP_TRACE3( "FS_HttpProcessResponse state = %d, datalen = %d, headers = 0x%x", http->state, rlen, http->header );
	if( http->state != FS_HS_WAIT_RESPONSE )
		return;
	
	/* http header not finish */
	if( ! http->header )
	{
		tmp = http->buf;
		http->buf = IFS_Malloc( http->buf_len + rlen + 1 );
		if( http->buf )
		{
			if( http->buf_len > 0 )
				IFS_Memcpy( http->buf, tmp, http->buf_len );
			IFS_Memcpy( http->buf + http->buf_len, buf, rlen );
			http->buf[http->buf_len + rlen] = '\0';
			http->buf_len += rlen;
		}
		FS_SAFE_FREE( tmp );
	
		p = IFS_Strstr( (FS_CHAR *)http->buf, "\r\n\r\n" );
		if( p )
		{
			FS_HttpProcessHeaders( http );
			data = http->buf;
			http->buf = FS_NULL;
			if( http->status >= 100 && http->status < 200 )
			{
				FS_HttpFreeHeaders( http );
			}
			else
			{
				FS_HTTP_TRACE2( "response start. chunked = %d, status = %d", http->chunked_encoding, http->status );
				http->response_start( http->user_data, http, http->status, http->header );
				
				/* report http response data */
				if( (llen = (http->buf_len - (p + 4 - (FS_CHAR *)data))) > 0 )
				{
					if( http->state == FS_HS_WAIT_RESPONSE )
					{
						FS_HTTP_TRACE1( "response data. data after header. len = %d", llen );
						if( http->chunked_encoding )
						{
							FS_HttpProcessChunkedEncoding( http, p + 4, llen );
						}
						else
						{
							http->response_data( http->user_data, http, p + 4, llen );
							http->rsp_data_len += llen;
						}
					}
				}
			}
			IFS_Free( data );
			http->buf_len = 0;
		}
	}
	else	/* http headers is OK. here are http entry partial data */
	{
		if( http->state == FS_HS_WAIT_RESPONSE && rlen > 0 )
		{
			FS_HTTP_TRACE1( "response data. data len = %d", rlen );
			if( http->chunked_encoding )
			{
				FS_HttpProcessChunkedEncoding( http, buf, rlen );
			}
			else
			{
				http->response_data( http->user_data, http, buf, rlen );
				http->rsp_data_len += rlen;
			}
		}
	}
	
	if( http->state == FS_HS_WAIT_RESPONSE )
		FS_HttpJudgeResponseFinish( http );
}

static void FS_HttpSslEvent_CB( FS_SslHandle hSsl, FS_SslEvent ev, FS_UINT4 param )
{
	FS_SslBuf *pSslBuf;
	FS_HttpSession *http;

	http = FS_HttpGetSessionBySslHandler( hSsl );
	if( http == FS_NULL || ! http->in_use ) return;

	switch( ev )
	{
		case FS_SSL_EV_CONNECTED:
			http->conn_close = FS_FALSE;
			if( http->state == FS_HS_REQUEST || http->state == FS_HS_RESEND_REQUEST )
			{
				FS_HttpSendRequest( http, FS_TRUE );
			}
			break;
		case FS_SSL_EV_SEND_OK:
			if( http->state == FS_HS_REQUEST || http->state == FS_HS_RESEND_REQUEST )
			{
				if ( http->request_data_offset < http->request_data_len ) 
				{
					FS_HttpSendRequest( http, FS_FALSE );
				} 
				else
				{
					http->state = FS_HS_WAIT_RESPONSE;
				}
			}
			else if( http->state == FS_HS_POST_DATA )
			{
				if( http->post_data->offset < http->post_data->data_len )
				{
					FS_HttpSendPostData( http );
				}
				else
				{
					http->state = FS_HS_WAIT_RESPONSE;
				}
			}
			break;
		case FS_SSL_EV_DATA_ARRIVE:
			pSslBuf = (FS_SslBuf *)param;
			FS_HttpProcessResponse( http, pSslBuf->buf, pSslBuf->len );
			break;
		case FS_SSL_EV_ERROR:
		case FS_SSL_EV_CLOSE:
			if( http->state != FS_HS_NONE && http->state != FS_HS_IDLE )
			{
				if( http->rsp_data_len <= 0 )
					FS_HttpResponseFinish( http, FS_HTTP_ERR_NET, FS_TRUE );
				else
					FS_HttpResponseFinish( http, FS_HTTP_ERR_OK, FS_TRUE );
				http->state = FS_HS_NONE;
				if( ev == FS_SSL_EV_ERROR )
				{
					FS_SslDestroy( http->hSsl );
					http->hSsl = FS_NULL;
				}
			}
			break;
		default:
			break;
	}
}

static void FS_HttpSockEvent_CB( FS_UINT4 socket, FS_ISockEvent ev, FS_UINT4 param )
{
	FS_BYTE *buf;
	FS_SINT4 rlen;
	FS_HttpSession *http;

	http = FS_HttpGetSessionBySocketId( socket );
	
	if( http == FS_NULL || ! http->in_use ) {
		FS_HTTP_TRACE2( "FS_HttpSockEvent_CB ERROR. socket event in wrong state. event = %d, http = 0x%x", ev, http );
		return;
	}

	FS_HTTP_TRACE3( "FS_HttpSockEvent_CB event = %d, http state = %d, param = %d", ev, http->state, param );
	
	switch( ev )
	{
		case FS_ISOCK_CONNECT:
			http->conn_close = FS_FALSE;
			if( http->state == FS_HS_REQUEST || http->state == FS_HS_RESEND_REQUEST )
			{
				FS_HttpSendRequest( http, FS_TRUE );
			}
			break;
		case FS_ISOCK_SENDOK:
			if( http->state == FS_HS_REQUEST || http->state == FS_HS_RESEND_REQUEST )
			{
				if ( http->request_data_offset < http->request_data_len )
				{
					FS_HttpSendRequest( http, FS_FALSE );
				} 
				else 
				{
					http->state = FS_HS_WAIT_RESPONSE;
				}
			}
			else if( http->state == FS_HS_POST_DATA )
			{
				if( http->post_data->offset < http->post_data->data_len )
				{
					FS_HttpSendPostData( http );
				}
				else
				{
					http->state = FS_HS_WAIT_RESPONSE;
				}
			}
			break;
		case FS_ISOCK_READ:
			buf = FS_HttpReadNetData( http, &rlen );
			if( buf )
			{
				FS_HttpProcessResponse( http, buf, rlen );
				IFS_Free( buf );
			}
			break;
		case FS_ISOCK_CLOSE:
		case FS_ISOCK_ERROR:
			if( http->state == FS_HS_WAIT_RESPONSE )
			{
				/* try to read data. avoid pending buffer lost */
				if( FS_ISOCK_CLOSE == ev )
				{
					buf = FS_HttpReadNetData( http, &rlen );
					if( buf )
					{
						FS_HttpProcessResponse( http, buf, rlen );
						IFS_Free( buf );
					}
				}
				
				if( FS_ISOCK_ERROR == ev && http->can_retry )
				{
					/* we will allow to retry once more */
					if( http->rsp_data_len <= 0 )	/* server may close the connection. we connect server and retry */
					{
						http->can_retry = FS_FALSE;
						http->state = FS_HS_RESEND_REQUEST;
						FS_HttpConnectServer( http );
						return;
					}
				}
				
				if( http->rsp_data_len <= 0 )
					FS_HttpResponseFinish( http, FS_HTTP_ERR_NET, FS_TRUE );
				else
					FS_HttpResponseFinish( http, FS_HTTP_ERR_OK, FS_TRUE );
			}
			else
			{
				if( http->socket )
				{
					IFS_SocketClose( http->socket );
					http->socket = 0;
				}
				FS_HttpResponseFinish( http, FS_HTTP_ERR_NET, FS_TRUE );
			}
			break;
		default:
			break;
	}
}

FS_HttpHandle FS_HttpCreateHandle( void *userData, 
	FS_HttpResponseStartFunc start, FS_HttpResponseDataFunc data, FS_HttpResponseEndFunc end )
{
	FS_HttpSession *http = IFS_Malloc( sizeof(FS_HttpSession) );
	FS_CHAR *str;
	
	if( http )
	{
		IFS_Memset( http, 0, sizeof(FS_HttpSession) );
		http->in_use = FS_TRUE;
		http->user_data = userData;
		http->response_start = start;
		http->response_data = data;
		http->response_end = end;
		http->accept = IFS_Strdup( "*/*, text/*, image/*, audio/*, video/*, application/*" );
		//http->accept_encoding = IFS_Strdup( "gzip, deflate" );
		//http->accept_charset = IFS_Strdup( "UTF-8" );
		//http->accept_language = IFS_Strdup( "zh-cn" );
		str = IFS_GetUserAgent( );
		if( str ) http->user_agent = IFS_Strdup( str );
		str = IFS_GetUaProfile( );
		if( str ) http->ua_profile = IFS_Strdup( str );

		FS_ListAdd( &GFS_HttpSessionList, &http->list );
	}
	return http;
}

void FS_HttpDestroyHandle( FS_HttpHandle hHttp )
{
	FS_HttpSession *http = (FS_HttpSession *)hHttp;

	if( FS_HttpSessionExist(http) && http->in_use )
	{
		/* 
		 * 这里只把这个http session标注为无用状态，不能马上释放内存 
		 * 马上释放内存会导致回调函数里访问到野指针
		 * 这里的做法是，通过发送消息，等这个消息处理完，处理新消息时释放
		 * 以确保不会访问到野指针
		 */
		http->in_use = FS_FALSE;
		FS_FreeHttpSession( http );
		IFS_PostMessage( FS_MSG_UTIL_CALL, (FS_UINT4)FS_HttpFreeUselessSession );
	}
}

void FS_HttpRequest(FS_HttpHandle hHttp, FS_SockAddr *addr, FS_CHAR *method, 
	FS_CHAR *url, FS_HttpPostDataStruct * post_data, FS_CHAR *add_headers )
{
	FS_HttpSession *http = (FS_HttpSession *)hHttp;
	FS_HttpState state;
	FS_BOOL last_is_security;

	if( ! FS_HttpSessionExist(http) || ! http->in_use  ) return;

	FS_COPY_TEXT( http->method, method );
	last_is_security = http->is_security;
	if( addr && addr->host )
	{
		FS_COPY_TEXT( http->addr.host, addr->host );
		http->addr.port = addr->port;
	}
	FS_HttpProcessUrl( url, http );
	if( post_data )
	{
		FS_HttpSavePostData( http, post_data );
	}

	if( add_headers && add_headers[0] != 0 )
	{
		FS_COPY_TEXT( http->addtional_headers, add_headers );
	}

	if( last_is_security == http->is_security )
		state = http->state;
	else
		state = FS_HS_NONE;
	
	http->state = FS_HS_REQUEST;
	http->can_retry = FS_TRUE;
	
	FS_HTTP_TRACE3( "FS_HttpRequest method = %s, url = %s, state = %d", method, url, state);
	if( state == FS_HS_NONE || http->conn_close )
		FS_HttpConnectServer( http );
	else
		FS_HttpSendRequest( http, FS_TRUE );
}

void FS_HttpRequestCancel( FS_HttpHandle hHttp, FS_BOOL callback )
{
	FS_HttpSession *http = (FS_HttpSession *)hHttp;

	if( ! FS_HttpSessionExist(http) ) return;
	
	FS_HTTP_TRACE2( "FS_HttpRequestCancel http state = %d, callback = %d", http->state, callback );
	if( http->in_use && http->state != FS_HS_NONE )
	{
		if( http->socket )
		{
			IFS_SocketClose( http->socket );
			http->socket = 0;
		}
		FS_HttpResponseFinish( hHttp, FS_HTTP_ERR_USER_CANCEL, callback );
	}
}

void FS_HttpSetRequestProgFunc( FS_HttpHandle hHttp, FS_HttpRequestProgFunc progFunc )
{
	FS_HttpSession *http = (FS_HttpSession *)hHttp;
	
	if( ! FS_HttpSessionExist(http) || ! http->in_use ) return;
	
	http->request_prog = progFunc;
}
