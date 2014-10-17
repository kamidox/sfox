#ifndef _FS_GRP_LIB_H_
#define _FS_GRP_LIB_H_

#include "inc\FS_Config.h"
//#include "inc\gui\FS_Font.h"

#define FS_R( c )				( c & 0xFF )
#define FS_G( c )				( (c >> 8) & 0xFF )
#define FS_B( c )				( (c >> 16) & 0xFF )

#define FS_DIB_RGB( r, g, b )		(((r << 16) | (g << 8) | b) & 0x00FFFFFF)

#define FS_S_NONE				0x00
#define FS_S_TRANS				0x01		// transparent
#define FS_S_BOLD				0x02		// bold
#define FS_S_ITALIC				0x04		// italic
#define FS_S_UNDERLINE			0x08		// under line

#define FS_TEXT_H				(IFS_GetLineHeight( ) - IFS_GetWidgetSpan( ))

#define FS_CLIP_NONE 				0
#define FS_CLIP_CLIENT				1
#define FS_CLIP_CWSB				2	/* client with status bar */

#define FS_DIR_UP					1
#define FS_DIR_DOWN				2
#define FS_DIR_LEFT					3
#define FS_DIR_RIGHT				4

// graphics interface declare
FS_BOOL		FS_InitGrpLib( void );
void 		FS_DeinitGrpLib( void );
FS_COLOR	FS_GetPixel( FS_SINT4 x, FS_SINT4 y, FS_Bitmap *pBmp );
FS_SINT4 	FS_DrawChar(FS_SINT4 x, FS_SINT4 y, FS_UINT2 nChar, FS_UINT2 attr);
void		FS_DrawBitmap( FS_SINT4 x, FS_SINT4 y, FS_Bitmap *pBmp, FS_UINT2 attr );
void		FS_DrawRect(FS_Rect *rect, FS_COLOR color);
void		FS_FillRect( FS_Rect *rect, FS_COLOR color);
void		FS_FillTrigon( FS_Rect *pRect, FS_UINT1 dir, FS_COLOR color);
void		FS_DrawPixel(FS_SINT4 x, FS_SINT4 y, FS_COLOR color, FS_Bitmap *pBmp);
void		FS_DrawLine(FS_SINT4 x1, FS_SINT4 y1, FS_SINT4 x2, FS_SINT4 y2, FS_COLOR color);
void		FS_DrawString(FS_SINT4 x1, FS_SINT4 y1, FS_CHAR *pStr, FS_UINT2 attr);
FS_SINT4	FS_DrawMultiLineString( FS_SINT4 initX, FS_Rect *rect, FS_CHAR * pStr, FS_UINT2 attr );
FS_SINT4	FS_StringHeight( FS_CHAR *str, FS_SINT4 initX, FS_SINT4 lineWF, FS_SINT4 *modWidth );
FS_SINT4	FS_StringWidth( FS_CHAR *str );

// return the original fg color
FS_COLOR	FS_SetFgColor( FS_COLOR color );
FS_COLOR	FS_SetBgColor( FS_COLOR color );
FS_COLOR	FS_SetTransColor( FS_COLOR color );
FS_BOOL		FS_PushClipRect( FS_Rect *rect, FS_UINT1 cType );
void		FS_PopClipRect( void );
void		FS_SetChipFlag( FS_BOOL bChip );
FS_SINT4	IFS_GetScreenWidth( void );
FS_SINT4	IFS_GetScreenHeight( void );

#endif
