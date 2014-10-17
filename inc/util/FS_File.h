#ifndef _FS_FILE_H
#define _FS_FILE_H
#include "inc\FS_Config.h"
#include "inc\inte\FS_Inte.h"

#define FS_DIR_ROOT		0
#define FS_DIR_EML		1
#define FS_DIR_MMS		2
#define FS_DIR_WEB		3
#define FS_DIR_TMP		4
#define FS_DIR_DCD		5
#define FS_DIR_STK		6

#define FS_MAX_PATH_LEN		128
#define FS_FILE_NAME_LEN	32

#define FS_FILE_BLOCK		4096
#define FS_URL_LEN			128
#define FS_MAX_URL_LEN		4096

FS_BOOL FS_FileCreate( FS_Handle *hFile, FS_SINT4 dir, FS_CHAR *pName, FS_UINT4 aflag );

FS_BOOL FS_FileOpen( FS_Handle *hFile, FS_SINT4 dir, FS_CHAR *pName, FS_UINT4 aflag );

FS_CHAR * FS_GetFileExt( FS_CHAR *filename );

FS_CHAR * FS_GetFileNameFromPath( FS_CHAR *path );

FS_BOOL FS_FileDelete( FS_SINT4 dir, FS_CHAR *pName );

FS_BOOL FS_FileMove( FS_SINT4 sdir, FS_CHAR *oldname, FS_SINT4 ddir, FS_CHAR *newname );

FS_BOOL FS_FileCopy( FS_SINT4 sdir, FS_CHAR *sname, FS_SINT4 ddir, FS_CHAR *dname );

FS_BOOL FS_InitFileSys( void );

void FS_DeinitFileSys( void );

/*
	return a 10 byte length local unique id
*/
FS_CHAR * FS_GetGuid( FS_CHAR * out );

void FS_GetAbsFileName( FS_SINT4 dir, FS_CHAR *pName, FS_CHAR *FullName );

#endif
