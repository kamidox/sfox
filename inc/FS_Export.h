#ifndef _FS_EXPORT_H
#define  _FS_EXPORT_H

typedef unsigned char		FS_UINT1;
typedef char				FS_SINT1;	
typedef unsigned short		FS_UINT2;	
typedef short				FS_SINT2;	
typedef unsigned int		FS_UINT4;	
typedef int					FS_SINT4;	
typedef unsigned char		FS_BYTE;
typedef unsigned int		FS_BOOL;
typedef char				FS_CHAR;
typedef unsigned short		FS_WCHAR;

#define FS_FALSE			0
#define FS_TRUE				1
#define FS_NULL				0

/****************************************************************
			export structure and function
****************************************************************/

/* color define, we use DIB to draw screen */
typedef FS_SINT4				FS_COLOR;

/* key event system want to handle */
typedef enum FS_KeyEvent_Tag
{
	FS_SOFTKEY1 = 1,
	FS_SOFTKEY2,
	FS_SOFTKEY3,
	FS_KEY_LEFT,
	FS_KEY_RIGHT,
	FS_KEY_UP,
	FS_KEY_DOWN,
	FS_KEY_PGUP,
	FS_KEY_PGDOWN,
	FS_KEY_0,
	FS_KEY_1,
	FS_KEY_2,
	FS_KEY_3,
	FS_KEY_4,
	FS_KEY_5,
	FS_KEY_6,
	FS_KEY_7,
	FS_KEY_8,
	FS_KEY_9
}FS_KeyEvent;

/* date time struct */
typedef struct FS_DateTime_Tag
{
	FS_UINT2		year;		// 200X
	FS_UINT1		month;		// 1->12
	FS_UINT1		day;		// 1->30
	FS_UINT1		week_day;	// 0 - 6 sun mon ...-> sat
	
	FS_UINT1		hour;		// 0 -> 23
	FS_UINT1		min;		// 0->59
	FS_UINT1		sec;			// 0->59
}FS_DateTime;

/* rectangle define */
typedef struct FS_Rect_Tag
{
	FS_SINT4		left;
	FS_SINT4		top;
	FS_SINT4		width;
	FS_SINT4		height;
}FS_Rect;

/* bitmap struct */
typedef struct FS_Bitmap_Tag
{
	FS_SINT4	width;
	FS_SINT4	height;
	FS_UINT4	bpp;		/* bytes per pixel: image decode result must be the same of system's BPP */
	FS_UINT4	pitch;

	FS_BYTE *	bits;
}FS_Bitmap;

/* device content */
typedef struct FS_DC_Tag
{
	FS_UINT1	bits_per_pixel;
	FS_UINT1	bytes_per_pixel;

	FS_SINT4	width;
	FS_SINT4	height;
	FS_UINT4	pitch;
	
	FS_COLOR	fg_color;
	FS_COLOR	bg_color;
	FS_COLOR	trans_color;		// transparent color

	FS_BYTE*	bits;
}FS_DC;

#define FS_IM_ABC		0x01
#define FS_IM_123		0x02
#define FS_IM_ENG		0x04
#define FS_IM_CHI		0x08

#define FS_IM_ALL		0xFF

typedef struct FS_EditParam_Tag
{
	FS_UINT1			preferred_method;
	FS_UINT1			allow_method;
	FS_SINT2			max_len;
	FS_Rect				rect;
	FS_BOOL 			limit_character;	/* if true. max_len is limit in charactor, not in bytes */
}FS_EditParam;

/****************************************************************
			export function
****************************************************************/

/* launch email main UI */
void FS_EmlMain( void );

/* quit email application, call by device to quit email app */
void FS_EmlExit( void );

/* launch browser main UI */
void FS_WebMain( void );

/* quit web application, call by device to quit web app */
void FS_WebExit( void );

void FS_MmsMain( void );

/* handle key event */
void FS_SendKeyEvent( FS_SINT4 event );

/* handle mouse event if any */
void FS_SendMouseEvent( FS_SINT4 x, FS_SINT4 y );

/* repaint GUI. eg. when f_soft app got focus, call this function */
void FS_GuiRepaint( void );

/* when dial up(socket init) failed. call this to alert user  */
void FS_NetConnResultInd( FS_BOOL bOk );

/* client call IFS_PostMessage. when device retrive the msg, call this to handle */
void FS_HandlePostMessage( FS_UINT2 msg, FS_UINT4 param );

/* 0 for english, 1 for chiness */
void FS_SetLanguage( FS_UINT4 lan );

void FS_MmsExit( void );

FS_BOOL FS_MmsIsFull( FS_SINT4 add_size );

FS_BOOL FS_MmsHasUnread( void );

void FS_MmsSendByMms( FS_CHAR *subject, FS_CHAR *to, FS_CHAR *text, FS_CHAR *file );

void FS_MmsOpenInbox( void );

FS_BOOL FS_PushIsFull( void );

void FS_WebOpenPushList( void );

void FS_PushInd( FS_BYTE *pdu, FS_SINT4 len );

void FS_WebOpenBookmarkList( void );

void FS_WebOpenUrl( FS_CHAR *url );

void FS_WebLoadWebDoc( FS_CHAR *file );

void FS_WebOpenHomePage( void );

FS_BOOL FS_PushHasUnread( void );

FS_BOOL FS_SystemInit( void );

void FS_SystemDeinit( void );

FS_SINT4 FS_DcdInit( void );

void FS_DcdDeinit( void );

void FS_DcdOpenChannelList( void );

void FS_DcdDrawIdle( void );

void FS_DcdResumeIdleTimer( void );

void FS_DcdPauseIdleTimer( void );

FS_SINT4 FS_StockInit( void );

void FS_StockDeinit( void );

void FS_StkMain( void );

void FS_SnsMain( void );

void FS_SnsExit( void );

#endif

