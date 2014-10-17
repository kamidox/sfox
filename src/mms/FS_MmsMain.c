#include "inc/FS_Config.h"

#ifdef FS_MODULE_MMS

#include "inc/FS_Config.h"
#include "inc/gui/FS_Gui.h"
#include "inc/gui/FS_WebGui.h"
#include "inc/res/FS_Res.h"
#include "inc/mms/FS_MmsFile.h"
#include "inc/mms/FS_MmsList.h"
#include "inc/mms/FS_MmsNet.h"
#include "inc/mms/FS_MmsConfig.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_Mime.h"
#include "inc/util/FS_NetConn.h"
#include "inc/util/FS_MemDebug.h"

#define FS_MMS_DETAIL_LEN			1024
#define FS_MAX_MMS_ADDR_LEN			1024
#define FS_MMS_AUDIO_DELAY			1000

typedef struct FS_MmsUIViewFormData_Tag
{
	FS_MmsFile *		mms;
	FS_SINT4			frame_count;
	FS_BOOL				preview;
	FS_SINT4			cur_frame;
	FS_UINT4			timer_id;
	FS_UINT4			audio_delay_timer;
	FS_BOOL				playing_audio;
	FS_BOOL				timer_enable;
}FS_MmsUIViewFormData;

typedef struct FS_MmsNetState_Tag
{
	FS_SINT4			second;
	FS_SINT4			offset;
	FS_SINT4			total;
	FS_SINT2			text_id;
	FS_UINT4			timer_id;
}FS_MmsNetState;

static FS_MmsUIViewFormData GFS_MmsUIViewFormData;
static FS_MmsHead *GFS_CurEditMmsHead;
static FS_BOOL GFS_CurEditMmsSaved;
static FS_MmsNetState GFS_MmsNetState;

static void FS_MmsViewNextFrame_CB( FS_Window *win );
static void FS_MmsSave_CB( FS_Window *win );
static FS_BOOL FS_MmsSendFormSaveData( void );
static FS_MmsHead * FS_MmsEditFormSaveMms( FS_BOOL is_send );
static void FS_MmsView_CB( FS_Window *win );
static void FS_MmsRecv_CB( FS_Window *win );
static void FS_MmsEdit_UI( FS_MmsFile *pMmsFile );
static void FS_MmsListBuild( FS_Window *win, FS_UINT1 mbox );
static void FS_MmsEditFormUpdateMmsDetailText( void );

void FS_MmsSysInit( void )
{
	FS_SystemInit( );
	FS_MmsConfigInit( );
}

static void FS_MmsEditFormSaveData( FS_Window *win, FS_MmsFile *pMmsFile )
{
	if( pMmsFile == FS_NULL )
	{
		pMmsFile = FS_CreateMmsFile( );
		FS_MmsFileAddFrame( pMmsFile, 0 );
	}
	else
	{
		if( FS_MmsFileGetFrameCount( pMmsFile ) <= 0 )
			FS_MmsFileAddFrame( pMmsFile, 0 );
	}
	win->private_data = (FS_UINT4)pMmsFile;		/* current edit mms file */
	win->private_data2 = 1;						/* current focus frame */
	GFS_CurEditMmsSaved = FS_TRUE;
}

static void FS_MmsEditFormContentChanged( void )
{
	GFS_CurEditMmsSaved = FS_FALSE;
}

static FS_MmsFile *FS_MmsEditFormGetMmsFile( void )
{
	FS_Window *win = FS_WindowFindId( FS_W_MmsEditFrm );
	if( win ){
		return (FS_MmsFile *)win->private_data;
	}else{
		return FS_NULL;
	}
}

static void FS_MmsEditFormSaveSubject( FS_CHAR *sub )
{
	FS_MmsFile *pMmsFile = FS_MmsEditFormGetMmsFile( );
	FS_COPY_TEXT( pMmsFile->data.head.subject, sub );
}

static void FS_MmsEditFormSetToAddr( FS_CHAR *addr )
{
	FS_MmsFile *pMmsFile = FS_MmsEditFormGetMmsFile( );
	FS_COPY_TEXT( pMmsFile->data.head.to, addr );
}

static void FS_MmsEditFormSetCcAddr( FS_CHAR *addr )
{
	FS_MmsFile *pMmsFile = FS_MmsEditFormGetMmsFile( );
	FS_COPY_TEXT( pMmsFile->data.head.cc, addr );
}

static void FS_MmsEditFormSetBccAddr( FS_CHAR *addr )
{
	FS_MmsFile *pMmsFile = FS_MmsEditFormGetMmsFile( );
	FS_COPY_TEXT( pMmsFile->data.head.bcc, addr );
}

static FS_CHAR *FS_MmsEditFormGetSubject( void )
{
	FS_MmsFile *pMmsFile = FS_MmsEditFormGetMmsFile( );
	return pMmsFile->data.head.subject;
}

static FS_SINT4 FS_MmsEditFormGetCurFrameNum( void )
{
	FS_Window *win = FS_WindowFindId( FS_W_MmsEditFrm );
	if( win ){
		return (FS_SINT4)win->private_data2;
	}else{
		return FS_NULL;
	}
}

static FS_CHAR * FS_MmsEditFormFormatFrameNumText( FS_SINT4 num )
{
	static FS_CHAR s_str[16];
	FS_Window *win = FS_WindowFindId( FS_W_MmsEditFrm );

	if( num < 0 )
		IFS_Sprintf( s_str, FS_Text(FS_T_FRAME_NUM), win->private_data2 );
	else
		IFS_Sprintf( s_str, FS_Text(FS_T_FRAME_NUM), num );

	return s_str;
}

static FS_SINT4 FS_MmsEditFormGetFrameCount( void )
{
	FS_MmsFile *pMmsFile;
	FS_Window *win = FS_WindowFindId( FS_W_MmsEditFrm );

	pMmsFile = (FS_MmsFile *)win->private_data;
	return FS_SmilGetFrameCount( pMmsFile->smil );
}

static void FS_MmsEditFormSetCurFrame_HD( void )
{
	FS_MmsFile *pMmsFile;
	FS_CHAR *file, *str;
	FS_SINT4 size;
	FS_Widget *wgt;
	FS_Window *win = FS_WindowFindId( FS_W_MmsEditFrm );
	FS_SINT4 frameNum = (FS_SINT4)win->private_data2;
	
	win->pane.view_port.top = IFS_GetWinTitleHeight( );
	pMmsFile = FS_MmsEditFormGetMmsFile( );
	/* load frame text */
	file = FS_MmsFileGetFrameTextFile( pMmsFile, frameNum );
	wgt = FS_WindowGetWidget( win, FS_W_MmsEditText );
	if( file )
	{
		size = FS_FileGetSize( -1, file );
		str = IFS_Malloc( size + 1 );
		if( str )
		{
			FS_FileRead( -1, file, 0, str, size );
			str[ size ] = 0;
			FS_WidgetSetText( wgt, str );
			IFS_Free( str );
		}
	}
	else
	{
		FS_WidgetSetText( wgt, FS_NULL );
	}
	FS_WidgetSetFocus( win, wgt );
	
	/* load frame image */
	file = FS_MmsFileGetFrameImageFile( pMmsFile, frameNum );
	if( file )
	{
		wgt = FS_WindowGetWidget( win, FS_W_MmsEditImage );
		str = FS_MmsFileGetFrameImageFileName( pMmsFile, frameNum );
		FS_WidgetSetText( wgt, str );
		
		wgt = FS_WindowGetWidget( win, FS_W_MmsEditImageArea );
		FS_WidgetSetImage( wgt, file );
	}
	else
	{
		wgt = FS_WindowGetWidget( win, FS_W_MmsEditImage );
		FS_WidgetSetText( wgt, FS_Text(FS_T_ADD_IMAGE) );
		wgt = FS_WindowGetWidget( win, FS_W_MmsEditImageArea );
		FS_WidgetSetImage( wgt, FS_NULL );
	}
	
	/* load frame video */
	file = FS_MmsFileGetFrameVideoFile( pMmsFile, frameNum );
	wgt = FS_WindowGetWidget( win, FS_W_MmsEditVideo );
	if( file )
	{
		str = FS_MmsFileGetFrameVideoFileName( pMmsFile, frameNum );
		FS_WidgetSetText( wgt, str );
	}
	else
	{
		FS_WidgetSetText( wgt, FS_Text(FS_T_ADD_VIDEO) );
	}
	
	/* load frame audio */
	file = FS_MmsFileGetFrameAudioFile( pMmsFile, frameNum );
	wgt = FS_WindowGetWidget( win, FS_W_MmsEditAudio );
	if( file )
	{
		str = FS_MmsFileGetFrameAudioFileName( pMmsFile, frameNum );
		FS_WidgetSetText( wgt, str );
	}
	else
	{
		FS_WidgetSetText( wgt, FS_Text(FS_T_ADD_AUDIO) );
	}
	
	/* set frame number */
	wgt = FS_WindowGetWidget( win, FS_W_MmsEditFrames );
	FS_WidgetSetText( wgt, FS_MmsEditFormFormatFrameNumText(frameNum) );
	FS_MmsEditFormUpdateMmsDetailText( );
	FS_InvalidateRect( win, FS_NULL );
	FS_DestroyWindowByID( FS_W_ProgressFrm );
}

static void FS_MmsEditFormSetCurFrame( FS_SINT4 frameNum )
{
	FS_Window *win = FS_WindowFindId( FS_W_MmsEditFrm );
	FS_MmsFile *pMmsFile = FS_MmsEditFormGetMmsFile( );
	
	if( frameNum > FS_MmsEditFormGetFrameCount( ) )
		return;	
	/* set frame number */
	win->private_data2 = frameNum;
	if( ! FS_MmsFileIsEmptyFrame(pMmsFile, frameNum) ){
		FS_MessageBox( FS_MS_NONE, FS_Text(FS_T_PLS_WAITING), FS_NULL, FS_FALSE );
	}
	IFS_PostMessage( FS_MSG_UTIL_CALL, (FS_UINT4)FS_MmsEditFormSetCurFrame_HD );	
}

/* return content-id or NULL */
static FS_CHAR *FS_MmsEditFormGetCurFrameImage( void )
{
	FS_MmsFile *pMmsFile;
	FS_UINT1 frameNum;
	FS_Window *win = FS_WindowFindId( FS_W_MmsEditFrm );

	pMmsFile = (FS_MmsFile *)win->private_data;
	frameNum = (FS_UINT1)win->private_data2;
	return FS_SmilGetImageCid( pMmsFile->smil, frameNum );
}

/* return content-id or NULL */
static FS_CHAR *FS_MmsEditFormGetCurFrameVideo( void )
{
	FS_MmsFile *pMmsFile;
	FS_UINT1 frameNum;
	FS_Window *win = FS_WindowFindId( FS_W_MmsEditFrm );

	pMmsFile = (FS_MmsFile *)win->private_data;
	frameNum = (FS_UINT1)win->private_data2;
	return FS_SmilGetVideoCid( pMmsFile->smil, frameNum );
}

/* return content-id or NULL */
static FS_CHAR *FS_MmsEditFormGetCurFrameAudio( void )
{
	FS_MmsFile *pMmsFile;
	FS_UINT1 frameNum;
	FS_Window *win = FS_WindowFindId( FS_W_MmsEditFrm );

	pMmsFile = (FS_MmsFile *)win->private_data;
	frameNum = (FS_UINT1)win->private_data2;
	return FS_SmilGetAudioCid( pMmsFile->smil, frameNum );
}

/* return content-id or NULL */
static FS_CHAR *FS_MmsEditFormGetCurFrameText( void )
{
	FS_MmsFile *pMmsFile;
	FS_UINT1 frameNum;
	FS_Window *win = FS_WindowFindId( FS_W_MmsEditFrm );

	pMmsFile = (FS_MmsFile *)win->private_data;
	frameNum = (FS_UINT1)win->private_data2;
	return FS_SmilGetTextCid( pMmsFile->smil, frameNum );
}

static FS_BOOL FS_MmsEditFormGetReadReportFlag( void )
{
	FS_MmsFile *pMmsFile;
	FS_Window *win = FS_WindowFindId( FS_W_MmsEditFrm );
	
	pMmsFile = (FS_MmsFile *)win->private_data;
	return FS_MmsFileGetReadReportFlag( pMmsFile );
}

static void FS_MmsEditFormSetReadReportFlag( FS_BOOL bSet )
{
	FS_MmsFile *pMmsFile;
	FS_Window *win = FS_WindowFindId( FS_W_MmsEditFrm );
	
	pMmsFile = (FS_MmsFile *)win->private_data;
	FS_MmsFileSetReadReportFlag( pMmsFile, bSet );
}

static FS_BOOL FS_MmsEditFormGetDlvReportFlag( void )
{
	FS_MmsFile *pMmsFile;
	FS_Window *win = FS_WindowFindId( FS_W_MmsEditFrm );
	
	pMmsFile = (FS_MmsFile *)win->private_data;
	return FS_MmsFileGetDlvReportFlag( pMmsFile );
}

static void FS_MmsEditFormSetDlvReportFlag( FS_BOOL bSet )
{
	FS_MmsFile *pMmsFile;
	FS_Window *win = FS_WindowFindId( FS_W_MmsEditFrm );
	
	pMmsFile = (FS_MmsFile *)win->private_data;
	FS_MmsFileSetDlvReportFlag( pMmsFile, bSet );
}

static FS_CHAR *FS_MmsEditFormGetMmsDetailText( void )
{
	static FS_CHAR s_detail[64];
	FS_SINT4 nFrames, nSize, nCur;
	FS_MmsFile *pMmsFile = FS_MmsEditFormGetMmsFile( );
	nSize = FS_MmsFileGetObjectTotalSize( pMmsFile );

	nFrames = FS_MmsEditFormGetFrameCount( );
	nCur = FS_MmsEditFormGetCurFrameNum( );
	IFS_Sprintf( s_detail, FS_Text(FS_T_FORMAT_MMS_DETAIL), nCur, nFrames, FS_KB(nSize) );
	return s_detail;
}

static void FS_MmsEditFormUpdateMmsDetailText( void )
{
	FS_CHAR *detail = FS_MmsEditFormGetMmsDetailText( );
	FS_Window *win = FS_WindowFindId( FS_W_MmsEditFrm );
	FS_Widget *wImage, *wAudio, *wFrames, *wVideo;
	wImage = FS_WindowGetWidget( win, FS_W_MmsEditImage );
	wAudio = FS_WindowGetWidget( win, FS_W_MmsEditAudio );
	wVideo = FS_WindowGetWidget( win, FS_W_MmsEditVideo );
	wFrames = FS_WindowGetWidget( win, FS_W_MmsEditFrames );
	wImage->tip = detail;
	wAudio->tip = detail;
	wVideo->tip = detail;
	wFrames->tip = detail;

	FS_RedrawWinStatusBar( win );
}

static void FS_MmsEditFormDelCurFrameImage( void )
{
	FS_Widget *wgt;
	FS_Window *win;
	FS_MmsFile *pMmsFile = FS_MmsEditFormGetMmsFile( );
	FS_CHAR *cid = FS_MmsEditFormGetCurFrameImage( );
	if( cid )
	{
		win = FS_WindowFindId( FS_W_MmsEditFrm );
		FS_MmsCodecDeleteEntry( &pMmsFile->data, cid );
		FS_SmilDelFrameImage( pMmsFile->smil, FS_MmsEditFormGetCurFrameNum() );
		wgt = FS_WindowGetWidget( win, FS_W_MmsEditImageArea );
		FS_WidgetSetImage( wgt, FS_NULL );
		wgt = FS_WindowGetWidget( win, FS_W_MmsEditImage );
		FS_WidgetSetText( wgt, FS_Text(FS_T_ADD_IMAGE) );

		FS_InvalidateRect( win, &win->client_rect );
	}
}

static void FS_MmsEditFormDelCurFrameVideo( void )
{
	FS_Widget *wgt;
	FS_Window *win;
	FS_MmsFile *pMmsFile = FS_MmsEditFormGetMmsFile( );
	FS_CHAR *cid = FS_MmsEditFormGetCurFrameVideo( );
	if( cid )
	{
		FS_MmsCodecDeleteEntry( &pMmsFile->data, cid );
		FS_SmilDelFrameVideo( pMmsFile->smil, FS_MmsEditFormGetCurFrameNum() );
		win = FS_WindowFindId( FS_W_MmsEditFrm );
		wgt = FS_WindowGetWidget( win, FS_W_MmsEditVideo );
		FS_WidgetSetText( wgt, FS_Text(FS_T_ADD_VIDEO) );

		FS_InvalidateRect( win, &win->client_rect );
	}
}

static void FS_MmsEditFormDelCurFrameAudio( void )
{
	FS_Widget *wgt;
	FS_Window *win;
	FS_MmsFile *pMmsFile = FS_MmsEditFormGetMmsFile( );
	FS_CHAR *cid = FS_MmsEditFormGetCurFrameAudio( );
	if( cid )
	{
		FS_MmsCodecDeleteEntry( &pMmsFile->data, cid );
		FS_SmilDelFrameAudio( pMmsFile->smil, FS_MmsEditFormGetCurFrameNum() );
		win = FS_WindowFindId( FS_W_MmsEditFrm );
		wgt = FS_WindowGetWidget( win, FS_W_MmsEditAudio );
		FS_WidgetSetText( wgt, FS_Text(FS_T_ADD_AUDIO) );

		FS_InvalidateRect( win, &win->client_rect );
	}
}

static void FS_MmsEditFormDelCurFrameText( void )
{
	FS_MmsFile *pMmsFile;
	FS_CHAR *content_id;
	FS_Window *win;
	FS_Widget *wgt;
	
	pMmsFile = FS_MmsEditFormGetMmsFile( );
	content_id = FS_MmsEditFormGetCurFrameText( );
	if( content_id )
	{
		win = FS_WindowFindId( FS_W_MmsEditFrm );
		wgt = FS_WindowGetWidget( win, FS_W_MmsEditText );
		if( wgt->text )
			FS_WidgetSetText( wgt, FS_NULL );
		FS_MmsCodecDeleteEntry( &pMmsFile->data, content_id );
		FS_SmilDelFrameText( pMmsFile->smil, FS_MmsEditFormGetCurFrameNum() );
	}
}

static void FS_MmsEditFormDelCurFrame( void )
{
	FS_MmsFile *pMmsFile;
	pMmsFile = FS_MmsEditFormGetMmsFile( );
	FS_MmsEditFormDelCurFrameImage( );
	FS_MmsEditFormDelCurFrameVideo( );
	FS_MmsEditFormDelCurFrameAudio( );
	FS_MmsEditFormDelCurFrameText( );
	if( FS_MmsEditFormGetFrameCount() > 1 )
		FS_SmilDelMmsFrame( pMmsFile->smil, FS_MmsEditFormGetCurFrameNum() );
	FS_MmsEditFormUpdateMmsDetailText( );
}

static void FS_MmsEditFormShowImage_HD( void )
{
	FS_Window *win = FS_WindowFindId( FS_W_MmsEditFrm );
	
	FS_InvalidateRect( win, FS_NULL );
	FS_DestroyWindowByID( FS_W_ProgressFrm );
}

static void FS_MmsEditFormShowImage( FS_CHAR *absFile )
{
	FS_Window *win = FS_WindowFindId( FS_W_MmsEditFrm );
	FS_Widget *wgt;
	FS_CHAR *str;
	
	wgt = FS_WindowGetWidget( win, FS_W_MmsEditImage );
	str = FS_GetFileNameFromPath( absFile );
	FS_WidgetSetText( wgt, str );
	
	wgt = FS_WindowGetWidget( win, FS_W_MmsEditImageArea );
	FS_WidgetSetImage( wgt, absFile );
	FS_MessageBox( FS_MS_NONE, FS_Text(FS_T_PLS_WAITING), FS_NULL, FS_FALSE );
	IFS_PostMessage( FS_MSG_UTIL_CALL, (FS_UINT4)FS_MmsEditFormShowImage_HD );
}

static void FS_MmsEditFormShowVideo( FS_CHAR *absFile )
{
	FS_Window *win = FS_WindowFindId( FS_W_MmsEditFrm );
	FS_Widget *wgt;
	FS_CHAR *str;
	
	wgt = FS_WindowGetWidget( win, FS_W_MmsEditVideo );
	str = FS_GetFileNameFromPath( absFile );
	FS_WidgetSetText( wgt, str );
	
	FS_RedrawWidget( win, wgt );
}

static void FS_MmsEditFormShowAudio( FS_CHAR *absFile )
{
	FS_Window *win = FS_WindowFindId( FS_W_MmsEditFrm );
	FS_Widget *wgt;
	FS_CHAR *str;
	
	wgt = FS_WindowGetWidget( win, FS_W_MmsEditAudio );
	str = FS_GetFileNameFromPath( absFile );
	FS_WidgetSetText( wgt, str );
	
	FS_RedrawWidget( win, wgt );
}

static FS_CHAR *FS_MmsViewFormGetFrameVideoName( FS_SINT4 num )
{
	FS_MmsFile *pMmsFile = GFS_MmsUIViewFormData.mms;
	FS_CHAR *cid, *ret = FS_NULL;
	FS_MmsEncEntry *pEntry = FS_NULL;
		
	if( pMmsFile->smil )
	{
		cid = FS_SmilGetVideoCid( pMmsFile->smil, num );
		if( cid )
		{
			pEntry = FS_MmsFileGetEntryByCid( pMmsFile, cid );
		}
	}
	else	/* no smil mms */
	{
		pEntry = FS_MmsFileGetEntryByIdx( pMmsFile, FS_MIME_VIDEO, num );
	}
	
	if( pEntry )
	{
		if( pEntry->content_location )
			ret = pEntry->content_location;
		else if( pEntry->param_name )
			ret = pEntry->param_name;
		else
			ret = pEntry->file;
	}
	return ret;
}


static FS_CHAR *FS_MmsViewFormGetFrameAudioName( FS_SINT4 num )
{
	FS_MmsFile *pMmsFile = GFS_MmsUIViewFormData.mms;
	FS_CHAR *cid, *ret = FS_NULL;
	FS_MmsEncEntry *pEntry = FS_NULL;
		
	if( pMmsFile->smil )
	{
		cid = FS_SmilGetAudioCid( pMmsFile->smil, num );
		if( cid )
		{
			pEntry = FS_MmsFileGetEntryByCid( pMmsFile, cid );
		}
	}
	else	/* no smil mms */
	{
		pEntry = FS_MmsFileGetEntryByIdx( pMmsFile, FS_MIME_AUDIO, num );
	}
	
	if( pEntry )
	{
		if( pEntry->content_location )
			ret = pEntry->content_location;
		else if( pEntry->param_name )
			ret = pEntry->param_name;
		else
			ret = pEntry->file;
	}
	return ret;
}

static FS_CHAR *FS_MmsViewFormGetFrameTextName( FS_SINT4 num )
{
	FS_MmsFile *pMmsFile = GFS_MmsUIViewFormData.mms;
	FS_CHAR *cid, *ret = FS_NULL;
	FS_MmsEncEntry *pEntry = FS_NULL;
		
	if( pMmsFile->smil )
	{
		cid = FS_SmilGetTextCid( pMmsFile->smil, num );
		if( cid )
		{
			pEntry = FS_MmsFileGetEntryByCid( pMmsFile, cid );
		}
	}
	else	/* no smil mms */
	{
		pEntry = FS_MmsFileGetEntryByIdx( pMmsFile, FS_MIME_TEXT, num );
	}
	
	if( pEntry )
	{
		if( pEntry->content_location )
			ret = pEntry->content_location;
		else if( pEntry->param_name )
			ret = pEntry->param_name;
		else
			ret = pEntry->file;
	}
	return ret;
}

static FS_CHAR *FS_MmsViewFormGetFrameImageName( FS_SINT4 num )
{
	FS_MmsFile *pMmsFile = GFS_MmsUIViewFormData.mms;
	FS_CHAR *cid, *ret = FS_NULL;
	FS_MmsEncEntry *pEntry = FS_NULL;

	if( pMmsFile->smil )
	{
		cid = FS_SmilGetImageCid( pMmsFile->smil, num );
		if( cid )
		{
			pEntry = FS_MmsFileGetEntryByCid( pMmsFile, cid );
		}
	}
	else	/* no smil mms */
	{
		pEntry = FS_MmsFileGetEntryByIdx( pMmsFile, FS_MIME_IMAGE | FS_MIME_VIDEO, num );
	}
	
	if( pEntry )
	{
		if( pEntry->content_location )
			ret = pEntry->content_location;
		else if( pEntry->param_name )
			ret = pEntry->param_name;
		else
			ret = pEntry->file;
	}
	return ret;
}

static FS_CHAR *FS_MmsViewFormGetFrameAudio( FS_SINT4 num )
{
	FS_MmsFile *pMmsFile = GFS_MmsUIViewFormData.mms;
	FS_CHAR *src;

	src = FS_MmsFileGetFrameAudioFile( pMmsFile, num );
	return src;
}

static void FS_MmsViewFormPlayAudio( FS_SINT4 num )
{
	FS_CHAR *src;
	
	src = FS_MmsViewFormGetFrameAudio( num );
	if( src && GFS_MmsUIViewFormData.playing_audio == FS_FALSE )
	{
		GFS_MmsUIViewFormData.playing_audio = FS_TRUE;
		IFS_PlayAudio( src, 1 );
	}
}

static void FS_MmsViewFormStopAudio( void )
{
	if( GFS_MmsUIViewFormData.audio_delay_timer )
	{
		IFS_StopTimer( GFS_MmsUIViewFormData.audio_delay_timer );
		GFS_MmsUIViewFormData.audio_delay_timer = 0;
	}
	if( GFS_MmsUIViewFormData.playing_audio )
	{
		GFS_MmsUIViewFormData.playing_audio = FS_FALSE;
		IFS_StopAudio( );
	}
}

static FS_CHAR *FS_MmsViewFormGetFrameImage( FS_SINT4 num )
{
	FS_MmsFile *pMmsFile = GFS_MmsUIViewFormData.mms;
	FS_CHAR *src;

	return src = FS_MmsFileGetFrameImageFile( pMmsFile, num );
}

static FS_CHAR *FS_MmsViewFormGetFrameVideo( FS_SINT4 num )
{
	FS_MmsFile *pMmsFile = GFS_MmsUIViewFormData.mms;
	FS_CHAR *src;

	return src = FS_MmsFileGetFrameVideoFile( pMmsFile, num );
}

static FS_CHAR *FS_MmsViewFormGetFrameText( FS_SINT4 num, FS_BOOL bText )
{
	FS_MmsFile *pMmsFile = GFS_MmsUIViewFormData.mms;
	FS_CHAR *src;
	FS_SINT4 size;
	FS_CHAR *txt = FS_NULL;
	
	src = FS_MmsFileGetFrameTextFile( pMmsFile, num );

	if( bText )
	{
		/* get mms text. we will alloc memory here. caller take response to free it. */
		if( src )
		{
			size = FS_FileGetSize( -1, src );
			if( size > 0 )
			{
				txt = IFS_Malloc( size + 1 );
				if( txt )
				{
					FS_FileRead( -1, src, 0, txt, size );
					txt[size] = 0;
				}
			}
		}
	}
	else
	{
		txt = src;
	}
	return txt;
}

static FS_SINT4 FS_MmsViewFormGetCurFrameNum( void )
{
	return GFS_MmsUIViewFormData.cur_frame;
}

static FS_SINT4 FS_MmsViewFormGetFrameCount( void )
{
	return GFS_MmsUIViewFormData.frame_count;
}

static FS_SINT4 FS_MmsViewFormGetCurFrameDur( void )
{
	FS_SINT4 ret = 0;

	if( GFS_MmsUIViewFormData.mms->smil )
		ret = FS_SmilGetFrameDur( GFS_MmsUIViewFormData.mms->smil, GFS_MmsUIViewFormData.cur_frame );
	
	return ret;
}

static void FS_MmsViewFormStopTimer( void )
{
	if( GFS_MmsUIViewFormData.timer_id )
	{
		IFS_StopTimer( GFS_MmsUIViewFormData.timer_id );
		GFS_MmsUIViewFormData.timer_id = 0;
	}
}

static void FS_MmsViewFormStartTimer( void )
{
	FS_SINT4 dur;
	if( GFS_MmsUIViewFormData.timer_enable && GFS_MmsUIViewFormData.timer_id == 0 )
	{
		dur = FS_MmsViewFormGetCurFrameDur( );
		if( dur > 0 )
		{
			GFS_MmsUIViewFormData.timer_id = IFS_StartTimer( FS_TIMER_ID_MMS_VIEW, dur * 1000, FS_MmsViewNextFrame_CB, FS_NULL );
		}
	}
}

static FS_BOOL FS_MmsViewFormIsPreview( void )
{
	return GFS_MmsUIViewFormData.preview;
}

FS_MmsFile *FS_MmsViewFormGetMmsFile( void )
{
	return GFS_MmsUIViewFormData.mms;
}

static FS_CHAR * FS_MmsViewFormGetSubject( void )
{
	FS_MmsFile *pMmsFile = FS_MmsViewFormGetMmsFile( );
	return pMmsFile->data.head.subject;
}

static void FS_MmsViewFormSaveData( FS_Window *win, FS_MmsFile *pMmsFile, FS_BOOL preview )
{
	IFS_Memset( &GFS_MmsUIViewFormData, 0, sizeof(FS_MmsUIViewFormData) );
	GFS_MmsUIViewFormData.mms = pMmsFile;
	GFS_MmsUIViewFormData.preview = preview;
	GFS_MmsUIViewFormData.cur_frame = 1;
	GFS_MmsUIViewFormData.frame_count = FS_MmsFileGetFrameCount( pMmsFile );
	GFS_MmsUIViewFormData.playing_audio = FS_FALSE;
	GFS_MmsUIViewFormData.timer_enable = FS_FALSE;	/* for cmcc spec: default to manual play */
}

static void FS_MmsViewFormDisableTimer( void )
{
	GFS_MmsUIViewFormData.timer_enable = FS_FALSE;
}

static void FS_MmsViewFormEnableTimer( void )
{
	GFS_MmsUIViewFormData.timer_enable = FS_TRUE;
}

static FS_BOOL FS_MmsViewFormIsTimerEnable( void )
{
	return GFS_MmsUIViewFormData.timer_enable;
}

static void FS_MmsViewFormPlayAudio_HD( void )
{
	/* play audio if any */
	FS_MmsViewFormPlayAudio( GFS_MmsUIViewFormData.cur_frame );	
}

static void FS_MmsViewFormPlayAudioTimerCallback( void *dummy )
{
	GFS_MmsUIViewFormData.audio_delay_timer = 0;
	IFS_PostMessage( FS_MSG_UTIL_CALL, (FS_UINT4)FS_MmsViewFormPlayAudio_HD );
}

static void FS_MmsViewFormPlayAudioDelay( void )
{
	GFS_MmsUIViewFormData.audio_delay_timer = IFS_StartTimer( FS_TIMER_ID_MMS_AUDIO, FS_MMS_AUDIO_DELAY, FS_MmsViewFormPlayAudioTimerCallback, FS_NULL );
}

static void FS_MmsViewFormShowFrame_HD( void )
{
	FS_WebWgt *wTxt = FS_NULL, *wImg = FS_NULL, *wVdo;
	FS_CHAR *src;
	FS_Window *vwin = FS_WindowFindId( FS_W_MmsViewFrm );
	FS_MmsFile *pMmsFile = FS_MmsViewFormGetMmsFile( );
	FS_SINT4 num = GFS_MmsUIViewFormData.cur_frame;
	
	FS_ClearWebWinContext( vwin );
	FS_MmsViewFormStopAudio( );
	FS_MmsViewFormStopTimer( );
	/* txt */
	src = FS_MmsViewFormGetFrameText( num, FS_TRUE );
	if( src )
	{
		wTxt = FS_WwCreateText( src );
		IFS_Free( src );
	}
	/* image */
	src = FS_MmsViewFormGetFrameImage( num );
	if( src )
	{
		wImg = FS_WwCreateImage( FS_NULL, FS_NULL, FS_NULL );
		FS_WWGT_SET_IMAGE( wImg );
		FS_WWGT_SET_MID_ALIGN( wImg );
		wImg->file = IFS_Strdup( src );
	}

	/* layout */
	if( FS_SmilLayoutImageFirst( pMmsFile->smil ) )
	{
		if( wImg ) FS_WebWinAddWebWgt( vwin, wImg, 0 );
		if( wTxt ) FS_WebWinAddWebWgt( vwin, wTxt, 1 );
	}
	else
	{
		if( wTxt ) FS_WebWinAddWebWgt( vwin, wTxt, 0 );
		if( wImg ) FS_WebWinAddWebWgt( vwin, wImg, 1 );
	}
	/* video */
	src = FS_MmsViewFormGetFrameVideo( num );
	if( src )
	{
		wVdo = FS_WwCreateText( FS_Text(FS_T_MMS_VIDEO_TIP) );
		FS_WebWinAddWebWgt( vwin, wVdo, 2 );
	}
	
	FS_WindowSetListIndex( vwin, num, FS_MmsViewFormGetFrameCount() );
	FS_InvalidateRect( vwin, FS_NULL );		
	/* start timer if any */
	FS_MmsViewFormStartTimer( );
	/* audio */
	FS_MmsViewFormPlayAudioDelay( );
	/* destroy waiting window */
	FS_DestroyWindowByID( FS_W_ProgressFrm );
}

static void FS_MmsViewFormShowFrame( FS_SINT4 num )
{
	FS_MmsFile *pMmsFile = FS_MmsViewFormGetMmsFile( );
	GFS_MmsUIViewFormData.cur_frame = num;
	if( ! FS_MmsFileIsEmptyFrame(pMmsFile, num) ){
		FS_MessageBox( FS_MS_NONE, FS_Text(FS_T_PLS_WAITING), FS_NULL, FS_FALSE );
	}
	IFS_PostMessage( FS_MSG_UTIL_CALL, (FS_UINT4)FS_MmsViewFormShowFrame_HD );
}

static FS_UINT1 FS_MmsListFormGetMBox( void )
{
	FS_UINT1 mbox = 0xFF;
	FS_Window *win = FS_WindowFindId( FS_W_MmsListFrm );
	if( win )
	{
		mbox = (FS_UINT1)win->private_data;
	}
 	return mbox;
}

static void FS_MmsListFormUpdateCurItem( FS_MmsHead *mms )
{
	FS_Window *win = FS_WindowFindId( FS_W_MmsListFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( win );
	FS_UINT1 mbox;
	FS_CHAR *str, sdate[32];

	if( wgt )
	{
		mbox = (FS_UINT1)win->private_data;
		if( mbox == FS_MMS_INBOX )
		{
			if( ! FS_MMS_IS_NTF(mms) )
			{
				FS_WidgetSetHandler( wgt, FS_MmsView_CB );
			}
			else
			{
				FS_WidgetSetHandler( wgt, FS_MmsRecv_CB );
			}

			if( FS_MMS_UNREAD(mms) )
			{
				if( FS_MMS_IS_NTF(mms) )
					FS_WidgetSetIcon( wgt, FS_I_NEW_NTF );
				else
					FS_WidgetSetIcon( wgt, FS_I_NEW_MSG );
			}
			else
			{
				FS_WidgetSetIcon( wgt, FS_I_READED_MSG );
			}
		}
		if( mms->subject[0] )
			str = mms->subject;
		else
			str = FS_Text( FS_T_NO_SUBJECT );
		
		FS_WidgetSetText( wgt, str );
		FS_DateStruct2DispStrShortForm( sdate, &mms->date );
		FS_COPY_TEXT( wgt->sub_cap, sdate );
	}
}

static void FS_MmsViewNextFrame_CB( FS_Window *win )
{
	FS_SINT4 num;

	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
	
	if( win == FS_NULL ) GFS_MmsUIViewFormData.timer_id = 0;
	if( win || FS_WindowIsTopMost( FS_W_MmsViewFrm ) )
	{
		num = FS_MmsViewFormGetCurFrameNum( );
		if( num < FS_MmsViewFormGetFrameCount() )
			FS_MmsViewFormShowFrame( num + 1 );
	}
	else
	{
		/* we restart the timer */
		FS_MmsViewFormStartTimer( );
	}
	
}

FS_MmsHead *FS_MmsListFormGetHead( void )
{
	FS_Window *lwin = FS_WindowFindId( FS_W_MmsListFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( lwin );

	if( wgt )
	{
		return (FS_MmsHead *)wgt->private_data;
	}
	else
	{
		return FS_NULL;
	}
}

static void FS_MmsViewPrevFrame_CB( FS_Window *win )
{
	FS_SINT4 num;
	
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
	
	num = FS_MmsViewFormGetCurFrameNum( );
	if( num > 1 )
		FS_MmsViewFormShowFrame( num - 1 );
}

static void FS_MmsViewRestart_CB( FS_Window *win )
{
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
	FS_MmsViewFormShowFrame( 1 );
}

static void FS_MmsViewDisableTimer_CB( FS_Window *win )
{
	FS_MmsViewFormDisableTimer( );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_MmsViewEnableTimer_CB( FS_Window *win )
{
	FS_MmsViewFormEnableTimer( );
	FS_MmsViewFormShowFrame( 1 );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_MmsDisplayNetStatus( FS_SINT4 status, FS_SINT4 errno )
{
	FS_CHAR *str, txt[64];
	FS_BOOL bSendMms = FS_TRUE;
	FS_Window *swin = FS_WindowFindId( FS_W_MmsSendingDlg );

	if( swin == FS_NULL )
	{
		bSendMms = FS_FALSE;
		swin = FS_WindowFindId( FS_W_MmsRecvingDlg );
	}
	
	if( swin )
	{
		IFS_StopTimer( GFS_MmsNetState.timer_id );
		GFS_MmsNetState.timer_id = 0;
		FS_DestroyWindow( swin );
		IFS_LeaveBackLight( );
		switch( status )
		{
			case FS_MMS_NET_ERR_NET:
				if( errno != 0 )
				{
					IFS_Sprintf( txt, "%s\n(errno = 0x%x)", FS_Text(FS_T_SERVER_ERR), errno );
					str = txt;
				}
				else
				{
					str = FS_Text( FS_T_NET_ERR );
				}
				break;
			case FS_MMS_NET_ERR_CONN:
				str = FS_Text( FS_T_DIAL_FAILED );
				break;			
			case FS_MMS_NET_ERR_FILE:
				str = FS_Text( FS_T_FILE_NOT_EXIST );
				break;
			case FS_MMS_NET_ERR_BUSY:
				str = FS_Text( FS_T_ERR_NET_BUSY );
				break;
			case FS_MMS_NET_ERR_MEMORY:
				str = FS_Text( FS_T_ERR_MEMORY );
				break;		
			case FS_MMS_NET_OK:
				if( bSendMms )
					str = FS_Text( FS_T_SENDOK );
				else
					str = FS_Text( FS_T_RECV_OK );
				break;
			default:
				IFS_Sprintf( txt, "%s\n(errno = %d)", FS_Text(FS_T_ERR_UNKNOW), errno );
				str = txt;
		}
		FS_MessageBox( FS_MS_OK, str, FS_NULL, FS_FALSE );
	}
}

static void FS_MmsSendFailed( void )
{
	FS_Window *win;
	FS_Widget *wgt;
	FS_MmsHead *mms;
	FS_UINT1 mbox;
	
	mms = GFS_CurEditMmsHead;
	FS_DestroyWindowByID( FS_W_MmsEditFrm );
	FS_DestroyWindowByID( FS_W_MmsSendFrm );

	if( mms && mms->mbox != FS_MMS_SENDBOX )
	{
		mms->mbox = FS_MMS_SENDBOX;
		FS_MmsSaveHeadList( );
		
		win = FS_WindowFindId( FS_W_MmsListFrm );
		if( win )
		{
			mbox = FS_MmsListFormGetMBox( );
			if( mbox == FS_MMS_DRAFT )
			{
				wgt = FS_WindowGetFocusItem( win );
				FS_WindowDelWidget( win, wgt );
			}
		}
	}
}

static void FS_MmsSendSuccess( FS_CHAR *msg_id )
{
	/* here param is a message-id return by server. we ignore it now */
	FS_Window *win;
	FS_MmsHead *mms;
	FS_Widget *wgt;
	FS_UINT1 mbox;
	
	mms = GFS_CurEditMmsHead;
	FS_DestroyWindowByID( FS_W_MmsEditFrm );
	FS_DestroyWindowByID( FS_W_MmsSendFrm );

	if( mms )
	{
		/* we send a mms from mms draft box or send box */
		mms->mbox = FS_MMS_OUTBOX;
		FS_MmsMoveTop( mms );
		if( msg_id )
			IFS_Strncpy( mms->msg_id, msg_id, FS_MMS_MSG_ID_LEN - 1 );
		if( ! FS_MmsConfigGetSaveSendFlag() )
		{
			FS_MmsDelHead( mms );
		}
		FS_MmsSaveHeadList( );
		
		win = FS_WindowFindId( FS_W_MmsListFrm );
		if( win )
		{
			mbox = FS_MmsListFormGetMBox( );
			if( mbox == FS_MMS_DRAFT || mbox == FS_MMS_SENDBOX )
			{
				wgt = FS_WindowGetFocusItem( win );
				FS_WindowDelWidget( win, wgt );
			}
			else if( mbox == FS_MMS_OUTBOX )
			{
				FS_MmsListBuild( win, mbox );
			}
		}
	}
}

static void FS_MmsRecvSuccess( FS_CHAR *file )
{
	FS_MmsEncHead head;
	FS_MmsHead *mms = FS_MmsListFormGetHead( );
	if( mms && file )
	{
		FS_MMS_CLR_NTF( mms );
		
		IFS_Strcpy( mms->file, file );
		IFS_Free( mms->msg_location );
		mms->msg_location = FS_NULL;

		FS_MmsCodecDecodeFileHead( &head, file );
		FS_MmsAddrToUserFormat( mms->address, sizeof(mms->address), head.from, -1 );
		if( head.subject )
			IFS_Strncpy( mms->subject, head.subject, sizeof(mms->subject) - 1 );
		if( head.read_report == FS_MMS_H_V_READ_REPORT_YES )
			FS_MMS_SET_READ_REPORT(mms);
		if( head.message_id )
			IFS_Strncpy( mms->msg_id, head.message_id, FS_MMS_MSG_ID_LEN - 1 );
		mms->msg_size = FS_FileGetSize( FS_DIR_MMS, file );
		if( head.date > 0 )
			FS_SecondsToDateTime( &mms->date, head.date, IFS_GetTimeZone() );
		
		FS_MmsCodecFreeHead( &head );
		FS_MmsSaveHeadList( );

		FS_MmsListFormUpdateCurItem( mms );
	}
}

static void FS_MmsDisplayNetPrograss( FS_Window *win, FS_MmsNetState *state )
{
	FS_CHAR str[64];
	
	IFS_Sprintf( str, "%d-%s(%dK/%dK)", state->second, FS_Text(state->text_id), 
		FS_KB(state->offset), FS_KB(state->total) );
	FS_MsgBoxSetText( win, str );
}

static void FS_MmsNetCounter_CB( void *dummy )
{
	FS_MmsNetState *pMmsNetState = &GFS_MmsNetState;
	
	FS_Window *win = FS_WindowFindId( FS_W_MmsSendingDlg );
	if( win == FS_NULL )
		win = FS_WindowFindId( FS_W_MmsRecvingDlg );
	
	pMmsNetState->timer_id = 0;
	if( win )
	{
		pMmsNetState->second ++;
		FS_MmsDisplayNetPrograss( win, pMmsNetState );
		pMmsNetState->timer_id = IFS_StartTimer( FS_TIMER_ID_MMS_COUNTER, 1000, FS_MmsNetCounter_CB, FS_NULL );
	}
}

static void FS_MmsNetCallback( void *user_data, FS_SINT4 ev, FS_UINT4 param )
{
	FS_BOOL bSendMms = FS_TRUE;
	FS_Window *win = FS_WindowFindId( FS_W_MmsSendingDlg );
	FS_MmsNetState *pMmsNetState = &GFS_MmsNetState;
	
	if( win == FS_NULL )
	{
		win = FS_WindowFindId( FS_W_MmsRecvingDlg );
		bSendMms = FS_FALSE;
	}
	
	switch( ev )
	{
		case FS_MMS_NET_CONNECTED:
			if( bSendMms )
				pMmsNetState->text_id = FS_T_SENDING;
			else
				pMmsNetState->text_id = FS_T_RECVING;
			FS_MmsDisplayNetPrograss( win, pMmsNetState );
			break;
		case FS_MMS_NET_SENDING:
			pMmsNetState->offset = param;
			FS_MmsDisplayNetPrograss( win, pMmsNetState ); 
			break;
		case FS_MMS_NET_RECVING:
			pMmsNetState->offset = param;
			FS_MmsDisplayNetPrograss( win, pMmsNetState ); 
 			break;
		case FS_MMS_NET_OK:
			if( bSendMms )
			{
				/* send mms ok */
				FS_MmsSendSuccess( (FS_CHAR *)param );
				FS_MmsDisplayNetStatus( FS_MMS_NET_OK, 0 );
			}
			else
			{
				/* recv mms ok */
				FS_MmsRecvSuccess( (FS_CHAR *)param );
				FS_MmsDisplayNetStatus( FS_MMS_NET_OK, 0 );
			}
			break;
		default:
			if( ev < 0 )
			{
				/* send failed. move mms to sendbox */
				if( bSendMms )
				{
					FS_MmsSendFailed( );
				}
				FS_MmsDisplayNetStatus( ev, (FS_SINT4)param );
			}
			break;
	}
}

static FS_BOOL FS_MmsSendDlgProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;

	if( cmd == FS_WM_COMMAND && wparam == FS_EV_NO )
	{
		/* when window auto destroy, we may call this too. */
		if( GFS_MmsNetState.timer_id )
		{
			IFS_StopTimer( GFS_MmsNetState.timer_id );
			GFS_MmsNetState.timer_id = 0;
		}
		FS_MmsNetCancel( );
		ret = FS_TRUE;
	}
	return ret;
}

static FS_BOOL FS_MmsSendCnf_CB( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_CHAR absFile[FS_MAX_PATH_LEN];
	FS_BOOL ret = FS_FALSE;
	if( wparam == FS_EV_YES )	// exit without save account data
	{
		FS_Window *msgBox;
		FS_MmsHead *mms;
		FS_MmsNetState *pMmsNetState = &GFS_MmsNetState;
				
		mms = GFS_CurEditMmsHead;

		IFS_EnterBackLight( );
		
		msgBox = FS_MessageBox( FS_MS_INFO_CANCEL, FS_NULL, FS_MmsSendDlgProc, FS_FALSE ); 
		msgBox->id = FS_W_MmsSendingDlg;
		
		pMmsNetState->offset = 0;;
		pMmsNetState->total = FS_FileGetSize( FS_DIR_MMS, mms->file );
		pMmsNetState->text_id = FS_T_CONNECTING;
		pMmsNetState->second = 0;
		pMmsNetState->timer_id = IFS_StartTimer( FS_TIMER_ID_MMS_COUNTER, 1000, FS_MmsNetCounter_CB, FS_NULL );
	
		FS_MmsDisplayNetPrograss( msgBox, pMmsNetState );
		FS_GetAbsFileName( FS_DIR_MMS, mms->file, absFile );
		ret = FS_MmsSend( absFile, FS_MmsNetCallback, FS_NULL );

		if( ret != FS_MMS_NET_OK )
		{
			FS_MmsDisplayNetStatus( ret, 0 );
		}
		ret = FS_TRUE;
	}
	return ret;
}

static void FS_MmsSend_HD( void )
{
	FS_MmsHead *mms;
	FS_CHAR str[64];
	FS_SINT4 size;
	
	mms = FS_MmsEditFormSaveMms( FS_TRUE );
	size = FS_MIN( mms->msg_size, IFS_GetMaxMmsSize() );
	IFS_Sprintf( str, "%s\n%d(KB)", FS_Text(FS_T_MMS_SEND_CNF), FS_KB(size) );
	FS_MessageBox( FS_MS_YES_NO, str, FS_MmsSendCnf_CB, FS_FALSE );
	FS_DestroyWindowByID( FS_W_ProgressFrm );
}

static void FS_MmsSend_CB( FS_Window *win )
{
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
	
	if( FS_MmsSendFormSaveData( ) )
	{
		FS_MessageBox( FS_MS_NONE, FS_Text(FS_T_PLS_WAITING), FS_NULL, FS_FALSE );
		IFS_PostMessage( FS_MSG_UTIL_CALL, (FS_UINT4)FS_MmsSend_HD );
	}
	else
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_INPUT_ADDR), FS_NULL, FS_FALSE );
	}
}

static void FS_MmsSendFormPBSelectHandler( FS_CHAR *phnums, void *param )
{
	FS_Window *win = FS_WindowFindId( FS_W_MmsSendFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( win );

	if( wgt && phnums )
	{
		FS_WidgetSetText( wgt, phnums );
	}
}

static void FS_MmsSendFormPhBook_CB( FS_Window *win )
{
	IFS_PBSelectPhoneNumber( FS_MmsSendFormPBSelectHandler, FS_NULL );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static FS_BOOL FS_MmsSendFormSaveData( void )
{
	FS_BOOL ret = FS_FALSE;
	FS_Window *swin = FS_WindowFindId( FS_W_MmsSendFrm );
	FS_CHAR *str;
	
	str = FS_WindowGetWidgetText( swin, FS_W_MmsSendFrmTo );
	FS_MmsEditFormSetToAddr( str );
	if( str && str[0] ) ret = FS_TRUE;
	str = FS_WindowGetWidgetText( swin, FS_W_MmsSendFrmCc );
	FS_MmsEditFormSetCcAddr( str );
	if( str && str[0] ) ret = FS_TRUE;
	str = FS_WindowGetWidgetText( swin, FS_W_MmsSendFrmBcc );
	FS_MmsEditFormSetBccAddr( str );
	str = FS_WindowGetWidgetText( swin, FS_W_MmsSendFrmSub );
	FS_MmsEditFormSaveSubject( str );
	FS_MmsEditFormContentChanged( );

	return ret;
}

static void FS_MmsSendFormSave_CB( FS_Window *win )
{
	FS_Window *swin = FS_WindowFindId( FS_W_MmsSendFrm );

	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );

	FS_MmsSendFormSaveData( );
	FS_MmsSave_CB( FS_NULL );
	FS_DestroyWindow( swin );
}

static void FS_MmsSendFormMenu_UI( FS_Window *win )
{
	FS_Widget *iPB = FS_NULL, *iSave, *iSend, *wgt;
	FS_Window *pMenu;
	FS_SINT4 i = 2;
	
	wgt = FS_WindowGetFocusItem( win );
	if( wgt->id != FS_W_MmsSendFrmSub )
	{
		i ++;
		iPB = FS_CreateMenuItem( 0, FS_Text(FS_T_PHONEBOOK) );
	}
	iSave = FS_CreateMenuItem( 0, FS_Text(FS_T_SAVE) );
	iSend = FS_CreateMenuItem( 0, FS_Text(FS_T_SEND) );
	
	pMenu = FS_CreateMenu( 0, i );
	FS_MenuAddItem( pMenu, iSend );
	if( iPB ) FS_MenuAddItem( pMenu, iPB );
	FS_MenuAddItem( pMenu, iSave );

	if( iPB ) FS_WidgetSetHandler( iPB, FS_MmsSendFormPhBook_CB );
	FS_WidgetSetHandler( iSave, FS_MmsSendFormSave_CB );
	FS_WidgetSetHandler( iSend, FS_MmsSend_CB );
	
	FS_MenuSetSoftkey( pMenu );
	
	FS_ShowWindow( pMenu );
}

static void FS_MmsSendFormBack_CB( FS_Window *swin )
{
	FS_CHAR *addr;
	FS_Window *win = FS_WindowFindId( FS_W_MmsEditFrm );
	FS_Widget *wgt;
	
	addr = FS_WindowGetWidgetText( swin, FS_W_MmsSendFrmTo );
	FS_MmsEditFormSetToAddr( addr );
	addr = FS_WindowGetWidgetText( swin, FS_W_MmsSendFrmCc );
	FS_MmsEditFormSetCcAddr( addr );
	addr = FS_WindowGetWidgetText( swin, FS_W_MmsSendFrmBcc );
	FS_MmsEditFormSetBccAddr( addr );
	addr = FS_WindowGetWidgetText( swin, FS_W_MmsSendFrmSub );
	FS_MmsEditFormSaveSubject( addr );
	wgt = FS_WindowGetWidget( win, FS_W_MmsEditSubject );
	FS_WidgetSetText( wgt, addr );
	FS_DestroyWindow( swin );
}

static void FS_MmsSendForm_UI( FS_Window *win )
{
	FS_Window *swin;
	FS_Widget *wTip, *wTo, *wCc, *wBcc, *wSub;
	FS_CHAR *addr = IFS_Malloc( FS_MAX_MMS_ADDR_LEN );
	FS_EditParam eParam = { FS_IM_123, FS_IM_123 | FS_IM_ABC, FS_DEFAULT_EDIT_LEN };
	
	if( addr == FS_NULL )
		return;
	
	swin = FS_CreateWindow( FS_W_MmsSendFrm, FS_Text(FS_T_SEND), FS_NULL );
	wTip = FS_CreateLabel( 0, FS_Text(FS_T_MMS_SEND_FORM_TIP), FS_I_INFO, 1 );
	IFS_Memset( addr, 0, FS_MAX_MMS_ADDR_LEN );
	FS_MmsFileGetToAddr( addr, FS_MAX_MMS_ADDR_LEN, FS_MmsEditFormGetMmsFile() );
	wTo = FS_CreateEditBox( FS_W_MmsSendFrmTo, addr, FS_I_TO, 1, &eParam );
	IFS_Memset( addr, 0, FS_MAX_MMS_ADDR_LEN );
	FS_MmsFileGetCcAddr( addr, FS_MAX_MMS_ADDR_LEN, FS_MmsEditFormGetMmsFile() );
	wCc = FS_CreateEditBox( FS_W_MmsSendFrmCc, addr, FS_I_CC, 1, &eParam );
	IFS_Memset( addr, 0, FS_MAX_MMS_ADDR_LEN );
	FS_MmsFileGetBccAddr( addr, FS_MAX_MMS_ADDR_LEN, FS_MmsEditFormGetMmsFile() );
	wBcc = FS_CreateEditBox( FS_W_MmsSendFrmBcc, addr, FS_I_BCC, 1, &eParam );

	eParam.preferred_method = FS_IM_CHI;
	eParam.allow_method = FS_IM_ALL;
	/* 
	 * !!! 
	 * ugly cmcc mmsc or cmcc gatway. 
	 * if mms subject too long(larger than 40 ch, 80 bytes). it will return an error of 0x40(Bad Request). 
	*/
	eParam.max_len = FS_MMS_SUB_LEN - 10;	
	eParam.limit_character = FS_TRUE;
	wSub = FS_CreateEditBox( FS_W_MmsSendFrmSub, FS_MmsEditFormGetSubject(), FS_I_SUBJECT, 1, &eParam );

	IFS_Free( addr );
	wTo->tip = FS_Text(FS_T_TO);
	wCc->tip = FS_Text(FS_T_CC);
	wBcc->tip = FS_Text(FS_T_BCC);
	wSub->tip = FS_Text(FS_T_SUBJECT);

	FS_WindowAddWidget( swin, wTip );
	FS_WindowAddWidget( swin, wTo );
	FS_WindowAddWidget( swin, wCc );
	FS_WindowAddWidget( swin, wBcc );
	FS_WindowAddWidget( swin, wSub );
	
	FS_WindowSetSoftkey( swin, 1, FS_Text(FS_T_MENU), FS_MmsSendFormMenu_UI );
	FS_WindowSetSoftkey( swin, 3, FS_Text(FS_T_BACK), FS_MmsSendFormBack_CB );
	
	FS_ShowWindow( swin );	
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_MmsPreviewSend_CB( FS_Window *win )
{
	FS_Window *vwin = FS_WindowFindId( FS_W_MmsViewFrm );
	FS_DestroyWindow( vwin );
	FS_MmsSendForm_UI( win );
}

static void FS_MmsReplySms_CB( FS_Window *win )
{
	FS_CHAR str[64];
	FS_MmsFile *pSrcMms;

	IFS_Memset( str, 0, sizeof(str) );
	pSrcMms = FS_MmsViewFormGetMmsFile( );
	FS_MmsFileGetFromAddr( str, sizeof(str) - 1, pSrcMms );

	IFS_WriteNewSms( FS_NULL, str );
	
	FS_DestroyWindowByID( FS_W_MmsViewMenu );
	FS_DestroyWindowByID( FS_W_MmsViewFrm );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_MmsReplyMms_CB( FS_Window *win )
{ 
	FS_CHAR str[64], *subject;
	FS_MmsFile *pSrcMms, *pDstMms;
	
	if( FS_MmsIsFull( 1024 ) )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_SPACE_IS_FULL), FS_NULL, FS_FALSE );
		FS_DestroyWindowByID( FS_W_MmsViewMenu );
		if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
			FS_DestroyWindow( win );
		return;
	}

	pSrcMms = FS_MmsViewFormGetMmsFile( );
	pDstMms = FS_CreateMmsFile( );
	if( pDstMms )
	{
		FS_MmsFileAddFrame( pDstMms, 0 );
		
		IFS_Memset( str, 0, sizeof(str) );
		FS_MmsFileGetFromAddr( str, sizeof(str) - 1, pSrcMms );
		FS_MmsFileSetToAddr( pDstMms, str );
		
		subject = FS_MmsFileGetSubject( pSrcMms );
		subject = FS_StrConCat( "Re:", subject, FS_NULL, FS_NULL );
		FS_MmsFileSetSubject( pDstMms, subject );
		IFS_Free( subject );
		
		FS_MmsEdit_UI( pDstMms );
	}
	else
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_UNKNOW_ERR), FS_NULL, FS_TRUE );
	}

	FS_DestroyWindowByID( FS_W_MmsViewMenu );
	FS_DestroyWindowByID( FS_W_MmsViewFrm );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_MmsReplyMenu_UI( FS_Window *win )
{ 
	FS_Widget *iReSms, *iReMms, *wgt;
	FS_Window *pMenu, *mwin;
	FS_Rect rect = { 0 };
	
	rect.top = IFS_GetScreenHeight( ) / 2;
	rect.width = IFS_GetScreenWidth( );
	rect.height = IFS_GetLineHeight( );
	
	mwin = FS_WindowFindId( FS_W_MmsViewMenu );
	if( mwin )
	{
		wgt = FS_WindowGetFocusItem( mwin );
		rect = FS_GetWidgetDrawRect( wgt );
	}
	
	pMenu = FS_CreatePopUpMenu( 0, &rect, 2 );
	iReSms = FS_CreateMenuItem( 0,  FS_Text(FS_T_RE_SMS) );
	iReMms = FS_CreateMenuItem( 0,  FS_Text(FS_T_RE_MMS) );
	
	FS_MenuAddItem( pMenu, iReSms );
	FS_MenuAddItem( pMenu, iReMms );
	
	FS_WidgetSetHandler( iReSms, FS_MmsReplySms_CB );
	FS_WidgetSetHandler( iReMms, FS_MmsReplyMms_CB );
	
	FS_MenuSetSoftkey( pMenu );
	FS_ShowWindow( pMenu );
}

static void FS_MmsForward_HD( void )
{
	FS_CHAR *str;
	FS_MmsFile *pMmsFile, *pSrcMms;
	
	if( FS_MmsIsFull( 1024 ) )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_SPACE_IS_FULL), FS_NULL, FS_FALSE );
		FS_DestroyWindowByID( FS_W_ProgressFrm );
		return;
	}

	pSrcMms = FS_MmsViewFormGetMmsFile( );
	pMmsFile = FS_MmsFileDuplicate( pSrcMms );
	if( pMmsFile )
	{
		str = FS_MmsFileGetSubject( pSrcMms );
		str = FS_StrConCat( "Fw:", str, FS_NULL, FS_NULL );
		FS_MmsFileSetSubject( pMmsFile, str );
		IFS_Free( str );
		
		FS_MmsEdit_UI( pMmsFile );
	}
	else
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_UNKNOW_ERR), FS_NULL, FS_TRUE );
	}
	
	FS_DestroyWindowByID( FS_W_MmsViewFrm );
	FS_DestroyWindowByID( FS_W_ProgressFrm );
}

static void FS_MmsForward_CB( FS_Window *win )
{
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
	
	FS_MessageBox( FS_MS_NONE, FS_Text(FS_T_PLS_WAITING), FS_NULL, FS_FALSE );
	IFS_PostMessage( FS_MSG_UTIL_CALL, (FS_UINT4)FS_MmsForward_HD );	
}

static void FS_MmsSaveContentHandle( FS_CHAR * path, void *param )
{
	FS_Window *win = FS_WindowFindId( FS_W_MmsViewContentListFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( win );
	FS_BOOL ret;
	FS_CHAR *str;
	FS_SINT4 len;
	
	if( path && path[0] && wgt && wgt->data )
	{
		len = IFS_Strlen( path );
		ret = IFS_FileCopy( wgt->data, path );
		if( ret )
		{
			str = IFS_Malloc( len + 32 );
			IFS_Sprintf( str, "%s %s", FS_Text(FS_T_SAVED_TO), path );
			FS_MessageBox( FS_MS_OK, str, FS_NULL, FS_FALSE );
			IFS_Free( str );
		}
		else
		{
			str = FS_Text( FS_T_SAVE_FILE_FAILED );
			FS_MessageBox( FS_MS_ALERT, str, FS_NULL, FS_FALSE );
		}
	}
}

static void FS_MmsContentListSaveContent_CB( FS_Window *win )
{
	FS_Widget *wgt = FS_WindowGetFocusItem( win );
	if( wgt->data )
	{
		IFS_FileDialogSave( wgt->text, FS_MmsSaveContentHandle, FS_NULL );
	}
}

static void FS_MmsViewSaveContent_CB( FS_Window *win )
{
	FS_CHAR *image, *audio, *text, *video;
	FS_SINT4 num;

	num = FS_MmsViewFormGetCurFrameNum( );
	image = FS_MmsViewFormGetFrameImageName( num );
	audio = FS_MmsViewFormGetFrameAudioName( num );
	text = FS_MmsViewFormGetFrameTextName( num );
	video = FS_MmsViewFormGetFrameVideoName( num );
	
	if( image == FS_NULL && audio == FS_NULL && text == FS_NULL && video == FS_NULL )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NO_CONTENT),  FS_NULL, FS_TRUE );
	}
	else
	{
		FS_Window *lwin;
		FS_Widget *wgt;

		lwin = FS_CreateWindow( FS_W_MmsViewContentListFrm, FS_Text(FS_T_SAVE_CONTENT), FS_NULL );
		if( image )
		{
			wgt = FS_CreateListItem( 0, image, FS_NULL, FS_I_IMAGE, 1 );
			wgt->data = IFS_Strdup( FS_MmsViewFormGetFrameImage(num) );
			FS_WindowAddWidget( lwin, wgt );
		}
		if( audio )
		{
			wgt = FS_CreateListItem( 0, audio, FS_NULL, FS_I_AUDIO, 1 );
			wgt->data = IFS_Strdup( FS_MmsViewFormGetFrameAudio(num) );
			FS_WindowAddWidget( lwin, wgt );
		}
		if( video )
		{
			wgt = FS_CreateListItem( 0, video, FS_NULL, FS_I_VIDEO, 1 );
			wgt->data = IFS_Strdup( FS_MmsViewFormGetFrameVideo(num) );
			FS_WindowAddWidget( lwin, wgt );
		}
		if( text )
		{
			wgt = FS_CreateListItem( 0, text, FS_NULL, FS_I_FILE, 1 );
			wgt->data = IFS_Strdup( FS_MmsViewFormGetFrameText(num, FS_FALSE) );
			FS_WindowAddWidget( lwin, wgt );
		}
		
		FS_WindowSetSoftkey( lwin, 1, FS_Text(FS_T_SAVE), FS_MmsContentListSaveContent_CB );
		FS_WindowSetSoftkey( lwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
		FS_ShowWindow( lwin );
	}
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_MmsPreviewMenu_UI( FS_Window *win )
{
	FS_Widget *iNext, *iSend, *iRestart, *iTimer, *iPrev;
	FS_Window *pMenu;
	iNext = FS_CreateMenuItem( 0, FS_Text(FS_T_NEXT_FRAME) );
	iSend = FS_CreateMenuItem( 0, FS_Text(FS_T_SEND) );
	iRestart = FS_CreateMenuItem( 0, FS_Text(FS_T_REPLAY) );
	if( FS_MmsViewFormIsTimerEnable() )
	{
		iTimer = FS_CreateMenuItem( 0,	FS_Text(FS_T_MANUAL_PLAY) );
		FS_WidgetSetHandler( iTimer, FS_MmsViewDisableTimer_CB );
	}
	else
	{
		iTimer = FS_CreateMenuItem( 0,	FS_Text(FS_T_AUTO_PLAY) );
		FS_WidgetSetHandler( iTimer, FS_MmsViewEnableTimer_CB );
	}
	iPrev = FS_CreateMenuItem( 0, FS_Text(FS_T_PREV_FRAME) );
	
	pMenu = FS_CreateMenu( 0, 5 );
	FS_MenuAddItem( pMenu, iNext );
	FS_MenuAddItem( pMenu, iPrev );
	FS_MenuAddItem( pMenu, iSend );
	FS_MenuAddItem( pMenu, iRestart );
	FS_MenuAddItem( pMenu, iTimer );
	
	FS_WidgetSetHandler( iNext, FS_MmsViewNextFrame_CB );
	FS_WidgetSetHandler( iSend, FS_MmsPreviewSend_CB );
	FS_WidgetSetHandler( iRestart, FS_MmsViewRestart_CB );
	FS_WidgetSetHandler( iPrev, FS_MmsViewPrevFrame_CB );
	
	FS_MenuSetSoftkey( pMenu );
	
	FS_ShowWindow( pMenu );
}

static void FS_MmsViewMenu_UI( FS_Window *win )
{
	FS_Widget *iNext, *iSaveContent, *iRestart, *iTimer, *iPrev, *iReply, *iForward;
	FS_Window *pMenu;
	
	if( FS_MmsViewFormIsPreview() )
	{
		FS_MmsPreviewMenu_UI( win );
		return;
	}
	
	iNext = FS_CreateMenuItem( 0,  FS_Text(FS_T_NEXT_FRAME) );
	iSaveContent = FS_CreateMenuItem( 0,  FS_Text(FS_T_SAVE_CONTENT) );
	iRestart = FS_CreateMenuItem( 0, FS_Text(FS_T_REPLAY) );
	if( FS_MmsViewFormIsTimerEnable() )
	{
		iTimer = FS_CreateMenuItem( 0,  FS_Text(FS_T_MANUAL_PLAY) );
		FS_WidgetSetHandler( iTimer, FS_MmsViewDisableTimer_CB );
	}
	else
	{
		iTimer = FS_CreateMenuItem( 0,  FS_Text(FS_T_AUTO_PLAY) );
		FS_WidgetSetHandler( iTimer, FS_MmsViewEnableTimer_CB );
	}
	iPrev = FS_CreateMenuItem( 0,  FS_Text(FS_T_PREV_FRAME) );
	iReply = FS_CreateMenuItem( 0,  FS_Text(FS_T_REPLY) );
	iForward = FS_CreateMenuItem( 0,  FS_Text(FS_T_FORWARD) );
	
	pMenu = FS_CreateMenu( FS_W_MmsViewMenu, 7 );
	FS_MenuAddItem( pMenu, iNext );
	FS_MenuAddItem( pMenu, iPrev );
	FS_MenuAddItem( pMenu, iSaveContent );
	FS_MenuAddItem( pMenu, iRestart );
	FS_MenuAddItem( pMenu, iTimer );
	FS_MenuAddItem( pMenu, iReply );
	FS_WGT_SET_SUB_MENU_FLAG( iReply );
	FS_MenuAddItem( pMenu, iForward );
	
	FS_WidgetSetHandler( iNext, FS_MmsViewNextFrame_CB );
	FS_WidgetSetHandler( iSaveContent, FS_MmsViewSaveContent_CB );
	FS_WidgetSetHandler( iRestart, FS_MmsViewRestart_CB );
	FS_WidgetSetHandler( iPrev, FS_MmsViewPrevFrame_CB );
	FS_WidgetSetHandler( iReply, FS_MmsReplyMenu_UI );
	FS_WidgetSetHandler( iForward, FS_MmsForward_CB );
	
	FS_MenuSetSoftkey( pMenu );
	
	FS_ShowWindow( pMenu );
}

static FS_BOOL FS_MmsViewWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	
	if( cmd == FS_WM_DESTROY )
	{
		FS_MmsViewFormStopAudio( );
		FS_MmsViewFormStopTimer( );
		
		if( ! FS_MmsViewFormIsPreview() )
		{
			/* not preview. we must destroy mms file */ 
			FS_MmsFile *pMmsFile;
			
			pMmsFile = FS_MmsViewFormGetMmsFile( );
			FS_DestroyMmsFile( pMmsFile );
		}
		ret = FS_TRUE;
	}
	else if( cmd == FS_WM_LOSTFOCUS )
	{
		FS_MmsViewFormStopAudio( );
		FS_MmsViewFormStopTimer( );
		ret = FS_TRUE;
	}
	else if( cmd == FS_WM_SETFOCUS )
	{
		/* audio */
		FS_MmsViewFormPlayAudioDelay( );
		/* start timer */
		FS_MmsViewFormStartTimer( );
		ret = FS_TRUE;
	}
	return ret;
}

static FS_MmsView_UI( FS_MmsFile *pMmsFile, FS_BOOL preview )
{
	FS_Window *vwin;
	
	vwin = FS_WebCreateWin( FS_W_MmsViewFrm, FS_Text(FS_T_MMS_VIEW), FS_MmsViewWndProc );
	vwin->show_index = FS_TRUE;
	FS_MmsViewFormSaveData( vwin, pMmsFile, preview );

	FS_WindowSetSoftkey( vwin, 1, FS_Text(FS_T_MENU), FS_MmsViewMenu_UI );
	FS_WindowSetSoftkey( vwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	FS_ShowWindow( vwin );
	
	FS_MmsViewFormShowFrame( 1 );
}

static void FS_MmsEditPreview_CB( FS_Window *win )
{
	FS_MmsView_UI( FS_MmsEditFormGetMmsFile(), FS_TRUE );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

/* return file name of the saved mms file */
static FS_MmsHead * FS_MmsEditFormSaveMms( FS_BOOL is_send )
{
	FS_MmsFile *pMmsFile;
	FS_MmsHead *mms;
	FS_CHAR *p;
	FS_UINT1 mbox;
	
	pMmsFile = FS_MmsEditFormGetMmsFile( );
	mms = FS_MmsListFormGetHead( );
	if( mms == FS_NULL ) mms = GFS_CurEditMmsHead;
	
	if( mms == FS_NULL || mms->mbox == FS_MMS_OUTBOX || mms->mbox == FS_MMS_INBOX
		|| (is_send && mms->mbox == FS_MMS_TEMPLATE ) )
	{
		/* save a new mms file */
		mms = FS_NEW( FS_MmsHead );
		if( mms )
		{
			IFS_Memset( mms, 0, sizeof(FS_MmsHead) );
			FS_GetGuid( mms->file );
			IFS_Strcat( mms->file, ".mms" );
			mms->mbox = FS_MMS_DRAFT;
			FS_MmsAddHead( mms );
		}
	}
	
	if( mms )
	{
		FS_MmsEncodeFile( mms->file, pMmsFile );
		/* subject */
		if( pMmsFile->data.head.subject )
		{
			IFS_Strncpy( mms->subject, pMmsFile->data.head.subject, FS_MMS_SUB_LEN - 1 );
			mms->subject[FS_MMS_SUB_LEN - 1] = 0;
		}
		/* to */
		if( pMmsFile->data.head.to )
		{
			IFS_Strncpy( mms->address, pMmsFile->data.head.to, FS_MMS_ADDR_LEN - 1 );
			mms->address[FS_MMS_ADDR_LEN - 1] = 0;
			p = IFS_Strchr( mms->address, ',' );	/* just save one phone number */
			if( p ) *p = 0;
		}
		/* msg size */
		mms->msg_size = FS_FileGetSize( FS_DIR_MMS, mms->file );
		/* date */
		IFS_GetDateTime( &mms->date );
	
		/* save to disk */
		FS_MmsSaveHeadList( );
	}

	mbox = FS_MmsListFormGetMBox( );
	if( mbox == FS_MMS_DRAFT || mbox == FS_MMS_SENDBOX || (!is_send && mbox == FS_MMS_TEMPLATE) )
		FS_MmsListFormUpdateCurItem( mms );
	GFS_CurEditMmsHead = mms;
	GFS_CurEditMmsSaved = FS_TRUE;
	return mms;
}

static void FS_MmsSave_HD( void )
{
	FS_Window *win;
	
	FS_MmsEditFormSaveMms( FS_FALSE );	
	FS_MessageBox( FS_MS_OK, FS_Text(FS_T_SAVED), FS_NULL, FS_TRUE );

	win = FS_WindowFindId( FS_W_MmsEditFrm );
	if( win ) FS_DestroyWindow( win );
	FS_DestroyWindowByID( FS_W_ProgressFrm );
}

static void FS_MmsSave_CB( FS_Window *win )
{
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
	
	FS_MessageBox( FS_MS_NONE, FS_Text(FS_T_PLS_WAITING), FS_NULL, FS_FALSE );
	IFS_PostMessage( FS_MSG_UTIL_CALL, (FS_UINT4)FS_MmsSave_HD );
}

static void FS_MmsEditAddFrame_UI( FS_Window *win )
{
	FS_SINT4 num;
	FS_MmsFile *pMmsFile = FS_MmsEditFormGetMmsFile( );
	
	num = FS_MmsEditFormGetFrameCount( );
	if( num >= IFS_GetMaxMmsFrames() )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_MMS_FRAMES_LIMITED), FS_NULL, FS_TRUE );
	}
	else
	{
		num = FS_MmsEditFormGetCurFrameNum( );
		FS_MmsFileAddFrame( pMmsFile, num );
		FS_MmsEditFormSetCurFrame( num + 1 );
	}
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_MmsEditSaveLayout_CB( FS_Window *win )
{
	FS_Widget *wgt;
	FS_BOOL bImgFirst = FS_FALSE;
	FS_MmsFile *pMmsFile;
	
	wgt = FS_WindowGetWidget( win, FS_W_MmsEditLayoutImageUp );
	if( FS_WGT_GET_CHECK(wgt) )
		bImgFirst = FS_TRUE;
	
	pMmsFile = FS_MmsEditFormGetMmsFile( );
	FS_SmilLayoutSetImageFirst( pMmsFile->smil, bImgFirst );
	FS_MmsEditFormContentChanged( );
	
	FS_DestroyWindow( win );
}

static void FS_MmsEditSetLayout_UI( FS_Window *win )
{
	FS_Window *lwin;
	FS_Widget *wImageUp, *wImageDn;
	FS_MmsFile *pMmsFile = FS_MmsEditFormGetMmsFile( );
	
	lwin = FS_CreateWindow( FS_W_MmsEditLayoutFrm, FS_Text(FS_T_SET_LAYOUT), FS_NULL );
	wImageUp = FS_CreateRadioBox( FS_W_MmsEditLayoutImageUp, FS_Text(FS_T_LAYOUT_IMAGE_UP) );
	wImageDn = FS_CreateRadioBox( FS_W_MmsEditLayoutImageDn, FS_Text(FS_T_LAYOUT_IMAGE_DN) );

	FS_WindowAddWidget( lwin, wImageUp );
	FS_WindowAddWidget( lwin, wImageDn );
	
	if( FS_SmilLayoutImageFirst(pMmsFile->smil) )
		FS_WidgetSetCheck( wImageUp, FS_TRUE );
	else
		FS_WidgetSetCheck( wImageDn, FS_TRUE );
	
	FS_WindowSetSoftkey( lwin, 1, FS_Text(FS_T_SAVE), FS_MmsEditSaveLayout_CB );
	FS_WindowSetSoftkey( lwin, 3, FS_Text(FS_T_CANCEL), FS_StandardKey3Handler );

	FS_ShowWindow( lwin );
	FS_DestroyWindowByID( FS_W_MmsEditMenu );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_MmsEditSaveDuration_CB( FS_Window *win )
{
	FS_Widget *wDur;
	FS_SINT4 dur = 0;
	FS_MmsFile *pMmsFile;
	
	wDur = FS_WindowGetWidget( win, FS_W_MmsEditDur );
	if( wDur->text )
		dur = IFS_Atoi( wDur->text );
	
	pMmsFile = FS_MmsEditFormGetMmsFile( );
	FS_SmilSetFrameDur( pMmsFile->smil, FS_MmsEditFormGetCurFrameNum(), dur );
	FS_MmsEditFormContentChanged( );
	
	FS_DestroyWindow( win );
}

static void FS_MmsEditSetDuration_UI( FS_Window *win )
{
	FS_Window *dwin;
	FS_Widget *wDur, *wDesc;
	FS_CHAR txt[16];
	FS_SINT4 dur;
	FS_EditParam eParam = { FS_IM_123, FS_IM_123, 12 };
	FS_MmsFile *pMmsFile = FS_MmsEditFormGetMmsFile( );
	
	dur = FS_SmilGetFrameDur( pMmsFile->smil, FS_MmsEditFormGetCurFrameNum() );
	IFS_Itoa( dur, txt, 10 );
	dwin = FS_CreateWindow( FS_W_MmsEditDurFrm, FS_Text(FS_T_SET_FRAME_DUR), FS_NULL );
	wDur = FS_CreateEditBox( FS_W_MmsEditDur, txt, FS_I_EDIT, 1, &eParam );
	wDesc = FS_CreateLabel( FS_W_MmsEditDurDesc, FS_Text(FS_T_MMS_DUR_DESC), FS_I_INFO, 0 );
	FS_WindowAddWidget( dwin, wDur );
	FS_WindowAddWidget( dwin, wDesc );

	FS_WindowSetSoftkey( dwin, 1, FS_Text(FS_T_SAVE), FS_MmsEditSaveDuration_CB );
	FS_WindowSetSoftkey( dwin, 3, FS_Text(FS_T_CANCEL), FS_StandardKey3Handler );
	FS_ShowWindow( dwin );

	FS_DestroyWindowByID( FS_W_MmsEditMenu );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_MmsAddImageHandle( FS_CHAR * path, void *param )
{
	FS_SINT4 size, mmsSize;
	FS_MmsFile *pMmsFile;
	FS_CHAR *content_id, *file, *video_cid;

	if( path == FS_NULL ) return;
	
	size = FS_FileGetSize( -1, path );
	if( size <= 0 )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_FILE_NOT_EXIST), FS_NULL, FS_FALSE );
		return;
	}
	
	pMmsFile = FS_MmsEditFormGetMmsFile( );
	mmsSize = FS_MmsFileGetObjectTotalSize( pMmsFile );	
	content_id = FS_MmsEditFormGetCurFrameImage( );
	if( content_id )
	{
		file = FS_MmsFileGetEntryFileByCid( pMmsFile, content_id );
		mmsSize -= FS_FileGetSize( -1, file );
	}
	video_cid = FS_MmsEditFormGetCurFrameVideo( );
	if( video_cid )
	{
		file = FS_MmsFileGetEntryFileByCid( pMmsFile, video_cid );
		mmsSize -= FS_FileGetSize( -1, file );
	}
	if( mmsSize + size >= IFS_GetMaxMmsSize() )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_FILE_TOO_LARGE), FS_NULL, FS_FALSE );
		return;
	}
	if( FS_MmsIsFull( mmsSize + size ) )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_SPACE_IS_FULL), FS_NULL, FS_FALSE );
		return;
	}

	if( video_cid )
	{
		FS_MmsEditFormDelCurFrameVideo( );
	}
	
	if( content_id )
	{
		FS_MmsCodecUpdateEntry( &pMmsFile->data, content_id, path, size );
	}
	else
	{
		content_id = FS_MmsCodecCreateEntry( &pMmsFile->data, path, size );
		FS_SmilAddFrameImage( pMmsFile->smil, FS_MmsEditFormGetCurFrameNum(), content_id );
	}
	FS_MmsEditFormShowImage( path );
	FS_MmsEditFormContentChanged( );
	FS_MmsEditFormUpdateMmsDetailText( );
}

static void FS_MmsAddVideoHandle( FS_CHAR * path, void *param )
{
	FS_SINT4 size, mmsSize;
	FS_MmsFile *pMmsFile;
	FS_CHAR *content_id, *file, *image_cid, *audio_cid;

	if( path == FS_NULL ) return;
	
	size = FS_FileGetSize( -1, path );
	if( size <= 0 )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_FILE_NOT_EXIST), FS_NULL, FS_FALSE );
		return;
	}
	
	pMmsFile = FS_MmsEditFormGetMmsFile( );
	mmsSize = FS_MmsFileGetObjectTotalSize( pMmsFile );	
	content_id = FS_MmsEditFormGetCurFrameVideo( );
	if( content_id )
	{
		file = FS_MmsFileGetEntryFileByCid( pMmsFile, content_id );
		mmsSize -= FS_FileGetSize( -1, file );
	}
	image_cid = FS_MmsEditFormGetCurFrameImage( );
	if( image_cid )
	{
		file = FS_MmsFileGetEntryFileByCid( pMmsFile, image_cid );
		mmsSize -= FS_FileGetSize( -1, file );
	}
	audio_cid = FS_MmsEditFormGetCurFrameAudio( );
	if( audio_cid )
	{
		file = FS_MmsFileGetEntryFileByCid( pMmsFile, audio_cid );
		mmsSize -= FS_FileGetSize( -1, file );
	}
	
	if( mmsSize + size >= IFS_GetMaxMmsSize() )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_FILE_TOO_LARGE), FS_NULL, FS_FALSE );
		return;
	}
	if( FS_MmsIsFull( mmsSize + size ) )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_SPACE_IS_FULL), FS_NULL, FS_FALSE );
		return;
	}

	if( image_cid )
	{
		FS_MmsEditFormDelCurFrameImage( );
	}
	if( audio_cid )
	{
		FS_MmsEditFormDelCurFrameAudio( );
	}
	
	if( content_id )
	{
		FS_MmsCodecUpdateEntry( &pMmsFile->data, content_id, path, size );
	}
	else
	{
		content_id = FS_MmsCodecCreateEntry( &pMmsFile->data, path, size );
		FS_SmilAddFrameVideo( pMmsFile->smil, FS_MmsEditFormGetCurFrameNum(), content_id );
	}
	FS_MmsEditFormShowVideo( path );
	FS_MmsEditFormContentChanged( );
	FS_MmsEditFormUpdateMmsDetailText( );
}

static void FS_MmsAddAudioHandle( FS_CHAR * path, void *param )
{
	FS_SINT4 size, mmsSize;
	FS_MmsFile *pMmsFile;
	FS_CHAR *content_id, *file, *video_cid;
	
	if( path == FS_NULL ) return;
	
	size = FS_FileGetSize( -1, path );
	if( size <= 0 )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_FILE_NOT_EXIST), FS_NULL, FS_FALSE );
		return;
	}
	
	pMmsFile = FS_MmsEditFormGetMmsFile( );
	mmsSize = FS_MmsFileGetObjectTotalSize( pMmsFile );
	content_id = FS_MmsEditFormGetCurFrameAudio( );
	if( content_id )
	{
		file = FS_MmsFileGetEntryFileByCid( pMmsFile, content_id );
		mmsSize -= FS_FileGetSize( -1, file );
	}
	video_cid = FS_MmsEditFormGetCurFrameVideo( );
	if( video_cid )
	{
		file = FS_MmsFileGetEntryFileByCid( pMmsFile, video_cid );
		mmsSize -= FS_FileGetSize( -1, file );
	}
	if( mmsSize + size >= IFS_GetMaxMmsSize() )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_FILE_TOO_LARGE), FS_NULL, FS_FALSE );
		return;
	}
	if( FS_MmsIsFull( mmsSize + size ) )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_SPACE_IS_FULL), FS_NULL, FS_FALSE );
		return;
	}
	
	if( video_cid )
	{
		FS_MmsEditFormDelCurFrameVideo( );
	}
	
	if( content_id )
	{
		FS_MmsCodecUpdateEntry( &pMmsFile->data, content_id, path, size );
	}
	else
	{
		content_id = FS_MmsCodecCreateEntry( &pMmsFile->data, path, size );
		FS_SmilAddFrameAudio( pMmsFile->smil, FS_MmsEditFormGetCurFrameNum(), content_id );
	}
	FS_MmsEditFormShowAudio( path );
	FS_MmsEditFormContentChanged( );
	FS_MmsEditFormUpdateMmsDetailText( );
}

static void FS_MmsEditFormAddText_CB( FS_CHAR *text )
{
	FS_MmsFile *pMmsFile;
	FS_CHAR *content_id;
	FS_SINT4 size;
	FS_CHAR filename[FS_FILE_NAME_LEN];
	
	if( text && text[0] )
	{
		size = IFS_Strlen( text );
		
		pMmsFile = FS_MmsEditFormGetMmsFile( );
		FS_GetLuid( filename );
		IFS_Strcat( filename, ".txt" );
		FS_FileWrite( FS_DIR_TMP, filename, 0, text, size );

		content_id = FS_MmsEditFormGetCurFrameText( );
		if( content_id )
		{
			FS_MmsCodecUpdateEntry( &pMmsFile->data, content_id, filename, size );
		}
		else
		{
			content_id = FS_MmsCodecCreateEntry( &pMmsFile->data, filename, size );
			FS_SmilAddFrameText( pMmsFile->smil, FS_MmsEditFormGetCurFrameNum(), content_id );
		}
	}
	else
	{
		FS_MmsEditFormDelCurFrameText( );
	}	
	FS_MmsEditFormContentChanged( );
	FS_MmsEditFormUpdateMmsDetailText( );
}

static void FS_MmsEditFormAddImage_CB( FS_Window *win )
{
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );

	IFS_FileDialogOpen( FS_FDO_IMAGE, FS_MmsAddImageHandle, FS_NULL );
}

static void FS_MmsEditFormAddVideo_CB( FS_Window *win )
{
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );

	IFS_FileDialogOpen( FS_FDO_VIDEO, FS_MmsAddVideoHandle, FS_NULL );
}

static void FS_MmsEditFormAddAudio_CB( FS_Window *win )
{
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
	
	IFS_FileDialogOpen( FS_FDO_AUDIO, FS_MmsAddAudioHandle, FS_NULL );
}

static void FS_MmsEditFormDelImage_CB( FS_Window *win )
{
	FS_CHAR *cid;

	cid = FS_MmsEditFormGetCurFrameImage( );
	if( cid )
	{
		FS_MmsEditFormDelCurFrameImage( );
	}
	FS_MmsEditFormContentChanged( );
	FS_MmsEditFormUpdateMmsDetailText( );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );	
}

static void FS_MmsEditFormDelVideo_CB( FS_Window *win )
{
	FS_CHAR *cid;

	cid = FS_MmsEditFormGetCurFrameVideo( );
	if( cid )
	{
		FS_MmsEditFormDelCurFrameVideo( );
	}
	FS_MmsEditFormContentChanged( );
	FS_MmsEditFormUpdateMmsDetailText( );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );	
}

static void FS_MmsEditFormDelAudio_CB( FS_Window *win )
{
	FS_CHAR *cid;

	cid = FS_MmsEditFormGetCurFrameAudio( );
	if( cid )
	{
		FS_MmsEditFormDelCurFrameAudio( );
	}
	FS_MmsEditFormContentChanged( );
	FS_MmsEditFormUpdateMmsDetailText( );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );	
}

static void FS_MmsEditFormImageOption_CB( FS_Window *win )
{
	FS_Window *menu;
	FS_CHAR *cid;
	FS_Rect rect;
	FS_Widget *wgt, *wAdd, *wDel;
	
	cid = FS_MmsEditFormGetCurFrameImage( );
	if( cid == FS_NULL )
	{
		/* no image. inform to add one */
		FS_MmsEditFormAddImage_CB( win );
	}
	else
	{
		wgt = FS_WindowGetFocusItem( win );
		rect = FS_GetWidgetDrawRect( wgt );
		menu = FS_CreatePopUpMenu( 0, &rect, 2 );

		wAdd = FS_CreateMenuItem( 0, FS_Text(FS_T_ADD_IMAGE) );
		wDel = FS_CreateMenuItem( 0, FS_Text(FS_T_DEL_IMAGE) );
		FS_WidgetSetHandler( wAdd, FS_MmsEditFormAddImage_CB );
		FS_WidgetSetHandler( wDel, FS_MmsEditFormDelImage_CB );
		FS_MenuAddItem( menu, wAdd );
		FS_MenuAddItem( menu, wDel );
		
		FS_MenuSetSoftkey( menu );
		FS_ShowWindow( menu );
	}
}

static void FS_MmsEditFormVideoOption_CB( FS_Window *win )
{
	FS_Window *menu;
	FS_CHAR *cid;
	FS_Rect rect;
	FS_Widget *wgt, *wAdd, *wDel;
	
	cid = FS_MmsEditFormGetCurFrameVideo( );
	if( cid == FS_NULL )
	{
		/* no image. inform to add one */
		FS_MmsEditFormAddVideo_CB( win );
	}
	else
	{
		wgt = FS_WindowGetFocusItem( win );
		rect = FS_GetWidgetDrawRect( wgt );
		menu = FS_CreatePopUpMenu( 0, &rect, 2 );

		wAdd = FS_CreateMenuItem( 0, FS_Text(FS_T_ADD_VIDEO) );
		wDel = FS_CreateMenuItem( 0, FS_Text(FS_T_DEL_VIDEO) );
		FS_WidgetSetHandler( wAdd, FS_MmsEditFormAddVideo_CB );
		FS_WidgetSetHandler( wDel, FS_MmsEditFormDelVideo_CB );
		FS_MenuAddItem( menu, wAdd );
		FS_MenuAddItem( menu, wDel );
		
		FS_MenuSetSoftkey( menu );
		FS_ShowWindow( menu );
	}
}

static void FS_MmsEditFormAudioOption_CB( FS_Window *win )
{
	FS_Window *menu;
	FS_CHAR *cid;
	FS_Rect rect;
	FS_Widget *wgt, *wAdd, *wDel;
	
	cid = FS_MmsEditFormGetCurFrameAudio( );
	if( cid == FS_NULL )
	{
		/* no image. inform to add one */
		FS_MmsEditFormAddAudio_CB( win );
	}
	else
	{
		wgt = FS_WindowGetFocusItem( win );
		rect = FS_GetWidgetDrawRect( wgt );
		menu = FS_CreatePopUpMenu( 0, &rect, 2 );

		wAdd = FS_CreateMenuItem( 0, FS_Text(FS_T_ADD_AUDIO) );
		wDel = FS_CreateMenuItem( 0, FS_Text(FS_T_DEL_AUDIO) );
		FS_WidgetSetHandler( wAdd, FS_MmsEditFormAddAudio_CB );
		FS_WidgetSetHandler( wDel, FS_MmsEditFormDelAudio_CB );
		FS_MenuAddItem( menu, wAdd );
		FS_MenuAddItem( menu, wDel );
		
		FS_MenuSetSoftkey( menu );
		FS_ShowWindow( menu );
	}
}

static void FS_MmsEditSelectFrame_CB( FS_Window *win )
{
	FS_Widget *wgt = FS_WindowGetFocusItem( win );
	
	FS_MmsEditFormSetCurFrame( wgt->private_data );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );	
}

static void FS_MmsEditSelectFrame_UI( FS_Window *win )
{
	FS_Window *menu;
	FS_Widget *wgt;
	FS_Widget *focusWgt = FS_WindowGetFocusItem( win );
	FS_Rect rect = FS_GetWidgetDrawRect( focusWgt );
	FS_SINT4 i, total = FS_MmsEditFormGetFrameCount( );
	
	menu = FS_CreatePopUpMenu( FS_W_MmsEditFrameList, &rect, total + 1 );
	for( i = 1; i <= total; i ++ )
	{
		wgt = FS_CreateMenuItem( i, FS_MmsEditFormFormatFrameNumText(i) );
		wgt->private_data = i;
		FS_WidgetSetHandler( wgt, FS_MmsEditSelectFrame_CB );
		FS_MenuAddItem( menu, wgt );
	}
	wgt = FS_CreateMenuItem( FS_W_MmsEditAddFrame, FS_Text(FS_T_ADD_FRAME) );
	FS_WidgetSetHandler( wgt, FS_MmsEditAddFrame_UI );
	FS_MenuAddItem( menu, wgt );
	
	FS_MenuSetSoftkey( menu );

	FS_ShowWindow( menu );
}

static void FS_MmsEditDelFrame_CB( FS_Window *win )
{
	FS_SINT4 num = FS_MmsEditFormGetCurFrameNum( );
	FS_MmsEditFormContentChanged( );
	FS_MmsEditFormDelCurFrame( );
	if( num > 1 )
		FS_MmsEditFormSetCurFrame( num - 1 );
	else
		FS_MmsEditFormSetCurFrame( 1 );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_MmsEditDeliverReport_CB( FS_Window *win )
{
	FS_BOOL bDlvRpt = FS_MmsEditFormGetDlvReportFlag( );
	FS_MmsEditFormSetDlvReportFlag( (FS_BOOL)(! bDlvRpt) );

	FS_DestroyWindowByID( FS_W_MmsEditMenu );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
	FS_MessageBox( FS_MS_OK, FS_Text(FS_T_SUCCESS),  FS_NULL, FS_FALSE );
}

static void FS_MmsEditReadReport_CB( FS_Window *win )
{
	FS_BOOL bSet = FS_MmsEditFormGetReadReportFlag( );
	FS_MmsEditFormSetReadReportFlag( (FS_BOOL)(! bSet) );
	
	FS_DestroyWindowByID( FS_W_MmsEditMenu );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
	FS_MessageBox( FS_MS_OK, FS_Text(FS_T_SUCCESS), FS_NULL, FS_FALSE );
}

static void FS_MmsEditAdvanceMenu_UI( FS_Window *win )
{
	FS_Widget *iLayout, *iFrameDur, *iDlvReport, *iReadReport, *wgt;
	FS_Window *pMenu, *mwin;
	FS_Rect rect = { 0 };
	FS_BOOL bDlvReport = FS_MmsEditFormGetDlvReportFlag( );
	FS_BOOL bReadReport = FS_MmsEditFormGetReadReportFlag( );

	rect.top = IFS_GetScreenHeight( ) / 2;
	rect.width = IFS_GetScreenWidth( );
	rect.height = IFS_GetLineHeight( );
	
	mwin = FS_WindowFindId( FS_W_MmsEditMenu );
	if( mwin )
	{
		wgt = FS_WindowGetFocusItem( mwin );
		rect = FS_GetWidgetDrawRect( wgt );
	}
	
	iLayout = FS_CreateMenuItem( 0,  FS_Text(FS_T_SET_LAYOUT) );
	iFrameDur = FS_CreateMenuItem( 0,  FS_Text(FS_T_SET_FRAME_DUR) );
	
	if( bDlvReport )
		iDlvReport = FS_CreateMenuItem( 0, FS_Text(FS_T_CLR_DLV_REPORT) );
	else
		iDlvReport = FS_CreateMenuItem( 0, FS_Text(FS_T_REQ_DLV_REPORT) );
	
	if( bReadReport )
		iReadReport = FS_CreateMenuItem( 0, FS_Text(FS_T_CLR_READ_REPORT) );
	else
		iReadReport = FS_CreateMenuItem( 0, FS_Text(FS_T_REQ_READ_REPORT) );
	
	pMenu = FS_CreatePopUpMenu( 0, &rect, 4 );
	
	FS_MenuAddItem( pMenu, iLayout );
	FS_MenuAddItem( pMenu, iFrameDur );
	FS_MenuAddItem( pMenu, iDlvReport );
	FS_MenuAddItem( pMenu, iReadReport );
	
	FS_WidgetSetHandler( iLayout, FS_MmsEditSetLayout_UI );
	FS_WidgetSetHandler( iFrameDur, FS_MmsEditSetDuration_UI );
	FS_WidgetSetHandler( iDlvReport, FS_MmsEditDeliverReport_CB );
	FS_WidgetSetHandler( iReadReport, FS_MmsEditReadReport_CB );
	
	FS_MenuSetSoftkey( pMenu );
	FS_ShowWindow( pMenu );
}

static void FS_MmsEditMenu_UI( FS_Window *win )
{
	FS_Widget *iView, *iSend, *iSave, *iAdvEdit, *iDelFrame, *iAddFrame;
	FS_Window *pMenu;
	iView = FS_CreateMenuItem( 0,  FS_Text(FS_T_MMS_PREVIEW) );
	iSend = FS_CreateMenuItem( 0,  FS_Text(FS_T_SEND) );
	iSave = FS_CreateMenuItem( 0,	FS_Text(FS_T_SAVE) );
	iAdvEdit = FS_CreateMenuItem( 0,  FS_Text(FS_T_ADV_EDIT) );
	iAddFrame = FS_CreateMenuItem( 0,  FS_Text(FS_T_ADD_FRAME) );
	iDelFrame = FS_CreateMenuItem( 0,  FS_Text(FS_T_DEL_FRAME) );
	
	pMenu = FS_CreateMenu( FS_W_MmsEditMenu, 6 );
	FS_MenuAddItem( pMenu, iSend );
	FS_MenuAddItem( pMenu, iView );
	FS_MenuAddItem( pMenu, iSave );
	FS_MenuAddItem( pMenu, iAdvEdit );
	FS_WGT_SET_SUB_MENU_FLAG( iAdvEdit );
	FS_MenuAddItem( pMenu, iAddFrame );
	FS_MenuAddItem( pMenu, iDelFrame );
	
	FS_WidgetSetHandler( iView, FS_MmsEditPreview_CB );
	FS_WidgetSetHandler( iSend, FS_MmsSendForm_UI );
	FS_WidgetSetHandler( iSave, FS_MmsSave_CB );
	FS_WidgetSetHandler( iAdvEdit, FS_MmsEditAdvanceMenu_UI );
	FS_WidgetSetHandler( iAddFrame, FS_MmsEditAddFrame_UI );
	FS_WidgetSetHandler( iDelFrame, FS_MmsEditDelFrame_CB );
	
	FS_MenuSetSoftkey( pMenu );
	
	FS_ShowWindow( pMenu );
}

static FS_BOOL FS_MmsEditExitCnf_CB( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	if( wparam == FS_EV_YES )	// exit without save account data
	{
		FS_Window *editWin;
		editWin = FS_WindowFindId( FS_W_MmsEditFrm );
		FS_DestroyWindow( editWin );
	}
	return ret;
}

static void FS_MmsEditExit_CB( FS_Window *win )
{
	if( ! GFS_CurEditMmsSaved )
	{
		FS_MessageBox( FS_MS_YES_NO, FS_Text(FS_T_EXIT_WITHOUT_SAVE), FS_MmsEditExitCnf_CB, FS_FALSE );
	}
	else
	{
		FS_Window *editWin;
		editWin = FS_WindowFindId( FS_W_MmsEditFrm );
		FS_DestroyWindow( editWin );
	}
}

static FS_BOOL FS_MmsEditWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	FS_Widget *wgt;
	FS_SINT4 num;
	
	if( cmd == FS_WM_COMMAND )
	{
		if( wparam == FS_EV_ITEM_VALUE_CHANGE )
		{
			wgt = (FS_Widget *)lparam;
			if( wgt->id == FS_W_MmsEditText )
			{
				FS_MmsEditFormAddText_CB( wgt->text );
				ret = FS_TRUE;
			}
			else if( wgt->id == FS_W_MmsEditSubject )
			{
				FS_MmsEditFormContentChanged( );
				FS_MmsEditFormSaveSubject( wgt->text );
			}
		}
		else if( wparam == FS_KEY_LEFT )
		{
			num = FS_MmsEditFormGetCurFrameNum( );
			if( num != 1 )
			{
				FS_MmsEditFormSetCurFrame( num - 1 );
			}
			ret = FS_TRUE;
		}
		else if( wparam == FS_KEY_RIGHT )
		{
			num = FS_MmsEditFormGetCurFrameNum( );
			if( num != FS_MmsEditFormGetFrameCount() )
			{
				FS_MmsEditFormSetCurFrame( num + 1 );
			}
			ret = FS_TRUE;
		}
	}
	else if( FS_WM_DESTROY == cmd )
	{
		FS_MmsFile *pMmsFile;
		
		pMmsFile = FS_MmsEditFormGetMmsFile( );
		FS_DestroyMmsFile( pMmsFile );
		GFS_CurEditMmsHead = FS_NULL;
	}
	return ret;
}

static void FS_MmsEdit_UI( FS_MmsFile *pMmsFile )
{
	FS_Window *win;
	FS_Widget *wText, *wImage, *wAudio, *wVideo, *areaImage, *wFrames, *wSubject;
	FS_CHAR *detail;
	FS_EditParam eParam = { FS_IM_CHI, FS_IM_ALL, FS_DEFAULT_EDIT_LEN };
	
	win = FS_CreateWindow( FS_W_MmsEditFrm, FS_Text(FS_T_MMS_EDIT), FS_MmsEditWndProc );

	FS_MmsEditFormSaveData( win, pMmsFile );

	wText = FS_CreateEditBox( FS_W_MmsEditText, FS_NULL, 0, 1, &eParam );	
	wAudio = FS_CreateListItem( FS_W_MmsEditAudio, FS_Text(FS_T_ADD_AUDIO), FS_NULL, FS_I_AUDIO, 1 );
	wImage = FS_CreateListItem( FS_W_MmsEditImage, FS_Text(FS_T_ADD_IMAGE), FS_NULL, FS_I_IMAGE, 1 );
	wVideo = FS_CreateListItem( FS_W_MmsEditVideo, FS_Text(FS_T_ADD_VIDEO), FS_NULL, FS_I_VIDEO, 1 );
	areaImage = FS_CreateImage( FS_W_MmsEditImageArea, FS_NULL );
	wFrames = FS_CreateComboBox( FS_W_MmsEditFrames, FS_MmsEditFormFormatFrameNumText(-1), FS_I_EDIT );
	
	eParam.preferred_method = FS_IM_CHI;
	eParam.allow_method = FS_IM_ALL;
	/* 
	 * !!! 
	 *  ugly cmcc mmsc or cmcc gatway.
	 * if mms subject too long(larger than 40 ch, 80 bytes). it will return an error of 0x40(Bad Request). 
	*/
	eParam.max_len = FS_MMS_SUB_LEN - 10;	
	eParam.limit_character = FS_TRUE;
	wSubject = FS_CreateEditBox( FS_W_MmsEditSubject, FS_NULL, FS_I_SUBJECT, 1, &eParam );
	
	detail = FS_MmsEditFormGetMmsDetailText( );
	wText->tip = FS_Text( FS_T_MMS_TEXT );
	wAudio->tip = detail;
	wImage->tip = detail;
	wVideo->tip = detail;
	wFrames->tip = detail;
	areaImage->tip = detail;
	wSubject->tip = FS_Text( FS_T_MMS_SUBJECT );
	
	FS_WindowAddWidget( win, wText );
	FS_WindowAddWidget( win, areaImage );
	FS_WindowAddWidget( win, wImage );
	FS_WindowAddWidget( win, wAudio );
	FS_WindowAddWidget( win, wVideo );
	FS_WindowAddWidget( win, wFrames );
	FS_WindowAddWidget( win, wSubject );

	FS_WidgetSetHandler( wImage, FS_MmsEditFormImageOption_CB );
	FS_WidgetSetHandler( wAudio, FS_MmsEditFormAudioOption_CB );
	FS_WidgetSetHandler( wVideo, FS_MmsEditFormVideoOption_CB );
	FS_WidgetSetHandler( wFrames, FS_MmsEditSelectFrame_UI );
	
	FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_MENU), FS_MmsEditMenu_UI );
	FS_WindowSetSoftkey( win, 3, FS_Text(FS_T_BACK), FS_MmsEditExit_CB );

	FS_WidgetSetText( wSubject, FS_MmsEditFormGetSubject() );
	FS_ShowWindow( win );
	
	FS_MmsEditFormSetCurFrame( 1 );
}

static void FS_MmsNew_CB( FS_Window *win )
{
	if( FS_MmsIsFull( 1024 ) )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_SPACE_IS_FULL), FS_NULL, FS_FALSE );
		return;
	}

	FS_MmsEdit_UI( FS_NULL );

	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_MmsViewSpace_UI( FS_Window *win )
{
	FS_SINT4 num, totalnum, size, totalsize;
	FS_CHAR str[64];

	FS_MmsGetSpaceDetail( &num, &size );
	totalnum = IFS_GetMaxMmsItemLimit( );
	totalsize = IFS_GetMaxMmsSizeLimit( );
	IFS_Sprintf( str, "\n%s: %d/%d\n%s: %d/%d(KB)", FS_Text(FS_T_ITEMS), num, totalnum,
		FS_Text(FS_T_SPACE), FS_KB(size), FS_KB(totalsize) );
	FS_StdShowDetail( FS_Text(FS_T_VIEW_SPACE), str );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_MmsConfigSave_CB( FS_Window *win )
{
	FS_Window *swin = FS_WindowFindId( FS_W_MmsConfigFrm );
	FS_Widget *wgt;
	FS_UINT2 port;
	
	wgt = FS_WindowGetWidget( swin, FS_W_MmsConfigProtoHttp );
	if( FS_WGT_GET_CHECK(wgt) )
	{
		FS_MmsConfigSetProtocol( FS_MMS_HTTP );
		port = 80;
	}
	else
	{
		FS_MmsConfigSetProtocol( FS_MMS_WSP );
		port = 9201;
	}

	wgt = FS_WindowGetWidget( swin, FS_W_MmsConfigProxyAddr );
	FS_MmsConfigSetProxyAddr( wgt->text );

	wgt = FS_WindowGetWidget( swin, FS_W_MmsConfigProxyPort );
	if( wgt->text )
	{
		port = IFS_Atoi( wgt->text );
	}
	FS_MmsConfigSetProxyPort( port );

	wgt = FS_WindowGetWidget( swin, FS_W_MmsConfigMmsCenter );
	FS_MmsConfigSetMmsCenterUrl( wgt->text );

	wgt = FS_WindowGetWidget( swin, FS_W_MmsConfigApn );
	FS_MmsConfigSetApn( wgt->text );

	wgt = FS_WindowGetWidget( swin, FS_W_MmsConfigUserName );
	FS_MmsConfigSetUserName( wgt->text );

	wgt = FS_WindowGetWidget( swin, FS_W_MmsConfigPasswd );
	FS_MmsConfigSetPassword( wgt->text );

	wgt = FS_WindowGetWidget( swin, FS_W_MmsConfigAutoRecv );
	FS_MmsConfigSetAutoRecvFlag( FS_WGT_GET_CHECK(wgt) );

	wgt = FS_WindowGetWidget( swin, FS_W_MmsConfigAllowAds );
	FS_MmsConfigSetAllowAdsFlag( FS_WGT_GET_CHECK(wgt) );

	wgt = FS_WindowGetWidget( swin, FS_W_MmsConfigAllowDlvRpt );
	FS_MmsConfigSetAllowDeliveryReportFlag( FS_WGT_GET_CHECK(wgt) );

	wgt = FS_WindowGetWidget( swin, FS_W_MmsConfigAllowReadRpt );
	FS_MmsConfigSetAllowReadReportFlag( FS_WGT_GET_CHECK(wgt) );

	wgt = FS_WindowGetWidget( swin, FS_W_MmsConfigSaveSend );
	FS_MmsConfigSetSaveSendFlag( FS_WGT_GET_CHECK(wgt) );

	FS_MmsConfigSave( );
	FS_MessageBox( FS_MS_OK, FS_Text(FS_T_CONFIG_UPDATED), FS_NULL, FS_FALSE );
	FS_DestroyWindow( swin );
}

static FS_BOOL FS_MmsConfigWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	FS_Widget *wgt;
	
	if( cmd == FS_WM_COMMAND && wparam == FS_EV_ITEM_VALUE_CHANGE )
	{
		wgt = ( FS_Widget *)lparam;
		if( wgt->id == FS_W_MmsConfigProtoHttp || wgt->id == FS_W_MmsConfigProtoWsp )
		{
			wgt = FS_WindowGetWidget( win, FS_W_MmsConfigProtoHttp );
			if( FS_WGT_GET_CHECK(wgt) )
			{
				wgt = FS_WindowGetWidget( win, FS_W_MmsConfigProxyPort );
				FS_WidgetSetText( wgt, "80" );
			}
			else
			{
				wgt = FS_WindowGetWidget( win, FS_W_MmsConfigProxyPort );
				FS_WidgetSetText( wgt, "9201" );
			}
			ret = FS_TRUE;
		}
	}
	return ret;
}

static void FS_MmsConfig_UI( FS_Window *win )
{
	FS_CHAR strPort[16], *strProxy, *strApn, *strUser, *strPasswd;
	FS_Widget *wProtHttp, *wProtWsp, *wProxyAddr, *wProxyPort, *wMmsCenter, *wAutoRecv, 
		*wAllowAds, *wAllowDR, *wAllowRR, *wSaveSend, *wApn, *wUser, *wPasswd;
	FS_Window *twin = FS_CreateWindow( FS_W_MmsConfigFrm, FS_Text(FS_T_SYS_SETTING), FS_MmsConfigWndProc );
	FS_EditParam eParam = { FS_IM_ABC, FS_IM_123 | FS_IM_ABC, FS_URL_LEN };
	/* use id as skin's index. must follow up GFS_Skins define */
	IFS_Itoa( FS_MmsConfigGetProxyPort(), strPort, 10 );
	strProxy = FS_MmsConfigGetProxyAddr();
	strApn = FS_MmsConfigGetApn( );
	strUser = FS_MmsConfigGetUserName( );
	strPasswd = FS_MmsConfigGetPassword( );
	
	wProtHttp = FS_CreateRadioBox( FS_W_MmsConfigProtoHttp, FS_Text(FS_T_PROTO_HTTP) );
	wProtWsp = FS_CreateRadioBox( FS_W_MmsConfigProtoWsp, FS_Text(FS_T_PROTO_WSP) );
	wProxyAddr = FS_CreateEditBox( FS_W_MmsConfigProxyAddr, strProxy, FS_I_EDIT, 1, &eParam );
	wMmsCenter = FS_CreateEditBox( FS_W_MmsConfigMmsCenter, FS_MmsConfigGetMmsCenterUrl(), FS_I_HOME, 1, &eParam );
	eParam.preferred_method = FS_IM_123;
	eParam.allow_method = FS_IM_123;
	eParam.max_len = 12;
	wProxyPort = FS_CreateEditBox( FS_W_MmsConfigProxyPort, strPort, FS_I_EDIT, 1, &eParam );
	eParam.preferred_method = FS_IM_ABC;
	eParam.allow_method = FS_IM_ALL;
	eParam.max_len = FS_URL_LEN - 1;
	wApn = FS_CreateEditBox( FS_W_MmsConfigApn, strApn, FS_I_EDIT, 1, &eParam );
	eParam.preferred_method = FS_IM_ABC;
	eParam.allow_method = FS_IM_ALL;
	eParam.max_len = FS_URL_LEN - 1;
	wUser = FS_CreateEditBox( FS_W_MmsConfigUserName, strUser, FS_I_EDIT, 1, &eParam );
	eParam.preferred_method = FS_IM_ABC;
	eParam.allow_method = FS_IM_ALL;
	eParam.max_len = FS_URL_LEN - 1;
	wPasswd = FS_CreateEditBox( FS_W_MmsConfigPasswd, strPasswd, FS_I_EDIT, 1, &eParam );

	wAutoRecv = FS_CreateCheckBox( FS_W_MmsConfigAutoRecv, FS_Text(FS_T_AUTO_RECV) );
	wAllowAds = FS_CreateCheckBox( FS_W_MmsConfigAllowAds, FS_Text(FS_T_ALLOW_ADS) );
	wAllowDR = FS_CreateCheckBox( FS_W_MmsConfigAllowDlvRpt, FS_Text(FS_T_ALLOW_DLV_RPT) );
	wAllowRR = FS_CreateCheckBox( FS_W_MmsConfigAllowReadRpt, FS_Text(FS_T_ALLOW_READ_RPT) );
	wSaveSend = FS_CreateCheckBox( FS_W_MmsConfigSaveSend, FS_Text(FS_T_SAVE_SEND) );
	
	wProxyAddr->tip = FS_Text(FS_T_PROXY_ADDR);
	wProxyPort->tip = FS_Text(FS_T_PROXY_PORT);
	wMmsCenter->tip = FS_Text(FS_T_MMS_CENTER);
	wProtHttp->tip = FS_Text(FS_T_PROTO_HTTP);
	wProtWsp->tip = FS_Text(FS_T_PROTO_WSP);
	wAutoRecv->tip = FS_Text(FS_T_AUTO_RECV);
	wAllowAds->tip = FS_Text(FS_T_ALLOW_ADS);
	wAllowDR->tip = FS_Text(FS_T_ALLOW_DLV_RPT);
	wAllowRR->tip = FS_Text(FS_T_ALLOW_READ_RPT);
	wSaveSend->tip = FS_Text(FS_T_SAVE_SEND);
	wApn->tip = FS_Text(FS_T_APN);
	wUser->tip = FS_Text(FS_T_USER_NAME);
	wPasswd->tip = FS_Text(FS_T_PASSWORD);
	
	twin->draw_status_bar = FS_TRUE;
	twin->pane.view_port.height -= IFS_GetLineHeight( );
	
	FS_WindowAddWidget( twin, wProtHttp );
	FS_WindowAddWidget( twin, wProtWsp );
	FS_WindowAddWidget( twin, wMmsCenter );
	FS_WindowAddWidget( twin, wProxyAddr );
	FS_WindowAddWidget( twin, wProxyPort );
	FS_WindowAddWidget( twin, wApn );
	FS_WindowAddWidget( twin, wUser );
	FS_WindowAddWidget( twin, wPasswd );

	FS_WindowAddWidget( twin, wAutoRecv );
	FS_WindowAddWidget( twin, wAllowAds );
	FS_WindowAddWidget( twin, wAllowDR );
	FS_WindowAddWidget( twin, wAllowRR );
	FS_WindowAddWidget( twin, wSaveSend );
	
	FS_WidgetSetCheck( wAutoRecv, FS_MmsConfigGetAutoRecvFlag() );
	FS_WidgetSetCheck( wAllowAds, FS_MmsConfigGetAllowAdsFlag() );
	FS_WidgetSetCheck( wAllowDR, FS_MmsConfigGetAllowDeliveryReportFlag() );
	FS_WidgetSetCheck( wAllowRR, FS_MmsConfigGetAllowReadReportFlag() );
	FS_WidgetSetCheck( wSaveSend, FS_MmsConfigGetSaveSendFlag() );
	
	if( FS_MmsConfigGetProtocol() == FS_MMS_HTTP )
		FS_WidgetSetCheck( wProtHttp, FS_TRUE );
	else
		FS_WidgetSetCheck( wProtWsp, FS_TRUE );
	
	FS_WindowSetSoftkey( twin, 1, FS_Text(FS_T_SAVE), FS_MmsConfigSave_CB );
	FS_WindowSetSoftkey( twin, 3, FS_Text(FS_T_CANCEL), FS_StandardKey3Handler );
	
	FS_ShowWindow( twin );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static FS_BOOL FS_MmsDelAllFolderCnf_CB( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	if( wparam == FS_EV_YES )	// exit without save account data
	{
		FS_MmsDelAllFolder( );
		ret = FS_TRUE;
	}
	return ret;
}

static void FS_MmsDelAllFolder_CB( FS_Window *win )
{
	FS_SINT4 total;
	FS_MmsGetSpaceDetail( &total, FS_NULL );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );

	if( total > 0 )
	{
		FS_MessageBox( FS_MS_YES_NO, FS_Text(FS_T_CONFIRM_DEL), FS_MmsDelAllFolderCnf_CB, FS_FALSE ); 
	}
}

static void FS_MmsMainMenu_UI( FS_Window *win )
{
	FS_Widget *iNew, *iSysSet, *iAbout, *iSpace, *iDelAll;
	FS_Window *pMenu;
	iNew = FS_CreateMenuItem( 0,  FS_Text(FS_T_MMS_NEW) );
	iSysSet = FS_CreateMenuItem( 0,  FS_Text(FS_T_SYS_SETTING) );
	iAbout = FS_CreateMenuItem( 0,  FS_Text(FS_T_ABOUT) );
	iSpace = FS_CreateMenuItem( 0,	FS_Text(FS_T_VIEW_SPACE) );
	iDelAll = FS_CreateMenuItem( 0,	FS_Text(FS_T_DEL_ALL) );
	
	pMenu = FS_CreateMenu( FS_W_EmlMainMenu, 5 );
	FS_MenuAddItem( pMenu, iNew );
	FS_MenuAddItem( pMenu, iSpace );
	FS_MenuAddItem( pMenu, iSysSet );
	FS_MenuAddItem( pMenu, iDelAll );
	FS_MenuAddItem( pMenu, iAbout );
	
	FS_WidgetSetHandler( iNew, FS_MmsNew_CB );
	FS_WidgetSetHandler( iSpace, FS_MmsViewSpace_UI );
	FS_WidgetSetHandler( iSysSet, FS_MmsConfig_UI );
	FS_WidgetSetHandler( iDelAll, FS_MmsDelAllFolder_CB );
	FS_WidgetSetHandler( iAbout, FS_ThemeSetting_UI );
	
	FS_MenuSetSoftkey( pMenu );

	FS_ShowWindow( pMenu );
}

static FS_CHAR *FS_MmsFormatReadReport( FS_MmsHead *mms, FS_BOOL bRead )
{
	FS_DateTime dt;
	FS_CHAR datestr[32];
	FS_CHAR *content = IFS_Malloc( 2048 );
	if( content )
	{
		IFS_GetDateTime( &dt );
		FS_DateStruct2DispStr( datestr, &dt );
		if( bRead )
		{
			IFS_Sprintf( content, FS_Text(FS_T_MMS_READ_REPORT), 
				mms->address, datestr, mms->address, mms->subject, mms->msg_id );
		}
		else
		{
			IFS_Sprintf( content, FS_Text(FS_T_MMS_DEL_REPORT), 
				mms->address, datestr, mms->address, mms->subject, mms->msg_id );
		}
	}
	return content;
}

static void FS_MmsView_CB( FS_Window *win )
{
	FS_MmsFile *pMmsFile;
	FS_MmsHead *mms = FS_MmsListFormGetHead( );

	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
	
	if( mms )
	{
		pMmsFile = FS_MmsDecodeFile( mms->file );
		if( pMmsFile )
		{
			FS_MmsView_UI( pMmsFile, FS_FALSE );
			if( FS_MMS_REQ_READ_REPORT(mms) && FS_MmsConfigGetAllowReadReportFlag() )
			{
				#if 1
				FS_CHAR *sub, *content = FS_MmsFormatReadReport( mms, FS_TRUE );
				FS_MMS_CLR_REQ_READ_REPORT(mms);
				if( content )
				{
					sub = FS_StrConCat( FS_Text(FS_T_READED), ":", mms->subject, FS_NULL );
					FS_MmsSendReadReportMms( mms->address, sub, content );
					FS_SAFE_FREE( sub );
					IFS_Free( content );
				}
				#else
				FS_MmsSendReadReportPdu( mms->msg_id, mms->address, FS_TRUE );
				#endif
			}
		}
		else
		{
			FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_UNKNOW_ERR), FS_NULL, FS_FALSE );
		}
		
		if( FS_MMS_UNREAD(mms) )
		{
			FS_MMS_SET_READ( mms );
			FS_MmsListFormUpdateCurItem( mms );
			FS_MmsSaveHeadList( );
		}
	}
	
}

static void FS_MmsEdit_CB( FS_Window *win )
{
	FS_MmsHead *mms = FS_MmsListFormGetHead( );
	if( mms )
	{
		FS_MmsFile *pMmsFile = FS_MmsDecodeFile( mms->file );
		if( pMmsFile )
		{
			FS_MmsEdit_UI( pMmsFile );
		}
		else
		{
			FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_UNKNOW_ERR), FS_NULL, FS_TRUE );
		}
	}

	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static FS_BOOL FS_MmsRecvDlgProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;

	if( cmd == FS_WM_COMMAND && wparam == FS_EV_NO )
	{
		if( GFS_MmsNetState.timer_id )
		{
			IFS_StopTimer( GFS_MmsNetState.timer_id );
			GFS_MmsNetState.timer_id = 0;
		}
		FS_MmsNetCancel( );
		ret = FS_TRUE;
	}
	return ret;
}

static void FS_MmsRecv_CB( FS_Window *win )
{
	FS_Window *msgBox;
	FS_MmsHead *mms;
	FS_MmsNetState *pMmsNetState = &GFS_MmsNetState;
	FS_CHAR *msg_location;
	FS_SINT4 ret;
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
	
	mms = FS_MmsListFormGetHead( );

	if( mms == FS_NULL ) return;
	
	/* check mms file size */
	if( mms->msg_size > IFS_GetMaxMmsSize() )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_MMS_TOO_LARGE), FS_NULL, FS_FALSE );
		return;
	}
	/* check expiry */
	if( mms->expiry > 0 )
	{
		if( FS_GetSeconds( IFS_GetTimeZone() ) > mms->expiry )
		{
			FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_MMS_EXPIRY), FS_NULL, FS_FALSE );
			return;
		}
	}
	/* check mms memory limit */
	if( FS_MmsIsFull( mms->msg_size ) )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_SPACE_IS_FULL), FS_NULL, FS_FALSE );
		return;
	}
	IFS_EnterBackLight( );
	msgBox = FS_MessageBox( FS_MS_INFO_CANCEL, FS_NULL, FS_MmsRecvDlgProc, FS_FALSE ); 
	msgBox->id = FS_W_MmsRecvingDlg;

	pMmsNetState->offset = 0;
	pMmsNetState->total = mms->msg_size;
	pMmsNetState->text_id = FS_T_CONNECTING;
	pMmsNetState->second = 0;
	pMmsNetState->timer_id = IFS_StartTimer( FS_TIMER_ID_MMS_COUNTER, 1000, FS_MmsNetCounter_CB, FS_NULL );
	
	FS_MmsDisplayNetPrograss( msgBox, pMmsNetState );
	msg_location = mms->msg_location;		
	ret = FS_MmsRecv( msg_location, FS_MmsNetCallback, FS_NULL );
	if( ret != FS_MMS_NET_OK )
	{
		FS_MmsDisplayNetStatus( ret, 0 );
	}
}

static void FS_MmsDetail_UI( FS_Window *win )
{
	FS_MmsHead *mms;
	FS_UINT1 mbox = FS_MmsListFormGetMBox( );
	FS_CHAR *str, *info;
	FS_SINT4 offset;

	mms = FS_MmsListFormGetHead( );
	if( mms )
	{
		if( mbox == FS_MMS_INBOX )
			str = FS_Text( FS_T_FROM );
		else
			str = FS_Text( FS_T_TO );

		info = IFS_Malloc( FS_MMS_DETAIL_LEN );
		if( info )
		{
			offset = 0;
			IFS_Memset( info, 0, FS_MMS_DETAIL_LEN );
			/* address */
			IFS_Sprintf( info + offset, "%s: %s", str, mms->address );
			str = IFS_PBFindNameByPhone( mms->address );
			if( str )
			{
				offset += IFS_Strlen( info + offset );
				IFS_Sprintf( info + offset, "(%s)", str );
			}
			/* date */
			offset += IFS_Strlen( info + offset );
			IFS_Sprintf( info + offset, "\n%s:", FS_Text(FS_T_DATE) );
			offset += IFS_Strlen( info + offset );
			FS_DateStruct2DispStr( info + offset, &mms->date );
			/* expire */
			if( mbox == FS_MMS_INBOX && FS_MMS_IS_NTF(mms) )
			{
				FS_DateTime date;
				FS_SecondsToDateTime( &date, mms->expiry, IFS_GetTimeZone() );
				
				offset += IFS_Strlen( info + offset );
				IFS_Sprintf( info + offset, "\n%s:", FS_Text(FS_T_EXPIRY) );
				offset += IFS_Strlen( info + offset );
				FS_DateStruct2DispStr( info + offset, &date );
			}
			/* size */
			offset += IFS_Strlen( info + offset );
			IFS_Sprintf( info + offset, "\n%s: %d(Bytes)\n%s: %s", 
				FS_Text(FS_T_SIZE), mms->msg_size, FS_Text(FS_T_SUBJECT), mms->subject );

			if( mbox == FS_MMS_INBOX )
			{
				if( FS_MMS_IS_NTF(mms) )
				{
					offset += IFS_Strlen( info + offset );
					IFS_Sprintf( info + offset, "\n%s: %s", FS_Text(FS_T_STATUS), FS_Text(FS_T_UNRECV) );
					offset += IFS_Strlen( info + offset );
					IFS_Sprintf( info + offset, "\n%s:\n%s", FS_Text(FS_T_LOCATION), mms->msg_location );
				}
				else
				{
					offset += IFS_Strlen( info + offset );
					if( FS_MMS_UNREAD(mms) )
						IFS_Sprintf( info + offset, "\n%s: %s", FS_Text(FS_T_STATUS), FS_Text(FS_T_UNREAD) );
					else
						IFS_Sprintf( info + offset, "\n%s: %s", FS_Text(FS_T_STATUS), FS_Text(FS_T_READED) );
				}
			}
			if( mms->msg_id[0] )
			{
				offset += IFS_Strlen( info + offset );
				IFS_Sprintf( info + offset, "\nid: %s", mms->msg_id );
			}
			FS_StdShowDetail( FS_Text(FS_T_DETAIL), info );
			IFS_Free( info );
		}
	}
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static FS_BOOL FS_MmsDelCnf_CB( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	if( wparam == FS_EV_YES )	// exit without save account data
	{
		FS_Window *lwin = FS_WindowFindId( FS_W_MmsListFrm );
		FS_Widget *wgt = FS_WindowGetFocusItem( lwin );
		FS_MmsHead *mms;
		
		if( wgt )
		{
			mms = (FS_MmsHead *)wgt->private_data;
			if( FS_MMS_UNREAD(mms) && FS_MMS_REQ_READ_REPORT(mms) )
			{
				FS_CHAR *sub, *content = FS_MmsFormatReadReport( mms, FS_FALSE );
				FS_MMS_CLR_REQ_READ_REPORT(mms);
				if( content )
				{
					sub = FS_StrConCat( FS_Text(FS_T_DEL), ":", mms->subject, FS_NULL );
					FS_MmsSendReadReportMms( mms->address, sub, content );
					FS_SAFE_FREE( sub );
					IFS_Free( content );
				}
			}
			FS_MmsDelHead( mms );
			FS_WindowDelWidget( lwin, wgt );
		}
		ret = FS_TRUE;
	}
	return ret;
}

static void FS_MmsDel_CB( FS_Window *win )
{
	FS_MmsHead *mms = FS_MmsListFormGetHead( );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );

	if( mms )
	{
		FS_MessageBox( FS_MS_YES_NO, FS_Text(FS_T_CONFIRM_DEL), FS_MmsDelCnf_CB, FS_FALSE ); 
	}
}

static void FS_MmsSetUnread_CB( FS_Window *win )
{
	FS_MmsHead *mms = FS_MmsListFormGetHead( );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );

	if( mms )
	{
		FS_MMS_SET_UNREAD( mms );
		FS_MmsSaveHeadList( );
		FS_MmsListFormUpdateCurItem( mms );
		FS_MessageBox( FS_MS_OK, FS_Text(FS_T_SUCCESS), FS_NULL, FS_TRUE ); 
	}
}

static void FS_MmsSetRead_CB( FS_Window *win )
{
	FS_MmsHead *mms = FS_MmsListFormGetHead( );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );

	if( mms )
	{
		FS_MMS_SET_READ( mms );
		FS_MmsSaveHeadList( );
		FS_MmsListFormUpdateCurItem( mms );
		FS_MessageBox( FS_MS_OK, FS_Text(FS_T_SUCCESS), FS_NULL, FS_TRUE ); 
	}
}

static void FS_MmsSaveAsTemplate_HD( void )
{
	FS_MmsHead *mms = FS_MmsListFormGetHead( );
	FS_MmsFile *pMmsFile, *pNewMms;
	FS_CHAR file[FS_FILE_NAME_LEN];
	
	pMmsFile = FS_MmsDecodeFile( mms->file );
	if( pMmsFile )
	{
		pNewMms = FS_MmsFileDuplicate( pMmsFile );
		if( pNewMms )
		{
			FS_GetGuid( file );
			IFS_Strcat( file, ".mms" );
			FS_MmsEncodeFile( file, pNewMms );
			FS_MmsHeadSaveAsTemplate( mms, file );
			FS_DestroyMmsFile( pNewMms );
			FS_MessageBox( FS_MS_OK, FS_Text(FS_T_SUCCESS), FS_NULL, FS_TRUE ); 
		}
		else
		{
			FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_UNKNOW_ERR), FS_NULL, FS_FALSE ); 
		}
		FS_DestroyMmsFile( pMmsFile );
	}
	else
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_UNKNOW_ERR), FS_NULL, FS_FALSE ); 
	}
	FS_DestroyWindowByID( FS_W_ProgressFrm );
}

static void FS_MmsSaveAsTemplate_CB( FS_Window *win )
{
	FS_MmsHead *mms = FS_MmsListFormGetHead( );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );

	if( mms == FS_NULL ) return;
	
	if(! FS_MMS_IS_NTF(mms) )
	{		
		if( FS_MmsIsFull( mms->msg_size ) )
		{
			FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_SPACE_IS_FULL), FS_NULL, FS_FALSE );
			return;
		}
		FS_MessageBox( FS_MS_NONE, FS_Text(FS_T_PLS_WAITING), FS_NULL, FS_FALSE );
		IFS_PostMessage( FS_MSG_UTIL_CALL, (FS_UINT4)FS_MmsSaveAsTemplate_HD );
	}
	else
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_CAN_NOT_SAVE), FS_NULL, FS_FALSE );
	}
}

static FS_BOOL FS_MmsDelAllCnf_CB( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	if( wparam == FS_EV_YES )	// exit without save account data
	{
		FS_Window *lwin = FS_WindowFindId( FS_W_MmsListFrm );
		FS_UINT1 mbox = FS_MmsListFormGetMBox( );
		
		FS_MmsDelHeadBox( mbox );
		FS_MessageBox( FS_MS_OK, FS_Text(FS_T_DEL_OK), FS_NULL, FS_TRUE );
		FS_DestroyWindow( lwin );
		ret = FS_TRUE;
	}
	return ret;
}

static void FS_MmsDelAll_CB( FS_Window *win )
{
	FS_MmsHead *mms = FS_MmsListFormGetHead( );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
	
	if( mms )
	{
		FS_MessageBox( FS_MS_YES_NO, FS_Text(FS_T_CONFIRM_DEL), FS_MmsDelAllCnf_CB, FS_FALSE );	
	}
}

static void FS_MmsListMenu_UI( FS_Window *win )
{
	FS_Widget *iDetail, *iDel, *iDelAll, *iOK, *iSetUnread = FS_NULL, *iSetRead = FS_NULL, *iTemplate = FS_NULL;
	FS_Window *pMenu;
	FS_CHAR *sok;
	FS_MmsHead *mms = FS_MmsListFormGetHead( );
	FS_UINT1 mbox = FS_MmsListFormGetMBox( );
	FS_WidgetEventHandler handler;
	FS_SINT4 nItems = 4;
	
	if( mbox == FS_MMS_INBOX )
	{
		if( mms && FS_MMS_IS_NTF(mms) )
		{
			sok = FS_Text( FS_T_RETR );
			handler = FS_MmsRecv_CB;
		}
		else
		{
			sok = FS_Text( FS_T_VIEW );
			handler = FS_MmsView_CB;
		}
		nItems ++;
		if( mms && FS_MMS_UNREAD(mms) )
			iSetRead = FS_CreateMenuItem( 0, FS_Text(FS_T_SET_READ) );
		else
			iSetUnread = FS_CreateMenuItem( 0, FS_Text(FS_T_SET_UNREAD) );
	}
	else
	{
		sok = FS_Text( FS_T_EDIT );
		handler = FS_MmsEdit_CB;
	}

	if( mbox != FS_MMS_TEMPLATE )
	{
		iTemplate = FS_CreateMenuItem( 0,  FS_Text(FS_T_SAVE_AS_TEMPLATE) );
		nItems ++;
	}
	
	iOK = FS_CreateMenuItem( 0, sok );
	iDetail = FS_CreateMenuItem( 0,  FS_Text(FS_T_DETAIL) );
	iDel = FS_CreateMenuItem( 0, FS_Text(FS_T_DEL) );
	iDelAll = FS_CreateMenuItem( 0,  FS_Text(FS_T_DEL_ALL) );

	pMenu = FS_CreateMenu( FS_W_EmlMainMenu, nItems );
	
	FS_MenuAddItem( pMenu, iOK );
	FS_MenuAddItem( pMenu, iDetail );
	if( iSetUnread )
	{
		FS_MenuAddItem( pMenu, iSetUnread );
		FS_WidgetSetHandler( iSetUnread, FS_MmsSetUnread_CB );
	}
	if( iSetRead )
	{
		FS_MenuAddItem( pMenu, iSetRead );
		FS_WidgetSetHandler( iSetRead, FS_MmsSetRead_CB );
	}
	if( iTemplate )
	{
		FS_MenuAddItem( pMenu, iTemplate );
		FS_WidgetSetHandler( iTemplate, FS_MmsSaveAsTemplate_CB );
	}
	FS_MenuAddItem( pMenu, iDel );
	FS_MenuAddItem( pMenu, iDelAll );
	
	FS_WidgetSetHandler( iOK, handler );
	FS_WidgetSetHandler( iDetail, FS_MmsDetail_UI );
	FS_WidgetSetHandler( iDel, FS_MmsDel_CB );
	FS_WidgetSetHandler( iDelAll, FS_MmsDelAll_CB );
	
	FS_MenuSetSoftkey( pMenu );
	
	FS_ShowWindow( pMenu );
}

static void FS_MmsListBuild( FS_Window *win, FS_UINT1 mbox )
{
	FS_MmsHead *mms;
	FS_List *head, *node;
	FS_Widget *wgt;
	FS_CHAR *str;

	FS_WindowDelWidgetList( win );
	
	head = FS_MmsGetHeadList( );
	node = head->next;
	while( node != head )
	{
		mms = FS_ListEntry( node, FS_MmsHead, list );
		node = node->next;
	
		if( mms->mbox == mbox )
		{
			if( mms->subject[0] )
				str = mms->subject;
			else
				str = FS_Text( FS_T_NO_SUBJECT );
			
			if( FS_MMS_UNREAD(mms) )
			{
				if( FS_MMS_IS_NTF(mms) )
					wgt = FS_CreateListItem( 0, str, FS_NULL, FS_I_NEW_NTF, 1 );
				else
					wgt = FS_CreateListItem( 0, str, FS_NULL, FS_I_NEW_MSG, 1 );
			}
			else
			{
				wgt = FS_CreateListItem( 0, str, FS_NULL, FS_I_READED_MSG, 1 );
			}
	
			if( FS_MMS_IS_NTF(mms) )
			{
				FS_WidgetSetHandler( wgt, FS_MmsRecv_CB );
			}
			else
			{
				if( mbox == FS_MMS_INBOX )
					FS_WidgetSetHandler( wgt, FS_MmsView_CB );
				else
					FS_WidgetSetHandler( wgt, FS_MmsEdit_CB );
			}
	
			wgt->private_data = (FS_UINT4)mms;
			FS_WindowAddWidget( win, wgt );
		}
	}
}

static void FS_MmsList_UI( FS_Window *win )
{
	FS_Widget *wgt = FS_WindowGetFocusItem( win );
	FS_UINT1 mbox;
	FS_CHAR *wtitle;
	FS_Window *lwin;

	if( wgt == FS_NULL )
	{
		mbox = FS_MMS_INBOX;
	}
	else
	{
		mbox = wgt->private_data;
	}
	
	if( mbox == FS_MMS_INBOX )
	{
		wtitle = FS_Text( FS_T_INBOX );
	}
	else
	{
		if( mbox == FS_MMS_OUTBOX )
			wtitle = FS_Text( FS_T_OUTBOX );
		else if( mbox == FS_MMS_DRAFT )
			wtitle = FS_Text( FS_T_DRAFTBOX );
		else if( mbox == FS_MMS_SENDBOX)
			wtitle = FS_Text( FS_T_SENDBOX );
		else
			wtitle = FS_Text( FS_T_MMS_TEMPLATE );
	}

	lwin = FS_CreateWindow( FS_W_MmsListFrm, wtitle, FS_NULL );
	lwin->private_data = mbox;
	lwin->show_index = FS_TRUE;

	FS_MmsListBuild( lwin, mbox );

	FS_WindowSetSoftkey( lwin, 1, FS_Text(FS_T_MENU), FS_MmsListMenu_UI );
	FS_WindowSetSoftkey( lwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	FS_ShowWindow( lwin );
}

//-----------------------------------------------------------------------------------
// exit the email main window
static void FS_MmsExit_CB( FS_Window *win )
{
	FS_MmsHeadDeinit( );
	FS_NetDisconnect( FS_APP_MMS );
	FS_DeactiveApplication( FS_APP_MMS );
	
	if( ! FS_HaveActiveApplication() )
	{
		FS_GuiExit( );
		IFS_SystemExit( );
	}
}

static FS_BOOL FS_MmsMainWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	FS_CHAR str[32];
	FS_SINT4 nUnread, nTotal;
	FS_Widget *wgt;
	
	if( cmd == FS_WM_PAINT )
	{
		FS_MmsGetFolderDetail( FS_MMS_INBOX, &nUnread, &nTotal );
		IFS_Sprintf( str, "%s (%d/%d)", FS_Text(FS_T_INBOX), nUnread, nTotal );
		wgt = FS_WindowGetWidget( win, FS_W_MmsInbox );
		FS_WidgetSetText( wgt, str );
		
		FS_MmsGetFolderDetail( FS_MMS_SENDBOX, &nUnread, &nTotal );
		IFS_Sprintf( str, "%s (%d)", FS_Text(FS_T_SENDBOX), nTotal );
		wgt = FS_WindowGetWidget( win, FS_W_MmsSendbox );
		FS_WidgetSetText( wgt, str );
		
		FS_MmsGetFolderDetail( FS_MMS_DRAFT, &nUnread, &nTotal );
		IFS_Sprintf( str, "%s (%d)", FS_Text(FS_T_DRAFTBOX), nTotal );
		wgt = FS_WindowGetWidget( win, FS_W_MmsDraft );
		FS_WidgetSetText( wgt, str );
		
		FS_MmsGetFolderDetail( FS_MMS_OUTBOX, &nUnread, &nTotal );
		IFS_Sprintf( str, "%s (%d)", FS_Text(FS_T_OUTBOX), nTotal );
		wgt = FS_WindowGetWidget( win, FS_W_MmsOutbox );
		FS_WidgetSetText( wgt, str );
		
		FS_MmsGetFolderDetail( FS_MMS_TEMPLATE, &nUnread, &nTotal );
		IFS_Sprintf( str, "%s (%d)", FS_Text(FS_T_MMS_TEMPLATE), nTotal );
		wgt = FS_WindowGetWidget( win, FS_W_MmsTemplate );
		FS_WidgetSetText( wgt, str );
		
		ret = FS_FALSE;
	}

	return ret;
}

//-----------------------------------------------------------------------------------
// open the email main window
void FS_MmsMain( void )
{	
	FS_Widget *liInBox, *liOutBox, *liDrafBox, *liTemplate, *liNew, *liSendBox;
	FS_Window *win;
	// sys init place here
	FS_MmsSysInit( );
	FS_ActiveApplication( FS_APP_MMS );
	// end init
	
	win = FS_CreateWindow( FS_W_MmsMainFrm, FS_Text(FS_T_MMS), FS_MmsMainWndProc );
	
	liNew = FS_CreateListItem( FS_W_MmsNew, FS_Text(FS_T_MMS_NEW), FS_NULL, FS_I_NEW_MSG, 1 );
	liInBox = FS_CreateListItem( FS_W_MmsInbox, FS_NULL, FS_NULL, FS_I_DIR, 1 );
	liOutBox = FS_CreateListItem( FS_W_MmsOutbox, FS_NULL, FS_NULL, FS_I_DIR, 1 );
	liDrafBox = FS_CreateListItem( FS_W_MmsDraft, FS_NULL, FS_NULL, FS_I_DIR, 1 );
	liSendBox = FS_CreateListItem( FS_W_MmsSendbox, FS_NULL, FS_NULL, FS_I_DIR, 1 );
	liTemplate = FS_CreateListItem( FS_W_MmsTemplate, FS_NULL, FS_NULL, FS_I_DIR, 1 );

	FS_WidgetSetHandler( liNew, FS_MmsNew_CB );
	FS_WidgetSetHandler( liInBox, FS_MmsList_UI );
	FS_WidgetSetHandler( liOutBox, FS_MmsList_UI );
	FS_WidgetSetHandler( liDrafBox, FS_MmsList_UI );
	FS_WidgetSetHandler( liSendBox, FS_MmsList_UI );
	FS_WidgetSetHandler( liTemplate, FS_MmsList_UI );

	liInBox->private_data = FS_MMS_INBOX;
	liOutBox->private_data = FS_MMS_OUTBOX;
	liDrafBox->private_data = FS_MMS_DRAFT;
	liSendBox->private_data = FS_MMS_SENDBOX;
	liTemplate->private_data = FS_MMS_TEMPLATE;
	
	FS_WindowAddWidget( win, liNew );
	FS_WindowAddWidget( win, liInBox );
	FS_WindowAddWidget( win, liSendBox );
	FS_WindowAddWidget( win, liDrafBox );
	FS_WindowAddWidget( win, liOutBox );
	FS_WindowAddWidget( win, liTemplate );

	FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_MENU), FS_MmsMainMenu_UI );
	FS_WindowSetSoftkey( win, 3, FS_Text(FS_T_EXIT), FS_MmsExit_CB );
	
	FS_ShowWindow( win );	
}

void FS_MmsExit( void )
{
	FS_MmsExit_CB( FS_NULL );
}

/* a new mms push notify arrived */
void FS_MmsNewNotify( FS_SINT4 size, FS_UINT4 date, FS_UINT4 expiry, 
	FS_CHAR *from, FS_CHAR *subject, FS_CHAR *content_location, FS_CHAR *msg_id )
{
	FS_Window *win;
	FS_MmsHead *mms;
	FS_BOOL repaint = FS_FALSE;
	
	mms = FS_NEW( FS_MmsHead );
	if( mms )
	{
		IFS_Memset( mms, 0, sizeof(FS_MmsHead) );
		mms->msg_size = size;
		FS_MmsAddrToUserFormat( mms->address, sizeof(mms->address) - 1, from, -1 );
		if( date > 0 )
			FS_SecondsToDateTime( &mms->date, date, IFS_GetTimeZone() );
		else
			IFS_GetDateTime( &mms->date );
		mms->expiry = expiry;
		
		if( subject )
			IFS_Strncpy( mms->subject, subject, FS_MMS_SUB_LEN - 1 );
		if( msg_id )
			IFS_Strncpy( mms->msg_id, msg_id, FS_MMS_MSG_ID_LEN - 1 );
		mms->mbox = FS_MMS_INBOX;
		if( content_location )
		{
			FS_MMS_SET_NTF( mms );
			mms->msg_location = IFS_Strdup( content_location );
		}
		else
		{
			/* its not a mms notify. its a mms delivery indication */
			IFS_Strncpy( mms->subject, FS_Text(FS_T_MMS_DELIVERY_IND), FS_MMS_SUB_LEN - 1 );
		}
		FS_MmsAddHead( mms );
		FS_MmsSaveHeadList( );

		/* rebuild mms list */
		win = FS_WindowFindId( FS_W_MmsListFrm );
		if( win && win->private_data == FS_MMS_INBOX )
		{
			FS_MmsListBuild( win, FS_MMS_INBOX );
			repaint = FS_TRUE;
		}
		/* update mms main win if amy */
		win = FS_GetTopFullScreenWindow( );
		if( win && win->id == FS_W_MmsMainFrm )
		{
			repaint = FS_TRUE;
		}

		if( repaint ) FS_GuiRepaint( );
		/* notify user */
		IFS_MmsNotify( FS_MmsIsFull(size), (FS_BOOL)(size > IFS_GetMaxMmsSize()), mms->address, mms->subject );
	}
}

/* 
	when mms auto retrive. this mean s auto retrive ok.
	file is a rel path of FS_DIR_MMS
*/
void FS_MmsAutoRecvComplete( FS_BOOL success, FS_CHAR *message_location, FS_UINT4 param )
{
	FS_UINT1 mbox;
	FS_CHAR *str, sdate[32], *file;
	FS_Window *win;
	FS_Widget *wgt;
	FS_MmsEncHead head;
	FS_MmsHead *mms = FS_MmsFindByMsgLocation( message_location );

	if( ! success )
	{
		IFS_MmsAutoRecvNotify( FS_FALSE, FS_NULL, FS_NULL );
		return;
	}

	file = (FS_CHAR *)param;
	if( mms == FS_NULL && success )
	{
		/* may have deleted by user. */
		FS_FileDelete( FS_DIR_MMS, file );
		return;
	}
	
	FS_MMS_CLR_NTF( mms );
	
	IFS_Strcpy( mms->file, file );
	IFS_Free( mms->msg_location );
	mms->msg_location = FS_NULL;
	
	FS_MmsCodecDecodeFileHead( &head, file );
	FS_MmsAddrToUserFormat( mms->address, sizeof(mms->address), head.from, -1 );
	if( head.subject )
		IFS_Strncpy( mms->subject, head.subject, sizeof(mms->subject) - 1 );
	if( head.read_report == FS_MMS_H_V_READ_REPORT_YES )
		FS_MMS_SET_READ_REPORT(mms);
	if( head.message_id )
		IFS_Strncpy( mms->msg_id, head.message_id, FS_MMS_MSG_ID_LEN - 1 );
	mms->msg_size = FS_FileGetSize( FS_DIR_MMS, file );
	if( head.date > 0 )
		FS_SecondsToDateTime( &mms->date, head.date, IFS_GetTimeZone() );

	FS_MmsCodecFreeHead( &head );
	FS_MmsSaveHeadList( );

	/* update list item if any */
	win = FS_WindowFindId( FS_W_MmsListFrm );
	if( win )
	{
		mbox = (FS_UINT1)win->private_data;
		if( mbox == FS_MMS_INBOX )
		{
			wgt = FS_WindowGetWidgetByPrivateData( win, (FS_UINT4)mms );
			FS_ASSERT( wgt != FS_NULL );			
			if( wgt )
			{
				if( mms->subject[0]  )
					str = mms->subject;
				else
					str = FS_Text( FS_T_NO_SUBJECT );
			
				FS_WidgetSetHandler( wgt, FS_MmsView_CB );
				FS_WidgetSetText( wgt, str );
				FS_DateStruct2DispStrShortForm( sdate, &mms->date );
				FS_COPY_TEXT( wgt->sub_cap, sdate );
				FS_WidgetSetIcon( wgt, FS_I_NEW_MSG );
				if( FS_WindowIsTopMost(FS_W_MmsListFrm) ) 
					FS_RedrawWidget( win, wgt );
			}
		}
	}
	/* update mms main win if amy */
	win = FS_GetTopFullScreenWindow( );
	if( win && win->id == FS_W_MmsMainFrm )
	{
		FS_GuiRepaint( );
	}
	/* notify user */
	IFS_MmsAutoRecvNotify( FS_TRUE, mms->address, mms->subject );
}

FS_BOOL FS_MmsHasUnread( void )
{
	FS_SINT4 total, unread;
	FS_MmsGetFolderDetail( FS_MMS_INBOX, &unread, &total );
	return (FS_BOOL)(unread > 0);
}

void FS_MmsSendByMms( FS_CHAR *subject, FS_CHAR *to, FS_CHAR *text, FS_CHAR *file )
{
	FS_CHAR *mime;
	FS_Widget *wgt;
	
	FS_MmsSysInit( );
	FS_ActiveApplication( FS_APP_MMS );

	if( FS_MmsIsFull( 1024 ) )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_SPACE_IS_FULL), FS_NULL, FS_FALSE );
		return;
	}
	
	FS_MmsEdit_UI( FS_NULL );

	if( subject ){
		FS_MmsEditFormSaveSubject( subject );
		wgt = FS_WindowGetWidget( FS_WindowFindId(FS_W_MmsEditFrm), FS_W_MmsEditSubject );
		if( wgt ) FS_WidgetSetText( wgt, subject );
	}
	if( to ){
		FS_MmsEditFormSetToAddr( to );
	}
	
	if( text ){
		FS_MmsEditFormAddText_CB( text );
		wgt = FS_WindowGetWidget( FS_WindowFindId(FS_W_MmsEditFrm), FS_W_MmsEditText );
		if( wgt ) FS_WidgetSetText( wgt, text );
	}

	mime = FS_GetMimeFromExt( file );
	if( IFS_Strnicmp( mime, "image", 5) == 0 ){
		FS_MmsAddImageHandle( file, FS_NULL );
	}else if( IFS_Strnicmp( mime, "audio", 5) == 0 ){
		FS_MmsAddAudioHandle( file, FS_NULL );
	}else if( IFS_Strnicmp( mime, "video", 5) == 0 ){
		FS_MmsAddVideoHandle( file, FS_NULL );
	}else{
		if( file ) FS_MessageBox(FS_MS_ALERT, FS_Text(FS_T_FILE_NOT_SUPPORT), FS_NULL, FS_FALSE );
	}
}

void FS_MmsOpenInbox( void )
{
	FS_MmsSysInit( );
	FS_ActiveApplication( FS_APP_MMS );
	
	FS_MmsList_UI( FS_NULL );
}

#endif	//FS_MODULE_MMS


