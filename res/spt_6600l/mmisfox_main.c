#define _MMISFOX_MAIN_C_  

#if 1//def SFOX_SUPPORT

#define SFOX_USE_NATIVE_DRAW_CHAR

/**---------------------------------------------------------------------------*
 **                         Dependencies                                      *
 **---------------------------------------------------------------------------*/
#include "std_header.h"
#include "sci_types.h"
#include "window_parse.h"
#include "mmk_app.h"
#include "guitextbox.h"
#include "guilcd.h"
#include "guifont.h"
#include "gui_lcd_ref.h"
#include "mmipub.h"
#include "mmiacc_id.h"
#include "mmk_timer.h"
#include "sig_code.h"
#include "app_tcp_if.h"
#include "mmiset.h"
#include "Image_proc.h"
#include "mmifmm.h"
#include "mmifmm_interface.h"
#include "mmi_appmsg.h"
#ifdef SFOX_USE_NATIVE_DRAW_CHAR
#include "spml_api.h"
#endif

#include "mmidisplay_data.h"
#include "im.h"
#include "guieditbox.h"
#include "mmibrowser.h"
#include "mmidc.h"

#include "FS_Export.h"
#include "FS_Inte.h"
#include "FS_ISocket.h"
#include "FS_IFile.h"
#include "FS_ISystem.h"

typedef struct T_SfoxMsg_Tag
{
	FS_UINT2 msg;
	FS_UINT4 param;
}T_SfoxMsg;

typedef struct FS_ITimerData_Tag
{
	FS_UINT4			id;
	void *				param;
	FS_TimerHandler 	handle;
}FS_ITimerData;

#define SFOX_FONT				SONG_FONT_20
#define MSG_APP_SFOX			0xe0e0
#define IFS_TIMER_COUNT			10
#define SFOX_EDIT_DEFAULT_LEN	512

#define SFOX_APP_STS_STOPPED		0
#define SFOX_APP_STS_TOP_VISIBLE	1
#define SFOX_APP_STS_SUSPENDED		2

static int GSFOX_AppStatus = SFOX_APP_STS_STOPPED;

static FS_ITimerData IGFS_TimerTable[IFS_TIMER_COUNT];

extern FS_SINT4 FS_CnvtUtf8ToUcs2( const FS_CHAR* utf8, FS_SINT4 utf8len, FS_UINT2 *ucs, FS_SINT4 ucslen );

extern FS_SINT4 FS_CnvtUcs2ToUtf8( FS_WCHAR *ucs2, FS_SINT4 ucs2len, FS_CHAR *utf8, FS_SINT4 utf8_len );

extern void FS_MemMove( void *dst, void *src, FS_UINT4 size );

extern FS_SINT4 FS_FileRead( FS_SINT4 dir, FS_CHAR *filename, FS_SINT4 offset, void *buf, FS_SINT4 blen );

extern FS_SINT4 FS_FileGetSize( FS_SINT4 dir, FS_CHAR *filename );

extern FS_SINT4 FS_FileWrite( FS_SINT4 dir, FS_CHAR *filename, FS_SINT4 offset, void *buf, FS_SINT4 blen );

extern FS_BOOL FS_PushClipRect( FS_Rect *rect, FS_UINT1 cType );

extern void FS_PopClipRect( void );

// TODO logs
#if 0
extern void FS_DebugPrintLog( FS_CHAR *fmt, ... );
#undef SCI_TRACE_LOW
#define SCI_TRACE_LOW	FS_DebugPrintLog
#endif

#define FS_FONT_API_ADAPTER
/************************************************************************
* FONT API adapter 
*************************************************************************/
#ifdef SFOX_USE_NATIVE_DRAW_CHAR
FS_SINT4 IFS_DrawChar(FS_SINT4 x, FS_SINT4 y, FS_UINT2 nChar, 
							FS_UINT2 attr, FS_COLOR fg, FS_COLOR bg )
{
	static FS_BYTE *buffer_ptr = FS_NULL;
	
	SPML_TEXT_METRICS_T metrics = {0};
	SPML_TEXT_PARAM_T param = {0};
	FS_SINT4 width = 0;
	wchar wstr_ptr[2] = {0};
	
	if (buffer_ptr == FS_NULL)
	{
		buffer_ptr = IFS_GetScreenBitmap( );
	}

	if ( x >= 239 || y >= 319 || 
		x + IFS_GetCharWidth(nChar) < 0 || y + IFS_GetCharHeight(nChar) < 0 )
	{
		SCI_TRACE_LOW("IFS_DrawChar ERROR. x=%d, y=%d", x, y);
		return 0;
	}
	
	param.font_color = (SPML_COLOR_T)fg;
	param.bg_color = bg;
	param.size = (SPML_TEXT_SIZE_T)SFOX_FONT;
	param.buffer_width = 240;
	param.buffer_height = 320;
	param.buffer_ptr = (SPML_COLOR_T *)buffer_ptr;

	param.display_rect.left = x;
	param.display_rect.top = y;
	param.display_rect.right = 239;
	param.display_rect.bottom = 319;

	param.clip_rect.left = x;
	param.clip_rect.top = y;
	param.clip_rect.right = 239;
	param.clip_rect.bottom = 319;

	wstr_ptr[0] = nChar;
	SPMLAPI_DrawText( &param, wstr_ptr, 1, &metrics );
	width = metrics.width;
	return (FS_SINT4)(width & 0xFFFF);
}

#else	// not SFOX_USE_NATIVE_DRAW_CHAR

FS_BYTE *IFS_GetCharBitmap( FS_UINT2 nChar )
{
	uint16 txtptr[2] = {0};

	txtptr[0] = nChar;
	return (FS_BYTE *)GUI_GetFontArray( (wchar *)txtptr, 1, SFOX_FONT );
}

#endif	//SFOX_USE_NATIVE_DRAW_CHAR

FS_SINT4 IFS_GetCharWidth( FS_UINT2 nChar )
{
	FS_SINT4 w = 0;
	w = (FS_SINT4)GUI_GetFontWidth( SFOX_FONT, nChar );
	return (w & 0xFF);
}

FS_SINT4 IFS_GetCharHeight( FS_UINT2 nChar )
{
	FS_SINT4 h = 0;
	h = (FS_SINT4)GUI_GetFontHeight( SFOX_FONT, nChar );
	return (h & 0xFF);
}

FS_SINT4 IFS_GetFontMaxHeight( void )
{
	return (FS_SINT4)(SFOX_FONT + 2);
}

#define FS_IMAGE_API_ADAPTER
/************************************************************************
* IMAGE API adapter 
*************************************************************************/
#define		SFOX_IMG_UNKNOW 	0
#define 	SFOX_IMG_BMP		1
#define 	SFOX_IMG_PNG		2
#define		SFOX_IMG_JPG		3
#define		SFOX_IMG_GIF		4

#define		SFOX_BPP			2

static BOOLEAN GSFOX_IsSFOXInUsing;

static FS_UINT4 SFOX_ReadUINT4(unsigned char* pData)
{
	FS_UINT4 tmp = 0;
	tmp += pData[3]; tmp = tmp<<8; 
	tmp += pData[2]; tmp = tmp<<8; 
	tmp += pData[1]; tmp = tmp<<8; 
	tmp += pData[0]; 
	return tmp;
}

/* big endian order */
static FS_UINT4 SFOX_ReadUINT4BE(unsigned char* pData)
{
	FS_UINT4 tmp = 0;
	tmp += pData[0]; tmp = tmp<<8; 
	tmp += pData[1]; tmp = tmp<<8; 
	tmp += pData[2]; tmp = tmp<<8; 
	tmp += pData[3]; 
	return tmp;
}

static FS_UINT2 SFOX_ReadUINT2(unsigned char* pData)
{
	unsigned short tmp = 0;
	tmp += pData[1]; tmp = tmp<<8; 
	tmp += pData[0]; 
	return tmp;
}

static int SFOX_GetImageInfo( unsigned char* data, int size, int *w, int *h)
{
	int i = 0, tmp = 0;
	
	if(w != 0)
		*w = 0;
	if(h != 0)
		*h = 0;
	if(data==0||size<4){
		return SFOX_IMG_UNKNOW;
	}
	if(data[0] == 'B' && data[1] == 'M'){  //bmp 
		if(size<54){
			return SFOX_IMG_UNKNOW;
		}
		if(w!=0){
			*w = SFOX_ReadUINT4(&data[18]);
		}
		if(h!=0){
			*h = SFOX_ReadUINT4(&data[22]);
		}
		return SFOX_IMG_BMP;
	} 
	else if(data[0]==0x89&&data[1]==0x50&&data[2]==0x4E&&data[3]==0x47&&
		data[4]==0x0d&&data[5]==0x0a&&data[6]==0x1a&&data[7]==0x0a){ // png
		if(w==0&&h==0){
			return SFOX_IMG_PNG;			
		}	
		i=8;
		// png is converted
		while(i+12<=size){
			tmp = SFOX_ReadUINT4BE(&data[i]);
			if(i+12+tmp>size){
				return SFOX_IMG_UNKNOW;
			}
			i += 4;
			if(data[i]==0x49&&data[i+1]==0x48&&data[i+2]==0x44&&data[i+3]==0x52&&tmp>8){
				if(w!=0){
					*w = SFOX_ReadUINT4BE(&data[i+4]);
				}
				if(h!=0){
					*h = SFOX_ReadUINT4BE(&data[i+8]);
				}
				return SFOX_IMG_PNG;
			}
			i += 8+tmp;
		}
		return SFOX_IMG_UNKNOW;
	}
	else if(data[0] == 'G' && data[1] == 'I' && data[2] == 'F'){ // gif
		if(size<13){
			return SFOX_IMG_UNKNOW;
		}
		if(w!=0){
			*w = SFOX_ReadUINT2(&data[6]);
		}
		if(h!=0){
			*h = SFOX_ReadUINT2(&data[8]);
		}
		return SFOX_IMG_GIF;
	}
	else if(data[0]==0xFF&&data[1]==0xD8){
		if(w==0&&h==0){
			return SFOX_IMG_JPG;			
		}	
		i = size-2;		
		while(i>0){
			i--; 			
			if(data[i] < 0xC0) continue; 
			if(data[i] > 0xC3) continue; 
			i--; 
			if(data[i] != 0xFF) continue; 
			i+=5; 
			break; 
		}
		if(i<5||i+4>size){
			return SFOX_IMG_UNKNOW;
		}	
		if(w!=0){
			*w = data[i+2]*256+data[i+3];
		}
		if(h!=0){
			*h = data[i]*256+data[i+1];
		}	
		return SFOX_IMG_JPG;			
	}
	
	return SFOX_IMG_UNKNOW;	
}

BOOLEAN SFOX_IsSFOXInUsing( void )
{
	return GSFOX_IsSFOXInUsing;
}

void SFOX_NotifyGifDecodeOneFrame(IMGPROC_GIF_DISPLAY_INFO_T   *display_info_ptr)
{
	(void)display_info_ptr;
}


BOOLEAN SFOX_IsContinueDecodeGif(void)
{
    return FALSE;
}

/*
get image size. width and height
file	[in] 	image file
w	    [out]	image width
h		[out]	image height

  return FS_TRUE if success.
*/
FS_BOOL IFS_ImageGetSize( FS_CHAR *file, FS_SINT4 *w, FS_SINT4 *h )
{
	uint8 *data;
	FS_SINT4 size = FS_FileGetSize( -1, file );
	if ( size > 0 )
	{
		data = SCI_ALLOC( size );
		if ( data )
		{
			FS_FileRead(-1, file, 0, data, size );
			SFOX_GetImageInfo(data, size, w, h);
			SCI_FREE( data );
			SCI_TRACE_LOW("IFS_ImageGetSize file=%s, w=%d, h=%d", file, *w, *h );
			return FS_TRUE;
		}
	}
	return FS_FALSE;
}

/*
decode image base on require width and height
file	[in] 	image file
w	    [in]	required width
h		[in]	required height
pBmp	[out]	decode result

  return a image handle, 0 on failed.
*/
FS_UINT4 IFS_ImageDecode( FS_CHAR *file, FS_SINT4 w, FS_SINT4 h, FS_Bitmap *pBmp )
{
	int imgType;
	uint8 *data = NULL;
	uint8 *pBits = NULL;
	GUIREF_IMG_DATA_T img_data = {0};
	BOOLEAN isJpeg = FALSE;
	BOOLEAN is_need_save_buf = FALSE;
	BOOLEAN bDecodeResult = FALSE;
	uint32 img_w = 0, img_h = 0;
	FS_UINT4 hImage = 0;
	uint16 total_frame_ptr = 1;
	
	FS_SINT4 size = FS_FileGetSize( -1, file );
	if ( size <= 0 )
	{
		SCI_TRACE_LOW("IFS_ImageDecode ERROR file=%s, size=%d", file, size );
		return 0;
	}
	data = SCI_ALLOC( size );
	if ( data == NULL )
	{
		SCI_TRACE_LOW("IFS_ImageDecode SCI_ALLOC( %d ) ERROR", size );
		return 0;
	}
	
	FS_FileRead(-1, file, 0, data, size );
	imgType = SFOX_GetImageInfo( data, size, &img_w, &img_h );
	if (imgType == SFOX_IMG_JPG )
		isJpeg = TRUE;
	else if (imgType == SFOX_IMG_GIF || imgType == SFOX_IMG_PNG)
		is_need_save_buf = TRUE;
	
	pBits = SCI_ALLOC( w * h * SFOX_BPP );
	if ( pBits == NULL )
	{
		SCI_TRACE_LOW("IFS_ImageDecode SCI_ALLOC( %d * %d * 2 ) ERROR", w, h );
		goto ERR_CATCH;
	}
	
	img_data.src_buf_ptr = data,
	img_data.src_data_size = size;
	
	if(!GUIREF_AllocMemory(isJpeg, TRUE, TRUE, is_need_save_buf) )
	{
		SCI_TRACE_LOW("IFS_ImageDecode GUIREF_AllocMemory ERROR" );
		goto ERR_CATCH;
	}
	
	memset( pBits, 0, w * h * SFOX_BPP );
	
	if( SFOX_IMG_PNG == imgType )
	{
		 bDecodeResult = GUIREF_DecodePng( pBits, 
										 	NULL, 
										 	w * h * SFOX_BPP,
										 	w,
										 	h,
										 	&img_w,
										 	&img_h,
										 	NULL,
										 	&img_data);
	}
	else if ( SFOX_IMG_JPG == imgType )
	{
		bDecodeResult = GUIREF_DecodeJpg( pBits, 
											NULL, 
											w * h * SFOX_BPP,
											w, 
											h, 
											&img_w, 
											&img_h, 
											NULL, 
											&img_data);
	}
	else if ( SFOX_IMG_BMP == imgType )
	{
		bDecodeResult = GUIREF_DecodeBmpWbmp( TRUE,
											pBits, 
											NULL, 
											w * h * SFOX_BPP,
											w, 
											h, 
											&img_w, 
											&img_h, 
											NULL, 
											&img_data);
	}
	else if ( SFOX_IMG_GIF == imgType )
	{
		GSFOX_IsSFOXInUsing = TRUE;
		bDecodeResult = GUIREF_DecodeGif( pBits, 
										NULL, 
										&total_frame_ptr,
										w * h * SFOX_BPP, 
										w,
										h,
										&img_data);
		GSFOX_IsSFOXInUsing = FALSE;
	}
	else
	{
		SCI_TRACE_LOW("IFS_ImageDecode ERROR. unsupport image type. file=%s, type=%d", file, imgType );
	}
	
	SCI_TRACE_LOW("IFS_ImageDecode file=%s, w=%d, h=%d, Result=%d", file, w, h, bDecodeResult );

	if ( bDecodeResult )
	{
		pBmp->width = w;
		pBmp->height = h;
		pBmp->bpp = SFOX_BPP;
		pBmp->pitch = w * SFOX_BPP;
		pBmp->bits = pBits;
		
		hImage = (FS_UINT4)pBits;	/* just for identifier */
		pBits = NULL;
	}
	GUIREF_FreeEncOrDecBuf();
ERR_CATCH:
	if ( data ) SCI_FREE( data );
	if ( pBits ) SCI_FREE( pBits );
	return hImage;
}

void IFS_ImageRelease( FS_UINT4 hImage, FS_Bitmap *pBmp )
{
	(void)hImage;
	if (pBmp && pBmp->bits)
	{
		SCI_FREE(pBmp->bits);
		pBmp->bits = NULL;
	}
}

void IFS_ImageDestroy( FS_UINT4 hImage )
{
	(void)hImage;
}

#define FS_SOCKET_API_ADAPTER
/************************************************************************
* SOCKET API adapter	
*************************************************************************/
#define SFOX_GPRS_STATE_NONE				0
#define SFOX_GPRS_STATE_CONNECTING			1
#define SFOX_GPRS_STATE_CONNECTED			2
#define SFOX_GPRS_STATE_DISCONNECTING		3

#define SFOX_SOCK_STATE_NONE				0
#define SFOX_SOCK_STATE_CONNECTING			1
#define SFOX_SOCK_STATE_CONNECTED			2

#define SFOX_SOCKET_CONNECT_TIME			200	//500ms
#define SFOX_SOCKET_POLL_TIME				500

static int GSFOX_SocketId;
static int GSFOX_GprsState;
static FS_UINT4 GSFOX_GprsTimerId;

static FS_UINT4 GSFOX_SocketTimerId;
static FS_ISockEvHandler GSFOX_SocketEventCB;
static int GSFOX_SocketState;
static int GSFOX_TimeoutCount;
static BOOLEAN GSFOX_NativeBrowserActive;

#if 0//def WIN32
int sci_sock_socket(int family, int type, int proto)
{
	return 1;
}

int sci_sock_connect(long s, 
				 struct sci_sockaddr * addr,
				 int addrlen)
{
	return -1;
}

struct sci_hostent * sci_gethostbyname(char *name)
{
	static struct sci_hostent hostip = {0};
	return &hostip;
}

int
sci_sock_errno(long s)
{
	return 22;
}

int
sci_sock_setsockopt(long s, 
					int   name,
					void *   arg)
{
	return 0;
}


void
SCI_FD_SET(long sock, sci_fd_set * set)
{

}

int   /* actually, boolean */
SCI_FD_ISSET(long sock, sci_fd_set * set)
{
	return 1;
}

void   
SCI_FD_ZERO(sci_fd_set * set)
{

}

int
sci_sock_select(sci_fd_set * in,   /* lists of sockets to watch */
				sci_fd_set * out,
				sci_fd_set * ex,
				long  tv)   /* ticks to wait */
{
	return 1;
}

int
sci_sock_send(long s, 
			  char *   buf,
			  int      len, 
			  int      flags)
{
	return len;
}

int
sci_sock_recv (long s, 
			   char *   buf,
			   int   len, 
			   int   flag)
{
	return len;
}

int
sci_sock_socketclose(long s)
{
	return 0;
}

#endif

MMI_RESULT_E SFOX_NetConnCallback( uint16 msg_id, DPARAM param ){

	MMI_GPRS_T *ret = (MMI_GPRS_T *)param;

	if( GSFOX_NativeBrowserActive ) {
		SCI_TRACE_LOW("SFOX_NetConnCallback return false because native browser is active");
		return MMI_RESULT_FALSE;
	}
	
	if( GSFOX_GprsState == SFOX_GPRS_STATE_NONE){
		SCI_TRACE_LOW("SFOX_NetConnCallback return false because AppIsNotActive");
		return MMI_RESULT_FALSE;
	}
	
	switch( msg_id )
	{
	case APP_MN_ACTIVATE_PDP_CONTEXT_CNF:
		if( GSFOX_GprsTimerId != 0 ){
			IFS_StopTimer( GSFOX_GprsTimerId );
			GSFOX_GprsTimerId = 0;
		}
		if( ret->result == MN_GPRS_ERR_SUCCESS ){
			GSFOX_GprsState = SFOX_GPRS_STATE_CONNECTED;
			FS_NetConnResultInd( FS_TRUE );
		}else{
			GSFOX_GprsState = SFOX_GPRS_STATE_NONE;
			FS_NetConnResultInd( FS_FALSE );
		}
		break;
	case APP_MN_DEACTIVATE_PDP_CONTEXT_IND:	
	case APP_MN_DEACTIVATE_PDP_CONTEXT_CNF:
		if( GSFOX_GprsTimerId != 0 ){
			IFS_StopTimer( GSFOX_GprsTimerId );
			GSFOX_GprsTimerId = 0;
		}
		GSFOX_GprsState = SFOX_GPRS_STATE_NONE;
		if ( msg_id == APP_MN_DEACTIVATE_PDP_CONTEXT_IND )
			FS_NetConnResultInd( FS_FALSE );
		break;
	default:
		break;
	}
	
	SCI_TRACE_LOW("SFOX_NetConnCallback msg_id = %d, result = %d, GprsState=%d", msg_id, ret->result, GSFOX_GprsState );
		
	return MMI_RESULT_TRUE;
}

static void SFOX_SocketTimerCallback( void * param )
{
	FS_UINT4 sockid = (FS_UINT4)param;
	sci_fd_set writefds, readfds, errfds; 
	int ret;

	if ( GSFOX_SocketId != sockid )
	{
		SCI_TRACE_LOW("SFOX_SocketTimerCallback wrong timer callback. sockid=%d, active sockid=%d", sockid, GSFOX_SocketId);
		return;
	}

	GSFOX_SocketTimerId = 0;

	if ( GSFOX_SocketState == SFOX_SOCK_STATE_CONNECTING )
	{
		GSFOX_TimeoutCount ++;
		if (GSFOX_TimeoutCount > 600)	// 600 * 100 ms = 60s, connect timeout
		{
			if (GSFOX_SocketEventCB)
			{
				GSFOX_SocketEventCB( sockid, FS_ISOCK_ERROR, 0 );
			}
			return;
		}
	}
		
	SCI_FD_ZERO( &writefds ); 
	SCI_FD_SET( sockid, &writefds ); 
	SCI_FD_ZERO( &readfds );
	SCI_FD_SET( sockid, &readfds );
	SCI_FD_ZERO( &errfds );
	SCI_FD_SET( sockid, &errfds );
	
	ret = sci_sock_select( &readfds, &writefds, &errfds, 0 );
	if ( ret == -1 )
	{
		SCI_TRACE_LOW( "SFOX_SocketTimerCallback sockid = %d, sci_sock_select ret = %d", sockid, ret );
		if (GSFOX_SocketEventCB)
		{
			GSFOX_SocketEventCB( sockid, FS_ISOCK_ERROR, 0 );
		}
		return;
	}

	if ( SCI_FD_ISSET( sockid, &errfds ) )
	{
		//test if we got error
		int err_no = sci_sock_errno( sockid );
		SCI_TRACE_LOW( "SFOX_SocketTimerCallback socket error happened. sockid=%d, err=%d", sockid, err_no );		
		if (GSFOX_SocketEventCB)
		{
			GSFOX_SocketEventCB( sockid, FS_ISOCK_ERROR, 0 );
		}
		return;
	}

	if ( SCI_FD_ISSET( sockid, &writefds ) )
	{	
		//test if we can send our request
		if ( SFOX_SOCK_STATE_CONNECTING == GSFOX_SocketState )
		{
			SCI_TRACE_LOW( "SFOX_SocketTimerCallback sockid=%d, dur=%d ms", sockid, GSFOX_TimeoutCount * SFOX_SOCKET_CONNECT_TIME );
			GSFOX_TimeoutCount = 0;			//clear timeout
			GSFOX_SocketState = SFOX_SOCK_STATE_CONNECTED;
			if (GSFOX_SocketEventCB)
			{
				GSFOX_SocketEventCB( sockid, FS_ISOCK_CONNECT, 0 );
			}
		}
		else if ( SFOX_SOCK_STATE_CONNECTED == GSFOX_SocketState )
		{
			if (GSFOX_SocketEventCB)
			{
				GSFOX_SocketEventCB( sockid, FS_ISOCK_SENDOK, 0 );
			}
		}
	}

	if ( SCI_FD_ISSET( sockid, &readfds ) )
	{
		//test if we got data to recieve
		SCI_TRACE_LOW( "SFOX_SocketTimerCallback socket can read. sockid=%d", sockid );
		if ( GSFOX_SocketState == SFOX_SOCK_STATE_CONNECTING )
		{
			SCI_TRACE_LOW( "SFOX_SocketTimerCallback connecting time is = %d ms", GSFOX_TimeoutCount * SFOX_SOCKET_CONNECT_TIME );
			GSFOX_TimeoutCount = 0;			//clear timeout
			GSFOX_SocketState = SFOX_SOCK_STATE_CONNECTED;
			if (GSFOX_SocketEventCB)
			{
				GSFOX_SocketEventCB( sockid, FS_ISOCK_CONNECT, 0 );
			}
		}

		if (GSFOX_SocketEventCB)
		{
			GSFOX_SocketEventCB( sockid, FS_ISOCK_READ, 0 );
		}
	}

	if (GSFOX_SocketState == SFOX_SOCK_STATE_CONNECTING)
		GSFOX_SocketTimerId = IFS_StartTimer( 0, SFOX_SOCKET_CONNECT_TIME, SFOX_SocketTimerCallback, (void *)sockid );
	else
		GSFOX_SocketTimerId = IFS_StartTimer( 0, SFOX_SOCKET_POLL_TIME, SFOX_SocketTimerCallback, (void *)sockid );
}

static void SFOX_NetTimerCallback( void * dummy )
{
	int state = GSFOX_GprsState;

	(void)dummy;
	SCI_TRACE_LOW("SFOX_NetTimerCallback GprsState=%d", GSFOX_GprsState );
	GSFOX_GprsTimerId = 0;
	GSFOX_GprsState = SFOX_GPRS_STATE_NONE;
	if (state == SFOX_GPRS_STATE_CONNECTING)
	{
		FS_NetConnResultInd( FS_FALSE );
	}
}

FS_BOOL IFS_NetConnect( FS_CHAR *apn, FS_CHAR *user, FS_CHAR *pass )
{
	ERR_MNGPRS_CODE_E ret;
	MN_DUAL_SYS_E dual_sys = MMIAPISET_GetActiveSim();
	
	SCI_TRACE_LOW("IFS_NetConnect apn = %s, GprsState=%d", apn, GSFOX_GprsState );
	if (GSFOX_GprsState != SFOX_GPRS_STATE_NONE)
	{
		FS_NetConnResultInd( FS_FALSE );
		return FS_FALSE;
	}
	
	MNGPRS_SetPdpContextPcoEx(dual_sys, 1, (uint8 *)user, (uint8 *)pass );
	ret = MNGPRS_SetAndActivePdpContextEx(dual_sys, (uint8 *)apn );
	if( ERR_MNGPRS_NO_ERR ==  ret ){
		GSFOX_GprsState = SFOX_GPRS_STATE_DISCONNECTING;
		GSFOX_GprsTimerId = IFS_StartTimer( 0, 60000, SFOX_NetTimerCallback, NULL );
		return FS_TRUE;
	}else{
		FS_NetConnResultInd( FS_FALSE );
		return FS_FALSE;
	}
}

void IFS_NetDisconnect( void )
{
	MN_DUAL_SYS_E dual_sys = MMIAPISET_GetActiveSim();
	SCI_TRACE_LOW( "IFS_NetDisconnect GprsState = %d", GSFOX_GprsState );
	if( GSFOX_GprsState == SFOX_GPRS_STATE_CONNECTED || GSFOX_GprsState == SFOX_GPRS_STATE_CONNECTING ){
		MNGPRS_ResetAndDeactivePdpContextEx( dual_sys );
		GSFOX_GprsState = SFOX_GPRS_STATE_DISCONNECTING;
		GSFOX_GprsTimerId = IFS_StartTimer( 0, 5000, SFOX_NetTimerCallback, NULL );
		SCI_Sleep(200);
	}
}

FS_BOOL IFS_SocketCreate( FS_UINT4 *sockid, FS_BOOL bTCP, FS_ISockEvHandler handler )
{
	int sid = sci_sock_socket( AF_INET, SOCK_STREAM, 0 );

	(void)bTCP;
	SCI_TRACE_LOW("IFS_SocketCreate socket id = %d, state=%d", sid, GSFOX_SocketState);
	if ( sid > 0 )
	{
		GSFOX_SocketEventCB = handler;
		*sockid = sid;
		GSFOX_SocketId = sid;
		return FS_TRUE;
	}
	else
	{
		return FS_FALSE;
	}
}

static int SFOX_ConvertToIPAddr( char *host, unsigned char *ipAddr )
{
	ipAddr[0] = (unsigned char)atoi(host);
	if ( ipAddr[0] == 0 ) {
		return 0;
	}
	host = strchr(host, '.');
	if ( host == FS_NULL )
	{
		return 0;
	}
	host ++;
	ipAddr[1] = (unsigned char)atoi(host);
	host = strchr(host, '.');
	if ( host == FS_NULL )
	{
		return 0;
	}
	host ++;
	ipAddr[2] = (unsigned char)atoi(host);
	host = strchr(host, '.');
	if (host == FS_NULL)
	{
		return 0;
	}
	host ++;
	ipAddr[3] = (unsigned char)atoi(host);
	return 1;
}



FS_BOOL IFS_SocketConnect( FS_UINT4 sockid, FS_CHAR *host, FS_UINT2 port )
{
	struct sci_hostent *hostip = NULL;
	struct sci_sockaddr him = {0};
	int ret, err_no = 0;
	uint8 uint8_buf[4] = {0};
	
	SCI_TRACE_LOW("IFS_SocketConnect sockid = %d, host = %s, port = %d, state = %d", sockid, host, port, GSFOX_SocketState);
	
	if( sockid == 0 )
	{
		return FS_FALSE;
	}
	
	ret = sci_sock_setsockopt((TCPIP_SOCKET_T)sockid, SO_NBIO, 0);
	if( ret < 0 )
	{
		err_no = sci_sock_errno( sockid );
		SCI_TRACE_LOW( "IFS_SocketConnect setsockopt error. sockid = %d, err_no = %d", sockid, err_no );
		return FS_FALSE;
	}
	
	him.family = AF_INET;
	him.port = (uint16)htons( port );

	if ( SFOX_ConvertToIPAddr( host, uint8_buf ) )
	{
		memcpy( &(him.ip_addr), uint8_buf, 4);
	}
	else
	{
		hostip = sci_gethostbyname( host );
		memcpy( &(him.ip_addr), hostip->h_addr_list[0], 4);
	}

	ret = sci_sock_connect((TCPIP_SOCKET_T)sockid, (struct sci_sockaddr*)&(him), sizeof(struct sci_sockaddr));
	SCI_TRACE_LOW("sci_sock_connect ret = %d", ret);
	
	if( ret < 0 ){ 
		err_no = sci_sock_errno( sockid );
		if ( 22 == err_no || 10035 == err_no ) {
			SCI_TRACE_LOW( "IFS_SocketConnect connect success with sockid = %d, now waiting for response", sockid );
		} else {
			SCI_TRACE_LOW( "IFS_SocketConnect connect error. sockid = %d, err_no = %d", sockid, err_no );
			return FS_FALSE;
		}
	}

	GSFOX_TimeoutCount = 0;
	GSFOX_SocketState = SFOX_SOCK_STATE_CONNECTING;
	GSFOX_SocketTimerId = IFS_StartTimer( 0, SFOX_SOCKET_CONNECT_TIME, SFOX_SocketTimerCallback, (void *)sockid );
	return FS_TRUE;
}

FS_SINT4 IFS_SocketSend( FS_UINT4 sockid, FS_BYTE *buf, FS_SINT4 len )
{
	int32 data_len = sci_sock_send((TCPIP_SOCKET_T)sockid, (char*)buf, len, 0);
	SCI_TRACE_LOW("IFS_SocketSend sock id=%d, len=%d", sockid, len);
	return data_len;
}

FS_SINT4 IFS_SocketSendTo( FS_UINT4 sockid, FS_BYTE *buf, FS_SINT4 len, FS_SockAddr *to )
{
	(void)sockid;
	(void)buf;
	(void)len;
	(void)to;
	// not used
	return -1;
}

FS_SINT4 IFS_SocketRecv( FS_UINT4 sockid, FS_BYTE *buf, FS_SINT4 len )
{
	len = (FS_SINT4)sci_sock_recv( (TCPIP_SOCKET_T)sockid, (char *)buf, len, 0 );
	SCI_TRACE_LOW("IFS_SocketRecv sock id=%d, len=%d", sockid, len);
	if (len <= 0 )
	{
		int err = sci_sock_errno((TCPIP_SOCKET_T)sockid);
		SCI_TRACE_LOW("IFS_SocketRecv ERROR. sockid=%d, err=%d", sockid, err);
	}
	return len;
}

FS_SINT4 IFS_SocketRecvFrom( FS_UINT4 sockid, FS_BYTE *buf, FS_SINT4 len, FS_SockAddr *from )
{
	(void)sockid;
	(void)buf;
	(void)len;
	(void)from;
	// not used
	return -1;
}

FS_BOOL IFS_SocketClose( FS_UINT4 sockid )
{
	SCI_TRACE_LOW("IFS_SocketClose sockid = %d, sock state=%d", sockid, GSFOX_SocketState);
	if ( GSFOX_SocketState != SFOX_SOCK_STATE_NONE )
	{
		GSFOX_SocketEventCB = NULL;
		GSFOX_SocketState = SFOX_SOCK_STATE_NONE;
		if (GSFOX_SocketTimerId > 0)
			IFS_StopTimer( GSFOX_SocketTimerId );
		GSFOX_SocketTimerId = 0;
		GSFOX_SocketId = 0;
		sci_sock_socketclose( (TCPIP_SOCKET_T)sockid );
		SCI_Sleep(200);
	}
	return FS_TRUE;
}

#define FS_STDLIB_API_ADAPTER
/************************************************************************
* STDLIB API adapter
*************************************************************************/
	
void * IFS_Malloc( FS_UINT4 nSize )
{
	return SCI_ALLOC( nSize );
}

/* !!! realloc must keep the content in the buffer */
void * IFS_Realloc( void *ptr, FS_UINT4 size )
{
	(void)size;
	(void)ptr;
	//DEPRECATED
	return FS_NULL;
}

void IFS_Free( void *p )
{
	SCI_FREE( p );
}

void * IFS_Memcpy( void *dst, void *src, FS_UINT4 size )
{
	memcpy(dst, src, size);
	return dst;
}

void * IFS_Memset( void *dst, int val, FS_UINT4 size )
{
	memset(dst, val, size);
	return dst;
}

void IFS_Memmove( void *dst, void *src, FS_UINT4 size )
{
	FS_MemMove( dst, src, size );
}

FS_SINT4 IFS_Memcmp( const void *m1, const void *m2, FS_UINT4 size )
{
	return memcmp( m1, m2, size );
}

FS_SINT4 IFS_Strlen( const FS_CHAR *pStr )
{
	return strlen( pStr );
}

FS_CHAR * IFS_Strcpy( FS_CHAR *dst, const FS_CHAR *src )
{
	return strcpy( dst, src );
}

FS_CHAR * IFS_Strncpy( FS_CHAR *dst, const FS_CHAR *src, FS_SINT4 len )
{
	return strncpy( dst, src, len );
}

FS_CHAR IFS_Toupper( FS_CHAR c )
{
	if (c >= 'a' && c <= 'z')
		return (FS_CHAR)(c - 0x20);
	else
		return c;
}

FS_SINT4 IFS_Strcmp( FS_CHAR *str1, FS_CHAR *str2 )
{
	return strcmp( str1, str2 );
}

FS_SINT4 IFS_Stricmp( FS_CHAR *str1, FS_CHAR *str2 )
{
	return stricmp( str1, str2 );
}

FS_SINT4 IFS_Strnicmp( FS_CHAR *str1, FS_CHAR *str2, FS_SINT4 len )
{
	return strnicmp( str1, str2, len );
}

FS_SINT4 IFS_Strncmp( FS_CHAR *str1, FS_CHAR *str2, FS_SINT4 len )
{
	return strncmp( str1, str2, len );
}

FS_CHAR * IFS_Strchr( FS_CHAR *str, FS_CHAR c )
{
	return strchr( str, c );
}

FS_CHAR * IFS_Strstr( FS_CHAR *str, FS_CHAR *substr )
{
	return strstr( str, substr );
}

FS_CHAR * IFS_Strdup( FS_CHAR *pStr )
{
	FS_SINT4 len = IFS_Strlen( pStr );
	FS_CHAR *ret = IFS_Malloc( len + 1 );
	IFS_Memcpy( ret, pStr, len + 1 );
	return ret;
}

FS_SINT4 IFS_Atoi( FS_CHAR *pStr )
{
	return atoi( pStr );
}

FS_SINT4 IFS_Sprintf( FS_CHAR *buf, FS_CHAR *fmt, ... );

FS_CHAR * IFS_Itoa( FS_SINT4 val, FS_CHAR *str, FS_SINT4 radis )
{
	(void)radis;
	IFS_Sprintf(str, "%d", val);
	return str;
}

FS_CHAR * IFS_Strcat( FS_CHAR *dst, const FS_CHAR *src)
{
	strcat( dst, src );
	return dst;
}

FS_SINT4 IFS_Rand( void )
{
	return rand( );
}

void IFS_SRand( FS_UINT4 seed )
{
	srand( seed );
}

FS_SINT4 IFS_Sprintf( FS_CHAR *buf, FS_CHAR *fmt, ... )
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsprintf(buf,fmt,args);
	va_end(args);
	return i;
}

/*
	!!! CAUTION
	@buf is @count len. Be sure not to exceed.
*/
FS_SINT4 IFS_Snprintf( FS_CHAR *buf, FS_UINT4 count, const FS_CHAR *fmt, ... )
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = _vsnprintf( buf, count, fmt, args );
	va_end(args);
	return i;
}

void IFS_GetDateTime( FS_DateTime *dt )
{
	SCI_TIME_T t = {0};
	SCI_DATE_T d = {0};
	
	TM_GetSysTime(&t);
	TM_GetSysDate(&d);
	
	dt->year = d.year;
	dt->month = d.mon;	/* 1->12 */
	dt->day = d.mday;
	dt->week_day = d.wday;
	dt->hour = t.hour;
	dt->min = t.min;
	dt->sec = t.sec;
}

FS_SINT4 IFS_GetTimeZone( void )
{
	return 8;	/* +8 zone */
}

#define FS_FILE_API_ADAPTER
/************************************************************************
* File System API adapter
*************************************************************************/
static FS_FDHandler GSFOX_FDHandler;
static void *GSFOX_FDParam;
void IFS_FileDialogOpen( FS_UINT1 filter, FS_FDHandler cb, void *param )
{
	(void)filter;
	GSFOX_FDHandler = cb;
	GSFOX_FDParam = param;
	MMIAPIFMM_OpenSelectPictureWin(
            MMIFMM_DEVICE_ALL,
            MMIFMM_PIC_ALL,
            0,
            MMIACC_SFOX_WIN_ID,
            NULL, 0
            );
}

void IFS_CameraNewPhoto( FS_FDHandler cb, void *param )
{
	SCI_TRACE_LOW("IFS_CameraNewPhoto GSFOX_IsSFOXInUsing=%d", GSFOX_IsSFOXInUsing);
	GSFOX_FDHandler = cb;
	GSFOX_FDParam = param;
	GSFOX_IsSFOXInUsing = TRUE;
	MMIAPIDC_OpenPhotoWin();
}

void SFOX_CameraNewPhotoDone( wchar *fname, uint16 fnlen, uint32 fsize )
{
	uint8 *utf8_str = (uint8 *)SCI_ALLOC( fnlen * 3 + 1 );
	memset( utf8_str, 0, fnlen * 3 + 1 );
	FS_CnvtUcs2ToUtf8( fname, fnlen, (FS_CHAR *)utf8_str, fnlen * 3 );
	SCI_TRACE_LOW("SFOX_CameraNewPhotoDone fname=%s, fnlen=%d, fsize=%d", utf8_str, fnlen, fsize );
	GSFOX_IsSFOXInUsing = FALSE;
	if (GSFOX_FDHandler)
	{
		GSFOX_FDHandler( utf8_str, GSFOX_FDParam );
		GSFOX_FDHandler = FS_NULL;
	}
	SCI_FREE(utf8_str);
}

//--------------------------------------------------------------------
// file operator interface
FS_BOOL IFS_FileCreate( FS_Handle *hFile, FS_CHAR *pFileName, FS_UINT4 aflag )
{
	FS_BOOL ret = FS_FALSE;
	SFS_HANDLE	sfs_handle = 0;
	uint16 full_path_ptr[128] = {0};

	(void)aflag;
	//open file
	FS_CnvtUtf8ToUcs2( pFileName, -1, full_path_ptr, 127 );
	sfs_handle = SFS_CreateFile( full_path_ptr, SFS_MODE_CREATE_ALWAYS | SFS_MODE_READ | SFS_MODE_WRITE, NULL, NULL);
	if (0 != sfs_handle)
	{
		*hFile = (FS_UINT4)sfs_handle;
		ret = FS_TRUE;
	}
	
	return ret;
}

//--------------------------------------------------------------------
// file operator interface
FS_BOOL IFS_FileOpen( FS_Handle *hFile, FS_CHAR *pFileName, FS_UINT4 aflag )
{
	FS_BOOL ret = FS_FALSE;
	SFS_HANDLE	sfs_handle = 0;
	uint16 full_path_ptr[128] = {0};
	
	(void)aflag;
	//open file
	FS_CnvtUtf8ToUcs2( pFileName, -1, full_path_ptr, 127 );
	sfs_handle = SFS_CreateFile( full_path_ptr, SFS_MODE_OPEN_EXISTING | SFS_MODE_READ | SFS_MODE_WRITE, NULL, NULL);
	//SCI_TRACE_LOW( "IFS_FileOpen file=%s, sfs_handle=%d", pFileName, sfs_handle );
	if (0 != sfs_handle)
	{
		*hFile = (FS_UINT4)sfs_handle;
		ret = FS_TRUE;
	}
	
	return ret;
}
/*
	return = 0 if reach to file end ; return <= 0 if error occured
	or return file readed
*/
FS_SINT4 IFS_FileRead( FS_Handle hFile, void *buf, FS_UINT4 len )
{
	SFS_ERROR_E err;
	uint32 ret = 0;
	err = SFS_ReadFile((SFS_HANDLE)hFile, buf, len, &ret, NULL);
	//SCI_TRACE_LOW( "IFS_FileRead len=%d, ret=%d, err=%d", len, ret, err );
	return (FS_SINT4)ret;
}

/*
	return <= 0 if error occured
	return file writed
*/
FS_SINT4 IFS_FileWrite( FS_Handle hFile, void *buf, FS_UINT4 len )
{
	SFS_ERROR_E err;
	uint32 ret = 0;
	err = SFS_WriteFile((SFS_HANDLE)hFile, buf, len, &ret, NULL);
	//SCI_TRACE_LOW( "IFS_FileWrite len=%d, ret=%d, err=%d", len, ret, err );
	return (FS_SINT4)ret;
}

FS_SINT4 IFS_FileSetPos( FS_Handle hFile, FS_SINT4 pos )
{
	FS_SINT4 ret = 0;
	if ( SFS_ERROR_NONE == SFS_SetFilePointer((SFS_HANDLE)hFile, pos, SFS_SEEK_BEGIN) )
	{
		ret = pos;
	}
	//SCI_TRACE_LOW( "IFS_FileSetPos pos=%d, ret=%d", pos, ret );
	return ret;
}

FS_BOOL IFS_FileClose( FS_Handle hFile )
{
	SFS_CloseFile( (SFS_HANDLE)hFile );
	return FS_TRUE;
}

FS_BOOL IFS_FileDelete( FS_CHAR *pFileName )
{
	FS_BOOL ret = FS_FALSE;
	uint16 full_path_ptr[128] = {0};
	
	FS_CnvtUtf8ToUcs2( pFileName, -1, full_path_ptr, 127 );
	if ( SFS_ERROR_NONE == SFS_DeleteFile( full_path_ptr, NULL ) )
	{
		ret = FS_TRUE;
	}
	
	return ret;
}

FS_BOOL IFS_FileMove( FS_CHAR * oldName, FS_CHAR *newName )
{
	FS_BOOL ret = FS_FALSE;
	uint16 full_path_src[128] = {0};
	uint16 full_path_dst[128] = {0};

	FS_CnvtUtf8ToUcs2( oldName, -1, full_path_src, 127 );
	FS_CnvtUtf8ToUcs2( newName, -1, full_path_dst, 127 );
	if ( SFS_ERROR_NONE == SFS_RenameFile( full_path_src, full_path_dst, NULL ) )
	{
		ret = FS_TRUE;
	}
	
	return ret;
}

FS_BOOL IFS_FileCopy( FS_CHAR * oldName, FS_CHAR *newName )
{
	(void)oldName;
	(void)newName;
	return FS_FALSE;
}

FS_SINT4 IFS_FileGetSize( FS_Handle hFile )
{
	SFS_ERROR_E err;
	uint32 size = 0;
	err = SFS_GetFileSize((SFS_HANDLE)hFile, &size);
	return (FS_SINT4)size;
}

FS_BOOL IFS_DirCreate( FS_CHAR *dir )
{
	FS_BOOL ret = FS_FALSE;
	uint16 full_path_ptr[128] = {0};
	
	//open file
	FS_CnvtUtf8ToUcs2( dir, -1, full_path_ptr, 127 );
	if ( SFS_ERROR_NONE == SFS_CreateDirectory( full_path_ptr ) )
	{
		ret = FS_TRUE;
	}
	
	return ret;
}
#define FS_CORE_API_ADAPTER
/************************************************************************
* System Core API adapter
*************************************************************************/
static FS_BOOL IFS_SaveTimerData( FS_UINT4 id, FS_TimerHandler cb, void *param )
{
	int i;
	for( i = 0; i < IFS_TIMER_COUNT; i ++ )
	{
		if( IGFS_TimerTable[i].id == 0 )
		{
			IGFS_TimerTable[i].id = id;
			IGFS_TimerTable[i].handle = cb;
			IGFS_TimerTable[i].param = param;
			return FS_TRUE;
		}
	}
	return FS_FALSE;
}

FS_ITimerData *IFS_GetTimerData( FS_UINT4 id )
{
	int i;
	for( i = 0; i < IFS_TIMER_COUNT; i ++ )
	{
		if( IGFS_TimerTable[i].id == id )
		{
			return &IGFS_TimerTable[i];
		}
	}
	return FS_NULL;
}

FS_UINT4 IFS_StartTimer( FS_UINT4 id, FS_UINT4 msecs, FS_TimerHandler cb, void *param )
{
	FS_UINT4 tid = MMK_CreateWinTimer( MMIACC_SFOX_WIN_ID, msecs, FALSE );
	(void)id;
	IFS_SaveTimerData( tid, cb, param );
	return tid;
}

void IFS_StopTimer( FS_UINT4 id )
{
	FS_ITimerData *pTD = IFS_GetTimerData( id );
	if( pTD )
	{
		pTD->id = 0;
	}
	MMK_StopTimer( (uint8)id );
}

/*
	post a message(not sendmessage). when device receive this msg, 
	must call FS_HandlePostMessage to handle the message
	@see FS_Config.h for FS_HandlePostMessage
*/
void IFS_PostMessage( FS_UINT2 msg, FS_UINT4 param )
{
	T_SfoxMsg sfoxmsg;
	
	sfoxmsg.msg = msg;
	sfoxmsg.param = param;

	if ( GSFOX_AppStatus != SFOX_APP_STS_STOPPED )
	{
		MMK_PostMsg( MMIACC_SFOX_WIN_ID, MSG_APP_SFOX, &sfoxmsg, sizeof(sfoxmsg) );
	}
	else
	{
		SCI_TRACE_LOW("IFS_PostMessage host app already exit. Call directly.");
		FS_HandlePostMessage( msg, param );
	}
}

/*--------------------------------------------------------------------------------------
		graplic library interface
----------------------------------------------------------------------------------------*/
/* 
	RGB interface
	clr is a real 24 bit RGB color, convert it to platform RGB, eg. RGB565 etc. 
	red:	(unsigned char)(clr >> 16)
	green:	(unsigned char)(clr >> 8)
	blue:	(unsigned char)clr
*/
FS_COLOR IFS_DDB_RGB( FS_BYTE r, FS_BYTE g, FS_BYTE b )
{
	return (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3)) & 0xFFFF;
}

FS_BYTE IFS_DDB_RED( FS_COLOR clr )
{
	return (FS_BYTE)((clr >> 8) & 0xF8);
}

FS_BYTE IFS_DDB_GREEN( FS_COLOR clr )
{
	return (FS_BYTE)((clr >> 3) & 0xFC);
}

FS_BYTE IFS_DDB_BLUE( FS_COLOR clr )
{
	return (FS_BYTE)((clr << 3) & 0xF8);
}

FS_BOOL IFS_SystemAllowDrawScreen( void )
{
	if ( GSFOX_AppStatus == SFOX_APP_STS_TOP_VISIBLE )
		return FS_TRUE;
	else
		return FS_FALSE;
}

void IFS_InvalidateRect( FS_Rect *pRect )
{
	FS_SINT4 right, bottom;
	FS_SINT4 left, top, width, height;
	
	if ( GSFOX_AppStatus != SFOX_APP_STS_TOP_VISIBLE )
		return;

	width = IFS_GetScreenWidth( );
	height = IFS_GetScreenHeight( );
	if ( pRect )
	{
		left = pRect->left;
		top = pRect->top;
		right = pRect->left + pRect->width;
		bottom = pRect->top + pRect->height;
	}
	else
	{
		left = 0;
		top = 0;
		right = width - 1;
		bottom = height - 1;
	}
	
	if (left < 0)
		left = 0;
	if (top < 0)
		top = 0;
	if (right > width - 1)
		right = width - 1;
	if (bottom > height - 1)
		bottom = height - 1;
	if (right <= 0 || right <= left)
		return;
	if (bottom <= 0 || bottom <= top)
		return;
	
	LCD_InvalidateRect( GUI_MAIN_LCD_ID, (uint16)left, (uint16)top, (uint16)right, (uint16)bottom ); 
}

void IFS_SystemExit( void )
{
	if ( GSFOX_AppStatus != SFOX_APP_STS_STOPPED )
		MMK_CloseWin(MMIACC_SFOX_WIN_ID);
}

FS_CHAR *IFS_GetRootDir( void )
{
	/* C: hide file system; D: UDisk; E: SD Card */
	return "E:\\sfox";
}

FS_CHAR *IFS_GetPathSep( void )
{
	return "\\";
}

FS_SINT4 IFS_GetScreenWidth( void )
{
	static FS_SINT4 width = 0;
    GUIREF_LCD_INFO_T lcd_info = {0};

	if ( width == 0 )
	{
		GUIREF_LCDGetInfo( GUI_MAIN_LCD_ID, &lcd_info );
		width = lcd_info.lcd_width;
	}
	return width;
}

FS_SINT4 IFS_GetScreenHeight( void )
{
	static FS_SINT4 height = 0;
    GUIREF_LCD_INFO_T lcd_info = {0};

	if ( height == 0 )
	{
		GUIREF_LCDGetInfo( GUI_MAIN_LCD_ID, &lcd_info );
		height = lcd_info.lcd_height;
	}
	return height;
}

FS_SINT4 IFS_GetWinTitleHeight( void )
{
	return 30;
}

FS_Bitmap *IFS_GetWinTitleBgImg( void )
{
	return FS_NULL;
}

void IFS_ReleaseWinTitleBgImg( FS_Bitmap *pBmp )
{
	(void)pBmp;
}

FS_Bitmap *IFS_GetSoftkeyBarBgImg( void )
{
	return FS_NULL;
}

void IFS_ReleaseSoftkeyBarBgImg( FS_Bitmap *pBmp )
{
	(void)pBmp;
}

FS_SINT4 IFS_GetSoftkeyHeight( void )
{
	return 29;
}

FS_SINT4 IFS_GetWidgetSpan( void )
{
	return 6;
}

FS_SINT4 IFS_GetLineHeight( void )
{
	return 29;
}

FS_SINT4 IFS_DcdGetChannelTabHeight( void )
{
	return 0;
}

FS_SINT4 IFS_DcdGetIdleDetailHeight( void )
{
	return 0;
}

FS_UINT1 IFS_GetBitsPerPixel( void )
{	
	static uint16 bits_per_pixel = 0;
    GUIREF_LCD_INFO_T lcd_info = {0};

	if ( bits_per_pixel == 0 )
	{
		GUIREF_LCDGetInfo( GUI_MAIN_LCD_ID, &lcd_info );
		bits_per_pixel = lcd_info.bits_per_pixel;
	}
	return (FS_UINT1)bits_per_pixel;
}

FS_BYTE *IFS_GetScreenBitmap( void )
{
	GUI_LCD_DEV_INFO dev = {GUI_MAIN_LCD_ID, GUI_BLOCK_MAIN};
	GUI_COLOR_T *block_buffer_ptr = PNULL;
	block_buffer_ptr = GUIBLOCK_GetBlockBuffer( &dev );

	return (FS_BYTE *)block_buffer_ptr;
}

void IFS_ReleaseScreenBitmap( FS_BYTE *pBits )
{
	(void)pBits;
}

FS_CHAR *	IFS_GetUserAgent( void )
{
	//return (FS_CHAR *)MMIBRW_GetUserProfile();
	return "MOT-Z3/AAUG2135 AA Release/07.18.2006 MIB/BER2.2 Profile/MIDP-2.0  Configuration/CLDC-1.1 EGE/1.0 Software/08.02.05R";
}

FS_CHAR * IFS_GetUaProfile( void )
{
	//return (FS_CHAR*)MMIBRW_GetUserAgent();
	return FS_NULL;
}

FS_BOOL IFS_IsInternationalRoaming( void )
{
	return FS_FALSE;
}

FS_BOOL IFS_BrowserOpenURL( FS_CHAR *url )
{
	FS_BOOL bOK;
	GSFOX_NativeBrowserActive = TRUE;
	bOK = MMICMSBRW_AccessUrl( url, MMIBRW_GetActiveSim() );
	if ( bOK )
	{
		GSFOX_GprsState = SFOX_GPRS_STATE_NONE;
		FS_NetConnResultInd( FS_FALSE );
	}
	else
	{
		GSFOX_NativeBrowserActive = FALSE;
	}
	return bOK;
}

#define FS_UI_API_ADAPTER
/************************************************************************
* UI API adapter
*************************************************************************/
LOCAL MMI_RESULT_E HandleSfoxWinMsg(
									MMI_WIN_ID_T win_id, 
									MMI_MESSAGE_ID_E msg_id, 
									DPARAM param
									);
LOCAL MMI_RESULT_E HandleSfoxEditWinMsg(
										MMI_WIN_ID_T		win_id, 
										MMI_MESSAGE_ID_E	msg_id, 
										DPARAM				param
										);

static FS_InputHandler g_input_callback = FS_NULL;
static void * g_input_context = FS_NULL;
static FS_EditParam g_input_param;
static uint16 *g_input_text = FS_NULL;	/* ucs2 format */
static uint16 g_input_text_len = 0;

WINDOW_TABLE( SFOX_WIN_TAB ) = 
{
	WIN_PRIO(WIN_ONE_LEVEL),
	WIN_FUNC((uint32)HandleSfoxWinMsg ),	
	WIN_ID(MMIACC_SFOX_WIN_ID),
	END_WIN
}; 

// 输入日程表的内容
WINDOW_TABLE( SFOX_EDIT_WIN_TAB ) = 
{
	WIN_PRIO( WIN_ONE_LEVEL ),
	WIN_FUNC((uint32) HandleSfoxEditWinMsg ),    
	WIN_ID( MMIACC_SFOX_EDIT_WIN_ID ),
	WIN_TITLE( TXT_NULL ),
	CREATE_TEXTEDITBOX_CTRL(
		MMI_EDITBOX_FULLSCREEN_CLIENT_LEFT, MMI_EDITBOX_FULLSCREEN_CLIENT_TOP, 
		MMI_EDITBOX_FULLSCREEN_CLIENT_RIGHT, MMI_EDITBOX_FULLSCREEN_CLIENT_BOTTOM,
		0, 0, 
		IM_DEFAULT_ALL_INPUT_MODE_SET, 
		IM_SMART_MODE,
		MMIACC_SFOX_EDIT_CTRL_ID
	),
	WIN_SOFTKEY(STXT_OK, TXT_NULL, STXT_RETURN),
	END_WIN
}; 

LOCAL MMI_RESULT_E HandleSfoxEditWinMsg(
									 MMI_WIN_ID_T		win_id, 
									 MMI_MESSAGE_ID_E	msg_id, 
									 DPARAM				param
									 )
{
    MMI_RESULT_E                    recode = MMI_RESULT_TRUE;
    MMI_STRING_T			        str = {0}; 
    MMI_CTRL_ID_T                   current_ctrl_id = MMIACC_SFOX_EDIT_CTRL_ID;
	uint8							*utf8_str = NULL;
	
	(void)param;
    switch(msg_id)
    {
	case MSG_OPEN_WINDOW:
		GUIEDITBOX_SetTextEditBoxMaxLen(
			current_ctrl_id,
			g_input_param.max_len,
			g_input_param.max_len);
		
		GUIEDITBOX_SetTextEditBoxStringInfo(
			current_ctrl_id,
			g_input_text,
			g_input_text_len
			);

		if (g_input_param.preferred_method == FS_IM_123)
		{
			IM_SetCurInputMethod(IM_DIGITAL_MODE);
		}
		else if (g_input_param.preferred_method == FS_IM_ABC)
		{
			IM_SetCurInputMethod(IM_MULTITAP_MODE);
		}
		else if(g_input_param.preferred_method == FS_IM_ENG)
		{
			IM_SetCurInputMethod(IM_ENGLISH_MODE);
		}
		MMK_SetAtvCtrl( win_id,  current_ctrl_id);
		break;
		
	case MSG_CTL_OK:        
		// get string info
		GUIEDITBOX_GetTextEditBoxInfo(current_ctrl_id, &str);
		utf8_str = (uint8 *)SCI_ALLOC( str.wstr_len * 3 + 1 );
		memset( utf8_str, 0, str.wstr_len * 3 + 1 );
		FS_CnvtUcs2ToUtf8( str.wstr_ptr, str.wstr_len, (FS_CHAR *)utf8_str, str.wstr_len * 3 );
		SCI_TRACE_LOW("EditBox return. wstr_len=%d, utf8_str=%s", str.wstr_len, utf8_str);
		if (g_input_callback)
		{
			g_input_callback( (FS_CHAR *)utf8_str, strlen((const char *)utf8_str), g_input_context );
		}

		SCI_FREE( utf8_str );
        MMK_CloseWin(win_id);
		break;
		
    case MSG_CTL_CANCEL:
        MMK_CloseWin(win_id);
        break;

	case MSG_CLOSE_WINDOW:
		g_input_callback = FS_NULL;
		g_input_context = FS_NULL;
		memset( &g_input_param, 0, sizeof(g_input_param) );
		if ( g_input_text )
		{
			SCI_FREE( g_input_text );
			g_input_text = NULL;
		}
		g_input_text_len = 0;
		break;
    default:

        recode = MMI_RESULT_FALSE;
        break;
    }
    
    return recode;
}

void IFS_InputDialog( FS_CHAR * txt, FS_EditParam *eParam, FS_InputHandler cb, void *context )
{
	FS_SINT4 len;

	g_input_callback = cb;
	g_input_context = context;
	g_input_param = *eParam;
	if ( txt )
	{
		len = strlen( txt );
		g_input_text = (uint16 *)SCI_ALLOC( len * 2 + 2 );
		memset( g_input_text, 0, len * 2 + 2 );
		g_input_text_len = (uint16)FS_CnvtUtf8ToUcs2( txt, -1, g_input_text, len );
	}
	else
	{
		g_input_text = FS_NULL;
		g_input_text_len = 0;
	}

	MMK_CreateWin((uint32 *)SFOX_EDIT_WIN_TAB,PNULL);
}

LOCAL MMI_RESULT_E HandleSfoxKeyMsg( MMI_MESSAGE_ID_E msg_id )
{
	MMI_RESULT_E recode = MMI_RESULT_TRUE;
	switch (msg_id)
	{
	/* sfox keypad event */
	case MSG_KEYDOWN_UP:
		FS_SendKeyEvent(FS_KEY_UP);
		break;
	case MSG_KEYDOWN_DOWN:
		FS_SendKeyEvent(FS_KEY_DOWN);
		break;
	case MSG_KEYDOWN_LEFT:
		FS_SendKeyEvent(FS_KEY_LEFT);
		break;
	case MSG_KEYDOWN_RIGHT:
		FS_SendKeyEvent(FS_KEY_RIGHT);
		break;
	case MSG_KEYDOWN_SPUP:
	case MSG_KEYDOWN_UPSIDE:
		FS_SendKeyEvent(FS_KEY_PGUP);
		break;
	case MSG_KEYDOWN_SPDW:
	case MSG_KEYDOWN_DOWNSIDE:
		FS_SendKeyEvent(FS_KEY_PGDOWN);
		break;
	case MSG_KEYDOWN_OK:
		FS_SendKeyEvent(FS_SOFTKEY1);
		break;
	case MSG_KEYDOWN_WEB:
		FS_SendKeyEvent(FS_SOFTKEY2);
		break;
	case MSG_KEYDOWN_CANCEL:
		FS_SendKeyEvent(FS_SOFTKEY3);
		break;
	case MSG_KEYDOWN_0:
		FS_SendKeyEvent(FS_KEY_0);
		break;
	case MSG_KEYDOWN_1:
		FS_SendKeyEvent(FS_KEY_1);
		break;
	case MSG_KEYDOWN_2:
		FS_SendKeyEvent(FS_KEY_2);
		break;
	case MSG_KEYDOWN_3:
		FS_SendKeyEvent(FS_KEY_3);
		break;
	case MSG_KEYDOWN_4:
		FS_SendKeyEvent(FS_KEY_4);
		break;
	case MSG_KEYDOWN_5:
		FS_SendKeyEvent(FS_KEY_5);
		break;
	case MSG_KEYDOWN_6:
		FS_SendKeyEvent(FS_KEY_6);
		break;
	case MSG_KEYDOWN_7:
		FS_SendKeyEvent(FS_KEY_7);
		break;
	case MSG_KEYDOWN_8:
		FS_SendKeyEvent(FS_KEY_8);
		break;
	case MSG_KEYDOWN_9:
		FS_SendKeyEvent(FS_KEY_9);
		break;
	default:
		recode = MMI_RESULT_FALSE;
		break;
	}
	return recode;
}

LOCAL MMI_RESULT_E HandleSfoxWinMsg(
									MMI_WIN_ID_T win_id, 
									MMI_MESSAGE_ID_E msg_id, 
									DPARAM param
									)
{
	MMI_RESULT_E recode = MMI_RESULT_TRUE;
	T_SfoxMsg *sfoxmsg = NULL;
	
	switch(msg_id)
	{
	case MSG_OPEN_WINDOW:
		SCI_TRACE_LOW("SFOX Main Window: MSG_OPEN_WINDOW");
		GSFOX_AppStatus = SFOX_APP_STS_TOP_VISIBLE;
		GSFOX_NativeBrowserActive = FALSE;
		//window is open, call target launch function
		FS_SnsMain();
		break;
		
	case MSG_FULL_PAINT:
		SCI_TRACE_LOW("SFOX Main Window: MSG_FULL_PAINT");
		GSFOX_AppStatus = SFOX_APP_STS_TOP_VISIBLE;
		FS_GuiRepaint( );
		break;

	case MSG_LOSE_FOCUS:
	{
		FS_Rect rect = {0};
		SCI_TRACE_LOW("SFOX Main Window: MSG_LOSE_FOCUS");
		FS_PushClipRect( &rect, 0 );
		GSFOX_AppStatus = SFOX_APP_STS_SUSPENDED;
		break;
	}
	case MSG_TIMER:
	{
		GUI_TIMER_ID_T tid = *( GUI_TIMER_ID_T*)param;
		FS_ITimerData *pTD = IFS_GetTimerData( tid );
		if( pTD )
		{
			pTD->handle( pTD->param );
			pTD->id = 0;
		}
		break;
	}
	case MSG_GET_FOCUS:
		SCI_TRACE_LOW("SFOX Main Window: MSG_GET_FOCUS");
		GSFOX_AppStatus = SFOX_APP_STS_TOP_VISIBLE;
		GSFOX_IsSFOXInUsing = FALSE;
		GSFOX_NativeBrowserActive = FALSE;
		FS_PopClipRect( );
		break;

	case MSG_CLOSE_WINDOW:
		SCI_TRACE_LOW("SFOX Main Window: MSG_CLOSE_WINDOW");
		GSFOX_AppStatus = SFOX_APP_STS_STOPPED;
		GSFOX_IsSFOXInUsing = FALSE;
		GSFOX_NativeBrowserActive = FALSE;
		// exit sfox
		FS_PopClipRect( );
		FS_SnsExit();
		break;
		
	case MSG_TP_PRESS_DOWN:
	{
        FS_SINT4 x, y;
        x = MMK_GET_TP_X(param);
        y = MMK_GET_TP_Y(param);
		FS_SendMouseEvent( x, y );
		break;
	}		

	case MSG_APP_SFOX:
		sfoxmsg = (T_SfoxMsg *)param;
		FS_HandlePostMessage( sfoxmsg->msg, sfoxmsg->param );
		break;

	case MSG_MULTIM_SELECTED_RETURN:
	{
		MULTIM_SELECT_RETURN_T *select_file = (MULTIM_SELECT_RETURN_T *)param;
		uint8 *utf8_str = (uint8 *)SCI_ALLOC( select_file->file_name_len * 3 + 1 );
		
		memset( utf8_str, 0, select_file->file_name_len * 3 + 1 );
		FS_CnvtUcs2ToUtf8( select_file->file_name_ptr, select_file->file_name_len, 
			(FS_CHAR *)utf8_str, select_file->file_name_len * 3 );
		SCI_TRACE_LOW("MSG_MULTIM_SELECTED_RETURN fname=%s, fnlen=%d, size=%d", 
			utf8_str, select_file->file_name_len, select_file->file_size );
		if( GSFOX_FDHandler )
		{
			GSFOX_FDHandler( utf8_str, GSFOX_FDParam );
			GSFOX_FDHandler = FS_NULL;
		}
		SCI_FREE(utf8_str);
		break;
	}
	default:
		recode = HandleSfoxKeyMsg(msg_id);
		break;
	}

	return recode;
}

PUBLIC void MMIAPISFOX_OpenMainWin(void)
{
	MMISET_LANGUAGE_TYPE_E language_type = MMISET_LANGUAGE_ENGLISH;

	MMIAPISET_GetLanguageType(&language_type);
	SCI_TRACE_LOW("MMIAPISFOX_OpenMainWin language=%d", language_type);
	if ( language_type == MMISET_LANGUAGE_SIMP_CHINESE )
	{
		FS_SetLanguage( 1 );
	}
	else
	{
		FS_SetLanguage( 0 );
	}
    MMK_CreateWin((uint32 *)SFOX_WIN_TAB,PNULL);
}

#else

PUBLIC void MMIAPISFOX_OpenMainWin(void)
{

}

#endif//SFOX_SUPPORT

