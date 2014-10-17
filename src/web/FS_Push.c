#include "inc/FS_Config.h"

#ifdef FS_MODULE_WEB

#include "inc/web/FS_Push.h"
#include "inc/web/FS_Wsp.h"
#include "inc/util/FS_Mime.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_MemDebug.h"

#define FS_PUSH_LIST_FILE		"Push.bin"

void FS_NewPushMsgNotify( FS_PushMsg *pMsg );

static FS_List GFS_PushList = {&GFS_PushList, &GFS_PushList};

static void FS_PushListWriteToFile( void )
{
	FS_Handle hFile;
	FS_List *node;
	FS_PushMsg * pMsg;
	FS_BYTE *buf, *pos;
	FS_SINT4 size = 0;
	
	node = GFS_PushList.next;
	
	if( FS_FileCreate( &hFile, FS_DIR_WEB, FS_PUSH_LIST_FILE, FS_OPEN_WRITE ) )
	{
		buf = IFS_Malloc( FS_FILE_BLOCK );
		if( buf )
		{
			pos = buf;
			while( node != &GFS_PushList )
			{
				pMsg = FS_ListEntry( node, FS_PushMsg, list );
				size = (FS_SINT4)(pos - buf);
				if( size + sizeof(FS_PushMsg) >= FS_FILE_BLOCK )
				{
					IFS_FileWrite( hFile, buf, size );
					pos = buf;
				}
				IFS_Memcpy( pos, pMsg, sizeof(FS_PushMsg) );
				pos += sizeof(FS_PushMsg);
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

static void FS_PushListReadFromFile( void )
{
	FS_Handle hFile;
	FS_BYTE *buf, *pos;
	FS_UINT4 i;
	FS_PushMsg * pMsg;

	FS_PushDeinit( );
	
	if( FS_FileOpen( &hFile, FS_DIR_WEB, FS_PUSH_LIST_FILE, FS_OPEN_READ ) )
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
						pMsg = IFS_Malloc( sizeof(FS_PushMsg) );
						if( pMsg )
						{
							IFS_Memcpy( pMsg, pos, sizeof(FS_PushMsg) );
							pos += sizeof(FS_PushMsg);
							FS_ListAddTail( &GFS_PushList, &pMsg->list );
						}
					}
				}
				IFS_Free( buf );
			}
		}
		IFS_FileClose( hFile );
	}
}

void FS_PushAddItem( FS_PushMsg *pMsg )
{
	FS_SINT4 nCount;
	FS_List *head = FS_PushGetList( );

	nCount = FS_ListCount( head );

	if( nCount < IFS_GetMaxPushItemLimit() )
	{
		FS_ListAddTail( head, &pMsg->list );
		FS_PushListWriteToFile( );
		FS_NewPushMsgNotify( pMsg );
	}
	else
	{
		IFS_PushListFull( );
	}
}

void FS_PushDelItem( FS_PushMsg *pMsg )
{
	FS_ListDel( &pMsg->list );
	IFS_Free( pMsg );
	
	FS_PushListWriteToFile( );
}

void FS_PushDelAll( void )
{
	FS_PushDeinit( );
	FS_FileDelete( FS_DIR_WEB, FS_PUSH_LIST_FILE );
}

void FS_PushSaveList( void )
{
	FS_PushListWriteToFile( );
}

FS_List *FS_PushGetList( void )
{
	if( FS_ListIsEmpty(&GFS_PushList) )
		FS_PushListReadFromFile( );
	
	return &GFS_PushList;
}

void FS_PushDeinit( void )
{
	FS_PushMsg *pMsg;
	FS_List *node = GFS_PushList.next;
	while( node != &GFS_PushList )
	{
		pMsg = FS_ListEntry( node, FS_PushMsg, list );
		node = node->next;
		
		FS_ListDel( &pMsg->list );
		IFS_Free( pMsg );
	}
}

FS_BOOL FS_PushIsFull( void )
{
	FS_SINT4 total;
	FS_List *head;

	head = FS_PushGetList( );
	total = FS_ListCount( head );
	if( total >= IFS_GetMaxPushItemLimit() )
	{
		return FS_TRUE;
	}
	else
	{
		return FS_FALSE;
	}
}

FS_BOOL FS_PushHasUnread( void )
{
	FS_List *head, *node;
	FS_PushMsg *pMsg;
	
	head = FS_PushGetList( );
	FS_ListForEach( node, head ){
		pMsg = FS_ListEntry( node, FS_PushMsg, list );
		if( ! FS_PUSHMSG_GET_FLAG(pMsg, FS_PUSHMSG_READ) )
			return FS_TRUE;
	}
	return FS_FALSE;
}

/*-------------------------------- push handler ---------------------------------------------------*/
extern void FS_SIProcessData( FS_BYTE *data, FS_SINT4 len );
extern void FS_SLProcessData( FS_BYTE *data, FS_SINT4 len );
extern void FS_MmsNotificationInd( FS_UINT1 tid, FS_BYTE *data, FS_SINT4 len );

static void FS_PushHandler( void *user_data, FS_WspHandle hWsp, FS_SINT4 event, FS_UINT4 param )
{
	FS_WspPushData *pData;
	FS_BOOL isMmsNtf = FS_FALSE;
	
	if( event == FS_WSP_EV_PUSH_IND )
	{
		pData = (FS_WspPushData *)param;
		if( pData->content_type == FS_WCT_APP_VND_WAP_SIC 
			|| ( pData->mime_content && IFS_Strnicmp(pData->mime_content, "application/vnd.wap.sic", 23) == 0) )
		{
			/* service indication */
			FS_SIProcessData( pData->data, pData->len );
		}
		else if( pData->content_type == FS_WCT_APP_VND_WAP_SLC 
			|| ( pData->mime_content && IFS_Strnicmp(pData->mime_content, "application/vnd.wap.slc", 23) == 0) )
		{
			/* service loading */
			FS_SLProcessData( pData->data, pData->len );
		}
		else if( pData->content_type == FS_WCT_APP_VND_WAP_MMS 
			|| ( pData->mime_content && IFS_Strnicmp(pData->mime_content, "application/vnd.wap.mms-message", 31) == 0) )
		{
			isMmsNtf = FS_TRUE;
			FS_MmsNotificationInd( pData->tid, pData->data, pData->len );
		}
		else
		{
			IFS_PushInd( pData->tid, pData->app_id_code, pData->app_id_value, 
				pData->content_type, pData->mime_content, pData->data, pData->len );
		}
	}
}

void FS_PushInd( FS_BYTE *pdu, FS_SINT4 len )
{
	if( IFS_SystemRuningBackgroud() )
	{
		FS_SystemInit( );
	}
	FS_WspPushInd( pdu, len, FS_PushHandler );
}

#ifdef FS_DEBUG_

#define FS_PUSH_TEST_FILE	"push.dat"

void FS_PushTest( void )
{
	FS_SINT4 size;
	FS_BYTE *buf;
	
	size = FS_FileGetSize( FS_DIR_ROOT, FS_PUSH_TEST_FILE );
	if( size > 0 )
	{
		buf = IFS_Malloc( size );
		if( buf )
		{
			FS_FileRead( FS_DIR_ROOT, FS_PUSH_TEST_FILE, 0, buf, size );
			FS_PushInd( buf, size );
			IFS_Free( buf );
		}
	}
}

#endif

#endif	//FS_MODULE_WEB


