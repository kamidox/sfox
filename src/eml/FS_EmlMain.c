#include "inc/FS_Config.h"

#ifdef FS_MODULE_EML

#include "inc\FS_Config.h"
#include "inc\inte\FS_Inte.h"
#include "inc\res\FS_Res.h"
#include "inc\gui\FS_Gui.h"
#include "inc\util\FS_File.h"
#include "inc\util\FS_Util.h"
#include "inc\eml\FS_EmlAct.h"
#include "inc\eml\FS_EmlFile.h"
#include "inc\eml\FS_Rfc822.h"
#include "inc\eml\FS_Pop3.h"
#include "inc\eml\FS_Smtp.h"
#include "inc\util\FS_NetConn.h"
#include "inc\util\FS_MemDebug.h"

#define FS_EML_DETAIL_LEN	1024
#define FS_EML_MAX_ACT_NUM	5

typedef enum FS_EmlAct_Tag
{
	FS_EmlActNone = 0,
	FS_EmlActRetrMail,
	FS_EmlActRetrHead,
	FS_EmlActDele,
	FS_EmlActSend
}FS_EmlAct;
//-----------------------------------------------------------------------------------

typedef struct FS_EmlUIContext_Tag
{
	FS_EmlAct		action;
	FS_UINT4		pop3_sess;
	FS_UINT4		smtp_sess;
	FS_UINT4		timer_id;
	FS_SINT2		text_id;
	FS_SINT4		second;
	FS_Window *		prograss_win;
	FS_SINT4		index;
	FS_SINT4		total;
	FS_SINT4		offset;
	FS_SINT4		size;
	FS_CHAR *		uid;
	FS_BOOL			complete;
}FS_EmlUIContext;

static FS_EmlUIContext GFS_EmlUICtx;

static FS_EmlAct GFS_EmlAct = FS_EmlActNone;

//-----------------------------------------------------------------------------------
// local function declare here
static void FS_EmlActBuildList( void );
static void FS_EmlNewAct_CB( FS_Window *win );
static void FS_EmlEditFrm_UI( FS_EmlFile *emlFile, FS_BOOL bView );
static void FS_EmlAttachDetail_CB( FS_Window *win );

static FS_BOOL FS_EmlSysInit( void )
{
	FS_SystemInit( );
	FS_EmlInitAct( );
	return FS_TRUE;
}

static FS_BOOL FS_EmlCreateActCnf_CB( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	if( wparam == FS_EV_YES )	// exit without save account data
	{
		FS_EmlNewAct_CB( FS_NULL );
	}
	return ret;
}

static FS_BOOL FS_EmlAccountExist( void )
{
	FS_BOOL ret = FS_FALSE;
	if( FS_EmlGetActiveAct() )
		ret = FS_TRUE;
	else
	{
		ret = FS_FALSE;
		FS_MessageBox( FS_MS_YES_NO, FS_Text(FS_T_EML_ACT_EMPTY_CREATE), FS_EmlCreateActCnf_CB, FS_FALSE);
	}
	return ret;
}

static void FS_EmlGetAddrDispName( FS_CHAR *out, FS_SINT4 len, FS_List *head, FS_BOOL bView )
{
	FS_EmlAddr *addr;
	FS_List *node = head->next;
	FS_SINT4 slen, tlen;
	if( out )
	{
		IFS_Memset( out, 0, len );
		slen = 0;
		while( node != head )
		{
			FS_BOOL bDispName;
			addr = FS_ListEntry( node, FS_EmlAddr, list );
			tlen = IFS_Strlen(addr->name);
			if( tlen > 0 && slen + tlen < len )
			{
				IFS_Memcpy( out + slen, addr->name, tlen );
				slen += tlen;
				bDispName = FS_TRUE;
			}
			else
			{
				bDispName = FS_FALSE;
			}
			tlen = IFS_Strlen(addr->addr);
			if( tlen > 0 && slen + tlen < len - 3 )
			{
				if( bDispName )
				{
					out[slen ++] = ' ';
					out[slen ++] = '<';
				}
				IFS_Memcpy( out + slen, addr->addr, tlen );
				slen += tlen;
				if( bDispName )
					out[slen ++] = '>';
			}
			
			node = node->next;
			if( node != head )
			{
				IFS_Strcpy( out + slen, ", " );
				slen += 2;
			}
#if 0
			FS_CHAR *dname;
			addr = FS_ListEntry( node, FS_EmlAddr, list );
			slen = IFS_Strlen( out );
			if( bView && IFS_Strlen(addr->name) > 0 )
				dname = addr->name;
			else
				dname = addr->addr;

			if( slen + IFS_Strlen(dname) < len )
				IFS_Strcpy( out + slen, dname );
			else
				break;
			
			node = node->next;
			if( node != head )
				IFS_Strcat( out, ", " );
#endif			
		}
	}
}

static FS_UINT1 FS_EmlGetCurMBox( void )
{
	FS_UINT1 mbox = FS_EmlInbox;
	FS_Window *mwin = FS_WindowFindId( FS_W_EmlMainFrm );
	FS_Widget *mailbox = FS_WindowGetFocusItem( mwin );
	if( mailbox )
		mbox = (FS_UINT1)mailbox->private_data;
	
	return mbox;
}

static FS_CHAR * FS_EmlGetCurMailUid( void )
{
	FS_CHAR *uid = FS_NULL;
	FS_Window *hwin = FS_WindowFindId( FS_W_EmlHeadListFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( hwin );
	if( wgt )
		uid = wgt->data;
	
	return uid;
}

static void FS_EmlSetReadFlag( FS_UINT1 mbox, FS_CHAR *uid )
{
	FS_Window *lstWin = FS_WindowFindId( FS_W_EmlHeadListFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( lstWin );
	FS_EmlSetReaded( mbox, uid, FS_TRUE );
	if( wgt ){
		FS_WidgetSetIcon( wgt, FS_I_READED_MSG );
	}
}

static void FS_EmlEditFormSetUid( FS_CHAR *uid )
{
	FS_CHAR *ouid;
	FS_Window *ewin = FS_WindowFindId( FS_W_EmlEditFrm );
	if( ewin )
	{
		if( ewin->private_data2 )
		{
			ouid = (FS_CHAR *)ewin->private_data2;
			IFS_Free( ouid );
		}
		ewin->private_data2 = (FS_UINT4)IFS_Strdup( uid );
	}
}

static FS_CHAR *FS_EmlEditFormGetUid( void )
{
	FS_CHAR *uid = FS_NULL;
	FS_Window *ewin = FS_WindowFindId( FS_W_EmlEditFrm );
	if( ewin )
	{
		uid = (FS_CHAR *)ewin->private_data2;
	}
	return uid;
}

//-----------------------------------------------------------------------------------
// exit the email main window
static void FS_EmlExit_CB( FS_Window *win )
{
	FS_EmlDeinitAct( );
	FS_EmlDeinitHead( );
	FS_NetDisconnect( FS_APP_EML );
	FS_DeactiveApplication( FS_APP_EML );
	if( ! FS_HaveActiveApplication() )
	{
		FS_GuiExit( );
		IFS_SystemExit( );
	}
}

static void FS_EmlSelectActType_CB( FS_Window *win )
{
	FS_Window *parent = FS_WindowFindId( FS_W_EmlActEditFrm );
	if( parent )
	{
		FS_Widget *actType = FS_WindowGetFocusItem( parent );
		FS_Widget *wgt = FS_WindowGetFocusItem( win );
		FS_WidgetSetText( actType, wgt->data );
		if( IFS_Strcmp(wgt->data,  FS_Text(FS_T_EML_POP3)) == 0 )
		{
			wgt = FS_WindowGetWidget( parent, FS_W_EmlRecvAddr );
			wgt->tip = FS_Text( FS_T_EML_TIP_POP3_ADDR );
			wgt = FS_WindowGetWidget( parent, FS_W_EmlRecvPort );
			wgt->tip = FS_Text( FS_T_EML_TIP_POP3_PORT );
			FS_WidgetSetText( wgt, "110" );
		}
		else
		{
			wgt = FS_WindowGetWidget( parent, FS_W_EmlRecvAddr );
			wgt->tip = FS_Text( FS_T_EML_TIP_IMAP4_ADDR );
			wgt = FS_WindowGetWidget( parent, FS_W_EmlRecvPort );
			wgt->tip = FS_Text( FS_T_EML_TIP_IMAP4_PORT );
			FS_WidgetSetText( wgt, "143" );
		}
		if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
			FS_DestroyWindow( win );
	}
}
//-----------------------------------------------------------------------------------
// exit the email main window
static void FS_EmlSelectActType_UI( FS_Window *win )
{
	FS_Window *popMenu;
	FS_Widget *itemPop3, *itemImap4;
	FS_Widget *focusWgt = FS_WindowGetFocusItem( win );
	if( focusWgt )
	{
		FS_Rect rect = FS_GetWidgetDrawRect( focusWgt );
		popMenu = FS_CreatePopUpMenu( FS_W_EmlActTypeMenu, &rect, 2 );
		itemPop3 = FS_CreateMenuItem( 0,  FS_Text(FS_T_EML_POP3) );
		itemImap4 = FS_CreateMenuItem( 0,  FS_Text(FS_T_EML_IMAP4) );
		itemPop3->data = IFS_Strdup( FS_Text(FS_T_EML_POP3) );
		itemImap4->data = IFS_Strdup( FS_Text(FS_T_EML_IMAP4) );
		FS_WidgetSetHandler( itemPop3, FS_EmlSelectActType_CB );
		FS_WidgetSetHandler( itemImap4, FS_EmlSelectActType_CB );		
		FS_MenuAddItem( popMenu, itemPop3 );
		FS_MenuAddItem( popMenu, itemImap4 );

		FS_MenuSetSoftkey( popMenu );

		FS_ShowWindow( popMenu );
	}
}

static void FS_EmlResetNetUICtx( FS_EmlUIContext *emlUICtx )
{
	if( emlUICtx->action == FS_EmlActNone ) return;
	
	IFS_LeaveBackLight( );
	if( emlUICtx->pop3_sess )
	{
		FS_Pop3QuitSession( emlUICtx->pop3_sess );
		emlUICtx->pop3_sess = 0;
	}
	if( emlUICtx->smtp_sess )
	{
		FS_SmtpQuitSession( emlUICtx->smtp_sess );
		emlUICtx->smtp_sess = 0;
	}
	if( emlUICtx->timer_id )
	{
		IFS_StopTimer( emlUICtx->timer_id);
		emlUICtx->timer_id= 0;
	}
	FS_SAFE_FREE( emlUICtx->uid );
	IFS_Memset( emlUICtx, 0, sizeof(FS_EmlUIContext) );
}

static void FS_EmlDisplayNetResult( FS_EmlUIContext *emlUICtx, FS_CHAR *extra_text )
{
	FS_CHAR *str;

	if( emlUICtx->action == FS_EmlActNone ) return;
	
	if( emlUICtx->timer_id ){
		IFS_StopTimer( emlUICtx->timer_id );
		emlUICtx->timer_id = 0;
	}

	emlUICtx->complete = FS_TRUE;
	str = FS_StrConCat( FS_Text(emlUICtx->text_id), "\r\n", extra_text, FS_NULL );
	FS_MsgBoxSetText( emlUICtx->prograss_win, str );
	FS_WindowSetSoftkey( emlUICtx->prograss_win, 3, FS_Text(FS_T_OK), FS_StandardKey3Handler );
	FS_RedrawSoftkeys( emlUICtx->prograss_win );
	IFS_Free( str );

	FS_EmlResetNetUICtx( emlUICtx );
}

static void FS_EmlDisplayNetPrograss( FS_EmlUIContext *emlUICtx )
{
	FS_CHAR str[64];

	if( emlUICtx->total > 0 || emlUICtx->size > 0 || emlUICtx->offset > 0 )
	{
		if( emlUICtx->size > 0 && emlUICtx->offset >= 0 )
		{
			IFS_Sprintf( str, "%d-%s (%dK/%dK)", emlUICtx->second, FS_Text(emlUICtx->text_id),
				FS_KB(emlUICtx->offset), FS_KB(emlUICtx->size) );
		}
		else if( emlUICtx->offset > 0 )
		{
			IFS_Sprintf( str, "%d-%s (%dK)", emlUICtx->second, FS_Text(emlUICtx->text_id), FS_KB(emlUICtx->offset) );
		}
		else
		{
			IFS_Sprintf( str, "%d-%s(%d/%d)", emlUICtx->second, FS_Text(emlUICtx->text_id),
				emlUICtx->index, emlUICtx->total );
		}
	}
	else
	{
		IFS_Sprintf( str, "%d-%s", emlUICtx->second, FS_Text(emlUICtx->text_id) );
	}
	FS_MsgBoxSetText( emlUICtx->prograss_win, str );
}

static void FS_EmlPop3CheckUidl( FS_EmlUIContext *emlUICtx, FS_List *uidl )
{
	FS_Pop3Uidl *uid;
	FS_SINT4 total = 0;
	FS_Window *win = FS_WindowFindId( FS_W_EmlRetrProcessFrm );
	FS_List *node = uidl->next;

	FS_EmlUpdateLocalUidl( uidl );
	while( node != uidl )
	{
		uid = FS_ListEntry( node, FS_Pop3Uidl, list );
		// did not exist in local, retrive it
		if( ! FS_EmlCheckLocalUid( uid->uid ) )
		{
			FS_EmlPushPending( uid->uid );
			total ++;
		}
		node = node->next;
	}
	if( total > 0 )
	{
		emlUICtx->total = total;
		emlUICtx->index = 0;
		emlUICtx->text_id = FS_T_RECVING;
		FS_EmlDisplayNetPrograss( emlUICtx );
	}
	else
	{
		emlUICtx->text_id = FS_T_EML_NO_NEW;
		FS_EmlDisplayNetResult( emlUICtx, FS_NULL );
	}

	if( FS_EmlPopPending( ) )
	{
		if( FS_EmlConfigGetRetrMode( ) )
		{
			FS_Pop3GetTop( emlUICtx->pop3_sess, FS_EmlPopPending( ) );
		}
		else
		{
			if( FS_EmlIsFull( 1024 ) )
			{
				emlUICtx->text_id = FS_T_SPACE_IS_FULL;
				FS_EmlDisplayNetResult( emlUICtx, FS_NULL );
				return;
			}
			FS_Pop3RetrMail( emlUICtx->pop3_sess, FS_EmlPopPending() );
		}
	}
}

static void FS_EmlSaveTopToInbox( FS_EmlUIContext *emlUICtx, FS_CHAR *topstr )
{
	FS_EmlHead emlHead;
	FS_SINT4 recved, total = emlUICtx->total;
	FS_UINT4 ss = emlUICtx->pop3_sess;
	FS_CHAR *uid = FS_EmlPopPending();
	/* msg does not exist */
	if( topstr == FS_NULL )
	{
		FS_EmlRemovePending( uid );
	}
	else
	{
		IFS_Memset( &emlHead, 0, sizeof(FS_EmlHead) );
		FS_EmlParseTop( &emlHead, &topstr );
		IFS_Strncpy( emlHead.uid, uid, FS_EML_UID_LEN - 1 );
		emlHead.msg_size = FS_Pop3GetSize( ss, uid );
		FS_EmlRemovePending( uid );
		FS_EmlAddHead( FS_EmlInbox, &emlHead );
		if( emlHead.subject )
			IFS_Free( emlHead.subject );

		if( FS_EmlIsFull( 0 ) )
		{
			emlUICtx->text_id = FS_T_SPACE_IS_FULL;
			FS_EmlDisplayNetResult( emlUICtx, FS_NULL );
			return;
		}
	}
	recved = total - FS_EmlGetPendingCount( );
	if( recved < total )
	{
		emlUICtx->index = recved;
		emlUICtx->text_id = FS_T_RECVING;
		FS_EmlDisplayNetPrograss( emlUICtx );
		FS_Pop3GetTop( ss, FS_EmlPopPending( ) );
	}
	else
	{
		emlUICtx->text_id = FS_T_RECV_OK;
		FS_EmlDisplayNetResult( emlUICtx, FS_NULL );
	}
}

static void FS_EmlSaveMail2Local( FS_CHAR *file )
{
	FS_CHAR *uid = FS_EmlGetCurMailUid( );
	FS_EmlHead emlHead, *pHead;
	if( uid && file )
	{
		pHead = FS_EmlGetHead( FS_EmlInbox, uid );
		if( pHead )
		{
			IFS_Memcpy( &emlHead, pHead, sizeof(FS_EmlHead) );
			if( pHead->subject )
				emlHead.subject = IFS_Strdup( pHead->subject );
			IFS_Strcpy( emlHead.file, file );
			emlHead.local = FS_TRUE;
			emlHead.read = FS_FALSE;
			FS_FileMove( FS_DIR_TMP, file, FS_DIR_EML, file );
			FS_EmlMoveHead( &emlHead, FS_EmlInbox, FS_EmlLocal );
			if( emlHead.subject )
				IFS_Free( emlHead.subject );
		}
	}
}

static void FS_EmlPop3HandleRetrResult( FS_EmlUIContext *emlUICtx, FS_CHAR *file )
{
	FS_Window *lwin;
	FS_Widget *wgt;
	FS_SINT4 recved, total;
	FS_UINT4 ss;
	FS_CHAR *uid;
	
	lwin = FS_WindowFindId( FS_W_EmlHeadListFrm );
	ss = emlUICtx->pop3_sess;
	
	if( emlUICtx->action == FS_EmlActRetrMail )
	{
		/* retrieve one mail */
		FS_EmlSaveMail2Local( file );	
		/* if config to server backup */
		if( FS_EmlConfigGetServerBackup( ) )
		{
			wgt = FS_WindowGetFocusItem( lwin );
			FS_WindowDelWidget( lwin, wgt );
			emlUICtx->text_id = FS_T_RECV_OK;
			FS_EmlDisplayNetResult( emlUICtx, FS_NULL );
		}
		else
		{
			uid = FS_EmlGetCurMailUid( );
			FS_Pop3DeleMail( ss, uid );
			wgt = FS_WindowGetFocusItem( lwin );
			FS_WindowDelWidget( lwin, wgt );
		}
	}
	else	
	{
		/* retrieve all new mail */
		FS_EmlHead emlHead;
		FS_CHAR *buf, *data;
		IFS_Memset( &emlHead, 0, sizeof(FS_EmlHead));
		buf = IFS_Malloc(FS_EML_HEAD_MAX_LEN);
		FS_ASSERT( buf != FS_NULL );
		if( buf )
		{
			IFS_Memset( buf, 0, FS_EML_HEAD_MAX_LEN );
			FS_FileRead( FS_DIR_TMP, file, 0, buf, FS_EML_HEAD_MAX_LEN - 1 );
			data = buf;
			FS_EmlParseTop( &emlHead, &data );
			IFS_Free( buf );
			
			emlHead.msg_size = FS_FileGetSize( FS_DIR_TMP, file );			
			if( FS_EmlIsFull( emlHead.msg_size ) )
			{
				FS_FileDelete( FS_DIR_TMP, file );
				FS_SAFE_FREE( emlHead.subject );
				emlUICtx->text_id = FS_T_SPACE_IS_FULL;
				FS_EmlDisplayNetResult( emlUICtx, FS_NULL );
				return;
			}

			uid = FS_EmlPopPending();
			IFS_Strcpy( emlHead.file, file );
			IFS_Strncpy( emlHead.uid, uid, sizeof(emlHead.uid) - 1 );
			FS_EmlAddHead( FS_EmlLocal, &emlHead );
			FS_SAFE_FREE( emlHead.subject );
			FS_FileMove( FS_DIR_TMP, file, FS_DIR_EML, file );
			FS_EmlRemovePending( uid );
		}
		total = emlUICtx->total;
		recved = total - FS_EmlGetPendingCount( );
		if( recved < total )
		{
			emlUICtx->index = recved;
			emlUICtx->text_id = FS_T_RECVING;
			emlUICtx->offset = 0;
			emlUICtx->size = 0;
			FS_EmlDisplayNetPrograss( emlUICtx );
			
			if( FS_EmlConfigGetServerBackup() )
				FS_Pop3RetrMail( ss, FS_EmlPopPending( ) );
			else
				FS_Pop3DeleMail( ss, emlHead.uid );
		}
		else
		{
			if( FS_EmlConfigGetServerBackup() )
			{
				emlUICtx->text_id = FS_T_RECV_OK;
				FS_EmlDisplayNetResult( emlUICtx, FS_NULL );
			}
			else
			{
				FS_Pop3DeleMail( ss, emlHead.uid );
			}
		}
	}
}

static void FS_EmlNetCounter_CB( FS_EmlUIContext *emlUICtx )
{
	FS_ASSERT( emlUICtx == &GFS_EmlUICtx );
	if( emlUICtx != &GFS_EmlUICtx || emlUICtx->prograss_win == FS_NULL ) return;

	emlUICtx->timer_id = 0;
	emlUICtx->second ++;
	FS_EmlDisplayNetPrograss( emlUICtx );
	emlUICtx->timer_id = IFS_StartTimer( FS_TIMER_ID_EML_NET, 1000, FS_EmlNetCounter_CB, emlUICtx );
}

static void FS_EmlPop3HandleDele( FS_EmlUIContext *emlUICtx )
{
	
	if( emlUICtx->action == FS_EmlActRetrHead )
	{
		if( FS_EmlPopPending() )
		{
			FS_Pop3RetrMail( emlUICtx->pop3_sess, FS_EmlPopPending( ) );
		}
		else
		{
			emlUICtx->text_id = FS_T_RECV_OK;
			FS_EmlDisplayNetResult( emlUICtx, FS_NULL );
		}
	}
	else /* if( GFS_EmlAct == FS_EmlActRetrMail || GFS_EmlAct == FS_EmlActDele ) */
	{
		if( emlUICtx->action == FS_EmlActRetrMail )
			emlUICtx->text_id = FS_T_RECV_OK;
		else
			emlUICtx->text_id = FS_T_SUCCESS;
		FS_EmlDisplayNetResult( emlUICtx, FS_NULL );
	}
}

static void FS_EmlPop3EventHandler( FS_UINT4 ss, FS_Pop3Event ev, FS_UINT4 param )
{
	FS_EmlUIContext *emlUICtx = &GFS_EmlUICtx;
	FS_ASSERT( emlUICtx->pop3_sess == ss );
	if( emlUICtx->pop3_sess != ss ) return;

	if( emlUICtx->complete ) return;
	
	if( ev == FS_POP3_EV_AUTH )
	{
		emlUICtx->text_id = FS_T_AUTH;
		FS_EmlDisplayNetPrograss( emlUICtx );
	}
	else if( ev == FS_POP3_EV_NET_ERR ){
		emlUICtx->text_id = FS_T_NET_ERR;
		FS_EmlDisplayNetResult( emlUICtx, FS_NULL );
	}
	else if( ev == FS_POP3_EV_SERVER_ERR )
	{
		emlUICtx->text_id = FS_T_SERVER_ERR;
		FS_EmlDisplayNetResult( emlUICtx, (FS_CHAR *)param );
	}
	else if( ev == FS_POP3_EV_MEMORY_ERR )
	{
		emlUICtx->text_id = FS_T_MEMORY_ERR;
		FS_EmlDisplayNetResult( emlUICtx, FS_NULL );
	}
	else if( ev == FS_POP3_EV_MSG_NOT_EXIST )
	{
		if( emlUICtx->action != FS_EmlActRetrHead )
		{
			emlUICtx->text_id = FS_T_EML_NOT_EXIST;
			FS_EmlDisplayNetResult( emlUICtx, FS_NULL );
		}
	}
	else if( ev == FS_POP3_EV_UIDL )
	{
		FS_EmlPop3CheckUidl( emlUICtx, (FS_List *)param );			
	}
	else if( ev == FS_POP3_EV_TOP )
	{
		FS_EmlSaveTopToInbox( emlUICtx, (FS_CHAR *)param );	
	}
	else if( ev == FS_POP3_EV_RETR_DATA )
	{
		emlUICtx->offset = (FS_SINT4)param;
		emlUICtx->text_id = FS_T_RECVING;
		FS_EmlDisplayNetPrograss( emlUICtx );
	}
	else if( ev == FS_POP3_EV_RETR )
	{
		FS_EmlPop3HandleRetrResult( emlUICtx, (FS_CHAR *)param );	
	}
	else if( ev == FS_POP3_EV_DELE )
	{
		FS_EmlPop3HandleDele( emlUICtx );
	}
}

static FS_BOOL FS_EmlPop3DlgProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_EmlUIContext *emlUICtx = &GFS_EmlUICtx;
	FS_BOOL ret = FS_FALSE;

	if( cmd == FS_WM_DESTROY )
	{
		FS_EmlResetNetUICtx( emlUICtx );
		ret = FS_TRUE;
	}
	return ret;
}

static void FS_EmlSmtpEventHandler( FS_UINT4 ss, FS_Pop3Event ev, FS_UINT4 param )
{
	FS_EmlUIContext *emlUICtx = &GFS_EmlUICtx;
	FS_ASSERT( emlUICtx->smtp_sess == ss );
	if( emlUICtx->smtp_sess != ss ) return;
	if( emlUICtx->complete ) return;

	if( ev == FS_SMTP_EV_AUTH )
	{
		emlUICtx->text_id = FS_T_AUTH;
		FS_EmlDisplayNetPrograss( emlUICtx );
	}
	else if( ev == FS_SMTP_EV_NET_ERR )
	{
		emlUICtx->text_id = FS_T_NET_ERR;
		FS_EmlDisplayNetResult( emlUICtx, FS_NULL );
	}
	else if( ev == FS_SMTP_EV_SERVER_ERR )
	{
		emlUICtx->text_id = FS_T_SERVER_ERR;
		FS_EmlDisplayNetResult( emlUICtx, (FS_CHAR *)param );
	}
	else if( ev == FS_SMTP_EV_MEMORY_ERR )
	{
		emlUICtx->text_id = FS_T_MEMORY_ERR;
		FS_EmlDisplayNetResult( emlUICtx, (FS_CHAR *)param );
	}
	else if( ev == FS_SMTP_EV_SENDING )
	{
		emlUICtx->text_id = FS_T_SENDING;
		emlUICtx->offset = (FS_SINT4)param;
		FS_EmlDisplayNetPrograss( emlUICtx );
	}
	else if( ev == FS_SMTP_EV_SEND_OK )
	{
		FS_Window *lwin;
		FS_UINT1 mbox = FS_EmlGetCurMBox();
		FS_DestroyWindow( FS_WindowFindId(FS_W_EmlEditFrm) );
		lwin = FS_WindowFindId( FS_W_EmlHeadListFrm );
		/* when create mail / draft box / local box reply. we need to save to out box */
		if( mbox != FS_EmlInbox || lwin == FS_NULL )
		{
			if( FS_EmlConfigGetSaveSend() )
				FS_EmlMoveMBox( FS_EmlDraft, FS_EmlOutbox, emlUICtx->uid );
			else
				FS_EmlDeleteFile( FS_EmlDraft, emlUICtx->uid, FS_FALSE );
			if( mbox == FS_EmlDraft && lwin )
				FS_WindowDelWidget( lwin, FS_WindowGetFocusItem(lwin) );
		}
		emlUICtx->text_id = FS_T_SENDOK;
		FS_EmlDisplayNetResult( emlUICtx, FS_NULL );
	}
}

static void FS_EmlNetConnCallback( FS_EmlUIContext *emlUICtx, FS_BOOL bOK )
{	
	FS_EmlHead *emlHead;
	FS_List head;

	if( emlUICtx!= &GFS_EmlUICtx ) return;
	if( emlUICtx->action == FS_EmlActNone ) return;
	if( ! bOK ){
		emlUICtx->text_id = FS_T_DIAL_FAILED;
		FS_EmlDisplayNetResult( emlUICtx, FS_NULL );
		return;
	}
	
	if( emlUICtx->action == FS_EmlActRetrHead ){
		emlUICtx->pop3_sess = FS_Pop3CreateSession( FS_EmlPop3EventHandler );
		FS_EmlClearPending( );
		FS_Pop3GetUidl( emlUICtx->pop3_sess, -1 );
	}else if( emlUICtx->action == FS_EmlActRetrMail ){
		emlUICtx->pop3_sess = FS_Pop3CreateSession( FS_EmlPop3EventHandler );
		FS_Pop3RetrMail( emlUICtx->pop3_sess, emlUICtx->uid );
	}else if( emlUICtx->action == FS_EmlActSend ){		
		emlHead = FS_EmlGetHead( FS_EmlDraft, emlUICtx->uid );
		if( emlHead == FS_NULL )
			emlHead = FS_EmlGetHead( FS_EmlOutbox, emlUICtx->uid );
		FS_ASSERT( emlHead != FS_NULL );
		FS_ListInit( &head );
		FS_EmlParseRcpt( &head, emlHead->file );
		FS_ASSERT( ! FS_ListIsEmpty(&head) );
		emlUICtx->smtp_sess = FS_SmtpCreateSession( FS_EmlSmtpEventHandler );
		FS_SmtpSendMail( emlUICtx->smtp_sess, &head, emlHead->file );
		FS_EmlFreeAddrList( &head );
	}else if( emlUICtx->action == FS_EmlActDele ){
		emlUICtx->pop3_sess = FS_Pop3CreateSession( FS_EmlPop3EventHandler );
		FS_Pop3DeleMail( emlUICtx->pop3_sess, emlUICtx->uid );
	}
}

static void FS_EmlRetrMailHead_CB( FS_Window *win )
{
	FS_Window *msgBox;
	FS_EmlUIContext *emlUICtx = &GFS_EmlUICtx;
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
	
	if( FS_EmlAccountExist() )
	{
		if( FS_EmlIsFull( 0 ) )
		{
			FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_SPACE_IS_FULL), FS_NULL, FS_FALSE );
			return;
		}
		
		IFS_EnterBackLight( );
		msgBox = FS_MessageBox( FS_MS_INFO_CANCEL, FS_NULL, FS_EmlPop3DlgProc, FS_FALSE );
		msgBox->id = FS_W_EmlRetrProcessFrm;
		
		emlUICtx->action = FS_EmlActRetrHead;
		emlUICtx->text_id = FS_T_CONNECTING;
		emlUICtx->second = 0;
		emlUICtx->timer_id = IFS_StartTimer( FS_TIMER_ID_EML_NET, 1000, FS_EmlNetCounter_CB, emlUICtx );
		emlUICtx->prograss_win = msgBox;
		FS_EmlDisplayNetPrograss( emlUICtx );

		FS_NetConnect( FS_EmlConfigGetApn(), FS_EmlConfigGetUser(), FS_EmlConfigGetPass(), 
			FS_EmlNetConnCallback, FS_APP_EML, FS_TRUE, emlUICtx );
	}
}

static void FS_EmlRetrive_CB( FS_Window *win )
{
	FS_CHAR *uid = FS_EmlGetCurMailUid( );
	FS_EmlHead *emlHead;
	FS_SINT4 maxsize;
	FS_Window *msgBox;
	FS_EmlUIContext *emlUICtx;
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );

	if( uid == FS_NULL ) return;
	
	maxsize = FS_EmlConfigGetMaxMail( );
	emlUICtx = &GFS_EmlUICtx;
	
	emlHead = FS_EmlGetHead( FS_EmlInbox, uid );
	if( emlHead->msg_size > maxsize * 1024 )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_EML_TOO_LARGE), FS_NULL, FS_TRUE );
		return;
	}
	if( FS_EmlIsFull( emlHead->msg_size ) )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_SPACE_IS_FULL), FS_NULL, FS_FALSE );
		return;
	}
	
	IFS_EnterBackLight( );
	msgBox = FS_MessageBox( FS_MS_INFO_CANCEL, FS_NULL, FS_EmlPop3DlgProc, FS_FALSE );
	msgBox->id = FS_W_EmlRetrProcessFrm;

	emlUICtx->action = FS_EmlActRetrMail;
	emlUICtx->text_id = FS_T_CONNECTING;
	emlUICtx->second = 0;
	emlUICtx->timer_id = IFS_StartTimer( FS_TIMER_ID_EML_NET, 1000, FS_EmlNetCounter_CB, emlUICtx );
	emlUICtx->prograss_win = msgBox;
	emlUICtx->size = emlHead->msg_size;
	emlUICtx->offset = 0;
	emlUICtx->uid = IFS_Strdup( uid );
	FS_EmlDisplayNetPrograss( emlUICtx );

	FS_NetConnect( FS_EmlConfigGetApn(), FS_EmlConfigGetUser(), FS_EmlConfigGetPass(), 
		FS_EmlNetConnCallback, FS_APP_EML, FS_TRUE, emlUICtx );
}

/*
	save a mail to draft, when uid is empty, add new to draft box 
	when uid is not empty, replace the mail specify by uid
*/
static void FS_EmlSaveToDraft( FS_CHAR *uid )
{
	FS_Window *lstWin = FS_WindowFindId( FS_W_EmlHeadListFrm );
	FS_Window *editWin = FS_WindowFindId( FS_W_EmlEditFrm );
	FS_EmlFile *emlFile;
	FS_EmlAccount *act;
	FS_EmlHead *head, dhead;
	FS_CHAR *str;
	FS_Widget *wgt;
	
	act = FS_EmlGetActiveAct( );

	/* here, we use a dummy head */
	if( uid == FS_NULL || IFS_Strlen(uid) == 0 )
	{
		IFS_Memset( &dhead, 0, sizeof(FS_EmlHead) );
		head = &dhead;
	}
	else
	{
		head = FS_EmlGetHead( FS_EmlGetCurMBox(), uid );
		/* must in reply email phase */
		if( head == FS_NULL )
		{
			head = FS_EmlGetHead( FS_EmlDraft, uid );
		}
	}
	
	if( editWin && act )
	{
		emlFile = (FS_EmlFile *)editWin->private_data;
		
		IFS_Strcpy( emlFile->from.addr, act->eml_addr );
		IFS_Strcpy( emlFile->from.name, act->disp_name );
		FS_EmlFreeAddrList( &emlFile->to );
		FS_EmlFreeAddrList( &emlFile->cc );
		FS_EmlFreeAddrList( &emlFile->bcc );
		FS_EmlParseAddrList( &emlFile->to, FS_WindowGetWidgetText( editWin, FS_W_EmlEditTo ) );
		FS_EmlParseAddrList( &emlFile->cc, FS_WindowGetWidgetText( editWin, FS_W_EmlEditCc ) );
		FS_EmlParseAddrList( &emlFile->bcc, FS_WindowGetWidgetText( editWin, FS_W_EmlEditBcc ) );
		str = FS_WindowGetWidgetText( editWin, FS_W_EmlEditSubject );
		FS_COPY_TEXT( emlFile->subject, str );
		str = FS_WindowGetWidgetText( editWin, FS_W_EmlEditText );
		FS_COPY_TEXT( emlFile->text , str );
		FS_EmlFileSave( FS_EmlDraft, emlFile, head );
		if( head == &dhead && head->subject )
			IFS_Free( head->subject );

		if( uid ) IFS_Strcpy( uid, head->uid );

		if( lstWin && uid )
		{
			head = FS_EmlGetHead( FS_EmlDraft, uid );
			if( head == FS_NULL )
				head = FS_EmlGetHead( FS_EmlOutbox, uid );
			if( head )
			{
				wgt = FS_WindowGetFocusItem( lstWin );
				if( head->subject == FS_NULL || head->subject[0] == 0 )
					str = FS_Text( FS_T_NO_SUBJECT );
				else
					str = head->subject;
				FS_WidgetSetText( wgt, str );
			}
		}
		
	}
}

static void FS_EmlSaveDraft_HD( void )
{
	FS_CHAR *uid, *euid;
	FS_Window * editWin = FS_WindowFindId( FS_W_EmlEditFrm );

	euid = FS_EmlEditFormGetUid( );
	uid = IFS_Malloc( FS_EML_UID_LEN );
	if( euid != FS_NULL )
		IFS_Strcpy( uid, euid );
	else
		IFS_Memset( uid, 0, FS_EML_UID_LEN );
	FS_EmlSaveToDraft( uid );
	
	if( uid ) IFS_Free( uid );
	FS_MessageBox( FS_MS_OK, FS_Text(FS_T_SAVED), FS_NULL, FS_FALSE );
	FS_DestroyWindow( editWin );
	FS_DestroyWindow( FS_WindowFindId(FS_W_ProgressFrm) );
}

static void FS_EmlSaveDraft_CB( FS_Window *win )
{
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
	FS_MessageBox( FS_MS_NONE, FS_Text(FS_T_PLS_WAITING), FS_NULL, FS_FALSE );
	IFS_PostMessage( FS_MSG_UTIL_CALL, (FS_UINT4)FS_EmlSaveDraft_HD );
}

static FS_BOOL FS_EmlSendMailDlgProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_EmlUIContext *emlUICtx = &GFS_EmlUICtx;
	FS_BOOL ret = FS_FALSE;

	if( cmd == FS_WM_DESTROY )
	{
		FS_EmlResetNetUICtx( emlUICtx );
		ret = FS_TRUE;
	}
	return ret;
}

void FS_EmlSend_HD( void )
{
	FS_EmlHead *emlHead;
	FS_List head;
	FS_Window *msgBox;
	FS_CHAR *uid, *euid;
	FS_EmlUIContext *emlUICtx = &GFS_EmlUICtx;
	
	uid = IFS_Malloc( FS_EML_UID_LEN );
	IFS_Memset( uid, 0, FS_EML_UID_LEN );
	euid = FS_EmlEditFormGetUid( );
	if( euid != FS_NULL )
		IFS_Strcpy( uid, euid );
	
	FS_EmlSaveToDraft( uid );
	
	if( euid == FS_NULL )
		FS_EmlEditFormSetUid( uid );
	emlHead = FS_EmlGetHead( FS_EmlDraft, uid );
	if( emlHead == FS_NULL )
		emlHead = FS_EmlGetHead( FS_EmlOutbox, uid );
	if( emlHead == FS_NULL )
	{
		IFS_Free( uid );
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_UNKNOW_ERR), FS_NULL, FS_FALSE );
		FS_DestroyWindowByID( FS_W_ProgressFrm );
		return;
	}
	FS_ListInit( &head );
	FS_EmlParseRcpt( &head, emlHead->file );
	if( FS_ListIsEmpty(&head) )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_INPUT_ADDR), FS_NULL, FS_FALSE );
		FS_DestroyWindowByID( FS_W_ProgressFrm );
		return;
	}
	FS_EmlFreeAddrList( &head );
	
	IFS_EnterBackLight( );
	msgBox = FS_MessageBox( FS_MS_INFO_CANCEL, FS_NULL, FS_EmlSendMailDlgProc, FS_FALSE ); 
	msgBox->id = FS_W_EmlSendProcessFrm;

	emlUICtx->action = FS_EmlActSend;
	emlUICtx->text_id = FS_T_CONNECTING;
	emlUICtx->second = 0;
	emlUICtx->timer_id = IFS_StartTimer( FS_TIMER_ID_EML_NET, 1000, FS_EmlNetCounter_CB, emlUICtx );
	emlUICtx->prograss_win = msgBox;
	emlUICtx->uid = uid;
	emlUICtx->offset = 0;
	emlUICtx->size = emlHead->msg_size;
	FS_EmlDisplayNetPrograss( emlUICtx );
	FS_DestroyWindowByID( FS_W_ProgressFrm );
	
	FS_NetConnect( FS_EmlConfigGetApn(), FS_EmlConfigGetUser(), FS_EmlConfigGetPass(), 
		FS_EmlNetConnCallback, FS_APP_EML, FS_TRUE, emlUICtx );
}

/* @todo when send failed. will save multi time of email */
static void FS_EmlSend_CB( FS_Window *win )
{
	FS_CHAR *to, *cc, *bcc;
	FS_Window *editWin;
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );

	editWin = FS_WindowFindId( FS_W_EmlEditFrm );
	to = FS_WindowGetWidgetText( editWin, FS_W_EmlEditTo );
	cc = FS_WindowGetWidgetText( editWin, FS_W_EmlEditCc );
	bcc = FS_WindowGetWidgetText( editWin, FS_W_EmlEditBcc );
	if( (to == FS_NULL || to[0] == 0) && (cc == FS_NULL || cc[0] == 0) && (bcc == FS_NULL || bcc[0] == 0))
	{
		FS_MessageBox(FS_MS_ALERT, FS_Text(FS_T_INPUT_ADDR), FS_NULL, FS_FALSE );
		return;
	}
	if( (to && to[0] && IFS_Strchr(to, '@') == FS_NULL)
		|| (cc && cc[0] && IFS_Strchr(cc, '@') == FS_NULL)
		|| (bcc && bcc[0] && IFS_Strchr(bcc, '@') == FS_NULL))
	{
		FS_MessageBox(FS_MS_ALERT, FS_Text(FS_T_INVALID_ADDR), FS_NULL, FS_FALSE );
		return;
	}
	FS_MessageBox( FS_MS_NONE, FS_Text(FS_T_PLS_WAITING), FS_NULL, FS_FALSE );
	IFS_PostMessage( FS_MSG_UTIL_CALL, (FS_UINT4)FS_EmlSend_HD );
}

static FS_SINT4 FS_EmlFileGetAttachCount( FS_EmlFile *emlFile )
{
	FS_MimeEntry *entry;
	FS_List * node;
	FS_SINT4 cnt = 0;
	
	node = emlFile->body.list.next;
	while( node != &emlFile->body.list )
	{
		entry = FS_ListEntry( node, FS_MimeEntry, list );
		node = node->next;
		if( entry->type == FS_MIME_MULTIPART )
			continue;
		if( entry->type == FS_MIME_TEXT && FS_STR_I_EQUAL(entry->subtype, "plain") && entry->disposition != FS_DPS_ATTACHMENT )
			continue;
		cnt ++;
	}
	return cnt;
}

static FS_SINT4 FS_EmlFileGetAttachTotalSize( FS_EmlFile *emlFile )
{
	FS_MimeEntry *entry;
	FS_List * node;
	FS_SINT4 size = 0;
	
	node = emlFile->body.list.next;
	while( node != &emlFile->body.list )
	{
		entry = FS_ListEntry( node, FS_MimeEntry, list );
		node = node->next;
		if( entry->type == FS_MIME_MULTIPART )
			continue;
		if( entry->type == FS_MIME_TEXT && FS_STR_I_EQUAL(entry->subtype, "plain") && entry->disposition != FS_DPS_ATTACHMENT )
			continue;
		size += entry->length;
	}
	return size;
}

static void FS_EmlAddAttachHandle( FS_CHAR * path, void *param )
{
	FS_SINT4 size, attach_size;
	FS_Widget *wgt;
	FS_Window *lwin = FS_WindowFindId( FS_W_EmlAttachListFrm );
	FS_Window *editWin = FS_WindowFindId( FS_W_EmlEditFrm );
	FS_EmlFile *emlFile;
	FS_MimeEntry *mimeEntry;
	FS_CHAR str[16];
	FS_UINT1 mbox = FS_EmlGetCurMBox( );
	FS_CHAR *uid = FS_EmlGetCurMailUid( );
	FS_EmlHead *pHead = FS_EmlGetHead( mbox, uid );
	
	if( path && editWin )
	{
		size = FS_FileGetSize( -1, path );
		if( size <= 0 )
		{
			FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_FILE_NOT_EXIST), FS_NULL, FS_FALSE );
			return;
		}
		
		emlFile = (FS_EmlFile *)editWin->private_data;
		attach_size = FS_EmlFileGetAttachTotalSize( emlFile );
		attach_size = (attach_size + size) * 4 / 3;		/* user base64 encode */
		if( attach_size >= FS_EmlConfigGetMaxMail() * 1024 )
		{
			FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_FILE_TOO_LARGE), FS_NULL, FS_FALSE );
			return;
		}
		if( pHead ) attach_size -= pHead->msg_size;
		if( FS_EmlIsFull( attach_size ) )
		{
			FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_SPACE_IS_FULL), FS_NULL, FS_FALSE );
			return;
		}
		
		/* add a mime entry to emlfile */
		mimeEntry = FS_NewMimeEntry( );
		mimeEntry->disp_name = IFS_Strdup( FS_GetFileNameFromPath(path) );
		mimeEntry->file_name = IFS_Strdup( path );
		mimeEntry->length = size;
		FS_ListAddTail( &emlFile->body.list, &mimeEntry->list );

		if( lwin )
		{
			/* create a list item */
			wgt = FS_CreateListItem( 0, mimeEntry->disp_name, FS_NULL, FS_I_FILE, 1 );
			wgt->data = IFS_Strdup( path );
			FS_WindowAddWidget( lwin, wgt );
			FS_WidgetSetHandler( wgt, FS_EmlAttachDetail_CB );
			FS_InvalidateRect( lwin, &lwin->client_rect );
		}

		if( FS_WindowIsTopMost(FS_W_EmlEditFrm) )
		{
			wgt = FS_WindowGetWidget( editWin, FS_W_EmlEditAttachList );
			IFS_Sprintf( str, "(%d)", FS_EmlFileGetAttachCount(emlFile) );
			FS_WidgetSetExtraText( wgt, str );
			FS_RedrawWidget( editWin, wgt );
		}
	}
}

static void FS_EmlAddAttach_CB( FS_Window *win )
{
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
	IFS_FileDialogOpen( FS_FDO_ALL, FS_EmlAddAttachHandle, FS_NULL );
}

static void FS_EmlReply( FS_BOOL bReplyAll )
{
	FS_SINT4 tlen;
	FS_EmlFile *emlFile;
	FS_EmlAddr *addrFrom;
	FS_CHAR *from, *subject, *text = FS_NULL, *msghead;
	FS_Widget *wgtText;
	FS_List *node;
	FS_MimeEntry *mimeEntry;
	FS_Window *ewin = FS_WindowFindId( FS_W_EmlEditFrm );
	
	/* get original message's data fields */
	emlFile = (FS_EmlFile *)ewin->private_data;
	addrFrom = IFS_Malloc( sizeof(FS_EmlAddr) );
	if( addrFrom ) IFS_Memcpy( addrFrom, &emlFile->from, sizeof(FS_EmlAddr) );
	subject = FS_StrConCat( "Re: ", emlFile->subject, FS_NULL, FS_NULL );
	from = FS_StrConCat( emlFile->from.name, "<", emlFile->from.addr, ">\r\n" );
	wgtText = FS_WindowGetWidget( ewin, FS_W_EmlEditText );
	if( FS_EmlConfigGetReplyCopy() )
	{
		msghead = IFS_Malloc( FS_MIME_HEAD_FIELD_MAX_LEN );
		if( msghead )
		{
			IFS_Sprintf( msghead, "%s%s: ", FS_Text(FS_T_EML_ORIG_MSG_BEGIN),
					FS_Text(FS_T_DATE) );
			tlen = IFS_Strlen( msghead );
			FS_DateStruct2DispStr( msghead + tlen, &emlFile->date );
			tlen = IFS_Strlen( msghead );
			IFS_Sprintf( msghead + tlen, "\r\n%s: %s\r\n%s: %s\r\n",
				FS_Text(FS_T_SUBJECT), 
				(emlFile->subject == FS_NULL ? "" : emlFile->subject),
				FS_Text(FS_T_FROM), from );
		}
		text = FS_StrConCat( msghead, wgtText->text, FS_NULL, FS_NULL );
		tlen = IFS_Strlen( text );
		if( tlen >= FS_EML_TEXT_MAX_LEN )
			text[FS_EML_TEXT_MAX_LEN - 1] = 0;
		if( msghead ) IFS_Free( msghead );
	}
	if( from ) IFS_Free( from );
	ewin->private_data = 0;
	FS_DestroyWindow( ewin );
	
	/* remove bcc fields, add from to rcpt list */
	if( ! bReplyAll )
	{
		FS_EmlFreeAddrList( &emlFile->to );
		FS_EmlFreeAddrList(  &emlFile->cc );
	}
	if( addrFrom )
		FS_ListAdd( &emlFile->to, &addrFrom->list );
	FS_EmlFreeAddrList( &emlFile->bcc );
	node = emlFile->body.list.next;
	while( node != &emlFile->body.list )
	{
		mimeEntry = FS_ListEntry( node, FS_MimeEntry, list );
		node = node->next;
		FS_ListDel( &mimeEntry->list );
		FS_FreeMimeEntry( mimeEntry );
		IFS_Free( mimeEntry );
	}
	FS_FreeMimeEntry( &emlFile->body );
	
	FS_SAFE_FREE( emlFile->subject );
	emlFile->subject = subject;
	FS_SAFE_FREE( emlFile->text );
	emlFile->text = text;
	
	/* show the new edit form */
	FS_EmlEditFrm_UI( emlFile, FS_FALSE );
}

static void FS_EmlReply_CB( FS_Window *win )
{
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
	
	if( FS_EmlIsFull( 1024 ) )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_SPACE_IS_FULL), FS_NULL, FS_FALSE );
		return;
	}

	FS_EmlReply( FS_FALSE );
}

static void FS_EmlForward_CB( FS_Window *win )
{
	FS_SINT4 tlen;
	FS_EmlFile *emlFile;
	FS_CHAR *from, *subject, *text = FS_NULL, *msghead;
	FS_Widget *wgtText;
	FS_Window *ewin = FS_WindowFindId( FS_W_EmlEditFrm );
	FS_UINT1 mbox = FS_EmlGetCurMBox( );
	FS_CHAR *uid = FS_EmlGetCurMailUid( );
	FS_EmlHead *pHead = FS_EmlGetHead( mbox, uid );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );

	if( FS_EmlIsFull(pHead->msg_size) )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_SPACE_IS_FULL), FS_NULL, FS_FALSE );
		return;
	}
	
	msghead = IFS_Malloc( FS_MIME_HEAD_FIELD_MAX_LEN );
	/* get original message's data fields */
	emlFile = (FS_EmlFile *)ewin->private_data;
	subject = FS_StrConCat( "Fw: ", emlFile->subject, FS_NULL, FS_NULL );
	from = FS_StrConCat( emlFile->from.name, "<", emlFile->from.addr, ">\r\n" );
	wgtText = FS_WindowGetWidget( ewin, FS_W_EmlEditText );
	if( msghead )
	{
		IFS_Sprintf( msghead, "%s%s: ", FS_Text(FS_T_EML_FWD_MSG_BEGIN),
				FS_Text(FS_T_DATE) );
		tlen = IFS_Strlen( msghead );
		FS_DateStruct2DispStr( msghead + tlen, &emlFile->date );
		tlen = IFS_Strlen( msghead );
		IFS_Sprintf( msghead + tlen, "\r\n%s: %s\r\n%s: %s\r\n",
			FS_Text(FS_T_SUBJECT), 
			(emlFile->subject == FS_NULL ? "" : emlFile->subject),
			FS_Text(FS_T_FROM), from );
	}
	
	text = FS_StrConCat( msghead, wgtText->text, FS_NULL, FS_NULL );
	tlen = IFS_Strlen( text );

	if( tlen >= FS_EML_TEXT_MAX_LEN )
		text[FS_EML_TEXT_MAX_LEN - 1] = 0;
	
	if( msghead ) IFS_Free( msghead );
	if( from ) IFS_Free( from );
	ewin->private_data = 0;
	FS_DestroyWindow( ewin );

	/* remove to, cc, bcc fields */
	FS_EmlFreeAddrList( &emlFile->to );
	FS_EmlFreeAddrList( &emlFile->cc );
	FS_EmlFreeAddrList( &emlFile->bcc );
	FS_SAFE_FREE( emlFile->subject );
	emlFile->subject = subject;
	FS_SAFE_FREE( emlFile->text );
	emlFile->text = text;
	
	/* show the new edit form */
	FS_EmlEditFrm_UI( emlFile, FS_FALSE );
}

static void FS_EmlReplyAll_CB( FS_Window *win )
{
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
	
	if( FS_EmlIsFull( 1024 ) )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_SPACE_IS_FULL), FS_NULL, FS_FALSE );
		return;
	}
	
	FS_EmlReply( FS_TRUE );
}

static void FS_EmlAttachDetail_CB( FS_Window *win )
{
	FS_Window *lwin = FS_WindowFindId( FS_W_EmlAttachListFrm );
	FS_CHAR *str;
	FS_SINT4 len, size;
	FS_Widget *wgt = FS_WindowGetFocusItem( lwin );
	
	if( wgt )
	{
		str = IFS_Malloc( FS_EML_DETAIL_LEN);
		IFS_Sprintf( str, "%s: %s\n", FS_Text(FS_T_DOC_TYPE), FS_GetMimeFromExt(wgt->text) );
		len = IFS_Strlen( str );

		IFS_Sprintf( str + len, "%s: %s\n", FS_Text(FS_T_FILE_NAME), wgt->text );
		len = IFS_Strlen( str );

		size = FS_FileGetSize( -1, wgt->data );
		IFS_Sprintf( str + len, "%s: %d(KB)", FS_Text(FS_T_SIZE), FS_KB(size) );
		
		FS_StdShowDetail( FS_Text(FS_T_DETAIL), str );
		IFS_Free( str );
	}
	
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_EmlViewSaveAttach_HD( void )
{
	FS_Window *lwin = FS_WindowFindId( FS_W_EmlAttachListFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( lwin );
	FS_Window *win = FS_WindowFindId( FS_W_ProgressFrm );
	FS_CHAR *str, *filename;
	FS_BOOL ret;
	FS_SINT4 len;
	
	if( win && wgt && wgt->data )
	{
		filename = (FS_CHAR *)win->private_data;
		ret = FS_FileCopy( -1, wgt->data, -1, filename );
		len = IFS_Strlen( filename );
		if( ret )
		{
			str = IFS_Malloc( len + 32 );
			IFS_Sprintf( str, "%s %s", FS_Text(FS_T_SAVED_TO), filename );
			FS_MessageBox( FS_MS_OK, str, FS_NULL, FS_FALSE );
			IFS_Free( str );
		}
		else
		{
			str = FS_Text( FS_T_SAVE_FILE_FAILED );
			FS_MessageBox( FS_MS_ALERT, str, FS_NULL, FS_FALSE );
		}
		IFS_Free( filename );
	}
	FS_DestroyWindow( win );
}

static void FS_EmlViewSaveAttachHandle( FS_CHAR *fileName, void *param )
{
	FS_Window *iwin;
	iwin = FS_MessageBox( FS_MS_NONE, FS_Text(FS_T_PLS_WAITING), FS_NULL, FS_FALSE );
	iwin->private_data = (FS_UINT4)IFS_Strdup( fileName );
	IFS_PostMessage( FS_MSG_UTIL_CALL, (FS_UINT4)FS_EmlViewSaveAttach_HD );
}

static void FS_EmlViewSaveAttach_CB( FS_Window *win )
{
	FS_Window *lwin = FS_WindowFindId( FS_W_EmlAttachListFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( lwin );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );

	if( wgt ){
		IFS_FileDialogSave( wgt->text, FS_EmlViewSaveAttachHandle, FS_NULL );
	}
}

static void FS_EmlViewAttachListMenu_UI( FS_Window *win )
{
	FS_Widget *itemDetail, *itemSave;
	FS_Window *pMenu;

	itemDetail = FS_CreateMenuItem( 0,	FS_Text(FS_T_DETAIL) );
	itemSave = FS_CreateMenuItem( 0,  FS_Text(FS_T_SAVE) );

	pMenu = FS_CreateMenu( 0, 2 );
	FS_MenuAddItem( pMenu, itemDetail );
	FS_MenuAddItem( pMenu, itemSave );
	FS_WidgetSetHandler( itemDetail, FS_EmlAttachDetail_CB );
	FS_WidgetSetHandler( itemSave, FS_EmlViewSaveAttach_CB );

	FS_MenuSetSoftkey( pMenu );
	FS_ShowWindow( pMenu );
}

static void FS_EmlFileDelAllAttach( FS_EmlFile *emlFile )
{
	FS_MimeEntry *entry;
	FS_List * node;
	
	node = emlFile->body.list.next;
	while( node != &emlFile->body.list )
	{
		entry = FS_ListEntry( node, FS_MimeEntry, list );
		node = node->next;

		FS_ListDel( &entry->list );
		FS_FreeMimeEntry( entry );
		IFS_Free( entry );
	}
}

static FS_BOOL FS_EmlEditAttachDelAllCnf_CB( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	if( wparam == FS_EV_YES )	// exit without save account data
	{
		FS_Window *editWin = FS_WindowFindId( FS_W_EmlEditFrm );
		FS_Window *lwin = FS_WindowFindId( FS_W_EmlAttachListFrm );
		FS_EmlFile *emlFile;
		
		emlFile = (FS_EmlFile *)editWin->private_data;
		FS_EmlFileDelAllAttach( emlFile );
		FS_WindowDelWidgetList( lwin );
		ret = FS_TRUE;
	}
	return ret;
}

static void FS_EmlEditAttachDelAll_CB( FS_Window *win )
{
	FS_Window *lwin = FS_WindowFindId( FS_W_EmlAttachListFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( lwin );

	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );

	if( wgt )
		FS_MessageBox( FS_MS_YES_NO, FS_Text(FS_T_CONFIRM_DEL), FS_EmlEditAttachDelAllCnf_CB, FS_FALSE );	
}


static void FS_EmlFileDelAttachByFile( FS_EmlFile *emlFile, FS_CHAR *file )
{
	FS_MimeEntry *entry;
	FS_List * node;
	FS_CHAR absFile[FS_MAX_PATH_LEN];
	
	if( emlFile == FS_NULL || file == FS_NULL ) return;
	
	node = emlFile->body.list.next;
	while( node != &emlFile->body.list )
	{
		entry = FS_ListEntry( node, FS_MimeEntry, list );
		node = node->next;

		if( entry->temp_file ){
			FS_GetAbsFileName( FS_DIR_TMP, entry->file_name, absFile );
		}else{
			IFS_Memset( absFile, 0, sizeof(absFile) );
			IFS_Strncpy( absFile, entry->file_name, sizeof(absFile) - 1 );
		}

		if( FS_STR_I_EQUAL( absFile, file ) ){
			FS_ListDel( &entry->list );
			FS_FreeMimeEntry( entry );
			IFS_Free( entry );
			return;
		}
	}
}

static FS_BOOL FS_EmlEditAttachDelCnf_CB( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	if( wparam == FS_EV_YES )	// exit without save account data
	{
		FS_Window *editWin = FS_WindowFindId( FS_W_EmlEditFrm );
		FS_Window *lwin = FS_WindowFindId( FS_W_EmlAttachListFrm );
		FS_Widget *wgt = FS_WindowGetFocusItem( lwin );
		FS_EmlFile *emlFile;
		if( wgt )
		{
			emlFile = (FS_EmlFile *)editWin->private_data;
			FS_EmlFileDelAttachByFile( emlFile, wgt->data );
			FS_WindowDelWidget( lwin, wgt );
		}
		ret = FS_TRUE;
	}
	return ret;
}

static void FS_EmlEditAttachDel_CB( FS_Window *win )
{
	FS_Window *lwin = FS_WindowFindId( FS_W_EmlAttachListFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( lwin );

	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );

	if( wgt )
		FS_MessageBox( FS_MS_YES_NO, FS_Text(FS_T_CONFIRM_DEL), FS_EmlEditAttachDelCnf_CB, FS_FALSE );	
}


static void FS_EmlEditAttachListMenu_UI( FS_Window * win )
{
	FS_Window *menu;
	FS_Widget *iDel, *iDelAll, *iDetail, *iAdd;
	
	menu = FS_CreateMenu( 0, 4 );

	iDetail = FS_CreateMenuItem( 0, FS_Text(FS_T_DETAIL) );
	iAdd = FS_CreateMenuItem( 0, FS_Text(FS_T_EML_ADD_ATTACH) );
	iDel = FS_CreateMenuItem( 0,  FS_Text(FS_T_DEL) );
	iDelAll = FS_CreateMenuItem( 0,  FS_Text(FS_T_DEL_ALL) );
	
	FS_MenuAddItem( menu, iDetail );
	FS_MenuAddItem( menu, iAdd );
	FS_MenuAddItem( menu, iDel );
	FS_MenuAddItem( menu, iDelAll );
	
	FS_WidgetSetHandler( iDetail, FS_EmlAttachDetail_CB );
	FS_WidgetSetHandler( iAdd, FS_EmlAddAttach_CB );
	FS_WidgetSetHandler( iDel, FS_EmlEditAttachDel_CB );
	FS_WidgetSetHandler( iDelAll, FS_EmlEditAttachDelAll_CB );
	
	FS_MenuSetSoftkey( menu );

	FS_ShowWindow( menu );
}

static void FS_EmlAttachList_UI( FS_Window *win)
{
	FS_EmlFile *emlFile;
	FS_MimeEntry *mimeEntry;
	FS_List *node;
	FS_Window *lwin, *ewin;
	FS_Widget *wgt;
	FS_BOOL bView;
	FS_CHAR absFile[FS_MAX_PATH_LEN];
	
	ewin = FS_WindowFindId( FS_W_EmlEditFrm );
	emlFile = (FS_EmlFile *)ewin->private_data;
	lwin = FS_CreateWindow( FS_W_EmlAttachListFrm, FS_Text(FS_T_EML_ATTACH_LIST), FS_NULL );
	// show mail attach list
	node = emlFile->body.list.next;
	while( node != &emlFile->body.list )
	{
		mimeEntry = FS_ListEntry( node, FS_MimeEntry, list );
		node = node->next;
		
		if( mimeEntry->type == FS_MIME_TEXT && IFS_Stricmp( mimeEntry->subtype, "plain" ) == 0 && mimeEntry->disposition != FS_DPS_ATTACHMENT )
		{
			continue;
		}
		else if( mimeEntry->type == FS_MIME_MULTIPART )
		{
			continue;
		}
		/* create a list item */
		wgt = FS_CreateListItem( 0, mimeEntry->disp_name, FS_NULL, FS_I_FILE, 1 );
		if( mimeEntry->temp_file ){
			FS_GetAbsFileName( FS_DIR_TMP, mimeEntry->file_name, absFile );
			wgt->data = IFS_Strdup( absFile );
		}else{
			IFS_Memset( absFile, 0, sizeof(absFile) );
			IFS_Strncpy( absFile, mimeEntry->file_name, sizeof(absFile) - 1 );
			wgt->data = IFS_Strdup( mimeEntry->file_name );
		}
		FS_WidgetSetHandler( wgt, FS_EmlAttachDetail_CB );
		FS_WindowAddWidget( lwin, wgt );
	}

	wgt = FS_WindowGetWidget( ewin, FS_W_EmlEditAttachList );
	bView = (FS_BOOL)wgt->private_data;
	if( bView )
		FS_WindowSetSoftkey( lwin, 1, FS_Text(FS_T_MENU), FS_EmlViewAttachListMenu_UI );
	else
		FS_WindowSetSoftkey( lwin, 1, FS_Text(FS_T_MENU), FS_EmlEditAttachListMenu_UI );

	FS_WindowSetSoftkey( lwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	FS_ShowWindow( lwin );
	
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static FS_CHAR * FS_EmlFileGetHtmlBody( FS_EmlFile *emlFile )
{
#if 1
	return FS_NULL;
#else
	FS_MimeEntry *entry;
	FS_List * node;

	if( emlFile == FS_NULL ) return FS_NULL;
	
	node = emlFile->body.list.next;
	while( node != &emlFile->body.list )
	{
		entry = FS_ListEntry( node, FS_MimeEntry, list );
		node = node->next;

		if( entry->type == FS_MIME_TEXT && FS_STR_I_EQUAL(entry->subtype, "html") && entry->disposition != FS_DPS_ATTACHMENT )
			return entry->file_name;
	}
	return FS_NULL;
#endif	
}

static void FS_EmlViewHtml_CB( FS_Window *win)
{
	FS_CHAR absFile[FS_MAX_PATH_LEN];
	FS_Window *editWin = FS_WindowFindId( FS_W_EmlEditFrm );
	FS_EmlFile *emlFile = (FS_EmlFile *)editWin->private_data;
	FS_CHAR *htmlFile = FS_EmlFileGetHtmlBody( emlFile );

	FS_GetAbsFileName( FS_DIR_TMP, htmlFile, absFile );
	FS_WebLoadWebDoc( absFile );
	
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );	
}


static void FS_EmlViewMenu_UI( FS_Window *win )
{
	FS_Widget *btnRe, *btnFw, *btnReAll, *btnAttach, *btnHtml;
	FS_Window *pMenu;
	FS_Window *ewin = FS_WindowFindId( FS_W_EmlEditFrm );
	FS_EmlFile *emlFile = (FS_EmlFile *)ewin->private_data;
	FS_CHAR* htmlFile = FS_EmlFileGetHtmlBody( emlFile );
	FS_SINT4 nItem = 4;
	
	btnRe = FS_CreateMenuItem( 0,  FS_Text(FS_T_REPLY) );
	btnFw = FS_CreateMenuItem( 0,  FS_Text(FS_T_FORWARD) );
	btnReAll = FS_CreateMenuItem( 0,  FS_Text(FS_T_REPLY_ALL) );
	btnAttach = FS_CreateMenuItem( 0, FS_Text(FS_T_EML_ATTACH_LIST) );
	if( htmlFile ) 
	{
		nItem ++;
		btnHtml = FS_CreateMenuItem( 0, FS_Text(FS_T_EML_HTML_VIEW) );
	}
	
	pMenu = FS_CreateMenu( FS_W_EmlViewMenu, nItem );
	FS_MenuAddItem( pMenu, btnRe );
	FS_MenuAddItem( pMenu, btnReAll );
	FS_MenuAddItem( pMenu, btnFw );
	FS_MenuAddItem( pMenu, btnAttach );
	if( htmlFile ) FS_MenuAddItem( pMenu, btnHtml );
	
	FS_WidgetSetHandler( btnRe, FS_EmlReply_CB );
	FS_WidgetSetHandler( btnFw, FS_EmlForward_CB );
	FS_WidgetSetHandler( btnReAll, FS_EmlReplyAll_CB );
	FS_WidgetSetHandler( btnAttach, FS_EmlAttachList_UI );
	if( htmlFile ) FS_WidgetSetHandler( btnHtml, FS_EmlViewHtml_CB );
	
	FS_MenuSetSoftkey( pMenu );

	FS_ShowWindow( pMenu );
}

static void FS_EmlEditMenu_UI( FS_Window *win )
{
	FS_Widget *btnSend, *btnSave, *btnAddAttach, *btnAttachList;

	FS_Window *pMenu;
	btnSend = FS_CreateMenuItem( 0,  FS_Text(FS_T_SEND) );
	btnSave = FS_CreateMenuItem( 0,  FS_Text(FS_T_SAVE) );
	btnAddAttach = FS_CreateMenuItem( 0, FS_Text(FS_T_EML_ADD_ATTACH) );
	btnAttachList = FS_CreateMenuItem( 0, FS_Text(FS_T_EML_ATTACH_LIST) );
	
	pMenu = FS_CreateMenu( FS_W_EmlEditMenu, 4 );
	FS_MenuAddItem( pMenu, btnSend );
	FS_MenuAddItem( pMenu, btnSave );
	FS_MenuAddItem( pMenu, btnAddAttach );
	FS_MenuAddItem( pMenu, btnAttachList );
	
	FS_WidgetSetHandler( btnSend, FS_EmlSend_CB );
	FS_WidgetSetHandler( btnSave, FS_EmlSaveDraft_CB );
	FS_WidgetSetHandler( btnAddAttach, FS_EmlAddAttach_CB );
	FS_WidgetSetHandler( btnAttachList, FS_EmlAttachList_UI );
	
	FS_MenuSetSoftkey( pMenu );

	FS_ShowWindow( pMenu );
}

static FS_BOOL FS_EmlEditWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	if( cmd == FS_WM_DESTROY )
	{
		FS_EmlFile *emlFile = (FS_EmlFile *)win->private_data;
		FS_CHAR *uid = (FS_CHAR *)win->private_data2;
		if( emlFile )
		{
			FS_FreeEmlFile( emlFile );
			IFS_Free( emlFile );
		}
		if( uid ) IFS_Free( uid );
		ret = FS_TRUE;
	}
	else if( cmd == FS_WM_PAINT )
	{
		FS_CHAR str[16];
		FS_Widget *wgt = FS_WindowGetWidget( win, FS_W_EmlEditAttachList );
		FS_EmlFile *emlFile = (FS_EmlFile *)win->private_data;
		if( wgt ){
			IFS_Sprintf( str, "(%d)", FS_EmlFileGetAttachCount(emlFile) );
			FS_WidgetSetExtraText( wgt, str );
		}
		ret = FS_FALSE;
	}
	return ret;
}

static void FS_EmlEditFrm_UI( FS_EmlFile *emlFile, FS_BOOL bView )
{
	FS_Window *editWin;
	FS_Widget *wSub, *wFrom = FS_NULL, *wDate = FS_NULL, *wTo = FS_NULL, *wCc = FS_NULL, *wBcc = FS_NULL, *wText, *wAttach;

	FS_CHAR str[16];
	FS_EditParam eParam = { FS_IM_ABC, FS_IM_ALL, FS_DEFAULT_EDIT_LEN };
	FS_CHAR *addr = IFS_Malloc( FS_EML_RCPT_MAX_LEN );

	FS_ASSERT( addr != FS_NULL );
	if( emlFile == FS_NULL )
	{
		emlFile = IFS_Malloc( sizeof(FS_EmlFile) );;
		FS_InitEmlFile( emlFile );
	}

	if( bView )
		editWin = FS_CreateWindow( FS_W_EmlEditFrm, FS_Text(FS_T_EML_VIEW), FS_EmlEditWndProc );
	else
		editWin = FS_CreateWindow( FS_W_EmlEditFrm, FS_Text(FS_T_EML_EDIT), FS_EmlEditWndProc );
	/* to */
	FS_EmlGetAddrDispName( addr, FS_EML_RCPT_MAX_LEN, &emlFile->to, bView );
	wTo = FS_CreateEditBox( FS_W_EmlEditTo, addr, FS_I_TO, 1, &eParam );
	FS_WidgetSetTip( wTo, FS_Text(FS_T_TO) );
	if( ! bView || ! FS_ListIsEmpty(&emlFile->cc) )
	{
		/* cc */
		FS_EmlGetAddrDispName( addr, FS_EML_RCPT_MAX_LEN, &emlFile->cc, bView );
		wCc = FS_CreateEditBox(FS_W_EmlEditCc, addr, FS_I_CC, 1, &eParam );
		FS_WidgetSetTip( wCc, FS_Text(FS_T_CC) );
	}
	if( bView)
	{
		/* from */
		IFS_Sprintf( addr, "%s <%s>", emlFile->from.name, emlFile->from.addr );
		wFrom = FS_CreateEditBox( FS_W_EmlEditFrom, addr, FS_I_FROM, 1, FS_NULL );
		FS_WidgetSetTip( wFrom, FS_Text(FS_T_FROM) );
		/* date */
		FS_DateStruct2DispStr( addr, &emlFile->date );
		wDate = FS_CreateEditBox( FS_W_EmlEditDate, addr, FS_I_INFO, 1, FS_NULL );
		FS_WidgetSetTip( wDate, FS_Text(FS_T_DATE) );
	}
	else
	{
		/* bcc */
		FS_EmlGetAddrDispName( addr, FS_EML_RCPT_MAX_LEN, &emlFile->bcc, bView );
		wBcc = FS_CreateEditBox(FS_W_EmlEditBcc, addr, FS_I_BCC, 1, &eParam );
		FS_WidgetSetTip( wBcc, FS_Text(FS_T_BCC) );
	}
	IFS_Free( addr );
	/* subject */
	eParam.preferred_method = FS_IM_CHI;
	wSub = FS_CreateEditBox( FS_W_EmlEditSubject, emlFile->subject, FS_I_SUBJECT, 1, &eParam );
	FS_WidgetSetTip( wSub, FS_Text(FS_T_SUBJECT) );
	/* attach list */
	wAttach = FS_CreateListItem( FS_W_EmlEditAttachList, FS_Text(FS_T_EML_ATTACH_LIST), FS_NULL, FS_I_FILE, 1 );
	IFS_Sprintf( str, "(%d)", FS_EmlFileGetAttachCount(emlFile) );
	FS_WidgetSetExtraText( wAttach, str );
	FS_WidgetSetTip( wAttach, FS_Text(FS_T_EML_ATTACH_LIST) );
	FS_WidgetSetHandler( wAttach, FS_EmlAttachList_UI );
	wAttach->private_data = bView;
	/* text */
	wText = FS_CreateScroller( FS_W_EmlEditText, emlFile->text );
	FS_WidgetSetTip( wText, FS_Text( FS_T_EML_TEXT ) );
	if( ! bView )
	{
		FS_WGT_SET_CAN_WRITE( wText );
		FS_WidgetSetHandler( wText, FS_StdEditBoxHandler );
	}
	/* add widget to window */
	if( wFrom ) FS_WindowAddWidget( editWin, wFrom );
	if( wTo ) FS_WindowAddWidget( editWin, wTo );
	if( wCc ) FS_WindowAddWidget( editWin, wCc);
	if( wBcc ) FS_WindowAddWidget( editWin, wBcc );
	if( wDate ) FS_WindowAddWidget( editWin, wDate );
	if( wSub ) FS_WindowAddWidget( editWin, wSub );
	if( wAttach ) FS_WindowAddWidget( editWin, wAttach );
	if( wText ) FS_WindowAddWidget( editWin, wText );
	/* save eml file param */
	editWin->private_data = (FS_UINT4)emlFile;
	if( bView )
	{
		if( wTo ) FS_WGT_CLR_CAN_WRITE( wTo );
		if( wCc ) FS_WGT_CLR_CAN_WRITE( wCc );
		if( wFrom ) FS_WGT_CLR_CAN_WRITE( wFrom );
		if( wDate ) FS_WGT_CLR_CAN_WRITE( wDate );
		if( wSub ) FS_WGT_CLR_CAN_WRITE( wSub );
	}
	
	if( bView )
		FS_WindowSetSoftkey( editWin, 1, FS_Text(FS_T_MENU), FS_EmlViewMenu_UI );
	else
		FS_WindowSetSoftkey( editWin, 1, FS_Text(FS_T_MENU), FS_EmlEditMenu_UI );

	FS_WindowSetSoftkey( editWin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	FS_ShowWindow( editWin );	
}

static void FS_EmlEdit_HD( void )
{
	FS_EmlHead *emlHead;
	FS_EmlFile *emlFile;
	FS_CHAR file_name[FS_MAX_PATH_LEN];
	FS_UINT1 mbox = FS_EmlGetCurMBox( );
	FS_CHAR *uid = FS_EmlGetCurMailUid( );
	if( uid )
	{
		/* when EmlEdit window destroyed, it will free emlFile */
		emlFile = IFS_Malloc( sizeof(FS_EmlFile) );
		/* wgt->data store the email UID */
		emlHead = FS_EmlGetHead( mbox, uid );
		FS_GetAbsFileName(FS_DIR_EML, emlHead->file, file_name );
		FS_EmlParseFile( emlFile, file_name );
		FS_EmlEditFrm_UI( emlFile, FS_FALSE );
		FS_EmlEditFormSetUid( uid );
		/* unread, set read flag */
		if( (mbox == FS_EmlInbox || mbox == FS_EmlLocal) && emlHead->read == FS_FALSE ) 
			FS_EmlSetReadFlag( mbox, uid );
	}
	FS_DestroyWindow( FS_WindowFindId(FS_W_ProgressFrm) );
}

static void FS_EmlEdit_CB( FS_Window *win )
{
	FS_CHAR *uid = FS_EmlGetCurMailUid( );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );

	if( uid == FS_NULL ) return;
	
	FS_MessageBox( FS_MS_NONE, FS_Text(FS_T_PLS_WAITING), FS_NULL, FS_FALSE );
	IFS_PostMessage( FS_MSG_UTIL_CALL, (FS_UINT4)FS_EmlEdit_HD );
}

static void FS_EmlView_HD( void )
{
	FS_EmlHead *emlHead;
	FS_EmlFile *emlFile;
	FS_CHAR file_name[FS_MAX_PATH_LEN];
	FS_UINT1 mbox = FS_EmlGetCurMBox( );
	FS_CHAR *uid = FS_EmlGetCurMailUid( );
	if( uid )
	{
		/* when EmlEdit window destroyed, it will free emlFile */
		emlFile = IFS_Malloc( sizeof(FS_EmlFile) );
		/* wgt->data store the email UID */
		emlHead = FS_EmlGetHead( mbox, uid );
		FS_GetAbsFileName(FS_DIR_EML, emlHead->file, file_name );
		FS_EmlParseFile( emlFile, file_name );
		FS_EmlEditFrm_UI( emlFile, FS_TRUE );
		/* unread, set read flag */
		if( emlHead->read == FS_FALSE )	
			FS_EmlSetReadFlag( mbox, uid );
	}
	FS_DestroyWindow( FS_WindowFindId(FS_W_ProgressFrm) );
}

static void FS_EmlView_CB( FS_Window *win )
{
	FS_CHAR *uid = FS_EmlGetCurMailUid( );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
	
	if( uid == FS_NULL ) return;
	
	FS_MessageBox( FS_MS_NONE, FS_Text(FS_T_PLS_WAITING), FS_NULL, FS_FALSE );
	IFS_PostMessage( FS_MSG_UTIL_CALL, (FS_UINT4)FS_EmlView_HD );
}

static void FS_EmlNew_CB( FS_Window *win )
{
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );

	if( FS_EmlIsFull(1024) ){
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_SPACE_IS_FULL), FS_NULL, FS_FALSE );
		return;
	}
	if( FS_EmlAccountExist() )
		FS_EmlEditFrm_UI( FS_NULL, FS_FALSE );
}

//-----------------------------------------------------------------------------------
// activate a account
static void FS_EmlActiveAct_CB( FS_Window *win )
{
	FS_Widget *wgt = FS_WindowGetFocusItem( FS_WindowFindId( FS_W_EmlActListFrm ) );
	if(wgt){
		FS_EmlActivateAct( wgt->text );
	}
	FS_DestroyWindow( win );
}

static FS_BOOL FS_EmlActDelCnf_CB( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	
	if( wparam == FS_EV_YES )	// exit without save account data
	{
		FS_Window *actMgr = FS_WindowFindId( FS_W_EmlActListFrm );
		FS_Widget *wgt = FS_WindowGetFocusItem( actMgr );
		if( wgt ){
			FS_EmlDelAccount( wgt->text );
		}
		ret = FS_TRUE;
	}
	return ret;
}
//-----------------------------------------------------------------------------------
// save account, will do some data check here
static void FS_EmlActDel_CB( FS_Window *win )
{
	FS_Window *actMgr = FS_WindowFindId( FS_W_EmlActListFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( actMgr );
	
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
	
	if( wgt ){
		FS_MessageBox( FS_MS_YES_NO, FS_Text(FS_T_CONFIRM_DEL), FS_EmlActDelCnf_CB, FS_FALSE );
	}
}

//-----------------------------------------------------------------------------------
// save account, will do some data check here
static void FS_EmlSaveAct_CB( FS_Window *win )
{
	FS_BOOL ok = FS_TRUE;
	FS_SINT4 len;
	FS_CHAR *actName, *emlAddr, *recvAddr, *smtpAddr, *userName, 
		*password, *recvPort, *smtpPort, *actType, *actDispName;
	FS_EmlAccount *act;
	FS_Widget *smtpAuth;
	actName = FS_WindowGetWidgetText( win, FS_W_EmlActName );
	actDispName = FS_WindowGetWidgetText( win, FS_W_EmlDispName );
	actType = FS_WindowGetWidgetText( win, FS_W_EmlActType );
	emlAddr = FS_WindowGetWidgetText( win, FS_W_EmlAddr );
	recvAddr = FS_WindowGetWidgetText( win, FS_W_EmlRecvAddr );
	smtpAddr = FS_WindowGetWidgetText( win, FS_W_EmlSmtpAddr );
	userName = FS_WindowGetWidgetText( win, FS_W_EmlUserName );
	password = FS_WindowGetWidgetText( win, FS_W_EmlPassword );
	recvPort = FS_WindowGetWidgetText( win, FS_W_EmlRecvPort );
	smtpPort= FS_WindowGetWidgetText( win, FS_W_EmlSmtpPort );
	smtpAuth = FS_WindowGetWidget( win, FS_W_EmlSmtpAuth );
	
	if( ok )
	{
		if( actName == FS_NULL )
			ok = FS_FALSE;
		else
		{
			len = IFS_Strlen( actName );
			if( len == 0 || len > FS_EML_MIN_STR )
				ok = FS_FALSE;
		}
	}
	if( ok )
	{
		if( emlAddr == FS_NULL )
			ok = FS_FALSE;
		else
		{
			len = IFS_Strlen( emlAddr );
			if( len == 0 || len > FS_EML_MID_STR )
				ok = FS_FALSE;
		}
	}
	if( ok )
	{
		if( recvAddr == FS_NULL )
			ok = FS_FALSE;
		else
		{
			len = IFS_Strlen( recvAddr );
			if( len == 0 || len > FS_EML_MID_STR )
				ok = FS_FALSE;
		}
	}
	if( ok )
	{
		if( smtpAddr == FS_NULL )
			ok = FS_FALSE;
		else
		{
			len = IFS_Strlen( smtpAddr );
			if( len == 0 || len > FS_EML_MID_STR )
				ok = FS_FALSE;
		}
	}
	if( ok )
	{
		if( userName == FS_NULL )
			ok = FS_FALSE;
		else
		{
			len = IFS_Strlen( userName );
			if( len == 0 || len > FS_EML_MIN_STR )
				ok = FS_FALSE;
		}
	}
	if( ok )
	{
		if( password == FS_NULL )
			ok = FS_FALSE;
		else
		{
			len = IFS_Strlen( password );
			if( len == 0 || len > FS_EML_MIN_STR )
				ok = FS_FALSE;
		}
	}
	if( ok )
	{
		if( recvPort == FS_NULL )
			ok = FS_FALSE;
		else
		{
			len = IFS_Strlen( recvPort );
			if( len == 0 || len > FS_EML_MIN_STR )
				ok = FS_FALSE;
		}
	}
	if( ok )
	{
		if( smtpPort == FS_NULL )
			ok = FS_FALSE;
		else
		{
			len = IFS_Strlen( smtpPort );
			if( len == 0 || len > FS_EML_MIN_STR )
				ok = FS_FALSE;
		}
	}

	if( ! ok )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_INPUT_ALL_DATA_PLS), FS_NULL, FS_FALSE );
		return;
	}
	else
	{	// here private data store the flag : create account ?
		if( win->private_data && FS_EmlGetAccount( actName ) )
		{
			FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_EML_ACT_EXIST), FS_NULL, FS_FALSE );
			return;
		}
	}
	// data check is ok, we can save account now
	act = IFS_Malloc( sizeof(FS_EmlAccount) );
	if( act )
	{
		IFS_Memset( act, 0, sizeof(FS_EmlAccount) );
		IFS_Strncpy( act->account_name, actName, sizeof(act->account_name) - 1 );
		if( actDispName )
			IFS_Strncpy( act->disp_name, actDispName, sizeof(act->disp_name) - 1 );
		if( win->private_data )
		{
			if( IFS_Strcmp(actType, FS_Text(FS_T_EML_POP3)) == 0 )
				act->type = FS_EML_POP3;
			else
				act->type = FS_EML_IMAP4;
		}
		IFS_Strncpy(act->recv_addr, recvAddr, sizeof(act->recv_addr) - 1 );
		IFS_Strncpy(act->smtp_addr, smtpAddr, sizeof(act->smtp_addr) - 1 );
		IFS_Strncpy(act->eml_addr, emlAddr, sizeof(act->eml_addr) - 1 );
		IFS_Strncpy(act->user_name, userName, sizeof(act->user_name) - 1 );
		IFS_Strncpy(act->password, password, sizeof(act->password) - 1 );
		act->recv_port = (FS_UINT2)IFS_Atoi( recvPort );
		act->smtp_port = (FS_UINT2)IFS_Atoi( smtpPort );
		act->smtp_auth = FS_WGT_GET_CHECK( smtpAuth );
		FS_EmlSaveAccount( act );
		IFS_Free( act );
		FS_EmlActBuildList( );
	}
	FS_DestroyWindow( win );
}

static void FS_EmlNotify_CB( FS_Window *win )
{
	FS_MessageBox( FS_MS_OK, FS_Text(FS_T_MODIFY_DENY), FS_NULL, FS_FALSE );
}
//-----------------------------------------------------------------------------------
// account edit ui bNewAct = FALSE, is to edit account, will not be allowed to edit account type
static void FS_EmlActEdit_UI( FS_EmlAccount *act, FS_BOOL bNewAct )
{
	FS_Widget *editActName, *editEmlType, *editEmlAddr, *editRecvAddr, 
		*editSmtpAddr, *editUserName, *editPassword, *editRecvPort, 
		*editSmtpAuth, *editSmtpPort, *editDispName;
	FS_Window *editWin;
	FS_CHAR pRecvPort[16];
	FS_CHAR pSmtpPort[16];
	FS_EditParam eParam = { FS_IM_ABC, FS_IM_ALL, FS_DEFAULT_EDIT_LEN };
	
	IFS_Itoa( act->recv_port, pRecvPort, 10 );
	IFS_Itoa( act->smtp_port, pSmtpPort, 10 );
	eParam.preferred_method = FS_IM_CHI;
	eParam.max_len = sizeof(act->account_name) - 1;
	editActName = FS_CreateEditBox( FS_W_EmlActName,  act->account_name, FS_I_EDIT, 1, &eParam );
	eParam.preferred_method = FS_IM_CHI;
	eParam.max_len = sizeof(act->disp_name) - 1;
	editDispName = FS_CreateEditBox( FS_W_EmlDispName,	act->disp_name, FS_I_EDIT, 1, &eParam );
	eParam.preferred_method = FS_IM_ABC;
	eParam.max_len = FS_EML_ADDR_LEN - 1;
	editEmlAddr = FS_CreateEditBox( FS_W_EmlAddr,  act->eml_addr, FS_I_EDIT, 1, &eParam );
	editRecvAddr = FS_CreateEditBox( FS_W_EmlRecvAddr,  act->recv_addr, FS_I_EDIT, 1, &eParam );
	editSmtpAddr = FS_CreateEditBox( FS_W_EmlSmtpAddr,  act->smtp_addr, FS_I_EDIT, 1, &eParam );
	eParam.preferred_method = FS_IM_ABC;
	eParam.max_len = sizeof(act->user_name) - 1;
	editUserName = FS_CreateEditBox( FS_W_EmlUserName,  act->user_name, FS_I_EDIT, 1, &eParam );
	eParam.preferred_method = FS_IM_123;
	eParam.max_len = sizeof(act->password) - 1;
	editPassword = FS_CreateEditBox( FS_W_EmlPassword,  act->password, FS_I_EDIT, 1, &eParam );
	FS_WGT_SET_MARK_CHAR( editPassword );
	FS_WGT_CLR_MULTI_LINE( editPassword );
	eParam.preferred_method = FS_IM_123;
	eParam.max_len = 12;
	eParam.allow_method = FS_IM_123;
	editRecvPort = FS_CreateEditBox( FS_W_EmlRecvPort,  pRecvPort, FS_I_EDIT, 1, &eParam );
	editSmtpPort = FS_CreateEditBox( FS_W_EmlSmtpPort,  pSmtpPort, FS_I_EDIT, 1, &eParam );	
	/* did not support IMAP4 now */
#if 0
	if( bNewAct )
	{
		editWin = FS_CreateWindow( FS_W_EmlActEditFrm, FS_Text(FS_T_EML_NEW_ACCOUNT), FS_NULL );
		editEmlType = FS_CreateComboBox( FS_W_EmlActType,  FS_Text(FS_T_EML_POP3), FS_I_EDIT );	
		FS_WidgetSetHandler( editEmlType, FS_EmlSelectActType_UI );
	}
	else
	{		
		editWin = FS_CreateWindow( FS_W_EmlActEditFrm, FS_Text(FS_T_EML_EDIT_ACCOUNT), FS_NULL );
		if( act->type == FS_EML_POP3 )
			editEmlType = FS_CreateEditBox( FS_W_EmlActType,  FS_Text(FS_T_EML_POP3), FS_I_EDIT, 1, FS_NULL );	
		else
			editEmlType = FS_CreateEditBox( FS_W_EmlActType,  FS_Text(FS_T_EML_IMAP4), FS_I_EDIT, 1, FS_NULL );	
	}
#else
	editWin = FS_CreateWindow( FS_W_EmlActEditFrm, FS_Text(FS_T_EML_EDIT_ACCOUNT), FS_NULL );
	editEmlType = FS_CreateEditBox( FS_W_EmlActType,  FS_Text(FS_T_EML_POP3), FS_I_EDIT, 1, FS_NULL );
	FS_WGT_CLR_CAN_WRITE( editEmlType );
#endif	
	editSmtpAuth = FS_CreateCheckBox( FS_W_EmlSmtpAuth, FS_Text(FS_T_EML_SMTP_AUTH) );
	FS_WidgetSetCheck( editSmtpAuth, act->smtp_auth );

	editActName->tip = FS_Text(FS_T_EML_TIP_ACCOUNT_NAME);
	editDispName->tip = FS_Text(FS_T_EML_TIP_DISP_NAME);
	editEmlAddr->tip =  FS_Text(FS_T_EML_TIP_EMAIL_ADDR);
	editRecvAddr->tip = FS_Text(FS_T_EML_TIP_POP3_ADDR);
	editSmtpAddr->tip = FS_Text(FS_T_EML_TIP_SMTP_ADDR);
	editUserName->tip = FS_Text(FS_T_EML_TIP_USER_NAME);
	editPassword->tip = FS_Text(FS_T_EML_TIP_PASSWORD);
	editRecvPort->tip = FS_Text(FS_T_EML_TIP_POP3_PORT);
	editSmtpPort->tip = FS_Text(FS_T_EML_TIP_SMTP_PORT);
	editEmlType->tip = FS_Text(FS_T_EML_TIP_ACCOUNT_TYPE);
	editSmtpAuth->tip = FS_Text(FS_T_EML_SMTP_AUTH);
	FS_WindowAddWidget( editWin, editActName );
	FS_WindowAddWidget( editWin, editEmlType );
	FS_WindowAddWidget( editWin, editDispName );
	FS_WindowAddWidget( editWin, editEmlAddr );
	FS_WindowAddWidget( editWin, editRecvAddr );
	FS_WindowAddWidget( editWin, editSmtpAddr );
	FS_WindowAddWidget( editWin, editUserName );
	FS_WindowAddWidget( editWin, editPassword );
	FS_WindowAddWidget( editWin, editRecvPort );
	FS_WindowAddWidget( editWin, editSmtpPort );
	FS_WindowAddWidget( editWin, editSmtpAuth );
	if( ! bNewAct )	// account name and type didnot allow to modify
	{
		FS_WidgetSetHandler( editEmlType, FS_EmlNotify_CB );
		FS_WidgetSetHandler( editActName, FS_EmlNotify_CB );
	}
	
	FS_WindowSetSoftkey( editWin, 1, FS_Text(FS_T_SAVE), FS_EmlSaveAct_CB );
	FS_WindowSetSoftkey( editWin, 3, FS_Text(FS_T_BACK), FS_StdExitWithoutSaveCnfHandler );

	editWin->private_data = bNewAct;
	FS_ShowWindow( editWin );
}

static void FS_EmlActEdit_CB( FS_Window *win )
{
	FS_EmlAccount *act;
	FS_Window *actMgr = FS_WindowFindId( FS_W_EmlActListFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( actMgr );
	if( wgt )
	{
		act = FS_EmlGetAccount( wgt->text );
		if( act )
			FS_EmlActEdit_UI( act, FS_FALSE );
	}
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_EmlNewAct_CB( FS_Window *win )
{
	FS_EmlAccount * act;
	
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );

	if( FS_EmlGetActNum() >= FS_EML_MAX_ACT_NUM )
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_EML_ACT_FULL), FS_NULL, FS_FALSE );
		return;
	}
	
	act = IFS_Malloc( sizeof(FS_EmlAccount) );
	if( act )
	{
		IFS_Memset( act, 0, sizeof(FS_EmlAccount) );
		act->type = FS_EML_POP3;
		act->recv_port = 110;
		act->smtp_port = 25;
		act->smtp_auth = FS_TRUE;
		FS_EmlActEdit_UI( act, FS_TRUE );
		IFS_Free( act );
	}
}

static FS_BOOL FS_EmlSysSettingWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	if( cmd == FS_WM_COMMAND && wparam == FS_EV_ITEM_VALUE_CHANGE )
	{
		FS_Widget *wgt = (FS_Widget *)lparam;
		if( wgt->id == FS_W_EmlSysSetRetrHead )
		{
			FS_EmlConfigSetRetrMode( FS_WGT_GET_CHECK(wgt) );
		}
		else if( wgt->id == FS_W_EmlSysSetServBack )
		{
			FS_EmlConfigSetServerBackup( FS_WGT_GET_CHECK(wgt) );
		}
		else if( wgt->id == FS_W_EmlSysSetReplyCopy )
		{
			FS_EmlConfigSetReplyCopy( FS_WGT_GET_CHECK(wgt) );
		}
		else if( wgt->id == FS_W_EmlSysSetSaveSend )
		{
			FS_EmlConfigSetSaveSend( FS_WGT_GET_CHECK(wgt) );
		}
		ret = FS_TRUE;
	}
	return ret;
}
static void FS_EmlSysSetSelectMaxMail_CB( FS_Window *win )
{
	FS_Window *swin;
	FS_Widget *wgt = FS_WindowGetFocusItem( win );
	swin = FS_WindowFindId( FS_W_EmlSysSettingFrm );
	if( swin && wgt )
	{
		FS_SINT4 size = (FS_SINT4)wgt->private_data;
		FS_CHAR str[16];
		
		FS_EmlConfigSetMaxMail( size );
		IFS_Itoa( size, str, 10 );
		IFS_Strcat( str, " K" );
		wgt = FS_WindowGetWidget( swin, FS_W_EmlSysSetMaxMail );
		FS_WidgetSetText( wgt, str );
	}
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_EmlSelectMaxMail_UI( FS_Window *win )
{
	FS_Window *popMenu;
	FS_Widget *item100k, *item300k, *item500k, *item1m;
	FS_Widget *focusWgt = FS_WindowGetFocusItem( win );
	if( focusWgt )
	{
		FS_Rect rect = FS_GetWidgetDrawRect( focusWgt );
		popMenu = FS_CreatePopUpMenu( FS_W_EmlSysSetMaxMailMenu, &rect, 4 );
		item100k = FS_CreateMenuItem( 0, "100 K" );
		item300k = FS_CreateMenuItem( 0, "300 K" );
		item500k = FS_CreateMenuItem( 0, "500 K" );
		item1m = FS_CreateMenuItem( 0, "1024 K" );
		
		item100k->private_data = 100;
		item300k->private_data = 300;
		item500k->private_data = 500;
		item1m->private_data = 1024;
				
		FS_WidgetSetHandler( item100k, FS_EmlSysSetSelectMaxMail_CB );
		FS_WidgetSetHandler( item300k, FS_EmlSysSetSelectMaxMail_CB );		
		FS_WidgetSetHandler( item500k, FS_EmlSysSetSelectMaxMail_CB );
		FS_WidgetSetHandler( item1m, FS_EmlSysSetSelectMaxMail_CB );		
		FS_MenuAddItem( popMenu, item100k );
		FS_MenuAddItem( popMenu, item300k );
		FS_MenuAddItem( popMenu, item500k );
		FS_MenuAddItem( popMenu, item1m );

		FS_MenuSetSoftkey( popMenu );

		FS_ShowWindow( popMenu );
	}
}

static void FS_EmlSysSettingSave_CB( FS_Window *win )
{
	FS_CHAR *str;
	str = FS_WindowGetWidgetText( win, FS_W_EmlSysSetApn );
	FS_EmlConfigSetApn( str );
	str = FS_WindowGetWidgetText( win, FS_W_EmlSysSetUser );
	FS_EmlConfigSetUser( str );
	str = FS_WindowGetWidgetText( win, FS_W_EmlSysSetPass );
	FS_EmlConfigSetPass( str );
	
	FS_EmlConfigSave( );
	FS_MessageBox( FS_MS_OK, FS_Text(FS_T_CONFIG_UPDATED), FS_NULL, FS_FALSE );
	FS_DestroyWindow( win );
}

static void FS_EmlSysSettingRestore_CB( FS_Window *win )
{
	FS_EmlConfigRestore( );
	FS_DestroyWindowByID( FS_W_EmlSysSettingFrm );
}

static void FS_EmlSysSetting_UI( FS_Window *win )
{
	FS_Widget *wRetrHead, *wServBack, *wReplyCopy, *wSaveSend, *wMaxMail, *wApn, *wUser, *wPass;
	FS_Window *sWin;
	FS_CHAR max_size[16];
	FS_EditParam eParam = { FS_IM_ABC, FS_IM_ALL, FS_URL_LEN - 1 };

	IFS_Itoa( FS_EmlConfigGetMaxMail(), max_size, 10 );
	IFS_Strcat( max_size, " K" );
	wRetrHead = FS_CreateCheckBox( FS_W_EmlSysSetRetrHead, FS_Text(FS_T_EML_RETR_HEAD) );
	wMaxMail = FS_CreateComboBox( FS_W_EmlSysSetMaxMail, max_size, FS_I_EDIT );
	wServBack = FS_CreateCheckBox( FS_W_EmlSysSetServBack, FS_Text(FS_T_EML_SERV_BACK) );
	wReplyCopy = FS_CreateCheckBox( FS_W_EmlSysSetReplyCopy, FS_Text(FS_T_EML_REPLY_COPY) );
	wSaveSend = FS_CreateCheckBox( FS_W_EmlSysSetSaveSend, FS_Text(FS_T_EML_SAVE_SEND) );
	wApn = FS_CreateEditBox( FS_W_EmlSysSetApn, FS_EmlConfigGetApn(), FS_I_EDIT, 1, &eParam );
	wUser = FS_CreateEditBox( FS_W_EmlSysSetUser, FS_EmlConfigGetUser(), FS_I_EDIT, 1, &eParam );
	wPass = FS_CreateEditBox( FS_W_EmlSysSetPass, FS_EmlConfigGetPass(), FS_I_EDIT, 1, &eParam );

	FS_WidgetSetHandler( wMaxMail, FS_EmlSelectMaxMail_UI );

	wRetrHead->tip = FS_Text(FS_T_EML_RETR_HEAD);
	wMaxMail->tip = FS_Text(FS_T_EML_MAX_MAIL);
	wServBack->tip = FS_Text(FS_T_EML_SERV_BACK);
	wReplyCopy->tip = FS_Text(FS_T_EML_REPLY_COPY);
	wSaveSend->tip = FS_Text(FS_T_EML_SAVE_SEND);
	wApn->tip = FS_Text(FS_T_APN);
	wUser->tip = FS_Text(FS_T_USER_NAME);
	wPass->tip = FS_Text(FS_T_PASSWORD);
	
	FS_WidgetSetCheck( wRetrHead, FS_EmlConfigGetRetrMode());
	FS_WidgetSetCheck( wServBack, FS_EmlConfigGetServerBackup());
	FS_WidgetSetCheck( wReplyCopy, FS_EmlConfigGetReplyCopy());
	FS_WidgetSetCheck( wSaveSend, FS_EmlConfigGetSaveSend());
	
	sWin = FS_CreateWindow( FS_W_EmlSysSettingFrm, FS_Text(FS_T_SYS_SETTING), FS_EmlSysSettingWndProc );

	sWin->draw_status_bar = FS_TRUE;
	sWin->pane.view_port.height -= IFS_GetLineHeight( );
	
	FS_WindowAddWidget( sWin, wRetrHead );
	FS_WindowAddWidget( sWin, wServBack );
	FS_WindowAddWidget( sWin, wReplyCopy );
	FS_WindowAddWidget( sWin, wSaveSend );
	FS_WindowAddWidget( sWin, wMaxMail );
	FS_WindowAddWidget( sWin, wApn );
	FS_WindowAddWidget( sWin, wUser );
	FS_WindowAddWidget( sWin, wPass );

	FS_WindowSetSoftkey( sWin, 1, FS_Text(FS_T_SAVE), FS_EmlSysSettingSave_CB );
	FS_WindowSetSoftkey( sWin, 3, FS_Text(FS_T_BACK), FS_EmlSysSettingRestore_CB );
	FS_ShowWindow( sWin );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

//-----------------------------------------------------------------------------------
static void FS_EmlActListMainMenu_UI( FS_Window *win )
{
	FS_Widget *mNewAct, *mSysSet, *btnUiSet;
	FS_Window *pMenu;
	mNewAct = FS_CreateMenuItem( 0,	FS_Text(FS_T_EML_NEW_ACCOUNT) );
	mSysSet = FS_CreateMenuItem( 0,  FS_Text(FS_T_SYS_SETTING) );
	btnUiSet = FS_CreateMenuItem( 0,  FS_Text(FS_T_ABOUT) );

	pMenu = FS_CreateMenu( FS_W_EmlActListMainMenu, 3 );
	FS_MenuAddItem( pMenu, mNewAct );
	FS_MenuAddItem( pMenu, mSysSet );
	FS_MenuAddItem( pMenu, btnUiSet );
	FS_WidgetSetHandler( mNewAct, FS_EmlNewAct_CB );
	FS_WidgetSetHandler( mSysSet, FS_EmlSysSetting_UI );
	FS_WidgetSetHandler( btnUiSet, FS_ThemeSetting_UI );
	
	FS_MenuSetSoftkey( pMenu );
	
	FS_ShowWindow( pMenu );
}

//-----------------------------------------------------------------------------------
static void FS_EmlActListMenu_UI( FS_Window *win )
{
	FS_Widget *itemDel, *itemActive, *itemEdit, *itemNew;
	FS_Window *pMenu;
	
	pMenu = FS_CreateMenu( FS_W_EmlActListMenu, 4 );
	itemNew = FS_CreateMenuItem( 0,  FS_Text(FS_T_NEW_ACT) );
	itemActive = FS_CreateMenuItem( 0,  FS_Text(FS_T_ACTIVATE) );
	itemEdit = FS_CreateMenuItem( 0,  FS_Text(FS_T_EDIT) );
	itemDel = FS_CreateMenuItem( 0,  FS_Text(FS_T_DEL) );

	FS_MenuAddItem( pMenu, itemNew );
	FS_MenuAddItem( pMenu, itemActive );
	FS_MenuAddItem( pMenu, itemEdit );
	FS_MenuAddItem( pMenu, itemDel );
	
	FS_WidgetSetHandler( itemNew, FS_EmlNewAct_CB );
	FS_WidgetSetHandler( itemActive, FS_EmlActiveAct_CB );
	FS_WidgetSetHandler( itemEdit, FS_EmlActEdit_CB );
	FS_WidgetSetHandler( itemDel, FS_EmlActDel_CB );

	FS_MenuSetSoftkey( pMenu );
	FS_ShowWindow( pMenu );
}

static void FS_EmlActBuildList( void )
{
	FS_Window *win = FS_WindowFindId( FS_W_EmlActListFrm );
	
	if( win )
	{
		FS_List *node, *head;
		FS_EmlAccount *act;
		FS_Widget *wgt;
		FS_WindowDelWidgetList( win );
		head = FS_EmlGetActList( );
		node = head->next;
		while( node != head )
		{
			act = FS_ListEntry( node, FS_EmlAccount, list );
			if( act->active )
				wgt = FS_CreateListItem( 0, act->account_name, FS_NULL, FS_I_CHECK, 1 );
			else
				wgt = FS_CreateListItem( 0, act->account_name, FS_NULL, FS_I_UNCHECK, 1 );
			FS_WindowAddWidget( win, wgt );
			FS_WidgetSetHandler( wgt, FS_EmlActEdit_CB );
			node = node->next;
		}
	}
}

static FS_BOOL FS_EmlActListWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	if( cmd == FS_WM_SETFOCUS )
	{
		FS_List *node, *head;
		FS_EmlAccount *act;
		FS_Widget *wgt;
		head = FS_WindowGetListItems( win );
		node = head->next;
		// handle for delete and activate account
		while( node != head )
		{
			wgt = FS_ListEntry( node, FS_Widget, list );
			node = node->next;
			act = FS_EmlGetAccount( wgt->text );
			if( act == FS_NULL )
			{
				FS_WindowDelWidget( win, wgt );
				continue;
			}
			
			if( act->active )
				FS_WidgetSetIcon( wgt, FS_I_CHECK );
			else
				FS_WidgetSetIcon( wgt, FS_I_UNCHECK );
		}
		ret = FS_TRUE;
	}
	return ret;
}

static void FS_EmlActList_UI( FS_Window *win )
{
	FS_Window *actListWin;
	FS_List *node, *head = FS_EmlGetActList( );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
	
	node = head->next;
	actListWin = FS_CreateWindow( FS_W_EmlActListFrm, FS_Text(FS_T_EML_ACT_MGR), FS_EmlActListWndProc );
	
	FS_WindowSetSoftkey( actListWin, 1, FS_Text(FS_T_MENU), FS_EmlActListMenu_UI );
	FS_WindowSetSoftkey( actListWin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	
	FS_EmlActBuildList( );
	FS_ShowWindow( actListWin );
}

static void FS_EmlViewSpace_UI( FS_Window *win )
{
	FS_CHAR str[64];
	FS_SINT4 item, item_limit, size, size_limit;

	item_limit = IFS_GetMaxEmlItemLimit( );
	size_limit = IFS_GetMaxEmlSizeLimit( );
	FS_EmlGetActSizeDetail( &size, &item );

	if(size > size_limit) size = size_limit;
	
	IFS_Sprintf( str, "\n%s: %d/%d\n%s: %d/%d(KB)", FS_Text(FS_T_ITEMS), item, item_limit,
			FS_Text(FS_T_SPACE), FS_KB(size), FS_KB(size_limit) );
	FS_StdShowDetail( FS_Text(FS_T_VIEW_SPACE), str );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

//-----------------------------------------------------------------------------------
// exit the email main window
static void FS_EmlMainMenu_UI( FS_Window *win )
{
	FS_Widget *btnActMgr, *btnRecv, *btnNew, *mSysSet, *btnUiSet, *btnViewSpace;
	FS_Window *pMenu;
	btnActMgr = FS_CreateMenuItem( 0,  FS_Text(FS_T_EML_ACT) );
	btnRecv = FS_CreateMenuItem( 0,  FS_Text(FS_T_EML_RECV) );
	btnNew = FS_CreateMenuItem( 0,  FS_Text(FS_T_EML_NEW) );
	mSysSet = FS_CreateMenuItem( 0,  FS_Text(FS_T_SYS_SETTING) );
	btnViewSpace = FS_CreateMenuItem( 0,  FS_Text(FS_T_VIEW_SPACE) );
	btnUiSet = FS_CreateMenuItem( 0,  FS_Text(FS_T_ABOUT) );
	
	pMenu = FS_CreateMenu( FS_W_EmlMainMenu, 6 );
	FS_MenuAddItem( pMenu, btnNew );
	FS_MenuAddItem( pMenu, btnViewSpace );
	FS_MenuAddItem( pMenu, btnRecv );
	FS_MenuAddItem( pMenu, btnActMgr );
	FS_MenuAddItem( pMenu, mSysSet );
	FS_MenuAddItem( pMenu, btnUiSet );
	
	FS_WidgetSetHandler( btnActMgr, FS_EmlActList_UI );
	FS_WidgetSetHandler( btnViewSpace, FS_EmlViewSpace_UI );
	FS_WidgetSetHandler( btnRecv, FS_EmlRetrMailHead_CB );
	FS_WidgetSetHandler( btnNew, FS_EmlNew_CB );
	FS_WidgetSetHandler( mSysSet, FS_EmlSysSetting_UI );
	FS_WidgetSetHandler( btnUiSet, FS_ThemeSetting_UI );
	
	FS_MenuSetSoftkey( pMenu );

	FS_ShowWindow( pMenu );
}

static void FS_EmlDetail_CB( FS_Window *win )
{
	FS_Window *emlList = FS_WindowFindId( FS_W_EmlHeadListFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( emlList );
	FS_UINT1 mbox = FS_EmlGetCurMBox( );
	FS_EmlHead *emlHead;
	FS_CHAR *info;
	FS_SINT4 len;
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );

	if( wgt && wgt->data )
	{
		emlHead = FS_EmlGetHead( mbox, wgt->data );
		info = IFS_Malloc( FS_EML_DETAIL_LEN );
		if( info )
		{
			IFS_Strcpy( info, FS_Text(FS_T_FROM) );
			IFS_Strcat( info, ":" );
			IFS_Strcat( info, emlHead->from.name );
			IFS_Strcat( info, "\n\t<" );
			IFS_Strcat( info, emlHead->from.addr );
			IFS_Strcat( info, ">\n" );

			IFS_Strcat( info, FS_Text(FS_T_DATE) );
			IFS_Strcat( info, ":" );
			len = IFS_Strlen( info );
			FS_DateStruct2DispStr( info + len, &emlHead->date );
			
			IFS_Strcat( info, "\n" );
			IFS_Strcat( info, FS_Text(FS_T_SUBJECT) );
			IFS_Strcat( info, ":" );
			if( emlHead->subject )
				IFS_Strcat( info, emlHead->subject );
			
			IFS_Strcat( info, "\n" );
			IFS_Strcat( info, FS_Text(FS_T_SIZE) );
			IFS_Strcat( info, ":" );
			len = IFS_Strlen( info );
			IFS_Itoa( FS_KB(emlHead->msg_size), info + len, 10 );
			IFS_Strcat( info, " (KB)" );
			FS_StdShowDetail( FS_Text(FS_T_DETAIL), info );
			IFS_Free( info );
		}
	}
}

static FS_BOOL FS_EmlFileDel_Cnf( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	FS_Window *emlList = FS_WindowFindId( FS_W_EmlHeadListFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( emlList );
	FS_UINT1 mbox = FS_EmlGetCurMBox( );
	if( wparam == FS_EV_YES )	// exit without save account data
	{
		FS_EmlDeleteFile( mbox, wgt->data, FS_TRUE );
		FS_WindowDelWidget( emlList, wgt );
		FS_InvalidateRect( emlList, &emlList->client_rect );
		ret = FS_TRUE;
	}
	return ret;
}

static void FS_EmlFileDel_CB( FS_Window *win )
{
	FS_Window *emlList = FS_WindowFindId( FS_W_EmlHeadListFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( emlList );
	FS_UINT1 mbox = FS_EmlGetCurMBox( );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );

	if( wgt && wgt->data )
	{
		FS_MessageBox( FS_MS_YES_NO, FS_Text(FS_T_CONFIRM_DEL), FS_EmlFileDel_Cnf, FS_FALSE );
	}
}

static FS_BOOL FS_EmlDelAll_Cnf( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	FS_Window *emlList = FS_WindowFindId( FS_W_EmlHeadListFrm );
	FS_UINT1 mbox = FS_EmlGetCurMBox( );
	
	if( wparam == FS_EV_YES )	// exit without save account data
	{
		FS_WindowDelWidgetList( emlList );
		FS_EmlDeleteAll( mbox );
		ret = FS_TRUE;
	}
	return ret;
}

static void FS_EmlDelAll_CB( FS_Window *win )
{
	FS_Window *emlList = FS_WindowFindId( FS_W_EmlHeadListFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( emlList );
	FS_UINT1 mbox = FS_EmlGetCurMBox( );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );

	if( wgt )
	{
		FS_MessageBox( FS_MS_YES_NO, FS_Text(FS_T_CONFIRM_DEL), FS_EmlDelAll_Cnf, FS_FALSE );
	}
}

static void FS_EmlDelSvr_CB( FS_Window *win )
{
	FS_CHAR *uid = FS_NULL;
	FS_Window *emlList = FS_WindowFindId( FS_W_EmlHeadListFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( emlList );
	FS_UINT1 mbox = FS_EmlGetCurMBox( );
	FS_Window *msgBox;
	FS_EmlUIContext *emlUICtx = &GFS_EmlUICtx;
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
	
	if( wgt && wgt->data )
	{
		/* delete local first */
		uid = IFS_Strdup( wgt->data );
		FS_EmlDeleteFile( mbox, wgt->data, FS_FALSE );
		FS_WindowDelWidget( emlList, wgt );
	}
	
	if( uid )
	{
		IFS_EnterBackLight( );
		msgBox = FS_MessageBox( FS_MS_INFO_CANCEL, FS_NULL, FS_EmlPop3DlgProc, FS_FALSE );
		msgBox->id = FS_W_EmlRetrProcessFrm;
		
		emlUICtx->action = FS_EmlActDele;
		emlUICtx->text_id = FS_T_CONNECTING;
		emlUICtx->second = 0;
		emlUICtx->timer_id = IFS_StartTimer( FS_TIMER_ID_EML_NET, 1000, FS_EmlNetCounter_CB, emlUICtx );
		emlUICtx->prograss_win = msgBox;
		FS_EmlDisplayNetPrograss( emlUICtx );
		emlUICtx->uid = uid;
		FS_NetConnect( FS_EmlConfigGetApn(), FS_EmlConfigGetUser(), FS_EmlConfigGetPass(), 
			FS_EmlNetConnCallback, FS_APP_EML, FS_TRUE, emlUICtx );
	}
}


static void FS_EmlHeadListMenu_UI( FS_Window *win )
{
	FS_Widget *itemDel, *itemDetail, *itemOp, *itemSvrDel = FS_NULL, *iDelAll;
	FS_Window *pMenu;
	FS_UINT1 mbox = FS_EmlGetCurMBox( );

	if( mbox == FS_EmlInbox )
	{
		itemOp = FS_CreateMenuItem( 0,  FS_Text(FS_T_RETR) );
		FS_WidgetSetHandler( itemOp, FS_EmlRetrive_CB );
		itemSvrDel = FS_CreateMenuItem( 0,	FS_Text(FS_T_EML_DEL_SVR) );
		FS_WidgetSetHandler( itemSvrDel, FS_EmlDelSvr_CB );
		itemDel = FS_CreateMenuItem( 0,  FS_Text(FS_T_EML_DEL) );
	}
	else if( mbox == FS_EmlLocal )
	{
		itemOp = FS_CreateMenuItem( 0,  FS_Text(FS_T_VIEW) );
		FS_WidgetSetHandler( itemOp, FS_EmlView_CB );
		itemSvrDel = FS_CreateMenuItem( 0,	FS_Text(FS_T_EML_DEL_SVR) );
		FS_WidgetSetHandler( itemSvrDel, FS_EmlDelSvr_CB );
		itemDel = FS_CreateMenuItem( 0,  FS_Text(FS_T_EML_DEL) );
	}
	else
	{
		itemOp = FS_CreateMenuItem( 0,  FS_Text(FS_T_EDIT) );
		FS_WidgetSetHandler( itemOp, FS_EmlEdit_CB );
		itemDel = FS_CreateMenuItem( 0,  FS_Text(FS_T_DEL) );
	}
	itemDetail = FS_CreateMenuItem( 0,  FS_Text(FS_T_DETAIL) );
	iDelAll = FS_CreateMenuItem( 0,  FS_Text(FS_T_DEL_ALL) );
	if( itemSvrDel )
		pMenu = FS_CreateMenu( FS_W_EmlHeadListMenu, 5 );
	else
		pMenu = FS_CreateMenu( FS_W_EmlHeadListMenu, 4 );
	FS_MenuAddItem( pMenu, itemOp );
	FS_MenuAddItem( pMenu, itemDetail );
	FS_MenuAddItem( pMenu, itemDel );
	if( itemSvrDel ) FS_MenuAddItem( pMenu, itemSvrDel );
	FS_MenuAddItem( pMenu, iDelAll );
	
	FS_WidgetSetHandler( itemDetail, FS_EmlDetail_CB );
	FS_WidgetSetHandler( itemDel, FS_EmlFileDel_CB );
	FS_WidgetSetHandler( iDelAll, FS_EmlDelAll_CB );
	
	FS_MenuSetSoftkey( pMenu );
	FS_ShowWindow( pMenu );
}

static void FS_EmlHeadList_UI( FS_Window *win )
{
	FS_UINT1 mbox;
	FS_SINT4 i = 0;
	FS_List *node, *head;
	FS_Window *lwin;
	FS_CHAR *title;
	FS_CHAR *subject;
	FS_WidgetEventHandler okHandler;
	FS_Widget *wgt = FS_WindowGetFocusItem( win );
	if( wgt && FS_EmlAccountExist() )
	{
		mbox = (FS_UINT1)(wgt->private_data);
		if( wgt->id == FS_W_EmlMainInbox )
			title = FS_Text( FS_T_EML_INBOX );
		else if( wgt->id == FS_W_EmlMainLocal )
			title = FS_Text( FS_T_RECVBOX );
		else if( wgt->id == FS_W_EmlMainOutbox )
			title = FS_Text( FS_T_OUTBOX );
		else
			title = FS_Text( FS_T_DRAFTBOX );

		head = FS_EmlGetHeadList( FS_NULL, mbox );
		lwin = FS_CreateWindow( FS_W_EmlHeadListFrm, title, FS_NULL );
		lwin->show_index = FS_TRUE;
		if( mbox == FS_EmlInbox )
		{
			okHandler = FS_EmlRetrive_CB;
		}
		else if( mbox == FS_EmlLocal )
		{
			okHandler = FS_EmlView_CB;
		}
		else
		{
			okHandler = FS_EmlEdit_CB;
		}
		
		FS_WindowSetSoftkey( lwin, 1, FS_Text(FS_T_MENU), FS_EmlHeadListMenu_UI );
		FS_WindowSetSoftkey( lwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
		
		node = head->next;
		while( node != head )
		{
			FS_EmlHead *eml = FS_ListEntry( node, FS_EmlHead, list );
			subject = eml->subject;
			if( subject == FS_NULL || subject[0] == 0 )
				subject = FS_Text( FS_T_NO_SUBJECT );
			
			if( (mbox == FS_EmlInbox || mbox == FS_EmlLocal) && eml->read )
				wgt = FS_CreateListItem( i, subject, FS_NULL, FS_I_READED_MSG, 1 );
			else
				wgt = FS_CreateListItem( i, subject, FS_NULL, FS_I_NEW_MSG, 1 );
			FS_WidgetSetHandler( wgt, okHandler );
			FS_WindowAddWidget( lwin, wgt );
			wgt->data = IFS_Strdup( eml->uid );	// save email uid here
			node = node->next;
			i ++;
		}
		FS_ShowWindow( lwin );
	}
}

static FS_BOOL FS_EmlMainWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	if( cmd == FS_WM_PAINT )
	{
		FS_Widget *wNew, *inbox, *local, *outbox, *draft;
		FS_EmlBoxInfo *pInfo;
		FS_CHAR str[32];
		FS_EmlAccount *act;

		wNew = FS_WindowGetWidget( win, FS_W_EmlMainNew );
		inbox = FS_WindowGetWidget( win, FS_W_EmlMainInbox );
		local = FS_WindowGetWidget( win, FS_W_EmlMainLocal );
		outbox = FS_WindowGetWidget( win, FS_W_EmlMainOutbox );
		draft = FS_WindowGetWidget( win, FS_W_EmlMainDraft );
		
		pInfo = FS_EmlGetMailBoxInfo( FS_EmlInbox );
		IFS_Sprintf( str, "%s (%d/%d)", FS_Text(FS_T_EML_INBOX), pInfo->unread, pInfo->total );
		FS_WidgetSetText( inbox, str );
		
		pInfo = FS_EmlGetMailBoxInfo( FS_EmlLocal );
		IFS_Sprintf( str, "%s (%d/%d)", FS_Text(FS_T_RECVBOX), pInfo->unread, pInfo->total );
		FS_WidgetSetText( local, str );
		
		pInfo = FS_EmlGetMailBoxInfo( FS_EmlOutbox );
		IFS_Sprintf( str, "%s (%d/%d)", FS_Text(FS_T_OUTBOX), pInfo->unread, pInfo->total );
		FS_WidgetSetText( outbox, str );
		
		pInfo = FS_EmlGetMailBoxInfo( FS_EmlDraft );
		IFS_Sprintf( str, "%s (%d/%d)", FS_Text(FS_T_DRAFTBOX), pInfo->unread, pInfo->total );
		FS_WidgetSetText( draft, str );
		
		if( inbox->data )
		{
			IFS_Free( inbox->data );
			inbox->data = FS_NULL;
		}
		act = FS_EmlGetActiveAct( );
		if( act ){
			inbox->data = IFS_Strdup( act->account_name );
		}else{
			inbox->data = IFS_Strdup( FS_Text(FS_T_EML_ACT_EMPTY) );
		}
		inbox->tip = inbox->data;
		local->tip = inbox->data;
		outbox->tip = inbox->data;
		draft->tip = inbox->data;
		wNew->tip = inbox->data;
		
		ret = FS_FALSE;
	}
	return ret;
}

//-----------------------------------------------------------------------------------
// open the email main window
void FS_EmlMain( void )
{	
	//FS_Widget *btnSysSetting, *btnRecv, *btnThemeSetting, *btnNew;
	FS_Widget *wNewEml, *liInBox, *liRecvBox, *liOutBox, *liDrafBox;
	FS_Window *win;
	// sys init place here
	FS_EmlSysInit( );
	FS_ActiveApplication( FS_APP_EML );
	// end init
	
	win = FS_CreateWindow( FS_W_EmlMainFrm, FS_Text(FS_T_EML), FS_EmlMainWndProc );
	
	FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_MENU), FS_EmlMainMenu_UI );
	FS_WindowSetSoftkey( win, 3, FS_Text(FS_T_EXIT), FS_EmlExit_CB );

	wNewEml = FS_CreateListItem( FS_W_EmlMainNew, FS_Text(FS_T_EML_NEW), FS_NULL, FS_I_NEW_MSG, 1 );
	liInBox = FS_CreateListItem( FS_W_EmlMainInbox, FS_NULL, FS_NULL, FS_I_DIR, 1 );
	liRecvBox = FS_CreateListItem( FS_W_EmlMainLocal, FS_NULL, FS_NULL, FS_I_DIR, 1 );
	liOutBox = FS_CreateListItem( FS_W_EmlMainOutbox, FS_NULL, FS_NULL, FS_I_DIR, 1 );
	liDrafBox = FS_CreateListItem( FS_W_EmlMainDraft, FS_NULL, FS_NULL, FS_I_DIR, 1 );

	liInBox->private_data = FS_EmlInbox;
	liRecvBox->private_data = FS_EmlLocal;
	liOutBox->private_data = FS_EmlOutbox;
	liDrafBox->private_data = FS_EmlDraft;
	
	FS_WidgetSetHandler( liInBox, FS_EmlHeadList_UI );
	FS_WidgetSetHandler( liRecvBox, FS_EmlHeadList_UI );
	FS_WidgetSetHandler( liOutBox, FS_EmlHeadList_UI );
	FS_WidgetSetHandler( liDrafBox, FS_EmlHeadList_UI );
	FS_WidgetSetHandler( wNewEml, FS_EmlNew_CB );

	win->draw_status_bar = FS_TRUE;
	win->pane.view_port.height -= IFS_GetLineHeight( );
	
	FS_WindowAddWidget( win, wNewEml );
	FS_WindowAddWidget( win, liInBox );
	FS_WindowAddWidget( win, liRecvBox );
	FS_WindowAddWidget( win, liOutBox );
	FS_WindowAddWidget( win, liDrafBox );

	FS_ShowWindow( win );	
}

void FS_EmlExit( void )
{
	FS_EmlExit_CB( FS_NULL );
}

#endif	//FS_MODULE_EML

