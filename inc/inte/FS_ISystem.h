#ifndef _FS_I_SYSTEM_H_
#define _FS_I_SYSTEM_H_

#include "inc/FS_Config.h"

typedef void ( * FS_TimerHandler )( void *param );

FS_UINT4 IFS_StartTimer( FS_UINT4 id, FS_UINT4 msecs, FS_TimerHandler cb, void *param );
void IFS_StopTimer( FS_UINT4 id );

/*
	post a message(not sendmessage). when device receive this msg, 
	must call FS_HandlePostMessage to handle the message
	@see FS_Config.h for FS_HandlePostMessage
*/
void		IFS_PostMessage( FS_UINT2 msg, FS_UINT4 param );

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
FS_COLOR IFS_DDB_RGB( FS_BYTE r, FS_BYTE g, FS_BYTE b );

FS_BYTE IFS_DDB_RED( FS_COLOR clr );

FS_BYTE IFS_DDB_GREEN( FS_COLOR clr );

FS_BYTE IFS_DDB_BLUE( FS_COLOR clr );

void IFS_InvalidateRect( FS_Rect *pRect );

void IFS_SystemExit( void );

FS_CHAR *IFS_GetRootDir( void );

FS_CHAR *IFS_GetPathSep( void );

FS_SINT4 IFS_GetScreenWidth( void );

FS_SINT4 IFS_GetScreenHeight( void );

FS_SINT4 IFS_GetWinTitleHeight( void );

FS_Bitmap *IFS_GetWinTitleBgImg( void );

void IFS_ReleaseWinTitleBgImg( FS_Bitmap *pBmp );

FS_Bitmap *IFS_GetSoftkeyBarBgImg( void );

void IFS_ReleaseSoftkeyBarBgImg( FS_Bitmap *pBmp );

FS_SINT4 IFS_GetSoftkeyHeight( void );

FS_SINT4 IFS_GetWidgetSpan( void );

FS_SINT4 IFS_GetLineHeight( void );

FS_SINT4 IFS_DcdGetChannelTabHeight( void );

FS_SINT4 IFS_DcdGetIdleDetailHeight( void );

FS_UINT1 IFS_GetBitsPerPixel( void );

FS_BYTE *IFS_GetScreenBitmap( void );

void IFS_ReleaseScreenBitmap( FS_BYTE *pBits );

FS_CHAR *	IFS_GetUserAgent( void );

FS_CHAR * IFS_GetUaProfile( void );

FS_BOOL IFS_IsInternationalRoaming( void );

/*--------------------------------------------------------------------------------------
		input dialog interface
----------------------------------------------------------------------------------------*/
/*
	input dialog call back
param:
	ok			if TRUE, inform that user input the content
	text		input text, can be NULL
	len 		input text len, typical strlen of text
	param		param pass to call input dialog interface
*/
typedef void ( * FS_InputHandler )( FS_CHAR * text, FS_SINT4 len, void *param );

/*
	input dialog
param:
	txt 		pre input text
	len 		txt buffer len limit
	method		input method, see define above
	cb			callback when user finish input
	param		param to call cb
*/
void IFS_InputDialog( FS_CHAR * txt, FS_EditParam *eParam, FS_InputHandler cb, void *context );

/*--------------------------------------------------------------------------------------
		file dialog interface
----------------------------------------------------------------------------------------*/
typedef void ( * FS_FDHandler )( FS_CHAR * filename, void *param );

#define FS_FDO_ALL		0
#define FS_FDO_IMAGE	1
#define FS_FDO_AUDIO	2
#define FS_FDO_VIDEO	3

/*
	@param [in]	type		Open a file/save a file
	@param [in]	preName		recommand file name, use for FS_FD_SAVE.
	@param [in] cb			callback function
	@param [in] param		callback param
	@remark open a file dialog to open/save file
*/
void IFS_FileDialogSave( FS_CHAR *preName, FS_FDHandler cb, void *param );

void IFS_FileDialogOpen( FS_UINT1 filter, FS_FDHandler cb, void *param );

void IFS_CameraNewPhoto( FS_FDHandler cb, void *param );

/*--------------------------------------------------------------------------------------
		phon book interface
----------------------------------------------------------------------------------------*/
/* phnums 可以包含多个收件人号码，用逗号隔开 */
typedef void ( * FS_PBHandler )( FS_CHAR * phnums, void *param );

void IFS_PBSelectPhoneNumber( FS_PBHandler cb, void *param );

/*
	get image size. width and height
	file	[in] 	image file
	w	    [out]	image width
	h		[out]	image height

	return FS_TRUE if success.
*/
FS_BOOL IFS_ImageGetSize( FS_CHAR *file, FS_SINT4 *w, FS_SINT4 *h );

/*
	decode image base on require width and height
	file	[in] 	image file
	w	    [in]	required width
	h		[in]	required height
	pBmp	[out]	decode result
	
	return a image handle, 0 on failed.
*/
FS_UINT4 IFS_ImageDecode( FS_CHAR *file, FS_SINT4 w, FS_SINT4 h, FS_Bitmap *pBmp );

void IFS_ImageRelease( FS_UINT4 hImage, FS_Bitmap *pBmp );

void IFS_ImageDestroy( FS_UINT4 hImage );

/*
	when there is next frame. call this function to inform core
	w and h must the same on IFS_ImageDecode
	can free the pBmp when FS_ImageFrameInd return.
*/
void FS_ImageFrameInd( FS_UINT4 hImage, FS_Bitmap *pBmp );

/*
 * file : the audio file abs path
 * nTimes : play times. if nTimes <= 0, play as many time as you can
*/
FS_BOOL IFS_PlayAudio( FS_CHAR *file, FS_SINT4 nTimes );

void IFS_StopAudio( void );

FS_CHAR *IFS_PBFindNameByPhone( FS_CHAR *phoneNum );

void IFS_WriteNewSms( FS_CHAR *text, FS_CHAR *num );

FS_BOOL IFS_PushInd( FS_UINT1 tid, FS_UINT1 app_id, FS_CHAR *app_str, 
		FS_UINT2 ct_code, FS_CHAR *content_type, FS_BYTE *data, FS_SINT4 len );

FS_SINT4 IFS_GetMaxCacheItem( void );

FS_SINT4 IFS_GetMaxCacheSize( void );

/* 彩信总空间大小限制 */
FS_SINT4 IFS_GetMaxMmsSizeLimit( void );

/* 彩信条数限制 */
FS_SINT4 IFS_GetMaxMmsItemLimit( void );

/* 单条彩信大小限制 */
FS_SINT4 IFS_GetMaxMmsSize( void );

/* 单条彩信帧数限制 */
FS_SINT4 IFS_GetMaxMmsFrames( void );

FS_SINT4 IFS_GetMaxPushItemLimit( void );

FS_SINT4 IFS_GetMaxEmlSizeLimit( void );

FS_SINT4 IFS_GetMaxEmlItemLimit( void );

void IFS_PushNotify( FS_CHAR *title, FS_CHAR *url );

void IFS_PushListFull( void );

void IFS_MmsNotify( FS_BOOL is_full, FS_BOOL is_too_large, FS_CHAR *from, FS_CHAR *subject );

void IFS_MmsAutoRecvNotify( FS_BOOL bSuccess, FS_CHAR *from, FS_CHAR *subject );

FS_UINT4 IFS_GetMemPoolSize( void );

void *IFS_GetMemPool( void );

void IFS_ReleaseMemPool( void *mem );

FS_SINT4 IFS_DrawChar( FS_SINT4 x, FS_SINT4 y, FS_UINT2 nChar, FS_UINT2 attr, FS_COLOR fg, FS_COLOR bg );

FS_BYTE *IFS_GetCharBitmap( FS_UINT2 nChar );

FS_SINT4 IFS_GetCharWidth( FS_UINT2 nChar );

FS_SINT4 IFS_GetCharHeight( FS_UINT2 nChar );

FS_SINT4 IFS_GetFontMaxHeight( void );

FS_BOOL IFS_SystemRuningBackgroud( void );

FS_BOOL IFS_SystemAllowDrawScreen( void );

FS_BOOL IFS_DcdCanDrawIdle( void );

void IFS_EnterBackLight( void );

void IFS_LeaveBackLight( void );

void IFS_WtaiMakeCall( FS_CHAR *phnum );

void IFS_WtaiAddPBEntry( FS_CHAR *phnum, FS_CHAR *name );

FS_CHAR *IFS_JvmCheckJad( FS_CHAR *jadFile );

FS_BOOL IFS_JvmInstallJar( FS_CHAR *jarFile );

FS_BOOL IFS_BrowserOpenURL( FS_CHAR *url );

#endif

