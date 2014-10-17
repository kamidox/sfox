#include "inc\inte\FS_Inte.h"

FS_InputHandler g_input_callback = FS_NULL;
void * g_input_context = FS_NULL;
FS_EditParam g_input_param;

FS_FDHandler g_fd_handle = FS_NULL;
void *g_fd_param = FS_NULL;

#ifdef FS_PLT_WIN
#include <stdio.h>
#include <windows.h>
#include "resource.h"

#define WM_SOCKET	(WM_USER + 1)
#define WM_CLIENT	(WM_USER + 2)
#define WM_SOCKET_SENDOK	(WM_USER + 3)

FS_BOOL FS_CnvtGBKToUtf8( FS_CHAR* gbStr, FS_SINT4 gbLen, FS_CHAR* utf8Str, FS_SINT4 *utf8Len );
FS_BOOL FS_CnvtUtf8ToGBK( FS_CHAR *utf8Str, FS_SINT4 utf8Len, FS_CHAR *gbStr, FS_SINT4 *outLen );

void ProcessSocketEvent( FS_UINT4 wParam, FS_UINT4 lParam );
void ProcessSocketSendOK( FS_UINT4 wParam, FS_UINT4 lParam );
void EndInput( void );

LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM) ;

HWND GFS_HWND;
HINSTANCE GFS_HINS;
HWND GFS_hEditWnd;

typedef void (*FS_AppEntryFunc)(void);
typedef struct _FS_AppEntry
{
	FS_AppEntryFunc		entry;
	FS_CHAR *			cmd;
	FS_CHAR *			desp;
}FS_AppEntry;

static FS_AppEntry	s_entrys[] =
{
#ifdef FS_MODULE_WEB
	{ FS_WebMain, 				"wap", 				"open browser" },
	{ FS_WebOpenHomePage,		"wap_homepage",		"open wap homepage" },
	{ FS_WebOpenBookmarkList,	"wap_bookmarks", 	"open wap bookmark list" },
	{ FS_WebOpenPushList,		"wap_pushs",		"open wap push msg list" },
#endif
#ifdef FS_MODULE_EML
	{ FS_EmlMain,				"email", 			"open email client" },
#endif
#ifdef FS_MODULE_MMS
	{ FS_MmsMain,				"mms", 				"open mms client" },
#endif
#ifdef FS_MODULE_DCD
	{ FS_DcdDrawIdle,			"dcd", 				"open dcd idle screen" },
	{ FS_DcdOpenChannelList,	"dcd_channels", 	"open dcd channel list" },	
#endif
#ifdef FS_MODULE_STK
	{ FS_StkMain,				"stock",			"open stock list" },	
#endif
#ifdef FS_MODULE_SNS
	{ FS_SnsMain,				"sns",				"open sns login win" },	
#endif
	/* the last entry */
	{ NULL,						NULL,				NULL }
};

extern FS_DC GFS_DC;
BOOL CALLBACK InputDlgProc( HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam ) ;

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	static TCHAR szAppName[] = TEXT ("HelloWin") ;
	HWND	hwnd ;
	MSG	msg ;
	WNDCLASS	wndclass ;
	int			i;
	
 	wndclass.style		  = CS_HREDRAW | CS_VREDRAW ;
 	wndclass.lpfnWndProc  = WndProc ;
	wndclass.cbClsExtra	  = 0 ;
	wndclass.cbWndExtra	  = 0 ;
	wndclass.hInstance	  = hInstance ;
	wndclass.hIcon		  = LoadIcon (NULL, IDI_APPLICATION) ;
  	wndclass.hCursor	  = LoadCursor (NULL, IDC_ARROW) ;
 	wndclass.hbrBackground	= (HBRUSH) GetStockObject (WHITE_BRUSH) ;
  	wndclass.lpszMenuName	= NULL ;
	wndclass.lpszClassName	= szAppName ;

	if (!RegisterClass (&wndclass))
	{
		MessageBox (	NULL, TEXT ("This program requires Windows NT!"), 
						szAppName, MB_ICONERROR) ;
		return 0 ;
	}
	
	if( stricmp(lpCmdLine, "help") == 0 )
	{
		char	buf[1024];
		int 	len = 0;
		sprintf(buf + len, "command list:\r\n");
		len = strlen(buf);
		for( i = 0; s_entrys[i].entry != NULL; i ++ )
		{
			sprintf(buf + len, "%s:\t%s\r\n", s_entrys[i].cmd, s_entrys[i].desp);
			len = strlen( buf );
		}
		MessageBox( NULL, buf, "help", MB_OK );
		return 0;
	}

	hwnd = CreateWindow( szAppName,	// window class name
			TEXT( "smartfox" ),	// window caption
			WS_VISIBLE | WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX | WS_BORDER /* WS_OVERLAPPEDWINDOW */,	// window style
			800,	// initial x position
			100,	// initial y position
			IFS_GetScreenWidth( ) + 2,	// initial x size
			IFS_GetScreenHeight( ) + 2 + 250,	// initial y size
			NULL,			// parent window handle
		    NULL,	        // window menu handle
		    hInstance,	    // program instance handle
		    NULL) ; 	    // creation parameters
    
	GFS_HWND = hwnd;
	GFS_HINS = hInstance;
	
	ShowWindow (hwnd, nCmdShow) ;
	UpdateWindow(hwnd);

	FS_SystemInit( );
	for( i = 0; s_entrys[i].entry != NULL; i ++ )
	{
		if( stricmp(lpCmdLine, s_entrys[i].cmd) == 0)
		{
			s_entrys[i].entry ();
			break;
		}
	}
	/* add default entrys */
	if( s_entrys[i].entry == NULL )
	{
#ifdef FS_MODULE_SNS
		FS_SnsMain();
#endif
	}
	
	//FS_MmsSendByMms("subject", "13555455125", "this is a mms text", "E:\\F_Soft\\sample\\media\\1.png");
	while (GetMessage (&msg, NULL, 0, 0))
     {
		TranslateMessage (&msg) ;
  		DispatchMessage (&msg) ;
     }
	return msg.wParam ;
}

void HandleKeyEvent( int keyCode )
{
	int ev = 0;
	
	switch( keyCode )
	{
		case 97:
		case 77:
			ev = FS_SOFTKEY1;
			break;
		case 99:
		case 27:
			ev = FS_SOFTKEY3;
			break;
		case 37:
		case 100:
		case 74:
			ev = FS_KEY_LEFT;
			break;
		case 39:
		case 102:
		case 76:
			ev = FS_KEY_RIGHT;
			break;
		case 38:
		case 104:
		case 73:
			ev = FS_KEY_UP;
			break;
		case 40:
		case 98:
		case 75:
			ev = FS_KEY_DOWN;
			break;
		case 33:
			ev = FS_KEY_PGUP;
			break;
		case 34:
			ev = FS_KEY_PGDOWN;
			break;
		case 101:
		case 13:
			ev = FS_SOFTKEY2;
			break;
	}
	if( ev != 0 )
		FS_SendKeyEvent( ev );
}

/*
softkey1: { 3, 44, 25, 25 }
softkey2: { 74, 44, 25, 25 }
softkey3: { 143, 44, 25, 25 }
key_left: { 44, 44, 25, 25 }
key_right: { 108, 44, 25, 25 }

key_up: { 74, 25, 25, 18 }
key_down: { 74, 74, 25, 18 }

key_1: { 16, 105, 40, 20 }
key_2: { 68, 105, 40, 20 }
key_3: { 117, 105, 40, 20 }

key_4: { 16, 138, 40, 20 }
key_5: { 68, 138, 40, 20 }
key_6: { 117, 138, 40, 20 }

key_7: { 16, 172, 40, 20 }
key_8: { 68, 172, 40, 20 }
key_9: { 117, 172, 40, 20 }

key_star: { 16, 206, 40, 20 }
key_0: { 68, 206, 40, 20 }
key_shut: { 117, 206, 40, 20 }
*/
BOOL HandleMouseEvent( int x, int y )
{
	int event = -1;

	if( y < IFS_GetScreenHeight( ) )
		return FALSE;

	y -= IFS_GetScreenHeight( );

	if( y >= 25 && y <= 25 + 18 )
	{
		if( x >= 74 && x <= 74 + 25 )
			event = FS_KEY_UP;
	}
	else if( y >= 74 && y <= 74 + 18 )
	{
		if( x >= 74 && x <= 74 + 25 )
			event = FS_KEY_DOWN;
	}
	else if( y >= 44 && y <= 44 + 25 )
	{
		if( x >= 3 && x <= 3 + 25 )
			event = FS_SOFTKEY1;
		else if( x >= 44 && x <= 44 + 25 )
			event = FS_KEY_LEFT;
		else if( x >= 74 && x <= 74 + 25 )
			event = FS_SOFTKEY2;
		else if( x >= 108 && x <= 108 + 25 )
			event = FS_KEY_RIGHT;
		else if( x >= 143 && x <= 143 + 25 )
			event = FS_SOFTKEY3;
	}
	else if( y >= 105 && y <= 105 + 20 )
	{
		if( x >= 16 && x <= 16 + 40 )
			event = FS_KEY_1;
		else if( x >= 68 && x <= 68 + 40 )
			event = FS_KEY_2;
		else if( x >= 117 && x <= 117 + 40 )
			event = FS_KEY_3;
	}
	else if( y >= 138 && y <= 138 + 20 )
	{
		if( x >= 16 && x <= 16 + 40 )
			event = FS_KEY_4;
		else if( x >= 68 && x <= 68 + 40 )
			event = FS_KEY_5;
		else if( x >= 117 && x <= 117 + 40 )
			event = FS_KEY_6;
	}
	else if( y >= 172 && y <= 172 + 20 )
	{
		if( x >= 16 && x <= 16 + 40 )
			event = FS_KEY_7;
		else if( x >= 68 && x <= 68 + 40 )
			event = FS_KEY_8;
		else if( x >= 117 && x <= 117 + 40 )
			event = FS_KEY_9;
	}
	else if( y >= 206 && y <= 206 + 20 )
	{
		if( x >= 16 && x <= 16 + 40 )
			event = FS_KEY_PGUP;
		else if( x >= 68 && x <= 68 + 40 )
			event = FS_KEY_0;
		else if( x >= 117 && x <= 117 + 40 )
			event = FS_KEY_PGDOWN;
	}
	
	if( event != -1 )
	{
		FS_SendKeyEvent( event );
	}
	return TRUE;
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HINSTANCE hInstance;
	static HBITMAP hBitmap;
	static int kbW, kbH;

	BITMAP bitmap;
	HDC					hdc, hdcMem;
	PAINTSTRUCT			ps;
	FS_BYTE *			pScrBit;
	int					i;

	switch (message)
	{
		case WM_CREATE :
			hInstance = ((LPCREATESTRUCT) lParam)->hInstance;
			hBitmap = LoadBitmap( hInstance, MAKEINTRESOURCE(IDB_KEYBOARD) );
			GetObject( hBitmap, sizeof (BITMAP), &bitmap );
			kbW = bitmap.bmWidth;
			kbH = bitmap.bmHeight;
			return 0 ;
		case WM_PAINT:
		{
			RECT rect = { 0, 0, IFS_GetScreenWidth( ), IFS_GetScreenHeight( ) };
			int pitch;
			static BITMAPINFO bmpInfo = 
			{
				40,
				176,	//rect.right - rect.left,
				220,	//rect.bottom - rect.top,
				1,
				24,
				0,
				0xc5c0,
				0,
				0,
				0,
				0
			};

			if ( lParam )
			{
				//rect = *((RECT *)lParam);
				//bmpInfo.bmiHeader.biWidth = rect.right - rect.left;
				//bmpInfo.bmiHeader.biHeight = rect.bottom - rect.top;
				//bmpInfo.bmiHeader.biSizeImage = bmpInfo.bmiHeader.biWidth * bmpInfo.bmiHeader.biHeight * 3;
			}

			if( GFS_DC.bits == NULL ) return 0;
			
			pitch = (rect.right - rect.left) * 3;
			hdc = BeginPaint (hwnd, &ps) ;
			
			pScrBit = malloc( pitch * (rect.bottom - rect.top) );
			for( i = rect.top; i < rect.bottom; i ++ )
			{
				memcpy( pScrBit + pitch * (i - rect.top), 
					GFS_DC.bits + (GFS_DC.height - i - 1) * GFS_DC.pitch + rect.left * GFS_DC.bytes_per_pixel, 
					pitch );
			}
			SetDIBitsToDevice( hdc, rect.left, rect.top, rect.right - rect.left, 
				rect.bottom - rect.top, 0, 0, 0, rect.bottom - rect.top, pScrBit, &bmpInfo, 0 );
			free( pScrBit );

			/* draw keyboard */
			hdcMem = CreateCompatibleDC (hdc);
			SelectObject( hdcMem, hBitmap );
			BitBlt( hdc, 0, IFS_GetScreenHeight( ), kbW, kbH, hdcMem, 0, 0, SRCCOPY );
			DeleteDC( hdcMem );

			EndPaint( hwnd, &ps ) ;
			return 0 ;
		}
		case WM_KEYDOWN:
		{
			HandleKeyEvent( wParam );
			return TRUE;
		}
		case WM_LBUTTONDOWN:
		{
			int x = LOWORD(lParam), y = HIWORD(lParam);
			if( GFS_hEditWnd )
			{
				EndInput( );
			}
			
			if( y >= 0 && y <= 20 )
				PostMessage( hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0 );

			if( HandleMouseEvent( x, y ) == FALSE )
				FS_SendMouseEvent( x, y );

			return TRUE;
		}
		case WM_DESTROY:
		{			
			FS_SystemDeinit( );
			DeleteObject( hBitmap );
			PostQuitMessage (0) ;
			return 0 ;
		}
		case WM_SOCKET_SENDOK:
		{
			ProcessSocketSendOK( wParam, lParam );
			return TRUE;
		}
		case WM_SOCKET:
		{
			ProcessSocketEvent( wParam, lParam );
			return TRUE;
		}
		case WM_CLIENT:
		{
			FS_HandlePostMessage( (FS_UINT2)wParam, lParam );
			return TRUE;
		}
	}
	return DefWindowProc (hwnd, message, wParam, lParam) ;
}

void EndInput( void )
{
	char *str;
	char *utf8;
	int len;
	
	if( g_input_param.max_len <= 0 )
	{
		g_input_param.max_len = 1024;
	}
	str = malloc( g_input_param.max_len + 1);
	memset( str, 0, g_input_param.max_len + 1 );
	GetWindowText( GFS_hEditWnd, str, g_input_param.max_len + 1);
	len = strlen( str );
	utf8 = malloc( len * 3 / 2 + 1 );
	FS_CnvtGBKToUtf8( str, len, utf8, FS_NULL );
	g_input_callback( utf8, strlen( utf8 ), g_input_context );
	free( str );
	free( utf8 );
	DestroyWindow( GFS_hEditWnd );
	GFS_hEditWnd = NULL;
}

static void IFS_FDCB( FS_CHAR * text, FS_SINT4 len, void *param )
{
	if(g_fd_handle)
		g_fd_handle( text, g_fd_param );
	g_fd_handle = NULL;
	g_fd_param = NULL;
}

FS_CHAR *Utf8ToGb2312( FS_CHAR *utf8 )
{
	FS_CHAR *str;
	int len;

	if( utf8 == FS_NULL ) return FS_NULL;
	len = strlen( utf8 );
	str = malloc( len + 1 );
	memset( str, 0, len + 1 );
	FS_CnvtUtf8ToGBK( utf8, -1, str, FS_NULL );
	return str;
}

#endif

#ifdef FS_PLT_VIENNA
#include "..\system\portab.h"
#include "..\system\sysprim.h"
#include "..\config\alconfig.h"
#include "..\ui\uif5\win\uifwindo.h"
#include "..\amoi\uzi\ziwindow.h"
#include "..\ui\uif\tools\uifutf8.h"
#include "..\amoi\alunicod.h"
#include "..\amoi\explorer\alexplorer.h"

UIWINDOW *GFS_HWND;

static void IFS_SendKeyEventInd( UINT16 keyCode )
{
	int ev = 0;	
	switch( keyCode )
	{
		case EV_MENU:
			ev = FS_SOFTKEY1;
			break;
		case EV_CANCEL:
			ev = FS_SOFTKEY3;
			break;
		case EV_LEFT:
			ev = FS_KEY_LEFT;
			break;
		case EV_RIGHT:
			ev = FS_KEY_RIGHT;
			break;
		case EV_UP:
			ev = FS_KEY_UP;
			break;
		case EV_DOWN:
			ev = FS_KEY_DOWN;
			break;
		case EV_STAR:
			ev = FS_KEY_PGUP;
			break;
		case EV_HASH:
			ev = FS_KEY_PGDOWN;
			break;
		case EV_SELECT:
			ev = FS_SOFTKEY2;
			break;
		case EV_KEY0:
			ev = FS_KEY_0;
			break;
		case EV_KEY1:
			ev = FS_KEY_1;
			break;
		case EV_KEY2:
			ev = FS_KEY_2;
			break;
		case EV_KEY3:
			ev = FS_KEY_3;
			break;	
		case EV_KEY4:
			ev = FS_KEY_4;
			break;
		case EV_KEY5:
			ev = FS_KEY_5;
			break;
		case EV_KEY6:
			ev = FS_KEY_6;
			break;
		case EV_KEY7:
			ev = FS_KEY_7;
			break;	
		case EV_KEY8:
			ev = FS_KEY_8;
			break;
		case EV_KEY9:
			ev = FS_KEY_9;
			break;
	}
	if( ev != 0 )
		FS_SendKeyEvent( ev );
}

static UINT32 IFS_MainWinProc( UIWINDOW * win, UINT16 cmd, UINT16 wParam, UINT32 lParam )
{
	switch(cmd)
	{
		case WM_CREATE:
			break;
		case WM_DESTROY:
			GFS_HWND = NULL;
			break;
			
		case WM_KILLFOCUS:
			break;

		case WM_SETFOCUS:
			FS_GuiRepaint( );
			return TRUE;
		case WM_PAINT:
			return TRUE;
		
		case WM_COMMAND:
			IFS_SendKeyEventInd( wParam );
			return TRUE;
		case WM_USER:
			FS_HandlePostMessage( wParam, lParam );
			return TRUE;
		default:
			break;
	}
	return ClassWindowProc(win, cmd, wParam, lParam);
}

extern const KEYBOARD	MBKeyboard;

void IFS_ShowMainWin( UIWINDOW *parent, char nApp, char *msg )
{
	UIWINDOW *win;
	
	static const UIDIALOGTEMPLATE dlg =
	{
		 IFS_MainWinProc,
		 "Text",
		 "demoWin",
		 WS_ALIGNCENTER | WS_FULLSCREEN | WS_TICK | WS_NOTIFYPARENTONCLOSE, 		 
		 0, 0,								/*	x , y				   */
		 AUTOCONFIGURE, AUTOCONFIGURE,		/*	W , H				   */
		 (void *)NULL,						/*	Data				   */
		 0L,								/*	secondsLeft 	   */
		 0, 								/*	wParam			   */
		 &MBKeyboard,						/*	keyboard			   */
		 0,
		 NULL				 
	};

	win = DialogCreate( parent, &dlg, 8888 );
	GFS_HWND = win;
	if( UIGetLanguageGrp( UIIntlGetLanguage() ) == ASIAN_CHINESE )
		FS_SetLanguage( 1 );
	else
		FS_SetLanguage( 0 );
	
	if( nApp == 1 )
		FS_EmlMain( );
	else if( nApp == 2 )
		FS_WebMain( );
	else if( nApp == 3 )
		FS_MmsMain( );
	else if( nApp == 4 )
		FS_DcdOpenChannelList( );
	else if( nApp == 5 )
		FS_StkMain( );
}

UINT32 IFS_GBToUTF8Ex( char *pGBSstr, char *pUTF8Dstr)
{
	UINT16 UCS2_len;	
	UINT16 UTF8_len;
	UINT16 GB_len = 0;
	UINT8 *UCS2string;
	char *pSrcGB = (char *)pGBSstr ;
	if( NULL != pSrcGB && NULL != pUTF8Dstr )
	{
		GB_len = GSMstrlen( pSrcGB );
		UCS2string = GSMMalloc( (GB_len + 1) * 2 );
		UCS2_len = ConvertGBStr2Unicode_zcq( (char *)UCS2string, pSrcGB );
		UTF8_len = UCS2ToUTF8RequiredBytes( ( const UINT8 *)UCS2string, UCS2_len );
		UCS2ToUTF8( UCS2string, UCS2_len, (char *)pUTF8Dstr, UTF8_len );
		GSMFree( UCS2string );
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


BOOLEAN IFS_UTF8ToGBEx( char *pUTF8Sstr, char *pGBDstr)
{	
	UINT16 Ucs2Len = 0;
	UINT16 UTF8len = 0;
	char *pUCS2string	= NULL;
	if ( NULL != pUTF8Sstr && NULL != pGBDstr )
	{
		Ucs2Len = UTF8ToUCS2RequiredBytes( pUTF8Sstr );
		pUCS2string = (char *)GSMMalloc( Ucs2Len + 2 );
		UTF8len = (UINT16)GSMstrlen( pUTF8Sstr );
		Ucs2Len = UTF8ToUCS2( pUTF8Sstr, UTF8len, (UINT16 *)pUCS2string, UTF8len );
		ConvertUnicodeStrtoGB_DH( (char *)pUCS2string, pGBDstr, Ucs2Len * 2 );
		GSMFree( pUCS2string );
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

static UINT32 IFS_InputDialogProc( UIWINDOW *win, UINT16 cmd, UINT16 wParam, UINT32 lParam )
{
	static UINT16 PrevInputMethod = 0;
	switch( cmd )
	{
		case WM_COMMAND:
			if( EV_CHILDCLOSED == wParam && IDC_AMOI_EDIT == LOWORD(lParam) )
			{
				char *ret = NULL;
				UINT16 len;
				len = GSMstrlen((char *)g_sUZIEdit.UZITextBuf);
				
				if( IDC_AMOI_EDIT == LOWORD(lParam) && g_sUZIEdit.LeftSoftKey )
				{
					ret = g_sUZIEdit.UZITextBuf;
					g_input_callback( ret, len, g_input_context );
				}
				WindowDestroy( win );
				return TRUE;
			}
			break;
		case WM_DESTROY:
			ALConfig.UZI_InputMethod = PrevInputMethod ;
			break;
		default:
			break;
	}
	return ClassWindowProc( win, cmd, wParam, lParam );
}

static char *IFS_SysPath2FsPath( char *path )
{
	static char s_path[256];
	int i, len = GSMstrlen( path );
	GSMmemcpy( s_path, path, len + 1 );
	for( i = 0; i < len; i ++ )
	{
		if( s_path[i] == '\\' ) s_path[i] = '/';
	}
	return s_path;
}

static UINT32 IFS_FileDialogHOOK(UIWINDOW * win, UINT32 wParam, UINT32 lParam)
{
	char *fpath;
	if ( wParam == OFN_MSG_SELECT && lParam != 0 )
	{	
		#ifdef SIMULATION
		fpath = IFS_SysPath2FsPath( (char *)lParam );
		#else
		fpath = (char *)lParam;
		#endif
		g_fd_handle( fpath, g_fd_param );
	}
	CloseFileDlg(win->id);
	return 0;
}
//-------------------------------------------------------------------------------------------------
static UINT32 IFS_FileDialogProc( UIWINDOW *win, UINT16 cmd, UINT16 wParam, UINT32 lParam )
{
	switch(cmd)
	{
		case WM_PAINT:
			return TRUE;

		case WM_COMMAND:
			if(EV_CHILDCLOSED == wParam)
				WindowDestroy(win);
			return TRUE;
			
		default:
			break;
	}
	return ClassWindowProc( win, cmd, wParam, lParam );
}

#endif

void IFS_InputDialog( FS_CHAR * txt, FS_EditParam *eParam, FS_InputHandler cb, void *context )
{
#ifdef FS_PLT_WIN
	char *gbstr;
	int len;
	HFONT hFont;
	LOGFONT logFont = {0};
	logFont.lfHeight = 12;
	logFont.lfWidth = 0;
	logFont.lfWeight = FW_NORMAL;
	logFont.lfCharSet = GB2312_CHARSET;
	logFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	logFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	logFont.lfQuality = DEFAULT_QUALITY;
	logFont.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
	hFont = CreateFontIndirect( &logFont );

	if( eParam->limit_character )
	{
		eParam->max_len *= 2;
	}
	
	g_input_callback = cb;
	g_input_context = context;
	g_input_param = *eParam;
	GFS_hEditWnd = CreateWindow( "edit",NULL,WS_CHILD|ES_MULTILINE|ES_AUTOHSCROLL|ES_AUTOVSCROLL ,50,25,100,20,GFS_HWND,NULL,GFS_HINS,NULL );
	SendMessage( GFS_hEditWnd, WM_SETFONT, (WPARAM)hFont, 0 );
	SendMessage( GFS_hEditWnd, EM_LIMITTEXT, eParam->max_len, 0 );
	if( txt )
	{
		len = strlen( txt );
		gbstr = malloc( len + 1 );
		memset( gbstr, 0, len + 1 );
		FS_CnvtUtf8ToGBK( txt, len, gbstr, FS_NULL );
		SetWindowText( GFS_hEditWnd, gbstr );
		free( gbstr );
	}
	MoveWindow( GFS_hEditWnd, eParam->rect.left + 2, eParam->rect.top + 2, eParam->rect.width - 4, eParam->rect.height - 4, FALSE);
	ShowWindow( GFS_hEditWnd,SW_SHOW );
	SetFocus( GFS_hEditWnd );
	SendMessage( GFS_hEditWnd, EM_SETSEL, 0, -1 );  
#endif
#ifdef FS_PLT_VIENNA
	UINT16 PreTextLen = 0, MaxEditLen;
	E_UZI_INPUT_METHOD method;
	FS_SINT4 len;
	UIWINDOW *win = WindowCreateEx( IFS_InputDialogProc,
		"Text",
		"Edit dialog",
		WS_FULLSCREEN,
		0, 0,
		AUTOCONFIGURE, AUTOCONFIGURE,
		WindowGetTopmost( ),
		IDD_AMOI_EDITDLG,
		NULL,
		0,
		NULL,
		( FontType ) 1,
		(void *)NULL,	 /* private data */
		1,
		0 );

	len = eParam->max_len;
	g_input_callback = cb;
	g_input_context = context;
	if( eParam->preferred_method & FS_IM_123 )
		method = E_UZI_123;
	else if( eParam->preferred_method & FS_IM_ABC )
		method = E_UZI_ABC;
	else
		method = E_UZI_PINYIN;
	
	ALConfig.UZI_InputMethod = method;
	if( txt )
	{
		PreTextLen = GSMstrlen( txt );
	}
	MaxEditLen = (len <= 0) ? (UZI_EDIT_TEXT_BUF_LEN / 2) : len;
	UZI_CreateEditDialog( win, txt, PreTextLen, MaxEditLen/3, MaxEditLen, IDC_AMOI_EDIT, 0, NULL );
#endif
}

void IFS_FileDialogSave( FS_CHAR *preName, FS_FDHandler cb, void *param )
{
#ifdef FS_PLT_WIN
	OPENFILENAME ofn;		// common dialog box structure
	char szFile[260];		// buffer for file name

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = GFS_HWND;
	ofn.lpstrFile = szFile;
	//
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of szFile to initialize itself.
	//
	FS_CnvtUtf8ToGBK( preName, -1, szFile, FS_NULL );
	
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "All\0*.*\0";
	ofn.nFilterIndex = 0;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST;

	// Display the Open dialog box. 

	if( GetOpenFileName(&ofn) == TRUE ) 
	{
		char *utf8;
		int len;

		len = strlen( ofn.lpstrFile );
		utf8 = malloc( len * 3 / 2 + 1 );
		FS_CnvtGBKToUtf8( ofn.lpstrFile, len, utf8, FS_NULL );
		cb( utf8, param );
		free( utf8 );
	}
	else
	{
		cb( NULL, param );
	}
#endif
#ifdef FS_PLT_VIENNA
	char str[128];
	GSMstrcpy( str, "N:\\Image\\" );
	GSMstrcat( str, preName );
	cb( str, param );
#endif
}

void IFS_FileDialogOpen( FS_UINT1 filter, FS_FDHandler cb, void *param )
{
#ifdef FS_PLT_WIN
	OPENFILENAME ofn;		// common dialog box structure
	char szFile[260];		// buffer for file name

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = GFS_HWND;
	ofn.lpstrFile = szFile;
	//
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of szFile to initialize itself.
	//
	ofn.lpstrFile[0] = '\0';
	
	ofn.nMaxFile = sizeof(szFile);
	if( filter == FS_FDO_IMAGE )
		ofn.lpstrFilter = "Image File\0*.jpg;*.bmp;*.wbmp;*.png;*.gif;*.jpeg;\0All\0*.*\0\0";
	else if( filter == FS_FDO_AUDIO )
		ofn.lpstrFilter = "Audio File\0*.mp3;*.aac;*.amr;*.mid;*.midi;*.mmf;\0All\0*.*\0\0";
	else if( filter == FS_FDO_VIDEO )
		ofn.lpstrFilter = "Video File\0*.mp4;*.3gp;*.3gpp;*.avi;\0All\0*.*\0\0";
	else
		ofn.lpstrFilter = "All\0*.*\0\0";
	
	ofn.nFilterIndex = 0;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	// Display the Open dialog box. 

	if( GetOpenFileName(&ofn) == TRUE ) 
	{
		char *utf8;
		int len;

		len = strlen( ofn.lpstrFile );
		utf8 = malloc( len * 3 / 2 + 1 );
		FS_CnvtGBKToUtf8( ofn.lpstrFile, len, utf8, FS_NULL );
		cb( utf8, param );
		free( utf8 );
	}
	else
	{
		cb( NULL, param );
	}
#endif
#ifdef FS_PLT_VIENNA
	char *s_filter = "*.jpg|*.jpeg|*.png|*.gif|*.bmp|*.wbmp|*.txt|*.mp3|*.mp4|*.3gp|*.mid|*.midi|*.mmf|*.amr|*.jar|*.jad|*.rar|*.exe|*.*||";
	UIWINDOW *handlewin = NULL;
	OPENFILENAME ofn;
	g_fd_param = param;
	g_fd_handle = cb;

	handlewin = WindowCreateEx( IFS_FileDialogProc,
								"Text",
								"File dialog",
								WS_FULLSCREEN | WS_NOTIFYPARENTONCLOSE | WS_TICK,
								0, 0,
								AUTOCONFIGURE, AUTOCONFIGURE,
								WindowGetTopmost( ),
								IDD_AMOI_EDITDLG,
								NULL,
								0,
								NULL,
								( FontType ) 1,
								NULL,	 /* private data */
								1,
								0 );
	ofn.bOpenFileDlg = TRUE;
	ofn.pFilePath = NULL;
	ofn.flags = OFN_ENABLEHOOK | OFN_VIEWIMAGE | OFN_PLAYSOUND;
	ofn.pszFilter = s_filter;
	ofn.win = handlewin;
	ofn.pfnHookProc = IFS_FileDialogHOOK;
	AmoiFileDialog( &ofn );
#endif
}
