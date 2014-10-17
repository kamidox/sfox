#ifndef _FS_EML_FILE_H_
#define _FS_EML_FILE_H_

#include "inc\FS_Config.h"
#include "inc\util\FS_File.h"
#include "inc\inte\FS_Inte.h"
#include "inc\util\FS_List.h"
#include "inc\eml\FS_EmlMain.h"
#include "inc\util\FS_Util.h"
#include "inc\util\FS_Mime.h"
#include "inc\eml\FS_EmlAct.h"

#define FS_EmlInbox	0
#define FS_EmlLocal 1
#define FS_EmlOutbox 2
#define FS_EmlDraft 3

typedef struct FS_EmlAddr_Tag
{
	FS_List 		list;
	FS_CHAR 		name[FS_EML_NAME_LEN];
	FS_CHAR 		addr[FS_EML_ADDR_LEN];
}FS_EmlAddr;

// email head infomation, for list
typedef struct FS_EmlHead_Tag
{
	FS_List			list;
	FS_UINT1		mbox;
	FS_CHAR	*		subject;
	FS_EmlAddr		from;
	FS_SINT4		msg_size;
	
	FS_DateTime		date;
	FS_BOOL			read;	/* read or unread */
	FS_BOOL			local;	/* on local or remote server */

	FS_CHAR			file[FS_FILE_NAME_LEN];
	FS_CHAR			uid[FS_EML_UID_LEN];
}FS_EmlHead;

// email data information
typedef struct FS_EmlFile_Tag
{
	FS_List			list;
	FS_CHAR *		subject;
	FS_EmlAddr		from;
	FS_List			to;			/* store FS_EmlAddr structure */
	FS_List			cc;
	FS_List			bcc;
	FS_DateTime		date;
	
	FS_CHAR *		text;			// for display, email text
	FS_MimeEntry	body;			// email body mime entry
}FS_EmlFile;

// email mailbox infomation
typedef struct FS_EmlBoxInfo_Tag
{
	FS_SINT4 		total;
	FS_SINT4		unread;
	FS_SINT4		size;
	FS_BOOL			up_to_date;		// sync?
}FS_EmlBoxInfo;

FS_EmlBoxInfo * FS_EmlGetMailBoxInfo( FS_UINT1 mbox );

FS_BOOL FS_EmlIsFull( FS_SINT4 add_size );

void FS_EmlGetActSizeDetail( FS_SINT4 *total_size, FS_SINT4 *total_item );

void FS_EmlFileSave( FS_UINT1 mbox, FS_EmlFile *emlFile, FS_EmlHead *head );

void FS_EmlAddHead( FS_UINT1 mbox, FS_EmlHead *head );

void FS_EmlDelHead( FS_UINT1 mbox, FS_CHAR *uid );

FS_EmlHead *FS_EmlGetHead( FS_UINT1 mbox, FS_CHAR *uid );

void FS_EmlSetReaded( FS_UINT1 mbox, FS_CHAR *uid, FS_BOOL bRead );

void FS_EmlMoveHead( FS_EmlHead *head, FS_UINT1 smbox, FS_UINT1 dmbox );

void FS_EmlDeleteFile( FS_UINT1 mbox, FS_CHAR *uid, FS_BOOL b_save_uidl );

void FS_EmlClearBox( FS_EmlAccount *act, FS_UINT1 mbox );

void FS_EmlDeleteAll( FS_UINT1 mbox );

FS_List * FS_EmlGetHeadList( FS_EmlAccount *act, FS_UINT1 mbox );

void FS_EmlDeinitHead( void );

void FS_EmlResetMBoxInfo( void );

FS_BOOL FS_EmlCheckLocalUid( FS_CHAR *uid );

void FS_EmlUpdateLocalUidl( FS_List *server_uidl );

void FS_EmlPushPending( FS_CHAR *uid );

FS_CHAR *FS_EmlPopPending( void );

FS_SINT4 FS_EmlGetPendingCount( void );

void FS_EmlRemovePending( FS_CHAR *uid );

void FS_EmlClearPending( void );

void FS_EmlMoveMBox( FS_UINT1 sbox, FS_UINT1 dbox, FS_CHAR *uid );

#endif
