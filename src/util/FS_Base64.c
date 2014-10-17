#include "inc/inte/FS_Inte.h"
#include "inc/util/FS_Base64.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_MemDebug.h"

typedef struct FS_B64Decoder_Tag
{
	FS_SINT4 buf_len;
	FS_CHAR buf[4];
}FS_B64Decoder;

#define FS_B64E_LINE			76
#define FS_B64_LINE				(FS_B64E_LINE * 3 / 4)
#define FS_B64_BLOCK			(4 * 1024)	// 4K
#define FS_B64E_BLOCK			((FS_B64_BLOCK * 4 / 3 ) + 3 + (FS_B64_BLOCK * 2 / FS_B64_LINE) + 4)

static const FS_CHAR GFS_B64Char[64] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const FS_CHAR GFS_B64Val[128] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
	-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
	-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1
};

#define FS_B64VAL(c)	(((FS_BYTE)c < 128) ? GFS_B64Val[(FS_BYTE)(c)] : -1)

/*	4bit binary to FS_CHAR 0-F  */
FS_CHAR FS_Hex2Chr( FS_BYTE n )
{
	n &= 0xF;
	if ( n < 10 )
		return ( FS_CHAR )( n + '0' );
	else
		return ( FS_CHAR )( n - 10 + 'A' );
}

/* FS_CHAR 0-F to 4bit binary */
FS_BYTE FS_Chr2Hex( FS_CHAR c )
{
	if ( c >= 'a' && c <= 'z' )  //  it's toupper
		c = c - 'a' + 'A';
	if ( c >= '0' && c <= '9' )
		return ( FS_SINT4 )( c - '0' );
	else if ( c >= 'A' && c <= 'F' )
		return ( FS_SINT4 )( c - 'A' + 10 );
	else
		return 0xFF;
}
//---------------------------------------------------------------------------
//	aLen 为 aSrc 的大小， aDest 所指的缓冲区必须至少为 aLen 的 3 倍！！！
//	返回 aDest 的长度
FS_SINT4 FS_QPEncode( FS_BYTE * src, FS_SINT4 srcLen, FS_CHAR* dst )
{
	FS_CHAR * p = dst;
	FS_SINT4    i = 0;

	if( srcLen < 0 )
		srcLen = IFS_Strlen( src );
	while ( i++ < srcLen )
	{
		*p++ = '=';
		*p++ = FS_Hex2Chr( (FS_BYTE)(*src >> 4) );
		*p++ = FS_Hex2Chr( *src++ );
	}
	*p = 0;  				//  aDest is an ASCIIZ string
	return ( p - dst );  	//  exclude the end of zero
}
/*
	out buffer size must as large as in.
*/
FS_SINT4 FS_QPDecode( FS_BYTE * out, FS_CHAR * in, FS_SINT4 inlen )
{
	FS_BYTE * p = out;
	FS_SINT4 n ;
	FS_BYTE	ch, cl;
	FS_CHAR * startSrc = in;		

	if( inlen <= 0 )
		n = IFS_Strlen( in );
	else
		n = inlen;
	
	while ( *in )  //  aSrc is an ASCIIZ string
	{
		if ( ( *in == '=' ) && ( n - 2 > 0 ) )
		{
			ch = FS_Chr2Hex( in[1] );
			cl = FS_Chr2Hex( in[2] );
			if ( ( ch == 0xFF ) || ( cl == 0xFF ) )
			{
				//ch = cl = 0xFF, We're in the end of the line, lintc, IGNORE it! VIDB00016264
				if( ch == cl )
					in += 3;
				else
					*p++ = *in++;
			}
			else
			{
				*p++ = ( ch << 4 ) | cl;
				in += 3;
			}
		}
		else
			*p++ = *in++;
		
		if( in - startSrc >= n )	/*如果到达了长度要求，则跳出循环 add by huangyc(Joey.Huang)*/
			break;
	}
	*p = '\0';
	return ( p - out );
}

FS_SINT4 FS_Base64Encode( FS_CHAR *out, FS_BYTE *in, FS_SINT4 inlen )
{
	FS_CHAR * p = out;
	FS_SINT4 i, slen = 0;
	FS_BYTE t;
	if( inlen < 0 )
		inlen = IFS_Strlen( in );
	
	for ( i = 0; i < inlen; i++ )
	{
		switch ( i % 3 )
		{
		case 0 :
			*p++ = GFS_B64Char[(FS_BYTE)(*in >> 2) & 0x3F];
			t = ( *in << 4 ) & 0x3F;
			in ++;
			break;
		case 1 :
			*p++ = GFS_B64Char[(FS_BYTE)(t | ( *in >> 4 )) & 0x3F];
			t = ( *in << 2 ) & 0x3F;
			in ++;
			break;
		case 2 :
			*p++ = GFS_B64Char[(FS_BYTE)(t | ( *in >> 6 )) & 0x3F];
			*p++ = GFS_B64Char[ *in  & 0x3F ];
			in ++;
			break;
		}
		slen ++;
		if( ( slen % FS_B64_LINE ) == 0 )
		{
			*p++ = '\r';
			*p++ = '\n';
		}
	}
	if ( inlen % 3 != 0 )
	{
		*p++ =  GFS_B64Char[ t & 0x3F ];
		if ( inlen % 3 == 1 )
			*p++ = '=';
		*p++ = '=';
	}
	*p++ = '\r';
	*p++ = '\n';
	*p = 0;
	return ( p - out );  //  exclude the end of zero
}
/*	
	encode a file, write to dst file directly
*/
FS_SINT4 FS_Base64EncodeFile( FS_CHAR *srcFile, FS_SINT4 s_off, FS_CHAR *dstFile, FS_SINT4 d_off )
{
	FS_CHAR *dst;
	FS_CHAR *p;
	FS_SINT4 i, ii, srcLen, size, slen = 0, total, ret = 0, srcSize = 0, rlen;
	FS_BYTE t, *src, *srcBuf;
	
	src = IFS_Malloc( FS_B64_BLOCK );

	FS_ASSERT( src != FS_NULL );
	if( src == FS_NULL ) return 0;
	
	dst = IFS_Malloc( FS_B64E_BLOCK );
	FS_ASSERT( dst != FS_NULL );
	if( dst == FS_NULL ){
		IFS_Free( src );
		return 0;
	}

	
	size = FS_FileGetSize( -1, srcFile ) - s_off;
	srcSize = size;
	srcBuf = src;
	total = size / FS_B64_BLOCK;
	srcLen = size > FS_B64_BLOCK ? FS_B64_BLOCK : size;
	slen = 0;
	for( ii = 0; ii <= total && srcLen > 0; ii ++ )
	{
		rlen = FS_FileRead( -1, srcFile, s_off, srcBuf, srcLen );
		FS_ASSERT( rlen == srcLen );
		s_off += rlen;
		
		p = dst;
		src = srcBuf;
		for ( i = 0; i < srcLen; i++ )
		{
			switch ( slen % 3 )
			{
			case 0 :
				*p++ = GFS_B64Char[(FS_BYTE)(*src >> 2) & 0x3F];
				t = ( *src << 4 ) & 0x3F;
				src ++;
				break;
			case 1 :
				*p++ = GFS_B64Char[(FS_BYTE)(t | ( *src >> 4 )) & 0x3F];
				t = ( *src << 2 ) & 0x3F;
				src ++;
				break;
			case 2 :
				*p++ = GFS_B64Char[(FS_BYTE)(t | ( *src >> 6 )) & 0x3F];
				*p++ = GFS_B64Char[ *src  & 0x3F ];
				src ++;
				break;
			}
			slen ++;
			if( slen % FS_B64_LINE == 0 )	/* 实现一行的字符不超过FS_B64E_LINE个 */
			{
				*p++ = '\r';
				*p++ = '\n';
			}
		}
		rlen = FS_FileWrite( -1, dstFile, d_off, dst, (FS_UINT4)(p - dst) );
		FS_ASSERT( rlen == (FS_SINT4)(p - dst) );
		d_off += rlen;
		
		size -= srcLen;
		srcLen = size > FS_B64_BLOCK ? FS_B64_BLOCK : size;
		ret += rlen;
	}

	p = dst;
	if ( srcSize % 3 != 0 )
	{	
		*p++ =	GFS_B64Char[ t & 0x3F ];
		if ( srcSize % 3 == 1 )
			*p++ = '=';
		*p++ = '=';				
	}
	*p++ = '\r';
	*p++ = '\n';
	rlen = FS_FileWrite( -1, dstFile, d_off, dst, (FS_UINT4)(p - dst) );
	FS_ASSERT( rlen == (FS_SINT4)(p - dst) );
	d_off += rlen;
	ret += rlen;
	
	IFS_Free( dst );
	IFS_Free( srcBuf );
	return ret;  //  exclude the end of zero
}

// encode a line, did not contain CRLF
FS_SINT4 FS_Base64EncodeLine( FS_CHAR *dst, FS_BYTE *src, FS_SINT4 srcLen )
{
	FS_CHAR * p = dst;
	FS_SINT4 i;
	FS_BYTE t;
	if( srcLen < 0 )
		srcLen = IFS_Strlen( src );
	
	for ( i = 0; i < srcLen; i++ )
	{
		switch ( i % 3 )
		{
		case 0 :
			*p++ = GFS_B64Char[(FS_BYTE)(*src >> 2) & 0x3F];
			t = ( *src << 4 ) & 0x3F;
			src ++;
			break;
		case 1 :
			*p++ = GFS_B64Char[(FS_BYTE)(t | ( *src >> 4 )) & 0x3F];
			t = ( *src << 2 ) & 0x3F;
			src ++;
			break;
		case 2 :
			*p++ = GFS_B64Char[(FS_BYTE)(t | ( *src >> 6 )) & 0x3F];
			*p++ = GFS_B64Char[ *src  & 0x3F ];
			src ++;
			break;
		}
	}
	if ( srcLen % 3 != 0 )
	{
		*p++ =	GFS_B64Char[ t & 0x3F ];
		if ( srcLen % 3 == 1 )
			*p++ = '=';
		*p++ = '=';
	}
	*p = 0;
	return ( p - dst );  //  exclude the end of zero
}

/*
	aDest 所指的缓冲区必须至少为 aSrc 长度的 0.75 倍！！！
	返回 aDest 的长度
	sLen指定aSrc的长度，如果为-1则aSrc必须以'\0'结尾	--add by huangyc(Joey.Huang)
*/
FS_SINT4 FS_Base64Decode( FS_BYTE * out, FS_CHAR * in, FS_SINT4 inlen )
{
	const FS_CHAR *inp = in;
	FS_BYTE *outp = out;
	FS_CHAR buf[4];
	
	if (inlen < 0)
		inlen = IFS_Strlen( in );
	
	while (inlen >= 4 && *inp != '\0') {
		buf[0] = *inp++;
		inlen--;
		if( FS_B64VAL(buf[0]) == -1 )
			break;
	
		buf[1] = *inp++;
		inlen--;
		if (FS_B64VAL(buf[1]) == -1) break;
	
		buf[2] = *inp++;
		inlen--;
		if (buf[2] != '=' && FS_B64VAL(buf[2]) == -1) break;
	
		buf[3] = *inp++;
		inlen--;
		if (buf[3] != '=' && FS_B64VAL(buf[3]) == -1) break;
	
		*outp++ = ((FS_B64VAL(buf[0]) << 2) & 0xfc) |
			  ((FS_B64VAL(buf[1]) >> 4) & 0x03);
		if (buf[2] != '=') {
			*outp++ = ((FS_B64VAL(buf[1]) & 0x0f) << 4) |
				  ((FS_B64VAL(buf[2]) >> 2) & 0x0f);
			if (buf[3] != '=') {
				*outp++ = ((FS_B64VAL(buf[2]) & 0x03) << 6) |
					   (FS_B64VAL(buf[3]) & 0x3f);
			}
		}
	}
	*outp = '\0';
	return outp - out;
}
//---------------------------------------------------------------------------

void * FS_B64NewDecoder( void )
{
	FS_B64Decoder *decoder;

	decoder = IFS_Malloc( sizeof(FS_B64Decoder) );
	if( decoder )
		IFS_Memset( decoder, 0, sizeof(FS_B64Decoder) );
	return decoder;
}

void FS_B64FreeDecoder( void * decoder)
{
	IFS_Free(decoder);
}

FS_SINT4 FS_B64DecoderDecode( void *hDecoder, FS_CHAR *in, FS_BYTE *out)
{
	FS_SINT4 len, total_len = 0;
	FS_SINT4 buf_len;
	FS_CHAR buf[4];
	FS_B64Decoder *decoder = (FS_B64Decoder *)hDecoder;
	
	buf_len = decoder->buf_len;
	IFS_Memcpy(buf, decoder->buf, sizeof(buf));

	for (;;) {
		// process 4 bytes per time
		while (buf_len < 4) {
			FS_CHAR c = *in;

			in++;
			if (c == '\0') break;
			if (c == '\r' || c == '\n') continue;
			if (c != '=' && FS_B64VAL(c) == -1)
				return total_len;
			buf[buf_len++] = c;
		}
		if (buf_len < 4 || buf[0] == '=' || buf[1] == '=') {
			decoder->buf_len = buf_len;
			IFS_Memcpy(decoder->buf, buf, sizeof(buf));
			return total_len;
		}
		len = FS_Base64Decode(out, buf, 4);
		out += len;
		total_len += len;
		buf_len = 0;
		if (len < 3) {
			decoder->buf_len = 0;
			return total_len;
		}
	}
}

