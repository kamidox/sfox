#include "inc\util\FS_File.h"
#include "inc\inte\FS_Inte.h"
#include "inc\util\FS_Util.h"
#include "inc\util\FS_MemDebug.h"

#define FS_FILE_SEED		"seed.bin"

static FS_UINT4 GFS_FileSeed = 0;
static FS_CHAR *GFS_EmlRoot;
static FS_CHAR *GFS_MmsRoot;
static FS_CHAR *GFS_WebRoot;
static FS_CHAR *GFS_DcdRoot;
static FS_CHAR *GFS_StkRoot;
static FS_CHAR *GFS_TmpRoot;
static FS_CHAR *GFS_DirRoot;
static FS_CHAR *GFS_DirSep;

void FS_GetAbsFileName( FS_SINT4 dir, FS_CHAR *pName, FS_CHAR *FullName )
{
	if( FS_DIR_EML == dir )
		IFS_Strcpy( FullName, GFS_EmlRoot );
	else if( FS_DIR_MMS == dir )
		IFS_Strcpy( FullName, GFS_MmsRoot );
	else if( FS_DIR_WEB == dir )
		IFS_Strcpy( FullName, GFS_WebRoot );
	else if( FS_DIR_TMP == dir )
		IFS_Strcpy( FullName, GFS_TmpRoot );
	else if( FS_DIR_DCD == dir )
		IFS_Strcpy( FullName, GFS_DcdRoot );	
	else if( FS_DIR_ROOT == dir )
		IFS_Strcpy( FullName, GFS_DirRoot );
	else if( FS_DIR_STK == dir )
		IFS_Strcpy( FullName, GFS_StkRoot );	
	else
	{
		IFS_Strcpy( FullName, pName );
		return;
	}
	
	IFS_Strcat( FullName, GFS_DirSep );
	IFS_Strcat( FullName, pName );
}

FS_BOOL FS_FileCreate( FS_Handle *hFile, FS_SINT4 dir, FS_CHAR *pName, FS_UINT4 aflag )
{
	FS_CHAR filename[FS_MAX_PATH_LEN];
	FS_GetAbsFileName( dir, pName, filename );
	return IFS_FileCreate( hFile, filename, aflag );
}

FS_BOOL FS_FileOpen( FS_Handle *hFile, FS_SINT4 dir, FS_CHAR *pName, FS_UINT4 aflag )
{
	FS_CHAR filename[FS_MAX_PATH_LEN];
	FS_GetAbsFileName( dir, pName, filename );
	return IFS_FileOpen( hFile, filename, aflag );
}

FS_BOOL FS_FileMove( FS_SINT4 sdir, FS_CHAR *oldname, FS_SINT4 ddir, FS_CHAR *newname )
{
	FS_CHAR oname[FS_MAX_PATH_LEN], nname[FS_MAX_PATH_LEN];
	FS_GetAbsFileName( sdir, oldname, oname );
	FS_GetAbsFileName( ddir, newname, nname );
	return IFS_FileMove( oname, nname );
}

FS_BOOL FS_FileCopy( FS_SINT4 sdir, FS_CHAR *sname, FS_SINT4 ddir, FS_CHAR *dname )
{
	FS_CHAR oname[FS_MAX_PATH_LEN], nname[FS_MAX_PATH_LEN];
	FS_GetAbsFileName( sdir, sname, oname );
	FS_GetAbsFileName( ddir, dname, nname );
	return IFS_FileCopy( oname, nname );
}

FS_BOOL FS_FileDelete( FS_SINT4 dir, FS_CHAR *pName )
{
	FS_CHAR filename[FS_MAX_PATH_LEN];
	FS_GetAbsFileName( dir, pName, filename );
	return IFS_FileDelete( filename );
}

FS_CHAR * FS_GetFileExt( FS_CHAR *filename )
{
	FS_CHAR *ret = FS_NULL, *ext = FS_NULL;
	
	if( filename )
	{
		ext = IFS_Strchr( filename, '.' );
		while( ext )
		{
			ext ++; 		// skip '.'
			ret = ext;
			ext = IFS_Strchr( ext, '.' );
		}
	}
	return ret;
}

FS_CHAR * FS_GetFileNameFromPath( FS_CHAR *path )
{
	FS_CHAR *ret = FS_NULL, *name = FS_NULL;
	
	if( path )
	{
		name = IFS_Strstr( path, GFS_DirSep );
		
		if( name == FS_NULL )
			return path;
		
		while( name )
		{
			name += IFS_Strlen( GFS_DirSep ); 		// skip sep
			ret = name;
			name = IFS_Strstr( name, GFS_DirSep );
		}
	}
	return ret;
}

FS_BOOL FS_InitFileSys( void )
{
	/* setup system path */
	GFS_DirRoot = IFS_Strdup( IFS_GetRootDir() );
	GFS_DirSep = IFS_Strdup( IFS_GetPathSep() );
	GFS_EmlRoot = FS_StrConCat( GFS_DirRoot, GFS_DirSep, "eml", FS_NULL );
	GFS_MmsRoot = FS_StrConCat( GFS_DirRoot, GFS_DirSep, "mms", FS_NULL );
	GFS_WebRoot = FS_StrConCat( GFS_DirRoot, GFS_DirSep, "web", FS_NULL );
	GFS_DcdRoot = FS_StrConCat( GFS_DirRoot, GFS_DirSep, "dcd", FS_NULL );
	GFS_StkRoot = FS_StrConCat( GFS_DirRoot, GFS_DirSep, "stk", FS_NULL );
	GFS_TmpRoot = FS_StrConCat( GFS_DirRoot, GFS_DirSep, "tmp", FS_NULL );
	
	IFS_DirCreate( GFS_DirRoot );
#ifdef FS_MODULE_EML
	IFS_DirCreate( GFS_EmlRoot );
#endif
#ifdef FS_MODULE_MMS
	IFS_DirCreate( GFS_MmsRoot );
#endif
#ifdef FS_MODULE_WEB
	IFS_DirCreate( GFS_WebRoot );
#endif
#ifdef FS_MODULE_DCD
	IFS_DirCreate( GFS_DcdRoot );
#endif
#ifdef FS_MODULE_STK
	IFS_DirCreate( GFS_StkRoot );
#endif
	IFS_DirCreate( GFS_TmpRoot );
	if( FS_FileRead( FS_DIR_ROOT, FS_FILE_SEED, 0, &GFS_FileSeed, sizeof(FS_UINT4) ) != sizeof(FS_UINT4) )
	{
		GFS_FileSeed = 1;
	}
	return FS_TRUE;
}

void FS_DeinitFileSys( void )
{
	FS_FileWrite( FS_DIR_ROOT, FS_FILE_SEED, 0, &GFS_FileSeed, sizeof(FS_UINT4) );
	
	FS_SAFE_FREE( GFS_DirRoot );
	FS_SAFE_FREE( GFS_DirSep );
	FS_SAFE_FREE( GFS_EmlRoot );
	FS_SAFE_FREE( GFS_MmsRoot );
	FS_SAFE_FREE( GFS_WebRoot );
	FS_SAFE_FREE( GFS_DcdRoot );
	FS_SAFE_FREE( GFS_StkRoot );
	FS_SAFE_FREE( GFS_TmpRoot );
}

/*
	return a 10 byte length local unique id
*/
FS_CHAR * FS_GetGuid( FS_CHAR * out )
{
	FS_CHAR str[16];
	FS_SINT4 len;
	
	IFS_Itoa( GFS_FileSeed ++, str, 10 );
	len = 10 - IFS_Strlen(str);
	out[0] = 0;
	if( len > 0 )
	{
		IFS_Strncpy( out, "000000000000000", len );
		out[len] = 0;
	}
	IFS_Strcat( out, str );
	FS_FileWrite( FS_DIR_ROOT, FS_FILE_SEED, 0, &GFS_FileSeed, sizeof(FS_UINT4) );
	return out;
}

