#ifndef _FS_IFILE_H_
#define _FS_IFILE_H_
#include "inc\FS_Config.h"

/* file handler */
typedef FS_UINT4 FS_Handle;

/* file access flag */
#define FS_OPEN_READ					0x0001
#define FS_OPEN_WRITE					0x0002
#define FS_READ_WRITE					(0x0001 | 0x0002)

/*--------------------------------------------------------------------------------------
		file operator interface
----------------------------------------------------------------------------------------*/

// create a new file, overwrite exist one
FS_BOOL 	IFS_FileCreate( FS_Handle *hFile, FS_CHAR *pFileName, FS_UINT4 aflag );

// open a file to read
FS_BOOL 	IFS_FileOpen( FS_Handle *hFile, FS_CHAR *pFileName, FS_UINT4 aflag );
/*
	return 0 if reach to file end ; return < 0 if error occured
	or return file readed
*/
FS_SINT4	IFS_FileRead( FS_Handle hFile, void *buf, FS_UINT4 len ); 
/*
	return <= 0 if error occured
	return file writed
*/
FS_SINT4	IFS_FileWrite( FS_Handle hFile, void * buf, FS_UINT4 len ); 

FS_SINT4	IFS_FileSetPos( FS_Handle hFile, FS_SINT4 pos );

FS_BOOL 	IFS_FileClose( FS_Handle hFile );

FS_BOOL 	IFS_FileMove( FS_CHAR * oldName, FS_CHAR *newName );

FS_BOOL 	IFS_FileCopy( FS_CHAR * oldName, FS_CHAR *newName );

FS_BOOL 	IFS_FileDelete( FS_CHAR * pFileName );

FS_SINT4	IFS_FileGetSize( FS_Handle hFile );

FS_BOOL 	IFS_DirCreate( FS_CHAR *dir );

#endif
