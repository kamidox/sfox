#ifndef _FS_EX_INTE_H_
#define _FS_EX_INTE_H_

#include "inc/FS_Config.h"
#include "inc/inte/FS_Inte.h"
#include "inc/util/FS_MemDebug.h"

#ifndef NULL
#define NULL					FS_NULL
#endif

#define time_t					FS_UINT4
#define size_t					FS_UINT4
#define datetime_t				FS_DateTime

#define malloc( l )				IFS_Malloc( l )
#define free( p )				IFS_Free( p )
#define realloc( p, l )			IFS_Realloc( p, l )

#define time( a )				FS_GetSeconds( 0 )
#define localtime( t )			IFS_GetDateTime( t )

#define memset( p, v, l )		IFS_Memset( p, v, l )
#define memcpy( d, s, l )		IFS_Memcpy( d, s, l )
#define memcmp( p1, p2, l )		IFS_Memcmp( p1, p2, l )

#define strlen( s )				IFS_Strlen( s )
#define strcat( d, s )			IFS_Strcat( d, s )
#define strcpy( d, s )			IFS_Strcpy( d, s )
#define strncpy( d, s, l )		IFS_Strncpy( d, s, l )
#define strcmp( s1, s2 )		IFS_Strcmp( s1, s2 )
#define strncmp( s1, s2, l )	IFS_Strncmp( s1, s2, l )
#define strstr( str, sub )		IFS_Strstr( str, sub )
#define atoi( s )				IFS_Atoi( s )
#define snprintf				IFS_Snprintf

#define srand( s )				IFS_SRand( s )
#define rand( )					IFS_Rand( )

#endif //_FS_EX_INTE_H_
