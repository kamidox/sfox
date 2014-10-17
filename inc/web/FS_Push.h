#ifndef _FS_PUSH_H_
#define _FS_PUSH_H_

#include "inc/FS_Config.h"
#include "inc/util/FS_File.h"
#include "inc/util/FS_List.h"

#define FS_MAX_PUSH_SUB_LEN			64

#define FS_PUSHMSG_PRI_NONE			0x00
#define FS_PUSHMSG_PRI_LOW 			0x01
#define FS_PUSHMSG_PRI_MEDIUM		0x02
#define FS_PUSHMSG_PRI_HIGH			0x04

#define FS_PUSHMSG_TYPE_SI			0x08
#define FS_PUSHMSG_TYPE_SL			0x10

#define FS_PUSHMSG_READ				0x20

#define FS_PUSHMSG_SET_FLAG( pMsg, fval )		((pMsg)->flag |= (fval))
#define FS_PUSHMSG_GET_FLAG( pMsg, fval )		((pMsg)->flag & (fval))

typedef struct FS_PushMsg_Tag
{
	FS_List			list;
	FS_CHAR			url[FS_URL_LEN];
	FS_CHAR			subject[FS_MAX_PUSH_SUB_LEN];
	FS_DateTime		date;
	FS_UINT4		flag;
}FS_PushMsg;

void FS_PushAddItem( FS_PushMsg *pMsg );

void FS_PushDelItem( FS_PushMsg *pMsg );

void FS_PushDelAll( void );

void FS_PushSaveList( void );

FS_List *FS_PushGetList( void );

void FS_PushDeinit( void );

#endif
