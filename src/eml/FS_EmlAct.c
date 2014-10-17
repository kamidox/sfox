#include "inc/FS_Config.h"

#ifdef FS_MODULE_EML

#include "inc\eml\FS_EmlAct.h"
#include "inc\inte\FS_Inte.h"
#include "inc\util\FS_File.h"
#include "inc\eml\FS_EmlFile.h"
#include "inc\util\FS_MemDebug.h"

#define FS_EML_ACT_LST_FILE		"EmlAct.lst"
#define FS_EML_CONFIG_FILE		"EmlConfig.cfg"

static FS_List GFS_EmlActList = { &GFS_EmlActList, &GFS_EmlActList };

static FS_EmlConfig GFS_EmlConfig = { FS_TRUE, 1024, FS_TRUE, FS_FALSE, FS_TRUE };

static void FS_EmlUpdateActFile( void )
{
	FS_BYTE *pBuf = FS_NULL, *pos;
	FS_SINT4 nSize, i = 0;
	FS_List *node;
	FS_EmlAccount *pAct;

	i = FS_ListCount( &GFS_EmlActList );
	if( i > 0 )
	{
		nSize = i * sizeof(FS_EmlAccount);
		pBuf = IFS_Malloc( nSize );
	}
	else
	{
		FS_FileDelete( FS_DIR_EML, FS_EML_ACT_LST_FILE );
	}
	
	if( pBuf )
	{
		pos = pBuf;
		node = GFS_EmlActList.next;
		while( node != &GFS_EmlActList )
		{
			pAct = FS_ListEntry( node, FS_EmlAccount, list );
			IFS_Memcpy( pos, pAct, sizeof(FS_EmlAccount));
			pos += sizeof(FS_EmlAccount);
			node = node->next;
		}
		FS_FileWrite( FS_DIR_EML, FS_EML_ACT_LST_FILE, 0, pBuf, nSize );
		IFS_Free( pBuf );
	}
}

void FS_EmlSaveAccount( FS_EmlAccount *act )
{
	FS_EmlAccount *pAct;
	FS_EmlAccount *thisAct;
	
	if( FS_ListIsEmpty(&GFS_EmlActList) )
		act->active = FS_TRUE;
	thisAct = FS_EmlGetAccount( act->account_name );
	if( thisAct )	// modify it
	{
		IFS_Memcpy( thisAct->disp_name, act->disp_name, sizeof(act->disp_name) );
		IFS_Memcpy( thisAct->user_name, act->user_name, sizeof(act->user_name) );
		IFS_Memcpy( thisAct->password, act->password, sizeof(act->password) );
		IFS_Memcpy( thisAct->eml_addr, act->eml_addr, sizeof(act->eml_addr) );
		IFS_Memcpy( thisAct->recv_addr, act->recv_addr, sizeof(act->recv_addr) );
		IFS_Memcpy( thisAct->smtp_addr, act->smtp_addr, sizeof(act->smtp_addr) );
		thisAct->recv_port = act->recv_port;
		thisAct->smtp_port = act->smtp_port;
		thisAct->smtp_auth = act->smtp_auth;
	}
	else	// add new
	{
		pAct = IFS_Malloc( sizeof(FS_EmlAccount) );
		if( pAct )
		{
			IFS_Memset( pAct, 0, sizeof(FS_EmlAccount) );
			IFS_Memcpy( pAct, act, sizeof(FS_EmlAccount) );
			// generate email box list file name, and save it
			FS_GetGuid( pAct->mbox_file[FS_EmlInbox] );
			IFS_Strcat( pAct->mbox_file[FS_EmlInbox], "_inbox.lst" );
			FS_GetGuid( pAct->mbox_file[FS_EmlLocal] );	
			IFS_Strcat( pAct->mbox_file[FS_EmlLocal], "_local.lst" );
			FS_GetGuid( pAct->mbox_file[FS_EmlOutbox] );	
			IFS_Strcat( pAct->mbox_file[FS_EmlOutbox], "_outbox.lst" );
			FS_GetGuid( pAct->mbox_file[FS_EmlDraft] );	
			IFS_Strcat( pAct->mbox_file[FS_EmlDraft], "_draft.lst" );
			FS_GetGuid( pAct->uidl );
			IFS_Strcat( pAct->uidl, "_uidl.lst" );
			
			FS_ListAdd( &GFS_EmlActList, &pAct->list );
		}
	}
	// save data to file
	FS_EmlUpdateActFile( );
}

void FS_EmlActivateAct( FS_CHAR *actname )
{
	FS_List *node = GFS_EmlActList.next;
	FS_EmlAccount *act;
	if( actname == FS_NULL )	// set the first account active
	{
		if( node != &GFS_EmlActList )
		{
			act = FS_ListEntry( node, FS_EmlAccount, list );
			act->active = FS_TRUE;
		}
	}
	else	// reset active account
	{
		while( node != &GFS_EmlActList )
		{
			act = FS_ListEntry( node, FS_EmlAccount, list );
			if( IFS_Strcmp( act->account_name, actname ) == 0 )
				act->active = FS_TRUE;
			else
				act->active = FS_FALSE;

			node = node->next;
		}
	}
	FS_EmlUpdateActFile( );
	FS_EmlResetMBoxInfo( );
}

void FS_EmlDelAccount( FS_CHAR *actname )
{
	FS_List *node = GFS_EmlActList.next;
	FS_EmlAccount *act;
	FS_BOOL reActivate = FS_FALSE;
	while( node != &GFS_EmlActList )
	{
		act = FS_ListEntry( node, FS_EmlAccount, list );
		node = node->next;
		if( IFS_Strcmp( act->account_name, actname ) == 0 )
		{
			reActivate = act->active;
			FS_ListDel( &act->list );
			FS_EmlClearBox( act, FS_EmlInbox );
			FS_EmlClearBox( act, FS_EmlLocal );
			FS_EmlClearBox( act, FS_EmlOutbox );
			FS_EmlClearBox( act, FS_EmlDraft );

			FS_FileDelete( FS_DIR_EML, act->uidl );
			IFS_Free( act );
			break;
		}
	}
	if( reActivate )
		FS_EmlActivateAct( FS_NULL );
	FS_EmlUpdateActFile( );
}

FS_EmlAccount * FS_EmlGetAccount( FS_CHAR *actname )
{
	FS_List *node = GFS_EmlActList.next;
	FS_EmlAccount *act;
	if( actname )
	{
		while( node != &GFS_EmlActList )
		{
			act = FS_ListEntry( node, FS_EmlAccount, list );
			if( IFS_Strcmp( act->account_name, actname ) == 0 )
				return act;
			node = node->next;
		}
	}
	return FS_NULL;
}

FS_EmlAccount * FS_EmlGetActiveAct( void )
{
	FS_List *node = GFS_EmlActList.next;
	FS_EmlAccount *act;
	while( node != &GFS_EmlActList )
	{
		act = FS_ListEntry( node, FS_EmlAccount, list );
		if( act->active )
			return act;
		node = node->next;
	}
	return FS_NULL;
}

FS_List * FS_EmlGetActList( void )
{
	return &GFS_EmlActList;
}

FS_SINT4 FS_EmlGetActNum( void )
{
	return FS_ListCount( &GFS_EmlActList );
}

void FS_EmlInitAct( void )
{
	FS_SINT4 size = 0, i, total;
	FS_BYTE * buf, *pos;
	FS_EmlAccount *pAct;
	FS_EmlDeinitAct( );
	FS_ListInit( &GFS_EmlActList );

	size = FS_FileGetSize( FS_DIR_EML, FS_EML_ACT_LST_FILE );
	if( size <= 0 ) return;
	
	buf = IFS_Malloc( size );
	FS_ASSERT( buf != FS_NULL );
	if( buf == FS_NULL ) return;
	
	if( FS_FileRead( FS_DIR_EML, FS_EML_ACT_LST_FILE, 0, buf, size ) != size )
	{
		IFS_Free( buf );
		return;
	}
	
	total = size / sizeof(FS_EmlAccount);
	pos = buf;
	for( i = 0; i < total; i ++ )
	{
		pAct = IFS_Malloc( sizeof(FS_EmlAccount) );
		if( pAct )
		{
			IFS_Memcpy( pAct, pos, sizeof(FS_EmlAccount) );
			FS_ListAdd( &GFS_EmlActList, &pAct->list );
			pos += sizeof(FS_EmlAccount);
		}
	}
	IFS_Free( buf );

	/* here, read email config data */
	FS_EmlConfigRestore();
}

void FS_EmlDeinitAct( void )  
{
	FS_EmlAccount *act;
	FS_List *node = GFS_EmlActList.next;
	while( node != &GFS_EmlActList )
	{
		act = FS_ListEntry( node, FS_EmlAccount, list );
		node = node->next;
		FS_ListDel( &act->list );
		IFS_Free( act );
	}
	FS_ListInit( &GFS_EmlActList );
}

/**************************************************************************
*
*		email system config interface
*
***************************************************************************/
void FS_EmlConfigSetServerBackup( FS_BOOL bval )
{
	GFS_EmlConfig.server_backup = bval;
}

FS_BOOL FS_EmlConfigGetServerBackup( void )
{
	return GFS_EmlConfig.server_backup;
}

/* max mail to retr, size in KB */
void FS_EmlConfigSetMaxMail( FS_SINT4 size )
{
	GFS_EmlConfig.max_mail = size;
}

FS_SINT4 FS_EmlConfigGetMaxMail( void )
{
	return GFS_EmlConfig.max_mail;
}

void FS_EmlConfigSetRetrMode( FS_BOOL bval )
{
	GFS_EmlConfig.retr_head = bval;
}

FS_BOOL FS_EmlConfigGetRetrMode( void )
{
	return GFS_EmlConfig.retr_head;
}

void FS_EmlConfigSetReplyCopy( FS_BOOL bval )
{
	GFS_EmlConfig.reply_copy = bval;
}

FS_BOOL FS_EmlConfigGetReplyCopy( void )
{
	return GFS_EmlConfig.reply_copy;
}

void FS_EmlConfigSetSaveSend( FS_BOOL bval )
{
	GFS_EmlConfig.save_send = bval;
}

FS_BOOL FS_EmlConfigGetSaveSend( void )
{
	return GFS_EmlConfig.save_send;
}

FS_CHAR * FS_EmlConfigGetApn( void )
{
	return GFS_EmlConfig.apn;
}

void FS_EmlConfigSetApn( FS_CHAR * apn )
{
	IFS_Memset( GFS_EmlConfig.apn, 0, sizeof(GFS_EmlConfig.apn) );
	if( apn )
		IFS_Strncpy( GFS_EmlConfig.apn, apn, sizeof(GFS_EmlConfig.apn) - 1 );
}

FS_CHAR * FS_EmlConfigGetUser( void )
{
	return GFS_EmlConfig.user;
}

void FS_EmlConfigSetUser( FS_CHAR * user )
{
	IFS_Memset( GFS_EmlConfig.user, 0, sizeof(GFS_EmlConfig.user) );
	if( user )
		IFS_Strncpy( GFS_EmlConfig.user, user, sizeof(GFS_EmlConfig.user) - 1 );
}

FS_CHAR * FS_EmlConfigGetPass( void )
{
	return GFS_EmlConfig.pass;
}

void FS_EmlConfigSetPass( FS_CHAR * pass )
{
	IFS_Memset( GFS_EmlConfig.pass, 0, sizeof(GFS_EmlConfig.pass) );
	if( pass )
		IFS_Strncpy( GFS_EmlConfig.pass, pass, sizeof(GFS_EmlConfig.pass) - 1 );
}

void FS_EmlConfigSave( void )
{
	FS_FileWrite( FS_DIR_EML, FS_EML_CONFIG_FILE, 0, &GFS_EmlConfig, sizeof(FS_EmlConfig) );	
}

void FS_EmlConfigRestore( void )
{
	FS_SINT4 rlen = FS_FileRead( FS_DIR_EML, FS_EML_CONFIG_FILE, 0, &GFS_EmlConfig, sizeof(FS_EmlConfig) );
	if( rlen != sizeof(FS_EmlConfig) )
	{
		GFS_EmlConfig.server_backup = FS_TRUE;
		GFS_EmlConfig.max_mail = 1024;
		GFS_EmlConfig.retr_head = FS_TRUE;
		GFS_EmlConfig.reply_copy = FS_FALSE;
		GFS_EmlConfig.save_send = FS_TRUE;
		IFS_Strcpy( GFS_EmlConfig.apn, "CMNET" );
	}
}

#endif	//FS_MODULE_EML
