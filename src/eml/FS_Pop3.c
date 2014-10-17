#include "inc/FS_Config.h"

#ifdef FS_MODULE_EML

#include "inc\inte\FS_ISocket.h"
#include "inc\eml\FS_EmlAct.h"
#include "inc\eml\FS_EmlMain.h"
#include "inc\util\FS_List.h"
#include "inc\util\FS_Util.h"
#include "inc\util\FS_File.h"
#include "inc\eml\FS_Pop3.h"
#include "inc\inte\FS_Inte.h"
#include "inc\util\FS_MemDebug.h"

#define FS_POP3_CMD_LEN			72
#define FS_POP3_LINE_LEN		128
#define FS_POP3_END_STR_LEN		15

typedef enum FS_Pop3ActType_Tag
{
	FS_POP3_ACT_NONE = 0,
	FS_POP3_ACT_AUTH,
	FS_POP3_ACT_UIDL,
	FS_POP3_ACT_DELE,
	FS_POP3_ACT_RETR,
	FS_POP3_ACT_TOP,
	FS_POP3_ACT_LIST
}FS_Pop3ActType;

typedef enum FS_Pop3Status_Tag
{
	FS_POP3_STS_NONE = 0,
	FS_POP3_STS_AUTH_SEND_USER,
	FS_POP3_STS_AUTH_SEND_PASS,
	FS_POP3_STS_TRANS,
	FS_POP3_STS_SEND_QUIT
}FS_Pop3Status;

typedef struct FS_Pop3CmdResultFile_Tag
{
	FS_CHAR			file[FS_FILE_NAME_LEN];
	FS_CHAR *		buffer;
}FS_Pop3CmdResultData;

typedef struct FS_Pop3Action_Tag
{
	/* action type eg. UIDL, RETR etc. */
	FS_Pop3ActType		act;
	FS_CHAR				cmd[FS_POP3_CMD_LEN];
	FS_UINT4			param;	/* optional. command param */
	/* action response data */
	FS_BOOL				rsp_ok;
	FS_CHAR *			rsp_data;
	FS_CHAR				end_str[FS_POP3_END_STR_LEN + 1];	/* collect end string '\r\n.\r\n' */
	FS_CHAR				file[FS_FILE_NAME_LEN];
	FS_SINT4			offset;
}FS_Pop3Action;

/**
	first, we may create pop3 session. and them when user send TOP command,
	we must send UIDL command first to cache uidl. But, when fetch uidl, it 
	may be that: user have not login in, so we must send USER and PASS command
	first, after we cache the uidl, we may send TOP command to fetch top. 

	So, there is 3 level pending command. - Joey
	pending1 is user for UIDL command,
	pending2 is user for TOP or RETR or DEL command.
*/
typedef struct FS_Pop3Session_Tag
{
	FS_List					list;
	FS_UINT4				socket;
	FS_Pop3Action			actions[3];		/* pop3 only need 3 level pending action */
	FS_Pop3Action *			cur_act;		/* current action, point to actions[] array */	
	
	FS_Pop3EventHandler		handler;
	FS_Pop3Status			status;
	FS_List					uidl;
}FS_Pop3Session;

static FS_List GFS_Pop3SessionList = { &GFS_Pop3SessionList, &GFS_Pop3SessionList };

static FS_Pop3Session *FS_Pop3GetSession( FS_UINT4 socket )
{
	FS_Pop3Session *ss;
	FS_List *node = GFS_Pop3SessionList.next;
	while( node != &GFS_Pop3SessionList )
	{
		ss = FS_ListEntry( node, FS_Pop3Session, list );
		if( ss->socket == socket )
			return ss;
		node = node->next;
	}
	return FS_NULL;
}

static void FS_Pop3FreeUidl( FS_Pop3Session * ss )
{
	FS_Pop3Uidl *uid;
	FS_List *node = ss->uidl.next;
	while( node != &ss->uidl )
	{
		uid = FS_ListEntry( node, FS_Pop3Uidl, list );
		node = node->next;
		FS_ListDel( &uid->list );
		IFS_Free( uid );
	}
	FS_ListInit( &ss->uidl );
}

static FS_SINT4 FS_Pop3GetMsgId( FS_Pop3Session * ss, FS_CHAR *uid )
{
	FS_Pop3Uidl *uidl;
	FS_List *node = ss->uidl.next;
	while( node != &ss->uidl )
	{
		uidl = FS_ListEntry( node, FS_Pop3Uidl, list );
		if( ! IFS_Strcmp( uid, uidl->uid) )
			return uidl->msg_id;
		node = node->next;
	}
	return -1;
}

static FS_SINT4 FS_Pop3GetMsgSize( FS_Pop3Session * ss, FS_CHAR *uid )
{
	FS_Pop3Uidl *uidl;
	FS_List *node = ss->uidl.next;
	while( node != &ss->uidl )
	{
		uidl = FS_ListEntry( node, FS_Pop3Uidl, list );
		if( ! IFS_Strcmp( uid, uidl->uid) )
			return uidl->msg_size;
		node = node->next;
	}
	return 0;
}

static void FS_Pop3SetMsgSize( FS_Pop3Session * ss, FS_SINT4 msgId, FS_SINT4 size )
{
	FS_Pop3Uidl *uidl;
	FS_List *node = ss->uidl.next;
	while( node != &ss->uidl )
	{
		uidl = FS_ListEntry( node, FS_Pop3Uidl, list );
		if( uidl->msg_id == msgId )
		{
			uidl->msg_size = size;
			break;
		}
		node = node->next;
	}
}

static void FS_Pop3ResetCurAction( FS_Pop3Session * ss )
{
	if( ss->cur_act )
	{
		if( ss->cur_act->rsp_data )
			IFS_Free( ss->cur_act->rsp_data );
		if( IFS_Strlen(ss->cur_act->file) > 0 )
			FS_FileDelete( FS_DIR_TMP, ss->cur_act->file );
		IFS_Memset( ss->cur_act, 0, sizeof(FS_Pop3Action) );
		ss->cur_act = FS_NULL;
	}
}

/* read net data form socket. once per FS_SOCKET_BUF_LEN */
static FS_CHAR * FS_Pop3ReadNetData( FS_Pop3Session * ss, FS_SINT4 *len )
{
	FS_SINT4 rlen;
	FS_CHAR *buf = IFS_Malloc( FS_SOCKET_BUF_LEN );	/* will free after data is processed */
	if( buf )
	{
		rlen = IFS_SocketRecv( ss->socket, buf, FS_SOCKET_BUF_LEN - 1 );
		if( rlen > 0 )
		{
			buf[rlen] = '\0';
		}
		else
		{
			IFS_Free( buf );
			buf = FS_NULL;
			ss->status = FS_POP3_STS_NONE;
			ss->handler( (FS_UINT4)ss, FS_POP3_EV_NET_ERR, 0 );
		}
	}
	else
	{
		ss->handler( (FS_UINT4)ss, FS_POP3_EV_MEMORY_ERR, 0 );
	}
	
	if( len )
		*len = rlen;
	return buf;
}

static void FS_Pop3ProcessGreeting( FS_Pop3Session * ss )
{
	FS_CHAR *buf;
	FS_EmlAccount *act = FS_EmlGetActiveAct( );
	buf = FS_Pop3ReadNetData( ss, FS_NULL );
	if( buf )
	{
		if( ! IFS_Strncmp( buf, "+OK", 3 ) )
		{
			ss->cur_act = &ss->actions[0];
			IFS_Strcpy( ss->cur_act->cmd, "USER " );
			if( act )
				IFS_Strcat( ss->cur_act->cmd, act->user_name );
			IFS_Strcat( ss->cur_act->cmd, "\r\n" );
			ss->handler( (FS_UINT4)ss, FS_POP3_EV_AUTH, 0 );
			IFS_SocketSend( ss->socket, ss->cur_act->cmd, IFS_Strlen(ss->cur_act->cmd) );
			ss->status = FS_POP3_STS_AUTH_SEND_USER;
		}
		else
		{
			ss->status = FS_POP3_STS_NONE;
			ss->handler( (FS_UINT4)ss, FS_POP3_EV_SERVER_ERR, (FS_UINT4)buf );
		}
		IFS_Free( buf );
	}
}

static void FS_Pop3ProcessUserCmdResp( FS_Pop3Session * ss )
{
	FS_CHAR *buf = FS_Pop3ReadNetData( ss, FS_NULL );
	FS_EmlAccount *act = FS_EmlGetActiveAct( );

	if( buf )
	{
		if( ! IFS_Strncmp( buf, "+OK", 3 ) )
		{
			ss->cur_act = &ss->actions[0];
			IFS_Strcpy( ss->cur_act->cmd, "PASS " );
			if( act )
				IFS_Strcat( ss->cur_act->cmd, act->password );
			IFS_Strcat( ss->cur_act->cmd, "\r\n" );
			IFS_SocketSend( ss->socket, ss->cur_act->cmd, IFS_Strlen(ss->cur_act->cmd) );
			ss->status = FS_POP3_STS_AUTH_SEND_PASS;
		}
		else
		{
			ss->status = FS_POP3_STS_NONE;
			ss->handler( (FS_UINT4)ss, FS_POP3_EV_SERVER_ERR, (FS_UINT4)buf );
		}
		IFS_Free( buf );
	}
}

static void FS_Pop3ProcessPassCmdResp( FS_Pop3Session * ss )
{
	FS_CHAR *buf = FS_Pop3ReadNetData( ss, FS_NULL );

	if( buf )
	{
		if( ! IFS_Strncmp( buf, "+OK", 3 ) )
			ss->status = FS_POP3_STS_TRANS;
		else
		{
			ss->status = FS_POP3_STS_NONE;
			ss->handler( (FS_UINT4)ss, FS_POP3_EV_SERVER_ERR, (FS_UINT4)buf );
		}
		IFS_Free( buf );
	}

	// check for pending command
	if( ss->status == FS_POP3_STS_TRANS && ss->actions[1].act != FS_POP3_ACT_NONE )
	{
		FS_Pop3ResetCurAction( ss );
		ss->cur_act = &ss->actions[1];
		IFS_SocketSend( ss->socket, ss->cur_act->cmd, IFS_Strlen(ss->cur_act->cmd) );
	}
}

static void FS_Pop3ParseUidlLine( FS_Pop3Uidl *uid, FS_CHAR *line )
{
	FS_CHAR *p;
	IFS_Memset( uid, 0, sizeof(FS_Pop3Uidl) );
	uid->msg_id = IFS_Atoi( line );
	p = IFS_Strchr( line, ' ' );
	if( p )
	{
		while( *p == ' ' ) p ++;
		IFS_Strncpy( uid->uid, p, FS_EML_UID_LEN - 1 );
	}
}

static void FS_Pop3HandleListLine( FS_Pop3Session *ss, FS_CHAR *line )
{
	FS_CHAR *p;
	FS_SINT4 size;
	FS_SINT4 msgId = IFS_Atoi( line );
	p = IFS_Strchr( line, ' ' );
	if( p )
	{
		while( *p == ' ' ) p ++;
		size = IFS_Atoi( p );
		FS_Pop3SetMsgSize( ss, msgId, size );
	}
}

static void FS_Pop3HandlePendingAction( FS_Pop3Session *ss )
{
	/* pending command param 'uid' is store in pending_cmd2 */
	FS_SINT4 id = FS_Pop3GetMsgId( ss, ss->actions[2].cmd );
	if( id < 0 )
	{
		ss->handler( (FS_UINT4)ss, FS_POP3_EV_MSG_NOT_EXIST, 0 );
	}
	else
	{
		FS_Pop3ResetCurAction( ss );
		ss->cur_act = &ss->actions[2];
		if( ss->cur_act->act == FS_POP3_ACT_TOP )
			IFS_Sprintf( ss->cur_act->cmd, "TOP %d 0\r\n", id );
		else if( ss->cur_act->act == FS_POP3_ACT_RETR )
			IFS_Sprintf( ss->cur_act->cmd, "RETR %d\r\n", id );
		else if( ss->cur_act->act == FS_POP3_ACT_DELE )
			IFS_Sprintf( ss->cur_act->cmd, "DELE %d\r\n", id );
		else if( ss->cur_act->act == FS_POP3_ACT_LIST )
			IFS_Sprintf( ss->cur_act->cmd, "LIST %d\r\n", id );
		IFS_SocketSend( ss->socket, ss->cur_act->cmd, IFS_Strlen(ss->cur_act->cmd) );
	}
}

static void FS_Pop3HandleRetrResult( FS_Pop3Session *ss, FS_CHAR *buf, FS_SINT4 len )
{
	FS_CHAR *p = buf;
	/* first in, create a temp file and save to it */
	if( ss->cur_act->offset == 0 )
	{
		FS_GetGuid( ss->cur_act->file );
		IFS_Strcat( ss->cur_act->file, ".eml" );
	}
	
	FS_FileWrite( FS_DIR_TMP, ss->cur_act->file, ss->cur_act->offset, p, len );
	ss->cur_act->offset += len;
	ss->handler( (FS_UINT4)ss, FS_POP3_EV_RETR_DATA, ss->cur_act->offset );
	if( len >= FS_POP3_END_STR_LEN )
	{
		IFS_Memcpy( ss->cur_act->end_str, p + len - FS_POP3_END_STR_LEN, FS_POP3_END_STR_LEN );
	}
	else if( len > 0 )
	{
		IFS_Memmove( ss->cur_act->end_str, ss->cur_act->end_str + len, FS_POP3_END_STR_LEN - len );
		IFS_Memcpy( ss->cur_act->end_str + FS_POP3_END_STR_LEN - len, p, len );
	}
	
	/* data finished, report to user */
	if( IFS_Strstr( ss->cur_act->end_str, "\r\n.\r\n") )
	{
		ss->handler( (FS_UINT4)ss, FS_POP3_EV_RETR, (FS_UINT4)&ss->cur_act->file );
		/*
			here, we must reset the cur_act, BUT. user may call another RETR cmd,
			then, here will reset the cur_act set in FS_Pop3RetrMail function.
			SO, we puth reset cur_act code in to interface, act as LAZY RESET.
		*/
	}
}

static void FS_Pop3HandleUidlResult( FS_Pop3Session *ss, FS_CHAR *buf )
{
	FS_CHAR line[FS_POP3_LINE_LEN];
	FS_Pop3Uidl *uid;
	
	if( ss->cur_act->param )	/* cmd: UIDL\r\n, multiline response */
	{
		while( FS_GetLine( line, FS_POP3_LINE_LEN, &buf ) )
		{
			if( line[0] == '+' )
				continue;
			
			/* reach to end line, now we send LIST cmd to fetch msg size */
			if( line[0] == '.' )
			{
				FS_Pop3ResetCurAction( ss );
				ss->cur_act = &ss->actions[0];
				ss->cur_act->act = FS_POP3_ACT_LIST;
				IFS_Strcpy( ss->cur_act->cmd, "LIST\r\n" );
				IFS_SocketSend( ss->socket, ss->cur_act->cmd, IFS_Strlen(ss->cur_act->cmd) );
				return;
			}
			uid = IFS_Malloc( sizeof(FS_Pop3Uidl) );
			if( uid )
			{
				FS_Pop3ParseUidlLine( uid, line );
				FS_ListAdd( &ss->uidl, &uid->list );
			}
		}
	}
	else			// cmd UIDL msgid\r\n, single line response
	{
		FS_Pop3Uidl ruid;
		buf += 4;	// skip "+OK "
		FS_Pop3ParseUidlLine( &ruid, buf );
		FS_ListInit( &ruid.list );
		ss->handler( (FS_UINT4)ss, FS_POP3_EV_UIDL, (FS_UINT4)&ruid.list );
	}
}

static void FS_Pop3HandleListResult( FS_Pop3Session *ss, FS_CHAR *buf )
{
	FS_CHAR line[FS_POP3_LINE_LEN];
	
	while( FS_GetLine( line, FS_POP3_LINE_LEN, &buf ) )
	{
		if( line[0] == '+' )
			continue;
		
		if( line[0] == '.' )	/* reach to end line, report result now */
		{
			/* here, we get a pending action. send pending action instead */
			if( ss->actions[2].act != FS_POP3_ACT_NONE )
			{
				FS_Pop3HandlePendingAction( ss );
			}
			else
			{
				ss->handler( (FS_UINT4)ss, FS_POP3_EV_UIDL, (FS_UINT4)&ss->uidl );
			}
			break;
		}
		FS_Pop3HandleListLine( ss, line );
	}
}


static void FS_Pop3ProcessTopCmdResp( FS_Pop3Session * ss )
{
	FS_SINT4 rlen;
	if( ss->cur_act->rsp_data == FS_NULL )
	{
		ss->cur_act->rsp_data = IFS_Malloc( FS_EML_HEAD_MAX_LEN );
		if( ss->cur_act->rsp_data )
		{
			IFS_Memset( ss->cur_act->rsp_data, 0, FS_EML_HEAD_MAX_LEN );
		}
		else
		{
			ss->handler( (FS_UINT4)ss, FS_POP3_EV_MEMORY_ERR, 0 );
		}	
	}
	
	if( ss->cur_act->rsp_data )
	{
		FS_CHAR *buf;
		rlen = IFS_Strlen( ss->cur_act->rsp_data );
		buf = ss->cur_act->rsp_data + rlen;

		rlen = IFS_SocketRecv( ss->socket, buf, FS_EML_HEAD_MAX_LEN - 1 - rlen );
		if( rlen > 0 )
		{
			buf[rlen] = '\0';
			if( IFS_Strncmp( buf, "+OK", 3 ) == 0 )
				ss->cur_act->rsp_ok = FS_TRUE;

			if( ! ss->cur_act->rsp_ok )
			{
				ss->handler( (FS_UINT4)ss, FS_POP3_EV_TOP, FS_NULL );
			}
			
			/* data finished */
			if( ss->cur_act && ss->cur_act->rsp_ok 
				&& IFS_Strstr( ss->cur_act->rsp_data, "\r\n.\r\n" ) )
			{
				buf = IFS_Strchr( ss->cur_act->rsp_data, '\n' );	/* skip +OK line */
				ss->handler( (FS_UINT4)ss, FS_POP3_EV_TOP, (FS_UINT4)buf + 1 );
			}
		}
		else
		{
			ss->status = FS_POP3_STS_NONE;
			ss->handler( (FS_UINT4)ss, FS_POP3_EV_NET_ERR, 0 );
		}
	}
}

static void FS_Pop3ProcessUidlCmdResp( FS_Pop3Session * ss )
{
	FS_CHAR *buf = FS_Pop3ReadNetData( ss, FS_NULL );
	if( buf )
	{
		if( IFS_Strncmp( buf, "+OK", 3 ) == 0 )
		{
			ss->cur_act->rsp_ok = FS_TRUE;
			FS_Pop3FreeUidl( ss );		/* clear any cache uidl */
		}
		
		if( ss->cur_act->rsp_ok )
			FS_Pop3HandleUidlResult( ss, buf );
		else
			ss->handler( (FS_UINT4)ss, FS_POP3_EV_SERVER_ERR, (FS_UINT4)buf );

		IFS_Free( buf );
	}
}

static void FS_Pop3ProcessRetrCmdResp( FS_Pop3Session * ss )
{
	FS_SINT4 len;
	FS_CHAR *buf = FS_Pop3ReadNetData( ss, &len );
	FS_CHAR *p;
	if( buf )
	{
		p = buf;
		if( IFS_Strncmp( buf, "+OK", 3 ) == 0 )
		{
			ss->cur_act->rsp_ok = FS_TRUE;
			p = IFS_Strchr( p, '\n' );
			if( p )
			{
				p ++;
				len = len - ( p - buf );
			}
			else
			{
				p = buf;
			}
		}
		
		if( ! ss->cur_act->rsp_ok )
			ss->handler( (FS_UINT4)ss, FS_POP3_EV_SERVER_ERR, (FS_UINT4)buf );
		
		if( ss->cur_act->rsp_ok && len > 0 )
			FS_Pop3HandleRetrResult( ss, p, len );

		IFS_Free( buf );
	}
}

static void FS_Pop3ProcessDeleCmdResp( FS_Pop3Session * ss )
{
	FS_CHAR *buf = FS_Pop3ReadNetData( ss, FS_NULL );
	if( buf )
	{
		if( IFS_Strncmp( buf, "+OK", 3 ) == 0 )
			ss->handler( (FS_UINT4)ss, FS_POP3_EV_DELE, 0 );
		else		/* when dele failed, we didnot to handle it */
			ss->handler( (FS_UINT4)ss, FS_POP3_EV_DELE, (FS_UINT4)buf );

		IFS_Free( buf );
	}
}

static void FS_Pop3ProcessListCmdResp( FS_Pop3Session * ss )
{
	FS_CHAR *buf = FS_Pop3ReadNetData( ss, FS_NULL );
	if( buf )
	{
		if( IFS_Strncmp( buf, "+OK", 3 ) == 0 )
			ss->cur_act->rsp_ok = FS_TRUE;

		if( ss->cur_act->rsp_ok )
			FS_Pop3HandleListResult( ss, buf );
		else
			ss->handler( (FS_UINT4)ss, FS_POP3_EV_SERVER_ERR, (FS_UINT4)buf );
		
		IFS_Free( buf );
	}
}

static void FS_Pop3ProcessNetData( FS_Pop3Session * ss )
{		
	if( ss->status == FS_POP3_STS_NONE )
		FS_Pop3ProcessGreeting( ss );
	else if( ss->status == FS_POP3_STS_AUTH_SEND_USER )
		FS_Pop3ProcessUserCmdResp( ss );
	else if( ss->status == FS_POP3_STS_AUTH_SEND_PASS )
		FS_Pop3ProcessPassCmdResp( ss );
	else if( ss->status == FS_POP3_STS_TRANS 
		&& ss->cur_act && ss->cur_act->act == FS_POP3_ACT_UIDL )
		FS_Pop3ProcessUidlCmdResp( ss );
	else if( ss->status == FS_POP3_STS_TRANS 
		&& ss->cur_act && ss->cur_act->act == FS_POP3_ACT_TOP )
		FS_Pop3ProcessTopCmdResp( ss );
	else if( ss->status == FS_POP3_STS_TRANS 
		&& ss->cur_act && ss->cur_act->act == FS_POP3_ACT_RETR )
		FS_Pop3ProcessRetrCmdResp( ss );
	else if( ss->status == FS_POP3_STS_TRANS 
		&& ss->cur_act && ss->cur_act->act == FS_POP3_ACT_DELE )
		FS_Pop3ProcessDeleCmdResp( ss );
	else if( ss->status == FS_POP3_STS_TRANS 
		&& ss->cur_act && ss->cur_act->act == FS_POP3_ACT_LIST )
		FS_Pop3ProcessListCmdResp( ss );
}

static void FS_Pop3SockEventHandler( FS_UINT4 sockid, FS_ISockEvent ev, FS_UINT4 param )
{
	FS_Pop3Session *ss = FS_Pop3GetSession( sockid );
	if( ss )
	{
		if( FS_ISOCK_SENDOK == ev )
		{
			if( ss->status == FS_POP3_STS_SEND_QUIT )
			{
				IFS_SocketClose( ss->socket );
				FS_ListDel( &ss->list );
				IFS_Free( ss );
			}
		}
		else if( FS_ISOCK_READ == ev )
		{
			FS_Pop3ProcessNetData( ss );
		}
		else if( FS_ISOCK_ERROR == ev )
		{
			IFS_SocketClose( ss->socket );
			ss->socket = 0;
			ss->status = FS_POP3_STS_NONE;
			ss->handler( (FS_UINT4)ss, FS_POP3_EV_NET_ERR, 0 );
		}
	}
}

static void FS_Pop3SessionInit( FS_Pop3Session *ss )
{
	FS_EmlAccount *act = FS_EmlGetActiveAct( );
	FS_Pop3ResetCurAction( ss );
	if( act )
	{
		if( ! IFS_SocketCreate( &ss->socket, FS_TRUE, FS_Pop3SockEventHandler )
			|| ! IFS_SocketConnect( ss->socket, act->recv_addr, act->recv_port ) )
		{
			ss->handler( (FS_UINT4)ss, FS_POP3_EV_NET_ERR, 0 );
		}
	}
}

FS_UINT4 FS_Pop3CreateSession( FS_Pop3EventHandler handler )
{
	FS_Pop3Session *session = IFS_Malloc( sizeof(FS_Pop3Session) );
	if( session )
	{
		IFS_Memset( session, 0, sizeof(FS_Pop3Session) );
		FS_ListInit( &session->uidl );
		FS_ListAdd( &GFS_Pop3SessionList, &session->list );
		session->handler = handler;
	}
	return (FS_UINT4)session;
}

void FS_Pop3GetUidl( FS_UINT4 session, FS_SINT4 msgId )
{
	FS_Pop3Session *ss = (FS_Pop3Session *)session;
	FS_Pop3Action *action;
	/* clear the eacho uidl if any */
	FS_Pop3FreeUidl( ss );
	
	if( ss->status == FS_POP3_STS_TRANS )
		action = &ss->actions[0];
	else
		action = &ss->actions[1];
	
	FS_Pop3ResetCurAction( ss );
	if( msgId != -1 )
	{
		IFS_Sprintf( action->cmd, "UIDL %d\r\n", msgId );
		action->param = FS_FALSE;	/* single line response */
	}
	else
	{
		IFS_Strcpy( action->cmd, "UIDL\r\n" );
		action->param = FS_TRUE;
	}

	action->act = FS_POP3_ACT_UIDL;
	if( ss->status == FS_POP3_STS_TRANS )
	{
		ss->cur_act = &ss->actions[0];
		IFS_SocketSend( ss->socket, ss->cur_act->cmd, IFS_Strlen(ss->cur_act->cmd) );
	}
	else
		FS_Pop3SessionInit( ss );
}

void FS_Pop3GetTop( FS_UINT4 session, FS_CHAR *uid )
{
	FS_Pop3Session *ss = (FS_Pop3Session *)session;
	if( ss->status == FS_POP3_STS_TRANS && ! FS_ListIsEmpty(&ss->uidl) )
	{
		FS_SINT4 id = FS_Pop3GetMsgId( ss, uid );
		if( id < 0 )
		{
			ss->handler( (FS_UINT4)ss, FS_POP3_EV_MSG_NOT_EXIST, 0 );
		}
		else
		{
			FS_Pop3ResetCurAction( ss );
			ss->cur_act = &ss->actions[0];
			ss->cur_act->act = FS_POP3_ACT_TOP;
			IFS_Sprintf( ss->cur_act->cmd, "TOP %d 0\r\n", id );
			IFS_SocketSend( ss->socket, ss->cur_act->cmd, IFS_Strlen(ss->cur_act->cmd) );
		}
	}
	else		
	{	// here, we must fetch uidl first
		ss->actions[2].act = FS_POP3_ACT_TOP;
		// save the uid for later use
		IFS_Strcpy( ss->actions[2].cmd, uid );
		FS_Pop3GetUidl( session, -1 );		
	}
	
}

FS_SINT4 FS_Pop3GetSize( FS_UINT4 session, FS_CHAR *uid )
{
	FS_Pop3Session *ss = (FS_Pop3Session *)session;
	FS_SINT4 size = 0;
	if( ! FS_ListIsEmpty(&ss->uidl) )
		size = FS_Pop3GetMsgSize( ss, uid );
	return size;
}

void FS_Pop3RetrMail( FS_UINT4 session, FS_CHAR *uid )
{
	FS_Pop3Session *ss = (FS_Pop3Session *)session;
	if( ss->status == FS_POP3_STS_TRANS && ! FS_ListIsEmpty(&ss->uidl) )
	{
		FS_SINT4 id = FS_Pop3GetMsgId( ss, uid );
		if( id < 0 )
		{
			ss->handler( (FS_UINT4)ss, FS_POP3_EV_MSG_NOT_EXIST, 0 );
		}
		else
		{
			FS_Pop3ResetCurAction( ss );
			ss->cur_act = &ss->actions[0];
			ss->cur_act->act = FS_POP3_ACT_RETR;
			IFS_Sprintf( ss->cur_act->cmd, "RETR %d\r\n", id );
			IFS_SocketSend( ss->socket, ss->cur_act->cmd, IFS_Strlen(ss->cur_act->cmd) );
		}
	}
	else		
	{	// here, we must fetch uidl first
		ss->actions[2].act = FS_POP3_ACT_RETR;
		// save the uid for later use
		IFS_Strcpy( ss->actions[2].cmd, uid );
		FS_Pop3GetUidl( session, -1 );		
	}
}

void FS_Pop3DeleMail( FS_UINT4 session, FS_CHAR *uid )
{
	FS_Pop3Session *ss = (FS_Pop3Session *)session;
	if( ss->status == FS_POP3_STS_TRANS && ! FS_ListIsEmpty(&ss->uidl) )
	{
		FS_SINT4 id = FS_Pop3GetMsgId( ss, uid );
		if( id < 0 )
		{
			ss->handler( (FS_UINT4)ss, FS_POP3_EV_MSG_NOT_EXIST, 0 );
		}
		else
		{
			FS_Pop3ResetCurAction( ss );
			ss->cur_act = &ss->actions[0];
			ss->cur_act->act = FS_POP3_ACT_DELE;
			IFS_Sprintf( ss->cur_act->cmd, "DELE %d\r\n", id );
			IFS_SocketSend( ss->socket, ss->cur_act->cmd, IFS_Strlen(ss->cur_act->cmd) );
		}
	}
	else		
	{	// here, we must fetch uidl first
		ss->actions[2].act = FS_POP3_ACT_DELE;
		// save the uid for later use
		IFS_Strcpy( ss->actions[2].cmd, uid );
		FS_Pop3GetUidl( session, -1 );		
	}
}

void FS_Pop3QuitSession( FS_UINT4 session )
{
	FS_Pop3Session *ss = (FS_Pop3Session *)session;
	FS_Pop3FreeUidl( ss );
	FS_Pop3ResetCurAction( ss );
	if( ss->status != FS_POP3_STS_NONE )
	{
		// must send QUIT cmd to post any DEL cmd
		ss->cur_act = &ss->actions[0];
		IFS_Strcpy( ss->cur_act->cmd, "QUIT\r\n" );
		ss->status = FS_POP3_STS_SEND_QUIT;
		IFS_SocketSend( ss->socket, ss->cur_act->cmd, IFS_Strlen(ss->cur_act->cmd) );
	}
	else
	{
		IFS_SocketClose( ss->socket );
		FS_ListDel( &ss->list );
		IFS_Free( ss );
	}
}

#endif	//FS_MODULE_EML

