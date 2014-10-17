#include "inc/FS_Config.h"

#ifdef FS_MODULE_EML

#include "inc/eml/FS_EmlFile.h"
#include "inc/util/FS_Base64.h"
#include "inc/util/FS_Mime.h"
#include "inc/eml/FS_Rfc822.h"
#include "inc/eml/FS_pop3.h"
#include "inc/util/FS_MemDebug.h"

typedef struct FS_EmlHeadList_Tag
{
	FS_UINT1		mbox;
	FS_List			head;
}FS_EmlHeadList;

typedef struct FS_EmlUidl_Tag
{
	FS_List			list;
	FS_CHAR			uid[FS_EML_UID_LEN];
}FS_EmlUidl;

static FS_EmlBoxInfo GFS_EmlMBox[4] = 
{ 
	{ 0, 0, FS_FALSE }, 
	{ 0, 0, FS_FALSE }, 
	{ 0, 0, FS_FALSE }, 
	{ 0, 0, FS_FALSE } 
};
/* email head list, cache a mail box here */
static FS_EmlHeadList GFS_EmlHead = { 0xff, &GFS_EmlHead.head, &GFS_EmlHead.head };
/* pending list. means that its check not local and about to retrive from server */
static FS_List GFS_EmlPendingList = { &GFS_EmlPendingList, &GFS_EmlPendingList };
static FS_List GFS_EmlUidl = { &GFS_EmlUidl, &GFS_EmlUidl };

static void FS_FreeEmlHead( FS_EmlHead *head )
{
	if( head->subject )
		IFS_Free( head->subject );
	IFS_Free( head );
}

static FS_SINT4 FS_EmlHeadLen( FS_EmlHead *head )
{
	FS_SINT4 len = 1;
	if( head->subject )
		len += IFS_Strlen( head->subject );
	return len + sizeof(FS_EmlHead);
}

static FS_SINT4 FS_EmlHeadFromBuffer( FS_BYTE *buf, FS_EmlHead *head )
{
	FS_SINT4 len = 1, size = sizeof(FS_EmlHead);
	
	IFS_Memcpy( head, buf, size );
	if( buf[size] != '\0' )
	{
		head->subject = IFS_Strdup( buf + size );
		len += IFS_Strlen( buf + size );
	}
	else
	{
		head->subject = FS_NULL;
	}
	return size + len;
	
}

static FS_SINT4 FS_EmlHeadToBuffer( FS_BYTE *buf, FS_EmlHead *head )
{
	FS_SINT4 len = 1, size = sizeof(FS_EmlHead);
	IFS_Memcpy( buf, head, size );
	if( head->subject )
	{
		len += IFS_Strlen( head->subject );
		IFS_Memcpy( buf + size, head->subject, len );
	}
	else
	{
		buf[size] = '\0';
	}
	return size + len;
}

/**
	read current active email account's mail box mail list
	all mail head is cache in GFS_EmlHead
	@param[in]		mbox	mail box index
*/
static void FS_EmlReadHead( FS_EmlAccount *act, FS_UINT1 mbox )
{
	FS_List *node;
	FS_BYTE *buf, *pos;
	FS_UINT4 i;
	FS_EmlHead *head;
	FS_SINT4 size;
	
	if( act == FS_NULL ) act = FS_EmlGetActiveAct( );
	if( act == FS_NULL ) return;

	FS_ASSERT( mbox <= FS_EmlDraft );
	if( mbox > FS_EmlDraft ) return;
	
	GFS_EmlMBox[mbox].total = 0;
	GFS_EmlMBox[mbox].unread = 0;
	GFS_EmlMBox[mbox].size = 0;
	GFS_EmlMBox[mbox].up_to_date = FS_TRUE;
	// free the old list first
	node = GFS_EmlHead.head.next;
	while( node != &GFS_EmlHead.head )
	{
		head = FS_ListEntry( node, FS_EmlHead, list );
		node = node->next;
		FS_ListDel( &head->list );
		FS_FreeEmlHead( head );
	}
	
	GFS_EmlHead.mbox = mbox;
	FS_ListInit( &GFS_EmlHead.head );

	size = FS_FileGetSize( FS_DIR_EML, act->mbox_file[mbox] );
	if( size <= 0 ) return;

	buf = IFS_Malloc( size );
	FS_ASSERT( buf != FS_NULL );
	if( buf == FS_NULL ) return;

	if( FS_FileRead( FS_DIR_EML, act->mbox_file[mbox], 0, buf, size ) != size )
	{
		IFS_Free( buf );
		return;
	}

	pos = buf;
	for( i = 0; (pos - buf) < size; i ++ ) 
	{
		head = IFS_Malloc( sizeof(FS_EmlHead) );
		if( head )
		{
			pos += FS_EmlHeadFromBuffer( pos, head );
			GFS_EmlMBox[mbox].size += head->msg_size;
			if( head->read == FS_FALSE )
				GFS_EmlMBox[mbox].unread ++;
			FS_ListAddTail( &GFS_EmlHead.head, &head->list );
		}
	}
	// get the new mbox infomation
	GFS_EmlMBox[mbox].total = i;

	IFS_Free( buf );
}

/*
	write GFS_EmlHead list to current account's mail list file
*/
static void FS_EmlWriteHead( void )
{
	FS_Handle hFile;
	FS_List *node;
	FS_EmlHead * emlHead;
	FS_BYTE *buf, *pos;
	FS_SINT4 size = 0;
	FS_EmlAccount *act = FS_EmlGetActiveAct( );
	FS_UINT1 mbox;

	if( act == FS_NULL ) return;
	
	node = GFS_EmlHead.head.next;
	mbox = GFS_EmlHead.mbox;
	if( mbox >= 4 ) return;
	
	if( FS_FileCreate( &hFile, FS_DIR_EML, act->mbox_file[mbox], FS_OPEN_WRITE ) )
	{
		buf = IFS_Malloc( FS_FILE_BLOCK );
		if( buf )
		{
			pos = buf;
			while( node != &GFS_EmlHead.head )
			{
				emlHead = FS_ListEntry( node, FS_EmlHead, list );
				size = (FS_SINT4)(pos - buf);
				if( size + FS_EmlHeadLen(emlHead) >= FS_FILE_BLOCK )
				{
					IFS_FileWrite( hFile, buf, size );
					pos = buf;
				}
				pos += FS_EmlHeadToBuffer( pos, emlHead );
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

/**
	retrieve a email head list, it will retrieve from cache list first.
	if wanted head list is not in cache, will read it from email list file.
	@param[in]		mbox		mail box to retrive
	
	@return			mail head list
*/
FS_List * FS_EmlGetHeadList( FS_EmlAccount *act, FS_UINT1 mbox )
{
	if( GFS_EmlHead.mbox != mbox )
		FS_EmlReadHead( act, mbox );
	return &GFS_EmlHead.head;
}

/*
	add a email head to mail box
	@param[in]		mbox		mail box to add in
	@param[in]		head		mail head information
*/
void FS_EmlAddHead( FS_UINT1 mbox, FS_EmlHead *head )
{
	FS_List *hlist = FS_EmlGetHeadList( FS_NULL, mbox );
	FS_EmlHead *emlhead = IFS_Malloc( sizeof(FS_EmlHead) );
	if( emlhead )
	{
		IFS_Memcpy( emlhead, head, sizeof(FS_EmlHead) );
		if( head->subject )
			emlhead->subject = IFS_Strdup( head->subject );
		emlhead->mbox = mbox;
		FS_ListAdd( &GFS_EmlHead.head, &emlhead->list );
		GFS_EmlMBox[mbox].total ++;
		GFS_EmlMBox[mbox].size += head->msg_size;
		if( ! emlhead->read )
			GFS_EmlMBox[mbox].unread ++;
		FS_EmlWriteHead( );
	}
}

/*
	delete a email head from mbox
	@param[in]		mbox		mail box to delete
	@param[in]		uid			email uid
*/
void FS_EmlDelHead( FS_UINT1 mbox, FS_CHAR *uid )
{
	FS_EmlHead *entry;
	FS_List *node, *lhead;

	if( uid == FS_NULL ) return;
	
	lhead = FS_EmlGetHeadList( FS_NULL, mbox );
	node = lhead->next;
	while( node != lhead )
	{
		entry = FS_ListEntry( node, FS_EmlHead, list );
		node = node->next;
		/* here, we must campare the uid, not the address */
		if( ! IFS_Strcmp(uid, entry->uid) )
		{
			GFS_EmlMBox[mbox].total --;
			GFS_EmlMBox[mbox].size -= entry->msg_size;
			if( ! entry->read )
				GFS_EmlMBox[entry->mbox].unread --;
			FS_ListDel( &entry->list );
			FS_FreeEmlHead( entry );
			break;
		}
	}
	FS_EmlWriteHead( );
}

/*
	get a email head from mail box
	@param 		mbox 		mail box id
	@param		uid			mail uid

	@return		email head data. this data is in cache list. caller must copy it.
				do not reference it or when cache changed. this can be dangerous.
*/
FS_EmlHead *FS_EmlGetHead( FS_UINT1 mbox, FS_CHAR *uid )
{
	FS_EmlHead *entry;
	FS_List *node, *lhead;

	if( uid == FS_NULL ) return FS_NULL;
	
	lhead = FS_EmlGetHeadList( FS_NULL, mbox );
	node = lhead->next;
	while( node != lhead )
	{
		entry = FS_ListEntry( node, FS_EmlHead, list );
		node = node->next;
		if( ! IFS_Strcmp(uid, entry->uid) )
		{
			return entry;
		}
	}
	return FS_NULL;
}

void FS_EmlSetReaded( FS_UINT1 mbox, FS_CHAR *uid, FS_BOOL bRead )
{
	FS_EmlHead *emlHead = FS_EmlGetHead( mbox, uid );
	if( emlHead )
	{
		emlHead->read = bRead;
		if( bRead )
			GFS_EmlMBox[mbox].unread --;
		else
			GFS_EmlMBox[mbox].unread ++;
		FS_EmlWriteHead( );
	}
}

/*
	move a email head from smbox to dmbox
	@param [in]		head		mail head information
	@param [in]		smbox		source mail box id
	@param [in]		dmbox		desternation mail box id
*/
void FS_EmlMoveHead( FS_EmlHead *head, FS_UINT1 smbox, FS_UINT1 dmbox )
{
	FS_EmlHead back;
	IFS_Memcpy( &back, head, sizeof(FS_EmlHead) );
	if( head->subject )
		back.subject = IFS_Strdup( head->subject );
	FS_EmlDelHead( smbox, back.uid );
	FS_EmlAddHead( dmbox, &back );
	if( back.subject )
		IFS_Free( back.subject );
}

/*
	get mail box information
	@param [in] 	mbox		mail box id
	@param [out]	total		total mails in this mail box
	@param [out]	unread		unread mails in this mail box
*/
FS_EmlBoxInfo * FS_EmlGetMailBoxInfo( FS_UINT1 mbox )
{
	if( ! GFS_EmlMBox[mbox].up_to_date )
		FS_EmlReadHead( FS_NULL, mbox );
	return &GFS_EmlMBox[mbox];
}

/*
	save a email into mail box. this mail data will encode into rfc822 format.
	@param [in] 	mbox		mail box to save to
	@param [in]		emlFile		mail data, may contain some attach file info
	@param [in/out]	head		email head information. cannot be NULL
			
	if strlen(head->file) is zero, this function will add new mail to mail box
	if strlen(head->file) is not zero, this function will update the exist one.
*/
void FS_EmlFileSave( FS_UINT1 mbox, FS_EmlFile *emlFile, FS_EmlHead *head )
{
	FS_BOOL bNew = FS_TRUE;
	FS_SINT4 org_size;
	FS_EmlAccount *act = FS_EmlGetActiveAct( );
	
	if( act && emlFile )
	{	
		if( head->file[0] != 0 ) bNew = FS_FALSE;
		
		FS_PackRfc822Message( emlFile, head->file );
		if( head->file[0] != 0 )
		{
			IFS_Memcpy( &head->from, &emlFile->from, sizeof(FS_EmlAddr) );
			FS_COPY_TEXT( head->subject, emlFile->subject );
			head->mbox = mbox;
			org_size = head->msg_size;
			head->msg_size = FS_FileGetSize( FS_DIR_EML, head->file );
			/* if this is new mail, add it to mail box head list */
			if( bNew )
			{
				/* update the status and modify time */
				head->read = FS_FALSE;
				IFS_GetDateTime( &head->date );
				FS_GetGuid( head->uid );	/* here, we gen a local UID */
				FS_EmlAddHead( mbox, head );
			}
			else
			{
				FS_EmlWriteHead( );
				GFS_EmlMBox[mbox].size += (head->msg_size - org_size);
			}
		}
	}
}

static void FS_EmlReadUidl( void )
{
	FS_BYTE *buf, *pos;
	FS_SINT4 size, total, i;
	FS_EmlAccount *act = FS_EmlGetActiveAct( );
	FS_EmlUidl *pUidl;
	
	size = FS_FileGetSize( FS_DIR_EML, act->uidl );
	if( size <= 0 ) return;

	buf = IFS_Malloc( size );
	FS_ASSERT( buf != FS_NULL );
	if( FS_FileRead( FS_DIR_EML, act->uidl, 0, buf, size ) != size )
	{
		IFS_Free( buf );
		return;
	}

	total = size / sizeof(FS_EmlUidl);
	pos = buf;
	for( i = 0; i < total; i ++ )
	{
		pUidl = IFS_Malloc( sizeof(FS_EmlUidl) );
		if( pUidl )
		{
			IFS_Memcpy( pUidl, pos, sizeof(FS_EmlUidl) );
			FS_ListAdd( &GFS_EmlUidl, &pUidl->list );
			pos += sizeof(FS_EmlUidl);
		}
	}
	IFS_Free( buf );
}

static void FS_EmlWriteUidl( void )
{
	FS_BYTE *pBuf = FS_NULL, *pos;
	FS_SINT4 nSize, i = 0;
	FS_List *node;
	FS_EmlUidl *pUidl;
	FS_EmlAccount *act = FS_EmlGetActiveAct( );
	
	i = FS_ListCount( &GFS_EmlUidl );
	if( i > 0 )
	{
		nSize = i * sizeof(FS_EmlUidl);
		pBuf = IFS_Malloc( nSize );
	}
	else
	{
		FS_FileDelete( FS_DIR_EML, act->uidl );
	}
	
	if( pBuf )
	{
		pos = pBuf;
		node = GFS_EmlUidl.next;
		while( node != &GFS_EmlUidl )
		{
			pUidl = FS_ListEntry( node, FS_EmlUidl, list );
			node = node->next;
			IFS_Memcpy( pos, pUidl, sizeof(FS_EmlUidl));
			pos += sizeof(FS_EmlUidl);
		}
		FS_FileWrite( FS_DIR_EML, act->uidl, 0, pBuf, nSize );
		IFS_Free( pBuf );
	}
}

static FS_List *FS_EmlGetUidl( void )
{
	if( FS_ListIsEmpty( &GFS_EmlUidl ) )
	{
		FS_EmlReadUidl( );
	}
	return &GFS_EmlUidl;
}

static void FS_EmlSaveUidl( FS_CHAR *uid )
{
	FS_EmlUidl *pUidl;
	FS_List *head = FS_EmlGetUidl( );

	pUidl = IFS_Malloc( sizeof(FS_EmlUidl) );
	FS_ASSERT( pUidl != FS_NULL );
	if( pUidl == FS_NULL ) return;
	
	IFS_Memset( pUidl, 0, sizeof(FS_EmlUidl) );
	IFS_Strncpy( pUidl->uid, uid, sizeof(pUidl->uid) - 1 );
	FS_ListAddTail( head, &pUidl->list );
}

static void FS_EmlDeinitUidl( void )
{
	FS_EmlUidl *pUidl;
	FS_List *node = GFS_EmlUidl.next;
	while( node != &GFS_EmlUidl )
	{
		pUidl = FS_ListEntry( node, FS_EmlUidl, list );
		node = node->next;
		FS_ListDel( &pUidl->list );
		IFS_Free( pUidl );
	}
}

/*
	delete a email file from mail box. it will delete the mail head and mail file
	@param [in] 	mbox		mail box to save to
	@param [in]		uid			mail uid
*/
void FS_EmlDeleteFile( FS_UINT1 mbox, FS_CHAR *uid, FS_BOOL b_save_uidl )
{
	FS_EmlHead *head = FS_EmlGetHead( mbox, uid );
	if( head )
	{
		FS_FileDelete( FS_DIR_EML, head->file );
		FS_EmlDelHead( mbox, uid );
		if( b_save_uidl )
		{
			FS_EmlSaveUidl( uid );			
			FS_EmlWriteUidl( );
		}
	}
}

void FS_EmlDeleteAll( FS_UINT1 mbox )
{
	FS_EmlAccount *act = FS_EmlGetActiveAct( );
	FS_List *headlist, *node;
	FS_EmlHead *pHead;
	
	FS_ASSERT( mbox <= FS_EmlDraft );

	if( mbox == FS_EmlInbox || mbox == FS_EmlLocal )
	{
		headlist = FS_EmlGetHeadList( act, mbox );
		node = headlist->next;
		while( node != headlist )
		{
			pHead = FS_ListEntry( node, FS_EmlHead, list );
			node = node->next;

			FS_EmlSaveUidl( pHead->uid );
		}

		FS_EmlWriteUidl( );
	}

	FS_EmlClearBox( act, mbox );
}

/*
	delete a mailbox, include all mail file in this mail box
	@param [in]		act			email account
	@param [in]		mbox		which mail box to delete
*/
void FS_EmlClearBox( FS_EmlAccount *act, FS_UINT1 mbox )
{
	FS_EmlHead *emlHead;
	FS_List *node;
	FS_EmlReadHead( act, mbox );
	node = GFS_EmlHead.head.next;
	while( node != &GFS_EmlHead.head )
	{
		emlHead = FS_ListEntry( node, FS_EmlHead, list );
		node = node->next;
		FS_FileDelete( FS_DIR_EML, emlHead->file );
		FS_ListDel( &emlHead->list );
		FS_FreeEmlHead( emlHead );
	}
	GFS_EmlHead.mbox = 0xFF;
	FS_ListInit( &GFS_EmlHead.head );
	GFS_EmlMBox[mbox].total = 0;
	GFS_EmlMBox[mbox].unread = 0;
	GFS_EmlMBox[mbox].size = 0;	
	FS_FileDelete( FS_DIR_EML, act->mbox_file[mbox ] );
}


void FS_EmlResetMBoxInfo( void )
{
	GFS_EmlMBox[FS_EmlInbox].up_to_date = FS_FALSE;
	GFS_EmlMBox[FS_EmlLocal].up_to_date = FS_FALSE;
	GFS_EmlMBox[FS_EmlOutbox].up_to_date = FS_FALSE;
	GFS_EmlMBox[FS_EmlDraft].up_to_date = FS_FALSE;
	GFS_EmlHead.mbox = 0xFF;

	FS_EmlDeinitUidl( );
}

/* 
	move a email from draft box to outbox
	@param [in]		sbox	src mail box
	@param [in]		dbox	dst mail box
	@param [in]		uid		mail uid
*/
void FS_EmlMoveMBox( FS_UINT1 sbox, FS_UINT1 dbox, FS_CHAR *uid )
{
	FS_EmlHead *emlHead;
	if( uid )
	{
		emlHead = FS_EmlGetHead( sbox, uid );
		if( emlHead )
			FS_EmlMoveHead( emlHead, sbox, dbox );
	}
}

/*
	check if a mail uid is exist in local, return false if not exist
*/
FS_BOOL FS_EmlCheckLocalUid( FS_CHAR *uid )
{
	FS_List *node, *head;
	FS_EmlHead *emlHead;
	FS_EmlUidl *pUidl;
	
	head = FS_EmlGetHeadList( FS_EmlGetActiveAct(), FS_EmlInbox );
	node = head->next;
	while( node != head )
	{
		emlHead = FS_ListEntry( node, FS_EmlHead, list );
		node = node->next;
		if( ! IFS_Strcmp( emlHead->uid, uid ) )
			return FS_TRUE;
	}
	
	head = FS_EmlGetHeadList( FS_EmlGetActiveAct(), FS_EmlLocal );
	node = head->next;
	while( node != head )
	{
		emlHead = FS_ListEntry( node, FS_EmlHead, list );
		node = node->next;
		if( ! IFS_Strcmp( emlHead->uid, uid ) )
			return FS_TRUE;
	}

	head = FS_EmlGetUidl( );
	node = head->next;
	while( node != head )
	{
		pUidl = FS_ListEntry( node, FS_EmlUidl, list );
		node = node->next;
		if( ! IFS_Strcmp( pUidl->uid, uid ) )
			return FS_TRUE;
	}
	
	return FS_FALSE;
}

static FS_BOOL FS_EmlUidExistInServer( FS_CHAR *uid, FS_List *server_uidl )
{
	FS_Pop3Uidl *pPop3Uidl;
	FS_List *node;

	node = server_uidl->next;
	while( node != server_uidl )
	{
		pPop3Uidl = FS_ListEntry( node, FS_Pop3Uidl, list );
		node = node->next;

		if( IFS_Strcmp( uid, pPop3Uidl->uid ) == 0 )
		{
			return FS_TRUE;
		}
	}
	return FS_FALSE;
}

void FS_EmlUpdateLocalUidl( FS_List *server_uidl )
{
	FS_EmlUidl *pUidl;
	FS_List *head, *node;
	FS_BOOL b_need_update = FS_FALSE;
	
	head = FS_EmlGetUidl( );
	node = head->next;
	while( node != head )
	{
		pUidl = FS_ListEntry( node, FS_EmlUidl, list );
		node = node->next;

		if( ! FS_EmlUidExistInServer( pUidl->uid, server_uidl ) )
		{
			b_need_update = FS_TRUE;
			FS_ListDel( &pUidl->list );
			IFS_Free( pUidl );
		}
	}

	if( b_need_update ) FS_EmlWriteUidl( );
}

/* add this eml to pending list, can fetch later to retrive from server */
void FS_EmlPushPending( FS_CHAR *uid )
{
	FS_EmlHead *head = IFS_Malloc( sizeof(FS_EmlHead) );
	if( head )
	{
		IFS_Memset( head, 0, sizeof(FS_EmlHead) );
		IFS_Strcpy( head->uid, uid );
		FS_ListAdd( &GFS_EmlPendingList, &head->list );
	}
}

/* get a pending email from pending list */
FS_CHAR *FS_EmlPopPending( void )
{
	FS_EmlHead *head;
	FS_List *node = GFS_EmlPendingList.next;
	if( node != &GFS_EmlPendingList )
	{
		head = FS_ListEntry( node, FS_EmlHead, list );
		return head->uid;
	}
	return FS_NULL;
}

FS_SINT4 FS_EmlGetPendingCount( void )
{
	return FS_ListCount( &GFS_EmlPendingList );
}


/* remove pending email from pending list, if head is not NULL, inform to copy pending list in to Inbox */
void FS_EmlRemovePending( FS_CHAR *uid )
{
	FS_EmlHead *emlhead;
	FS_List *node = GFS_EmlPendingList.next;
	while( node != &GFS_EmlPendingList )
	{
		emlhead = FS_ListEntry( node, FS_EmlHead, list );
		if( ! IFS_Strcmp( emlhead->uid, uid ) )
		{
			// remove from pending list
			FS_ListDel( &emlhead->list );
			FS_FreeEmlHead( emlhead );
			break;
		}
		node = node->next;
	}
}

void FS_EmlClearPending( void )
{
	FS_EmlHead *head;
	FS_List *node = GFS_EmlPendingList.next;
	while( node != &GFS_EmlPendingList )
	{
		head = FS_ListEntry( node, FS_EmlHead, list );
		node = node->next;
		FS_ListDel( &head->list );
		FS_FreeEmlHead( head );
	}
	
}

void FS_EmlDeinitHead( void )  
{
	FS_EmlHead *head;
	FS_List *node = GFS_EmlHead.head.next;
	while( node != &GFS_EmlHead.head )
	{
		head = FS_ListEntry( node, FS_EmlHead, list );
		node = node->next;
		FS_ListDel( &head->list );
		FS_FreeEmlHead( head );
	}
	FS_EmlClearPending( );
	FS_EmlDeinitUidl( );
	GFS_EmlMBox[FS_EmlInbox].up_to_date = FS_FALSE;
	GFS_EmlMBox[FS_EmlLocal].up_to_date = FS_FALSE;
	GFS_EmlMBox[FS_EmlOutbox].up_to_date = FS_FALSE;
	GFS_EmlMBox[FS_EmlDraft].up_to_date = FS_FALSE;
}

void FS_EmlGetActSizeDetail( FS_SINT4 *total_size, FS_SINT4 *total_item )
{
	FS_EmlBoxInfo *pInfo;

	*total_size = 0;
	*total_item = 0;
	pInfo = FS_EmlGetMailBoxInfo( FS_EmlInbox );
	*total_item += pInfo->total;
	
	pInfo = FS_EmlGetMailBoxInfo( FS_EmlLocal );
	*total_size += pInfo->size;
	*total_item += pInfo->total;

	pInfo = FS_EmlGetMailBoxInfo( FS_EmlDraft );
	*total_size += pInfo->size;
	*total_item += pInfo->total;

	pInfo = FS_EmlGetMailBoxInfo( FS_EmlOutbox );
	*total_size += pInfo->size;
	*total_item += pInfo->total;
}


FS_BOOL FS_EmlIsFull( FS_SINT4 add_size )
{
	FS_SINT4 total_size = 0, total_item = 0;

	FS_EmlGetActSizeDetail( &total_size, &total_item );

	if( (total_size + add_size) >= IFS_GetMaxEmlSizeLimit() ){
		return FS_TRUE;
	}

	if( total_item >= IFS_GetMaxEmlItemLimit() ){
		return FS_TRUE;
	}
	return FS_FALSE;
}

#endif	//FS_MODULE_EML

