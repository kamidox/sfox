#include "inc/FS_Config.h"
#include "inc/inte/FS_ExInte.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_MemDebug.h"
#include "inc/res/FS_TimerID.h"

#include <setjmp.h>

#ifdef FS_MODULE_IMAGE
#define FS_MAKEWORD( a, b )	((FS_UINT2)(((FS_BYTE)(a)) | ((FS_UINT2)((FS_BYTE)(b))) << 8))

#define FS_DWORD_PAD( x )		(((x) + 3) & ~3)

#define FS_SetPixelBpp( pDstPixel, clr, bpp ) 				\
do {														\
	if( bpp == 1 )											\
	{														\
		*(pDstPixel) = clr; 								\
	}														\
	else if( bpp == 2 )										\
	{														\
		*(FS_UINT2 *)(pDstPixel) = clr; 					\
	}														\
	else if( bpp == 3 )										\
	{														\
		*(FS_UINT2 *)(pDstPixel) = clr; 					\
		*((pDstPixel) + 2) = clr >> 16; 					\
	}														\
	else if( bpp == 4 )										\
	{														\
		*(FS_UINT4 *)(pDstPixel) = clr; 					\
	}														\
} while( 0 )	

/*------------------------------- jpeg decoder -----------------------------------------*/
#include "module/gif/gif_lib.h"

#define GIF_TRANSPARENT 	0x01
#define GIF_USER_INPUT		0x02
#define GIF_DISPOSE_MASK	0x07
#define GIF_DISPOSE_SHIFT	2

#define GIF_NOT_TRANSPARENT	-1

// No disposal specified. The decoder is
// not required to take any action.
#define GIF_DISPOSE_NONE	0		
// Do not dispose. The graphic is to be left
// in place.
#define GIF_DISPOSE_LEAVE	1		
// Restore to background color. The area used by the									
// graphic must be restored to the background color.
#define GIF_DISPOSE_BACKGND 2		
// Restore to previous. The decoder is required to									
// restore the area overwritten by the graphic with 								
// what was there prior to rendering the graphic.
#define GIF_DISPOSE_RESTORE 3		

#define FS_MAX_GIF_ANI_NUM				4
#define FS_GIF_TIMER_ID_BASE			FS_TIMER_ID_NUM

static FS_UINT4 GFS_GifTimerId[FS_MAX_GIF_ANI_NUM];

typedef struct FS_GifInputStream_Tag
{
	FS_BYTE * 			data;
	FS_SINT4			data_len;
	FS_SINT4			offset;
}FS_GifInputStream;

typedef struct FS_GifData_Tag
{
	FS_GifInputStream	istream;
	GifFileType *		gif_file;
	FS_Bitmap 			bitmap;
	FS_COLOR			bg_color;
	FS_UINT4			timer_id;
	FS_UINT4			ref_tid;
	FS_BOOL				dirty;
}FS_GifData;

static FS_UINT4 FS_GifGetTimerId( void )
{
	FS_SINT4 i;
	for( i = 0; i < FS_MAX_GIF_ANI_NUM; i ++ )
	{
		if( GFS_GifTimerId[i] == 0 )
		{
			GFS_GifTimerId[i] = FS_GIF_TIMER_ID_BASE + i;
			return GFS_GifTimerId[i];
		}
	}
	return 0;
}

static void FS_GifResetTimerId( FS_UINT4 tid )
{
	FS_SINT4 i = tid - FS_GIF_TIMER_ID_BASE;
	if( i >= 0 && i < FS_MAX_GIF_ANI_NUM )
	{
		GFS_GifTimerId[i] = 0;
	}
}

static void FS_GifRelease( FS_GifData *gif )
{
	DGifCloseFile( gif->gif_file );
	gif->gif_file = FS_NULL;
	IFS_Free( gif->istream.data );
	gif->istream.data = FS_NULL;
}

static int FS_GifReadData( GifFileType *gifFile, GifByteType *buf, int len )
{
	int ret;
	FS_GifData *gif = (FS_GifData *)gifFile->UserData;

	if( gif->istream.offset + len < gif->istream.data_len )
	{
		ret = len;
	}
	else
	{
		ret = gif->istream.data_len - gif->istream.offset;
	}

	if( ret > 0 )
	{
		IFS_Memcpy( buf, gif->istream.data + gif->istream.offset, ret );
		gif->istream.offset += ret;
	}

	return ret;
}

static void FS_GifCopyLine( FS_BYTE *pDst, FS_BYTE *pSrc, int width, int x, double factor, GifColorType *pColorTable, FS_SINT4 trans )
{
	FS_SINT4 nRow = -1, nLastRow;
	GifPixelType pixel;
	GifColorType *pColor;
	FS_COLOR color;
	FS_UINT1 bpp = IFS_GetBitsPerPixel( );
	bpp = (bpp + 7) >> 3;
	
	if( width )
	{
		do {
			pixel = *pSrc;
			nLastRow = nRow;
			nRow = (FS_SINT4)(x * factor);
			if( nRow != nLastRow )
			{
				/* a new row */
				if( pixel != trans )
				{
					pColor = pColorTable + pixel;
					color = IFS_DDB_RGB( pColor->Red, pColor->Green, pColor->Blue );
					FS_SetPixelBpp( pDst, color, bpp );
				}
				pDst += bpp;
			}

			x ++;
			pSrc ++;
			width --;
		} while( width );
	}
}

void FS_GifClose( FS_GifData *gif )
{
	if( gif->timer_id )
	{
		FS_GifResetTimerId( gif->ref_tid );
		IFS_StopTimer( gif->timer_id );
	}

	FS_SAFE_FREE( gif->bitmap.bits );
	if( gif->gif_file )
	{
		DGifCloseFile( gif->gif_file );
	}
	FS_SAFE_FREE( gif->istream.data );
	IFS_Free( gif );
}

FS_GifData * FS_GifOpen( FS_CHAR *absfile )
{
	FS_BOOL bSuccess = FS_FALSE;
	FS_GifData *gif;

	gif = IFS_Malloc( sizeof(FS_GifData) );
	if( gif )
	{
		IFS_Memset( gif, 0, sizeof(FS_GifData) );
		gif->istream.data_len = FS_FileGetSize( -1, absfile );
		if( gif->istream.data_len > 0 )
		{
			gif->istream.data = IFS_Malloc( gif->istream.data_len );
			if( gif->istream.data )
			{
				FS_FileRead( -1, absfile, 0, gif->istream.data, gif->istream.data_len );
				gif->gif_file = DGifOpen( gif, FS_GifReadData );
				if( gif->gif_file )
				{
					bSuccess = FS_TRUE;
				}
			}
		}
	}

	if( ! bSuccess )
	{
		if( gif )
		{
			FS_GifClose( gif );
			gif = FS_NULL;
		}
	}
	
	return gif;
}

FS_SINT4 FS_GifNextFrame( FS_GifData *gif, FS_SINT4 w, FS_SINT4 h )
{	
	FS_BYTE *pDst, *pBit2, *pLine;
	FS_SINT4 x, y, iW, iH, pass, i, nLastLine, nLine = -1, nRow = -1, ret = -1;
	double factor;
	FS_UINT1 bpp;
	GifRecordType RecordType;
	FS_SINT4 delay = 0;
	FS_SINT4 dispose = 0;
	FS_SINT4 transparent = GIF_NOT_TRANSPARENT;
	int ExtCode;
	GifByteType *pExt;
	GifColorType* pColorTable;
	FS_Bitmap *pBmp;
	const int InterlacedOffset[] = { 0, 4, 2, 1 }; /* The way Interlaced image should. */
	const int InterlacedJumps[] = { 8, 8, 4, 2 };    /* be read - offsets and jumps... */	
	
	bpp = IFS_GetBitsPerPixel( );
	bpp = (bpp + 7) >> 3;
	if( gif->bitmap.bits == FS_NULL )
	{
		gif->bitmap.bpp = bpp;
		gif->bitmap.width = w;
		gif->bitmap.height = h;
		gif->bitmap.pitch = w * gif->bitmap.bpp;
		/* two bitmap + one line */
		gif->bitmap.bits = IFS_Malloc( (h * 2 + 2) * gif->bitmap.pitch + gif->gif_file->SWidth * bpp );
		gif->bg_color = 0xFFFFFF;
		gif->dirty = FS_TRUE;
	}

	if( gif->bitmap.bits == FS_NULL )
		goto DONE;
	
	if( gif->dirty )
	{
		gif->dirty = FS_FALSE;
		/* get GIF backgroud */
		if( gif->gif_file->SColorMap )
		{
			GifColorType *pBgColor;
			if( gif->gif_file->SColorMap->ColorCount < gif->gif_file->SBackGroundColor )
			{
				pBgColor = gif->gif_file->SColorMap->Colors + gif->gif_file->SBackGroundColor;
				gif->bg_color = IFS_DDB_RGB( pBgColor->Red, pBgColor->Green, pBgColor->Blue );
			}
		}
		/* fill GIF backgroud */
		pDst = gif->bitmap.bits;
		for( y = 0; y < h * 2; y ++ )
		{
			for( x = 0; x < w; x ++ )
			{
				FS_SetPixelBpp( pDst, gif->bg_color, bpp );
				pDst += bpp;
			}
		}
	}
	
	/* get GIF image frame */
	do {
		if( DGifGetRecordType( gif->gif_file, &RecordType ) == GIF_ERROR ) break;

		if( RecordType == IMAGE_DESC_RECORD_TYPE )
		{
			if( DGifGetImageDesc( gif->gif_file ) != GIF_ERROR )
			{
				if( delay > 0 && delay < 10 )
					delay = 10;
				ret = delay * 10;

				
				factor = (double)w / gif->gif_file->SWidth;
				x = gif->gif_file->Image.Left;
				y = gif->gif_file->Image.Top;
				
				pBmp = &gif->bitmap;
				pBit2 = gif->bitmap.bits + pBmp->pitch * pBmp->height;
				pLine = pBit2 + pBmp->pitch * pBmp->height;
				if( gif->gif_file->Image.ColorMap )
					pColorTable = gif->gif_file->Image.ColorMap->Colors;
				else
					pColorTable = gif->gif_file->SColorMap->Colors;

				IFS_Memcpy( pBmp->bits, pBit2, pBmp->pitch * pBmp->height );
				
				iW = gif->gif_file->Image.Width;
				iH = gif->gif_file->Image.Height;
				nRow = (FS_SINT4)(x * factor);
				if( gif->gif_file->Image.Interlace )
				{
					// Need to perform 4 passes on the images:
					for( pass = 0; pass < 4; pass ++ )
					{
						for( i = InterlacedOffset[pass]; i < iH; i += InterlacedJumps[pass] )
						{
							if( DGifGetLine( gif->gif_file, pLine, iW ) == GIF_ERROR )
							{
								ret = -1;
								goto DONE;
							}

							nLastLine = nLine;
							nLine = (FS_SINT4)((i + y) * factor);
							if( nLine != nLastLine )
							{
								/* a new line */
								pDst = pBmp->bits + ( nLine * pBmp->pitch + nRow * pBmp->bpp );
								if( pDst < pBmp->bits + pBmp->pitch * pBmp->height )
									FS_GifCopyLine( pDst, pLine, iW, x, factor, pColorTable, transparent );
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
						if( DGifGetLine( gif->gif_file, pLine, iW ) == GIF_ERROR )
						{
							ret = -1;
							goto DONE;
						}

						nLastLine = nLine;
						nLine = (FS_SINT4)((i + y) * factor);
						if( nLine != nLastLine )
						{
							/* a new line */
							pDst = pBmp->bits + ( nLine * pBmp->pitch + nRow * pBmp->bpp );
							if( pDst < pBmp->bits + pBmp->pitch * pBmp->height )
								FS_GifCopyLine( pDst, pLine, iW, x, factor, pColorTable, transparent );
							else
								goto DONE;
						}
					}
				}

				// Prepare second image with next starting				
				if( dispose == GIF_DISPOSE_BACKGND)
				{
					x = gif->gif_file->Image.Left;
					y = gif->gif_file->Image.Top;
					iW = gif->gif_file->Image.Width;
					iH = gif->gif_file->Image.Height;
					for( i = 0; i < iH; i ++ )
					{
						FS_SetPixelBpp( pBit2, gif->bg_color, bpp );
						pBit2 += bpp;
					}
				}				
				else if( dispose != GIF_DISPOSE_RESTORE )
				{					
					// Copy current -> next (Update)
					IFS_Memcpy( pBit2, pBmp->bits, pBmp->pitch * pBmp->height );
				}
				dispose = 0;

				/* we just get the gif first frame */
				goto DONE;
			}
		}
		else if( RecordType == EXTENSION_RECORD_TYPE )
		{
			/* process extension blocks in file */
			if( DGifGetExtension( gif->gif_file, &ExtCode, &pExt ) == GIF_ERROR ) 
				goto DONE;

			if( ExtCode == GRAPHICS_EXT_FUNC_CODE )
			{
				delay = FS_MAKEWORD(pExt[2], pExt[3]);
				if( pExt[1] & GIF_TRANSPARENT ) transparent = pExt[4];
				dispose = ( pExt[1] >> GIF_DISPOSE_SHIFT) & GIF_DISPOSE_MASK;
			}
			
			while( pExt != NULL )
			{
				if( DGifGetExtensionNext( gif->gif_file, &pExt ) == GIF_ERROR )
					goto DONE;
			}
		}
	} while( RecordType != TERMINATE_RECORD_TYPE );

DONE:
	return ret;
}

static void FS_GifTimer_CB( FS_GifData *gif )
{
	FS_SINT4 delay;
	
	if( ! IFS_SystemAllowDrawScreen() )
	{
		gif->timer_id = IFS_StartTimer( gif->ref_tid, 100, FS_GifTimer_CB, gif );
		return;
	}
	
	delay = FS_GifNextFrame( gif, gif->bitmap.width, gif->bitmap.height );
	if( delay > 0 )
	{
		FS_ImageFrameInd( (FS_UINT4)gif, &gif->bitmap );
		gif->timer_id = IFS_StartTimer( gif->ref_tid, delay, FS_GifTimer_CB, gif );
	}
	else
	{
		/* restart gif animation */
		gif->istream.offset = 0;
		DGifCloseFile( gif->gif_file );
		gif->gif_file = DGifOpen( gif, FS_GifReadData );
		if( gif->gif_file )
		{
			gif->dirty = FS_TRUE;
			delay = FS_GifNextFrame( gif, gif->bitmap.width, gif->bitmap.height );
			if( delay > 0 )
			{
				FS_ImageFrameInd( (FS_UINT4)gif, &gif->bitmap );
				gif->timer_id = IFS_StartTimer( gif->ref_tid, delay, FS_GifTimer_CB, gif );
			}
		}
	}
}

FS_BOOL FS_GifGetSize( char *filename, int *w, int *h )
{
	FS_BOOL ret = FS_FALSE;
	FS_GifData *gif = FS_GifOpen( filename );
	*w = *h = 0;
	if( gif )
	{
		*w = gif->gif_file->SWidth;
		*h = gif->gif_file->SHeight;
		ret = FS_TRUE;
		FS_GifClose( gif );
	}
	return ret;
}

FS_UINT4 FS_GifDecode( FS_CHAR *filename, FS_SINT4 w, FS_SINT4 h, FS_Bitmap *pBmp )
{
	FS_SINT4 delay;
	FS_GifData *gif = FS_GifOpen( filename );
	if( gif )
	{
		delay = FS_GifNextFrame( gif, w, h );
		if( delay >= 0 )
		{
			*pBmp = gif->bitmap;
			if( delay > 0 )
			{
				gif->ref_tid = FS_GifGetTimerId( );
				
				if( gif->ref_tid > 0 )
				{
					pBmp->bits = IFS_Malloc( pBmp->pitch * pBmp->height );
					if( pBmp->bits )
					{
						IFS_Memcpy( pBmp->bits, gif->bitmap.bits, pBmp->pitch * pBmp->height );
					}
					gif->timer_id = IFS_StartTimer( gif->ref_tid, delay, FS_GifTimer_CB, gif );
				}
				else
				{
					/* no timer id. decode gif animation as a static image */
					gif->bitmap.bits = FS_NULL;
					FS_GifClose( gif );
					gif = FS_NULL;
				}
			}
			else
			{
				/* not a animation. decode gif as a static image */
				gif->bitmap.bits = FS_NULL;
				FS_GifClose( gif );
				gif = FS_NULL;
			}
		}
		else
		{
			pBmp->bits = FS_NULL;
			FS_GifClose( gif );
			gif = FS_NULL;
		}
	}

	return (FS_UINT4)gif;
}

void FS_GifDestroy( FS_UINT4 hIm )
{
	FS_GifData *gif = (FS_GifData *)hIm;
	if( gif ) 
	{
		FS_GifClose( gif );
	}
}

/*------------------------------- jpeg decoder -----------------------------------------*/
#include "module/jpeg/jpeglib.h"

struct my_error_mgr {
	struct jpeg_error_mgr pub;	/* "public" fields */
	jmp_buf setjmp_buffer;		/* for return to caller */
};

typedef struct FS_JpegInputStream_Tag
{
	FS_BYTE *		data;
	FS_SINT4		len;
	FS_SINT4		offset;
}FS_JpegInputStream;

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

int FS_JpegStreamRead( unsigned int file, unsigned char *buf, int size )
{
	FS_JpegInputStream *stream;

	stream = (FS_JpegInputStream *)file;
	size = FS_MIN( size, stream->len - stream->offset );
	if( size > 0 )
	{
		IFS_Memcpy( buf, stream->data + stream->offset, size );
		stream->offset += size;
	}
	return size;
}

FS_BOOL FS_JPEGGetSize( char *filename, int *w, int *h )
{	
	static FS_JpegInputStream s_jpg_stream;
	FS_BOOL ret = 0;
	
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;

	*w = *h = 0;

	/* Step 0: init jpeg input stream */
	s_jpg_stream.len = FS_FileGetSize( -1, filename );
	if( s_jpg_stream.len <= 0 )
		return 0;
	s_jpg_stream.data = IFS_Malloc( s_jpg_stream.len );
	if( s_jpg_stream.data == FS_NULL )
		return 0;
	FS_FileRead( -1, filename, 0, s_jpg_stream.data, s_jpg_stream.len );
	s_jpg_stream.offset = 0;
	
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
		IFS_Free( s_jpg_stream.data );
		IFS_Memset( &s_jpg_stream, 0, sizeof(FS_JpegInputStream) );
		return ret;
	}
	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source (eg, a file) */

	jpeg_stdio_src(&cinfo, (FS_UINT4)&s_jpg_stream);

	/* Step 3: read file parameters with jpeg_read_header() */

	jpeg_read_header(&cinfo, TRUE);

	*w = cinfo.image_width;
	*h = cinfo.image_height;
	ret = 1;
	
	jpeg_destroy_decompress(&cinfo);

	IFS_Free( s_jpg_stream.data );
	IFS_Memset( &s_jpg_stream, 0, sizeof(FS_JpegInputStream) );

	return ret;
}

FS_UINT4 FS_JPEGDecode( char *filename, int w, int h, FS_Bitmap *pBmp )
{
	static FS_JpegInputStream s_jpg_stream;

	JSAMPARRAY buffer;
	unsigned char *out_pixels, *inptr;
	int col, i, nLine = -1, nLastLine, nRow = -1, nLastRow;
	FS_UINT1 bpp;
	double factor;
	FS_COLOR color;
	
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;

	/* Step 0: init jpeg input stream */
	s_jpg_stream.len = FS_FileGetSize( -1, filename );
	if( s_jpg_stream.len <= 0 )
		return 0;
	s_jpg_stream.data = IFS_Malloc( s_jpg_stream.len );
	if( s_jpg_stream.data == FS_NULL )
		return 0;
	FS_FileRead( -1, filename, 0, s_jpg_stream.data, s_jpg_stream.len );
	s_jpg_stream.offset = 0;

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
		IFS_Free( s_jpg_stream.data );
		IFS_Memset( &s_jpg_stream, 0, sizeof(FS_JpegInputStream) );
		return 0;
	}
	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source (eg, a file) */
	jpeg_stdio_src(&cinfo, (FS_UINT4)&s_jpg_stream);

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
	pBmp->pitch = w * bpp;
	pBmp->bits = IFS_Malloc( pBmp->pitch * (pBmp->height + 2) );
	
	i = 0;
	out_pixels = pBmp->bits;
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, ((cinfo.output_width + 3) * cinfo.output_components), 1);
	
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
					color = IFS_DDB_RGB( inptr[0], inptr[1], inptr[2] );
					FS_SetPixelBpp( out_pixels, color, bpp );
					out_pixels += bpp;
				}
				inptr += cinfo.output_components;
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
	IFS_Free( s_jpg_stream.data );
	IFS_Memset( &s_jpg_stream, 0, sizeof(FS_JpegInputStream) );
	return 0;
}

/*------------------------------- png decoder -----------------------------------------*/
#include "module/png/png.h"

typedef struct FS_PngInputStream_Tag
{
	FS_BYTE *		data;
	FS_SINT4		len;
	FS_SINT4		offset;
}FS_PngInputStream;

static void *FS_PngMalloc( png_structp png_ptr, png_size_t size )
{
	return IFS_Malloc( size );
}

static void FS_PngFree( png_structp png_ptr, void * p )
{
	IFS_Free( p );
}

static void FS_PngError( png_structp png_ptr, const char *msg )
{
	longjmp( png_jmpbuf(png_ptr), 1);
}

static void FS_PngWarning( png_structp png_ptr, const char *msg )
{
	/* do nothing */
}

static void FS_PngDataRead( png_structp png_ptr, png_voidp buf, png_size_t size )
{
	FS_PngInputStream *stream;
	stream = (FS_PngInputStream *)png_get_io_ptr( png_ptr );

	size = FS_MIN( (FS_SINT4)size, stream->len - stream->offset );
	if( size > 0 )
	{
		IFS_Memcpy( buf, stream->data + stream->offset, size );
		stream->offset += size;
	}
}

FS_BOOL FS_PngGetSize( char *filename, int *w, int *h )
{
	static FS_PngInputStream s_stream;
	
	png_structp png_ptr;
	png_infop info_ptr;
	png_uint_32 width, height;
	int bit_depth, color_type;
	
	*w = *h = 0;
	IFS_Memset( &s_stream, 0, sizeof(FS_PngInputStream) );
	s_stream.len = FS_FileGetSize( -1, filename );
	if( s_stream.len <= 0 )
		return FS_FALSE;
	s_stream.data = IFS_Malloc( s_stream.len );
	if( s_stream.data == FS_NULL )
		return FS_FALSE;
	FS_FileRead( -1, filename, 0, s_stream.data, s_stream.len );
	
	png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, FS_NULL, FS_NULL, FS_NULL );

	if( png_ptr == FS_NULL ) return 0;

	//png_set_mem_fn( png_ptr, FS_NULL, FS_PngMalloc, FS_PngFree );
	png_set_error_fn( png_ptr, FS_NULL, FS_PngError, FS_PngWarning );

	info_ptr = png_create_info_struct( png_ptr );
	if( info_ptr == FS_NULL )
	{
		png_destroy_read_struct( &png_ptr, FS_NULL, FS_NULL );
		return FS_FALSE;
	}

	if( setjmp( png_jmpbuf(png_ptr) ) )
	{
		if( s_stream.data )
		{
			IFS_Free( s_stream.data );
			s_stream.data = FS_NULL;
		}
		png_destroy_read_struct( &png_ptr, &info_ptr, FS_NULL );
		return FS_FALSE;
	}
	png_set_read_fn( png_ptr, &s_stream, FS_PngDataRead );
	png_read_info( png_ptr, info_ptr );
	png_get_IHDR( png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, FS_NULL, FS_NULL, FS_NULL );
	*w = width;
	*h = height;

	IFS_Free( s_stream.data );
	s_stream.data = FS_NULL;
	png_destroy_read_struct( &png_ptr, &info_ptr, FS_NULL );
	return FS_TRUE;
}

FS_UINT4 FS_PngDecode( char *filename, int w, int h, FS_Bitmap *pBmp )
{
	static FS_PngInputStream s_stream;

	png_structp png_ptr;
	png_infop info_ptr;
	png_uint_32 y, col, width, height;
	png_bytep row_buf = FS_NULL;
	int bit_depth, color_type;
	int num_pass, pass;
	FS_UINT1 bpp;
	double factor;
	int i, nLine = -1, nLastLine, nRow = -1, nLastRow;
	FS_BYTE *out_pixels, *inptr;
	FS_COLOR color;

	IFS_Memset( &s_stream, 0, sizeof(FS_PngInputStream) );
	s_stream.len = FS_FileGetSize( -1, filename );
	if( s_stream.len <= 0 )
		return 0;
	s_stream.data = IFS_Malloc( s_stream.len );
	if( s_stream.data == FS_NULL )
		return 0;
	FS_FileRead( -1, filename, 0, s_stream.data, s_stream.len );
	
	png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, FS_NULL, FS_NULL, FS_NULL );

	if( png_ptr == FS_NULL ) return 0;

	//png_set_mem_fn( png_ptr, FS_NULL, FS_PngMalloc, FS_PngFree );
	png_set_error_fn( png_ptr, FS_NULL, FS_PngError, FS_PngWarning );

	info_ptr = png_create_info_struct( png_ptr );
	if( info_ptr == FS_NULL )
	{
		png_destroy_read_struct( &png_ptr, FS_NULL, FS_NULL );
		return 0;
	}

	if( setjmp( png_jmpbuf(png_ptr) ) )
	{
CLEANUP:
		if( s_stream.data )
		{
			IFS_Free( s_stream.data );
			s_stream.data = FS_NULL;
		}
		if( pBmp->bits )
		{
			IFS_Free( pBmp->bits );
			pBmp->bits = FS_NULL;
		}
		IFS_Memset( pBmp, 0, sizeof(FS_Bitmap) );
		if( row_buf )
		{
			IFS_Free( row_buf );
			row_buf = FS_NULL;
		}
		png_destroy_read_struct( &png_ptr, &info_ptr, FS_NULL );
		return 0;
	}
	png_set_read_fn( png_ptr, &s_stream, FS_PngDataRead );
	png_read_info( png_ptr, info_ptr );
	png_get_IHDR( png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, FS_NULL, FS_NULL, FS_NULL );

	if( bit_depth == 16 )
		png_set_strip_16(png_ptr);
	if( color_type == PNG_COLOR_TYPE_PALETTE )
		png_set_expand(png_ptr);
	if( bit_depth < 8 )
		png_set_expand(png_ptr);
	if( png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) )
		png_set_expand(png_ptr);

	png_set_strip_alpha( png_ptr );
	if( color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA )
		png_set_gray_to_rgb( png_ptr );
		
	png_read_update_info( png_ptr, info_ptr );
	
	bpp = IFS_GetBitsPerPixel( );
	bpp = (bpp + 7) >> 3;
	pBmp->width = w;
	pBmp->height = h;
	pBmp->bpp = bpp;
	pBmp->pitch = w * bpp;
	pBmp->bits = IFS_Malloc( pBmp->pitch * (pBmp->height + 2) );
	if( pBmp->bits == FS_NULL ) goto CLEANUP;
	factor = (double)w / width;
	i = 0;
	
	row_buf = IFS_Malloc( png_get_rowbytes(png_ptr, info_ptr) );
	if( row_buf == FS_NULL ) goto CLEANUP;
	
	num_pass = png_set_interlace_handling( png_ptr );
	for( pass = 0; pass < num_pass; pass++ )
	{
		for( y = 0; y < height; y++ )
		{
			png_read_row( png_ptr, row_buf, FS_NULL );
			
			nLastLine = nLine;
			nLine = (int)(y * factor);
			if( nLine != nLastLine )
			{
				/* Assume put_scanline_someplace wants a pointer and sample count. */
				out_pixels = pBmp->bits + ( i * pBmp->pitch );
				inptr = row_buf;
				i ++;
				for( col = 0; col < width; col ++ )
				{
					nLastRow = nRow;
					nRow = (int)( col * factor );
					if( nRow != nLastRow )
					{
						color = IFS_DDB_RGB( inptr[0], inptr[1], inptr[2] );
						FS_SetPixelBpp( out_pixels, color, bpp );
						out_pixels += bpp;
					}
					inptr += 3;
				}
			}
		}
	}
	IFS_Free( row_buf );
	row_buf = FS_NULL;
	png_read_end( png_ptr, info_ptr );
	png_destroy_read_struct( &png_ptr, &info_ptr, FS_NULL );
	IFS_Free( s_stream.data );
	s_stream.data = FS_NULL;
	return 0;
}

/*------------------------------- bmp decoder -----------------------------------------*/

typedef struct FS_BmpFileHeader_Tag {	/* bmfh */
	FS_UINT4		size;
	FS_UINT2		reserved1;
	FS_UINT2		reserved2;
	FS_UINT4		bits_offset;
}FS_BmpFileHeader;

typedef struct FS_BmpInfoHeader_Tag {	/* bmih */
	FS_UINT4		size;
	FS_UINT4		width;
	FS_UINT4		height;
	FS_UINT2		planes;
	FS_UINT2		bit_count;
	FS_UINT4		compression;
	FS_UINT4		size_image;
	FS_UINT4		x_pels_per_meter;
	FS_UINT4		y_pels_per_meter;
	FS_UINT4		clr_used;
	FS_UINT4		clr_important;
}FS_BmpInfoHeader;

typedef struct FS_RGBQuad_Tag { 	/* rgbq */
	FS_BYTE			blue;
	FS_BYTE			green;
	FS_BYTE			red;
	FS_BYTE			reserved;
}FS_RGBQuad;

static FS_BOOL FS_BmpReadHead( FS_CHAR *filename, FS_BmpFileHeader *bmfh, FS_BmpInfoHeader *bmih )
{
	FS_UINT4 size;
	FS_BYTE head[54];
	size = FS_FileGetSize( -1, filename );

	if( 54 != FS_FileRead( -1, filename, 0, head, 54 ) )
		return FS_FALSE;
	if( head[0] != 0x42 || head[1] != 0x4D )
		return FS_FALSE;
	
	bmfh->size = FS_LE_BYTE_TO_UINT4( head+ 2 );
	bmfh->reserved1 = FS_LE_BYTE_TO_UINT2( head + 6 );
	bmfh->reserved2 = FS_LE_BYTE_TO_UINT2( head + 8 );
	bmfh->bits_offset = FS_LE_BYTE_TO_UINT4( head + 10 );

	bmih->size = FS_LE_BYTE_TO_UINT4( head + 14 );
	bmih->width = FS_LE_BYTE_TO_UINT4( head + 18 );
	bmih->height = FS_LE_BYTE_TO_UINT4( head + 22 );
	bmih->planes = FS_LE_BYTE_TO_UINT2( head + 26 );
	bmih->bit_count = FS_LE_BYTE_TO_UINT2( head + 28 );
	bmih->compression = FS_LE_BYTE_TO_UINT4( head + 30 );
	bmih->size_image = FS_LE_BYTE_TO_UINT4( head + 34 );
	bmih->x_pels_per_meter = FS_LE_BYTE_TO_UINT4( head + 38 );
	bmih->y_pels_per_meter = FS_LE_BYTE_TO_UINT4( head + 42 );
	bmih->clr_used = FS_LE_BYTE_TO_UINT4( head + 46 );
	bmih->clr_important = FS_LE_BYTE_TO_UINT4( head + 50 );
#if 0	
	if( 12 != FS_FileRead( -1, filename, 2, bmfh, 12 ) )
		return FS_FALSE;
	if( bmfh->size != size || bmfh->reserved1 != 0 || bmfh->reserved2 != 0 )
		return FS_FALSE;
	if( 40 != FS_FileRead( -1, filename, 14, bmih, 40 ) )
		return FS_FALSE;
#endif	
	/* current do not support compression */
	if( bmih->size < 40 || bmih->planes != 1 || bmih->compression != 0 || bmih->bit_count > 24 )
		return FS_FALSE;
	if( bmih->width > 0x7FFFFFFF || bmih->height > 0x7FFFFFFF )
		return FS_FALSE;
	
	return FS_TRUE;
}

static FS_COLOR FS_BmpGetPixelRGB( FS_BYTE *row, FS_SINT4 col, FS_UINT2 bit_count, FS_RGBQuad * clr_table )
{
	FS_COLOR color = 0;
	FS_SINT4 index = -1;
	switch( bit_count )
	{
		case 1:
			row += (col >> 3);
			index = (*row >> (7 - (col & 7))) & 0x01;
			break;
		case 2:
			row += (col >> 2);
			index = (*row >> ((3 - (col & 3)) << 1)) & 0x03;
			break;
		case 4:
			row += (col >> 1);
			index = (*row >> ((1 - (col & 0x01)) << 2)) & 0x0F;
			break;
		case 8:
			row += col;
			index = *row;
			break;
		case 24:
			row += col * 3;
			color = IFS_DDB_RGB( row[2], row[1], row[0] );
			break;
	}

	if( index != -1 )
		color = IFS_DDB_RGB( clr_table[index].red, clr_table[index].green, clr_table[index].blue );
	
	return color;
}

FS_BOOL FS_BmpGetSize( FS_CHAR *filename, FS_SINT4 *w, FS_SINT4 *h )
{
	FS_BmpFileHeader bmfh;
	FS_BmpInfoHeader bmih;

	*w = *h = 0;
	if( ! FS_BmpReadHead(filename, &bmfh, &bmih) )
		return FS_FALSE;

	*w = (FS_SINT4)bmih.width;
	*h = (FS_SINT4)bmih.height;
	return FS_TRUE;
}

FS_BOOL FS_BmpDecode( FS_CHAR *filename, FS_SINT4 w, FS_SINT4 h, FS_Bitmap *pBmp )
{
	FS_BmpFileHeader bmfh;
	FS_BmpInfoHeader bmih;
	FS_RGBQuad *clr_table;
	FS_BYTE *buf;
	FS_SINT4 len, offset;
	FS_UINT1 bpp;
	double factor;
	FS_SINT4 i, nLine = -1, nLastLine, nRow = -1, nLastRow;
	FS_UINT4 col, y;
	FS_SINT4 pitch;
	FS_BYTE *out_pixels, *line;
	FS_COLOR color;
	
	if( ! FS_BmpReadHead(filename, &bmfh, &bmih) )
		return FS_FALSE;

	len = bmfh.size - 14 - bmih.size;
	offset = 14 + bmih.size;
	buf = IFS_Malloc( len );
	if( buf == FS_NULL )
		return FS_FALSE;

	if( len != FS_FileRead( -1, filename, offset, buf, len ) )
	{
		IFS_Free( buf );
		return FS_FALSE;
	}

	bpp = IFS_GetBitsPerPixel( );
	bpp = (bpp + 7) >> 3;
	pBmp->width = w;
	pBmp->height = h;
	pBmp->bpp = bpp;
	pBmp->pitch = w * bpp;
	pBmp->bits = IFS_Malloc( pBmp->pitch * pBmp->height );
	if( pBmp->bits == FS_NULL )
	{
		IFS_Free( buf );
		return FS_FALSE;
	}
	
	factor = (double)w / bmih.width;
	i = 0;
	clr_table = (FS_RGBQuad *)buf;
	pitch = FS_DWORD_PAD((bmih.width * bmih.bit_count) >> 3);	/* 32 bit align */
	line = (buf - offset + bmfh.bits_offset) + (bmih.height * pitch);
	for( y = 0; y < bmih.height; y ++ )
	{
		line -= pitch;
		nLastLine = nLine;
		nLine = (int)(y * factor);
		if( nLine != nLastLine )
		{
			out_pixels = pBmp->bits + ( i * pBmp->pitch );
			i ++;
			for( col = 0; col < bmih.width; col ++ )
			{
				nLastRow = nRow;
				nRow = (FS_SINT4)( col * factor );
				if( nRow != nLastRow )
				{
					color = FS_BmpGetPixelRGB( line, col, bmih.bit_count, clr_table );
					FS_SetPixelBpp( out_pixels, color, bpp );
					out_pixels += bpp;
				}
			}
		}
	}
	IFS_Free( buf );
	return FS_TRUE;
}

/*------------------------------- wbmp decoder -----------------------------------------*/

/*
 *	Appendix A: Specification of well-defined WBMP Types - [SPEC-WAESpec-19991104.pdf]
 */

/* return wbmp header length */
FS_UINT1 FS_WbmpGetSize( FS_CHAR *filename, FS_SINT4 *w, FS_SINT4 *h )
{
	FS_BYTE buf[16], *p;
	FS_SINT4 len;
	
	*w = *h = 0;
	if( (len = FS_FileRead(-1, filename, 0, buf, 16)) < 4 )
		return FS_FALSE;
	if( buf[0] != 0 || buf[1] & 0x9F )
		return FS_FALSE;
	p = buf + 2;
	p += FS_UIntVarToUInt4( (FS_UINT4 *)w, p );
	p += FS_UIntVarToUInt4( (FS_UINT4 *)h, p );
	if( *w <= 0 || *h <= 0 )
	{
		*w = *h = 0;
		return FS_FALSE;
	}
	return p - buf;
}

FS_BOOL FS_WbmpDecode( FS_CHAR *filename, FS_SINT4 w, FS_SINT4 h, FS_Bitmap *pBmp )
{
	FS_SINT4 width, height, size, pitch;
	FS_UINT1 hlen;
	FS_BYTE *buf;
	FS_UINT1 bpp;
	double factor;
	FS_SINT4 i,col, y, nLine = -1, nLastLine, nRow = -1, nLastRow;
	FS_BYTE *out_pixels, *line;
	FS_COLOR color;
	
	if( (hlen = FS_WbmpGetSize(filename, &width, &height)) == 0 )
		return FS_FALSE;

	pitch = (width + 7) >> 3;
	size = FS_FileGetSize( -1, filename );
	if( size < hlen + pitch * height )
		return FS_FALSE;

	size -= hlen;
	buf = IFS_Malloc( size );
	if( buf == FS_NULL )
		return FS_FALSE;
	if( size != FS_FileRead( -1, filename, hlen, buf, size ) )
	{
		IFS_Free( buf );
		return FS_FALSE;
	}

	bpp = IFS_GetBitsPerPixel( );
	bpp = (bpp + 7) >> 3;
	pBmp->width = w;
	pBmp->height = h;
	pBmp->bpp = bpp;
	pBmp->pitch = w * bpp;
	pBmp->bits = IFS_Malloc( pBmp->pitch * pBmp->height );
	if( pBmp->bits == FS_NULL )
	{
		IFS_Free( buf );
		return FS_FALSE;
	}
	
	factor = (double)w / width;
	i = 0;
	line = buf;
	for( y = 0; y < height; y ++ )
	{
		nLastLine = nLine;
		nLine = (int)(y * factor);
		if( nLine != nLastLine )
		{
			out_pixels = pBmp->bits + ( i * pBmp->pitch );
			i ++;
			for( col = 0; col < width; col ++ )
			{
				nLastRow = nRow;
				nRow = (FS_SINT4)( col * factor );
				if( nRow != nLastRow )
				{
					if( (line[col >> 3] >> (7 - (col & 7))) & 0x01 )
						color = 0xFFFFFF;	/* white */
					else
						color = 0x000000;	/* black */
					FS_SetPixelBpp( out_pixels, color, bpp );
					out_pixels += bpp;
				}
			}
		}
		line += pitch;
	}
	IFS_Free( buf );
	return FS_TRUE;
	
}

#endif

FS_BOOL FS_ImageGetSize( char *filename, int *w, int *h )
{
#ifdef FS_MODULE_IMAGE
	FS_BYTE buf[4];
	FS_FileRead( -1, filename, 0, buf, 4 );

	if( buf[0] == 'G' && buf[1] == 'I' && buf[2] == 'F' )
	{
		return FS_GifGetSize( filename, w, h );
	}
	else if( buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF )
	{
		return FS_JPEGGetSize( filename, w, h );
	}
	else if( buf[0] == 0x89 && buf[1] == 'P' && buf[2] == 'N' && buf[3] == 'G' )
	{
		return FS_PngGetSize( filename, w, h );
	}
	else if( buf[0] == 'B' && buf[1] == 'M' )
	{
		return FS_BmpGetSize( filename, w, h );
	}
	else if( buf[0] == 0 )
	{
		return FS_WbmpGetSize( filename, w, h );
	}
	
	return FALSE;
#else
	IFS_ImageGetSize( filename, w, h );
#endif
}

FS_UINT4 FS_ImageDecode( FS_CHAR *file, FS_SINT4 w, FS_SINT4 h, FS_Bitmap *pBmp )
{
#ifdef FS_MODULE_IMAGE
	FS_BYTE buf[4];
	FS_FileRead( -1, file, 0, buf, 4 );

	if( buf[0] == 'G' && buf[1] == 'I' && buf[2] == 'F' )
	{	
		return FS_GifDecode( file, w, h, pBmp );
	}
	else if( buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF )
	{
		return FS_JPEGDecode( file, w, h, pBmp );
	}
	else if( buf[0] == 0x89 && buf[1] == 'P' && buf[2] == 'N' && buf[3] == 'G' )
	{
		return FS_PngDecode( file, w, h, pBmp );
	}
	else if( buf[0] == 'B' && buf[1] == 'M' )
	{
		FS_BmpDecode( file, w, h, pBmp );
	}
	else if( buf[0] == 0 )
	{
		FS_WbmpDecode( file, w, h, pBmp );
	}
	
	return 0;
#else
	IFS_ImageDecode( file, w, h, pBmp );
#endif
}

void FS_ImageRelease( FS_UINT4 hImage, FS_Bitmap *pBmp )
{
#ifdef FS_MODULE_IMAGE
	IFS_Free( pBmp->bits );
	pBmp->bits = FS_NULL;
#else
	IFS_ImageRelease( hImage, pBmp );
#endif
}

void FS_ImageDestroy( FS_UINT4 hImage )
{
#ifdef FS_MODULE_IMAGE
	if( hImage ) FS_GifDestroy( hImage );
#else
	IFS_ImageDestroy( hImage );
#endif
}

void FS_ImageScale( FS_SINT4 *w, FS_SINT4 *h, FS_SINT4 max_w )
{
	double factor;
	FS_SINT4 iW, iH;
	iW = *w;
	iH = *h;
	if( iW > max_w )
	{
		factor = (double)max_w / iW;
		iH = (FS_SINT4)(iH * factor);
		iW = max_w;
	}
	*w = iW;
	*h = iH;
}

/*------------------------ Self Memery Manager --------------------------------*/
#ifdef FS_MODULE_MM

#define FS_MEM_SIZE(x) ((4 + (x) + sizeof(FS_MemHdr) - 1)&(~(sizeof(FS_MemHdr) - 1)))

typedef struct FS_MemHdr_Tag
{
	FS_UINT4 	next;
	FS_UINT4 	len;
}FS_MemHdr;

static FS_BYTE *GFS_MemBase;
static FS_BYTE *GFS_MemEnd;
static FS_MemHdr GFS_MemHdr;

static FS_UINT4 GFS_MemLeft;

FS_BOOL FS_MemInit( void )
{
	void *root;
	FS_UINT4 size;

	size = IFS_GetMemPoolSize( );
	root = IFS_GetMemPool( );
	if( root == NULL )
	{
		FS_TRACE0( "[ERROR] FS_MemInit failed\r\n" );
		return FS_FALSE;
	}
	size &= ~(sizeof(FS_MemHdr)-1); /* align to header size	*/
		
	GFS_MemBase = (FS_BYTE *)root; /* setup memory handler	*/
	GFS_MemEnd = (FS_BYTE *)root + size;
	GFS_MemHdr.next = 0;
	GFS_MemHdr.len = 0;
	((FS_MemHdr *) GFS_MemBase)->next = size;
	((FS_MemHdr *) GFS_MemBase)->len = size;
	GFS_MemLeft = size;
	IFS_Memset( GFS_MemBase + sizeof(FS_MemHdr), 0, size - sizeof(FS_MemHdr) );
	return TRUE;
}

void FS_MemDeinit( void )
{
	FS_MemHdr *p, *n;
	p = &GFS_MemHdr;							/* root of dyn memory		*/
	n = (FS_MemHdr *) (GFS_MemBase + p->next); /* first free block	*/
	if( GFS_MemLeft != IFS_GetMemPoolSize( ) )
	{
		FS_TRACE0( "[ERROR] FS_MemDeinit. memory leak.\r\n");
	}
	IFS_ReleaseMemPool( GFS_MemBase );
	GFS_MemBase = FS_NULL;
	GFS_MemEnd = FS_NULL;
	GFS_MemHdr.next = 0;
	GFS_MemHdr.len = 0;
	GFS_MemLeft = 0;
}

void * FS_Malloc( FS_UINT4 size )
{
	FS_MemHdr *p, *n, *l;
	
	size = (FS_UINT4)FS_MEM_SIZE( size );
	
	if( size >= GFS_MemLeft )
	{
		FS_TRACE0( "FS_Malloc(): no memory\r\n" );
		return 0;
	}
	
	if( GFS_MemBase + GFS_MemHdr.next > GFS_MemEnd
		|| GFS_MemBase + GFS_MemHdr.next < GFS_MemBase )
	{
		FS_TRACE0( "FS_Malloc(): corrupted memory\r\n" );
		return 0;
	}

	p = &GFS_MemHdr;							/* root of dyn memory		*/
	n = (FS_MemHdr *) (GFS_MemBase + p->next); /* first free block	*/
	while( (FS_BYTE *)n < GFS_MemEnd )
	{
		if (n->len == size) 			/* fits exactly:			*/
		{
			p->next = n->next;			/* just remove from chain	*/
			GFS_MemLeft -= size;
			IFS_Memset( n, 0, size );
			*((FS_UINT4 *)n) = size;
			return ((FS_BYTE *)n) + 4;
		}
		if( n->len > size ) 			/* take it from a big one	*/
		{
			l = (FS_MemHdr *)( (FS_BYTE *)n + size );	/* new header	  */
			l->next = n->next;						/* setup chain to next	  */
			l->len = (FS_UINT4)( n->len - size );		/* remaining memory   */
			p->next += size;						/* link with previous block */
			GFS_MemLeft -= size;
			IFS_Memset( n, 0, size );
			*((FS_UINT4 *)n) = size;
			return ((FS_BYTE *)n) + 4;
		}
		p = n;
		n = (FS_MemHdr *) ( GFS_MemBase + n->next );
	}

	if ((FS_BYTE *)n == GFS_MemEnd)
	{
		FS_TRACE0( "FS_Malloc(): no memory block big enough to allocate size requested\r\n" );
	}
	else
	{
		FS_TRACE0( "FS_Malloc(): Error: free list corruption is likely\r\n" );
	}

	return 0;
}

void FS_Free( void *ptr )
{
	FS_MemHdr *p, *n;
	FS_UINT4 size;
	FS_BYTE *mem = ((FS_BYTE *)ptr) - 4;
	
	size = *((FS_UINT4 *)mem);
	if ( mem < GFS_MemBase || mem >= GFS_MemEnd ||
	   mem + size > GFS_MemEnd ||
	   mem + size <= GFS_MemBase)
	{
		FS_TRACE0( "FS_Free(): invalid\r\n" );
		return; 						/* nothing to free			*/
	}

	p = &GFS_MemHdr;						/* root of dyn memory		*/
	n = (FS_MemHdr *) (GFS_MemBase + p->next); /* first free block	*/

	/*
	** Skip through the Free Link List until we get to where the current pointer
	** should be added
	*/
	while ((FS_BYTE *) n < GFS_MemEnd && (FS_BYTE *) n < mem)
	{									/* search allocated area	*/
		p = n;
		n = (FS_MemHdr *) (GFS_MemBase + n->next);
	}

	/*
	** Check that the select memory isn't already free
	*/
	if (mem == (FS_BYTE *) p || mem == (FS_BYTE *) n)
	{
		FS_TRACE0( "FS_Free(): already free\r\n" );
		return; 						/* already free 			*/
	}

	/*
	** Memory not already free
	*/
		
	/*
	** This IFS_Memset should only be performed after we are sure that the memory should be freed
	*/
	IFS_Memset(mem, 0, size);

	if (p != &GFS_MemHdr && (FS_BYTE *) p + p->len == mem)
	{									/* adjacent to left free:	*/
		p->len += size; 				/* just add it				*/
	}
	else
	{
		p->next = (FS_UINT4) (mem - GFS_MemBase); /* new free link		*/
		p = (FS_MemHdr *) mem;			/* to new header			*/
		p->next = (FS_UINT4) ((FS_BYTE *) n - GFS_MemBase); /* link to next	*/
		p->len = size;
	}

	if ((FS_BYTE *) n < GFS_MemEnd && ((mem + size) == (FS_BYTE *) n))
	{									/* adjacent to right free:	*/
		p->next = n->next;				/* eliminate link and		*/
		p->len += n->len;				/* eat up the space 		*/
	}

	GFS_MemLeft += size;
}

#endif
