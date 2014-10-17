#ifndef _FS_CHARSET_H_
#define _FS_CHARSET_H_

#include "inc/FS_Config.h"

FS_BOOL FS_CnvtUtf8ToGBK( const FS_CHAR *utf8Str, FS_SINT4 utf8Len, FS_CHAR *gbStr, FS_SINT4 *outLen );

FS_BOOL FS_CnvtUcs2ToGBK( const FS_UINT2* unicodStr, FS_UINT2 unicodLen, FS_CHAR* gbStr, FS_UINT2 * outLen );

FS_BOOL FS_CnvtGBKToUtf8( FS_CHAR* gbStr, FS_SINT4 gbLen, FS_CHAR* utf8Str, FS_SINT4 *utf8Len );

FS_BOOL FS_CnvtGBKToUcs2( FS_CHAR* gbStr, FS_SINT4 gbLen, FS_WCHAR* ucs2Str, FS_SINT4 *ucs2Len );

FS_UINT2 FS_CnvtUcs2ToGBKChar( FS_WCHAR nCode );

FS_SINT4 FS_CnvtUtf8ToUcs2Char( const FS_CHAR* utf8Char, FS_UINT2 *ucs );

FS_SINT4 FS_CnvtUcs2ToUtf8Char( FS_WCHAR ucs2Char, FS_CHAR *utf8Str );

FS_UINT2 FS_CnvtGBKToUcs2Char( FS_WCHAR nCode );

FS_SINT4 FS_CnvtUtf8ToUcs2( const FS_CHAR* utf8, FS_SINT4 utf8len, FS_UINT2 *ucs, FS_SINT4 ucslen );

FS_SINT4 FS_CnvtUcs2ToUtf8( FS_WCHAR *ucs2, FS_SINT4 ucs2len, FS_CHAR *utf8, FS_SINT4 utf8_len );

#endif
