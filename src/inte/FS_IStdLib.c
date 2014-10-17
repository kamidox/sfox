#include "inc/inte/FS_Inte.h"
#ifdef FS_PLT_WIN
#include <windows.h>
#include "resource.h"
#include <time.h>
#include <stdio.h>
#endif

#ifdef FS_PLT_VIENNA
#include "..\system\portab.h"
#include "..\system\sysprim.h"
#include "..\global\gsmerror.h"
#include "..\global\types.h"
#include "..\system\syslib.h"
#include "..\uh\amoiuh.h"
#include "..\global\Gsmprim.h"
#include "..\ui\uif5\win\uifwindo.h"
#endif

#ifdef FS_MODULE_MM
void * FS_Malloc( FS_UINT4 size );
void FS_Free( void *ptr );
#endif

void * IFS_Malloc( FS_UINT4 nSize )
{
#ifdef FS_MODULE_MM
	return FS_Malloc( nSize );
#endif

#ifdef FS_PLT_WIN
	return malloc( nSize );
#endif
#ifdef FS_PLT_VIENNA
	return GSMMallocNULL( nSize );
#endif
}

/* !!! realloc must keep the content in the buffer */
void * IFS_Realloc( void *ptr, FS_UINT4 size )
{
#ifdef FS_MODULE_MM
	void *p = FS_Malloc( size );
	if( p )
	{
		if( ptr ) IFS_Memcpy( p, ptr, size );
	}
	if( ptr ) FS_Free( ptr );
	return p;
#endif

#ifdef FS_PLT_WIN
	return realloc( ptr, size );
#endif
#ifdef FS_PLT_VIENNA
	void *p = GSMMallocNULL( size );
	if( p )
	{
		if( ptr ) GSMmemcpy( p, ptr, size );
	}
	if( ptr ) GSMFree( ptr );
	return p;
#endif
}

void IFS_Free( void *p )
{
#ifdef FS_MODULE_MM
	FS_Free( p );
	return;
#endif

#ifdef FS_PLT_WIN
	free( p );
#endif
#ifdef FS_PLT_VIENNA
	GSMFree( p );
#endif
}

void * IFS_Memcpy( void *dst, void *src, FS_UINT4 size )
{
#ifdef FS_PLT_WIN
	return memcpy( dst, src, size );
#endif
#ifdef FS_PLT_VIENNA
	GSMmemcpy( dst, src, size );
	return dst;
#endif
}

void * IFS_Memset( void *dst, int val, FS_UINT4 size )
{
#ifdef FS_PLT_WIN
	return memset( dst, val, size );
#endif
#ifdef FS_PLT_VIENNA
	GSMmemset( dst, val, size );
	return dst;
#endif
}

void IFS_Memmove( void *dst, void *src, FS_UINT4 size )
{
#ifdef FS_PLT_WIN
	memmove( dst, src, size );
#endif
#ifdef FS_PLT_VIENNA
	FS_MemMove( dst, src, size );
#endif
}

FS_SINT4 IFS_Memcmp( const void *m1, const void *m2, FS_UINT4 size )
{
#ifdef FS_PLT_WIN
	return memcmp( m1, m2, size );
#endif
#ifdef FS_PLT_VIENNA
	return GSMmemcmp( m1, m2, size );
#endif
}

FS_SINT4 IFS_Strlen( const FS_CHAR *pStr )
{
#ifdef FS_PLT_WIN
	return strlen( pStr );
#endif
#ifdef FS_PLT_VIENNA
	return GSMstrlen( pStr );
#endif
}

FS_CHAR * IFS_Strcpy( FS_CHAR *dst, const FS_CHAR *src )
{
#ifdef FS_PLT_WIN
	return strcpy( dst, src );
#endif
#ifdef FS_PLT_VIENNA
	return GSMstrcpy( dst, src );
#endif
}

FS_CHAR * IFS_Strncpy( FS_CHAR *dst, const FS_CHAR *src, FS_SINT4 len )
{
#ifdef FS_PLT_WIN
	return strncpy( dst, src, len );
#endif
#ifdef FS_PLT_VIENNA
	return GSMstrncpy( dst, src, len );
#endif
}

FS_CHAR IFS_Toupper( FS_CHAR c )
{
#ifdef FS_PLT_WIN
	return toupper( c );
#endif
#ifdef FS_PLT_VIENNA
	return GSMtoupper( c );
#endif
}

FS_SINT4 IFS_Strcmp( FS_CHAR *str1, FS_CHAR *str2 )
{
#ifdef FS_PLT_WIN
	return strcmp( str1, str2 );
#endif
#ifdef FS_PLT_VIENNA
	return GSMstrcmp( str1, str2 );
#endif
}

FS_SINT4 IFS_Stricmp( FS_CHAR *str1, FS_CHAR *str2 )
{
#ifdef FS_PLT_WIN
	return stricmp( str1, str2 );
#endif
#ifdef FS_PLT_VIENNA
	return GSMstricmp( str1, str2 );
#endif
}

FS_SINT4 IFS_Strnicmp( FS_CHAR *str1, FS_CHAR *str2, FS_SINT4 len )
{
#ifdef FS_PLT_WIN
	return strnicmp( str1, str2, len );
#endif
#ifdef FS_PLT_VIENNA
	return GSMstrnicmp( str1, str2, len );
#endif
}

FS_SINT4 IFS_Strncmp( FS_CHAR *str1, FS_CHAR *str2, FS_SINT4 len )
{
#ifdef FS_PLT_WIN
	return strncmp( str1, str2, len );
#endif
#ifdef FS_PLT_VIENNA
	return GSMstrncmp( str1, str2, len );
#endif
}

FS_CHAR * IFS_Strchr( FS_CHAR *str, FS_CHAR c )
{
#ifdef FS_PLT_WIN
	return strchr( str, c );
#endif
#ifdef FS_PLT_VIENNA
	return GSMstrchr( str, c );
#endif
}

FS_CHAR * IFS_Strstr( FS_CHAR *str, FS_CHAR *substr )
{
#ifdef FS_PLT_WIN
	return strstr( str, substr );
#endif
#ifdef FS_PLT_VIENNA
	extern char *strstr(const char *, const char * );
	return strstr( str, substr );
#endif
}


FS_CHAR * IFS_Strdup( FS_CHAR *pStr )
{
#ifdef FS_MODULE_MM
	FS_SINT4 len = IFS_Strlen( pStr );
	FS_CHAR *ret = IFS_Malloc( len + 1 );
	IFS_Memcpy( ret, pStr, len + 1 );
	return ret;
#endif

#ifdef FS_PLT_WIN
	return strdup( pStr );
#endif
#ifdef FS_PLT_VIENNA
	return GSMstrdup( pStr );
#endif
}

FS_SINT4 IFS_Atoi( FS_CHAR *pStr )
{
#ifdef FS_PLT_WIN
	return atoi( pStr );
#endif
#ifdef FS_PLT_VIENNA
	return GSMatoi( pStr );
#endif
}

FS_CHAR * IFS_Itoa( FS_SINT4 val, FS_CHAR *str, FS_SINT4 radis )
{
#ifdef FS_PLT_WIN
	return itoa( val, str, radis );
#endif
#ifdef FS_PLT_VIENNA
	return GSMitoa( val, str, radis );
#endif
}

FS_CHAR * IFS_Strcat( FS_CHAR *dst, const FS_CHAR *src)
{
#ifdef FS_PLT_WIN
	return strcat( dst, src );
#endif
#ifdef FS_PLT_VIENNA
	return GSMstrcat( dst, src );
#endif
}

FS_SINT4 IFS_Rand( void )
{
#ifdef FS_PLT_WIN
	return rand( );
#endif
#ifdef FS_PLT_VIENNA
	return GSMrand( 0xFFFF );
#endif
}

void IFS_SRand( FS_UINT4 seed )
{
#ifdef FS_PLT_WIN
	srand( seed );
#endif
#ifdef FS_PLT_VIENNA
	GSMsrand( seed );
#endif
}

FS_SINT4 IFS_Sprintf( FS_CHAR *buf, FS_CHAR *fmt, ... )
{
#ifdef FS_PLT_WIN
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsprintf(buf,fmt,args);
	va_end(args);
	return i;
#endif
#ifdef FS_PLT_VIENNA
	va_list args;
	int i;

	va_start(args, fmt);
	i = GSMvsprintf( buf, fmt, args );
	va_end(args);
	return i;
#endif
}

/*
	!!! CAUTION
	@buf is @count len. Be sure not to exceed.
*/
FS_SINT4 IFS_Snprintf( FS_CHAR *buf, FS_UINT4 count, const FS_CHAR *fmt, ... )
{
#ifdef FS_PLT_WIN
	va_list args;
	int i;

	va_start(args, fmt);
	i = _vsnprintf( buf, count, fmt, args );
	va_end(args);
	return i;
#endif
#ifdef FS_PLT_VIENNA
	//TODO
	static char s_buf[2048];
	va_list args;
	int i;

	va_start(args, fmt);
	i = GSMvsprintf( s_buf, fmt, args );
	va_end(args);

	GSMmemset( buf, 0, count );
	i = i > (count - 1) ? (count - 1) : i;
	GSMmemcpy( buf, s_buf, i );
	
	return i;
#endif
}

void IFS_GetDateTime( FS_DateTime *dt )
{
#ifdef FS_PLT_WIN
	time_t t;
	struct tm *date;
	time( &t );
	date = localtime( &t );
	dt->year = date->tm_year;
	dt->month = date->tm_mon + 1;	/* 1->12 */
	dt->day = date->tm_mday;
	dt->week_day = date->tm_wday;
	dt->hour = date->tm_hour;
	dt->min = date->tm_min;
	dt->sec = date->tm_sec;
	if( dt->year < 50 )
		dt->year += 2000;
	else if( dt->year < 1900 )
		dt->year += 1900;
#endif
#ifdef FS_PLT_VIENNA
	UHDateTime t;
	UHGetTime( &t );
	dt->year = t.year;
	dt->month = t.month;
	dt->day = t.day;
	dt->week_day = t.dayofweek;
	dt->hour = t.hour;
	dt->min = t.min;
	dt->sec = t.sec;
#endif
}

FS_SINT4 IFS_GetTimeZone( void )
{
	return 8;	/* +8 zone */
}

