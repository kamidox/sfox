#include "inc\util\FS_Util.h"
#include "inc\inte\FS_Inte.h"
#include "inc\util\FS_File.h"
#include "inc\util\FS_Base64.h"
#include "inc\gui\FS_gui.h"
#include "inc\util\FS_Charset.h"
#include "inc\util\FS_NetConn.h"
#include "inc\mms\FS_MmsConfig.h"
#include "inc\web\FS_WebConfig.h"
#include "inc\util\FS_MemDebug.h"

void FS_ResDeinit( void );

FS_CHAR * FS_GetLuid( FS_CHAR * out )
{
	static FS_UINT4 s_seed = 0;
	FS_CHAR str[16];
	FS_SINT4 len;

	if( s_seed == 0 ) s_seed = FS_GetSeconds( 0 );
	
	IFS_Itoa( s_seed ++, str, 10 );
	len = 10 - IFS_Strlen(str);
	out[0] = 0;
	if( len > 0 )
	{
		IFS_Strncpy( out, "000000000000000", len );
		out[len] = 0;
	}
	IFS_Strcat( out, str );
	return out;
}

FS_CHAR *FS_Strndup( FS_CHAR *str, FS_SINT4 len )
{
	FS_CHAR *ret = IFS_Malloc( len + 1);
	if( ret )
	{
		IFS_Memcpy( ret, str, len );
		ret[len] = 0;
	}
	return ret;
}

void FS_ExtractQuote( FS_CHAR *str, FS_CHAR quote_chr )
{
	FS_CHAR *p = str;

	if ((str = IFS_Strchr(str, quote_chr)))
	{
		p = str;
		while ((p = IFS_Strchr(p + 1, quote_chr)) && ( *(p - 1) == '\\'))
		{
			IFS_Memmove(p - 1, p, IFS_Strlen(p) + 1);
			p--;
		}
		if( p )
		{
			IFS_Memmove( str, str + 1, p - str );
			*(p - 1) = '\0';
		}
	}
}

FS_CHAR *FS_StrConCat( FS_CHAR *str1, FS_CHAR *str2, FS_CHAR *str3, FS_CHAR *str4 )
{
	FS_SINT4 len = 0, len1 = 0, len2 = 0, len3 = 0, len4 = 0;
	FS_CHAR *ret;
	if( str1 )
		len1 = IFS_Strlen( str1 );
	if( str2 )
		len2 = IFS_Strlen( str2 );
	if( str3 )
		len3 = IFS_Strlen( str3 );
	if( str4 )
		len4 = IFS_Strlen( str4 );
	len = len1 + len2 + len3 + len4 + 1;
	ret = IFS_Malloc( len );
	if( ret )
	{
		if( str1 )
			IFS_Memcpy( ret, str1, len1 );
		if( str2 )
			IFS_Memcpy( ret + len1, str2, len2 );
		if( str3 )
			IFS_Memcpy( ret + len1 + len2, str3, len3 );
		if( str4 )
			IFS_Memcpy( ret + len1 + len2 + len3, str4, len4 );
		ret[len - 1] = '\0';
	}
	return ret;
}
//-------------------------------------------------------------------------------------------------
// read x line of file, return read bytes, return -1 on error
FS_SINT4 FS_FileReadXLine( FS_CHAR *filename, FS_SINT4 offset, FS_CHAR *out, FS_SINT4 olen )
{
	FS_SINT4 rlen = -1, t;
	rlen = FS_FileRead( -1, filename, offset, out, olen );
	t = rlen;
	while( rlen > 0 && out[rlen - 1] != '\n' ) rlen --;
	if( rlen == 0 )
		rlen = t;
	return rlen;
}

FS_SINT4 FS_FileRead( FS_SINT4 dir, FS_CHAR *filename, FS_SINT4 offset, void *buf, FS_SINT4 blen )
{
	FS_BOOL fret = FS_FALSE;
	FS_Handle hFile;
	FS_SINT4 rlen = 0, size;
	if( dir == -1 )
		fret = IFS_FileOpen( &hFile, filename, FS_OPEN_READ );
	else
		fret = FS_FileOpen( &hFile, dir, filename, FS_OPEN_READ );

	if( fret )
	{
		size = IFS_FileGetSize( hFile );
		if( size > offset )
		{
			IFS_FileSetPos( hFile, offset );
			rlen = (size - offset) > blen ? blen : (size - offset);
			rlen = IFS_FileRead( hFile, buf, rlen );		
		}
		IFS_FileClose( hFile );
	}
	return rlen;
}

FS_SINT4 FS_FileWrite( FS_SINT4 dir, FS_CHAR *filename, FS_SINT4 offset, void *buf, FS_SINT4 blen )
{
	FS_BOOL fret = FS_FALSE;
	FS_Handle hFile;
	FS_SINT4 rlen = -1;
	if( dir == -1 )
	{
		if( offset > 0 )
			fret = IFS_FileOpen( &hFile, filename, FS_OPEN_WRITE );
		else	// create a file
			fret = IFS_FileCreate( &hFile, filename, FS_OPEN_WRITE );
	}
	else
	{
		if( offset > 0 )
			fret = FS_FileOpen( &hFile, dir, filename, FS_OPEN_WRITE );
		else	// create a file
			fret = FS_FileCreate( &hFile, dir, filename, FS_OPEN_WRITE );
	}

	if( fret )
	{
		if( offset > 0 )
			IFS_FileSetPos( hFile, offset );
		IFS_FileWrite( hFile, buf, blen );		
		IFS_FileClose( hFile );
		rlen = blen;
	}
	return rlen;
}

FS_SINT4 FS_FileGetSize( FS_SINT4 dir, FS_CHAR *filename )
{
	FS_BOOL fret;
	FS_Handle hFile;
	FS_SINT4 rlen = 0;
	if( dir == -1 )
		fret = IFS_FileOpen( &hFile, filename, FS_OPEN_READ );
	else
		fret = FS_FileOpen( &hFile, dir, filename, FS_OPEN_READ );
	if( fret )
	{
		rlen = IFS_FileGetSize( hFile );
		IFS_FileClose( hFile );
	}
	return rlen;
}

void FS_DateStr2Struct( FS_DateTime *dt, FS_CHAR *str )
{
	FS_CHAR *week_day[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	FS_CHAR *month[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	FS_CHAR *tp;
	FS_SINT4 i;
	// default value
	IFS_GetDateTime( dt );
	for( i = 0; i < 7; i ++ )
	{
		if( ! IFS_Strnicmp( str, week_day[i], 3 ) )
		{
			dt->week_day = i;
			break;
		}
	}
	tp = IFS_Strchr( str, ',' );
	if( ! tp )
		return;
	tp ++;
	while( *tp == ' ' || *tp == '\t' ) tp ++;
	// day
	dt->day = (FS_UINT1)IFS_Atoi( tp );
	tp ++;
	tp = IFS_Strchr( tp, ' ' );
	if( ! tp )
		return;
	while( *tp == ' ' || *tp == '\t' ) tp ++;
	// month
	for( i = 0; i < 12; i ++ )
	{
		if( ! IFS_Strnicmp( tp, month[i], 3 ) )
		{
			dt->week_day = i;
			break;
		}
	}
	tp = IFS_Strchr( tp, ' ' );
	if( ! tp )
		return;
	while( *tp == ' ' || *tp == '\t' ) tp ++;
	// year
	dt->year = (FS_UINT2)IFS_Atoi( tp );
	if( dt->year < 50 )
		dt->year += 2000;
	else if( dt->year < 1900 )
		dt->year += 1900;
	tp ++;
	tp = IFS_Strchr( tp, ' ' );
	if( ! tp )
		return;
	while( *tp == ' ' || *tp == '\t' ) tp ++;
	// hour
	dt->hour = (FS_UINT1)IFS_Atoi( tp );
	tp = IFS_Strchr( tp, ':' );
	if( ! tp )
		return;
	tp ++;
	while( *tp == ' ' || *tp == '\t' ) tp ++;
	// min
	dt->min = (FS_UINT1)IFS_Atoi( tp );
	tp = IFS_Strchr( tp, ':' );
	if( ! tp )
		return;
	tp ++;
	while( *tp == ' ' || *tp == '\t' ) tp ++;
	// sec
	dt->sec = (FS_UINT1)IFS_Atoi( tp );
}

static void FS_DateStructToStr( FS_CHAR *str, FS_DateTime *dt )
{
	FS_CHAR *week_day[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	FS_CHAR *month[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	FS_CHAR tstr[16];
	
	IFS_Strcpy( str, week_day[dt->week_day % 6] );
	IFS_Strcat( str, ", " );
	IFS_Itoa( dt->day, tstr, 10 );
	if( dt->day < 10 )
		IFS_Strcat( str, "0" );
	IFS_Strcat( str, tstr );
	IFS_Strcat( str, " " );
	IFS_Strcat( str, month[dt->month % 12] );
	IFS_Strcat( str, " " );
	IFS_Itoa( dt->year, tstr, 10 );
	IFS_Strcat( str, tstr );
	IFS_Strcat( str, " " );
	IFS_Itoa( dt->hour, tstr, 10 );
	if( dt->hour < 10 )
		IFS_Strcat( str, "0" );
	IFS_Strcat( str, tstr );
	IFS_Strcat( str, ":" );
	IFS_Itoa( dt->min, tstr, 10 );
	if( dt->min < 10 )
		IFS_Strcat( str, "0" );
	IFS_Strcat( str, tstr );
	IFS_Strcat( str, ":" );
	IFS_Itoa( dt->sec, tstr, 10 );
	if( dt->min < 10 )
		IFS_Strcat( str, "0" );
	IFS_Strcat( str, tstr );
	IFS_Strcat( str, " " );
}

void FS_DateTimeFormatText( FS_CHAR *str, FS_DateTime *dt )
{
	FS_CHAR *week_day[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	FS_CHAR *month[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	FS_CHAR tstr[16];
	
	IFS_Strcpy( str, week_day[dt->week_day % 6] );
	IFS_Strcat( str, ", " );
	IFS_Itoa( dt->day, tstr, 10 );
	if( dt->day < 10 )
		IFS_Strcat( str, "0" );
	IFS_Strcat( str, tstr );
	IFS_Strcat( str, " " );
	IFS_Strcat( str, month[dt->month % 12] );
	IFS_Strcat( str, " " );
	IFS_Itoa( dt->year, tstr, 10 );
	IFS_Strcat( str, tstr );
	IFS_Strcat( str, " " );
	IFS_Itoa( dt->hour, tstr, 10 );
	if( dt->hour < 10 )
		IFS_Strcat( str, "0" );
	IFS_Strcat( str, tstr );
	IFS_Strcat( str, ":" );
	IFS_Itoa( dt->min, tstr, 10 );
	if( dt->min < 10 )
		IFS_Strcat( str, "0" );
	IFS_Strcat( str, tstr );
	IFS_Strcat( str, ":" );
	IFS_Itoa( dt->sec, tstr, 10 );
	if( dt->min < 10 )
		IFS_Strcat( str, "0" );
	IFS_Strcat( str, tstr );
	IFS_Strcat( str, " GMT" );
}

FS_SINT4 FS_DateStruct2DispStr( FS_CHAR *str, FS_DateTime *dt )
{
	IFS_Sprintf( str, "%04d/%02d/%02d %02d:%02d:%02d", dt->year, dt->month, dt->day, dt->hour, dt->min, dt->sec );
	return IFS_Strlen( str );
}

FS_SINT4 FS_DateStruct2DispStrShortForm( FS_CHAR *str, FS_DateTime *dt )
{
	IFS_Sprintf( str, "%02d/%02d %02d:%02d", dt->month, dt->day, dt->hour, dt->min );
	return IFS_Strlen( str );
}

FS_CHAR *FS_GetLine(FS_CHAR *buf, FS_SINT4 len, FS_CHAR **str)
{
	if (!**str)
		return FS_NULL;

	for( ; **str && len > 1; --len)
		if((*buf++ = *(*str)++) == '\n')
			break;
	*buf = '\0';

	return buf;
}

FS_UINT4 FS_GetDayNumByDateFromEpoch( FS_DateTime *dt )
{
	FS_BYTE  year_type;
	FS_UINT4 day_num;
	FS_UINT4 curYear,curmonth;
	FS_UINT2 day_of_month[2][13] =
	{
		{0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
		{0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
	};

	day_num = 0;
	
	curYear = 1970;	/* epoch year 1970.1.1 0:0:0 */
	while(dt->year > curYear)
	{
		year_type = ((curYear % 4 == 0 && curYear % 100 != 0) || curYear % 400 == 0);
		day_num += (365 + year_type);
		curYear++;
	}
   	year_type = ((curYear % 4 == 0 && curYear % 100 != 0) || curYear % 400 == 0);

	curmonth = 1;
	while(curmonth < dt->month )
	{
		day_num += day_of_month[year_type][curmonth];
		curmonth++;
	}

	day_num += dt->day - 1;

	return day_num;    
}

void FS_GetDateByDayNumFromEpoch( FS_DateTime *dt, FS_UINT4 day_num )
{
	FS_DateTime begin_date = {1970, 1, 1, 4, 0, 0, 0};
	FS_SINT4 i;                     // for loop
	FS_UINT4 curYear, curMonth = 1, curDay = 1, curweekday = 1; 
	FS_UINT4 days_from_begin = 0;				// add-up days, the key variable
	FS_BYTE year_type;              // whether be leap year;
	FS_UINT2 day_of_month[2][13] =
	{
		{0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
		{0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
	};

	days_from_begin = 0;
	year_type = ((begin_date.year % 4==0 && begin_date.year % 100 != 0) || begin_date.year % 400==0 );
	
	// get the days of the start year 
	for (i = 1; i < begin_date.month; i++)
	{
		days_from_begin += day_of_month[year_type][i];
	}
	
	days_from_begin += begin_date.day;
	
	// get the lasted days
	days_from_begin += day_num;
	
	// calculate the current year
	curYear = begin_date.year;

	//calculate current year and remain days
	while(1)
	{
		year_type = ((curYear % 4 == 0 && curYear % 100 != 0) || curYear % 400 == 0);

		if(days_from_begin <= (FS_UINT4)(365 + year_type))
			break;

		days_from_begin -= (365 + year_type);
		curYear += 1;
	}
	
	//current year is leep year?
	year_type = ((curYear % 4 == 0 && curYear % 100 != 0) || curYear % 400==0 );
	
	//calculate cur month and cur day info
	for (i = 1; i <= 12; i++)
	{
		if(days_from_begin <= day_of_month[year_type][i])
		{
			curDay   = days_from_begin;
			curMonth = i;
			break;
		}
		days_from_begin  -= day_of_month[year_type][i];
	}
 
	//calculate current week day info
	curweekday = (day_num + begin_date.week_day) % 7; 

	dt->year  = curYear;
	dt->month = curMonth;
	dt->day = curDay;
	dt->week_day = curweekday;
}

FS_UINT4 FS_GetSeconds( FS_SINT4 zone )
{
	FS_UINT4 sec;
	FS_DateTime dt;

	if( zone > 12 || zone < -12 ) zone = 8;	/* default to +8 zone */
	
	IFS_GetDateTime( &dt );
	sec = FS_GetDayNumByDateFromEpoch(&dt) * 24 * 3600 + dt.hour * 3600 + dt.min * 60 + dt.sec;
	return sec - (zone * 3600);
}

void FS_SecondsToDateTime( FS_DateTime *dt, FS_UINT4 secs, FS_SINT4 zone )
{
	FS_UINT4 day_num;

	if( zone > 12 || zone < -12 ) zone = 8;	/* default to +8 zone */
	secs += zone * 3600;
	day_num = secs / (24 * 3600);
	FS_GetDateByDayNumFromEpoch( dt, day_num );
	secs = (secs % ( 24 * 3600));
	dt->hour = secs / 3600;
	secs = secs % 3600;
	dt->min = secs / 60;
	dt->sec = secs % 60;
}

/*-------------------------------------------------------------------------*/
/*-----------------------standard lib interface----------------------------*/
void FS_MemMove( void *dst, void *src, FS_UINT4 size )
{
	FS_BYTE *tmp, *s;
	
	if (dst <= src)
	{
		tmp = (FS_BYTE *) dst;
		s = (FS_BYTE *) src;
		while (size--)
			*tmp++ = *s++;
	}
	else
	{
		tmp = (FS_BYTE *) dst + size;
		s = (FS_BYTE *) src + size;
		while (size--)
			*--tmp = *--s;
	}
}

FS_CHAR * FS_StrNCpy( FS_CHAR *dst, FS_CHAR *src, FS_UINT4 size )
{
	FS_UINT1 i;
	for( i = 0; i < size; i ++ )
	{
		if( (*dst ++ = *src ++) == '\0' )
			break;
	}
	return dst;
}

FS_SINT4 FS_TrimRight( FS_CHAR *str, FS_SINT4 len )
{
	FS_SINT4 rlen = len;
	if( rlen < 0 ) rlen = IFS_Strlen( str );

	while( rlen > 0 && (str[rlen - 1] == ' ' || str[rlen - 1] == '\t') )
	{
		str[rlen - 1] = '\0';
		rlen --;
	}
	return rlen;
}

FS_SINT4 FS_TrimCrlf( FS_CHAR *str, FS_SINT4 len )
{
	FS_SINT4 i, rlen = len;
	if( str == FS_NULL ) return 0;
	if( rlen < 0 ) rlen = IFS_Strlen( str );

	for( i = 0; i < rlen - 1; i ++ )
	{
		if( str[i] == 0x0D && str[i + 1] == 0x0A ){
			IFS_Memmove( str + i, str + i + 2, rlen - i - 1 );
			rlen -= 2;
		}else if( str[i] == 0 ){
			break;
		}
	}
	return rlen;
}

FS_SINT4 FS_TrimBlankSpace( FS_CHAR *str, FS_SINT4 len )
{
	FS_SINT4 i, rlen = len, ireserve;
	if( str == FS_NULL ) return 0;
	if( rlen < 0 ) rlen = IFS_Strlen( str );

	for( i = 0; i < rlen - 1; i ++ )
	{
		if( FS_IsWhiteSpace(str[i]) ){
			len = 1;
			if( str[i] == ' ' )
				ireserve = 1;
			else
				ireserve = 0;
			
			while( FS_IsWhiteSpace(str[i + len]) )
				len ++;
			IFS_Memmove( str + i + ireserve, str + i + len, rlen - i - len + 1 );
			rlen -= len - ireserve;
		}else if( str[i] == 0 ){
			break;
		}
	}
	if ( FS_IsWhiteSpace(str[rlen - 1]) )
	{
		str[rlen - 1] = 0;
		rlen --;
	}
	return rlen;
}

FS_CHAR FS_ToUpper( FS_CHAR c )
{
	FS_CHAR ret = c;
	if( c >= 'a' && c <= 'z' )
		ret = c - 32;
	return ret;
}

FS_CHAR *FS_StrStrI( FS_CHAR *str, FS_CHAR *sub )
{
	FS_SINT4 i = 0, slen, j;
	slen = IFS_Strlen( sub );
	while( str[i] )
	{
		for( j = 0; j < slen; j ++ )
		{
			if( FS_ToUpper( str[i + j] ) != FS_ToUpper( sub[j] ) )
				break;
		}

		if( j == slen )
			break;

		i ++;
	}
	
	if( str[i] == 0 )
		return FS_NULL;
	else
		return str + i;
}

FS_BOOL FS_ContentIdEqual( FS_CHAR *cid, FS_CHAR *str )
{
	FS_BOOL ret = FS_FALSE;
	FS_SINT4 cidLen;
	FS_CHAR *p;
	
	if( cid && str )
	{
		if( cid[0] == '<' )
			cid ++;
		p = IFS_Strchr( cid, '>' );
		if( p )
			cidLen = p - cid;
		else
			cidLen = IFS_Strlen( cid );

		if( str[0] == '<' )
			str ++;
		if( IFS_Strnicmp( str, "cid:", 4 ) == 0 )
			str += 4;

		if( IFS_Strnicmp( cid, str, cidLen ) == 0 )
			ret = FS_TRUE;

		/* some ugly mms codec's cid may contain '"' char */
		if( ret == FS_FALSE && IFS_Strstr( cid, str ) )
			ret = FS_TRUE;
	}
	return ret;
}

static FS_BOOL FS_NeedCoding( FS_CHAR *str, FS_SINT4 slen )
{
	FS_SINT4 i, len;
	FS_CHAR c;
	if( slen < 0 )
		len = IFS_Strlen( str );
	else
		len = slen;
	for( i = 0; i < len; i ++ )
	{
		c = str[i];
		if( c < 32 || c > 128 )
			return FS_TRUE;
	}
	return FS_FALSE;
}

FS_SINT4 FS_Rfc2047EncodeString( FS_CHAR *buf, FS_CHAR *str, FS_CHAR *charset )
{
	FS_SINT4 tlen, rlen = IFS_Strlen( str );
	rlen = FS_TrimRight( str, rlen );
	if( FS_NeedCoding(str, rlen) )
	{
		if( charset == FS_NULL ) charset = "UTF-8";
		
		IFS_Sprintf( buf, "=?%s?B?", charset );
		tlen = IFS_Strlen( buf );
		FS_Base64EncodeLine( buf + tlen, str, rlen );
		IFS_Strcat( buf, "?=" );
		rlen = IFS_Strlen( buf );
	}
	else
	{
		IFS_Memcpy( buf, str, rlen + 1 );
	}
	return rlen;
}

/* convert hex string to hex value */
FS_UINT4 FS_HexValue( FS_CHAR *hexStr )
{
	FS_UINT4 ret = 0;
	FS_BYTE b, i = 0;
	FS_CHAR c;
	
	while( 1 )
	{
		c = hexStr[i ++];
		if( (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') )
		{
			b = FS_Chr2Hex( c );
			ret = (ret << 4) | (b & 0x0F);
		}
		else
		{
			return ret;
		}
	}
}

FS_SINT4 FS_OctetHexDump( FS_CHAR *str, FS_BYTE *data, FS_SINT4 dlen )
{
	FS_SINT4 i, len = 0;
	for( i = 0; i < dlen; i ++ )
	{
		IFS_Sprintf( str + len, "%02x ", data[i] );
		len += 3;
		if( ((i + 1) % 16 ) == 0 ) 
		{
			IFS_Strcpy( str + len, "\r\n" );
			len += 2;
		}
	}
	return len;
}

FS_SINT1 FS_UInt4ToUIntVar( FS_BYTE *uintvar, FS_UINT4 value )
{
	FS_SINT1 len = 0, i, j = 0;
	FS_BYTE split[5];
	
	/* Blow the 32 bit value into (up to) 5 7-bit pieces */
	for( i = 0; i < 5; i ++ )
	{
		split[i] = ( FS_BYTE ) ( value & 0x7F );
		value >>= 7;
		if( value == 0 )
			break;
	}
	
	/*
	   Now we copy the split bytes into the passed buffer, reversing the significance of
	   the byte order. The last byte naturally doesn't get the continuation flag.
	*/
	for ( j = 0; i > 0; j ++, i -- )
	{
		uintvar[j] = ( FS_BYTE )( split[i] | 0x80 );
	}
	uintvar[j] = split[i];
	
	/* We'll run the new uintvar under the nose of uintvar_valid in the debug version. */
	return j + 1;
}

FS_SINT1 FS_UIntVarToUInt4( FS_UINT4 *value, FS_BYTE *uintvar )
{
	FS_SINT4 i, vlen = 0;

	*value = 0;
	while( uintvar[vlen ++] & 0x80 )
		;	/* empty here */
	if( vlen > 5 || uintvar[0] == 0x80 )
		return 0;	/* not a valid uintvar */

	for( i = 0; i < vlen; i ++ )
	{
		*value = (*value << 7) | (uintvar[i] & 0x7F);
	}

	return (FS_SINT1)vlen;
}

/* 目录名称必须合法，不能包含 ? 等非法字符 */
static FS_BOOL FS_InvalidDirName( FS_CHAR *dir )
{
	FS_CHAR *p;
	FS_SINT4 i;
	
	p = IFS_Strchr( dir, '/' );
	
	if( p == FS_NULL ) return FS_TRUE;
	for( i = 0; dir + i < p; i ++ )
	{
		if( dir[i] == '?' || dir[i] == '"' || dir[i] == '|' || dir[i] == '\\'
			|| dir[i] == '<' || dir[i] == '>' || dir[i] == ':' || dir[i] == '*' )
		{
			/* containe invalid dir char */
			return FS_TRUE;
		}
	}
	return FS_FALSE;	/* here, its a valid dir name */
}

/* alloc memory inside. caller take resposibility to free it. */
FS_CHAR *FS_UrlGetDir( FS_CHAR *url )
{
	FS_CHAR *dir, *p;
	FS_SINT4 len;
	
	p = url;
	
	p = IFS_Strstr( p, "//" );
	if( p ) 
		p += 2;
	else
		p = url;
	
	p = IFS_Strchr( p, '/' );
	if( p == FS_NULL )
		dir = IFS_Strdup( url );
	else
	{
		while( p )
		{
			p = p + 1;
			dir = p;
			if( FS_InvalidDirName( p ) )
			{
				break;
			}
			p = IFS_Strchr( p, '/' );
		}
		
		len = dir - url - 1;
		if( len > 0 )
			dir = FS_Strndup( url, len );
	}
	
	return dir;
}

/* alloc memory inside. caller take resposibility to free it. */
FS_CHAR *FS_UrlGetHost( FS_CHAR *url )
{
	FS_CHAR *host, *p;
	FS_SINT4 len;
	
	p = IFS_Strstr( url, "//" );
	if( p )
		p += 2;
	else
		p = url;
	
	p = IFS_Strchr( p, '/' );
	if( p )
	{
		len = p - url;
		if( len > 0 )
			host = FS_Strndup( url, len );
	}
	else
	{
		host = IFS_Strdup( url );
	}

	return host;
}

static FS_CHAR *FS_UrlNeedPadding( FS_CHAR *url )
{
	return FS_NULL;
}

FS_CHAR *FS_ComposeAbsUrl( FS_CHAR *host, FS_CHAR *relateUrl )
{
	FS_CHAR *absUrl;
	FS_SINT4 tlen;
	
	if( host && IFS_Strnicmp(relateUrl, "http://", 7) != 0 && IFS_Strnicmp(relateUrl, "https://", 8) != 0 )
	{
		FS_CHAR *curHost;
		/* host root */
		if( relateUrl[0] == '/' )
		{
			curHost = FS_UrlGetHost( host );
		}
		else
		{
			curHost = FS_UrlGetDir( host );
		}
		
		if( curHost )
		{
			if( relateUrl[0] == '/' )
			{
				absUrl = FS_StrConCat( curHost, relateUrl, FS_UrlNeedPadding(relateUrl), FS_NULL );
			}
			else
			{
				tlen = IFS_Strlen( curHost );

				/* process parent url dir */
				while( IFS_Strncmp( relateUrl, "../", 3 ) == 0 )
				{
					relateUrl += 3;
					/* back one dir */					
					while( tlen > 0 && curHost[tlen - 1] != '/' )
						tlen --;

					if( curHost[tlen - 1] == '/' && curHost[tlen - 2] != '/' ) curHost[tlen - 1] = '\0';
				}
				absUrl = FS_StrConCat( curHost, "/", relateUrl, FS_UrlNeedPadding(relateUrl) );
			}
		}
		else
		{
			absUrl = FS_StrConCat( "http://", relateUrl, FS_UrlNeedPadding(relateUrl), FS_NULL );
		}
		FS_SAFE_FREE( curHost );
	}
	else
	{
		absUrl = FS_StrConCat( relateUrl, FS_UrlNeedPadding(relateUrl), FS_NULL, FS_NULL );
	}

	return absUrl;
}

FS_SINT4 FS_UrlEncode( FS_CHAR *out, FS_SINT4 out_buf_len, FS_CHAR *str, FS_SINT4 len )
{
	FS_BYTE c;
	FS_SINT4 i, j = 0;
	FS_CHAR *estr = out;
	
	if( str == FS_NULL || out_buf_len < 2 )
		return 0;
	
	if( len <= 0 )
		len = IFS_Strlen( str );
	
	for( i = 0; i < len && j < out_buf_len - 1; i ++ )
	{
		c = str[i];
		if( ( c >= 'a' && c <= 'z')
			|| (c >= 'A' && c <= 'Z')
			|| (c >= '0' && c <= '9')
			|| c == '.' || c == '-' || c == '*' || c == '_' )
		{
			estr[j ++] = c;
		}
		else if( c == ' ' )
		{
			estr[j ++] = '+';
		}
		else
		{
			if ( j < out_buf_len - 3 )
			{
				estr[j ++] = '%';
				estr[j ++] = FS_Hex2Chr( (FS_BYTE)(c >> 4) );
				estr[j ++] = FS_Hex2Chr( c );
			}
			else
			{
				/* out buffer is not enough, quit */
				break;
			}
		}
	}
	estr[j] = '\0';
	
	return j;
}

/* convert charset to local -> UTF8 */
FS_CHAR *FS_ProcessCharset( FS_CHAR *str, FS_SINT4 slen, FS_CHAR *charset, FS_SINT4 *outlen )
{
	FS_SINT4 len = slen;
	FS_CHAR *utf8 = FS_NULL;

	/* TODO only support UTF-8 now. other local charset must convert to UTF8 */
	if( charset && str && str[0] && (IFS_Strnicmp(charset, "GB", 2) == 0) )
	{
		if( len < 0 ) len = IFS_Strlen( str );
		utf8 = IFS_Malloc( (len * 3 / 2) + 1 );
		
		if( utf8 )
			FS_CnvtGBKToUtf8( str, len, utf8, outlen );
	}
	return utf8;
}

/* process <p> </p> */
FS_CHAR * FS_ProcessParagraph( FS_CHAR *text, FS_SINT4 len )
{
	FS_CHAR *p, *p2;
	if( text == FS_NULL ) return FS_NULL;
	if( len <= 0 )
		len = IFS_Strlen( text );

	/* transfer to low case <p> */
	p = IFS_Strstr( text, "<P" );
	while ( p && p - text < len ) {
		IFS_Memcpy( p, "<p", 2 );
		p += 2;
		p = IFS_Strstr( text, "</P>" );
		if ( p ) {
			IFS_Memcpy( p, "</p>", 4 );
			p += 4;
		}
		p = IFS_Strstr( p, "<P" );
	}

	p = IFS_Strstr( text, "<p" );
	while ( p && p - text < len ) {
		p2 = IFS_Strchr( p + 2, '>' );
		if ( p2 && p2 - p < 128 ) {
			/* we consider this as a valid paragraph begin tag */
			IFS_Memcpy( p, "\r\n", 2 );
			if ( p2 - p > 2 ) {
				IFS_Memmove( p + 2, p2 + 1, IFS_Strlen(p2) );
			} else {
				p[2] = 0x20;
			}

			p = IFS_Strstr( text, "</p>" );
			if ( p ) {
				IFS_Memcpy( p, "\r\n  ", 4 );
			}
		}
		p = IFS_Strstr( p, "<p" );
	}
	return text;
}

void FS_ProcessEsc( FS_CHAR *text, FS_SINT4 len )
{
	FS_CHAR *p = text;
	FS_SINT4 i, tlen = len;
	FS_UINT2 nCode;
	
	if( text == FS_NULL ) return;
	if( tlen < 0 )
		tlen = IFS_Strlen( text );
	
	while( (p = IFS_Strchr(p, '&')) != FS_NULL )
	{
		/* space */
		if( IFS_Strnicmp(p, "&nbsp;", 6) == 0 )
		{
			*p = ' ';
			IFS_Memmove( p + 1, p + 6, (tlen - (p + 6 - text) + 1));
		}
		else if( IFS_Strnicmp(p, "&lt;", 4) == 0 )
		{
			*p = '<';
			IFS_Memmove( p + 1, p + 4, (tlen - (p + 4 - text) + 1));
		}
		else if( IFS_Strnicmp(p, "&gt;", 4) == 0 )
		{
			*p = '>';
			IFS_Memmove( p + 1, p + 4, (tlen - (p + 4 - text) + 1));
		}
		else if( IFS_Strnicmp(p, "&apos;", 6) == 0 )
		{
			*p = 0x27;	/* ' char */
			IFS_Memmove( p + 1, p + 6, (tlen - (p + 6 - text) + 1));
		}
		else if( IFS_Strnicmp(p, "&amp;", 5) == 0 )
		{
			*p = '&';
			IFS_Memmove( p + 1, p + 5, (tlen - (p + 5 - text) + 1));
			p --;	/* rollback to run another round of ESC */
		}
		else if( IFS_Strnicmp(p, "&quot;", 6) == 0 )
		{
			*p = '"';
			IFS_Memmove( p + 1, p + 6, (tlen - (p + 6 - text) + 1));
		}
		else if( IFS_Strnicmp(p, "&#", 2) == 0 )
		{
			FS_CHAR *pEnd = IFS_Strchr( p, ';' );
			if( ! pEnd ) return;
			
			if( FS_CHAR_EQUAL(p[2], 'x') )	
			{
				/* hex char entry */
				nCode = ( FS_UINT2 )FS_HexValue( p + 3 );
			}
			else
			{
				/* decimal char entry */
				nCode = IFS_Atoi( p + 2 );
			}

			i = FS_CnvtUcs2ToUtf8Char( nCode, p );
			p += i - 1;
			IFS_Memmove( p + 1, pEnd + 1, (tlen - (pEnd - text)));
		}
		else
		{
			len = 0;
			while( *(p + len) && *(p + len) != ';' && len < 10 )
				len ++;

			if( *(p + len) == ';' )
				len ++;
			
			if( len > 0 && len < 10 && *(p + len - 1) == ';' )
				IFS_Memmove( p, p + len, (tlen - (p + len - text) + 1));
		}
		p ++;
	}

	FS_TrimBlankSpace( text, -1 );
}

/* Decodes headers based on RFC2045 and RFC2047. */
void FS_Rfc2047DecodeString( FS_CHAR *out, FS_CHAR *str )
{
	FS_CHAR *p = str;
	FS_CHAR *outp = out;
	FS_CHAR *eword_begin_p, *encoding_begin_p, *text_begin_p,
			*eword_end_p;
	FS_CHAR charset[32];
	FS_CHAR encoding;
	FS_CHAR *conv_str;
	FS_SINT4 len;

	while( *p != '\0' )
	{
		FS_CHAR *decoded_text = FS_NULL;

		eword_begin_p = IFS_Strstr( p, "=?" );
		if( ! eword_begin_p )
		{
			IFS_Strcpy(outp, p);
			return;
		}
		encoding_begin_p = IFS_Strchr(eword_begin_p + 2, '?');
		if( ! encoding_begin_p )
		{
			IFS_Strcpy( outp, p );
			return;
		}
		text_begin_p = IFS_Strchr(encoding_begin_p + 1, '?');
		if( ! text_begin_p )
		{
			IFS_Strcpy( outp, p );
			return;
		}
		eword_end_p = IFS_Strstr(text_begin_p + 1, "?=");
		if( ! eword_end_p )
		{
			IFS_Strcpy( outp, p );
			return;
		}

		if (p == str)
		{
			IFS_Memcpy( outp, p, eword_begin_p - p );
			outp += eword_begin_p - p;
			p = eword_begin_p;
		}
		else
		{
			/* ignore spaces between encoded words */
			FS_CHAR *sp;

			for ( sp = p; sp < eword_begin_p; sp++ )
			{
				if ( *sp != ' ' || *sp != '\t' )
				{
					IFS_Memcpy( outp, sp, eword_begin_p - sp );
					outp += eword_begin_p - sp;
					p = eword_begin_p;
					break;
				}
			}
		}

		IFS_Memset( charset, 0, sizeof(charset) );
		IFS_Strncpy( charset, eword_begin_p + 2, sizeof(charset) - 1 );
		encoding = IFS_Toupper(*(encoding_begin_p + 1));
		
		if (encoding == 'B') {
			decoded_text = IFS_Malloc( eword_end_p - text_begin_p );
			len = FS_Base64Decode( decoded_text, text_begin_p + 1, eword_end_p - (text_begin_p + 1));
			decoded_text[len] = '\0';
		} else if (encoding == 'Q') {
			decoded_text = IFS_Malloc( eword_end_p - text_begin_p );
			len = FS_QPDecode( decoded_text, text_begin_p + 1, eword_end_p - (text_begin_p + 1));
		} else {
			IFS_Memcpy( outp, p, eword_end_p + 2 - p );
			outp += eword_end_p + 2 - p;
			p = eword_end_p + 2;
			continue;
		}

		/* convert to local charset */
		conv_str = FS_ProcessCharset( decoded_text, -1, charset, FS_NULL );
		if( conv_str )
		{
			len = IFS_Strlen( conv_str );
			IFS_Memcpy(outp, conv_str, len);
			IFS_Free( conv_str );
		}
		else
		{
			len = IFS_Strlen( decoded_text );
			IFS_Memcpy( outp, decoded_text, len );
		}
		outp += len;

		IFS_Free( decoded_text );

		p = eword_end_p + 2;
	}

	*outp = '\0';
}

/*--------------------------------system utility-------------------------------*/

static FS_BOOL GFS_SysInited;

#ifdef FS_MODULE_MM
FS_BOOL FS_MemInit( void );
void FS_MemDeinit( );
#endif

void FS_SystemDeinit( void )
{
	if( ! GFS_SysInited ) return;
	
	FS_GuiDeinit( );
	FS_ResDeinit( );
	FS_DeinitFileSys( );
	FS_DebugDumpMem( );
#ifdef FS_MODULE_MM
	FS_MemDeinit( );
#endif
	GFS_SysInited = FS_FALSE;
}

/* system init. must first call this function */
FS_BOOL FS_SystemInit( void )
{
	if( GFS_SysInited ) return FS_TRUE;
	
	GFS_SysInited = FS_TRUE;
#ifdef FS_MODULE_MM
	FS_MemInit( );
#endif
	FS_InitFileSys( );
#ifdef FS_DEBUG_
	FS_FileDelete( FS_DIR_ROOT, "trace.txt" );
	FS_FileDelete( FS_DIR_ROOT, "assert.txt" );
#endif
	FS_GuiInit( );
#ifdef FS_MODULE_WEB
	FS_WebConfigInit( );
#endif
#ifdef FS_MODULE_MMS
	FS_MmsConfigInit( );
#endif
	return FS_TRUE;
}

typedef void (* FS_SysMsgHandler)( void );

extern void FS_WebHandleEvent( FS_UINT1 event, FS_UINT4 param );

void FS_HandlePostMessage( FS_UINT2 msg, FS_UINT4 param )
{
	FS_SysMsgHandler handle;
	FS_UINT2 msgType = msg & 0xFF00;
	FS_UINT1 evType = msg & 0xFF;
	
	switch( msgType )
	{
		case FS_MSG_UTIL_CALL:
			handle = (FS_SysMsgHandler)param;	
			handle();
			break;
#ifdef FS_MODULE_WEB
		case FS_MSG_WEB_EVENT:
			FS_WebHandleEvent( evType, param );
			break;
#endif
		default:
			break;
	}
}

static FS_UINT4 GFS_ActiveApps;

void FS_ActiveApplication( FS_UINT4 app )
{
	GFS_ActiveApps |= app;
}

void FS_DeactiveApplication( FS_UINT4 app )
{
	GFS_ActiveApps &= (~app);
}

FS_BOOL FS_HaveActiveApplication( void )
{
	return (FS_BOOL)(GFS_ActiveApps != 0);
}

FS_BOOL FS_ApplicationIsActive( FS_UINT4 app )
{
	return (FS_BOOL)(GFS_ActiveApps & app);
}
