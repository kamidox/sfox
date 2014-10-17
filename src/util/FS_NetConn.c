#include "inc/util/FS_NetConn.h"
#include "inc/gui/FS_Gui.h"
#include "inc/res/FS_Res.h"
#include "inc/inte/FS_Inte.h"
#include "inc/inte/FS_ISocket.h"
#include "inc/util/FS_File.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_MemDebug.h"

#define _NETCONN_DEBUG

#ifdef _NETCONN_DEBUG
#define FS_NET_TRACE0(a)				FS_TRACE0( "[NET]" a "\r\n")
#define FS_NET_TRACE1(a,b)				FS_TRACE1( "[NET]" a "\r\n", b)
#define FS_NET_TRACE2(a,b,c)			FS_TRACE2( "[NET]" a "\r\n", b, c)
#define FS_NET_TRACE3(a,b,c,d)			FS_TRACE3( "[NET]" a "\r\n", b, c, d)
#else
#define FS_NET_TRACE0(a)
#define FS_NET_TRACE1(a,b)
#define FS_NET_TRACE2(a,b,c)
#define FS_NET_TRACE3(a,b,c,d)
#endif

typedef enum FS_NetState_Tag
{
	FS_NET_NONE = 0,
	FS_NET_INITING,
	FS_NET_INITED
}FS_NetState;

typedef struct FS_NetConnState_Tag
{
	FS_NetState				state;
	FS_NetConnCallback		callback;
	void *					user_data;
	FS_UINT4				timer_id;
	FS_SINT4				retry_times;
	
	FS_CHAR					apn[FS_URL_LEN];
	FS_CHAR					user[FS_URL_LEN];
	FS_CHAR					pass[FS_URL_LEN];

	FS_UINT4				apps;
	FS_UINT4				cur_app;

	FS_BOOL					cb_result;
}FS_NetConnState;

static FS_NetConnState GFS_NetConnState;

static void FS_NetPostCallback( void )
{
	FS_NetConnState *connSts = &GFS_NetConnState;

	if ( connSts->callback ) {
		FS_NetConnCallback cb = connSts->callback;
		void *user_data = connSts->user_data;
		
		connSts->callback = FS_NULL;
		connSts->user_data = FS_NULL;
		cb( user_data, connSts->cb_result );
	}
}

void FS_NetConnect( FS_CHAR *apn, FS_CHAR *user, FS_CHAR *pass, FS_NetConnCallback callback, 
	FS_UINT4 app, FS_BOOL highPri, void *user_data )
{
	FS_NetConnState *connSts = &GFS_NetConnState;
	
	FS_NET_TRACE2( "FS_NetConnect state=%d, app=0x%x", connSts->state, connSts->apps );
	if( connSts->state == FS_NET_NONE )
	{
		connSts->callback = callback;
		connSts->user_data = user_data; 
		connSts->state = FS_NET_INITING;
		connSts->retry_times = 3;
		IFS_Strncpy( connSts->apn, apn, sizeof(connSts->apn) - 1 );
		IFS_Strncpy( connSts->user, user, sizeof(connSts->user) - 1 );
		IFS_Strncpy( connSts->pass, pass, sizeof(connSts->pass) - 1 );
		connSts->cur_app = app;
		IFS_NetDisconnect( );
		IFS_NetConnect( apn, user, pass );
	}
	else if( connSts->state == FS_NET_INITED )
	{
		if( callback ){
			connSts->callback = callback;
			connSts->user_data = user_data; 
			if( apn && IFS_Stricmp(apn, connSts->apn) == 0 ){
				FS_NET_TRACE1( "FS_NetConnect The same APN(%s) is already active", apn );
				connSts->apps |= app;
				connSts->cb_result = FS_TRUE;
				IFS_PostMessage( FS_MSG_UTIL_CALL, (FS_UINT4)FS_NetPostCallback );
			}else{
				connSts->cb_result = FS_FALSE;
				IFS_PostMessage( FS_MSG_UTIL_CALL, (FS_UINT4)FS_NetPostCallback );
			}
		}
	}
	else
	{
		connSts->cb_result = FS_FALSE;
		IFS_PostMessage( FS_MSG_UTIL_CALL, (FS_UINT4)FS_NetPostCallback );
	}
}

static void FS_NetConnRetry_CB( void *dummy )
{
	FS_NetConnState *connSts = &GFS_NetConnState;
	if( connSts->state == FS_NET_NONE )
	{
		connSts->timer_id = 0;
		FS_NET_TRACE1( "FS_NetConnRetry_CB retry_times=%d", connSts->retry_times );
		connSts->state = FS_NET_INITING;
		IFS_NetConnect( connSts->apn, connSts->user, connSts->pass );
	}
	else
	{
		FS_NET_TRACE1( "FS_NetConnRetry_CB ERROR, state=%d", connSts->state );
	}
}

/* when socket init failed. show alert msg to user */
void FS_NetConnResultInd( FS_BOOL bResult )
{
	FS_NetConnState *connSts = &GFS_NetConnState;

	FS_NET_TRACE3( "FS_NetConnResultInd state=%d, result=%d, cur_app=0x%x", 
		connSts->state, bResult, connSts->cur_app );

	if ( connSts->state == FS_NET_NONE ) {
		return;
	}

	if( ! bResult )
	{
		connSts->state = FS_NET_NONE;
		/* conn failed. we may retry */
		if ( connSts->retry_times > 0 )
			connSts->retry_times --;
		if( connSts->retry_times > 0 ){
			/* we will retry after 3 seconds */
			if ( connSts->timer_id ) {
				IFS_StopTimer( connSts->timer_id );
				connSts->timer_id = 0;
			}
			connSts->timer_id = IFS_StartTimer( FS_TIMER_ID_CONN_RETRY, 3000, FS_NetConnRetry_CB, FS_NULL );
			return;
		}else{
			/* failed to connect gprs */
			IFS_Memset( connSts->apn, 0, sizeof(connSts->apn) );
			IFS_Memset( connSts->user, 0, sizeof(connSts->user) );
			IFS_Memset( connSts->pass, 0, sizeof(connSts->pass) );
			connSts->cur_app = 0;
		}
	}
	else if ( connSts->state == FS_NET_INITING )
	{
		connSts->apps |= connSts->cur_app;
		connSts->cur_app = 0;
		connSts->retry_times = 0;
		connSts->state = FS_NET_INITED;
	}

	if( connSts->callback )
	{
		connSts->cb_result = FS_TRUE;
		IFS_PostMessage( FS_MSG_UTIL_CALL, (FS_UINT4)FS_NetPostCallback );
	}
}

void FS_NetDisconnect( FS_UINT4 app )
{
	FS_NetConnState *connSts = &GFS_NetConnState;

	connSts->apps &= (~app);
	FS_NET_TRACE1( "FS_NetDisconnect apps=0x%x", connSts->apps );
	/* other application use this connection */
	if( connSts->apps != 0 ) return;
	
	connSts->state = FS_NET_NONE;
	if( connSts->timer_id )
	{
		IFS_StopTimer( connSts->timer_id );
		connSts->timer_id = 0;
	}
	IFS_Memset( connSts, 0, sizeof(FS_NetConnState) );
	IFS_NetDisconnect( );
}

