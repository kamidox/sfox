#ifndef _FS_MMS_LIST_H_
#define _FS_MMS_LIST_H_

#include "inc\FS_Config.h"
#include "inc\util\FS_List.h"
#include "inc\util\FS_File.h"

#define FS_MMS_ADDR_LEN		32
#define FS_MMS_MSG_ID_LEN	40	/* @see OMA-MMS-CONF */
#define FS_MMS_SUB_LEN		40	/* @see OMA-MMS-CONF */

#define FS_MMS_INBOX		0	/* 收件箱，存储收到的新彩信 */
#define FS_MMS_OUTBOX		1	/* 己发彩信，存储发送成功的彩信 */
#define FS_MMS_DRAFT		2	/* 草稿箱，存储编辑后暂时保存的彩信 */
#define FS_MMS_TEMPLATE		4	/* 彩信模块 */
#define FS_MMS_SENDBOX		5	/* 发件箱，存储发送失败的彩信 */

#define FS_MMSF_READ			0x01
#define FS_MMSF_NOTIFY			0x02
#define FS_MMSF_READ_REPORT		0x04

#define FS_MMS_UNREAD( mms )				(!((mms)->flag & FS_MMSF_READ))
#define FS_MMS_IS_NTF( mms )				((mms)->flag & FS_MMSF_NOTIFY )
#define FS_MMS_REQ_READ_REPORT( mms )		((mms)->flag & FS_MMSF_READ_REPORT )

#define FS_MMS_SET_NTF( mms )				((mms)->flag |= FS_MMSF_NOTIFY)
#define FS_MMS_CLR_NTF( mms )				((mms)->flag &= (~FS_MMSF_NOTIFY) )
#define FS_MMS_CLR_REQ_READ_REPORT( mms )		((mms)->flag &= (~FS_MMSF_READ_REPORT) )
#define FS_MMS_SET_UNREAD( mms )				((mms)->flag &= (~FS_MMSF_READ) )

#define FS_MMS_SET_READ( mms )			((mms)->flag |= FS_MMSF_READ)
#define FS_MMS_SET_READ_REPORT( mms )	((mms)->flag |= FS_MMSF_READ_REPORT )

typedef struct FS_MmsHead_Tag
{
	FS_List 			list;

	FS_UINT1			mbox;
	FS_CHAR 			subject[FS_MMS_SUB_LEN + 1];
	FS_CHAR				address[FS_MMS_ADDR_LEN];
	FS_SINT4			msg_size;
	FS_DateTime			date;
	FS_UINT4			expiry;
	
	FS_CHAR				file[FS_FILE_NAME_LEN];
	FS_CHAR				msg_id[FS_MMS_MSG_ID_LEN];
	FS_UINT1			flag;
	FS_CHAR *			msg_location;	/* for mms notify only */
}FS_MmsHead;

void FS_MmsHeadDeinit( void );

FS_List * FS_MmsGetHeadList( void );

void FS_MmsAddHead( FS_MmsHead *mms );

void FS_MmsDelHead( FS_MmsHead *mms );

void FS_MmsMoveTop( FS_MmsHead *mms );

void FS_MmsSaveHeadList( void );

void FS_MmsDelHeadBox( FS_UINT1 mbox );

void FS_MmsGetFolderDetail( FS_UINT1 mbox, FS_SINT4 *pUnread, FS_SINT4 *pTotal );

void FS_MmsGetSpaceDetail( FS_SINT4 *pTotal, FS_SINT4 *pSize );

void FS_MmsHeadSaveAsTemplate( FS_MmsHead *mms, FS_CHAR *file );

void FS_MmsDelAllFolder( void );

FS_MmsHead *FS_MmsFindByMsgLocation( FS_CHAR *message_location );

#endif
