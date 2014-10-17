#include "inc/FS_Config.h"

#ifdef FS_MODULE_EML

#include "inc/eml/FS_Smtp.h"
#include "inc/inte/FS_ISocket.h"
#include "inc/eml/FS_EmlAct.h"
#include "inc/eml/FS_EmlFile.h"
#include "inc/util/FS_Base64.h"

#define FS_SMTP_CMD_LEN		128

typedef enum FS_SmtpStatus_Tag
{
	FS_SMTP_STS_NONE = 0,
	FS_SMTP_STS_AUTH
}FS_SmtpStatus;

typedef enum FS_SmtpCmd_Tag
{
	FS_SMTP_CMD_NONE = 0,
	FS_SMTP_CMD_HELO,
	FS_SMTP_CMD_EHLO,
	FS_SMTP_CMD_AUTH,
	FS_SMTP_CMD_USER,
	FS_SMTP_CMD_PASS,
	FS_SMTP_CMD_MAIL,
	FS_SMTP_CMD_RCPT,
	FS_SMTP_CMD_DATA,
	FS_SMTP_CMD_SEND_DATA,		/* email data sended to smtp server */
	FS_SMTP_CMD_QUIT
}FS_SmtpCmd;

typedef struct FS_SmtpSession_Tag
{
	FS_List					list;

	FS_UINT4				socket;
	FS_List					rcpt;
	FS_CHAR					file[FS_FILE_NAME_LEN];
	FS_SINT4				offset;	/* user when read file to send */
	
	FS_SmtpStatus			status;
	FS_SmtpCmd				cmd;
	FS_CHAR					cmd_str[FS_SMTP_CMD_LEN];

	FS_SmtpEventHandler		handler;
}FS_SmtpSession;

typedef struct FS_SmtpRcpt_Tag
{
	FS_List			list;
	FS_CHAR *		mail_addr;
}FS_SmtpRcpt;

static FS_List GFS_SmtpSessionList = { &GFS_SmtpSessionList, &GFS_SmtpSessionList };

static FS_SmtpSession *FS_SmtpGetSession( FS_UINT4 socket )
{
	FS_SmtpSession *ss;
	FS_List *node = GFS_SmtpSessionList.next;
	while( node != &GFS_SmtpSessionList )
	{
		ss = FS_ListEntry( node, FS_SmtpSession, list );
		if( ss->socket == socket )
			return ss;
		node = node->next;
	}
	return FS_NULL;
}

static void FS_SmtpCopyRcpt( FS_SmtpSession *ss, FS_List *rcpt )
{
	FS_SmtpRcpt *d;
	FS_EmlAddr *s;
	FS_List *node = rcpt->next;
	while( node != rcpt )
	{
		s = FS_ListEntry( node, FS_EmlAddr, list );
		d = IFS_Malloc( sizeof(FS_SmtpRcpt) );
		if( d )
		{
			d->mail_addr = IFS_Strdup( s->addr );
			FS_ListAdd( &ss->rcpt, &d->list );
		}
		node = node->next;
	}
}

static FS_CHAR *FS_SmtpGetRcpt( FS_SmtpSession *ss )
{
	FS_SmtpRcpt *rcpt;
	FS_List *node = ss->rcpt.next;
	if( node != &ss->rcpt )
	{
		rcpt = FS_ListEntry( node, FS_SmtpRcpt, list );
		return rcpt->mail_addr;
	}
	return FS_NULL;
}

static void FS_SmtpRemoveRcpt( FS_SmtpSession *ss )
{
	FS_SmtpRcpt *rcpt;
	FS_List *node = ss->rcpt.next;
	if( node != &ss->rcpt )
	{
		rcpt = FS_ListEntry( node, FS_SmtpRcpt, list );
		FS_ListDel( &rcpt->list );
		if( rcpt->mail_addr )
			IFS_Free( rcpt->mail_addr );
		IFS_Free( rcpt );
	}
}

static void FS_SmtpFreeRcpt( FS_SmtpSession *ss )
{
	FS_SmtpRcpt *rcpt;
	FS_List *node = ss->rcpt.next;
	while( node != &ss->rcpt )
	{
		rcpt = FS_ListEntry( node, FS_SmtpRcpt, list );
		node = node->next;
		FS_ListDel( &rcpt->list );
		if( rcpt->mail_addr )
			IFS_Free( rcpt->mail_addr );
		IFS_Free( rcpt );
	}
	FS_ListInit( &ss->rcpt );
}

/* read net data form socket. once per FS_SOCKET_BUF_LEN */
static FS_CHAR * FS_SmtpReadNetData( FS_SmtpSession * ss )
{
	FS_SINT4 rlen;
	FS_CHAR *buf = IFS_Malloc( FS_SOCKET_BUF_LEN );	/* will free after data is processed */
	if( buf )
	{
		rlen = IFS_SocketRecv( ss->socket, (FS_BYTE *)buf, FS_SOCKET_BUF_LEN - 1 );
		if( rlen > 0 )
		{
			buf[rlen] = '\0';
		}
		else
		{
			IFS_Free( buf );
			buf = FS_NULL;
			ss->status = FS_SMTP_STS_NONE;
			ss->handler( (FS_UINT4)ss, FS_SMTP_EV_NET_ERR, 0 );
		}
	}
	else
	{
		ss->handler( (FS_UINT4)ss, FS_SMTP_EV_MEMORY_ERR, 0 );
	}
	return buf;
}

static FS_BOOL FS_SmtpReplyOk( FS_SmtpSession * ss, FS_CHAR *buf, FS_BOOL report )
{
	if( buf[0] == '2' || buf[0] == '3' )
		return FS_TRUE;
	else
	{
		if( report )
		{
			ss->status = FS_SMTP_STS_NONE;
			ss->handler( (FS_UINT4)ss, FS_SMTP_EV_SERVER_ERR, (FS_UINT4)buf );
		}
		return FS_FALSE;
	}
}

static void FS_SmtpProcessGreeting( FS_SmtpSession * ss )
{
	FS_CHAR *buf;
	FS_EmlAccount *act = FS_EmlGetActiveAct( );
	buf = FS_SmtpReadNetData( ss );
	if( buf )
	{
		if( FS_SmtpReplyOk( ss, buf, FS_TRUE ) )
		{
			if( act->smtp_auth )
			{
				IFS_Sprintf( ss->cmd_str, "EHLO %s\r\n", act->user_name );
				ss->cmd = FS_SMTP_CMD_EHLO;
			}
			else
			{
				IFS_Sprintf( ss->cmd_str, "HELO %s\r\n", act->user_name );
				ss->cmd = FS_SMTP_CMD_HELO;
			}
			ss->handler( (FS_UINT4)ss, FS_SMTP_EV_AUTH, 0 );
			IFS_SocketSend( ss->socket, (FS_BYTE *)ss->cmd_str, IFS_Strlen(ss->cmd_str) );
		}
		IFS_Free( buf );
	}
}

static void FS_SmtpProcessHeloCmdResp( FS_SmtpSession * ss )
{
	FS_CHAR *buf;
	FS_EmlAccount *act = FS_EmlGetActiveAct( );
	buf = FS_SmtpReadNetData( ss );
	if( buf )
	{
		if( FS_SmtpReplyOk( ss, buf, FS_TRUE ) )
		{
			ss->status = FS_SMTP_STS_AUTH;
			IFS_Sprintf( ss->cmd_str, "MAIL FROM: <%s>\r\n", act->eml_addr );
			ss->cmd = FS_SMTP_CMD_MAIL;
			IFS_SocketSend( ss->socket, (FS_BYTE *)ss->cmd_str, IFS_Strlen(ss->cmd_str) );
		}
		IFS_Free( buf );
	}
}

static void FS_SmtpProcessEhloCmdResp( FS_SmtpSession * ss )
{
	FS_CHAR *buf;
	FS_EmlAccount *act = FS_EmlGetActiveAct( );
	buf = FS_SmtpReadNetData( ss );
	if( buf )
	{
		if( FS_SmtpReplyOk( ss, buf, FS_FALSE ) )
		{
			IFS_Strcpy( ss->cmd_str, "AUTH LOGIN\r\n" );
			ss->cmd = FS_SMTP_CMD_AUTH;
			IFS_SocketSend( ss->socket, (FS_BYTE *)ss->cmd_str, IFS_Strlen(ss->cmd_str) );
		}
		else	/* did not support EHLO command, we use HELO instead */
		{
			IFS_Sprintf( ss->cmd_str, "HELO %s\r\n", act->user_name );
			ss->cmd = FS_SMTP_CMD_HELO;
			IFS_SocketSend( ss->socket, (FS_BYTE *)ss->cmd_str, IFS_Strlen(ss->cmd_str) );
		}
		IFS_Free( buf );
	}
}

static void FS_SmtpProcessAuthCmdResp( FS_SmtpSession * ss )
{
	FS_CHAR *buf;
	FS_EmlAccount *act = FS_EmlGetActiveAct( );
	buf = FS_SmtpReadNetData( ss );
	if( buf )
	{
		if( FS_SmtpReplyOk( ss, buf, FS_FALSE ) )
		{
			FS_Base64EncodeLine(  ss->cmd_str, (FS_BYTE *)act->user_name, IFS_Strlen(act->user_name) );
			IFS_Strcat( ss->cmd_str, "\r\n" );
			ss->cmd = FS_SMTP_CMD_USER;
			IFS_SocketSend( ss->socket, (FS_BYTE *)ss->cmd_str, IFS_Strlen(ss->cmd_str) );
		}
		else	/* did not support AUTH LOGIN command, we use HELO instead */
		{
			IFS_Sprintf( ss->cmd_str, "HELO %s\r\n", act->user_name );
			ss->cmd = FS_SMTP_CMD_HELO;
			IFS_SocketSend( ss->socket, (FS_BYTE *)ss->cmd_str, IFS_Strlen(ss->cmd_str) );
		}
		IFS_Free( buf );
	}
}

static void FS_SmtpProcessUserCmdResp( FS_SmtpSession * ss )
{
	FS_CHAR *buf;
	FS_EmlAccount *act = FS_EmlGetActiveAct( );
	buf = FS_SmtpReadNetData( ss );
	if( buf )
	{
		if( FS_SmtpReplyOk( ss, buf, FS_TRUE ) )
		{
			FS_Base64EncodeLine(  ss->cmd_str, (FS_BYTE *)act->password, IFS_Strlen(act->password) );
			IFS_Strcat( ss->cmd_str, "\r\n" );
			ss->cmd = FS_SMTP_CMD_PASS;
			IFS_SocketSend( ss->socket, (FS_BYTE *)ss->cmd_str, IFS_Strlen(ss->cmd_str) );
		}
		IFS_Free( buf );
	}
}

static void FS_SmtpProcessPassCmdResp( FS_SmtpSession * ss )
{
	FS_CHAR *buf;
	FS_EmlAccount *act = FS_EmlGetActiveAct( );
	buf = FS_SmtpReadNetData( ss );
	if( buf )
	{
		if( FS_SmtpReplyOk( ss, buf, FS_TRUE ) )
		{
			ss->status = FS_SMTP_STS_AUTH;
			IFS_Sprintf( ss->cmd_str, "MAIL FROM: <%s>\r\n", act->eml_addr );
			ss->cmd = FS_SMTP_CMD_MAIL;
			IFS_SocketSend( ss->socket, (FS_BYTE *)ss->cmd_str, IFS_Strlen(ss->cmd_str) );
		}
		IFS_Free( buf );
	}
}

/* if ok, we send RCPT TO: cmd */
static void FS_SmtpProcessMailCmdResp( FS_SmtpSession * ss )
{
	FS_CHAR *buf;
	FS_CHAR *rcpt = FS_SmtpGetRcpt( ss );
	buf = FS_SmtpReadNetData( ss );
	if( buf )
	{
		if( FS_SmtpReplyOk( ss, buf, FS_TRUE ) )
		{
			IFS_Sprintf( ss->cmd_str, "RCPT TO: <%s>\r\n", rcpt );
			ss->cmd = FS_SMTP_CMD_RCPT;
			FS_SmtpRemoveRcpt( ss );
			IFS_SocketSend( ss->socket, (FS_BYTE *)ss->cmd_str, IFS_Strlen(ss->cmd_str) );
		}
		IFS_Free( buf );
	}
}

/* if we got another rcpt, send RCPT TO: cmd again */
static void FS_SmtpProcessRcptCmdResp( FS_SmtpSession * ss )
{
	FS_CHAR *buf;
	FS_CHAR *rcpt = FS_SmtpGetRcpt( ss );
	buf = FS_SmtpReadNetData( ss );
	if( buf )
	{
		/* if rcpt is not valid, we ignore this error */
		if( rcpt )
		{
			IFS_Sprintf( ss->cmd_str, "RCPT TO: <%s>\r\n", rcpt );
			FS_SmtpRemoveRcpt( ss );
			ss->cmd = FS_SMTP_CMD_RCPT;
			IFS_SocketSend( ss->socket, (FS_BYTE *)ss->cmd_str, IFS_Strlen(ss->cmd_str) );
		}
		/* rcpt list send complete. we send DATA cmd */
		else
		{
			IFS_Strcpy( ss->cmd_str, "DATA\r\n" );
			ss->cmd = FS_SMTP_CMD_DATA;
			IFS_SocketSend( ss->socket, (FS_BYTE *)ss->cmd_str, IFS_Strlen(ss->cmd_str) );
		}
		IFS_Free( buf );
	}
}

/* this function is called to send email data. more data is send when socket is writable */
static void FS_SmtpSendData( FS_SmtpSession *ss )
{
	FS_CHAR *buf;
	FS_SINT4 rlen;
	buf = IFS_Malloc( FS_SOCKET_PKG_LEN );
	if( buf )
	{
		rlen = FS_FileRead( FS_DIR_EML, ss->file, ss->offset, buf, FS_SOCKET_PKG_LEN );
		if( rlen > 0 )
		{
			rlen = IFS_SocketSend( ss->socket, (FS_BYTE *)buf, rlen );
			ss->offset += rlen;
			ss->handler( (FS_UINT4)ss, FS_SMTP_EV_SENDING, ss->offset );
			ss->cmd = FS_SMTP_CMD_SEND_DATA;
		}
		else if( rlen == 0 )	/* file ends */
		{
			IFS_SocketSend( ss->socket, (FS_BYTE *)"\r\n.\r\n", 5 );
			ss->handler( (FS_UINT4)ss, FS_SMTP_EV_SEND_OK, 0 );
		}
		IFS_Free( buf );
	}
}

/* if we got another rcpt, send RCPT TO: cmd again */
static void FS_SmtpProcessDataCmdResp( FS_SmtpSession * ss )
{
	FS_CHAR *buf;
	FS_BOOL bOk = FS_FALSE;
	FS_CHAR *rcpt = FS_SmtpGetRcpt( ss );
	buf = FS_SmtpReadNetData( ss );
	if( buf )
	{
		if( FS_SmtpReplyOk( ss, buf, FS_TRUE ) )
			bOk = FS_TRUE;
		
		IFS_Free( buf );	/* free memory first */
		if( bOk )
			FS_SmtpSendData( ss );
	}
}

static void FS_SmtpProcessNetData( FS_SmtpSession * ss )
{
	if( ss->cmd == FS_SMTP_CMD_NONE )
		FS_SmtpProcessGreeting( ss );
	else if( ss->cmd == FS_SMTP_CMD_HELO )
		FS_SmtpProcessHeloCmdResp( ss );
	else if( ss->cmd == FS_SMTP_CMD_EHLO )
		FS_SmtpProcessEhloCmdResp( ss );
	else if( ss->cmd == FS_SMTP_CMD_AUTH )
		FS_SmtpProcessAuthCmdResp( ss );
	else if( ss->cmd == FS_SMTP_CMD_USER )
		FS_SmtpProcessUserCmdResp( ss );
	else if( ss->cmd == FS_SMTP_CMD_PASS )
		FS_SmtpProcessPassCmdResp( ss );
	else if( ss->cmd == FS_SMTP_CMD_MAIL )
		FS_SmtpProcessMailCmdResp( ss );
	else if( ss->cmd == FS_SMTP_CMD_RCPT )
		FS_SmtpProcessRcptCmdResp( ss );
	else if( ss->cmd == FS_SMTP_CMD_DATA )
		FS_SmtpProcessDataCmdResp( ss );
}

static void FS_SmtpSockEventHandler( FS_UINT4 socket, FS_ISockEvent ev, FS_UINT4 param )
{
	FS_SmtpSession *ss = FS_SmtpGetSession( socket );
	if( ss )
	{
		if( FS_ISOCK_SENDOK == ev )
		{
			if( ss->cmd == FS_SMTP_CMD_QUIT )
			{
				IFS_SocketClose( ss->socket );
				FS_ListDel( &ss->list );
				FS_SmtpFreeRcpt( ss );
				IFS_Free( ss );
			}
		}
		else if( FS_ISOCK_READ == ev )
		{
			FS_SmtpProcessNetData( ss );
		}
		else if( FS_ISOCK_WRITE == ev && ss->cmd == FS_SMTP_CMD_SEND_DATA )
		{
			FS_SmtpSendData( ss );
		}
		else if( FS_ISOCK_ERROR == ev )
		{
			ss->handler( (FS_UINT4)ss, FS_SMTP_EV_NET_ERR, 0 );
			IFS_SocketClose( ss->socket );
			ss->socket = 0;
			ss->status = FS_SMTP_STS_NONE;
		}
	}
}

static void FS_SmtpSessionInit( FS_SmtpSession *ss )
{
	FS_EmlAccount *act = FS_EmlGetActiveAct( );
	if( act )
	{
		if( ! IFS_SocketCreate( &ss->socket, FS_TRUE, FS_SmtpSockEventHandler )
			|| ! IFS_SocketConnect( ss->socket, act->smtp_addr, act->smtp_port ) )
		{
			ss->handler( (FS_UINT4)ss, FS_SMTP_EV_NET_ERR, 0 );
		}
	}
}

/*
	create a smtp session, return the session handler
*/
FS_UINT4 FS_SmtpCreateSession( FS_SmtpEventHandler handler )
{
	FS_SmtpSession *ss = IFS_Malloc( sizeof(FS_SmtpSession) );
	if( ss )
	{
		IFS_Memset( ss, 0, sizeof(FS_SmtpSession) );
		FS_ListInit( &ss->rcpt );
		ss->handler = handler;
		FS_ListAdd( &GFS_SmtpSessionList, &ss->list );
	}
	return (FS_UINT4)ss;
}

/*
	Send a email over smtp.
	emil file is store in head->file, email head contain from and to, cc, bcc field
*/
FS_BOOL FS_SmtpSendMail( FS_UINT4 session, FS_List *rcpt, FS_CHAR *file )
{
	FS_SmtpSession *ss = (FS_SmtpSession *)session;
	FS_EmlAccount *act = FS_EmlGetActiveAct( );

	if( act == FS_NULL || FS_ListIsEmpty(rcpt) )
		return FS_FALSE;
	
	FS_SmtpCopyRcpt( ss, rcpt );
	IFS_Strcpy( ss->file, file );
	if( ss->status == FS_SMTP_STS_AUTH )
	{
		IFS_Sprintf( ss->cmd_str, "MAIL FROM: <%s>\r\n", act->eml_addr );
		ss->cmd = FS_SMTP_CMD_MAIL;
		IFS_SocketSend( ss->socket, (FS_BYTE *)ss->cmd_str, IFS_Strlen(ss->cmd_str) );
	}
	else
	{
		FS_SmtpSessionInit( ss );
	}
	return FS_TRUE;
}

/*
	quit smtp session.
*/
void FS_SmtpQuitSession( FS_UINT4 session )
{
	FS_SmtpSession *ss = (FS_SmtpSession *)session;
	FS_SmtpFreeRcpt( ss );
	if( ss->status == FS_SMTP_STS_AUTH )
	{
		IFS_Strcpy( ss->cmd_str, "QUIT\r\n" );
		ss->cmd = FS_SMTP_CMD_QUIT;
		IFS_SocketSend( ss->socket, (FS_BYTE *)ss->cmd_str, IFS_Strlen(ss->cmd_str) );
	}
	else
	{
		FS_ListDel( &ss->list );
		IFS_SocketClose( ss->socket );
		IFS_Free( ss );
	}
}

#endif	//FS_MODULE_EML

