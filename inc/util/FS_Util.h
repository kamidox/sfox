#ifndef _FS_STD_LIB_H_
#define _FS_STD_LIB_H_

#include "inc\FS_Config.h"

#define FS_MIN( v1, v2 )		( (v1) > (v2) ? (v2) : (v1) )
#define FS_MAX( v1, v2 )		( (v1) > (v2) ? (v1) : (v2) )
#define FS_ABS( v )				( (v) > 0 ? (v) : -(v) )

#define FS_SAFE_FREE( a )		\
	do							\
	{							\
		if( a )					\
		{						\
			IFS_Free( a );		\
			a = FS_NULL;			\
		}						\
	} while( 0 )			


#define FS_COPY_TEXT( a, b )		\
	do								\
	{								\
		if( (a) )					\
		{							\
			IFS_Free( (a) );		\
			(a) = FS_NULL;			\
		}							\
		if( (b) && (b)[0] )				\
		{								\
			(a) = IFS_Strdup( (b) );	\
		}								\
	} while( 0 )

#define FS_NEW( a )			IFS_Malloc( sizeof(a) )

#define FS_CHAR_EQUAL( a, b )	((a) == (b) || (a) == (b - 32) )

#define FS_STR_I_EQUAL( a, b )	((a) && (b) && (IFS_Stricmp((a), (b)) == 0 ))

#define FS_STR_NI_EQUAL( a, b, l )	((a) && (b) && (IFS_Strnicmp((a), (b), (l)) == 0 ))

#define FS_KB(a)	(((a) + 1023) >> 10 )

#define FS_LE_BYTE_TO_UINT4(b)		(FS_UINT4)(((b)[0]) | ((b)[1] << 8) | ((b)[2] << 16) | ((b)[3] << 24))

#define FS_LE_BYTE_TO_UINT2(b)		(FS_UINT2)(((b)[0]) | ((b)[1] << 8))

#define FS_UINT4_TO_LE_BYTE( val, b )	\
	do {								\
		(b)[0] = (val) & 0xFF;			\
		(b)[1] = ((val) >> 8) & 0xFF;	\
		(b)[2] = ((val) >> 16) & 0xFF;	\
		(b)[3] = ((val) >> 24) & 0xFF;	\
	} while( 0 )

#define FS_IsWhiteSpace( b )		( (b) == ' ' || (b) == '\t' || (b) == '\r' || (b) == '\n' )

FS_CHAR * FS_GetLuid( FS_CHAR * out );

FS_CHAR *FS_StrConCat( FS_CHAR *str1, FS_CHAR *str2, FS_CHAR *str3, FS_CHAR *str4 );

void FS_ExtractQuote( FS_CHAR *str, FS_CHAR quote_chr );

FS_CHAR *FS_Strndup( FS_CHAR *str, FS_SINT4 len );

FS_SINT4 FS_FileRead( FS_SINT4 dir, FS_CHAR *filename, FS_SINT4 offset, void *buf, FS_SINT4 blen );

FS_SINT4 FS_FileWrite( FS_SINT4 dir, FS_CHAR *filename, FS_SINT4 offset, void *buf, FS_SINT4 blen );

FS_SINT4 FS_FileReadXLine( FS_CHAR *filename, FS_SINT4 offset, FS_CHAR *out, FS_SINT4 olen );

FS_SINT4 FS_FileGetSize( FS_SINT4 dir, FS_CHAR *filename );

void FS_DateStr2Struct( FS_DateTime *dt, FS_CHAR *str );

void FS_DateTimeFormatText( FS_CHAR *str, FS_DateTime *dt );

FS_SINT4 FS_DateStruct2DispStr( FS_CHAR *str, FS_DateTime *dt );

FS_SINT4 FS_DateStruct2DispStrShortForm( FS_CHAR *str, FS_DateTime *dt );

FS_UINT4 FS_GetDayNumByDateFromEpoch( FS_DateTime *dt );

FS_UINT4 FS_GetSeconds( FS_SINT4 zone );

void FS_GetDateByDayNumFromEpoch( FS_DateTime *dt, FS_UINT4 day_num );

void FS_SecondsToDateTime( FS_DateTime *dt, FS_UINT4 secs, FS_SINT4 zone );

/*
	get a line from str
	buf		out			the line data place here
	len		in			buf len
	str		in			input stream, will auto move the str position
*/
FS_CHAR *FS_GetLine(FS_CHAR *buf, FS_SINT4 len, FS_CHAR **str);

/*------------------------------standard lib interface-------------------------*/
void FS_MemMove( void *dst, void *src, FS_UINT4 size );

FS_CHAR * FS_StrNCpy( FS_CHAR *dst, FS_CHAR *src, FS_UINT4 size );

FS_SINT4 FS_TrimRight( FS_CHAR *str, FS_SINT4 len );

FS_SINT4 FS_TrimCrlf( FS_CHAR *str, FS_SINT4 len );

FS_SINT4 FS_TrimBlankSpace( FS_CHAR *str, FS_SINT4 len );

FS_CHAR FS_ToUpper( FS_CHAR c );

FS_CHAR *FS_StrStrI( FS_CHAR *str, FS_CHAR *sub );

FS_BOOL FS_ContentIdEqual( FS_CHAR *cid, FS_CHAR *str );

FS_UINT4 FS_HexValue( FS_CHAR *hexStr );

FS_SINT4 FS_OctetHexDump( FS_CHAR *str, FS_BYTE *data, FS_SINT4 dlen );

FS_SINT1 FS_UInt4ToUIntVar( FS_BYTE *uintvar, FS_UINT4 value );

FS_SINT1 FS_UIntVarToUInt4( FS_UINT4 *value, FS_BYTE *uintvar );

FS_CHAR *FS_ProcessCharset( FS_CHAR *str, FS_SINT4 slen, FS_CHAR *charset, FS_SINT4 *outlen );

FS_CHAR *FS_ProcessParagraph( FS_CHAR *text, FS_SINT4 len );

void FS_ProcessEsc( FS_CHAR *text, FS_SINT4 len );

FS_SINT4 FS_Rfc2047EncodeString( FS_CHAR *buf, FS_CHAR *str, FS_CHAR *charset );

void FS_Rfc2047DecodeString( FS_CHAR *out, FS_CHAR *str );

/* alloc memory inside. caller take resposibility to free it. */
FS_CHAR *FS_UrlGetDir( FS_CHAR *url );

/* alloc memory inside. caller take resposibility to free it. */
FS_CHAR *FS_UrlGetHost( FS_CHAR *url );

FS_CHAR *FS_ComposeAbsUrl( FS_CHAR *host, FS_CHAR *relateUrl );

FS_SINT4 FS_UrlEncode( FS_CHAR *out, FS_SINT4 out_buf_len, FS_CHAR *str, FS_SINT4 len );

/*--------------------------------system utility-------------------------------*/

#define FS_MSG_UTIL_CALL			0

#define FS_MSG_WEB_EVENT			0x1000

FS_BOOL FS_SystemInit( void );

void FS_SystemDeinit( void );

#define FS_APP_WEB		0x01
#define FS_APP_MMS		0x02
#define FS_APP_EML		0x04
#define FS_APP_DCD		0x08
#define FS_APP_STK		0x10
#define FS_APP_SNS		0x20
#define FS_APP_ALL		0xFFFFFFFF

void FS_ActiveApplication( FS_UINT4 app );

void FS_DeactiveApplication( FS_UINT4 app );

FS_BOOL FS_HaveActiveApplication( void );

FS_BOOL FS_ApplicationIsActive( FS_UINT4 app );

#endif
