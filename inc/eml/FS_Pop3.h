#include "inc\FS_Config.h"

typedef enum FS_Pop3Event_Tag
{
	FS_POP3_EV_NET_ERR,
	FS_POP3_EV_SERVER_ERR,
	FS_POP3_EV_MEMORY_ERR,
	FS_POP3_EV_MSG_NOT_EXIST,
	FS_POP3_EV_FILE_ERR,
	FS_POP3_EV_BAD_MAIL,

	FS_POP3_EV_AUTH,
	FS_POP3_EV_UIDL,
	FS_POP3_EV_TOP,
	FS_POP3_EV_RETR,
	FS_POP3_EV_RETR_DATA,
	FS_POP3_EV_DELE
}FS_Pop3Event;

typedef struct FS_Pop3Uidl_Tag
{
	FS_List			list;
	FS_SINT4		msg_id;
	FS_CHAR			uid[FS_EML_UID_LEN];
	FS_SINT4		msg_size;
}FS_Pop3Uidl;

typedef void ( *FS_Pop3EventHandler )( FS_UINT4 ss, FS_Pop3Event ev, FS_UINT4 param );

// create a pop3 session, return pop3 session handle
FS_UINT4 FS_Pop3CreateSession( FS_Pop3EventHandler handler );

void FS_Pop3GetUidl( FS_UINT4 session, FS_SINT4 msgId );

/* 
	only support get size for single mail
	this function return immediately. of not cache the uidl, return 0;
*/
FS_SINT4 FS_Pop3GetSize( FS_UINT4 session, FS_CHAR *uid );

void FS_Pop3GetTop( FS_UINT4 session, FS_CHAR *uid );

void FS_Pop3RetrMail( FS_UINT4 session, FS_CHAR *uid );

void FS_Pop3DeleMail( FS_UINT4 session, FS_CHAR *uid );

void FS_Pop3QuitSession( FS_UINT4 session );


