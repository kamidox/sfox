#include "inc/FS_Config.h"

#ifdef FS_MODULE_MMS

#include "inc/mms/FS_MmsFile.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_Mime.h"
#include "inc/util/FS_MemDebug.h"

#define FS_MMS_HEAD_LEN		512

FS_SINT4 FS_MmsAddrToUserFormat( FS_CHAR *out, FS_SINT4 olen, FS_CHAR *in, FS_SINT4 ilen )
{
	FS_CHAR *p = FS_NULL;
	FS_SINT4 len = 0;
	
	if( in && out )
	{
		olen --;	/* save a byte to write str term char */
		if( ilen <= 0 )
			ilen = IFS_Strlen( in );

		len = ilen;
		while( len > 0 && in[len - 1] != '/' )
			len --;

		if( len != 0 )
			p = in + len - 1;
		
		if( p )
		{
			len = p - in;
			len = FS_MIN( len, olen );
			IFS_Memcpy( out, in, len );
			out[len] = 0;
		}
		else
		{
			len = FS_MIN( ilen, olen );
			IFS_Strncpy( out, in, len );
			out[len] = 0;
		}
	}
	return len;
}

static FS_BOOL FS_MmsFileEntryTypeMatch( FS_MmsEncEntry *pEntry, FS_SINT4 type )
{
	FS_SINT4 et = 0;
	FS_BOOL ret = FS_FALSE;
	if( FS_MIME_TEXT == type )
	{
		if( pEntry->ct_code == FS_WCT_TEXT_PLAIN 
			|| FS_STR_I_EQUAL( pEntry->content_type, "text/plain" ) )
		{
			ret = FS_TRUE;
		}
	}
	else
	{
		if( pEntry->ct_code )
			et = FS_GetTypeFromMimeCode( pEntry->ct_code );
		else
			et = FS_GetTypeFromMime( pEntry->content_type );
		
		if( et & type )
		{
			/* here, it's '&' not '=='. becuase type may combined. MMS Image and Video is in the save area */
			ret = FS_TRUE;
		}
	}

	return ret;
}

static FS_SINT4 FS_MmsAddrListToUserFormat( FS_CHAR *out, FS_SINT4 olen, FS_CHAR *in )
{
	FS_SINT4 tlen, offset = 0;
	FS_CHAR *p, *str = in;
	
	if( str )
	{
		p = IFS_Strchr( str, ',' );
		while( 1 )
		{
			if( p )
			{
				tlen = p - str;
				if( tlen + offset > olen )
					break;
				offset += FS_MmsAddrToUserFormat( out + offset, olen - offset, str, tlen );
				out[offset ++] = ',';
				str = p + 1;
				p = IFS_Strchr( str, ',' );
			}
			else
			{
				offset += FS_MmsAddrToUserFormat( out + offset, olen - offset, str, -1 );
				break;
			}
		}
	}
	
	return offset;
}

static FS_SINT4 FS_MmsFileGetEntryCountByType( FS_MmsFile *pMmsFile, FS_SINT4 type )
{
	FS_SINT4 ret = 0;
	FS_List *node;
	FS_MmsEncEntry *pEntry;

	node = pMmsFile->data.body.entry_list.next;
	while( node != &pMmsFile->data.body.entry_list )
	{
		pEntry = FS_ListEntry( node, FS_MmsEncEntry, list );
		node = node->next;
		
		if( FS_MmsFileEntryTypeMatch( pEntry, type ) )
			ret ++;
	}
	return ret;
}

FS_MmsEncEntry * FS_MmsFileGetEntryByCid( FS_MmsFile *pMmsFile, FS_CHAR *cid )
{
	FS_List *node;
	FS_MmsEncEntry *pEntry;
	FS_MmsEncMultipart *body = &pMmsFile->data.body;
	
	if( body->count <= 0 )
		return FS_NULL;
	
	node = body->entry_list.next;

	if( cid == FS_NULL )
		return FS_ListEntry( node, FS_MmsEncEntry, list );
	
	while( node != &body->entry_list )
	{
		pEntry = FS_ListEntry( node, FS_MmsEncEntry, list );
		node = node->next;
		if( FS_ContentIdEqual(pEntry->content_id, cid)
			|| FS_STR_I_EQUAL(pEntry->content_location, cid) )
		{
			return pEntry;
		}
	}

	return FS_NULL;
}

FS_MmsEncEntry *FS_MmsFileGetEntryByIdx( FS_MmsFile *pMmsFile, FS_SINT4 type, FS_SINT4 index )
{
	FS_List *node;
	FS_MmsEncEntry *pEntry;
	FS_MmsEncMultipart *body = &pMmsFile->data.body;

	if(  body->count < index )
		return FS_NULL;

	node = body->entry_list.next;
	while( node != &body->entry_list )
	{
		pEntry = FS_ListEntry( node, FS_MmsEncEntry, list );
		node = node->next;
		
		if( FS_MmsFileEntryTypeMatch( pEntry, type ) )
		{
			index --;
		}
		
		if( index == 0 )
		{
			return pEntry;
		}
	}
	
	return FS_NULL;
}

/* will return abs file path name */
FS_CHAR *FS_MmsFileGetEntryFileByCid( FS_MmsFile *pMmsFile, FS_CHAR *cid )
{
	static FS_CHAR s_absfile[FS_MAX_PATH_LEN];
	FS_CHAR *ret = FS_NULL;
	
	FS_MmsEncEntry *pEntry = FS_MmsFileGetEntryByCid( pMmsFile, cid );
	if( pEntry )
	{
		if( pEntry->temp_file )
		{
			FS_GetAbsFileName( FS_DIR_TMP, pEntry->file, s_absfile );
			ret = s_absfile;
		}
		else
		{
			ret = pEntry->file;
		}
	}
	return ret;
}

/* will return abs file path name. index is begin from one(1) */
FS_CHAR *FS_MmsFileGetEntryFileByIdx( FS_MmsFile *pMmsFile, FS_SINT4 type, FS_SINT4 index )
{
	static FS_CHAR s_absfile[FS_MAX_PATH_LEN];
	FS_CHAR *ret = FS_NULL;
	FS_MmsEncEntry *pEntry;

	pEntry = FS_MmsFileGetEntryByIdx( pMmsFile, type, index );
	if( pEntry )
	{
		/* we find one */
		if( pEntry->temp_file )
		{
			FS_GetAbsFileName( FS_DIR_TMP, pEntry->file, s_absfile );
			ret = s_absfile;
		}
		else
		{
			ret = pEntry->file;
		}
	}
	
	return ret;
}

FS_SINT4 FS_MmsFileGetToAddr( FS_CHAR *out, FS_SINT4 olen, FS_MmsFile *pMmsFile )
{
	return FS_MmsAddrListToUserFormat( out, olen, pMmsFile->data.head.to );
}

FS_SINT4 FS_MmsFileGetFromAddr( FS_CHAR *out, FS_SINT4 olen, FS_MmsFile *pMmsFile )
{
	return FS_MmsAddrListToUserFormat( out, olen, pMmsFile->data.head.from );
}

FS_SINT4 FS_MmsFileGetCcAddr( FS_CHAR *out, FS_SINT4 olen, FS_MmsFile *pMmsFile )
{
	return FS_MmsAddrListToUserFormat( out, olen, pMmsFile->data.head.cc );
}

FS_SINT4 FS_MmsFileGetBccAddr( FS_CHAR *out, FS_SINT4 olen, FS_MmsFile *pMmsFile )
{
	return FS_MmsAddrListToUserFormat( out, olen, pMmsFile->data.head.bcc );
}

void FS_MmsFileSetToAddr( FS_MmsFile *pMmsFile, FS_CHAR *addr )
{
	FS_COPY_TEXT( pMmsFile->data.head.to, addr );
}

void FS_MmsFileSetCcAddr( FS_MmsFile *pMmsFile, FS_CHAR *addr )
{
	FS_COPY_TEXT( pMmsFile->data.head.cc, addr );
}

void FS_MmsFileSetBccAddr( FS_MmsFile *pMmsFile, FS_CHAR *addr )
{
	FS_COPY_TEXT( pMmsFile->data.head.to, addr );
}

FS_CHAR * FS_MmsFileGetSubject( FS_MmsFile *pMmsFile )
{
	return pMmsFile->data.head.subject;
}

void FS_MmsFileSetSubject( FS_MmsFile *pMmsFile, FS_CHAR *str )
{
	FS_COPY_TEXT( pMmsFile->data.head.subject, str );
}

FS_SINT4 FS_MmsFileGetObjectTotalSize( FS_MmsFile *pMmsFile )
{
	FS_SINT4 nSize = 0;

	FS_List *node;
	FS_MmsEncEntry *pEntry;
	node = pMmsFile->data.body.entry_list.next;
	while( node != &pMmsFile->data.body.entry_list )
	{
		pEntry = FS_ListEntry( node, FS_MmsEncEntry, list );
		node = node->next;

		if( pEntry->data_len <= 0 )
		{
			if( pEntry->temp_file )
				pEntry->data_len = FS_FileGetSize( FS_DIR_TMP, pEntry->file );
			else
				pEntry->data_len = FS_FileGetSize( -1, pEntry->file );
		}

		nSize += pEntry->data_len;
	}
	return nSize > 0 ? (nSize + FS_MMS_HEAD_LEN) : nSize;
}

void FS_MmsFileAddFrame( FS_MmsFile *pMmsFile, FS_SINT4 index )
{
	if( pMmsFile->smil )
	{
		FS_SmilAddMmsFrame( pMmsFile->smil, index );
	}
}

FS_MmsFile * FS_MmsDecodeFile( FS_CHAR *file )
{
	FS_MmsFile *pMmsFile;
	FS_MmsEncEntry *pEntry;
	FS_CHAR absFile[FS_MAX_PATH_LEN];
	FS_BOOL ret = FS_FALSE;
	
	pMmsFile = FS_NEW( FS_MmsFile );
	if( pMmsFile )
	{
		IFS_Memset( pMmsFile, 0, sizeof(FS_MmsFile) );
		FS_ListInit( &pMmsFile->data.body.entry_list );
		ret = FS_MmsCodecDecodeFile( &pMmsFile->data, file );
		if( ret )
		{
			if( pMmsFile->data.head.ct_code == FS_MMS_H_V_MULTIPART_RELATED
				|| FS_STR_I_EQUAL(pMmsFile->data.head.content_type, "application/vnd.wap.multipart.related") )
			{
				if( FS_STR_I_EQUAL(pMmsFile->data.head.param_type, "application/smil" ) )
				{
					 pEntry = FS_MmsFileGetEntryByCid( pMmsFile, pMmsFile->data.head.param_start );
					 if( pEntry && FS_STR_I_EQUAL(pEntry->content_type, "application/smil" ) )
					 {
						FS_GetAbsFileName( FS_DIR_TMP, pEntry->file, absFile );
						pMmsFile->smil = FS_SmilDecodeFile( absFile );
					 }
				}
			}
		}
	}

	if( ret == FS_FALSE && pMmsFile )
	{
		IFS_Free( pMmsFile );
		pMmsFile = FS_NULL;
	}
	
	return pMmsFile;
}

void FS_MmsEncodeFile( FS_CHAR *file, FS_MmsFile *pMmsFile )
{
	FS_CHAR absFile[FS_MAX_PATH_LEN], luid[32];
	FS_MmsEncEntry *pEntry;
	FS_CHAR *cid = "<smil>", *ct = "application/smil", *smilfile;
	
	if( pMmsFile->smil )
	{
		smilfile = FS_MmsFileGetEntryFileByCid( pMmsFile, cid );
		if( smilfile == FS_NULL )
		{
			FS_GetLuid( luid );
			IFS_Strcat( luid, ".smil" );
			FS_GetAbsFileName( FS_DIR_TMP, luid, absFile );
			FS_SmilEncodeFile( absFile, pMmsFile->smil );
			
			pEntry = FS_NEW( FS_MmsEncEntry );
			if( pEntry )
			{
				IFS_Memset( pEntry, 0, sizeof(FS_MmsEncEntry) );
				/* file */
				pEntry->file = IFS_Strdup( luid );
				pEntry->temp_file = FS_TRUE;
				/* generate a content-id */
				pEntry->content_id = IFS_Strdup( cid );
				/* get content-type from file extension */
				pEntry->content_type = IFS_Strdup( ct );
				pEntry->data_len = FS_FileGetSize( FS_DIR_TMP, luid );
			
				FS_ListAdd( &pMmsFile->data.body.entry_list, &pEntry->list );
				pMmsFile->data.body.count ++;
			}
		}
		else
		{
			FS_SmilEncodeFile( smilfile, pMmsFile->smil );
			pEntry = FS_MmsFileGetEntryByCid( pMmsFile, cid );
			pEntry->data_len = FS_FileGetSize( FS_DIR_TMP, pEntry->file );
		}

		pMmsFile->data.head.ct_code = FS_MMS_H_V_MULTIPART_RELATED;
		FS_SAFE_FREE( pMmsFile->data.head.param_start );
		FS_SAFE_FREE( pMmsFile->data.head.param_type );
		pMmsFile->data.head.param_start = IFS_Strdup( cid );
		pMmsFile->data.head.param_type = IFS_Strdup( ct );
	}

	FS_MmsCodecEncodeFile( file, &pMmsFile->data );
}

FS_MmsFile *FS_CreateMmsFile( void )
{
	FS_MmsFile *pMmsFile;
	
	pMmsFile = FS_NEW( FS_MmsFile );
	if( pMmsFile )
	{
		IFS_Memset( pMmsFile, 0, sizeof(FS_MmsFile) );
		FS_ListInit( &pMmsFile->data.body.entry_list );

		pMmsFile->smil = FS_CreateDefaultMmsSmil( );
		pMmsFile->data.head.ct_code = FS_MMS_H_V_MULTIPART_RELATED;
		pMmsFile->data.head.delivery_report = FS_MMS_H_V_DELIVERY_REPORT_NO;
		pMmsFile->data.head.read_report = FS_MMS_H_V_READ_REPORT_NO;
		pMmsFile->data.head.message_class = FS_MMS_H_V_CLASS_PERSONAL;
		pMmsFile->data.head.priority = FS_MMS_H_V_PRIORITY_NORMAL;
		pMmsFile->data.head.message_type = FS_M_SEND_REQ;
		pMmsFile->data.head.tid = IFS_Malloc( FS_MMS_TID_LEN );
		FS_GetLuid( pMmsFile->data.head.tid );
	}
	return pMmsFile;
}

void FS_DestroyMmsFile( FS_MmsFile *pMmsFile )
{
	if( pMmsFile )
	{
		if( pMmsFile->smil )
			FS_FreeMmsSmil( pMmsFile->smil );
		FS_MmsCodecFreeData( &pMmsFile->data );
		IFS_Free( pMmsFile );
	}
}

/* return a abs file name of frame text */
FS_CHAR * FS_MmsFileGetFrameTextFile( FS_MmsFile *pMmsFile, FS_SINT4 num )
{
	FS_CHAR *cid, *src = FS_NULL;

	if( pMmsFile->smil )
	{
		cid = FS_SmilGetTextCid( pMmsFile->smil, num );
		if( cid )
		{
			src = FS_MmsFileGetEntryFileByCid( pMmsFile, cid );
		}
	}
	else	/* no smil mms */
	{
		src = FS_MmsFileGetEntryFileByIdx( pMmsFile, FS_MIME_TEXT, num );
	}
	return src;
}

/* return a abs file name of frame image/video */
FS_CHAR *FS_MmsFileGetFrameImageFile( FS_MmsFile *pMmsFile, FS_SINT4 num )
{
	FS_CHAR *cid, *src = FS_NULL;
	if( pMmsFile->smil )
	{
		cid = FS_SmilGetImageCid( pMmsFile->smil, num );
		if( cid )
		{
			src = FS_MmsFileGetEntryFileByCid( pMmsFile, cid );
		}
	}
	else	/* no smil mms */
	{
		src = FS_MmsFileGetEntryFileByIdx( pMmsFile, FS_MIME_IMAGE, num );
	}
	return src;
}
/* return a abs file name of frame audio */
FS_CHAR *FS_MmsFileGetFrameAudioFile( FS_MmsFile *pMmsFile, FS_SINT4 num )
{
	FS_CHAR *cid, *src = FS_NULL;
	if( pMmsFile->smil )
	{
		cid = FS_SmilGetAudioCid( pMmsFile->smil, num );
		if( cid )
		{
			src = FS_MmsFileGetEntryFileByCid( pMmsFile, cid );
		}
	}
	else	/* no smil mms */
	{
		src = FS_MmsFileGetEntryFileByIdx( pMmsFile, FS_MIME_AUDIO, num );
	}
	return src;
}

FS_CHAR *FS_MmsFileGetFrameVideoFile( FS_MmsFile *pMmsFile, FS_SINT4 num )
{
	FS_CHAR *cid, *src = FS_NULL;
	if( pMmsFile->smil )
	{
		cid = FS_SmilGetVideoCid( pMmsFile->smil, num );
		if( cid )
		{
			src = FS_MmsFileGetEntryFileByCid( pMmsFile, cid );
		}
	}
	else	/* no smil mms */
	{
		src = FS_MmsFileGetEntryFileByIdx( pMmsFile, FS_MIME_VIDEO, num );
	}
	return src;
}

FS_BOOL FS_MmsFileIsEmptyFrame( FS_MmsFile *pMmsFile, FS_SINT4 num )
{
	if( FS_MmsFileGetFrameVideoFile(pMmsFile, num) )
		return FS_FALSE;
	if( FS_MmsFileGetFrameAudioFile(pMmsFile, num) )
		return FS_FALSE;
	if( FS_MmsFileGetFrameImageFile(pMmsFile, num) )
		return FS_FALSE;
	
	return FS_TRUE;
}

FS_CHAR *FS_MmsFileGetFrameTextFileName( FS_MmsFile *pMmsFile, FS_SINT4 num )
{
	FS_CHAR *cid, *ret = FS_NULL;
	FS_MmsEncEntry *pEntry = FS_NULL;
	
	if( pMmsFile->smil )
	{
		cid = FS_SmilGetTextCid( pMmsFile->smil, num );
		if( cid )
		{
			pEntry = FS_MmsFileGetEntryByCid( pMmsFile, cid );
		}
	}
	else	/* no smil mms */
	{
		pEntry = FS_MmsFileGetEntryByIdx( pMmsFile, FS_MIME_TEXT, num );
	}
	
	if( pEntry )
	{
		if( pEntry->content_location )
			ret = pEntry->content_location;
		else if( pEntry->param_name )
			ret = pEntry->param_name;
		else
			ret = FS_GetFileNameFromPath( pEntry->file );
	}
	return ret;
}

FS_CHAR *FS_MmsFileGetFrameImageFileName( FS_MmsFile *pMmsFile, FS_SINT4 num )
{
	FS_CHAR *cid, *ret = FS_NULL;
	FS_MmsEncEntry *pEntry = FS_NULL;
	
	if( pMmsFile->smil )
	{
		cid = FS_SmilGetImageCid( pMmsFile->smil, num );
		if( cid )
		{
			pEntry = FS_MmsFileGetEntryByCid( pMmsFile, cid );
		}
	}
	else	/* no smil mms */
	{
		pEntry = FS_MmsFileGetEntryByIdx( pMmsFile, FS_MIME_IMAGE, num );
	}
	
	if( pEntry )
	{
		if( pEntry->content_location )
			ret = pEntry->content_location;
		else if( pEntry->param_name )
			ret = pEntry->param_name;
		else
			ret = FS_GetFileNameFromPath( pEntry->file );
	}
	return ret;
}

FS_CHAR *FS_MmsFileGetFrameAudioFileName( FS_MmsFile *pMmsFile, FS_SINT4 num )
{
	FS_CHAR *cid, *ret = FS_NULL;
	FS_MmsEncEntry *pEntry = FS_NULL;
	
	if( pMmsFile->smil )
	{
		cid = FS_SmilGetAudioCid( pMmsFile->smil, num );
		if( cid )
		{
			pEntry = FS_MmsFileGetEntryByCid( pMmsFile, cid );
		}
	}
	else	/* no smil mms */
	{
		pEntry = FS_MmsFileGetEntryByIdx( pMmsFile, FS_MIME_AUDIO, num );
	}
	
	if( pEntry )
	{
		if( pEntry->content_location )
			ret = pEntry->content_location;
		else if( pEntry->param_name )
			ret = pEntry->param_name;
		else
			ret = FS_GetFileNameFromPath( pEntry->file );
	}
	return ret;
}


FS_CHAR *FS_MmsFileGetFrameVideoFileName( FS_MmsFile *pMmsFile, FS_SINT4 num )
{
	FS_CHAR *cid, *ret = FS_NULL;
	FS_MmsEncEntry *pEntry = FS_NULL;
	
	if( pMmsFile->smil )
	{
		cid = FS_SmilGetVideoCid( pMmsFile->smil, num );
		if( cid )
		{
			pEntry = FS_MmsFileGetEntryByCid( pMmsFile, cid );
		}
	}
	else	/* no smil mms */
	{
		pEntry = FS_MmsFileGetEntryByIdx( pMmsFile, FS_MIME_VIDEO, num );
	}
	
	if( pEntry )
	{
		if( pEntry->content_location )
			ret = pEntry->content_location;
		else if( pEntry->param_name )
			ret = pEntry->param_name;
		else
			ret = FS_GetFileNameFromPath( pEntry->file );
	}
	return ret;
}

FS_MmsFile * FS_MmsFileDuplicate( FS_MmsFile *pSrcMms )
{
	FS_MmsFile *pDstMms;
	FS_SINT4 i, nFrames;
	FS_CHAR *cid, *mfile, *dstFile, tfile[FS_FILE_NAME_LEN];
	
	pDstMms = FS_CreateMmsFile( );
	if( pDstMms )
	{
		FS_COPY_TEXT( pDstMms->data.head.subject, pSrcMms->data.head.subject );
		nFrames = FS_MmsFileGetFrameCount( pSrcMms );
		for( i = 1; i <= nFrames; i ++ )
		{
			FS_SmilAddMmsFrame( pDstMms->smil, 0 );
			/* frame text file */
			mfile = FS_MmsFileGetFrameTextFile( pSrcMms, i );
			if( mfile )
			{
				dstFile = FS_MmsFileGetFrameTextFileName( pSrcMms, i );
				if( IFS_Strstr( mfile, dstFile ) )
				{
					/* handle of filename duplication */
					IFS_Strcpy( tfile, "X" );
					IFS_Strcat( tfile, dstFile );
					dstFile = tfile;
				}
				
				FS_FileCopy( -1, mfile, FS_DIR_TMP, dstFile );
				cid = FS_MmsCodecCreateEntry( &pDstMms->data, dstFile, -1 );
				FS_SmilAddFrameText( pDstMms->smil, i, cid );
			}
			/* frame image/video file */
			mfile = FS_MmsFileGetFrameImageFile( pSrcMms, i );
			if( mfile )
			{
				dstFile = FS_MmsFileGetFrameImageFileName( pSrcMms, i );
				if( IFS_Strstr( mfile, dstFile ) )
				{
					/* handle of filename duplication */
					IFS_Strcpy( tfile, "X" );
					IFS_Strcat( tfile, dstFile );
					dstFile = tfile;
				}
				
				FS_FileCopy( -1, mfile, FS_DIR_TMP, dstFile );
				cid = FS_MmsCodecCreateEntry( &pDstMms->data, dstFile, -1 );
				FS_SmilAddFrameImage( pDstMms->smil, i, cid );
			}
			/* frame audio file */
			mfile = FS_MmsFileGetFrameAudioFile( pSrcMms, i );
			if( mfile )
			{
				dstFile = FS_MmsFileGetFrameAudioFileName( pSrcMms, i );
				if( IFS_Strstr( mfile, dstFile ) )
				{
					/* handle of filename duplication */
					IFS_Strcpy( tfile, "X" );
					IFS_Strcat( tfile, dstFile );
					dstFile = tfile;
				}
				
				FS_FileCopy( -1, mfile, FS_DIR_TMP, dstFile );
				cid = FS_MmsCodecCreateEntry( &pDstMms->data, dstFile, -1 );
				FS_SmilAddFrameAudio( pDstMms->smil, i, cid );
			}
			/* frame dur */
			if( pSrcMms->smil )
			{
				FS_SmilSetFrameDur( pDstMms->smil,i, FS_SmilGetFrameDur( pSrcMms->smil, i) );
			}
		}
	}

	return pDstMms;
}

FS_SINT4 FS_MmsFileGetFrameCount( FS_MmsFile *pMmsFile )
{
	FS_SINT4 nTxt, nImg, nAdo, nVdo, ret;
	if( pMmsFile->smil )
	{
		ret = FS_SmilGetFrameCount( pMmsFile->smil );
	}
	else
	{
		nTxt = FS_MmsFileGetEntryCountByType( pMmsFile, FS_MIME_TEXT );
		nImg = FS_MmsFileGetEntryCountByType( pMmsFile, FS_MIME_IMAGE );
		nAdo = FS_MmsFileGetEntryCountByType( pMmsFile, FS_MIME_AUDIO );
		nVdo = FS_MmsFileGetEntryCountByType( pMmsFile, FS_MIME_VIDEO );
		ret = FS_MAX( nTxt, nImg );
		ret = FS_MAX( ret, nAdo );
		ret = FS_MAX( ret, nVdo );
	}
	return ret;
}

FS_BOOL FS_MmsFileGetReadReportFlag( FS_MmsFile *pMmsFile )
{
	return (FS_BOOL)( pMmsFile->data.head.read_report == FS_MMS_H_V_READ_REPORT_YES );
}

void FS_MmsFileSetReadReportFlag( FS_MmsFile *pMmsFile, FS_BOOL bSet )
{
	if( bSet )
		pMmsFile->data.head.read_report = FS_MMS_H_V_READ_REPORT_YES;
	else
		pMmsFile->data.head.read_report = FS_MMS_H_V_READ_REPORT_NO;
}

FS_BOOL FS_MmsFileGetDlvReportFlag( FS_MmsFile *pMmsFile )
{
	return (FS_BOOL)( pMmsFile->data.head.delivery_report == FS_MMS_H_V_DELIVERY_REPORT_YES );
}

void FS_MmsFileSetDlvReportFlag( FS_MmsFile *pMmsFile, FS_BOOL bSet )
{
	if( bSet )
		pMmsFile->data.head.delivery_report = FS_MMS_H_V_DELIVERY_REPORT_YES;
	else
		pMmsFile->data.head.delivery_report = FS_MMS_H_V_DELIVERY_REPORT_NO;
}

#endif	//FS_MODULE_MMS


