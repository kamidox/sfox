#ifndef _FS_FONT_H_
#define _FS_FONT_H_

#include "inc\FS_Config.h"

const FS_BYTE * FS_GetCharBitmapBits( FS_UINT2 nChar );

FS_SINT4 FS_GetCharWidth( FS_UINT2 nChar );

FS_SINT4 FS_GetCharBitmapWidth( FS_UINT2 nChar );

FS_SINT4 FS_GetCharHeight( FS_UINT2 nChar );

FS_SINT4 FS_GetCharMaxHeight( void );

#endif
