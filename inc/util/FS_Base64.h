#ifndef _FS_BASE64_H_
#define _FS_BASE64_H_
#include "inc\FS_Config.h"
#include "inc\inte\FS_Inte.h"

FS_SINT4 FS_Base64Encode( FS_CHAR *out, FS_BYTE *in, FS_SINT4 inlen );

FS_SINT4 FS_Base64EncodeFile( FS_CHAR *srcFile, FS_SINT4 s_off, FS_CHAR *dstFile, FS_SINT4 d_off );

FS_SINT4 FS_Base64EncodeLine( FS_CHAR *out, FS_BYTE *in, FS_SINT4 inlen );

FS_SINT4 FS_Base64Decode( FS_BYTE * out, FS_CHAR * in, FS_SINT4 inlen );

void * FS_B64NewDecoder( void );

void FS_B64FreeDecoder( void * decoder);

FS_SINT4 FS_B64DecoderDecode( void *hDecoder, FS_CHAR *in, FS_BYTE *out);

FS_SINT4 FS_QPEncode( FS_BYTE * src, FS_SINT4 srcLen, FS_CHAR * dst );

FS_SINT4 FS_QPDecode( FS_BYTE * out, FS_CHAR * in, FS_SINT4 inlen );

FS_CHAR FS_Hex2Chr( FS_BYTE n );

FS_BYTE FS_Chr2Hex( FS_CHAR c );

#endif
