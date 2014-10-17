#ifndef _FS_MEM_DEBUG_H_
#define _FS_MEM_DEBUG_H_

#include "inc\FS_Config.h"

#ifdef FS_DEBUG_
void * FS_DebugMalloc( FS_SINT4 size, FS_CHAR *file, FS_SINT4 line );
void *FS_DebugRealloc( void *ptr,  FS_SINT4 size, FS_CHAR *file, FS_SINT4 line );
void FS_DebugFree( void *ptr, FS_CHAR *file, FS_SINT4 line );
void FS_DebugDumpMem( void );
void * FS_DebugStrdup( FS_CHAR *str, FS_CHAR *file, FS_SINT4 line );
void FS_DebugAssertFailed( FS_CHAR *file, FS_SINT4 line, FS_CHAR *cond );
void FS_DebugPrintLog( FS_CHAR *fmt, ... );

#define IFS_Malloc( size )			FS_DebugMalloc( size, __FILE__, __LINE__ )
#define IFS_Realloc( ptr, size )	FS_DebugRealloc( ptr, size, __FILE__, __LINE__ )
#define IFS_Strdup( str )			FS_DebugStrdup( str, __FILE__, __LINE__ )
#define IFS_Free( ptr ) 			FS_DebugFree( ptr, __FILE__, __LINE__ )

#define FS_ASSERT( cond )			\
	do {							\
		if( !(cond) )				\
		{							\
			FS_DebugAssertFailed( __FILE__, __LINE__, #cond );			\
		}							\
	} while ( 0 )

#ifdef FS_TRACE_
#define FS_TRACE0( a )						FS_DebugPrintLog( a )
#define FS_TRACE1( a, b )					FS_DebugPrintLog( a, b )
#define FS_TRACE2( a, b, c )				FS_DebugPrintLog( a, b, c )
#define FS_TRACE3( a, b, c, d )				FS_DebugPrintLog( a, b, c, d )
#else //FS_TRACE_
#define FS_TRACE0( a )
#define FS_TRACE1( a, b )
#define FS_TRACE2( a, b, c )
#define FS_TRACE3( a, b, c, d )
#endif //FS_TRACE_

#else //FS_DEBUG_
#define FS_DebugDumpMem( )
#define FS_ASSERT( cond )

#define FS_TRACE0( a )
#define FS_TRACE1( a, b )
#define FS_TRACE2( a, b, c )
#define FS_TRACE3( a, b, c, d )
#endif //FS_DEBUG_

#endif
