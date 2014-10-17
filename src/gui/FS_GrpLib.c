#include "inc/gui/FS_GrpLib.h"
#include "inc/inte/FS_Inte.h"
#include "inc/util/FS_List.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_Charset.h"
#include "inc/gui/FS_Font.h"
#include "inc/util/FS_MemDebug.h"

// global var define
FS_DC GFS_DC;			// device content
// char bitmap
static FS_Bitmap GFS_CharBmp;
// clip rect list
typedef struct FS_ClipRect_Tag
{
	FS_List list;
	FS_Rect rect;
}FS_ClipRect;

static FS_BOOL GFS_Chipping = FS_TRUE;	// use chip rect or not
static FS_List GFS_ClipRectList = { &GFS_ClipRectList, &GFS_ClipRectList };

#ifdef FS_DEBUG_

static void FS_SetPixel( FS_BYTE *pDstPixel, FS_COLOR clr )
{
	if( GFS_DC.bytes_per_pixel == 1 )
	{								
		*(pDstPixel) = clr;
	}								
	else if( GFS_DC.bytes_per_pixel == 2 )
	{										
		*((FS_UINT2 *)(pDstPixel)) = clr;
	}											
	else if( GFS_DC.bytes_per_pixel == 3 )				
	{													
		*((FS_UINT2 *)(pDstPixel)) = clr;						
		*((pDstPixel) + 2) = clr >> 16; 						
	}													
	else if( GFS_DC.bytes_per_pixel == 4 )				
	{													
		*((FS_UINT4 *)(pDstPixel)) = clr;					
	}													
}

#else

#define FS_SetPixel(pDstPixel, clr)							\
do {														\
	if( GFS_DC.bytes_per_pixel == 1 )						\
	{														\
		*(pDstPixel) = clr;									\
	}														\
	else if( GFS_DC.bytes_per_pixel == 2 )					\
	{														\
		*((FS_UINT2 *)(pDstPixel)) = clr;						\
	}														\
	else if( GFS_DC.bytes_per_pixel == 3 ) 					\
	{														\
		*((FS_UINT2 *)(pDstPixel)) = clr;						\
		*((pDstPixel) + 2) = clr >> 16;						\
	}														\
	else if( GFS_DC.bytes_per_pixel == 4 )					\
	{														\
		*((FS_UINT4 *)(pDstPixel)) = clr;						\
	}														\
} while( 0 )	

#endif
//-------------------------------------------------------------
// init char bitmap
static FS_BOOL FS_InitCharBitmap( void )
{
	if( GFS_CharBmp.bits )	/* has inited, return */
		return FS_TRUE;
	
	GFS_CharBmp.bits = IFS_Malloc( GFS_DC.bytes_per_pixel * IFS_GetLineHeight( ) * IFS_GetLineHeight( ) );
	if( GFS_CharBmp.bits == FS_NULL )
		return FS_FALSE;
	GFS_CharBmp.width = IFS_GetLineHeight( );
	GFS_CharBmp.height = IFS_GetLineHeight( );
	GFS_CharBmp.pitch = GFS_DC.bytes_per_pixel * IFS_GetLineHeight( );
	return FS_TRUE;
}

static void FS_DeinitCharBitmap( void )
{
	if( GFS_CharBmp.bits )
	{
		IFS_Free( GFS_CharBmp.bits );
		GFS_CharBmp.bits = FS_NULL;
	}
	IFS_Memset( &GFS_CharBmp, 0, sizeof(GFS_CharBmp) );
}

FS_BOOL FS_PushClipRect( FS_Rect *rect, FS_UINT1 cType )
{
	FS_SINT4 bm;
	FS_ClipRect *clipRect = IFS_Malloc( sizeof(FS_ClipRect) );
	if( clipRect == FS_NULL )
		return FS_FALSE;
	
	if( rect )
	{
		clipRect->rect = *rect;
	}
	else
	{
		clipRect->rect.left = 0;
		clipRect->rect.top = 0;
		clipRect->rect.width = GFS_DC.width;;
		clipRect->rect.height = GFS_DC.height;
	}
	
	if( cType == FS_CLIP_CLIENT )
		bm = IFS_GetSoftkeyHeight( );
	else if( cType == FS_CLIP_CWSB )
		bm = IFS_GetSoftkeyHeight( ) + IFS_GetLineHeight( );
	else
		bm = 0;
	
	if( cType != FS_CLIP_NONE )
	{
		if( clipRect->rect.top < IFS_GetWinTitleHeight( ) )
			clipRect->rect.top = IFS_GetWinTitleHeight( );

		if( clipRect->rect.top + clipRect->rect.height > IFS_GetScreenHeight( ) - bm )
			clipRect->rect.height = IFS_GetScreenHeight( ) - bm - clipRect->rect.top;
	}
	FS_ListAdd( &GFS_ClipRectList, &clipRect->list );
	return FS_TRUE;
}

void FS_PopClipRect( void )
{
	FS_ClipRect *clipRect;
	if( ! FS_ListIsEmpty(&GFS_ClipRectList) )
	{
		clipRect = FS_ListEntry( GFS_ClipRectList.next, FS_ClipRect, list );
		FS_ListDel( GFS_ClipRectList.next );
		IFS_Free( clipRect );
	}
}
#if 0
//-------------------------------------------------------------
// expand char bits to char bitmap
static FS_Bitmap * FS_ExpandCharBitmap( FS_BYTE *pBits, FS_SINT4 w, FS_SINT4 h, FS_UINT2 attr )
{
	int x, y;
	FS_BYTE b, bold = 0;
	FS_BYTE *pExt = GFS_CharBmp.bits;
	FS_BYTE *pLine = GFS_CharBmp.bits;
	GFS_CharBmp.width = w;
	GFS_CharBmp.height = h;
	GFS_CharBmp.pitch = w * GFS_DC.bytes_per_pixel;
	GFS_CharBmp.bpp = GFS_DC.bytes_per_pixel;
	
	if( attr & FS_S_BOLD )
	{
		bold = 1;
		GFS_CharBmp.width ++;
		GFS_CharBmp.pitch =  GFS_CharBmp.width * GFS_DC.bytes_per_pixel;
	}
	
	for( y = 0; y < h; y ++ )
	{
		pExt = pLine;
		for( x = 0; x < w + bold; x ++ )
			FS_SetPixel( pExt + x * GFS_DC.bytes_per_pixel, GFS_DC.bg_color );
		
		for( x = 0; x < w; x ++ )
		{
			if( ! (x & 0x07) )
			{
				b = *pBits;
				pBits ++;
			}
			if( b & ( 0x80 >> (x & 0x07 ) ) )	// here, this pixel set
			{
				FS_SetPixel( pExt, GFS_DC.fg_color );
				if( bold ) FS_SetPixel( pExt + GFS_DC.bytes_per_pixel, GFS_DC.fg_color );
			}
			pExt += GFS_DC.bytes_per_pixel;
		}
		pLine += GFS_CharBmp.pitch;
	}

	return &GFS_CharBmp;
}
#endif

FS_Bitmap *FS_GetCharBitmap( FS_UINT2 nChar, FS_UINT2 attr )
{
	const FS_BYTE *pBits;
	FS_SINT4 cWidth, w, h;
	int x, y;
	FS_BYTE b, bold = 0;
	FS_BYTE *pExt = GFS_CharBmp.bits;
	FS_BYTE *pLine = GFS_CharBmp.bits;
	
	GFS_CharBmp.width = FS_GetCharWidth( nChar );
	GFS_CharBmp.height = FS_GetCharHeight( nChar );
	GFS_CharBmp.pitch = GFS_CharBmp.width * GFS_DC.bytes_per_pixel;
	GFS_CharBmp.bpp = GFS_DC.bytes_per_pixel;
	
	if( attr & FS_S_BOLD )
	{
		bold = 1;
	}

	w = GFS_CharBmp.width;
	h = GFS_CharBmp.height;
	cWidth = FS_GetCharBitmapWidth( nChar );
	pBits = FS_GetCharBitmapBits( nChar );
	
	for( y = 0; y < h; y ++ )
	{
		pExt = pLine;
		for( x = 0; x < w; x ++ )
			FS_SetPixel( pExt + x * GFS_DC.bytes_per_pixel, GFS_DC.bg_color );
		
		for( x = 0; x < cWidth; x ++ )
		{
			if( ! (x & 0x07) )
			{
				b = *pBits;
				pBits ++;
			}
			if( b & ( 0x80 >> (x & 0x07 ) ) )	// here, this pixel set
			{
				FS_SetPixel( pExt, GFS_DC.fg_color );
				if( bold ) FS_SetPixel( pExt + GFS_DC.bytes_per_pixel, GFS_DC.fg_color );
			}
			pExt += GFS_DC.bytes_per_pixel;
		}
		pLine += GFS_CharBmp.pitch;
	}

	return &GFS_CharBmp;

}

//-------------------------------------------------------------
// get the pixel color of the postion
FS_COLOR FS_GetPixel( FS_SINT4 x, FS_SINT4 y, FS_Bitmap *pBmp )
{
	FS_COLOR ret = 0;
	FS_BYTE *pSrc = FS_NULL;
	FS_UINT4 bpp;

	x = FS_MAX( x, 0 );
	y = FS_MAX( y, 0 );
	if( pBmp == FS_NULL )	// get the logical screen color
	{
		if( x < GFS_DC.width && y < GFS_DC.height )
		{
			pSrc = GFS_DC.bits + y * GFS_DC.pitch + x * GFS_DC.bytes_per_pixel;
			bpp = GFS_DC.bytes_per_pixel;
		}
	}
	else
	{
		if( x < pBmp->width && y < pBmp->height )
		{
			pSrc = pBmp->bits + y * pBmp->pitch + x * pBmp->bpp;
			bpp = pBmp->bpp;
		}
	}
	if( pSrc != FS_NULL )
	{
		/* 
		 * here is a temp solution of bigendian.
		 * we suppose when bpp is not the same to system bpp, it's a resource bitmap, witch pixel data is little endian.
		 * otherwise, we suppose that the pixel data's endian is the same to system.
		*/
		if( pBmp->bpp != GFS_DC.bytes_per_pixel && pBmp->bpp == 3 ){
			ret = (FS_COLOR)FS_LE_BYTE_TO_UINT4( pSrc );
			switch( bpp )
			{
				case 1:
					ret &= 0xFF;
					break;
				case 2:
					ret &= 0xFFFF;
					break;
				case 3:
					ret &= 0xFFFFFF;
					break;
				case 4:
					break;
			}
		}
		else
		{
			switch( bpp )
			{
				case 1:
					ret = *pSrc;
					break;
				case 2:
					ret = *((FS_UINT2 *)pSrc);
					break;
				case 3:
					ret = *((FS_UINT2 *)pSrc);
					ret |= ((*(pSrc + 2)) << 16);
					break;
				case 4:
					ret = *((FS_UINT4 *)pSrc);
					break;
			}
		}
	}

	return ret;
}
//-------------------------------------------------------------
// return two rect's intersaction area
static FS_Rect FS_GetIntersactionRect( FS_Rect *r1, FS_Rect *r2 )
{
	FS_Rect ret;
	FS_SINT4 xend, yend;

	ret.left = FS_MAX( r1->left, r2->left );
	ret.top = FS_MAX( r1->top, r2->top );
	xend = FS_MIN( r1->left + r1->width, r2->left + r2->width );
	yend = FS_MIN( r1->top + r1->height, r2->top + r2->height );
	ret.width = xend - ret.left;
	ret.height = yend - ret.top;
	/* have no intersaction area */
	if( ret.width < 0 || ret.height < 0 )
	{
		ret.left = 0;
		ret.top = 0;
		ret.width = 0;
		ret.height = 0;
	}
	return ret;
}
//-------------------------------------------------------------
// get the need redraw rect base on the clip rect
static void FS_GetClipRect( FS_Rect *rect )
{
	FS_Rect ret = {0 };
	ret.width = GFS_DC.width;
	ret.height = GFS_DC.height;
	if( GFS_Chipping )
	{
		FS_ClipRect *clipRect;
		FS_List *node = GFS_ClipRectList.next;
		while( node != &GFS_ClipRectList )
		{
			clipRect = FS_ListEntry( node, FS_ClipRect, list );
			ret = FS_GetIntersactionRect( &ret, &clipRect->rect );
			node = node->next;
		}
	}
	*rect = ret;
}

static void FS_GetRedrawRect( FS_Rect *rect )
{
	FS_Rect clip;
	FS_GetClipRect( &clip );
	*rect = FS_GetIntersactionRect( rect, &clip );
}

//-------------------------------------------------------------
// draw pixel
void FS_DrawPixel(FS_SINT4 x, FS_SINT4 y, FS_COLOR color, FS_Bitmap *pBmp)
{
	FS_BYTE *pDst;
	FS_Rect clip;
	FS_GetClipRect( &clip );
	if( pBmp == FS_NULL )	// draw directory to my logical screen
	{
		if( x >= clip.left	&&							
			y >= clip.top	&& 							
			x < (clip.left + clip.width) && 		
			y < (clip.top + clip.height) )		
		{
			pDst = GFS_DC.bits + y * GFS_DC.pitch + x * GFS_DC.bytes_per_pixel;
			FS_SetPixel( pDst, color );
		}
	}
	else	// draw to dst bitmap
	{
		if( x < pBmp->width && y < pBmp->height )
		{
			pDst = pBmp->bits + y * pBmp->pitch + x * pBmp->bpp;
			FS_SetPixel( pDst, color );
		}
	}
}

//-------------------------------------------------------------
// draw line
void FS_DrawLine(FS_SINT4 x1, FS_SINT4 y1, FS_SINT4 x2, FS_SINT4 y2, FS_COLOR color)
{
	FS_SINT4 xstep, ystep, xdelta, ydelta, rem;
	FS_Rect rect;
	FS_BYTE *pDst;
	FS_GetClipRect( &rect );
	
	if( x1 == x2 )
	{
		if( x1 < rect.left || x1 >= (rect.left + rect.width) ) return;
		if( y2 > y1 )
		{
			ystep = 1;
			y1 = FS_MAX( y1, rect.top);
			y2 = FS_MIN( y2, rect.top + rect.height );
			if( y2 <= y1 ) return;
		}
		if( y2 < y1 )
		{
			ystep = -1;
			y1 = FS_MIN( y1, rect.top + rect.height );
			y2 = FS_MAX( y2, rect.top );
			if( y2 >= y1 ) return;
		}
		xstep = ystep * GFS_DC.pitch;
		
		pDst = GFS_DC.bits + y1 * GFS_DC.pitch + x1 * GFS_DC.bytes_per_pixel;
		for( ydelta = y1; y1 != y2; y1 += ystep )
		{
			FS_SetPixel( pDst, color );
			pDst += xstep;
		}
	}
	else if( y1 == y2 )
	{
		if( y1 < rect.top || y1 > (rect.top + rect.height) ) return;
		if( x2 > x1 )
		{
			xstep = 1;
			x1 = FS_MAX( x1, rect.left );
			x2 = FS_MIN( x2, rect.left + rect.width );
			if( x2 < x1 ) return;
		}
		if( x2 < x1 )
		{
			xstep = -1;
			x1 = FS_MIN( x1, rect.left + rect.width );
			x2 = FS_MAX( x2, rect.left );
			if( x2 > x1 ) return;
		}
		ystep = xstep * GFS_DC.bytes_per_pixel;
		pDst = GFS_DC.bits + y1 * GFS_DC.pitch + x1 * GFS_DC.bytes_per_pixel;
		for( xdelta = x1; x1 != x2; x1 += xstep )
		{
			FS_SetPixel( pDst, color );
			pDst += ystep;
		}
	}	
	else	// draw bias
	{
		xdelta = x2 > x1 ? x2 - x1 : x1 - x2;
		ydelta = y2 > y1 ? y2 - y1 : y1 - y2;
		xstep = x2 > x1 ? 1 : -1;
		ystep = y2 > y1 ? 1 : -1;

		if( xdelta > ydelta )
		{
			rem = xdelta >> 1;
			while( x1 != x2 )
			{
				x1 += xstep;
				rem += ydelta;
				if( rem >= xdelta )
				{
					rem -= xdelta;
					y1 += ystep;
					FS_DrawPixel( x1, y1, color, FS_NULL );
				}
				else
				{
					FS_DrawPixel( x1, y1, color, FS_NULL);
				}
			}
		}
		else
		{
			rem = ydelta >> 1;
			while( y1 != y2 )
			{
				y1 += ystep;
				rem += xdelta;
				if( rem >= ydelta )
				{
					rem -= ydelta;
					x1 += xstep;
					FS_DrawPixel( x1, y1, color, FS_NULL );
				}
				else
				{
					FS_DrawPixel( x1, y1, color, FS_NULL );
				}
			}
		}
	}
}
//-------------------------------------------------------------
// fill rect
void FS_FillRect( FS_Rect *pRect, FS_COLOR color)
{
	FS_SINT4 xpos, ypos, x, y, x2, y2, pitch;
	FS_Rect rect = *pRect;
	FS_BYTE *pDst;
	FS_GetRedrawRect( &rect );
	x = rect.left;
	y = rect.top;
	x2 = rect.left + rect.width;
	y2 = rect.top + rect.height;
	
	pDst = GFS_DC.bits + y * GFS_DC.pitch + x * GFS_DC.bytes_per_pixel;
	pitch = (x2 - x) * GFS_DC.bytes_per_pixel;
	for( ypos = y; ypos < y2; ypos ++ )
	{
		for( xpos = x; xpos < x2; xpos ++ )
		{
			FS_SetPixel( pDst, color );
			pDst += GFS_DC.bytes_per_pixel;
		}
		pDst += (GFS_DC.pitch - pitch);
	}
}
//-------------------------------------------------------------
// draw rect
void FS_DrawRect(FS_Rect *pRect, FS_COLOR color)
{
	FS_Rect rect = *pRect;
	FS_DrawLine( rect.left, rect.top, rect.left, rect.top + rect.height, color );
	FS_DrawLine( rect.left, rect.top, rect.left + rect.width, rect.top, color );
	FS_DrawLine( rect.left, rect.top + rect.height, 
		rect.left + rect.width + 1, rect.top + rect.height, color );
	FS_DrawLine( rect.left + rect.width - 1, rect.top, 
		rect.left + rect.width - 1, rect.top + rect.height, color );
}

//-------------------------------------------------------------
// draw a trigon and fill it
void FS_FillTrigon( FS_Rect *pRect, FS_UINT1 dir, FS_COLOR color)
{
	FS_SINT4 x1, y1, w = 1, i;
	FS_Rect rect = *pRect;
	rect.left = rect.left + (rect.width >> 2);
	rect.width = rect.width >> 1;
	rect.top = rect.top + (rect.height >> 2);
	rect.height = rect.height >> 1;
		
	if( dir == FS_DIR_UP )
	{	
		x1 = rect.left + (rect.width >> 1);
		y1 = rect.top;
		for( i = 0; i < rect.height; i ++ )
		{
			FS_DrawLine( x1, y1, x1 + w, y1, color );
			x1 --;
			w += 2;
			y1 ++;
		}
	}
	else if( dir == FS_DIR_DOWN )
	{
		x1 = rect.left + (rect.width >> 1);
		y1 = rect.top + rect.height;
		for( i = 0; i < rect.height; i ++ )
		{
			FS_DrawLine( x1, y1, x1 + w, y1, color );
			x1 --;
			w += 2;
			y1 --;
		}
	}
	else if( dir == FS_DIR_RIGHT )
	{
		x1 = rect.left + rect.width;
		y1 = rect.top + (rect.height >> 1);
		for( i = 0; i < rect.width; i ++ )
		{
			FS_DrawLine( x1, y1, x1, y1 + w, color );
			x1 --;
			w += 2;
			y1 --;
		}
	}
	else
	{
		x1 = rect.left;
		y1 = rect.top + (rect.height >> 1);
		for( i = 0; i < rect.width; i ++ )
		{
			FS_DrawLine( x1, y1, x1, y1 + w, color );
			x1 ++;
			w += 2;
			y1 --;
		}
	}
}
//-------------------------------------------------------------
// draw bitmap to logial screen
// !!!note: bitmap's bytes per pixel MUST the same of DC's bytes per pixel
void FS_DrawBitmap( FS_SINT4 x, FS_SINT4 y, FS_Bitmap *pBmp, FS_UINT2 attr )
{
	FS_BYTE *pDst, *pSrc;
	FS_SINT4 x1 = -1, y1 = -1, i, j, w, h, pitch;
	FS_Rect clip;
	FS_COLOR pixel;	
	FS_GetClipRect( &clip );

	if( x + pBmp->width < clip.left	||							
		y + pBmp->height < clip.top	||		
		x > (clip.left + clip.width) || 
		y > (clip.top + clip.height) )		// all in clip rect
	{
		return;
	}	
	x1 = FS_MAX( x, clip.left );
	y1 = FS_MAX( y, clip.top );
	w = FS_MIN( pBmp->width + x - x1, clip.width + clip.left - x );
	h = FS_MIN( pBmp->height + y - y1, clip.height + clip.top - y );
	w = FS_MIN( w, clip.width );
	h = FS_MIN( h, clip.height );
	
	pDst = GFS_DC.bits + y1 * GFS_DC.pitch + x1 * GFS_DC.bytes_per_pixel;
	pSrc = pBmp->bits + (y1 - y) * pBmp->pitch + (x1 - x) * pBmp->bpp;
	pitch = w * GFS_DC.bytes_per_pixel;
	
	// handle for transparent
	if( attr & FS_S_TRANS )
	{
		for( i = y1; i < y1 + h; i ++ )
		{
			for( j = x1; j < x1 + w; j ++ )
			{
				pixel = FS_GetPixel( j - x, i - y, pBmp );
				/* Not match to platform bpp, default to DIB RGB, convert it to DDB RGB */
				if( pBmp->bpp != GFS_DC.bytes_per_pixel && pBmp->bpp == 3 )
					pixel = IFS_DDB_RGB( (FS_BYTE)(pixel >> 16), (FS_BYTE)(pixel >> 8), (FS_BYTE)pixel );
					
				/* if match for tranparent color, let it be */
				if( pixel != GFS_DC.trans_color )
					FS_SetPixel( pDst, pixel );
				
				pDst += GFS_DC.bytes_per_pixel;
			}
			pDst += ( GFS_DC.pitch - pitch );
		}
	}
	else	
	{	/* copy bitmap data */
		if( pBmp->bpp == GFS_DC.bytes_per_pixel )
		{
			for( i = y1; i < y1 + h; i ++ )
			{
				IFS_Memcpy( pDst, pSrc, GFS_DC.bytes_per_pixel * w );
				pDst += GFS_DC.pitch;
				pSrc += pBmp->pitch;
			}
		}
		else
		{
			/* TODO, convert to DDB bitmap */
		}
	}
}

//-------------------------------------------------------------
// draw one char, return the char width
FS_SINT4 FS_DrawChar(FS_SINT4 x, FS_SINT4 y, FS_UINT2 nChar, FS_UINT2 attr)
{
#ifdef FS_USE_NATIVE_DRAW_CHAR
	FS_SINT4 cWidth, cHeight;
	FS_Rect clip;

	cWidth = IFS_GetCharWidth( nChar );
	cHeight = IFS_GetCharHeight( nChar );
	FS_GetClipRect( &clip );
	if( x >= clip.left + clip.width || y >= clip.top + clip.height )
		return cWidth;
	if( x + cWidth <= clip.left || y + cHeight <= clip.top )
		return cWidth;

	return IFS_DrawChar( x, y, nChar, attr, GFS_DC.fg_color, GFS_DC.bg_color );
#else 
	FS_Bitmap *pCharBmp;
	FS_SINT4 cWidth, cHeight;
	FS_Rect clip;
	FS_GetClipRect( &clip );

	pCharBmp = FS_GetCharBitmap( nChar, attr );
	// do some clip here
	cWidth = pCharBmp->width;
	cHeight = pCharBmp->height;
	if( x >= clip.left + clip.width || y >= clip.top + clip.height )
		return cWidth;
	if( x + cWidth <= clip.left || y + cHeight <= clip.top )
		return cWidth;
	FS_DrawBitmap( x, y, pCharBmp, attr );
	
	if( attr & FS_S_UNDERLINE )
	{
		y += FS_GetCharMaxHeight();
		FS_DrawLine( x, y , x + cWidth, y, GFS_DC.fg_color );
	}

	return pCharBmp->width;
#endif // end FS_USE_NATIVE_DRAW_CHAR
}

//-------------------------------------------------------------
// draw astring
// !!!NOTE: pStr must be ASCII or GB code, and terminate by zero
void FS_DrawString(FS_SINT4 x, FS_SINT4 y, FS_CHAR * pStr, FS_UINT2 attr)
{
	FS_SINT4 i, len, dotW, charW, dotNum;
	FS_UINT2 nChar;
	FS_Rect clip;
	FS_BOOL bOverFlow = FS_FALSE;
	FS_GetClipRect( &clip );
	
	if( x > clip.left + clip.width || y > clip.top + clip.height )
		return;

	dotW = FS_StringWidth( "." );
	i = FS_StringWidth( pStr );
	if( x + i > GFS_DC.width )
		bOverFlow = FS_TRUE;

	dotNum = 0;
	i = 0;
	len = IFS_Strlen( pStr );
	while( i < len )
	{
		i += FS_CnvtUtf8ToUcs2Char( pStr + i, &nChar );
		charW = FS_GetCharWidth( nChar );

		if( bOverFlow && x + charW >= ((clip.left + clip.width) - (dotW * 3)) )
		{
			nChar = '.';
			dotNum ++;
		}
		
		if( x >= clip.left + clip.width - IFS_GetWidgetSpan( ) )
			break;
		
		x += FS_DrawChar( x, y, nChar, attr );
		
		if( dotNum >= 3 )
			break;
	}
}

FS_SINT4 FS_DrawMultiLineString( FS_SINT4 initX, FS_Rect *rect, FS_CHAR * pStr, FS_UINT2 attr )
{
	FS_SINT4 i, len;
	FS_UINT2 nChar;
	FS_SINT4 x = initX, y = rect->top, cW, cH = FS_TEXT_H;
	FS_Rect clip;

	FS_GetClipRect( &clip );
	if( x > clip.left + clip.width || y > clip.top + clip.height )
		return 0;

	i = 0;
	len = IFS_Strlen( pStr );
	while( i < len )
	{
		i += FS_CnvtUtf8ToUcs2Char( pStr + i, &nChar );
		cW = FS_GetCharWidth( nChar );
				
		if( x + cW > rect->left + rect->width )
		{
			x = rect->left;
			y += cH;
			if( y + cH > clip.height + clip.top )
				break;
			if( y + cH > rect->top + rect->height )
				break;
		}
		if( nChar >= 0x20 )	// visiable char
		{
			if( y >= clip.top )
				x += FS_DrawChar( x, y, nChar, attr );
			else
				x += cW;
		}
		else if( nChar == '\n' )	// LF
		{
			x = rect->left;
			y += cH;
			if( y + cH > clip.height + clip.top )
				break;
			if( y + cH > rect->top + rect->height )
				break;
		}
		else if( nChar == '\t' )
		{
			x += cH;
		}
	}
	
	return ( y - rect->top );
}

//-------------------------------------------------------------
// measure the string width
FS_SINT4 FS_StringWidth( FS_CHAR *pStr )
{
	FS_SINT4 ret = 0, i, len;
	FS_UINT2 nChar;

	if( pStr == FS_NULL ) return 0;
	i = 0;
	len = IFS_Strlen( pStr );
	while( i < len )
	{
		i += FS_CnvtUtf8ToUcs2Char( pStr + i, &nChar );
		ret += FS_GetCharWidth( nChar );
	}
	return ret;
}

//-------------------------------------------------------------
// measure the string width
FS_SINT4 FS_StringHeight( FS_CHAR *pStr, FS_SINT4 initX, FS_SINT4 lineW, FS_SINT4 *modWidth )
{
	FS_SINT4 i, len;
	FS_UINT2 nChar;
	FS_SINT4 cW, x, y, cH = FS_TEXT_H;
	
	y = cH;
	x = initX;
	i = 0;
	len = IFS_Strlen( pStr );
	while( i < len )
	{
		i += FS_CnvtUtf8ToUcs2Char( pStr + i, &nChar );
		cW = FS_GetCharWidth( nChar );
		
		if( x + cW > lineW )
		{
			x = 0;
			y += cH;
		}
		x += cW;
		if( nChar == '\n' )	// LF
		{
			x = 0;
			y += cH;
		}
		else if( nChar == '\t' )
		{
			x += cH;
		}
	}

	if( modWidth )
		*modWidth = x;
	return y;
}

//-------------------------------------------------------------
// set fg color
FS_COLOR FS_SetFgColor( FS_COLOR color )
{
	FS_COLOR ret = GFS_DC.fg_color;
	GFS_DC.fg_color = color;
	return ret;
}
//-------------------------------------------------------------
// set bg color
FS_COLOR FS_SetBgColor( FS_COLOR color )
{
	FS_COLOR ret = GFS_DC.bg_color;
	GFS_DC.bg_color = color;
	return ret;
}
//-------------------------------------------------------------
// set transparent color
FS_COLOR FS_SetTransColor( FS_COLOR color )
{
	FS_COLOR ret = GFS_DC.trans_color;
	GFS_DC.trans_color = color;
	return ret;
}

void FS_SetChipFlag( FS_BOOL bChip )
{
	GFS_Chipping = bChip;
}

//-------------------------------------------------------------
// init the graphics lib
FS_BOOL FS_InitGrpLib( void )
{
	FS_DC *pDc = &GFS_DC;
	
	IFS_Memset( pDc, 0, sizeof(FS_DC) );
	pDc->bits_per_pixel = IFS_GetBitsPerPixel( );
	pDc->height = IFS_GetScreenHeight( );
	pDc->width = IFS_GetScreenWidth( );
	pDc->bits = IFS_GetScreenBitmap( );
	
	pDc->bytes_per_pixel = (pDc->bits_per_pixel + 7) >> 3;	
	pDc->pitch = pDc->bytes_per_pixel * pDc->width;
	
	IFS_Memset( pDc->bits, 0xFF, pDc->height * pDc->width * pDc->bytes_per_pixel );
	pDc->fg_color = IFS_DDB_RGB( 0, 0, 0 );
	pDc->bg_color = IFS_DDB_RGB( 0xFF, 0xFF, 0xFF );
	pDc->trans_color = GFS_DC.bg_color;
	return FS_InitCharBitmap( );
}

void FS_DeinitGrpLib( void )
{
	FS_DC *pDc = &GFS_DC;
	FS_DeinitCharBitmap( );		
	if( pDc->bits )
	{
		IFS_ReleaseScreenBitmap( pDc->bits );
		pDc->bits = FS_NULL;
	}
	while( ! FS_ListIsEmpty(&GFS_ClipRectList) )
	{
		FS_PopClipRect( );
	}
	IFS_Memset( pDc, 0, sizeof(FS_DC) );
}

