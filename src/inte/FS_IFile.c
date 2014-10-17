#include "inc/inte/FS_IFile.h"
#include "inc/inte/FS_ISystem.h"

#ifdef FS_PLT_WIN
#include <windows.h>
FS_CHAR *Utf8ToGb2312( FS_CHAR *utf8 );
#endif

#ifdef FS_PLT_VIENNA
#include "..\system\portab.h"
#include "..\system\sysprim.h"
#include "..\system\syslib.h"
#include "..\amoi\explorer\amoifs.h"

static char *IFS_FsPath2SysPath( char *path )
{
	static char s_path[256];
	int i, len = GSMstrlen( path );
	GSMmemcpy( s_path, path, len + 1 );
	#ifdef SIMULATION
	if( s_path[0] != '/' )
	{
		for( i = 0; i < len; i ++ )
		{
			if( s_path[i] == '/' ) s_path[i] = '\\';
		}
	}
	#endif
	return s_path;
}

#endif
//--------------------------------------------------------------------
// file operator interface
FS_BOOL IFS_FileCreate( FS_Handle *hFile, FS_CHAR *pFileName, FS_UINT4 aflag )
{
#ifdef FS_PLT_WIN
	FS_BOOL ret = FS_FALSE;
	FS_UINT4 acsflg = 0;
	HANDLE handle;
	FS_CHAR *fname;
	
	if( aflag & FS_OPEN_READ )
		acsflg |= GENERIC_READ;

	if( aflag & FS_OPEN_WRITE )
		acsflg |= GENERIC_WRITE;

	fname = Utf8ToGb2312( pFileName );
	handle = CreateFile( fname, acsflg, 0, FS_NULL, CREATE_ALWAYS, 0, 0 );
	free( fname );
	if( handle != NULL && (unsigned long)handle != 0xFFFFFFFF )
	{
		*hFile = (FS_UINT4)handle;
		ret = FS_TRUE;
	}
	return ret;
#endif
#ifdef FS_PLT_VIENNA
	UFSFD h;
	UINT16 flag = 0;
	FS_BOOL ret = FS_FALSE;
	if( aflag & FS_OPEN_READ )
		flag |= UFS_READ;
	
	flag |= UFS_WRITE;

	UFS_FileDelete( IFS_FsPath2SysPath(pFileName) );
	if( (h = UFS_FileOpen( IFS_FsPath2SysPath(pFileName), UFS_CREATE | UFS_OVERWRITE | flag)) > 0 )
	{
		*hFile = (FS_UINT4)h;
		ret = FS_TRUE;
	}
	return ret;
#endif
}

//--------------------------------------------------------------------
// file operator interface
FS_BOOL IFS_FileOpen( FS_Handle *hFile, FS_CHAR *pFileName, FS_UINT4 aflag )
{
#ifdef FS_PLT_WIN
	FS_BOOL ret = FS_FALSE;
	HANDLE handle;
	FS_UINT4 acsflg = 0;
	FS_CHAR *fname;
	if( aflag & FS_OPEN_READ )
		acsflg |= GENERIC_READ;

	if( aflag & FS_OPEN_WRITE )
		acsflg |= GENERIC_WRITE;
	
	fname = Utf8ToGb2312( pFileName );
	handle = CreateFile( fname, acsflg, 0, FS_NULL, OPEN_EXISTING, 0, 0 );
	free( fname );
	if( handle != NULL && (unsigned long)handle != 0xFFFFFFFF )
	{
		*hFile = (FS_UINT4)handle;
		ret = FS_TRUE;
	}
	return ret;
#endif
#ifdef FS_PLT_VIENNA
	UFSFD h;
	UINT16 flag = 0;
	FS_BOOL ret = FS_FALSE;
	if( aflag & FS_OPEN_READ )
		flag |= UFS_READ;
	
	if( aflag & FS_OPEN_WRITE )
		flag |= UFS_WRITE;
	
	if( (h = UFS_FileOpen( IFS_FsPath2SysPath(pFileName), flag)) > 0 )
	{
		*hFile = (FS_UINT4)h;
		ret = FS_TRUE;
	}
	return ret;
#endif
}
/*
	return = 0 if reach to file end ; return <= 0 if error occured
	or return file readed
*/
FS_SINT4 IFS_FileRead( FS_Handle hFile, void *buf, FS_UINT4 len )
{
#ifdef FS_PLT_WIN
	FS_SINT4 ret = 0;
	ReadFile( (HANDLE)hFile, buf, len, &ret, FS_NULL);
	return ret;
 #endif 
#ifdef FS_PLT_VIENNA
	FS_SINT4 ret;
	ret = UFS_FileRead( (UFSFD)hFile, len, buf );
	return ret;
#endif
}

/*
	return <= 0 if error occured
	return file writed
*/
FS_SINT4 IFS_FileWrite( FS_Handle hFile, void *buf, FS_UINT4 len )
{
#ifdef FS_PLT_WIN
	FS_SINT4 ret;
	WriteFile( (HANDLE)hFile, buf, len, &ret, FS_NULL);
	return ret;
#endif	
#ifdef FS_PLT_VIENNA
	FS_SINT4 ret;
	ret = UFS_FileWrite( (UFSFD)hFile, len, buf );
	return ret;
#endif
}

FS_SINT4 IFS_FileSetPos( FS_Handle hFile, FS_SINT4 pos )
{
#ifdef FS_PLT_WIN
	return SetFilePointer( (HANDLE)hFile, pos, NULL, 0 );
#endif
#ifdef FS_PLT_VIENNA
	FS_SINT4 ret = -1;
	if( UFS_FileSetPos( (UFSFD)hFile, pos ) == AFS_OK )
		ret = pos;
	return ret;
#endif
}

FS_BOOL IFS_FileClose( FS_Handle hFile )
{
#ifdef FS_PLT_WIN
	return CloseHandle( (HANDLE)hFile );
#endif
#ifdef FS_PLT_VIENNA
	FS_BOOL ret = FS_FALSE;
	if( UFS_FileClose( (UFSFD)hFile ) == AFS_OK )
		ret = FS_TRUE;
	return ret;
#endif
}

FS_BOOL IFS_FileDelete( FS_CHAR *pFileName )
{
#ifdef FS_PLT_WIN
	FS_BOOL ret;
	FS_CHAR *fname;
	fname = Utf8ToGb2312( pFileName );
	ret = DeleteFile( pFileName );
	free( fname );
	return ret;
#endif
#ifdef FS_PLT_VIENNA
	FS_BOOL ret = FS_FALSE;
	if( AFS_OK == UFS_FileDelete( pFileName ) )
		ret = FS_TRUE;
	return ret;
#endif
}

FS_BOOL IFS_FileMove( FS_CHAR * oldName, FS_CHAR *newName )
{
#ifdef FS_PLT_WIN
	FS_CHAR *sName, *dName;
	FS_BOOL ret;
	
	sName = Utf8ToGb2312( oldName );
	dName = Utf8ToGb2312( newName );
	ret = MoveFile( sName, dName );
	free( sName );
	free( dName );
	return ret;
#endif
#ifdef FS_PLT_VIENNA
	FS_BOOL ret = FS_FALSE;
	char *srcFile = GSMstrdup(IFS_FsPath2SysPath(oldName) );
	if( AFS_OK == UFS_FileMove( srcFile, IFS_FsPath2SysPath(newName), FS_TRUE ) )
		ret = FS_TRUE;
	GSMFree( srcFile );
	return ret;
#endif
}

FS_BOOL IFS_FileCopy( FS_CHAR * oldName, FS_CHAR *newName )
{
#ifdef FS_PLT_WIN
	FS_CHAR *sName, *dName;
	FS_BOOL ret;
	
	sName = Utf8ToGb2312( oldName );
	dName = Utf8ToGb2312( newName );
	return CopyFile( sName, dName, FS_FALSE );
	free( sName );
	free( dName );
	return ret;
#endif
#ifdef FS_PLT_VIENNA
	FS_BOOL ret = FS_FALSE;
	char *srcFile = GSMstrdup(IFS_FsPath2SysPath(oldName) );
	if( AFS_OK == UFS_FileCopy( srcFile, IFS_FsPath2SysPath(newName), FS_TRUE ) )
		ret = FS_TRUE;
	GSMFree( srcFile );
	return ret;
#endif
}

FS_SINT4 IFS_FileGetSize( FS_Handle hFile )
{
#ifdef FS_PLT_WIN
	return GetFileSize( (HANDLE)hFile, FS_NULL );
#endif
#ifdef FS_PLT_VIENNA
	FS_SINT4 ret = 0;
	UFS_FileGetSize( (UFSFD)hFile, &ret );
	return ret;
#endif
}

FS_BOOL IFS_DirCreate( FS_CHAR *dir )
{
#ifdef FS_PLT_WIN
	FS_BOOL ret;
	FS_CHAR *fname;

	fname = Utf8ToGb2312( dir );
	ret = CreateDirectory( fname, FS_NULL );
	free( fname );
	return ret;
#endif
#ifdef FS_PLT_VIENNA
	FS_BOOL ret = FS_FALSE;
	if( AFS_OK == UFS_DirCreate( IFS_FsPath2SysPath(dir) ) )
		ret = FS_TRUE;
	return ret;
#endif
}

