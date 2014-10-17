#ifndef _FS_MMS_FILE_H_
#define _FS_MMS_FILE_H_

#include "inc/FS_Config.h"
#include "inc/mms/FS_MmsCodec.h"
#include "inc/mms/FS_Smil.h"

typedef struct FS_MmsFile_Tag
{
	FS_MmsSmil *	smil;

	FS_MmsEncData	data;
}FS_MmsFile;

FS_MmsFile *FS_CreateMmsFile( void );

void FS_DestroyMmsFile( FS_MmsFile *pMmsFile );

void FS_MmsEncodeFile( FS_CHAR *file, FS_MmsFile *pMmsFile );

FS_MmsFile * FS_MmsDecodeFile( FS_CHAR *file );

FS_MmsEncEntry * FS_MmsFileGetEntryByCid( FS_MmsFile *pMmsFile, FS_CHAR *cid );

FS_CHAR *FS_MmsFileGetEntryFileByCid( FS_MmsFile *pMmsFile, FS_CHAR *cid );

FS_SINT4 FS_MmsFileGetObjectTotalSize( FS_MmsFile *pMmsFile );

FS_CHAR *FS_MmsFileGetEntryFileByIdx( FS_MmsFile *pMmsFile, FS_SINT4 type, FS_SINT4 index );

FS_SINT4 FS_MmsFileGetFrameCount( FS_MmsFile *pMmsFile );

FS_MmsEncEntry *FS_MmsFileGetEntryByIdx( FS_MmsFile *pMmsFile, FS_SINT4 type, FS_SINT4 index );

FS_SINT4 FS_MmsFileGetToAddr( FS_CHAR *out, FS_SINT4 olen, FS_MmsFile *pMmsFile );

FS_SINT4 FS_MmsFileGetCcAddr( FS_CHAR *out, FS_SINT4 olen, FS_MmsFile *pMmsFile );

FS_SINT4 FS_MmsFileGetBccAddr( FS_CHAR *out, FS_SINT4 olen, FS_MmsFile *pMmsFile );

void FS_MmsFileSetToAddr( FS_MmsFile *pMmsFile, FS_CHAR *addr );

void FS_MmsFileSetCcAddr( FS_MmsFile *pMmsFile, FS_CHAR *addr );

void FS_MmsFileSetBccAddr( FS_MmsFile *pMmsFile, FS_CHAR *addr );

FS_SINT4 FS_MmsFileGetFromAddr( FS_CHAR *out, FS_SINT4 olen, FS_MmsFile *pMmsFile );

FS_CHAR * FS_MmsFileGetSubject( FS_MmsFile *pMmsFile );

void FS_MmsFileSetSubject( FS_MmsFile *pMmsFile, FS_CHAR *str );

FS_SINT4 FS_MmsAddrToUserFormat( FS_CHAR *out, FS_SINT4 olen, FS_CHAR *in, FS_SINT4 ilen );

FS_CHAR * FS_MmsFileGetFrameTextFile( FS_MmsFile *pMmsFile, FS_SINT4 num );

FS_CHAR *FS_MmsFileGetFrameImageFile( FS_MmsFile *pMmsFile, FS_SINT4 num );

FS_CHAR *FS_MmsFileGetFrameAudioFile( FS_MmsFile *pMmsFile, FS_SINT4 num );

FS_CHAR *FS_MmsFileGetFrameVideoFile( FS_MmsFile *pMmsFile, FS_SINT4 num );

FS_CHAR *FS_MmsFileGetFrameTextFileName( FS_MmsFile *pMmsFile, FS_SINT4 num );

FS_CHAR *FS_MmsFileGetFrameImageFileName( FS_MmsFile *pMmsFile, FS_SINT4 num );

FS_CHAR *FS_MmsFileGetFrameAudioFileName( FS_MmsFile *pMmsFile, FS_SINT4 num );

FS_CHAR *FS_MmsFileGetFrameVideoFileName( FS_MmsFile *pMmsFile, FS_SINT4 num );

FS_BOOL FS_MmsFileIsEmptyFrame( FS_MmsFile *pMmsFile, FS_SINT4 num );

FS_MmsFile * FS_MmsFileDuplicate( FS_MmsFile *pSrcMms );

void FS_MmsFileAddFrame( FS_MmsFile *pMmsFile, FS_SINT4 index );

FS_BOOL FS_MmsFileGetDlvReportFlag( FS_MmsFile *pMmsFile );

void FS_MmsFileSetDlvReportFlag( FS_MmsFile *pMmsFile, FS_BOOL bSet );

void FS_MmsFileSetReadReportFlag( FS_MmsFile *pMmsFile, FS_BOOL bSet );

FS_BOOL FS_MmsFileGetReadReportFlag( FS_MmsFile *pMmsFile );

#endif
