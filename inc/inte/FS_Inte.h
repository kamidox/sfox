#ifndef _FS_IFS_H_
#define _FS_IFS_H_
#include "inc/FS_Config.h"
#include "inc/inte/FS_IFile.h"
#include "inc/inte/FS_ISystem.h"

/* decalre from FS_Inte.c for gernal interface */
void *		IFS_Malloc( FS_UINT4 nSize );
void * 		IFS_Realloc( void *ptr, FS_UINT4 size );	/* only for gif lib */
void		IFS_Free( void *p );
void *		IFS_Memcpy( void *dst, void *src, FS_UINT4 size );
void *		IFS_Memset( void *dst, int val, FS_UINT4 size );
void		IFS_Memmove( void *dst, void *src, FS_UINT4 size );
FS_SINT4	IFS_Memcmp( const void *m1, const void *m2, FS_UINT4 size );
FS_SINT4	IFS_Strlen( const FS_CHAR *pStr );
FS_CHAR *	IFS_Strdup( FS_CHAR *pStr );
FS_CHAR * 	IFS_Strcpy( FS_CHAR *dst, const FS_CHAR *src );
FS_CHAR *	IFS_Strncpy( FS_CHAR *dst, const FS_CHAR *src, FS_SINT4 len );
FS_CHAR		IFS_Toupper( FS_CHAR c );
FS_SINT4 	IFS_Strcmp( FS_CHAR *str1, FS_CHAR *str2 );
FS_SINT4	IFS_Stricmp( FS_CHAR *str1, FS_CHAR *str2 );
FS_SINT4	IFS_Strnicmp( FS_CHAR *str1, FS_CHAR *str2, FS_SINT4 len );
FS_SINT4	IFS_Strncmp( FS_CHAR *str1, FS_CHAR *str2, FS_SINT4 len );
FS_CHAR *	IFS_Strchr( FS_CHAR *str, FS_CHAR c );
FS_CHAR *	IFS_Strstr( FS_CHAR *str, FS_CHAR *substr );
FS_SINT4 	IFS_Atoi( FS_CHAR *pStr );
FS_CHAR * 	IFS_Strcat( FS_CHAR *dst, const FS_CHAR *src);
FS_CHAR *	IFS_Itoa( FS_SINT4 val, FS_CHAR *str, FS_SINT4 radis );
FS_SINT4	IFS_Sprintf( FS_CHAR *buf, FS_CHAR *fmt, ... );
FS_SINT4	IFS_Snprintf( FS_CHAR *buf, FS_UINT4 count, const FS_CHAR *fmt, ... );

FS_SINT4	IFS_Rand( void );
void		IFS_SRand( FS_UINT4 seed );

/*--------------------------------------------------------------------------------------
		date time interface
----------------------------------------------------------------------------------------*/
void IFS_GetDateTime( FS_DateTime *dt );

FS_SINT4 IFS_GetTimeZone( void );

#endif
