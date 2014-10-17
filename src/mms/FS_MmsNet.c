#include "inc/FS_Config.h"

#ifdef FS_MODULE_MMS

#include "inc/mms/FS_MmsNet.h"
#include "inc/web/FS_Wsp.h"
#include "inc/web/FS_Http.h"
#include "inc/mms/FS_MmsCodec.h"
#include "inc/mms/FS_MmsConfig.h"
#include "inc/mms/FS_MmsFile.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_Mime.h"
#include "inc/util/FS_NetConn.h"
#include "inc/res/FS_TimerID.h"
#include "inc/util/FS_MemDebug.h"

extern void FS_MmsNewNotify( FS_SINT4 size, FS_UINT4 date, FS_UINT4 expiry, 
		FS_CHAR *from, FS_CHAR *subject, FS_CHAR *content_location, FS_CHAR *msg_id );

extern void FS_MmsAutoRecvComplete( FS_BOOL success, FS_CHAR *message_location, FS_UINT4 param );

#define FS_MMS_NET_CALLBACK( data, ev, param )		\
	if( (data)->callback ) (data)->callback( (data)->user_data, (ev), (FS_UINT4)(param) )

#define FS_MMS_NET_TASK_SEND			1
#define FS_MMS_NET_TASK_RECV			2
#define FS_MMS_NET_TASK_RESP			3
#define FS_MMS_NET_TASK_READ_REPORT		4

#define FS_MMS_PDU_LEN				1024

typedef struct FS_MmsNetData_Tag
{
	FS_MmsNetFunc			callback;
	void *					user_data;
	FS_WspHandle			wsp;
	FS_HttpHandle			http;
	
	FS_UINT4				timer_id;
	FS_BOOL					in_use;
	FS_BOOL 				send_notify_resp;
	FS_BOOL					finished;
	FS_BOOL					success;	/* operate success */
	FS_UINT1				task;
	FS_CHAR					file[FS_FILE_NAME_LEN];
	FS_SINT4				offset;
	FS_CHAR	*				str;
	FS_SINT4				size;
}FS_MmsNetData;

static FS_MmsNetData GFS_MmsNetData;

static void FS_MmsNetReset( void )
{
	FS_MmsNetData *data = &GFS_MmsNetData;
	
	if( data->in_use )
	{
		data->in_use = FS_FALSE;
		if( data->wsp )
		{
			FS_WspAbortReq( data->wsp, FS_FALSE );
			FS_WspDestroyHandle( data->wsp );
			data->wsp = FS_NULL;
		}

		if( data->http )
		{
			FS_HttpRequestCancel( data->http, FS_FALSE );
			FS_HttpDestroyHandle( data->http );
			data->http = FS_NULL;
		}

		if( data->timer_id )
		{
			IFS_StopTimer( data->timer_id );
			data->timer_id = 0;
		}
		
		FS_NetDisconnect( FS_APP_MMS );
		FS_SAFE_FREE( data->str );
 		IFS_Memset( data, 0, sizeof(FS_MmsNetData) );
	}
}

static void FS_MmsNetRecvReportResult_CB( void *dummy )
{
	FS_MmsNetData *data = &GFS_MmsNetData;
	
	if( data->in_use )
	{
		data->timer_id = 0;
		if( ! data->finished )
		{
			if( data->task == FS_MMS_NET_TASK_RECV )
			{
				data->finished = FS_TRUE;
				data->success = FS_TRUE;
				FS_MMS_NET_CALLBACK( data, FS_MMS_NET_OK, data->file );
			}
			else if( data->task == FS_MMS_NET_TASK_RESP )
			{
				data->finished = FS_TRUE;
				data->success = FS_TRUE;
				FS_MMS_NET_CALLBACK( data, FS_MMS_NET_OK, 0 );
			}
			else if( data->task == FS_MMS_NET_TASK_READ_REPORT )
			{
				data->finished = FS_TRUE;
				data->success = FS_TRUE;
			}
		}
		FS_MmsNetReset( );
	}
}

static void FS_MmsProcessSendResponse( FS_MmsNetData * data, FS_WspResultData * rData )
{
	FS_MmsEncHead head;
	data->finished = FS_TRUE;
	if( rData->status == FS_WSP_STATUS_OK )
	{
		FS_MmsCodecDecodeHead( &head, rData->data, rData->len );
		data->success = FS_TRUE;
		FS_MMS_NET_CALLBACK( data, FS_MMS_NET_OK, head.message_id );
		FS_MmsCodecFreeHead( &head );
	}
	else
	{
		FS_MMS_NET_CALLBACK( data, FS_MMS_NET_ERR_NET, rData->status );
	}
}

static void FS_MmsSendNotifyResp( FS_MmsNetData *data, FS_CHAR *tid, FS_UINT1 status )
{
	FS_SockAddr addr;
	FS_WspPostDataStruct pData;
	FS_HttpPostDataStruct hPData;
	FS_SINT4 size;
	FS_BYTE *buf;
	FS_MmsEncHead head;
	FS_CHAR *url;
	
	buf = IFS_Malloc( FS_MMS_PDU_LEN );
	if( buf )
	{
		IFS_Memset( buf, 0, FS_MMS_PDU_LEN );

		IFS_Memset( &head, 0, sizeof(FS_MmsEncHead) );
		head.message_type = FS_M_NOTIFY_RESP_IND;
		head.tid = tid;
		head.status = status;
		if( FS_MmsConfigGetAllowDeliveryReportFlag( ) )
			head.delivery_report_allow = FS_MMS_H_V_DELIVERY_REPORT_YES;
		else
			head.delivery_report_allow = FS_MMS_H_V_DELIVERY_REPORT_NO;
		size = FS_MmsCodecEncodeHead( buf, &head );
		url = FS_MmsConfigGetMmsCenterUrl( );

		if( data->wsp )
		{
			IFS_Memset( &pData, 0, sizeof(FS_WspPostDataStruct) );
			pData.content_type = "application/vnd.wap.mms-message";
			pData.data = buf;
			pData.data_len = size;

			FS_WspPostReq( data->wsp, url, &pData );
		}
		else if( data->http )
		{
			addr.port = FS_MmsConfigGetProxyPort( );
			addr.host = FS_MmsConfigGetProxyAddr( );
			
			IFS_Memset( &hPData, 0, sizeof(FS_HttpPostDataStruct) );
			hPData.content_type = "application/vnd.wap.mms-message";
			hPData.data = buf;
			hPData.data_len = size;

			FS_HttpRequest( data->http, &addr, "POST", url, &hPData, FS_NULL );
		}
		data->timer_id = IFS_StartTimer( FS_TIMER_ID_MMS_NET, 5000, FS_MmsNetRecvReportResult_CB, FS_NULL );
		IFS_Free( buf );
	}
}

static void FS_MmsProcessRecvResponse( FS_MmsNetData * data, FS_WspResultData * rData )
{
	FS_MmsEncHead head;

	if( data->send_notify_resp )
	{
		data->finished = FS_TRUE;
		data->success = FS_TRUE;
		FS_MMS_NET_CALLBACK( data, FS_MMS_NET_OK, data->file );
		return;
	}
	
	if( rData->status == FS_WSP_STATUS_OK )
	{
		if( ! data->file[0] )
		{
			if( rData->content_type == FS_WCT_APP_VND_WAP_MMS 
				|| ( FS_STR_NI_EQUAL(rData->mime_content, "application/vnd.wap.mms-message", 31)) )
			{
				FS_GetGuid( data->file );
				IFS_Strcat( data->file, ".mms" );
			}
			else
			{
				data->finished = FS_TRUE;
				FS_MMS_NET_CALLBACK( data, FS_MMS_NET_ERR_UNKNOW, 0 );
				return;
			}
		}
		FS_FileWrite( FS_DIR_MMS, data->file, data->offset, rData->data, rData->len );
		data->offset += rData->len;
		FS_MMS_NET_CALLBACK( data, FS_MMS_NET_RECVING, data->offset );
		
		if( rData->done )
		{
			if( FS_MmsCodecDecodeFileHead( &head, data->file ) && head.tid )
			{
				FS_MmsSendNotifyResp( data, head.tid, FS_MMS_H_V_STATUS_RETRIEVED );
				FS_MmsCodecFreeHead( &head );
				data->send_notify_resp = FS_TRUE;
			}
			else
			{
				data->finished = FS_TRUE;
				data->success = FS_TRUE;
				FS_MMS_NET_CALLBACK( data, FS_MMS_NET_OK, data->file );
			}
		}
	}
	else
	{
		data->finished = FS_TRUE;
		FS_MMS_NET_CALLBACK( data, FS_MMS_NET_ERR_NET, rData->status );
	}
}

static void FS_MmsWspEventHandler( void *user_data, FS_WspHandle hWsp, FS_SINT4 event, FS_UINT4 param )
{
	FS_MmsNetData *data = (FS_MmsNetData *)user_data;
	FS_WspResultData *rData;
	
	switch( event )
	{
		case FS_WSP_EV_CONNECT_CNF:
			FS_MMS_NET_CALLBACK( data, FS_MMS_NET_CONNECTED, 0 );
			break;
		case FS_WSP_EV_INVOKE_IND:
			if( data->task == FS_MMS_NET_TASK_SEND )
			{
				FS_MMS_NET_CALLBACK( data, FS_MMS_NET_SENDING, param );
			}
			break;
		case FS_WSP_EV_RESULT_IND:
			rData = (FS_WspResultData *)param;
			if( data->task == FS_MMS_NET_TASK_SEND )
			{
				FS_MmsProcessSendResponse( data, rData );
			}
			else if( data->task == FS_MMS_NET_TASK_RECV )
			{
				FS_MmsProcessRecvResponse( data, rData );
			}
			else if( data->task == FS_MMS_NET_TASK_RESP )
			{
				data->success = FS_TRUE;
				data->finished = FS_TRUE;
			}
			else if( data->task == FS_MMS_NET_TASK_READ_REPORT )
			{
				data->success = FS_TRUE;
				data->finished = FS_TRUE;
			}
			break;
		case FS_WSP_ERR_MEMORY:
			if( ! data->finished )
			{
				data->finished = FS_TRUE;
				FS_MMS_NET_CALLBACK( data, FS_MMS_NET_ERR_MEMORY, 0 );
			}
			break;
		case FS_WSP_ERR_NET:
			if( ! data->finished )
			{
				data->finished = FS_TRUE;
				FS_MMS_NET_CALLBACK( data, FS_MMS_NET_ERR_NET, 0 );
			}
			break;
		case FS_WSP_ERR_UNKNOW:
			if( ! data->finished )
			{
				data->finished = FS_TRUE;
				FS_MMS_NET_CALLBACK( data, FS_MMS_NET_ERR_UNKNOW, 0 );
			}
			break;
		default:
			if( event < 0 )
			{
				if( ! data->finished )
				{
					data->finished = FS_TRUE;
					FS_MMS_NET_CALLBACK( data, FS_MMS_NET_ERR_UNKNOW, 0 );
				}
			}
			break;
	}

	if( data->finished && data->in_use )
	{
		FS_MmsNetReset( );
	}
}

static void FS_MmsHttpResponseStart( void *user_data, FS_HttpHandle hHttp, FS_SINT4 status, FS_HttpHeader *headers )
{
	FS_MmsNetData *data = (FS_MmsNetData *)user_data;
	if( status != FS_HTTP_OK || ! FS_STR_NI_EQUAL(headers->content_type, "application/vnd.wap.mms-message", 31) )
	{
		if( ! data->finished )
		{
			if( data->task == FS_MMS_NET_TASK_SEND || ! data->send_notify_resp )
			{
				/* here, we ignore the mms-notify-resp response status. */
				data->finished = FS_TRUE;
				FS_MMS_NET_CALLBACK( data, FS_MMS_NET_ERR_NET, status );
			}
		}
	}

	if( data->task == FS_MMS_NET_TASK_RECV && ! data->finished && ! data->send_notify_resp )
	{
		/* mms recv response start. create a file here */
		FS_MMS_NET_CALLBACK( data, FS_MMS_NET_CONNECTED, 0 );
		FS_GetGuid( data->file );
		IFS_Strcat( data->file, ".mms" );
	}
}

static void FS_MmsHttpResponseData( void *user_data, FS_HttpHandle hHttp, FS_BYTE *rdata, FS_SINT4 rdata_len )
{
	FS_MmsEncHead head;
	FS_MmsNetData *data = (FS_MmsNetData *)user_data;

	if( data->finished )
		return;
	
	if( data->task == FS_MMS_NET_TASK_SEND )
	{
		/* mms send response */
		data->finished = FS_TRUE;
		FS_MmsCodecDecodeHead( &head, rdata, rdata_len );
		data->success = FS_TRUE;
		FS_MMS_NET_CALLBACK( data, FS_MMS_NET_OK, head.message_id );
		FS_MmsCodecFreeHead( &head );
	}
	else if( data->task == FS_MMS_NET_TASK_RECV )
	{
		if( ! data->send_notify_resp )
		{
			/* mms recv response */
			FS_FileWrite( FS_DIR_MMS, data->file, data->offset, rdata, rdata_len );
			data->offset += rdata_len;
			FS_MMS_NET_CALLBACK( data, FS_MMS_NET_RECVING, data->offset );
		}
		else
		{
			if( ! data->finished )
			{
				data->finished = FS_TRUE;
				data->success = FS_TRUE;
				FS_MMS_NET_CALLBACK( data, FS_MMS_NET_OK, data->file );
			}
		}
	}
	else if( data->task == FS_MMS_NET_TASK_RESP )
	{
		if( ! data->finished )
		{
			data->finished = FS_TRUE;
			data->success = FS_TRUE;
			FS_MMS_NET_CALLBACK( data, FS_MMS_NET_OK, data->file );
		}
	}
	else if( data->task == FS_MMS_NET_TASK_READ_REPORT )
	{
		if( ! data->finished )
		{
			data->finished = FS_TRUE;
			data->success = FS_TRUE;
		}
	}
}

static void FS_MmsHttpResponseEnd( void *user_data, FS_HttpHandle hHttp, FS_SINT4 error_code )
{
	FS_MmsEncHead head;
	FS_MmsNetData *data = (FS_MmsNetData *)user_data;

	if( ! data->finished )
	{
		if( data->task == FS_MMS_NET_TASK_SEND )
		{
			/* mms send response end */
			data->finished = FS_TRUE;
			FS_MMS_NET_CALLBACK( data, FS_MMS_NET_ERR_UNKNOW, error_code );
		}
		else if( data->task == FS_MMS_NET_TASK_RECV )
		{
			/* mms recv response end */
			if( ! data->send_notify_resp )
			{
				if( FS_MmsCodecDecodeFileHead( &head, data->file ) && head.tid )
				{
					FS_MmsSendNotifyResp( data, head.tid, FS_MMS_H_V_STATUS_RETRIEVED );
					FS_MmsCodecFreeHead( &head );
					data->send_notify_resp = FS_TRUE;
				}
				else
				{
					data->finished = FS_TRUE;
					data->success = FS_TRUE;
					FS_MMS_NET_CALLBACK( data, FS_MMS_NET_OK, data->file );
				}
			}
			else
			{
				data->finished = FS_TRUE;
				data->success = FS_TRUE;
				FS_MMS_NET_CALLBACK( data, FS_MMS_NET_OK, data->file );
			}
	 	}
		else if( data->task == FS_MMS_NET_TASK_RESP )
		{
			/* mms send response end */
			data->finished = FS_TRUE;
			FS_MMS_NET_CALLBACK( data, FS_MMS_NET_ERR_UNKNOW, error_code );
		}
		else if( data->task == FS_MMS_NET_TASK_READ_REPORT )
		{
			/* mms send response end */
			data->success = FS_TRUE;
			data->finished = FS_TRUE;
		}
	}
	
	if( data->finished )
	{
		FS_MmsNetReset( );
	}
}

void FS_MmsHttpSendPrograss( void *user_data, FS_HttpHandle hHttp, FS_SINT4 offset )
{
	FS_MmsNetData *data = (FS_MmsNetData *)user_data;

	if( offset == 0 )
	{
		FS_MMS_NET_CALLBACK( data, FS_MMS_NET_CONNECTED, 0 );
	}
	else
	{
		FS_MMS_NET_CALLBACK( data, FS_MMS_NET_SENDING, offset );
	}
}

void FS_MmsNetCancel( void )
{
	FS_MmsNetData *data = &GFS_MmsNetData;
	
	if( data->in_use )
	{
		if( data->file[0] && ! data->success )
			FS_FileDelete( FS_DIR_MMS, data->file );
		
		FS_MmsNetReset( );
	}
}

static void FS_MmsNetSend( FS_MmsNetData *data )
{
	FS_SockAddr addr;
	FS_WspPostDataStruct pData;
	FS_HttpPostDataStruct hPData;
	FS_CHAR *url = FS_MmsConfigGetMmsCenterUrl( );
	
	if( data->wsp )
	{
		IFS_Memset( &pData, 0, sizeof(FS_WspPostDataStruct) );
		pData.content_type = "application/vnd.wap.mms-message";
		pData.data_is_file = FS_TRUE;
		pData.data = data->str;
		pData.data_len = data->size;
	
		FS_WspPostReq( data->wsp, url, &pData );
	}
	else if( data->http )
	{
		IFS_Memset( &hPData, 0, sizeof(FS_HttpPostDataStruct) );
		hPData.content_type = "application/vnd.wap.mms-message";
		hPData.data_is_file = FS_TRUE;
		hPData.data = data->str;
		hPData.data_len = data->size;
		
		addr.port = FS_MmsConfigGetProxyPort( );
		addr.host = FS_MmsConfigGetProxyAddr( );
		FS_HttpSetRequestProgFunc( data->http, FS_MmsHttpSendPrograss );
		FS_HttpRequest( data->http, &addr, "POST", url, &hPData, FS_NULL );
	}
	else
	{
		FS_MMS_NET_CALLBACK( data, FS_MMS_NET_ERR_UNKNOW, 0 );
		FS_MmsNetReset( );
	}
}

static void FS_MmsNetRecv( FS_MmsNetData *data )
{
	FS_CHAR *url;
	FS_SockAddr addr;

#ifdef FS_PLT_WIN
	/* for window version debug */
	url = "http://211.138.147.4/GSbhxLn9yjeA";
#else
	url = data->str;
#endif
	
	if( data->wsp )
	{
		FS_WspGetReq( data->wsp, url );
	}
	else if( data->http )
	{
		addr.port = FS_MmsConfigGetProxyPort( );
		addr.host = FS_MmsConfigGetProxyAddr( );
		FS_HttpRequest( data->http, &addr, "GET", url, FS_NULL, FS_NULL );
	}
	else
	{
		FS_MMS_NET_CALLBACK( data, FS_MMS_NET_ERR_UNKNOW, 0 );
		FS_MmsNetReset( );
	}
}

static void FS_MmsNetSendReadReport( FS_MmsNetData *data )
{
	FS_SockAddr addr;
	FS_WspPostDataStruct pData;
	FS_HttpPostDataStruct hPData;
	FS_CHAR *url;

	url = FS_MmsConfigGetMmsCenterUrl( );
	if( data->wsp )
	{
		IFS_Memset( &pData, 0, sizeof(FS_WspPostDataStruct) );
		pData.content_type = "application/vnd.wap.mms-message";
		pData.data = data->str;
		pData.data_len = data->size;

		FS_WspPostReq( data->wsp, url, &pData );
	}
	else if( data->http )
	{
		addr.port = FS_MmsConfigGetProxyPort( );
		addr.host = FS_MmsConfigGetProxyAddr( );
		
		IFS_Memset( &hPData, 0, sizeof(FS_HttpPostDataStruct) );
		hPData.content_type = "application/vnd.wap.mms-message";
		hPData.data = data->str;
		hPData.data_len = data->size;

		FS_HttpRequest( data->http, &addr, "POST", url, &hPData, FS_NULL );
	}
	data->timer_id = IFS_StartTimer( FS_TIMER_ID_MMS_NET, 10000, FS_MmsNetRecvReportResult_CB, FS_NULL );
}

static void FS_MmsNetConnCallback( FS_MmsNetData *data, FS_BOOL ok )
{
	FS_SockAddr addr;
	if( ok )
	{
		FS_MMS_NET_CALLBACK( data, FS_MMS_NET_CONN_OK, 0 );
		if( FS_MmsConfigGetProtocol() == FS_MMS_WSP )
		{
			addr.port = FS_MmsConfigGetProxyPort( );
			addr.host = FS_MmsConfigGetProxyAddr( );
			data->wsp = FS_WspCreateHandle( &addr, data, FS_MmsWspEventHandler );
		}
		else
		{
			data->http = FS_HttpCreateHandle( data, FS_MmsHttpResponseStart, 
				FS_MmsHttpResponseData, FS_MmsHttpResponseEnd );
		}

		if( data->wsp == FS_NULL && data->http == FS_NULL )
		{
			FS_MMS_NET_CALLBACK( data, FS_MMS_NET_ERR_UNKNOW, 0 );
			FS_MmsNetReset( );
			return;
		}
		
		if( data->task == FS_MMS_NET_TASK_RESP )
		{
			FS_MmsSendNotifyResp( data, data->str, FS_MMS_H_V_STATUS_DEFERRED );
		}
		else if( data->task == FS_MMS_NET_TASK_SEND )
		{
			FS_MmsNetSend( data );
		}
		else if( data->task == FS_MMS_NET_TASK_RECV )
		{
			FS_MmsNetRecv( data );
		}
		else if( data->task == FS_MMS_NET_TASK_READ_REPORT )
		{
			FS_MmsNetSendReadReport( data );
		}
		else
		{
			FS_MMS_NET_CALLBACK( data, FS_MMS_NET_ERR_UNKNOW, 0 );
			FS_MmsNetReset( );
		}
	}
	else
	{
		FS_MMS_NET_CALLBACK( data, FS_MMS_NET_ERR_CONN, 0 );
		FS_MmsNetReset( );
	}
}

/* this mms file must be abs file path name */
FS_SINT4 FS_MmsSend( FS_CHAR *file, FS_MmsNetFunc callback, void *user_data )
{
	FS_MmsNetData *data;
	FS_SINT4 size = 0;
	
	if( file ) size = FS_FileGetSize( -1, file );
	if( size <= 0 ) return FS_MMS_NET_ERR_FILE;
	
	data = &GFS_MmsNetData;
	if( data->in_use ){
		FS_MmsNetCancel( );
	}
	IFS_Memset( data, 0, sizeof(FS_MmsNetData) );
	data->task = FS_MMS_NET_TASK_SEND;
	data->in_use = FS_TRUE;
	data->callback = callback;
	data->user_data = user_data;
	data->size = size;
	data->str = IFS_Strdup( file );
	FS_NetConnect( FS_MmsConfigGetApn(), FS_MmsConfigGetUserName(), FS_MmsConfigGetPassword(),
		FS_MmsNetConnCallback, FS_APP_MMS, FS_TRUE, data );
	return FS_MMS_NET_OK;
}

FS_SINT4 FS_MmsRecv( FS_CHAR *url, FS_MmsNetFunc callback, void * user_data )
{
	FS_MmsNetData *data;
	FS_SINT4 size = 0;

	data = &GFS_MmsNetData;
	if( data->in_use ){
		FS_MmsNetCancel( );
	}
	
	IFS_Memset( data, 0, sizeof(FS_MmsNetData) );
	data->task = FS_MMS_NET_TASK_RECV;
	data->in_use = FS_TRUE;
	data->callback = callback;
	data->user_data = user_data;
	data->str = IFS_Strdup( url );
	FS_NetConnect( FS_MmsConfigGetApn(), FS_MmsConfigGetUserName(), FS_MmsConfigGetPassword(),
		FS_MmsNetConnCallback, FS_APP_MMS, FS_TRUE, data );
	return FS_MMS_NET_OK;
}

static FS_MmsReadReportResult( FS_CHAR *mmsfile, FS_SINT4 ev, FS_UINT4 param )
{
	FS_MmsNetData *data;
	
	data = &GFS_MmsNetData;
	
	if( ev == FS_MMS_NET_OK )
	{
		FS_FileDelete( FS_DIR_MMS, mmsfile );
		IFS_Free( mmsfile );
	}
	else if( ev < 0 )
	{
		FS_FileDelete( FS_DIR_MMS, mmsfile );
		IFS_Free( mmsfile );
	}
}

void FS_MmsSendReadReportPdu( FS_CHAR *msg_id, FS_CHAR *to, FS_BOOL bRead )
{
	FS_MmsNetData *data = &GFS_MmsNetData;
	FS_BYTE *buf;
	FS_MmsEncHead head;
	
	if( data->in_use ) return;
	
	/* send a pdu read report M_READ_REC_IND */
	buf = IFS_Malloc( FS_MMS_PDU_LEN );
	if( buf )
	{
		data->in_use = FS_TRUE;
		data->task = FS_MMS_NET_TASK_READ_REPORT;
		IFS_Memset( &head, 0, sizeof(FS_MmsEncHead) );
		head.message_type = FS_M_READ_REC_IND;
		head.message_id = IFS_Strdup( msg_id );
		head.to = IFS_Strdup( to );
		head.date = FS_GetSeconds( IFS_GetTimeZone() );
		if( bRead )
			head.read_status = FS_MMS_H_V_READ;
		else
			head.read_status = FS_MMS_H_V_DEL_WITHOUT_READ;
		data->size = FS_MmsCodecEncodeHead( buf, &head );
		data->str = buf;
		FS_NetConnect( FS_MmsConfigGetApn(), FS_MmsConfigGetUserName(), FS_MmsConfigGetPassword(),
			FS_MmsNetConnCallback, FS_APP_MMS, FS_FALSE, data );
	}
}

void FS_MmsSendReadReportMms( FS_CHAR *to, FS_CHAR *subject, FS_CHAR *content )
{
	FS_MmsNetData *data = &GFS_MmsNetData;
	FS_MmsFile *pMmsFile;
	FS_SINT4 len;
	FS_CHAR absfile[FS_MAX_PATH_LEN], file[FS_FILE_NAME_LEN], *cid, *rptFile = "readreport.txt";
	
	if( data->in_use || content == FS_NULL || to == FS_NULL ) return;
	
	/* send a normal mms read report */
	pMmsFile = FS_CreateMmsFile( );
	if( pMmsFile )
	{
		pMmsFile->data.head.message_class = FS_MMS_H_V_CLASS_AUTO;
		pMmsFile->data.head.read_report = FS_MMS_H_V_READ_REPORT_NO;
		pMmsFile->data.head.delivery_report = FS_MMS_H_V_DELIVERY_REPORT_NO;
		pMmsFile->data.head.to = IFS_Strdup( to );
		if( subject ) pMmsFile->data.head.subject = IFS_Strdup( subject );
		FS_MmsFileAddFrame( pMmsFile, 0 );
		len = IFS_Strlen( content );
		FS_FileWrite( FS_DIR_TMP, rptFile, 0, content, len );
		cid = FS_MmsCodecCreateEntry( &pMmsFile->data, rptFile, len );
		FS_SmilAddFrameText( pMmsFile->smil, 1, cid );
		FS_GetGuid( file );
		IFS_Strcat( file, ".mms" );
		FS_MmsEncodeFile( file, pMmsFile );
		FS_DestroyMmsFile( pMmsFile );
		
		FS_GetAbsFileName( FS_DIR_MMS, file, absfile );
		/* will remenber this file name. and delete it when send complete */
		rptFile = IFS_Strdup( file );	
		if( FS_MmsSend( absfile, FS_MmsReadReportResult, rptFile ) != FS_MMS_NET_OK )
		{
			IFS_Free( rptFile );
			FS_FileDelete( FS_DIR_MMS, file );
		}
	}
}

static void FS_MmsAutoRecvResult( void *user_data, FS_SINT4 ev, FS_UINT4 param )
{
	FS_MmsNetData *data;
	
	data = &GFS_MmsNetData;
	
	if( ev == FS_MMS_NET_OK )
	{
		FS_MmsAutoRecvComplete( FS_TRUE, data->str, param );
	}
	else if( ev < 0 )
	{
		FS_MmsAutoRecvComplete( FS_FALSE, data->str, ev );
	}
}

static void FS_MmsNotificationResp( FS_MmsEncHead *head )
{
	FS_MmsNetData *data;
	FS_BOOL bAutoRetr;

	data = &GFS_MmsNetData;
	bAutoRetr = FS_MmsConfigGetAutoRecvFlag( );
	/* mms net busy */
	if( data->in_use )
	{
		if( bAutoRetr ) IFS_MmsAutoRecvNotify( FS_FALSE, head->from, head->subject );
		return;
	}

	data->in_use = FS_TRUE;
	if( bAutoRetr && head->message_size <= (FS_UINT4)IFS_GetMaxMmsSize() && !FS_MmsIsFull(head->message_size) )
	{
		data->str = IFS_Strdup( head->content_location );
		data->task = FS_MMS_NET_TASK_RECV;
		data->callback = FS_MmsAutoRecvResult;
	}
	else
	{
		data->str = IFS_Strdup( head->tid );
		data->task = FS_MMS_NET_TASK_RESP;
		if( bAutoRetr ) IFS_MmsAutoRecvNotify( FS_FALSE, head->from, head->subject );
	}
	
	FS_NetConnect( FS_MmsConfigGetApn(), FS_MmsConfigGetUserName(), FS_MmsConfigGetPassword(),
		FS_MmsNetConnCallback, FS_APP_MMS, FS_FALSE, data);
}

void FS_MmsNotificationInd( FS_UINT1 tid, FS_BYTE *data, FS_SINT4 len )
{
	FS_SINT4 rlen;
	FS_MmsEncHead head;
	rlen = FS_MmsCodecDecodeHead( &head, data, len );
	if( rlen > 0 )
	{	
		if( IFS_SystemRuningBackgroud() )
		{
			FS_MmsConfigInit( );
		}
		
		if( head.message_class != FS_MMS_H_V_CLASS_ADDS || FS_MmsConfigGetAllowAdsFlag() )
		{
			if( head.message_type == FS_M_DELIVERY_IND )
			{
				FS_SAFE_FREE( head.content_location );
			}
			
			FS_MmsNewNotify( head.message_size, head.date, head.expiry, head.from, 
				head.subject, head.content_location, head.message_id );
			/* here. we may handle some response. */
			if( head.content_location )
			{
				FS_MmsNotificationResp( &head );
			}
		}
	}

	FS_MmsCodecFreeHead( &head );
}

#endif	//FS_MODULE_MMS

