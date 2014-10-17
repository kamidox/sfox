#include "inc/FS_Config.h"

#ifdef FS_MODULE_MMS

#include "inc/mms/FS_MmsList.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_MemDebug.h"

#define FS_MMS_LIST_FILE	"MmsList.bin"

static FS_List GFS_MmsHeadList = { &GFS_MmsHeadList, &GFS_MmsHeadList };

static FS_SINT4 FS_MmsHeadLen( FS_MmsHead *head )
{
	FS_SINT4 len = 1;
	if( head->msg_location )
		len += IFS_Strlen( head->msg_location );
	return len + sizeof(FS_MmsHead);
}

static FS_SINT4 FS_MmsHeadFromBuffer( FS_BYTE *buf, FS_MmsHead *head )
{
	FS_SINT4 len = 1, size = sizeof(FS_MmsHead);
	
	IFS_Memcpy( head, buf, size );
	if( buf[size] != '\0' )
	{
		head->msg_location = IFS_Strdup( buf + size );
		len += IFS_Strlen( buf + size );
	}
	else
	{
		head->msg_location = FS_NULL;
	}
	return size + len;
}

static FS_SINT4 FS_MmsHeadToBuffer( FS_BYTE *buf, FS_MmsHead *head )
{
	FS_SINT4 len = 1, size = sizeof(FS_MmsHead);
	IFS_Memcpy( buf, head, size );
	if( head->msg_location )
	{
		len += IFS_Strlen( head->msg_location );
		IFS_Memcpy( buf + size, head->msg_location, len );
	}
	else
	{
		buf[size] = '\0';
	}
	return size + len;
}

/**
	read mms list to GFS_MmsheadList
*/
static void FS_MmsReadHead( void )
{
	FS_Handle hFile;
	FS_BYTE *buf, *pos;
	FS_UINT4 i;
	FS_MmsHead *head;

	FS_MmsHeadDeinit( );
	
	if( FS_FileOpen( &hFile, FS_DIR_MMS, FS_MMS_LIST_FILE, FS_OPEN_READ ) )
	{
		FS_SINT4 size = IFS_FileGetSize( hFile );
		if( size > 0 )
		{
			buf = IFS_Malloc( size );
			if( buf )
			{
				if( IFS_FileRead( hFile, buf, size ) == size )
				{
					pos = buf;
					for( i = 0; (pos - buf) < size; i ++ ) 
					{
						head = IFS_Malloc( sizeof(FS_MmsHead) );
						if( head )
						{
							pos += FS_MmsHeadFromBuffer( pos, head );
							FS_ListAddTail( &GFS_MmsHeadList, &head->list );
						}
					}
				}
				IFS_Free( buf );
			}
		}
		IFS_FileClose( hFile );
	}
}

static void FS_MmsWriteHead( void )
{
	FS_Handle hFile;
	FS_List *node;
	FS_MmsHead * head;
	FS_BYTE *buf, *pos;
	FS_SINT4 size = 0;
	
	if( FS_FileCreate( &hFile, FS_DIR_MMS, FS_MMS_LIST_FILE, FS_OPEN_WRITE ) )
	{
		buf = IFS_Malloc( FS_FILE_BLOCK );
		if( buf )
		{
			pos = buf;
			node = GFS_MmsHeadList.next;
			while( node != &GFS_MmsHeadList )
			{
				head = FS_ListEntry( node, FS_MmsHead, list );
				size = (FS_SINT4)(pos - buf);
				if( size + FS_MmsHeadLen(head) >= FS_FILE_BLOCK )
				{
					IFS_FileWrite( hFile, buf, size );
					pos = buf;
				}
				pos += FS_MmsHeadToBuffer( pos, head );
				node = node->next;
			}
			size = (FS_SINT4)(pos - buf);
			if( size > 0 )
				IFS_FileWrite( hFile, buf, size );
			IFS_Free( buf );
		}
		IFS_FileClose( hFile );
	}
}

void FS_MmsAddHead( FS_MmsHead *mms )
{
	FS_List *head = FS_MmsGetHeadList( );

	FS_ListAdd( head, &mms->list );
}

void FS_MmsDelHead( FS_MmsHead *mms )
{
	FS_ListDel( &mms->list );
	if( mms->file[0] )
		FS_FileDelete( FS_DIR_MMS, mms->file );
	FS_SAFE_FREE( mms->msg_location );
	IFS_Free( mms );

	FS_MmsWriteHead( );
}

void FS_MmsMoveTop( FS_MmsHead *mms )
{	
	FS_List *head;
	
	if( mms == FS_NULL ) return;
	head = FS_MmsGetHeadList( );
	FS_ListDel( &mms->list );
	FS_ListAdd( head, &mms->list );
}

void FS_MmsDelHeadBox( FS_UINT1 mbox )
{
	FS_List *node, *head = FS_MmsGetHeadList( );
	FS_MmsHead *mms;
	
	node = head->next;
	while( node != head )
	{
		mms = FS_ListEntry( node, FS_MmsHead, list );
		node = node->next;
		
		if( mms->mbox == mbox )
		{
			FS_ListDel( &mms->list );
			if( mms->file[0] )
				FS_FileDelete( FS_DIR_MMS, mms->file );
			FS_SAFE_FREE( mms->msg_location );
			IFS_Free( mms );
		}
	}

	FS_MmsWriteHead( );
}

void FS_MmsDelAllFolder( void )
{
	FS_List *node, *head = FS_MmsGetHeadList( );
	FS_MmsHead *mms;
	
	node = head->next;
	while( node != head )
	{
		mms = FS_ListEntry( node, FS_MmsHead, list );
		node = node->next;
		
		FS_ListDel( &mms->list );
		if( mms->file[0] )
			FS_FileDelete( FS_DIR_MMS, mms->file );
		FS_SAFE_FREE( mms->msg_location );
		IFS_Free( mms );
	}
	
	FS_MmsWriteHead( );
}

void FS_MmsSaveHeadList( void )
{
	FS_MmsWriteHead( );
}

void FS_MmsHeadDeinit( void )
{
	FS_MmsHead *head;
	FS_List *node = GFS_MmsHeadList.next;
	
	while( node != &GFS_MmsHeadList )
	{
		head = FS_ListEntry( node, FS_MmsHead, list );
		node = node->next;
		
		FS_ListDel( &head->list );
		FS_SAFE_FREE( head->msg_location );
		IFS_Free( head );
	}
}

FS_List * FS_MmsGetHeadList( void )
{
	if( FS_ListIsEmpty(&GFS_MmsHeadList) )
		FS_MmsReadHead( );

	return &GFS_MmsHeadList;
}

void FS_MmsGetFolderDetail( FS_UINT1 mbox, FS_SINT4 *pUnread, FS_SINT4 *pTotal )
{
	FS_SINT4 unread = 0, total = 0;
	FS_MmsHead *mms;
	FS_List *node, *head = FS_MmsGetHeadList( );

	node = head->next;
	while( node != head )
	{
		mms = FS_ListEntry( node, FS_MmsHead, list );
		node = node->next;
		
		if( mms->mbox == mbox )
		{
			total ++;
			if( mbox == FS_MMS_INBOX && FS_MMS_UNREAD(mms) )
				unread ++;
		}
	}

	*pUnread = unread;
	*pTotal = total;
}

void FS_MmsGetSpaceDetail( FS_SINT4 *pTotal, FS_SINT4 *pSize )
{
	FS_SINT4 size = 0, total = 0;
	FS_MmsHead *mms;
	FS_List *node, *head = FS_MmsGetHeadList( );
	
	node = head->next;
	while( node != head )
	{
		mms = FS_ListEntry( node, FS_MmsHead, list );
		node = node->next;
		
		total ++;
		if( pSize && ! FS_MMS_IS_NTF(mms) )
		{
			if( mms->msg_size <= 0 )
			{
				mms->msg_size = FS_FileGetSize( FS_DIR_MMS, mms->file );
			}
			size += mms->msg_size;
		}
	}
	
	if( pSize ) *pSize = size;
	*pTotal = total;
}

void FS_MmsHeadSaveAsTemplate( FS_MmsHead *mms, FS_CHAR *file )
{
	FS_MmsHead *pTmp;

	if( FS_MMS_IS_NTF(mms) )
		return;
	
	pTmp = FS_NEW( FS_MmsHead );
	if( pTmp )
	{
		IFS_Memcpy( pTmp, mms, sizeof(FS_MmsHead) );
		pTmp->mbox = FS_MMS_TEMPLATE;
		pTmp->address[0];
		
		IFS_Strcpy( pTmp->file, file );
		FS_MMS_SET_UNREAD( pTmp );
		pTmp->msg_location = FS_NULL;

		FS_MmsAddHead( pTmp );
		FS_MmsSaveHeadList( );
	}
}

FS_MmsHead *FS_MmsFindByMsgLocation( FS_CHAR *message_location )
{
	FS_MmsHead *mms;
	FS_List *node, *head = FS_MmsGetHeadList( );

	node = head->next;
	while( node != head )
	{
		mms = FS_ListEntry( node, FS_MmsHead, list );
		node = node->next;

		if( FS_MMS_IS_NTF(mms) )
		{
			if( FS_STR_I_EQUAL(mms->msg_location, message_location) )
			{
				return mms;
			}
		}
	}
	return FS_NULL;
}

FS_BOOL FS_MmsIsFull( FS_SINT4 add_size )
{
	FS_SINT4 total, size;
	FS_MmsGetSpaceDetail( &total, &size );
	if( size + add_size >= IFS_GetMaxMmsSizeLimit() )
	{
		return FS_TRUE;
	}
	if( total >= IFS_GetMaxMmsItemLimit() )
	{
		return FS_TRUE;
	}
	return FS_FALSE;
}

#endif	//FS_MODULE_MMS


