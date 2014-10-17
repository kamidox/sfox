#include "inc\FS_Config.h"

#ifdef FS_DEBUG_
#include "inc\util\FS_List.h"
#include "inc\inte\FS_Inte.h"
#include "inc\util\FS_File.h"
#include "inc\util\FS_Util.h"

#include <stdarg.h>
#include <stdio.h>

#define FS_ASSERT_OUT_FILE		"assert.txt"
#define FS_TRACE_OUT_FILE		"trace.txt"

#define FS_MEM_GUARD_LEN	16
#define FS_MEM_GUARD_CHR	0x58

FS_List GFS_MemEntryList = { &GFS_MemEntryList, &GFS_MemEntryList };

typedef enum FS_MemType_Tag
{
	FS_MEM_NORMAL = 0,
	FS_MEM_CORRUPT,
	FS_MEM_OVERFLOW
}FS_MemType;

typedef struct FS_MemEntry_Tag
{
	FS_List			list;
	void *			ptr;
	FS_SINT4		len;
	FS_CHAR *		file;
	FS_SINT4		line;
	FS_MemType		type;
}FS_MemEntry;

typedef struct FS_MemInfo_Tag
{
	FS_UINT4		malloc_times;		// total malloc times	
	FS_UINT4		total_size;			// current total malloc size
	FS_UINT4		climax_size;
}FS_MemInfo;

static FS_MemInfo GFS_MemInfo = { 0, 0, 0 };

static void FS_LogMemInfo( FS_BOOL malloc, FS_SINT4 len )
{
	if( malloc )
	{
		GFS_MemInfo.malloc_times ++;
		GFS_MemInfo.total_size += len;
		if( GFS_MemInfo.total_size > GFS_MemInfo.climax_size )
			GFS_MemInfo.climax_size = GFS_MemInfo.total_size;
	}
	else
	{
		GFS_MemInfo.total_size -= len;
	}
}

static void FS_MemCheckGuard( FS_MemEntry *entry )
{
	FS_SINT4 i;
	FS_BYTE *guard = (FS_BYTE *)entry->ptr + entry->len;
	for( i = 0; i < FS_MEM_GUARD_LEN; i ++ )
	{
		if( guard[i] != FS_MEM_GUARD_CHR )
		{
			FS_MemEntry *ne = IFS_Malloc( sizeof(FS_MemEntry) );
			if( ne )
			{
				IFS_Memset( ne, 0, sizeof(FS_MemEntry) );
				ne->file = IFS_Strdup( entry->file );
				ne->line = entry->line;
				ne->ptr = entry->ptr;
				ne->type = FS_MEM_OVERFLOW;
				FS_ListAdd( &GFS_MemEntryList, &ne->list );
			}
			break;
		}
	}
}

//-------------------------------------------------------------------------------
void * FS_DebugMalloc( FS_SINT4 size, FS_CHAR *file, FS_SINT4 line )
{
	void * ret = FS_NULL;
	FS_MemEntry *entry = IFS_Malloc( sizeof(FS_MemEntry) );
	if( entry )
	{
		IFS_Memset( entry, 0, sizeof(FS_MemEntry) );
		if( file )
			entry->file = IFS_Strdup( file );
		entry->line = line;
		entry->len = size;
		FS_ListAdd( &GFS_MemEntryList, &entry->list );

		entry->ptr = IFS_Malloc( size + FS_MEM_GUARD_LEN );
		if( entry->ptr )
			IFS_Memset( (FS_BYTE *)entry->ptr + size, FS_MEM_GUARD_CHR, FS_MEM_GUARD_LEN );
		ret = entry->ptr;
	}
	FS_LogMemInfo( FS_TRUE, size );
	return ret;
}

//-------------------------------------------------------------------------------
void * FS_DebugStrdup( FS_CHAR *str, FS_CHAR *file, FS_SINT4 line )
{
	FS_CHAR * ret = FS_DebugMalloc( IFS_Strlen(str) + 1, file, line );
	if( ret )
		IFS_Strcpy( ret, str );
	return ret;
}

//-------------------------------------------------------------------------------
void FS_DebugFree( void *ptr, FS_CHAR *file, FS_SINT4 line )
{
	FS_MemEntry *entry;
	FS_List *node = GFS_MemEntryList.next;
	FS_SINT4 size;
	while( node != &GFS_MemEntryList )
	{
		entry = FS_ListEntry( node, FS_MemEntry, list );
		node = node->next;
		if( entry->ptr == ptr && entry->type != FS_MEM_CORRUPT )
		{
			size = entry->len;
			FS_ListDel( &entry->list );
			FS_MemCheckGuard( entry );
			if( entry->file )
				IFS_Free( entry->file );
			if( entry->ptr )
				IFS_Free( entry->ptr );
			IFS_Free( entry );
			FS_LogMemInfo( FS_FALSE, size );
			
			return;
		}
	}
	/* free corrupt */
	entry = IFS_Malloc( sizeof(FS_MemEntry) );
	if( entry )
	{
		IFS_Memset( entry, 0, sizeof(FS_MemEntry) );
		entry->file = IFS_Strdup( file );
		entry->line = line;
		entry->ptr = ptr;
		entry->type = FS_MEM_CORRUPT;
		FS_ListAdd( &GFS_MemEntryList, &entry->list );
	}
}

void *FS_DebugRealloc( void *ptr,  FS_SINT4 size, FS_CHAR *file, FS_SINT4 line )
{
	void *p = FS_DebugMalloc( size, file, line );
	if( p && ptr )
	{
		IFS_Memcpy( p, ptr, size );
	}
	if( ptr ) FS_DebugFree( ptr, file, line );
	return p;
}

//-------------------------------------------------------------------------------
void FS_DebugDumpMem( void )
{
	static FS_CHAR txt[2048];
	FS_CHAR filename[64];
	FS_MemEntry *entry;
	FS_List *node = GFS_MemEntryList.next;
	FS_Handle hFile;
	
	IFS_Sprintf( filename, "%s%smem_dump.txt", IFS_GetRootDir(), IFS_GetPathSep() );
	if( IFS_FileCreate( &hFile, filename, FS_OPEN_WRITE ) )
	{
		IFS_Sprintf( txt, "total malloc time: %u, climax size: %u Bytes\r\n", GFS_MemInfo.malloc_times, GFS_MemInfo.climax_size );
		if( GFS_MemInfo.total_size > 0 )
			IFS_Strcat( txt, "memery status bad!!!\r\n" );
		else
			IFS_Strcat( txt, "memery status ok\r\n" );
		IFS_FileWrite( hFile, txt, IFS_Strlen(txt) );
		while( node != &GFS_MemEntryList )
		{
			entry = FS_ListEntry( node, FS_MemEntry, list );
			node = node->next;
			if( entry->type == FS_MEM_NORMAL )
				IFS_Sprintf( txt, "LEAK file = %s, line = %d, len = %d, addr = 0x%X\r\n", entry->file, entry->line, entry->len, entry->ptr );
			else if( entry->type == FS_MEM_OVERFLOW )
				IFS_Sprintf( txt, "OVERFLOW file = %s, line = %d, len = %d, addr = 0x%X\r\n", entry->file, entry->line, entry->len, entry->ptr );
			else if( entry->type == FS_MEM_CORRUPT )
				IFS_Sprintf( txt, "CORRUPT file = %s, line = %d, len = %d, addr = 0x%X\r\n", entry->file, entry->line, entry->len, entry->ptr );
			IFS_FileWrite( hFile, txt, IFS_Strlen(txt) );
			IFS_Free( entry->file );
			IFS_Free( entry );
		}
		IFS_FileClose( hFile );
	}
}

void FS_DebugAssertFailed( FS_CHAR *file, FS_SINT4 line, FS_CHAR *cond )
{
	static FS_CHAR s_buf[2048];
	FS_CHAR filename[64];
	FS_Handle hFile;
	FS_SINT4 offset, len;
	FS_BOOL ret = FS_TRUE;
	
	IFS_Sprintf( s_buf, "Assert Failed!!! (%s), file = %s, line = %d\r\n", cond, file, line );
	len = IFS_Strlen( s_buf );

	IFS_Sprintf( filename, "%s%s%s", IFS_GetRootDir(), IFS_GetPathSep(), FS_ASSERT_OUT_FILE );
	if( ! IFS_FileOpen( &hFile, filename, FS_READ_WRITE ) )
	{
		ret = IFS_FileCreate( &hFile, filename, FS_READ_WRITE );
	}
	
	if( ret )
	{
		offset = IFS_FileGetSize( hFile );
		IFS_FileSetPos( hFile, offset );
		IFS_FileWrite( hFile, s_buf, len );
		IFS_FileClose( hFile );
	}
}

void FS_DebugPrintLog( FS_CHAR *fmt, ... )
{
	static FS_CHAR s_buf[4096];
	FS_CHAR filename[64];
	FS_Handle hFile;
	FS_SINT4 offset, len;
	va_list args;
	FS_BOOL ret = FS_TRUE;
	FS_DateTime dt = {0};

	IFS_GetDateTime( &dt );
	IFS_Snprintf( s_buf, sizeof(s_buf) - 1, "%02d:%02d:%02d-", dt.hour, dt.min, dt.sec );

	va_start( args, fmt );
	_vsnprintf( s_buf + IFS_Strlen(s_buf), sizeof(s_buf) - 1 - IFS_Strlen(s_buf), fmt, args );
	va_end( args );
	
	len = IFS_Strlen( s_buf );
	if ( s_buf[len - 1] != '\n' )
	{
		s_buf[len] = '\n';
		len ++;
	}

	IFS_Snprintf( filename, sizeof(filename) - 1, "%s%s%s", IFS_GetRootDir(), IFS_GetPathSep(), FS_TRACE_OUT_FILE );
	if( ! IFS_FileOpen( &hFile, filename, FS_READ_WRITE ) )
	{
		ret = IFS_FileCreate( &hFile, filename, FS_READ_WRITE );
	}
	
	if( ret )
	{
		offset = IFS_FileGetSize( hFile );
		IFS_FileSetPos( hFile, offset );
		IFS_FileWrite( hFile, s_buf, len );
		IFS_FileClose( hFile );
	}
}

#endif

