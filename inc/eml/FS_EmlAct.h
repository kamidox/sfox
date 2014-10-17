#ifndef _FS_EML_ACT_H_
#define _FS_EML_ACT_H_
#include "inc/FS_Config.h"
#include "inc/util/FS_List.h"
#include "inc/util/FS_File.h"
#include "inc/eml/FS_EmlMain.h"

typedef enum FS_EmlType_Tag
{
	FS_EML_POP3 = 1,
	FS_EML_IMAP4	
}FS_EmlType;

typedef struct FS_EmlAccount_Tag
{
	// account info
	FS_List				list;
	FS_CHAR 			account_name[FS_EML_MIN_STR];	// account name
	FS_EmlType			type;							// pop3 or imap4
	FS_CHAR				disp_name[FS_EML_MIN_STR];
	FS_CHAR				eml_addr[FS_EML_ADDR_LEN];
	FS_CHAR				recv_addr[FS_EML_ADDR_LEN];		// may pop3 or imap4 address
	FS_CHAR				smtp_addr[FS_EML_ADDR_LEN];
	FS_CHAR				user_name[FS_EML_MIN_STR];
	FS_CHAR				password[FS_EML_MIN_STR];
	FS_UINT2			recv_port;
	FS_UINT2			smtp_port;
	FS_BOOL				smtp_auth;						// auth login when login smtp
	FS_BOOL				ssl_on;							// reserved. use ssl connection
	FS_BOOL				active;							// active? if true, then is current account

	// account related data info, eg. inbox, outbox etc.
	FS_CHAR				mbox_file[4][FS_FILE_NAME_LEN];
	FS_CHAR				uidl[FS_FILE_NAME_LEN];	/* deleted mail's uidl list */
}FS_EmlAccount;

/* email system config data */
typedef struct FS_EmlConfig_Tag
{
	FS_BOOL 		server_backup;	/* reserve server backup mail */
	FS_SINT4		max_mail;		/* max email to retrieve in KB */
	FS_BOOL 		retr_head;		/* retrieve mail head first */
	FS_BOOL			reply_copy;		/* copy original email text when reply */
	FS_BOOL			save_send;		/* save sended mail to outbox */
	
	FS_CHAR			apn[FS_URL_LEN];
	FS_CHAR			user[FS_URL_LEN];
	FS_CHAR			pass[FS_URL_LEN];	
}FS_EmlConfig;

void FS_EmlActivateAct( FS_CHAR *actname );
void FS_EmlSaveAccount( FS_EmlAccount *act );
void FS_EmlDelAccount( FS_CHAR *actname );
void FS_EmlInitAct( void );
void FS_EmlDeinitAct( void );
FS_EmlAccount * FS_EmlGetAccount( FS_CHAR *actname );
FS_EmlAccount * FS_EmlGetActiveAct( void );
FS_List * FS_EmlGetActList( void );
FS_SINT4 FS_EmlGetActNum( void );

/***************** email system config interface **************************/

void FS_EmlConfigSetServerBackup( FS_BOOL bval );

FS_BOOL FS_EmlConfigGetServerBackup( void );

void FS_EmlConfigSetMaxMail( FS_SINT4 size );

FS_SINT4 FS_EmlConfigGetMaxMail( void );

void FS_EmlConfigSetRetrMode( FS_BOOL bval );

FS_BOOL FS_EmlConfigGetRetrMode( void );

void FS_EmlConfigSetReplyCopy( FS_BOOL bval );

FS_BOOL FS_EmlConfigGetReplyCopy( void );

void FS_EmlConfigSetSaveSend( FS_BOOL bval );

FS_BOOL FS_EmlConfigGetSaveSend( void );

FS_CHAR * FS_EmlConfigGetApn( void );

void FS_EmlConfigSetApn( FS_CHAR * apn );

FS_CHAR * FS_EmlConfigGetUser( void );

void FS_EmlConfigSetUser( FS_CHAR * user );

FS_CHAR * FS_EmlConfigGetPass( void );

void FS_EmlConfigSetPass( FS_CHAR * pass );

void FS_EmlConfigSave( void );

void FS_EmlConfigRestore( void );

#endif
