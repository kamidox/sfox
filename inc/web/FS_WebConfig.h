#ifndef _FS_WEB_CONFIG_H_
#define _FS_WEB_CONFIG_H_

#include "inc/FS_Config.h"
/* protocol */
#define FS_WEB_WSP 		0
#define FS_WEB_HTTP 		1

#define FS_SC_MAX_NUM		10

typedef enum FS_WebShortCut_Tag
{
	FS_WSC_NONE = 0,
	FS_WSC_HOME_PAGE,			/* open home page */
	FS_WSC_FAVORITE,			/* open my favorite */
	FS_WSC_RECOMMAND,			/* open recommand url list */
	FS_WSC_GOTO_URL,				/* open url edit box */
	FS_WSC_HISTORY,				/* open history list */

	FS_WSC_SAVE_URL,				/* save current page url */

	FS_WSC_GEN_SETTING,		
	FS_WSC_NET_SETTING,		
	FS_WSC_SHORTCUT_SETTING,	

	FS_WSC_FORWARD,			
	FS_WSC_REFRESH,			
	FS_WSC_BACKWARD,

	FS_WSC_PAGE_START,
	FS_WSC_PAGE_END,
	FS_WSC_COUNT
}FS_WebShortCut;

/*---------------------------- getters -----------------------------------*/
FS_SINT4 FS_WebConfigGetProtocol( void );

FS_CHAR *FS_WebConfigGetProxyAddr( void );

FS_UINT2 FS_WebConfigGetProxyPort( void );

FS_BOOL FS_WebConfigGetUseProxy( void );

FS_BOOL FS_WebConfigGetShowTitleFlag( void );

FS_BOOL FS_WebConfigGetShowImageFlag( void );

FS_BOOL FS_WebConfigGetUseCacheImageFlag( void );

FS_CHAR * FS_WebConfigGetHomePage( void );

FS_SINT4 FS_WebConfigGetMaxNetworkTimer( void );

/*---------------------------- setters -----------------------------------*/
void FS_WebConfigSetProtocol( FS_UINT1 protocol );

void FS_WebConfigSetProxyAddr( FS_CHAR *proxy );

void FS_WebConfigSetProxyPort( FS_UINT2 port );

void FS_WebConfigSetUseProxy( FS_BOOL use );

void FS_WebConfigSetShowTitleFlag( FS_BOOL bYes );

void FS_WebConfigSetShowImageFlag( FS_BOOL bYes );

void FS_WebConfigSetPlayAudioFlag( FS_BOOL bYes );

FS_BOOL FS_WebConfigGetPlayAudioFlag( void );

void FS_WebConfigSetUseCacheImageFlag( FS_BOOL bYes );

void FS_WebConfigSetHomePage( FS_CHAR * url );

FS_CHAR * FS_WebConfigGetApn( void );

void FS_WebConfigSetApn( FS_CHAR * apn );

FS_CHAR * FS_WebConfigGetUserName( void );

void FS_WebConfigSetUserName( FS_CHAR * user_name );

FS_CHAR * FS_WebConfigGetPassword( void );

void FS_WebConfigSetPassword( FS_CHAR * password );

void FS_WebConfigInit( void );

void FS_WebConfigSave( void );

void FS_WebConfigSetMaxNetworkTimer( FS_SINT4 value );

FS_WebShortCut FS_WebConfigGetShortCut( FS_KeyEvent key );

void FS_WebConfigSetShortCut( FS_WebShortCut shortcut, FS_KeyEvent key );

FS_BOOL FS_WebConfigGetShortCutEnableFlag( void );

void FS_WebConfigSetShortCutEnableFlag( FS_BOOL enable );

void FS_WebConfigSetDefault( void );


#endif
