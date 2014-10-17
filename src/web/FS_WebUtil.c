#include "inc/FS_Config.h"

#ifdef FS_MODULE_WEB

#include "inc/FS_Config.h"
#include "inc/web/FS_WebUtil.h"
#include "inc/web/FS_Http.h"
#include "inc/web/FS_Wsp.h"
#include "inc/web/FS_WebDoc.h"
#include "inc/web/FS_WebConfig.h"
#include "inc/web/FS_History.h"
#include "inc/web/FS_Cookie.h"
#include "inc/util/FS_File.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_Base64.h"
#include "inc/res/FS_Res.h"
#include "inc/util/FS_NetConn.h"
#include "inc/util/FS_MemDebug.h"

#define _WEB_DEBUG

#ifdef _WEB_DEBUG
#define FS_WEB_TRACE0(a)				FS_TRACE0( "[WEB]" a "\r\n")
#define FS_WEB_TRACE1(a,b)				FS_TRACE1( "[WEB]" a "\r\n", b)
#define FS_WEB_TRACE2(a,b,c)			FS_TRACE2( "[WEB]" a "\r\n", b, c)
#define FS_WEB_TRACE3(a,b,c,d)			FS_TRACE3( "[WEB]" a "\r\n", b, c, d)
#else
#define FS_WEB_TRACE0(a)
#define FS_WEB_TRACE1(a,b)
#define FS_WEB_TRACE2(a,b,c)
#define FS_WEB_TRACE3(a,b,c,d)
#endif

#define FS_WEB_WIN_CUR_VIEWPORT( param )	( FS_WebWinGetFocusWebWgtPos(param->win) )

#define FS_WEB_EV_GET						0x01
#define FS_WEB_EV_POST						0x02

#define FS_WEB_POST_EVENT( ev, param )		( IFS_PostMessage( (FS_MSG_WEB_EVENT | ev), (FS_UINT4)param))

#define FS_METHOD_GET			0
#define FS_METHOD_POST			1

#define FS_WEB_REQ_STS_NONE			0
#define FS_WEB_REQ_STS_SEND_REQ		1
#define FS_WEB_REQ_STS_RECV_PAGE	2
#define FS_WEB_REQ_STS_GET_IMG		3

typedef struct FS_WebEventParam_Tag
{
	FS_Window *			win;
	FS_CHAR *			url;
	FS_BOOL				send_referer;
	FS_CHAR *			data;
	FS_SINT4			dlen;
}FS_WebEventParam;

typedef struct FS_WebRequest_Tag
{
	FS_UINT4			timer_id;
	FS_SINT4			seconds;
	FS_SINT4			state;
	FS_SINT2			img_index;
	FS_SINT2			img_total;
}FS_WebRequest;

typedef struct FS_NetParam_Tag
{
	FS_Window *			win;
	FS_SaxHandle		hsax;

	FS_CHAR *			url;
	FS_CHAR *			parent_url;
	FS_WebTask *		task;
	FS_SINT4			offset;
	FS_SINT4			content_len;
	
	FS_CHAR				file[FS_FILE_NAME_LEN];
	FS_CHAR				filename[FS_FILE_NAME_LEN];		/* filename get from url */
	FS_BOOL				temp_file;
	FS_BOOL				file_save_dlg;
	FS_BOOL				send_referer;
	FS_CHAR *			post_data;
	FS_SINT4			data_len;
	FS_SINT4			method;
}FS_NetParam;

static FS_WebRequest GFS_WebRequest;
static FS_NetParam GFS_NetParam;
static FS_HttpHandle GFS_hHttp;
static FS_WspHandle GFS_hWsp;

/* last web page url. for backup and restore */
static FS_CHAR *GFS_LastWebPageUrl;
/* current web page url. */
static FS_CHAR * GFS_CurWebPageUrl;
/* current request url. eg. when download image. this url is the image url */
static FS_CHAR * GFS_CurRequestUrl;

static void FS_WebSendGetRequest( FS_Window *win, FS_CHAR *url, FS_BOOL send_ref );
static void FS_WebDisplayState( FS_NetParam *net_data );
void FS_RestoreWebPageUrl( void );

static void FS_DummyDataProvider( void *userData, FS_SaxHandle hsax )
{
	return;
}

static void FS_ResetNetParam( FS_NetParam *net_data )
{
	FS_Window *win = net_data->win;
	
	if( net_data->task )
	{
		FS_FreeWebTask( net_data->task );
		IFS_Free( net_data->task );
		net_data->task = FS_NULL;	
	}
	FS_SAFE_FREE( net_data->url );
	FS_SAFE_FREE( net_data->parent_url );
	FS_SAFE_FREE( net_data->post_data );
	if( net_data->hsax )
	{
		/* user this to end sax handler */
		FS_WEB_TRACE1( "FS_ResetNetParam sax done. hsax = 0x%x", net_data->hsax );
		FS_SaxDataFeed( net_data->hsax, FS_NULL, 0, FS_TRUE );
		net_data->hsax = FS_NULL;
		if( net_data->temp_file )
		{
			FS_FileDelete( FS_DIR_WEB, net_data->file );
			net_data->file[0] = 0;
		}
	}
	IFS_Memset( net_data, 0, sizeof(FS_NetParam) );
	IFS_LeaveBackLight( );
	net_data->win = win;
}

static void FS_ResetWebRequest( void )
{
	if( GFS_WebRequest.timer_id )
	{
		IFS_StopTimer( GFS_WebRequest.timer_id );
		GFS_WebRequest.timer_id = 0;
	}
	IFS_Memset( &GFS_WebRequest, 0, sizeof(FS_WebRequest) );
	GFS_WebRequest.state = FS_WEB_REQ_STS_NONE;
	FS_WebDisplayState( &GFS_NetParam );
}

static void FS_NetRequestCancel_CB( FS_Window *win )
{
	FS_WEB_TRACE1( "FS_NetRequestCancel_CB web state = %d", GFS_WebRequest.state );
	
	FS_RemoveTaskList( win );
	FS_ResetNetParam( &GFS_NetParam );
	FS_ResetWebRequest( );
	FS_LayoutWebWin( win );
	
	if( GFS_hHttp )
	{
		FS_HttpRequestCancel( GFS_hHttp, FS_FALSE );
	}
	if( GFS_hWsp )
	{
		FS_WspAbortReq( GFS_hWsp, FS_FALSE );
	}
}

static void FS_DisplayHttpError( FS_SINT4 error_code )
{
	/* we do not display error messages when we are getting image */
	if( GFS_WebRequest.state == FS_WEB_REQ_STS_GET_IMG ) return;

	if( error_code == FS_HTTP_ERR_NET )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_TRUE );
	}
}

static void FS_DisplayWspError( FS_SINT4 error_code )
{
	FS_CHAR *info = FS_NULL;
	/* we do not display error messages when we are getting image */
	if( GFS_WebRequest.state == FS_WEB_REQ_STS_GET_IMG ) return;

	if( error_code == FS_WSP_ERR_NET )
	{
		info =  FS_Text( FS_T_NET_ERR );
	}
	else if( error_code == FS_WSP_ERR_MEMORY )
	{
		info = FS_Text( FS_T_MEMORY_ERR );
	}
	else if( error_code != FS_WSP_ERR_USER_ABORT )
	{
		info = FS_Text( FS_T_UNKNOW_ERR );
	}

	if( info )
	{
		FS_MessageBox( FS_MS_ALERT, info, FS_NULL, FS_TRUE );
	}
}

static void FS_GetFileNameFromUrl( FS_CHAR *out, FS_CHAR *url )
{
	FS_CHAR *name = FS_NULL, *p = FS_NULL;
	
	if( url )
	{
		p = IFS_Strchr( url, '/' );
		while( p )
		{
			p ++;
			name = p;
			p = IFS_Strchr( p, '/' );
		}
	}
	if( name )
	{
		p = IFS_Strchr( name, '?' );
		if( p )
			IFS_Strncpy( out, name, p - name );
		else
			IFS_Strncpy( out, name, FS_FILE_NAME_LEN - 1 );

		/* not a valid file name */
		if( IFS_Strchr( out, '.' ) == FS_NULL )
			out[0] = 0;
		
	}
}

static FS_SINT4 FS_GenIfModifiedHeader( FS_CHAR *out, FS_DateTime *date )
{
	FS_SINT4 len;
	IFS_Strcpy( out, "If-Modified-Since: " );
	len = IFS_Strlen( out );
	FS_DateTimeFormatText( out + len, date );
	IFS_Strcat( out, "\r\n" );

	return IFS_Strlen( out );
}

static void FS_RefreshWebCounter_CB( void *dummy )
{
	GFS_WebRequest.timer_id = 0;
	if( GFS_WebRequest.state != FS_WEB_REQ_STS_NONE )
	{
		GFS_WebRequest.seconds ++;
		GFS_WebRequest.timer_id = IFS_StartTimer( FS_TIMER_ID_WEB_COUNTER, 1000, FS_RefreshWebCounter_CB, FS_NULL );
	}
	FS_WebDisplayState( &GFS_NetParam );
}

extern void FS_WebMainMenu_UI( FS_Window *win );
extern void FS_HistoryBack_CB( FS_Window *win );

static void FS_WebDisplayState( FS_NetParam *net_data )
{
	FS_CHAR lkey[64],
		*rkey = FS_Text( FS_T_STOP );
	FS_WidgetEventHandler lKeyHandler = FS_NULL, 
		rKeyHandler = FS_NetRequestCancel_CB;
	FS_Window *win = net_data->win;

#ifndef FS_PLT_WIN
	rkey = "";
#endif

	if( net_data->win == FS_NULL ) return;
	
	FS_WEB_TRACE1( "FS_WebDisplayState state = %d", GFS_WebRequest.state );
	switch( GFS_WebRequest.state )
	{
		case FS_WEB_REQ_STS_NONE:
			IFS_Strcpy( lkey, FS_Text(FS_T_MENU) );
			rkey = FS_Text( FS_T_WEB_BACK );
			lKeyHandler = FS_WebMainMenu_UI;
			rKeyHandler = FS_HistoryBack_CB;
			break;
		case FS_WEB_REQ_STS_SEND_REQ:
			IFS_Sprintf( lkey, "%d-%s", GFS_WebRequest.seconds, FS_Text(FS_T_WEB_REQUEST) );
			break;
		case FS_WEB_REQ_STS_RECV_PAGE:
			if( net_data->content_len <= 0 )
			{
				IFS_Sprintf( lkey, "%d-%s[%dK]", GFS_WebRequest.seconds, FS_Text(FS_T_DOWNLOADING), 
					FS_KB(net_data->offset) );
			}
			else
			{
				IFS_Sprintf( lkey, "%d-%s[%d/%dK]", GFS_WebRequest.seconds, FS_Text(FS_T_DOWNLOADING),
					FS_KB(net_data->offset), FS_KB(net_data->content_len) );
			}
			break;
		case FS_WEB_REQ_STS_GET_IMG:
			IFS_Sprintf( lkey, "%d-%s[%d/%d]", GFS_WebRequest.seconds, FS_Text(FS_T_DOWNLOAD_IMG),
				GFS_WebRequest.img_index, GFS_WebRequest.img_total );
			break;
		default:
			/* cannot happend */
			FS_ASSERT( FS_FALSE );
			break;
	}
	
	FS_WindowSetSoftkey( win, 1, lkey, lKeyHandler );
	FS_WindowSetSoftkey( win, 3, rkey, rKeyHandler );
	FS_RedrawSoftkeys( win );
}

static void FS_PrepareUserData( FS_NetParam *net_data, FS_Window *win, FS_CHAR *url )
{
	FS_WebTask *task = net_data->task;
	net_data->task = FS_NULL;
	FS_ResetNetParam( net_data );
	net_data->task = task;
	net_data->url = FS_ComposeAbsUrl( FS_GetCurWebPageUrl(), url );
	net_data->win = win;
	
	FS_SetCurRequestUrl( net_data->url );
	IFS_EnterBackLight( );
	if( GFS_WebRequest.state == FS_WEB_REQ_STS_SEND_REQ )
	{
		if( FS_GetCurWebPageUrl() ) net_data->parent_url = IFS_Strdup( FS_GetCurWebPageUrl( ) );
		FS_SetCurWebPageUrl( net_data->url );
		GFS_WebRequest.timer_id = IFS_StartTimer( FS_TIMER_ID_WEB_COUNTER, 1000, FS_RefreshWebCounter_CB, FS_NULL );
	}
	FS_WebDisplayState( net_data );
}

static void FS_WebContextAddImage( FS_Window *win, FS_CHAR *file, FS_CHAR *dname )
{
	FS_WebImage *img;
	FS_CHAR *ext1, *ext2, *mime;
	
	if( file == FS_NULL || dname == FS_NULL ) return;
	mime = FS_GetMimeFromExt( file );
	if( ! FS_STR_NI_EQUAL(mime, "image/", 6) ) return;
	ext1 = FS_GetFileExt( file );
	ext2 = FS_GetFileExt( dname );
	if( ! FS_STR_I_EQUAL(ext1, ext2) ) return;
	
	img = FS_NEW( FS_WebImage );
	if( img )
	{
		IFS_Memset( img, 0, sizeof(FS_WebImage) );
		IFS_Strcpy( img->file, file );
		IFS_Strcpy( img->dname, dname );

		FS_ListAdd( &win->context.image_list, &img->list );
	}
}

static void FS_WebContextSetBgSound( FS_Window *win, FS_CHAR *file, FS_CHAR *dname )
{
	FS_CHAR *ext1, *ext2, *mime;
	
	if( file == FS_NULL || dname == FS_NULL ) return;
	if( win->context.bgsound.file[0] ) return;
	mime = FS_GetMimeFromExt( file );
	if( ! FS_STR_NI_EQUAL(mime, "audio/", 6) ) return;
	ext1 = FS_GetFileExt( file );
	ext2 = FS_GetFileExt( dname );
	if( ! FS_STR_I_EQUAL(ext1, ext2) ) return;

	IFS_Strncpy( win->context.bgsound.dname, dname, sizeof(win->context.bgsound.dname) - 1 );
	IFS_Strncpy( win->context.bgsound.file, file, sizeof(win->context.bgsound.file) - 1 );
}

static void FS_RunWebTimerTask_CB( void *dummy )
{
	FS_NetParam *net_data = &GFS_NetParam;
	FS_WebTask *task;
	FS_Window *win;
	
	GFS_WebRequest.timer_id = 0;
	if( net_data && net_data->task )
	{			
		task = net_data->task;
		win = net_data->win;
		net_data->task = FS_NULL;
		FS_ResetNetParam( net_data );

		if( task->url )
			FS_WebGoToUrl( win, task->url, FS_FALSE );
		else if( task->form )
			FS_SubmitForm( win, task->form );
		
		FS_FreeWebTask( task );
		IFS_Free( task );
	}
}

static void FS_StartWebTimerTask( FS_NetParam *net_data )
{
	FS_List *node;
	FS_WebTask *task;
	FS_SINT4 maxtimer;

	maxtimer = FS_WebConfigGetMaxNetworkTimer( );
	node = net_data->win->context.task_list.next;
	if( node != &net_data->win->context.task_list )
	{
		task = FS_ListEntry( node, FS_WebTask, list );
		FS_ListDel( &task->list );
		if( task->type == FS_WTSK_TIMER && task->delay == 0 )
		{
			task->delay = 1;
		}

		if(task->type == FS_WTSK_TIMER && task->delay > maxtimer )
		{
			task->delay = maxtimer;
		}
		
		if( task->type == FS_WTSK_TIMER && net_data->task == FS_NULL )
		{
			net_data->task = task;
		}
		else
		{
			FS_FreeWebTask( task );
			IFS_Free( task );
		}
	}

	if( net_data->task )
	{
		GFS_WebRequest.timer_id = IFS_StartTimer( FS_TIMER_ID_WEB_COUNTER, 1000 * net_data->task->delay, FS_RunWebTimerTask_CB, FS_NULL );
	}
}

static FS_BOOL FS_IsWebTaskFinish( FS_NetParam *net_data )
{
	FS_List *node;
	FS_WebTask *task;

	node = net_data->win->context.task_list.next;
	while( node != &net_data->win->context.task_list )
	{
		task = FS_ListEntry( node, FS_WebTask, list );
		node = node->next;

		if( task->type != FS_WTSK_TIMER )
			return FS_FALSE;
	}
	return FS_TRUE;
}

static FS_SINT2 FS_WebGetImageTaskNum( FS_NetParam *net_data )
{
	FS_List *node;
	FS_WebTask *task;
	FS_SINT2 num = 0;
	
	node = net_data->win->context.task_list.next;
	while( node != &net_data->win->context.task_list )
	{
		task = FS_ListEntry( node, FS_WebTask, list );
		node = node->next;

		if( task->type == FS_WTSK_IMAGE || task->type == FS_WTSK_SOUND )
			num ++;
	}
	return num;
}

static void FS_WebExecuteTasks( FS_NetParam *net_data )
{
	FS_List *node;
	FS_WebTask *task;
	FS_Window *win;
	FS_CHAR *url, *file, filename[FS_MAX_PATH_LEN];

RUN_FIRST_TASK:	
	win = net_data->win;
	node = win->context.task_list.next;

	/* start new task if any */
	if( ! FS_IsWebTaskFinish(net_data) )
	{
		task = FS_ListEntry( node, FS_WebTask, list );

		while( task->type == FS_WTSK_TIMER )
		{
			node = node->next;
			task = FS_ListEntry( node, FS_WebTask, list );
		}
		FS_ListDel( &task->list );

		/* not a valid task */
		if( task->url == FS_NULL && task->form == FS_NULL )
		{
			net_data->task = FS_NULL;
			FS_FreeWebTask( task );
			IFS_Free( task );
			goto RUN_FIRST_TASK;
		}
		
		net_data->task = task;
		if( task->type == FS_WTSK_IMAGE || task->type == FS_WTSK_SOUND )
		{			
			GFS_WebRequest.img_index ++;
			FS_WebDisplayState( net_data );
			
			url = FS_ComposeAbsUrl( FS_GetCurWebPageUrl(), task->url );
			file = FS_CacheFindFile( url );
			FS_WEB_TRACE2( "FS_WebExecuteTasks url = %s, file = %s", url, file );
			FS_SAFE_FREE( url );
			if( file )
			{
				if( task->type == FS_WTSK_IMAGE && task->wwgt )
				{
					FS_WebContextAddImage( net_data->win, file, file );
					FS_GetAbsFileName( FS_DIR_WEB, file, filename );
					FS_COPY_TEXT( task->wwgt->file, filename );
				}
				else if( task->type == FS_WTSK_SOUND )
				{
					FS_WebContextSetBgSound( net_data->win, file, file );
				}
				net_data->task = FS_NULL;
				FS_FreeWebTask( task );
				IFS_Free( task );
				
				goto RUN_FIRST_TASK;
			}

			FS_WebSendGetRequest( win, task->url, FS_FALSE );
		}
		else if( task->type == FS_WTSK_GOTO )
		{
			/* we treat timer task as normal redirect task here */
			FS_ClearWebWinContext( win );
			net_data->task = FS_NULL;

			if( task->url )
			{
				FS_WebGoToUrl( win, task->url, FS_FALSE );
			}
			else if( task->form )
			{
				FS_SubmitForm( win, task->form );
			}

			FS_FreeWebTask( task );
			IFS_Free( task );
		}
	}
	else
	{
		/* here, we the web request is completed */
		FS_ResetNetParam( net_data );
		FS_ResetWebRequest( );
		
		FS_LayoutWebWin( win );
		FS_StartWebTimerTask( net_data );
	}
}

static void FS_LoadWebDocImages( FS_Window *win )
{
	FS_List *node;
	FS_WebTask *task;
	FS_CHAR *url, *file, filename[FS_MAX_PATH_LEN];
	FS_BOOL bLayout = FS_FALSE;
	
	node = win->context.task_list.next;
	while( node != &win->context.task_list )
	{
		task = FS_ListEntry( node, FS_WebTask, list );
		node = node->next;
		if( task->type == FS_WTSK_IMAGE || task->type == FS_WTSK_SOUND )
		{
			url = FS_ComposeAbsUrl( FS_GetCurWebPageUrl(), task->url );
			file = FS_CacheFindFile( url );
			if( task->type == FS_WTSK_IMAGE && file && task->wwgt && FS_WebConfigGetShowImageFlag() )
			{
				FS_WebContextAddImage( win, file, file );
				FS_GetAbsFileName( FS_DIR_WEB, file, filename );
				FS_COPY_TEXT( task->wwgt->file, filename );
				bLayout = FS_TRUE;
			}
			else if( task->type == FS_WTSK_SOUND && file && FS_WebConfigGetPlayAudioFlag() )
			{
				FS_WebContextSetBgSound( win, file, file );
				bLayout = FS_TRUE;
			}
			FS_SAFE_FREE( url );
		}
	}

	if( bLayout ) FS_LayoutWebWin( win );
}

static void FS_WebSendGetRequest( FS_Window *win, FS_CHAR *url, FS_BOOL send_ref )
{
	FS_WebEventParam *ev;

	ev = FS_NEW( FS_WebEventParam );
	if( ev )
	{
		IFS_Memset( ev, 0, sizeof(FS_WebEventParam) );
		ev->win = win;
		ev->url = IFS_Strdup( url );
		ev->send_referer = send_ref;
		FS_WEB_POST_EVENT( FS_WEB_EV_GET, ev );
	}
}

static FS_CHAR *FS_WtaiGetParam( FS_CHAR *str, FS_SINT4 idx )
{
	FS_CHAR *ret, *p, *p2;

	p = str;
	while( p && idx > 0 )
	{
		p = IFS_Strchr( p, ';' );
		idx --;
		if( p ) p ++;
	}

	if( p == FS_NULL ) return FS_NULL;
	while( *p && *(p + 1) == 0x20 ) p ++;
	p2 = IFS_Strchr( p, ';' );
	if( p2 ){
		ret = FS_Strndup( p, p2 - p );
	}else{
		ret = IFS_Strdup( p );
	}
	return ret;
}

static void FS_WtaiScript( FS_CHAR *wtai )
{
	FS_CHAR *script;
	FS_CHAR *p1, *p2;
	
	if( IFS_Strnicmp(wtai, "wtai://", 7 ) != 0 ) return;
	script = wtai + 7;
	if( FS_STR_NI_EQUAL(script, "wp/mc", 5) )
	{
		p1 = FS_WtaiGetParam( script + 5, 1 );
		if( p1 )
		{
			IFS_WtaiMakeCall( p1 );
			IFS_Free( p1 );
		}
	}
	else if( FS_STR_NI_EQUAL(script, "wp/ap", 5) )
	{
		p1 = FS_WtaiGetParam( script + 5, 1 );
		p2 = FS_WtaiGetParam( script + 5, 2 );
		if( p1 )
		{
			IFS_WtaiAddPBEntry( p1, p2 );
			IFS_Free( p1 );
			FS_SAFE_FREE( p2 );
		}
	}
	else
	{
		script = FS_StrConCat( FS_Text(FS_T_NOT_SUPPORT), wtai, FS_NULL, FS_NULL );		
		FS_MessageBox( FS_MS_ALERT, script, FS_NULL, FS_FALSE );
		IFS_Free( script );
	}
}

void FS_WebGoToUrl( FS_Window *win, FS_CHAR *url, FS_BOOL send_ref )
{	
	if( FS_STR_NI_EQUAL(url, "sfox::", 6) )
	{
		/* this is our script */
		FS_SFoxWebScript( url );
		return;
	}
	else if( FS_STR_NI_EQUAL(url, "wtai:", 5) )
	{
		/* this is wtai script */
		FS_WtaiScript( url );
		return;
	}
	else if( FS_STR_NI_EQUAL(url, "tel:", 4) )
	{
		IFS_WtaiMakeCall( url + 4 );
		return;
	}
	
	if( win->id != FS_W_WebMainFrm )
	{
		FS_WebOpenUrl( url );
		return;
	}
	
	FS_WebSendGetRequest( win, url, send_ref );
	if( GFS_WebRequest.state != FS_WEB_REQ_STS_NONE )
	{
		/* !!! cannot cancel first. because after cancel, the param 'url' may become invalidate */
		FS_NetRequestCancel_CB( win );
	}
	GFS_WebRequest.state = FS_WEB_REQ_STS_SEND_REQ;
}

static void FS_WebSendPostRequest( FS_Window *win, FS_CHAR *url, FS_CHAR *data, FS_SINT4 dlen, FS_BOOL send_ref )
{
	FS_WebEventParam *ev;

	ev = FS_NEW( FS_WebEventParam );
	if( ev )
	{
		IFS_Memset( ev, 0, sizeof(FS_WebEventParam) );
		ev->win = win;
		ev->url = IFS_Strdup( url );
		ev->send_referer = send_ref;
		ev->data = IFS_Malloc( dlen + 1 );
		FS_ASSERT( ev->data != FS_NULL );
		IFS_Memcpy( ev->data, data, dlen );
		ev->data[dlen] = 0;
		ev->dlen = dlen;
		FS_WEB_POST_EVENT( FS_WEB_EV_POST, ev );
	}
}

void FS_WebPostData( FS_Window *win, FS_CHAR *url, FS_CHAR *data, FS_SINT4 dlen, FS_BOOL send_ref )
{
	FS_WebSendPostRequest( win, url, data, dlen, send_ref );
	if( GFS_WebRequest.state != FS_WEB_REQ_STS_NONE )
	{
		/* !!! cannot cancel first. because after cancel, the param 'url' may become invalidate */
		FS_NetRequestCancel_CB( win );
	}
	GFS_WebRequest.state = FS_WEB_REQ_STS_SEND_REQ;
}

static void FS_WebHandleJadContent( FS_CHAR *jadFile )
{
	FS_CHAR *str, *jarUrl, *jadUrl;
	
	str = IFS_JvmCheckJad( jadFile );
	if( str != FS_NULL ){
		if( FS_STR_NI_EQUAL(str, "http://", 7) ){
			jarUrl = IFS_Strdup( str );
		}else{
			jadUrl = FS_GetCurRequestUrl( );
			jarUrl = FS_ComposeAbsUrl( jadUrl, str );
		}
		FS_WebGoToUrl( GFS_NetParam.win, jarUrl, FS_FALSE );
		IFS_Free( jarUrl );
	}
}

static void FS_FileDownloadSave_CB( FS_CHAR *path, FS_CHAR *srcFile )
{
	FS_SINT4 len;
	FS_BOOL ret;
	FS_CHAR *str;
	
	if( path && path[0] )
	{
		len = IFS_Strlen( path );
		ret = FS_FileMove( FS_DIR_WEB, srcFile, -1, path );
		if( ret )
		{
			/* handle java download */
			str = FS_GetMimeFromExt( srcFile );
			if( FS_STR_I_EQUAL(str, "text/vnd.sun.j2me.app-descriptor" ) ){
				FS_WebHandleJadContent( path );
			}else if( FS_STR_I_EQUAL(str, "application/java-archive" ) ){
				IFS_JvmInstallJar( path );
			}else{
				str = IFS_Malloc( len + 32 );
				IFS_Sprintf( str, "%s %s", FS_Text(FS_T_SAVED_TO), path );
				FS_MessageBox( FS_MS_OK, str, FS_NULL, FS_FALSE );
				IFS_Free( str );
			}
		}
		else
		{
			FS_FileDelete( FS_DIR_WEB, srcFile );
			str = FS_Text( FS_T_SAVE_FILE_FAILED );
			FS_MessageBox( FS_MS_ALERT, str, FS_NULL, FS_FALSE );
		}
	}
	
	IFS_Free( srcFile );
}

static void FS_ServerResponseEnd( FS_NetParam *net_data )
{
	FS_WebTask *task;
	FS_Window *win = net_data->win;
	FS_CHAR filename[FS_MAX_PATH_LEN];
	FS_CHAR *file;
	
	FS_WEB_TRACE2( "FS_ServerResponseEnd web state = %d, task = 0x%x", GFS_WebRequest.state, net_data->task );

	if( net_data->file_save_dlg )
	{
		FS_ResetWebRequest( );
		if( net_data->offset > 0 )
		{
			file = IFS_Strdup( net_data->file );
			/* we need a file save dialog to save downloaded file */
			IFS_FileDialogSave( net_data->filename, FS_FileDownloadSave_CB, file );
		}
		FS_ResetNetParam( net_data );
	}
	else
	{
		if( GFS_WebRequest.state == FS_WEB_REQ_STS_RECV_PAGE )
		{
			GFS_WebRequest.state = FS_WEB_REQ_STS_GET_IMG;
			GFS_WebRequest.img_total = FS_WebGetImageTaskNum( net_data );
			FS_CacheEntrySetTitle( net_data->url, net_data->win->context.title );
		}
		
		if( net_data->task )
		{
			/* handle complete excute task if any */
			task = net_data->task;
			net_data->task = FS_NULL;
			if( task->type == FS_WTSK_IMAGE && task->wwgt && net_data->file[0] )
			{
				FS_WebContextAddImage( net_data->win, net_data->file, net_data->filename );
				FS_GetAbsFileName( FS_DIR_WEB, net_data->file, filename );
				FS_COPY_TEXT( task->wwgt->file, filename );
			}
			else if( task->type == FS_WTSK_SOUND && net_data->file[0] )
			{
				FS_WebContextSetBgSound( net_data->win, net_data->file, net_data->filename );
			}
			FS_FreeWebTask( task );
			IFS_Free( task );
		}
		
		FS_ResetNetParam( net_data );
		/* down load image etc. we must run tasks because there may be a complete excute task */
		FS_WebExecuteTasks( net_data );
	}
}

static void FS_HttpResponseData( FS_NetParam *net_data, FS_HttpHandle hHttp, FS_BYTE *data, FS_SINT4 data_len )
{
	FS_WEB_TRACE2( "FS_HttpResponseData web state = %d, data len = %d", GFS_WebRequest.state, data_len );
	
	if( GFS_WebRequest.state == FS_WEB_REQ_STS_NONE ) return;
		
	if(data_len == FS_FileWrite( FS_DIR_WEB, net_data->file, net_data->offset, data, data_len ) )
	{
		net_data->offset += data_len;	
		if( net_data->hsax )
		{
			FS_SaxDataFeed( net_data->hsax, data, data_len, FS_FALSE );
			FS_InvalidateRect( net_data->win, FS_NULL );
		}
		if( GFS_WebRequest.state == FS_WEB_REQ_STS_RECV_PAGE ) FS_WebDisplayState( net_data );
	}
	else
	{
		FS_WEB_TRACE1( "[ERROR] FS_HttpResponseData FileWrite errors. data len = %d", data_len );
		FS_NetRequestCancel_CB( net_data->win );
	}
}

static void FS_HttpResponseEnd( FS_NetParam *net_data, FS_HttpHandle hHttp, FS_SINT4 error_code )
{
	FS_Window *win = net_data->win;

	FS_WEB_TRACE2( "FS_HttpResponseEnd web state = %d, error code = %d", GFS_WebRequest.state, error_code );
	if( GFS_WebRequest.state == FS_WEB_REQ_STS_NONE ) return;
	if( net_data->url == FS_NULL ) return;
	
	if( net_data->hsax )
	{
		/* user this to end sax handler */
		FS_WEB_TRACE1( "FS_HttpResponseEnd sax done. sax = 0x%x", net_data->hsax );
		FS_SaxDataFeed( net_data->hsax, FS_NULL, 0, FS_TRUE );
		net_data->hsax = FS_NULL;
		if( net_data->temp_file )
		{
			FS_FileDelete( FS_DIR_WEB, net_data->file );
			net_data->file[0] = 0;
		}
	}
	if( IFS_Strstr(net_data->file, ".bwml") != FS_NULL )
	{
		FS_BinWmlProcessFile( net_data->win, net_data->file, FS_NULL );
	}
	FS_ServerResponseEnd( net_data );
	FS_DisplayHttpError( error_code );
}

static void FS_HttpResponseStart( FS_NetParam *net_data, FS_HttpHandle hHttp, FS_SINT4 status, FS_HttpHeader *headers )
{
	FS_CHAR *ext, *charset;
	FS_BOOL is_embed = FS_FALSE;
	FS_HistoryEntry *he;
	FS_SINT4 tlen;

	FS_WEB_TRACE3( "FS_HttpResponseStart web state = %d, status code = %d, request url = %s", GFS_WebRequest.state, status, net_data->url );
	FS_WEB_TRACE2( "FS_HttpResponseStart content len = %d, content type = %s", headers->content_length, headers->content_type );
	if( GFS_WebRequest.state == FS_WEB_REQ_STS_NONE ) return;
	if( net_data->url == FS_NULL ) return;	/* user may cancel the request */

	if( GFS_WebRequest.state == FS_WEB_REQ_STS_SEND_REQ )
	{
		GFS_WebRequest.state = FS_WEB_REQ_STS_RECV_PAGE;
		FS_WebDisplayState( net_data );
	}
	
	if( headers && headers->cookies )
		FS_CookieSave( net_data->url, headers->cookies );

	if( net_data->task && net_data->task->is_embed )
		is_embed = FS_TRUE;
	
	if( status == FS_HTTP_MOVED || status == FS_HTTP_FOUND ||  status == FS_HTTP_SEE_OTHER )
	{
		if( headers->location )
		{
			if( GFS_WebRequest.state == FS_WEB_REQ_STS_GET_IMG )
			{
				FS_WebSendGetRequest( net_data->win, headers->location, FS_FALSE );
				FS_HttpRequestCancel( hHttp, FS_FALSE );
			}
			else
			{
				FS_WebGoToUrl( net_data->win, headers->location, FS_FALSE );
			}
		}
	}
	else if( status == FS_HTTP_NOT_MODIFIED )
	{
		he = FS_CacheFindEntry( net_data->url );
		if( he )
			ext = FS_GetFileExt( he->file );
		else
			ext = FS_NULL;

		if( ext )
		{		
			if( IFS_Stricmp(ext, "htm") == 0 || IFS_Stricmp(ext, "html") == 0 )
			{
				if( ! is_embed )
				{
					FS_HistorySetViewport( net_data->parent_url, FS_WEB_WIN_CUR_VIEWPORT( net_data ) );
					FS_ClearWebWinContext( net_data->win );
					FS_HistorySetCurrent( net_data->url );
				}
				FS_HtmlProcessFile( net_data->win, he->file, he->charset, FS_NULL );
			}
			else if( IFS_Stricmp(ext, "wml") == 0 )
			{
				if( ! is_embed )
				{
					FS_HistorySetViewport( net_data->parent_url, FS_WEB_WIN_CUR_VIEWPORT( net_data ) );
					FS_ClearWebWinContext( net_data->win );
					FS_HistorySetCurrent( net_data->url );
				}
				FS_WmlProcessFile( net_data->win, he->file, FS_NULL, FS_NULL );
			}
			else if( IFS_Stricmp(ext, "bwml") == 0 )
			{
				if( ! is_embed )
				{
					FS_HistorySetViewport( net_data->parent_url, FS_WEB_WIN_CUR_VIEWPORT( net_data ) );
					FS_ClearWebWinContext( net_data->win );
					FS_HistorySetCurrent( net_data->url );
				}
				FS_BinWmlProcessFile( net_data->win, he->file, FS_NULL );
			}
			else
			{
				/* for image. it may use this to process image file */
				IFS_Strcpy( net_data->file, he->file );
				if( ! is_embed )
				{
					net_data->file_save_dlg = FS_TRUE;
				}				
			}
		}

		FS_NetRequestCancel_CB( net_data->win );
	}
	else if( status >= FS_HTTP_OK && headers->content_type )
	{
		charset = FS_GetParam( &headers->params, "charset" );
		net_data->content_len = headers->content_length;
		
		if( ! is_embed && ( IFS_Strnicmp( headers->content_type, "text/html", 9 ) == 0
			|| IFS_Strnicmp( headers->content_type, "application/xhtml", 17 ) == 0 
			|| IFS_Strnicmp( headers->content_type, "application/vnd.wap.xhtml", 25 ) == 0 ) )
		{
			if( ! is_embed )
			{
				FS_HistorySetViewport( net_data->parent_url, FS_WEB_WIN_CUR_VIEWPORT( net_data ) );
				FS_ClearWebWinContext( net_data->win );
			}
			net_data->hsax = FS_HtmlProcessFile( net_data->win, FS_NULL, charset, FS_DummyDataProvider );
			FS_GetGuid( net_data->file );
			IFS_Strcat( net_data->file, ".html" );

			if( ! is_embed )
				FS_HistoryPushEntry( net_data->url, net_data->file );

			if( status >= FS_HTTP_OK && status < 300 )
				FS_CacheAddEntry( net_data->url, net_data->file, charset );
			else
				net_data->temp_file = FS_TRUE;		/* just temp file. we will delete it when response complete */
		}
		else if( ! is_embed && ( IFS_Strnicmp( headers->content_type, "text/vnd.wap.wml", 16 ) ) == 0 )
		{
			if( ! is_embed )
			{
				FS_HistorySetViewport( net_data->parent_url, FS_WEB_WIN_CUR_VIEWPORT( net_data ) );
				FS_ClearWebWinContext( net_data->win );
			}
			net_data->hsax = FS_WmlProcessFile( net_data->win, FS_NULL, FS_NULL, FS_DummyDataProvider );
			FS_GetGuid( net_data->file );
			IFS_Strcat( net_data->file, ".wml" );
	
			if( ! is_embed )
				FS_HistoryPushEntry( net_data->url, net_data->file );

			if( status == FS_HTTP_OK )
				FS_CacheAddEntry( net_data->url, net_data->file, FS_GetParam(&headers->params, "charset") );
			else
				net_data->temp_file = FS_TRUE; 	/* just temp file. we will delete it when response complete */
		}
		else if( ! is_embed &&( IFS_Strnicmp( headers->content_type, "application/vnd.wap.wmlc", 24 ) ) == 0 )
		{
			if( ! is_embed )
			{
				FS_HistorySetViewport( net_data->parent_url, FS_WEB_WIN_CUR_VIEWPORT( net_data ) );
				FS_ClearWebWinContext( net_data->win );
			}
			FS_GetGuid( net_data->file );
			IFS_Strcat( net_data->file, ".bwml" );
		
			if( ! is_embed )
				FS_HistoryPushEntry( net_data->url, net_data->file );
		
			if( status == FS_HTTP_OK )
				FS_CacheAddEntry( net_data->url, net_data->file, FS_GetParam(&headers->params, "charset") );
			else
				net_data->temp_file = FS_TRUE; 	/* just temp file. we will delete it when response complete */
		}
		else
		{
			FS_GetFileNameFromUrl( net_data->filename, net_data->url );
			FS_GetGuid( net_data->file );
			ext = FS_GetExtFromMime( headers->content_type );
			if( ext == FS_NULL )
				ext = FS_GetFileExt( net_data->filename );
			if( ext )
			{
				IFS_Strcat( net_data->file, "." );
				tlen = IFS_Strlen( net_data->file );
				IFS_Strncpy( net_data->file + tlen, ext, sizeof(net_data->file) - tlen - 1 );
			}
			/* check if the filename get from url is correct or not */
			ext = FS_GetMimeFromExt( net_data->filename );
			if( ! FS_STR_I_EQUAL(ext, headers->content_type) )
			{
				IFS_Strcpy( net_data->filename, net_data->file );
			}
			FS_WEB_TRACE1( "FS_HttpResponseStart download file %s", net_data->file );
			FS_CacheAddEntry( net_data->url, net_data->file, FS_NULL );

			if( ! is_embed )
			{
				/* when we are download image/audio. we must restore the current web page url. */
				FS_RestoreWebPageUrl();
				net_data->file_save_dlg = FS_TRUE;
			}
		}
	}
}

static void FS_WspEventHandler( FS_NetParam *net_data, FS_WspHandle hWsp, FS_SINT4 event, FS_UINT4 param )
{
	FS_CHAR *ext;
	FS_WspResultData *rData;
	FS_Window *win = net_data->win;
	FS_BOOL is_embed = FS_FALSE;
	FS_SINT4 tlen;

	FS_WEB_TRACE2( "FS_WspEventHandler web state = %d, event = %d", GFS_WebRequest.state, event );
	if( GFS_WebRequest.state == FS_WEB_REQ_STS_NONE ) return;
	
	if( event == FS_WSP_EV_CONNECT_CNF )
	{
		/* 
			connected to server. can print some state message here. 
			we just ignore this indication
		*/
	}
	else if( event == FS_WSP_EV_RESULT_IND )
	{
		if( GFS_WebRequest.state == FS_WEB_REQ_STS_SEND_REQ )
		{
			GFS_WebRequest.state = FS_WEB_REQ_STS_RECV_PAGE;
			FS_WebDisplayState( net_data );
		}
		
		if( net_data->task && net_data->task->is_embed )
			is_embed = FS_TRUE;
		
		rData = (FS_WspResultData *)param;
		
		FS_WEB_TRACE3( "FS_WspEventHandler status code = %d, offset = %d, data len = %d", rData->status, net_data->offset, rData->len );

		if( rData->status == FS_WSP_STATUS_MOVED_PERM 
			|| rData->status == FS_WSP_STATUS_MOVED_TEMP
			||  rData->status == FS_WSP_STATUS_SEE_OTHER )
		{
			if( GFS_WebRequest.state == FS_WEB_REQ_STS_GET_IMG )
			{
				FS_WebSendGetRequest( net_data->win, rData->location, FS_FALSE );
				FS_WspAbortReq( hWsp, FS_FALSE );
			}
			else
			{
				FS_WebGoToUrl( net_data->win, rData->location, FS_FALSE );
			}
		}
		else
		{
			/* show the wap page */
			if( net_data->file[0] == 0 )
			{
				if( ! is_embed && rData->content_type == FS_WCT_TEXT_WML )
				{
					FS_GetGuid( net_data->file );
					IFS_Strcat( net_data->file, ".wml" );
					
					if( ! is_embed )
					{
						FS_HistorySetViewport( net_data->parent_url, FS_WEB_WIN_CUR_VIEWPORT( net_data ) );
						FS_ClearWebWinContext( net_data->win );
						FS_HistoryPushEntry( net_data->url, net_data->file );
					}
					
					if( rData->status == FS_WSP_STATUS_OK )
						FS_CacheAddEntry( net_data->url, net_data->file, FS_NULL );
					else
						net_data->temp_file = FS_TRUE;		/* just temp file. we will delete it when response complete */
					
					net_data->hsax = FS_WmlProcessFile( net_data->win, FS_NULL, FS_NULL, FS_DummyDataProvider );
				}
				else if( ! is_embed && ( rData->content_type == FS_WCT_TEXT_HTML 
					|| (rData->mime_content && IFS_Strnicmp( rData->mime_content, "application/xhtml", 17 ) == 0 )
					|| (rData->mime_content && IFS_Strnicmp( rData->mime_content, "application/vnd.wap.xhtml", 25 ) ) == 0) )
				{
					FS_GetGuid( net_data->file );
					IFS_Strcat( net_data->file, ".htm" );
					
					if( ! is_embed )
					{
						FS_HistorySetViewport( net_data->parent_url, FS_WEB_WIN_CUR_VIEWPORT( net_data ) );
						FS_ClearWebWinContext( net_data->win );
						FS_HistoryPushEntry( net_data->url, net_data->file );
					}
					
					if( rData->status == FS_WSP_STATUS_OK )
						FS_CacheAddEntry( net_data->url, net_data->file, FS_NULL );
					else
						net_data->temp_file = FS_TRUE;		/* just temp file. we will delete it when response complete */
					
					net_data->hsax = FS_HtmlProcessFile( net_data->win, FS_NULL, FS_NULL, FS_DummyDataProvider );
				}
				else if( ! is_embed && rData->content_type == FS_WCT_APP_WMLC )
				{
					FS_GetGuid( net_data->file );
					IFS_Strcat( net_data->file, ".bwml" );
					
					if( ! is_embed )
					{
						FS_HistorySetViewport( net_data->parent_url, FS_WEB_WIN_CUR_VIEWPORT( net_data ) );
						FS_ClearWebWinContext( net_data->win );
						FS_HistoryPushEntry( net_data->url, net_data->file );
					}
					
					if( rData->status == FS_WSP_STATUS_OK )
						FS_CacheAddEntry( net_data->url, net_data->file, FS_NULL );
					else
						net_data->temp_file = FS_TRUE;		/* just temp file. we will delete it when response complete */
				}
				else
				{
					FS_GetFileNameFromUrl( net_data->filename, net_data->url );
					FS_GetGuid( net_data->file );
					ext = FS_GetExtFromMimeCode( rData->content_type );
					if( ext == FS_NULL )
						ext = FS_GetFileExt( net_data->filename );
					if( ext )
					{
						IFS_Strcat( net_data->file, "." );
						tlen = IFS_Strlen( net_data->file );
						IFS_Strncpy( net_data->file + tlen, ext, sizeof(net_data->file) - tlen - 1 );
					}
					/* check if the filename get from url is correct or not */
					if( FS_GetMimeCodeFromExt( net_data->filename ) != rData->content_type )
					{
						IFS_Strcpy( net_data->filename, net_data->file );
					}
					
					FS_CacheAddEntry( net_data->url, net_data->file, FS_NULL );
					
					/* not a embed task. we must popup a filedialog to save download file */
					if( ! is_embed )
					{
						/* when we are download image/audio. we must restore the current web page url. */
						FS_RestoreWebPageUrl();
						net_data->file_save_dlg = FS_TRUE;
					}
				}
			}

			FS_FileWrite( FS_DIR_WEB, net_data->file, net_data->offset, rData->data, rData->len );
			net_data->offset += rData->len;
			if( GFS_WebRequest.state == FS_WEB_REQ_STS_RECV_PAGE ) FS_WebDisplayState( net_data );

			if( net_data->hsax )
			{
				FS_WEB_TRACE3("FS_WspEventHandler data = 0x%x, sax = 0x%x, done = %d", rData->data, net_data->hsax, rData->done );
				FS_SaxDataFeed( net_data->hsax, rData->data, rData->len, rData->done );
				if( rData->done ) net_data->hsax = FS_NULL;
				FS_InvalidateRect( net_data->win, &net_data->win->client_rect );
			}

			if( rData->done )
			{
				if( IFS_Strstr( net_data->file, ".bwml" ) != FS_NULL )
				{
					FS_BinWmlProcessFile( net_data->win, net_data->file, FS_NULL );
				}
				FS_ServerResponseEnd( net_data );
			}
		}
	}
	else if( event == FS_WSP_EV_DISCONNECT_IND || event < 0 )
	{
		FS_ServerResponseEnd( net_data );
		FS_DisplayWspError( event );
	}
}

static void FS_WebNetConnCallback( FS_NetParam *user_data, FS_BOOL ok )
{
	FS_SockAddr addr = { FS_NULL, 0 };
	FS_HistoryEntry *he;
	FS_CHAR *headers = FS_NULL, *curUrl;
	FS_SINT4 offset = 0;
	FS_WspPostDataStruct wPData;
	FS_HttpPostDataStruct hPData;

	FS_WEB_TRACE1( "FS_WebNetConnCallback result = %d", ok );
	if( ! ok )
	{
		FS_ServerResponseEnd( user_data );
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_DIAL_FAILED), FS_NULL, FS_TRUE );
		return;
	}
	
	if( FS_WebConfigGetProtocol( ) == FS_WEB_WSP )
	{
		/* we go wsp here */
		if( GFS_hWsp == FS_NULL )
		{
			addr.host = FS_WebConfigGetProxyAddr( );
			addr.port = FS_WebConfigGetProxyPort( );
			GFS_hWsp = FS_WspCreateHandle( &addr, user_data, FS_WspEventHandler );
		}
		
		if( GFS_hWsp == FS_NULL )
		{
			FS_ServerResponseEnd( user_data );
			FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_BUSY), FS_NULL, FS_TRUE );
			return;
		}
		
		if( user_data->method == FS_METHOD_GET )
		{
			FS_WspGetReq( GFS_hWsp, user_data->url );
		}
		else
		{
			IFS_Memset( &wPData, 0, sizeof(FS_WspPostDataStruct) );
			wPData.data = user_data->post_data;
			wPData.data_len = user_data->data_len;
			wPData.int_enc_content = FS_WCT_APP_FORM_URLENCODE;
			if( user_data->send_referer )
			{
				wPData.referer = FS_GetCurWebPageUrl( );
			}
			
			FS_WspPostReq( GFS_hWsp, user_data->url, &wPData );
			
			IFS_Free( user_data->post_data );
			user_data->post_data = FS_NULL;
			user_data->data_len = 0;
		}		
	}
	else
	{
		/* we go http here */
		if( GFS_hHttp == FS_NULL )
		{
			GFS_hHttp = FS_HttpCreateHandle( user_data, FS_HttpResponseStart, FS_HttpResponseData, FS_HttpResponseEnd );
		}
		
		if( GFS_hHttp == FS_NULL )
		{
			FS_ServerResponseEnd( user_data );
			FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_BUSY), FS_NULL, FS_TRUE );
			return;
		}
		
		IFS_Memset( &hPData, 0, sizeof(FS_HttpPostDataStruct) );
		hPData.content_type = "application/x-www-form-urlencoded";
		hPData.data = user_data->post_data;
		hPData.data_len = user_data->data_len;

		headers = IFS_Malloc( 2048 );
		if( headers )
		{
			IFS_Memset( headers, 0, 2048 );

			if( user_data->method == FS_METHOD_GET )
			{
				he = FS_CacheFindEntry( user_data->url );
				if( he )
				{
					offset += FS_GenIfModifiedHeader( headers, &he->date );
				}
			}
			
			if( user_data->send_referer )
			{
				curUrl = FS_GetCurWebPageUrl( );
				IFS_Sprintf( headers + offset, "Referer: %s\r\n", curUrl );
				offset += IFS_Strlen( headers + offset );
			}

			offset += FS_CookieGet( headers + offset, 2048 - offset, user_data->url );
		}
		
		if( FS_WebConfigGetUseProxy( ) )
		{
			addr.host = FS_WebConfigGetProxyAddr( );
			addr.port = FS_WebConfigGetProxyPort( );
		}
		if( user_data->method == FS_METHOD_GET )
			FS_HttpRequest( GFS_hHttp, &addr, "GET", user_data->url, FS_NULL, headers );
		else
			FS_HttpRequest( GFS_hHttp, &addr, "POST", user_data->url, &hPData, headers );

		FS_SAFE_FREE( user_data->post_data );
		user_data->post_data = FS_NULL;
		user_data->data_len = 0;
		FS_SAFE_FREE( headers );
	}
}

static void FS_GotoUrl( FS_Window *win, FS_CHAR *url, FS_BOOL send_referer )
{
	FS_NetParam *user_data;
	FS_HistoryEntry *he;
	FS_CHAR *curUrl;
	
	FS_WEB_TRACE1( "FS_GotoUrl url = %s", url );
	if( url[0] == '#' )
	{
		curUrl = FS_GetCurWebPageUrl( );
		he = FS_CacheFindEntry( curUrl );
		if( he )
		{
			FS_LoadWebDoc( win, he->file, (url + 1), FS_FALSE, FS_TRUE );
			return;
		}
	}
	user_data = &GFS_NetParam;
	FS_PrepareUserData( user_data, win, url );
	user_data->send_referer = send_referer;
	user_data->method = FS_METHOD_GET;
	FS_NetConnect( FS_WebConfigGetApn(), FS_WebConfigGetUserName(), FS_WebConfigGetPassword(),
		FS_WebNetConnCallback, FS_APP_WEB, FS_TRUE, user_data);
}

static void FS_PostData( FS_Window *win, FS_CHAR *url, FS_CHAR *data, FS_SINT4 dlen, FS_BOOL send_referer )
{
	FS_NetParam *net_data = &GFS_NetParam;
		
	FS_WEB_TRACE2( "FS_PostData url = %s, post data len = %d", url, dlen );
	FS_PrepareUserData( net_data, win, url );
	net_data->send_referer = send_referer;
	net_data->post_data = IFS_Malloc( dlen + 1 );
	net_data->data_len = dlen;
	IFS_Memcpy( net_data->post_data, data, dlen );
	net_data->post_data[dlen] = 0;
	net_data->method = FS_METHOD_POST;
	FS_NetConnect( FS_WebConfigGetApn(), FS_WebConfigGetUserName(), FS_WebConfigGetPassword(),
		FS_WebNetConnCallback, FS_APP_WEB, FS_TRUE, net_data );
}

void FS_QuitTransSession( void )
{
	if( GFS_hHttp )
	{
		FS_HttpDestroyHandle( GFS_hHttp );
		GFS_hHttp = FS_NULL;
	}
	if( GFS_hWsp )
	{
		FS_WspDestroyHandle( GFS_hWsp );
		GFS_hWsp = FS_NULL;
	}
	FS_ResetNetParam( &GFS_NetParam );
	GFS_NetParam.win = FS_NULL;	/* FS_ResetNetParam will keep the win. we reset it here. */
	FS_ResetWebRequest( );
}

void FS_WebUtilDeinit( void )
{
	FS_QuitTransSession( );
	FS_SAFE_FREE( GFS_CurWebPageUrl );
	FS_SAFE_FREE( GFS_CurRequestUrl );
	FS_SAFE_FREE( GFS_LastWebPageUrl );
}

void FS_LoadWebDoc( FS_Window *win, FS_CHAR *file, FS_CHAR *card, 
	FS_BOOL bSetTop, FS_BOOL bPushHistory )
{
	FS_CHAR *ext, *charset = FS_NULL;
	FS_HistoryEntry *he;
	
	ext = FS_GetFileExt( file );
	if( ! ext ) return;
	
	FS_ClearWebWinContext( win );
	he = FS_CacheFindEntryByFile( file );
	if( he )
	{
		charset = he->charset;
		if( bPushHistory ) FS_HistoryPushEntry( he->url, he->file );
	}

	if( bSetTop )
		FS_WindowSetViewPort( win, FS_HistoryFindEntryViewportByFile( file ) );
	
	if( IFS_Stricmp(ext, "html") == 0 || IFS_Stricmp(ext, "htm") == 0 )
		FS_HtmlProcessFile( win, file, charset, FS_NULL );
	else if( IFS_Stricmp(ext, "wml") == 0 )
		FS_WmlProcessFile( win, file, card, FS_NULL );
	else if( IFS_Stricmp(ext, "bwml") == 0 )
		FS_BinWmlProcessFile( win, file, card );

	FS_LoadWebDocImages( win );
	/* remove current tasks */
	FS_RemoveTaskList( win );	
	/* stop timer task if any */
	FS_ResetWebRequest( );
	
	FS_SetFocusToEyeableWebWgt( win );
}

void FS_SetCurWebPageUrl( FS_CHAR *url )
{
	FS_COPY_TEXT( GFS_LastWebPageUrl, GFS_CurWebPageUrl );
	FS_COPY_TEXT( GFS_CurWebPageUrl, url );
}

void FS_RestoreWebPageUrl( void )
{
	FS_COPY_TEXT( GFS_CurWebPageUrl, GFS_LastWebPageUrl );
	FS_SAFE_FREE( GFS_LastWebPageUrl );
}

void FS_SetCurRequestUrl( FS_CHAR *url )
{
	FS_COPY_TEXT( GFS_CurRequestUrl, url );
}

FS_CHAR *FS_GetCurWebPageUrl( void )
{ 
	return GFS_CurWebPageUrl;
}

FS_CHAR *FS_GetCurRequestUrl( void )
{ 
	return GFS_CurRequestUrl;
}

FS_BOOL FS_WebNetBusy( void )
{
	if( GFS_WebRequest.state == FS_WEB_REQ_STS_NONE ) 
		return FS_FALSE;
	else
		return FS_TRUE;
}

static void FS_WebPageDownloadComplete( void *dummy )
{
	FS_HttpRequestCancel( GFS_hHttp, FS_TRUE );
}

void FS_WebHandleEvent( FS_UINT1 event, FS_UINT4 param )
{
	FS_WebEventParam *ev = (FS_WebEventParam *)param;
	switch( event )
	{
		case FS_WEB_EV_GET:
			FS_GotoUrl( ev->win, ev->url, ev->send_referer );
			IFS_Free( ev->url );
			IFS_Free( ev );
			break;
		case FS_WEB_EV_POST:
			FS_PostData( ev->win, ev->url, ev->data, ev->dlen, ev->send_referer );
			IFS_Free( ev->url );
			IFS_Free( ev->data );
			IFS_Free( ev );
			break;
		default:
			break;
	}
}

/*
	when parser encouter a end doc tag, eg. </html> etc.
	use this to avoid stupid waiting for some server that 
	did not send Content-Lenght header fields.
*/
void FS_WebDocParseEnd( void )
{
	if( GFS_hHttp )
	{
		IFS_PostMessage( FS_MSG_UTIL_CALL, ( FS_UINT4 )FS_WebPageDownloadComplete );
	}
}

#endif	//FS_MODULE_WEB


