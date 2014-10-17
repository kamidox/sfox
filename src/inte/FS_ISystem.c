#include "inc/inte/FS_Inte.h"

#ifdef FS_PLT_WIN
#include <windows.h>
#include "resource.h"
#include <time.h>
#include <stdio.h>
#include <setjmp.h>
#include "jpeglib.h"
#include "gif_lib.h"
#include "inc/util/FS_MemDebug.h"

extern HWND GFS_HWND;
extern HWND hwndEdit;
extern HINSTANCE GFS_HINS;

HANDLE GFS_MusicPlayer;
#define WM_CLIENT	(WM_USER + 2)
FS_CHAR *Utf8ToGb2312( FS_CHAR *utf8 );

#endif

#ifdef FS_PLT_VIENNA

#include "..\system\portab.h"
#include "..\system\sysprim.h"
#include "..\global\gsmerror.h"
#include "..\global\types.h"
#include "..\system\syslib.h"
#include "..\uh\amoiuh.h"
#include "..\..\custy9\uh\common\amouhdcaps.h"
#include "..\global\Gsmprim.h"
#include "..\ui\uif5\win\uifwindo.h"
#include "..\config\mboxes.h"
#include "..\uh\amoiuh.h"
#include "alidsymb.h"

extern UIWINDOW *GFS_HWND;
#endif

typedef struct FS_ITimerData_Tag
{
	FS_UINT4 			id;
	void *				param;
	FS_TimerHandler		handle;
}FS_ITimerData;

#define IFS_TIMER_COUNT		10

static FS_ITimerData IGFS_TimerTable[IFS_TIMER_COUNT];

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

#ifdef FS_PLT_WIN
void CALLBACK IFS_TimerProc( HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime )
{
	FS_ITimerData *pTD = IFS_GetTimerData( idEvent );
	KillTimer( GFS_HWND, idEvent );
	if( pTD )
	{
		pTD->handle( pTD->param );
		pTD->id = 0;
	}
}
#endif

#ifdef FS_PLT_VIENNA
void IFS_TimerHandle( UINT32 *pid )
{
	FS_ITimerData *pTD = IFS_GetTimerData( *pid );
	if( pTD )
	{
		pTD->handle( pTD->param );
		pTD->id = 0;
	}
	GSMFree( pid );
}
#endif

FS_UINT4 IFS_StartTimer(FS_UINT4 id, FS_UINT4 msecs, FS_TimerHandler cb, void *param )
{
	IFS_SaveTimerData( id, cb, param );
#ifdef FS_PLT_WIN
	SetTimer( GFS_HWND, id, msecs, IFS_TimerProc );
	return id;
#endif
#ifdef FS_PLT_VIENNA
	{
		FS_UINT4 *pid = GSMMalloc( sizeof(FS_UINT4) );
		*pid = id;
		ALStartTimer( id, MSECS(msecs), IFS_TimerHandle, pid );
		return id;
	}
#endif
}

void IFS_StopTimer( FS_UINT4 id )
{
	FS_ITimerData *pTD = IFS_GetTimerData( id );
	if( pTD )
	{
		pTD->id = 0;
	}
#ifdef FS_PLT_WIN
	KillTimer( GFS_HWND, id );
#endif
#ifdef FS_PLT_VIENNA
	ALStopTimer( id );
#endif
}

FS_COLOR IFS_DDB_RGB( FS_BYTE r, FS_BYTE g, FS_BYTE b )
{
#ifdef FS_PLT_WIN
	return ((r << 16) | (g << 8) | b) & 0xFFFFFF;
#endif
#ifdef FS_PLT_VIENNA
	return (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3)) & 0xFFFF;
#endif
}

FS_BYTE IFS_DDB_RED( FS_COLOR clr )
{
#ifdef FS_PLT_WIN
	return (clr >> 16) & 0xFF;
#endif
#ifdef FS_PLT_VIENNA
	return (clr >> 8) & 0xF8;
#endif
}

FS_BYTE IFS_DDB_GREEN( FS_COLOR clr )
{
#ifdef FS_PLT_WIN
	return (clr >> 8) & 0xFF;
#endif
#ifdef FS_PLT_VIENNA
	return (clr >> 3) & 0xFC;
#endif
}

FS_BYTE IFS_DDB_BLUE( FS_COLOR clr )
{
#ifdef FS_PLT_WIN
	return clr & 0xFF;
#endif
#ifdef FS_PLT_VIENNA
	return (clr << 3) & 0xF8;
#endif
}

FS_CHAR *IFS_GetRootDir( void )
{
#ifdef FS_PLT_WIN
	return "C:\\sfox";
#endif
#ifdef FS_PLT_VIENNA
	#ifdef SIMULATION
		return "/usr/F_SOFT";
	#else
		return "N:\\F_SOFT";
	#endif
#endif
}

FS_CHAR *IFS_GetPathSep( void )
{
#ifdef FS_PLT_WIN
	return "\\";
#endif
#ifdef FS_PLT_VIENNA
	#ifdef SIMULATION
		return "/";
	#else
		return "\\";
	#endif
#endif
}

FS_SINT4 IFS_GetScreenWidth( void )
{
#ifdef FS_PLT_WIN
	return 176;
#endif
#ifdef FS_PLT_VIENNA
	return 240;
#endif
}

FS_SINT4 IFS_GetScreenHeight( void )
{
#ifdef FS_PLT_WIN
	return 220;
#endif
#ifdef FS_PLT_VIENNA
	return 320;
#endif
}

FS_SINT4 IFS_GetWinTitleHeight( void )
{
#ifdef FS_PLT_WIN
	return 20;
#endif
#ifdef FS_PLT_VIENNA
	return 32;
#endif
}

FS_BOOL FS_ImageGetSize( char *filename, int *w, int *h );
FS_UINT4 FS_ImageDecode( FS_CHAR *file, FS_SINT4 w, FS_SINT4 h, FS_Bitmap *pBmp );

FS_Bitmap *IFS_GetWinTitleBgImg( void )
{
#ifdef FS_MODULE_IMAGE
	FS_CHAR file[128];
	FS_SINT4 w = 0, h = 0;
	FS_Bitmap *pBmp = FS_NULL;
	
	IFS_Sprintf( file, "%s%s%s", IFS_GetRootDir(), IFS_GetPathSep(), "title_bg.bmp" );
	FS_ImageGetSize( file, &w, &h);
	if( w > 0 && h > 0 ){
		pBmp = IFS_Malloc( sizeof(FS_Bitmap) );
		IFS_Memset( pBmp, 0, sizeof(FS_Bitmap) );
		FS_ImageDecode( file, w, h, pBmp );
	}
	return pBmp;
#endif
	return FS_NULL;
}

void IFS_ReleaseWinTitleBgImg( FS_Bitmap *pBmp )
{
#ifdef FS_MODULE_IMAGE
	#ifdef FS_DEBUG_	
	void FS_DebugFree( void *ptr, FS_CHAR *file, FS_SINT4 line );
	FS_DebugFree( pBmp->bits, __FILE__, __LINE__ );
	#else
	IFS_Free( pBmp->bits );
	#endif
	IFS_Free( pBmp );
#endif
}

FS_SINT4 IFS_GetSoftkeyHeight( void )
{
#ifdef FS_PLT_WIN
	return 20;
#endif
#ifdef FS_PLT_VIENNA
	return 32;
#endif
}

FS_Bitmap *IFS_GetSoftkeyBarBgImg( void )
{
	return IFS_GetWinTitleBgImg( );
}

void IFS_ReleaseSoftkeyBarBgImg( FS_Bitmap *pBmp )
{
	IFS_ReleaseWinTitleBgImg( pBmp );
}

FS_SINT4 IFS_GetWidgetSpan( void )
{
#ifdef FS_PLT_WIN
	return 4;
#endif
#ifdef FS_PLT_VIENNA
	return 6;
#endif
}

FS_SINT4 IFS_GetLineHeight( void )
{
#ifdef FS_PLT_WIN
	return 20;
#endif
#ifdef FS_PLT_VIENNA
	return 32;
#endif
}

FS_SINT4 IFS_DcdGetChannelTabHeight( void )
{
#ifdef FS_PLT_WIN
		return 6;
#endif
#ifdef FS_PLT_VIENNA
		return 10;
#endif
}

FS_SINT4 IFS_DcdGetIdleDetailHeight( void )
{
	return 3 * IFS_GetLineHeight();
}

FS_UINT1 IFS_GetBitsPerPixel( void )
{
#ifdef FS_PLT_WIN
	return 24;
#endif
#ifdef FS_PLT_VIENNA
	return 16;
#endif
}

FS_BYTE *IFS_GetScreenBitmap( void )
{
#ifdef FS_PLT_WIN
	FS_BYTE * pBits;
	pBits = IFS_Malloc( 3 * IFS_GetScreenWidth( ) * IFS_GetScreenHeight( ) );
	return pBits;
#endif
#ifdef FS_PLT_VIENNA
	extern SCREEN ALScreenLogical;
	return ALScreenLogical.Pixels;
#endif
}

void IFS_ReleaseScreenBitmap( FS_BYTE *pBits )
{
#ifdef FS_PLT_WIN
	if( pBits ) IFS_Free( pBits );
#endif
#ifdef FS_PLT_VIENNA
#endif
}

void IFS_SystemExit( void )
{
#ifdef FS_PLT_WIN
	#ifdef FS_MODULE_DCD
	FS_DcdDeinit( );
	#endif
	#ifdef FS_MODULE_STK
	FS_StockDeinit( );
	#endif	
	//DestroyWindow( GFS_HWND );
	PostMessage( GFS_HWND, WM_DESTROY, 0, 0 );
#endif
#ifdef FS_PLT_VIENNA
	WindowDestroyByID( 8888 );
#endif
}

FS_BOOL IFS_SystemRuningBackgroud( void )
{
#ifdef FS_PLT_WIN
	return FS_FALSE;
#endif
#ifdef FS_PLT_VIENNA
	if( WindowFindId( 8888 ) )
	{
		return FS_FALSE;
	}
	else
	{
		return FS_TRUE;
	}
#endif
}

FS_BOOL IFS_SystemAllowDrawScreen( void )
{
#ifdef FS_PLT_WIN
	return FS_TRUE;
#endif
#ifdef FS_PLT_VIENNA
	if( WindowGetTopmost( ) == GFS_HWND )
	{
		return FS_TRUE;
	}
	else
	{
		return FS_FALSE;
	}
#endif
}

FS_BOOL IFS_DcdCanDrawIdle( void )
{
#ifdef FS_PLT_WIN
	return FS_TRUE;
#endif
#ifdef FS_PLT_VIENNA
	if( WindowGetTopmost( ) == WindowFindId( IDW_READYDIALOG ) )
	{
		return FS_TRUE;
	}
	else
	{
		return FS_FALSE;
	}
#endif
}

void IFS_EnterBackLight( void )
{
}

void IFS_LeaveBackLight( void )
{
}

void IFS_PostMessage( FS_UINT2 msg, FS_UINT4 param )
{
#ifdef FS_PLT_WIN
	PostMessage( GFS_HWND, WM_CLIENT, msg, param );
#endif
#ifdef FS_PLT_VIENNA
	UHRECT rect = { 0 };
	rect.w = IFS_GetScreenWidth( );
	rect.h = IFS_GetScreenHeight( );
	PostMessage( GFS_HWND, WM_USER, msg, param );
	UHDIScreenUpdate( &rect );
#endif
}

void IFS_InvalidateRect( FS_Rect *pRect )
{
#ifdef FS_PLT_WIN
	RECT area = {0, 0, IFS_GetScreenWidth( ), IFS_GetScreenHeight( )};
	if( pRect )
	{
		area.left = pRect->left;
		area.right = pRect->left + pRect->width + 1;
		area.top = pRect->top;
		area.bottom = pRect->top + pRect->height;
	}
	InvalidateRect( GFS_HWND, &area, TRUE );
	//SendMessage( GFS_HWND, WM_PAINT, 0, &area );
#endif
#ifdef FS_PLT_VIENNA
	UHRECT *rect = GSMMalloc( sizeof(UHRECT) );
	if( pRect )
	{
		rect->x = (INT16)pRect->left;
		rect->y = (INT16)pRect->top;
		rect->w = (UINT16)pRect->width;
		rect->h = (UINT16)pRect->height;
	}
	else
	{
		rect->x = 0;
		rect->y = 0;
		rect->w = IFS_GetScreenWidth( );
		rect->h = IFS_GetScreenHeight( );
	}
	GSMSendMessage( DISAP, DIScreenUpdate, rect );
#endif
}

FS_CHAR * IFS_GetUserAgent( void )
{
	//return "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0; MyIE2; .NET CLR 1.1.4322)";
	//return "Optimay-OptiVerse/2.0 UP.Browser/6.2.3.8.c.1.101 (GUI) MMP/2.0";
	//return "Amoi-A675/Plat-V-FT/WAP2.0/MIDP2.0/CLDC1.0";
	//return "Amoi-M636/Plat-V-FT/WAP2.0";
	return "MOT-Z3/AAUG2135 AA Release/07.18.2006 MIB/BER2.2 Profile/MIDP-2.0  Configuration/CLDC-1.1 EGE/1.0 Software/08.02.05R";
	//return "MOT-K1/08.22.07R MIB/BER2.2 Profile/MIDP-2.0 Configuration/CLDC-1.0";
}

FS_CHAR * IFS_GetUaProfile( void )
{
	//return "http://devgate2.openwave.com/uaprof/OPWVSDK70.xml";
	//return "http://nds1.nds.nokia.com/uaprof/N6230ir200.xml";
	return FS_NULL;
}

/* 是否是国际漫游状态 */
FS_BOOL IFS_IsInternationalRoaming( void )
{
	return FS_FALSE;
}

#ifdef FS_PLT_WIN

extern FS_FileRead( FS_SINT4 dir, FS_CHAR * filename, FS_SINT4 offset, void * buf, FS_SINT4 blen );
FS_SINT4 FS_FileGetSize( FS_SINT4 dir, FS_CHAR *filename );

/*------------------------------- jpeg decoder -----------------------------------------*/
struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;		/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

static void my_error_exit( j_common_ptr cinfo )
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

FS_BOOL IFS_JPEGGetSize( char *filename, int *w, int *h )
{
	FS_BOOL ret = 0;
	FS_Handle infile;
	int bpp;
	
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;

	*w = *h = 0;

	if( ! IFS_FileOpen( &infile, filename, FS_OPEN_READ ) )
		return 0;

	/* Step 1: allocate and initialize JPEG decompression object */

	/* We set up the normal JPEG error routines, then override error_exit. */
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer))
	{
		/* If we get here, the JPEG code has signaled an error.
		 * We need to clean up the JPEG object, close the input file, and return.
		 */
		jpeg_destroy_decompress(&cinfo);
		IFS_FileClose( infile );
		return ret;
	}
	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source (eg, a file) */

	jpeg_stdio_src(&cinfo, infile);

	/* Step 3: read file parameters with jpeg_read_header() */

	jpeg_read_header(&cinfo, TRUE);

	bpp = IFS_GetBitsPerPixel( );
	bpp = (bpp + 7) >> 3;
	if( bpp == cinfo.num_components )
	{
		*w = cinfo.image_width;
		*h = cinfo.image_height;
		ret = 1;
	}
	else
	{
		ret = 0;
	}
	jpeg_destroy_decompress(&cinfo);

	IFS_FileClose( infile );

	return ret;
}

FS_UINT4 IFS_JPEGDecode( char *filename, int w, int h, FS_Bitmap *pBmp )
{
	JSAMPARRAY buffer;
	unsigned char *out_pixels, *inptr;
	int col, i, nLine = -1, nLastLine, nRow = -1, nLastRow;
	FS_UINT1 bpp;
	double factor;
	FS_Handle infile;
	
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;

	if( ! IFS_FileOpen( &infile, filename, FS_OPEN_READ ) )
		return 0;

	/* Step 1: allocate and initialize JPEG decompression object */

	/* We set up the normal JPEG error routines, then override error_exit. */
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer))
	{
		/* If we get here, the JPEG code has signaled an error.
		 * We need to clean up the JPEG object, close the input file, and return.
		 */
		jpeg_destroy_decompress(&cinfo);
		IFS_FileClose( infile );
		return 0;
	}
	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source (eg, a file) */
	jpeg_stdio_src(&cinfo, infile);

	/* Step 3: read file parameters with jpeg_read_header() */
	jpeg_read_header(&cinfo, TRUE);

	/* Step 4: set decompress param */

	/* Step 5: Start decompressor */
	jpeg_start_decompress(&cinfo);

	factor = (double)w / cinfo.output_width;
	bpp = IFS_GetBitsPerPixel( );
	bpp = (bpp + 7) >> 3;
	pBmp->width = w;
	pBmp->height = h;
	pBmp->bpp = bpp;
	pBmp->pitch = w * bpp;	/* TODO cinfo.output_components */;
	pBmp->bits = malloc( pBmp->pitch * (pBmp->height + 2) );
	
	i = 0;
	out_pixels = pBmp->bits;
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, ((cinfo.output_width + 3) * bpp), 1);
	
	/* Step 6: while (scan lines remain to be read) */
	/* Here we use the library's state variable cinfo.output_scanline as the
	* loop counter, so that we don't have to keep track ourselves.
	*/
	while (cinfo.output_scanline < cinfo.output_height)
	{
		/* jpeg_read_scanlines expects an array of pointers to scanlines.
		 * Here the array is only one element long, but you could ask for
		 * more than one scanline at a time if that's more convenient.
		 */
		jpeg_read_scanlines(&cinfo, buffer, 1);

		nLastLine = nLine;
		nLine = (int)(cinfo.output_scanline * factor);
		if( nLine != nLastLine )
		{
			/* Assume put_scanline_someplace wants a pointer and sample count. */
			out_pixels = pBmp->bits + ( i * pBmp->pitch );
			i ++;
			inptr = buffer[0];
			for( col = 0; col < (int)cinfo.output_width; col ++ )
			{
				nLastRow = nRow;
				nRow = (int)( col * factor );
				if( nRow != nLastRow )
				{
					out_pixels[2] = inptr[0];	/* can omit GETJSAMPLE() safely */
					out_pixels[1] = inptr[1];
					out_pixels[0] = inptr[2];
					out_pixels += 3;
				}
				inptr += 3;
			}
		}
	}
	
	/* Step 7: Finish decompression */
	jpeg_finish_decompress(&cinfo);

	/* Step 8: Release JPEG decompression object */
	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress(&cinfo);

	/* After finish_decompress, we can close the input file.
	* Here we postpone it until after no more JPEG errors are possible,
	* so as to simplify the setjmp error logic above.  (Actually, I don't
	* think that jpeg_destroy can do an error exit, but why assume anything...)
	*/
	IFS_FileClose( infile );
	return 0;
}

/*------------------------------- gif decoder -----------------------------------------*/
typedef struct GIFInputStream_Tag
{
	unsigned char *		buffer;
	UINT				buf_len;
	UINT				offset;
}GIFInputStream;

static int IFS_GIFReadStream( GifFileType *gifFile, GifByteType *buf, int len )
{
	int ret;
	GIFInputStream *iStream = (GIFInputStream *)gifFile->UserData;

	if( iStream->offset + len < iStream->buf_len )
	{
		ret = len;
	}
	else
	{
		ret = iStream->buf_len - iStream->offset;
	}

	if( ret > 0 )
	{
		memcpy( buf, iStream->buffer + iStream->offset, ret );
		iStream->offset += ret;
	}

	return ret;
}

static IFS_GIFCopyLine( FS_BYTE *pDst, FS_BYTE *pSrc, int width, int x, int factor, GifColorType *pColorTable )
{
	GifPixelType pixel;
	GifColorType *pColor;
	if( width )
	{
		do {
			pixel = *pSrc;
			if( x % factor == 0 )
			{
				pColor = pColorTable + pixel;
				pDst[0] = pColor->Blue;
				pDst[1] = pColor->Green;
				pDst[2] = pColor->Red;
				pDst += 3;
			}

			x ++;
			pSrc ++;
			width --;
		} while( width );
	}
}

FS_BOOL IFS_GIFGetSize( char *filename, int *w, int *h )
{
	GIFInputStream *iStream;
	unsigned long rlen;
	FS_BOOL ret = FALSE;
	GifFileType *GIFFile;
	HANDLE hFile;

	*w = *h = 0;
	hFile = CreateFile( filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0 );
	if( hFile == 0 || (unsigned long)hFile == 0xFFFFFFFF )
		return ret;
	
	iStream = (GIFInputStream *) malloc( sizeof(GIFInputStream) );
	iStream->buf_len = GetFileSize( hFile, NULL );
	iStream->offset = 0;
	iStream->buffer = (unsigned char *)malloc( iStream->buf_len );
	ReadFile( hFile, iStream->buffer, iStream->buf_len, &rlen, NULL) ;
	CloseHandle( hFile );
	GIFFile = DGifOpen( iStream, IFS_GIFReadStream );

	if( GIFFile )
	{
		ret = TRUE;
		*w = GIFFile->SWidth;
		*h = GIFFile->SHeight;
		DGifCloseFile( GIFFile );
	}

	free( iStream->buffer );
	free( iStream );
	
	return ret;
}

FS_UINT4 IFS_GIFDecode( char *filename, int w, int h, FS_Bitmap *pBmp )
{	
	GIFInputStream *iStream;
	unsigned long rlen;
	FS_BOOL ret = FALSE;
	GifFileType *GIFFile;
	int sW, sH, x, y, iW, iH, pass, i, factor;
	int ExtCode;
	GifByteType *pExt;
	GifPixelType * pLine = NULL;
	FS_COLOR GIFBgColor = 0xFFFFFF;
	FS_BYTE *pDst;
	GifRecordType RecordType;
	GifColorType *pColorTable;
	const int InterlacedOffset[] = { 0, 4, 2, 1 }; /* The way Interlaced image should. */
	const int InterlacedJumps[] = { 8, 8, 4, 2 };    /* be read - offsets and jumps... */
	HANDLE hFile;

	if( filename == NULL || w <= 0 || h <= 0 )
		return 0;

	hFile = CreateFile( filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0 );
	if( hFile == 0 || (unsigned long)hFile == 0xFFFFFFFF )
		return FS_NULL;
	
	iStream = (GIFInputStream *) malloc( sizeof(GIFInputStream) );
	iStream->buf_len = GetFileSize( hFile, NULL );
	iStream->offset = 0;
	iStream->buffer = (unsigned char *)malloc( iStream->buf_len );
	ReadFile( hFile, iStream->buffer, iStream->buf_len, &rlen, NULL) ;
	CloseHandle( hFile );
	GIFFile = DGifOpen( iStream, IFS_GIFReadStream );
	
	if( GIFFile )
	{
		ret = TRUE;
		sW = GIFFile->SWidth;
		sH = GIFFile->SHeight;
		
		factor = sW / w;
		pBmp->width = w;
		pBmp->height = h;
		pBmp->bpp = 3;
		pBmp->pitch = pBmp->width * pBmp->bpp;
		pBmp->bits = malloc( pBmp->pitch * (pBmp->height + 2) );

		pLine = malloc( GIFFile->SWidth * sizeof(GifPixelType) );
		/* get GIF backgroud */
		if( GIFFile->SColorMap )
		{
			GifColorType *pBgColor;
			if( GIFFile->SColorMap->ColorCount < GIFFile->SBackGroundColor )
			{
				pBgColor = GIFFile->SColorMap->Colors + GIFFile->SBackGroundColor;
				GIFBgColor = IFS_DDB_RGB( pBgColor->Red, pBgColor->Green, pBgColor->Blue );
			}
		}
		/* fill GIF backgroud */
		pDst = pBmp->bits;
		for( y = 0; y < h; y ++ )
		{
			for( x = 0; x < w; x ++ )
			{
				pDst[0] = GetBValue( GIFBgColor );
				pDst[1] = GetGValue( GIFBgColor );
				pDst[2] = GetRValue( GIFBgColor );
				pDst += 3;
			}
		}

		/* get GIF first frame */
		do {
			if( DGifGetRecordType( GIFFile, &RecordType ) == GIF_ERROR ) break;

			if( RecordType == IMAGE_DESC_RECORD_TYPE )
			{
				if( DGifGetImageDesc( GIFFile ) != GIF_ERROR )
				{
					ret = TRUE;
					x = GIFFile->Image.Left;
					y = GIFFile->Image.Top;

					if( GIFFile->Image.ColorMap )
						pColorTable = GIFFile->Image.ColorMap->Colors;
					else
						pColorTable = GIFFile->SColorMap->Colors;

					iW = GIFFile->Image.Width;
					iH = GIFFile->Image.Height;
					if( GIFFile->Image.Interlace )
					{
						// Need to perform 4 passes on the images:
						for( pass = 0; pass < 4; pass ++ )
						{
							for( i = InterlacedOffset[pass]; i < iH; i += InterlacedJumps[pass] )
							{
								if( DGifGetLine( GIFFile, pLine, iW ) == GIF_ERROR )
								{
									ret = FALSE;
									goto DONE;
								}
								if( factor == 1 || (i + y) % factor == 0 )
								{
									pDst = pBmp->bits + ( ((i + y) / factor) * pBmp->pitch + (x / factor) * pBmp->bpp );
									if( pDst < pBmp->bits + pBmp->pitch * pBmp->height )
										IFS_GIFCopyLine( pDst, pLine, iW, x, factor, pColorTable );
									else
										goto DONE;
								}
							}
						}
					}
					else
					{
						for( i = 0; i < iH; i ++ )
						{
							if( DGifGetLine( GIFFile, pLine, iW ) == GIF_ERROR )
							{
								ret = FALSE;
								goto DONE;
							}
							if( factor == 1 || (i + y) % factor == 0 )
							{								
								pDst = pBmp->bits + ( ((i + y) / factor) * pBmp->pitch + (x / factor) * pBmp->bpp );
								if( pDst < pBmp->bits + pBmp->pitch * pBmp->height )
									IFS_GIFCopyLine( pDst, pLine, iW, x, factor, pColorTable );
								else
									goto DONE;
							}
						}
					}

					/* we just get the gif first frame */
					goto DONE;
				}
			}
			else if( RecordType == EXTENSION_RECORD_TYPE )
			{
				/* Skip any extension blocks in file: */
				if( DGifGetExtension( GIFFile, &ExtCode, &pExt ) == GIF_ERROR ) 
					goto DONE;
				
				while( pExt != NULL )
				{
					if( DGifGetExtensionNext( GIFFile, &pExt ) == GIF_ERROR)
						goto DONE;
				}
			}
		} while( RecordType != TERMINATE_RECORD_TYPE );
	}

DONE:
	if( GIFFile ) DGifCloseFile( GIFFile );
	free( iStream->buffer );
	free( iStream );
	if( pLine ) free( pLine );
	return (FS_UINT4)pBmp;
}

#endif

#ifdef FS_MODULE_GIF
FS_BOOL FS_GifGetSize( char *filename, int *w, int *h );
FS_UINT4 FS_GifDecode( FS_CHAR *filename, FS_SINT4 w, FS_SINT4 h, FS_Bitmap *pBmp );
void FS_GifDestroy( FS_UINT4 hIm );
#endif

#ifdef FS_MODULE_PNG
FS_UINT4 FS_PngDecode( char *filename, int w, int h, FS_Bitmap *pBmp );
FS_BOOL FS_PngGetSize( char *filename, int *w, int *h );
#endif

FS_BOOL IFS_ImageGetSize( char *filename, int *w, int *h )
{
#ifdef FS_PLT_VIENNA
	UINT8 type;
	return Amoi_GetImgInfo( FS_NULL, 0, filename, &type, w, h );
#endif
	return FS_FALSE;
}

FS_UINT4 IFS_ImageDecode( FS_CHAR *file, FS_SINT4 w, FS_SINT4 h, FS_Bitmap *pBmp )
{
#ifdef FS_PLT_VIENNA
	UHBITMAP bmp;
	FS_Bitmap * pRetBmp = pBmp;
	GSMmemset( &bmp, 0, sizeof(UHBITMAP) );
	if ( Amoi_DecodeImg( FS_NULL, 0, file, &bmp, w, h, FALSE ) )
	{
		pRetBmp->width = bmp.Width;
		pRetBmp->height = bmp.Height;
		pRetBmp->bpp = bmp.BitsPerPixel >> 3;
		pRetBmp->bits = bmp.Pixels;
		pRetBmp->pitch = pRetBmp->bpp * bmp.Width;
	}
	return pRetBmp;
#endif
	return 0;
}

void IFS_ImageRelease( FS_UINT4 hImage, FS_Bitmap *pBmp )
{
#ifdef FS_PLT_VIENNA
	GSMFree( pBmp->bits );
	pBmp->bits = FS_NULL;
#endif
}

void IFS_ImageDestroy( FS_UINT4 hImage )
{
}

FS_BOOL IFS_PlayAudio( FS_CHAR *file, FS_SINT4 nTimes )
{
#ifdef FS_PLT_WIN
	FS_BOOL ret = FS_FALSE;
	FS_CHAR *gbfile;
	SHELLEXECUTEINFO ShellInfo;

	gbfile = Utf8ToGb2312( file );
	memset( &ShellInfo, 0, sizeof(SHELLEXECUTEINFO) );
	ShellInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShellInfo.fMask = SEE_MASK_CONNECTNETDRV | SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
	ShellInfo.hwnd = GFS_HWND;
	ShellInfo.lpVerb = "open";
	ShellInfo.lpFile = gbfile;
	ShellInfo.nShow = SW_HIDE;

	if( ShellExecuteEx( &ShellInfo ) )
	{
		ret = FS_TRUE;
		GFS_MusicPlayer = ShellInfo.hProcess;
	}
	else
	{
		GFS_MusicPlayer = 0;
	}
	free( gbfile );
	return ret;
#endif
#ifdef FS_PLT_VIENNA
	#ifndef SIMULATION
	ALPlaySoundHFS(0, 1, 100, 20, file);
	#endif
	return FS_TRUE;
#endif	
}

void IFS_StopAudio( void )
{
#ifdef FS_PLT_WIN
	if( GFS_MusicPlayer > 0 )
	{
		TerminateProcess( GFS_MusicPlayer, 0 );
	}
#endif
#ifdef FS_PLT_VIENNA
	#ifndef SIMULATION
	ALStopSound(20);
	#endif
#endif	
}

void IFS_PBSelectPhoneNumber( FS_PBHandler cb, void *param )
{
	cb("13859901820, 5534567", param);
}

FS_CHAR *IFS_PBFindNameByPhone( FS_CHAR *phoneNum )
{
	return "Joey.Huang";
}

void IFS_WriteNewSms( FS_CHAR *text, FS_CHAR *num )
{
}

FS_BOOL IFS_PushInd( FS_UINT1 tid, FS_UINT1 app_id, FS_CHAR *app_str, 
		FS_UINT2 ct_code, FS_CHAR *content_type, FS_BYTE *data, FS_SINT4 len )
{
	return FS_FALSE;
}

FS_SINT4 IFS_GetMaxCacheItem( void )
{
	return 50;
}

FS_SINT4 IFS_GetMaxCacheSize( void )
{
	return 200 * 1024;
}

FS_SINT4 IFS_GetMaxMmsSizeLimit( void )
{
	return 1024 * 1024;
}


FS_SINT4 IFS_GetMaxMmsItemLimit( void )
{
	return 100;
}

FS_SINT4 IFS_GetMaxMmsSize( void )
{
	return 100 * 1024;	// GPRS终端为100KB，3G终端为300KB
}

FS_SINT4 IFS_GetMaxMmsFrames( void )
{
	return 20;			// 中国移动规范规定为20帧
}

FS_SINT4 IFS_GetMaxPushItemLimit( void )
{
	return 10;
}

FS_SINT4 IFS_GetMaxEmlSizeLimit( void )
{
	return 1024 * 1024;
}

FS_SINT4 IFS_GetMaxEmlItemLimit( void )
{
	return 200;
}

/* notify user: a new push message recieved */
void IFS_PushNotify( FS_CHAR *title, FS_CHAR *url )
{
	
}

/* notify user: push list is full. may flash icon is something */
void IFS_PushListFull( void )
{
	
}

/* notify user: a new mms push arrived */
void IFS_MmsNotify( FS_BOOL is_full, FS_BOOL is_too_large, FS_CHAR *from, FS_CHAR *subject )
{
	
}

/* notify user: auto retrived result*/
void IFS_MmsAutoRecvNotify( FS_BOOL bSuccess, FS_CHAR *from, FS_CHAR *subject )
{
#ifdef FS_PLT_VIENNA
	if( bSuccess )
		AmoiDialogNotifyExMx( WindowGetTopmost(), 0, "mms auto retrived ok", 0, 0, TRUE, 2 );
	else
		AmoiDialogNotifyExMx( WindowGetTopmost(), 0, "mms auto retrived failed!!!", 0, 0, TRUE, 2 );
#endif
}

#define IFS_MEM_POOL_SIZE 		(600 * 1024)
FS_UINT4 IFS_GetMemPoolSize( void )
{
	return IFS_MEM_POOL_SIZE;
}

void *IFS_GetMemPool( void )
{
#ifdef FS_PLT_WIN
	return malloc( IFS_MEM_POOL_SIZE );
#endif
#ifdef FS_PLT_VIENNA
	return GSMMallocNULL( IFS_MEM_POOL_SIZE );
#endif
}

void IFS_ReleaseMemPool( void *mem )
{
#ifdef FS_PLT_WIN
	free( mem );
#endif
#ifdef FS_PLT_VIENNA
	GSMFree( mem );
#endif
}

FS_BYTE *IFS_GetCharBitmap( FS_UINT2 nChar )
{
#ifdef FS_PLT_VIENNA
	UHBITMAP *bmp = UHALFontGetCharBitmap( nChar, 0, Font24X24 );
	return bmp->Pixels;
#else
	return NULL;
#endif
}

FS_SINT4 IFS_GetCharWidth( FS_UINT2 nChar )
{
#ifdef FS_PLT_VIENNA
	UHBITMAP *bmp = UHALFontGetCharBitmap( nChar, 0, Font24X24 );
	return bmp->Width;
#else
	return 0;
#endif
}

FS_SINT4 IFS_GetCharHeight( FS_UINT2 nChar )
{
#ifdef FS_PLT_VIENNA
	UHBITMAP *bmp = UHALFontGetCharBitmap( nChar, 0, Font24X24 );
	return bmp->Height;
#else
	return 0;
#endif
}

FS_SINT4 IFS_GetFontMaxHeight( void )
{
#ifdef FS_PLT_VIENNA
	return 24;
#else
	return 0;
#endif
}

void IFS_WtaiMakeCall( FS_CHAR *phnum )
{
	
}

void IFS_WtaiAddPBEntry( FS_CHAR *phnum, FS_CHAR *name )
{
	
}

FS_CHAR *IFS_JvmCheckJad( FS_CHAR *jadFile )
{
	static FS_CHAR jarUrl[1024];
	
	FS_CHAR *str, *p, *buf;
	FS_SINT4 len;

	len = FS_FileGetSize( -1, jadFile );
	if( len <= 0 ) return FS_NULL;
	buf = IFS_Malloc( len + 1 );
	IFS_Memset( buf, 0, len + 1 );
	FS_FileRead( -1, jadFile, 0, buf, len );
	str = IFS_Strstr( buf, "MIDlet-Jar-URL:" );
	if( str == FS_NULL ){
		IFS_Free( buf );
		return FS_NULL;
	}
	str += IFS_Strlen( "MIDlet-Jar-URL:" );
	while( *str == ' ' || *str == '\t' ) str ++;
	p = IFS_Strchr( str, '\n' );
	if( p == FS_NULL ){
		IFS_Free( buf );
		return FS_NULL;
	}
	if( *(p - 1) == '\r' ) p --;
	len = p - str;
	if( len > sizeof(jarUrl) + 1 ){
		IFS_Free( buf );
		return FS_NULL;
	}
	IFS_Memset( jarUrl, 0, len + 1 );
	IFS_Memcpy( jarUrl, str, len );
	
	IFS_Free( buf );
	return jarUrl;
}

FS_BOOL IFS_JvmInstallJar( FS_CHAR *jarFile )
{
	return FS_TRUE;
}

void IFS_CameraNewPhoto( FS_FDHandler cb, void *param )
{

}

FS_BOOL IFS_BrowserOpenURL( FS_CHAR *url )
{
	ShellExecute(NULL, "open", url, "", "", SW_SHOWNORMAL);
	return FS_TRUE;
}
