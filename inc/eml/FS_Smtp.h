#include "inc/FS_Config.h"
#include "inc/util/FS_List.h"

typedef enum FS_SmtpEvent_Tag
{
	FS_SMTP_EV_AUTH,
	FS_SMTP_EV_SENDING,
	FS_SMTP_EV_SEND_OK,
	
	FS_SMTP_EV_NET_ERR,
	FS_SMTP_EV_MEMORY_ERR,
	FS_SMTP_EV_SERVER_ERR
	
}FS_SmtpEvent;

typedef void ( *FS_SmtpEventHandler )( FS_UINT4 ss, FS_SmtpEvent ev, FS_UINT4 param );

/*
	create a smtp session, return the session handler
*/
FS_UINT4 FS_SmtpCreateSession( FS_SmtpEventHandler handler );

/*
	Send a email over smtp.
	emil file is store in head->file, email head contain from and to, cc, bcc field
*/
FS_BOOL FS_SmtpSendMail( FS_UINT4 ss, FS_List *rcpt, FS_CHAR *file );

/*
	quit smtp session.
*/
void FS_SmtpQuitSession( FS_UINT4 ss );

