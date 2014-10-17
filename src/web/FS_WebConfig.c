#include "inc/FS_Config.h"

#ifdef FS_MODULE_WEB

#include "inc/web/FS_WebConfig.h"
#include "inc/inte/FS_Inte.h" 
#include "inc/util/FS_File.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_MemDebug.h"

#define FS_WEB_CFG_FILE		"WebConfig.bin"

typedef struct FS_WebConfig_Tag
{
	/* connection and protocol setting */
	FS_UINT1		protocol;
	FS_BOOL			use_proxy;

	FS_CHAR			proxy_addr[FS_URL_LEN];
	FS_UINT2		proxy_port;

	FS_CHAR			apn[FS_URL_LEN];
	FS_CHAR			user_name[FS_URL_LEN];
	FS_CHAR			password[FS_URL_LEN];
	
	/* general options */
	FS_BOOL			play_audio;
	FS_BOOL			show_image;
	FS_BOOL			use_cache_image;
	FS_CHAR			home_page[FS_URL_LEN];
	FS_SINT4		max_network_timer;

	FS_BOOL					sc_enable;
	FS_WebShortCut			sc_table[FS_SC_MAX_NUM];
}FS_WebConfig;

#define FS_DEFAULT_HOME_PAGE	"http://wap.monternet.com"
#define FS_DETAULT_APN			"CMWAP"

#define FS_DEFAULT_MAX_NETWORK_TIMER	30
static FS_WebConfig GFS_WebConfig;

FS_SINT4 FS_WebConfigGetProtocol( void )
{
	return GFS_WebConfig.protocol;
}

FS_CHAR *FS_WebConfigGetProxyAddr( void )
{
	return GFS_WebConfig.proxy_addr;
}

FS_UINT2 FS_WebConfigGetProxyPort( void )
{
	return GFS_WebConfig.proxy_port;
}

FS_BOOL FS_WebConfigGetUseProxy( void )
{
	return GFS_WebConfig.use_proxy;
}

void FS_WebConfigSetProtocol( FS_UINT1 protocol )
{
	GFS_WebConfig.protocol = protocol;
}

void FS_WebConfigSetProxyAddr( FS_CHAR *proxy )
{
	if( proxy )
		IFS_Strncpy( GFS_WebConfig.proxy_addr, proxy, sizeof(GFS_WebConfig.proxy_addr) - 1 );
	else
		GFS_WebConfig.proxy_addr[0] = 0;
}

void FS_WebConfigSetProxyPort( FS_UINT2 port )
{
	GFS_WebConfig.proxy_port = port;
}

void FS_WebConfigSetUseProxy( FS_BOOL use )
{
	GFS_WebConfig.use_proxy = use;
}

FS_BOOL FS_WebConfigGetShowImageFlag( void )
{
	return GFS_WebConfig.show_image;
}

void FS_WebConfigSetShowImageFlag( FS_BOOL bYes )
{
	GFS_WebConfig.show_image = bYes;
}

FS_BOOL FS_WebConfigGetPlayAudioFlag( void )
{
	return GFS_WebConfig.play_audio;
}

void FS_WebConfigSetPlayAudioFlag( FS_BOOL bYes )
{
	GFS_WebConfig.play_audio = bYes;
}

FS_BOOL FS_WebConfigGetUseCacheImageFlag( void )
{
	return GFS_WebConfig.use_cache_image;
}

void FS_WebConfigSetUseCacheImageFlag( FS_BOOL bYes )
{
	GFS_WebConfig.use_cache_image = bYes;
}

FS_CHAR * FS_WebConfigGetHomePage( void )
{
	return GFS_WebConfig.home_page;
}

void FS_WebConfigSetHomePage( FS_CHAR * url )
{
	if( url )
		IFS_Strncpy( GFS_WebConfig.home_page, url, sizeof(GFS_WebConfig.home_page) - 1 );
	else
		IFS_Strcpy( GFS_WebConfig.home_page, FS_DEFAULT_HOME_PAGE );
}

FS_CHAR * FS_WebConfigGetApn( void )
{
	return GFS_WebConfig.apn;
}

void FS_WebConfigSetApn( FS_CHAR * apn )
{
	if( apn )
	{
		IFS_Memset( GFS_WebConfig.apn, 0, sizeof(GFS_WebConfig.apn) );
		IFS_Strncpy( GFS_WebConfig.apn, apn, sizeof(GFS_WebConfig.apn) - 1 );
	}
	else
	{
		IFS_Strcpy( GFS_WebConfig.apn, FS_DETAULT_APN );
	}
}

FS_CHAR * FS_WebConfigGetUserName( void )
{
	return GFS_WebConfig.user_name;
}

void FS_WebConfigSetUserName( FS_CHAR * user_name )
{
	if( user_name )
		IFS_Strncpy( GFS_WebConfig.user_name, user_name, sizeof(GFS_WebConfig.user_name) - 1 );
	else
		IFS_Strcpy( GFS_WebConfig.user_name, "" );
}

FS_CHAR * FS_WebConfigGetPassword( void )
{
	return GFS_WebConfig.password;
}

void FS_WebConfigSetPassword( FS_CHAR * password )
{
	if( password )
		IFS_Strncpy( GFS_WebConfig.password, password, sizeof(GFS_WebConfig.password) - 1 );
	else
		IFS_Strcpy( GFS_WebConfig.password, "" );
}


FS_SINT4 FS_WebConfigGetMaxNetworkTimer( void )
{
	return GFS_WebConfig.max_network_timer;
}

void FS_WebConfigSetMaxNetworkTimer( FS_SINT4 value )
{
	if( value <= 0 )
		GFS_WebConfig.max_network_timer = FS_DEFAULT_MAX_NETWORK_TIMER;
	else
		GFS_WebConfig.max_network_timer = value;
}

void FS_WebConfigShortCutSetDefault( void )
{
	IFS_Memset( GFS_WebConfig.sc_table, 0, sizeof(GFS_WebConfig.sc_table) );
	GFS_WebConfig.sc_enable = FS_TRUE;

	GFS_WebConfig.sc_table[0] = FS_WSC_NET_SETTING;
	GFS_WebConfig.sc_table[1] = FS_WSC_HISTORY;
	GFS_WebConfig.sc_table[2] = FS_WSC_FAVORITE;
	GFS_WebConfig.sc_table[3] = FS_WSC_RECOMMAND;
	GFS_WebConfig.sc_table[4] = FS_WSC_BACKWARD;
	GFS_WebConfig.sc_table[5] = FS_WSC_REFRESH;
	GFS_WebConfig.sc_table[6] = FS_WSC_FORWARD;
	GFS_WebConfig.sc_table[7] = FS_WSC_HOME_PAGE;
	GFS_WebConfig.sc_table[8] = FS_WSC_GOTO_URL;
	GFS_WebConfig.sc_table[9] = FS_WSC_SAVE_URL;
}

FS_BOOL FS_WebConfigGetShortCutEnableFlag( void )
{
	return GFS_WebConfig.sc_enable;
}

void FS_WebConfigSetShortCutEnableFlag( FS_BOOL enable )
{
	GFS_WebConfig.sc_enable = enable;
}

FS_WebShortCut FS_WebConfigGetShortCut( FS_KeyEvent key )
{
	FS_SINT4 index;
	index = key - FS_KEY_0;
	if( index >= 0 && index < FS_SC_MAX_NUM )
	{
		return GFS_WebConfig.sc_table[index];
	}
	else
	{
		return FS_WSC_NONE;
	}
}

void FS_WebConfigSetShortCut( FS_WebShortCut shortcut, FS_KeyEvent key )
{
	FS_SINT4 index;
	index = key - FS_KEY_0;
	if( index >= 0 && index < FS_SC_MAX_NUM )
	{
		GFS_WebConfig.sc_table[index] = shortcut;
	}
}

void FS_WebConfigInit( void )
{
	if( FS_FileRead( FS_DIR_WEB, FS_WEB_CFG_FILE, 0, &GFS_WebConfig, sizeof(FS_WebConfig) ) != sizeof(FS_WebConfig) )
	{
		/* set to default */
#ifdef FS_PLT_WIN
		GFS_WebConfig.protocol = FS_WEB_HTTP;
		GFS_WebConfig.use_proxy = FS_FALSE;
		GFS_WebConfig.proxy_port = 80;
#else
		GFS_WebConfig.protocol = FS_WEB_WSP;
		GFS_WebConfig.use_proxy = FS_TRUE;
		GFS_WebConfig.proxy_port = 9201;
#endif
		IFS_Strcpy( GFS_WebConfig.proxy_addr, "10.0.0.172" );
		IFS_Strcpy( GFS_WebConfig.apn, FS_DETAULT_APN );
		GFS_WebConfig.show_image = FS_TRUE;
		GFS_WebConfig.use_cache_image = FS_TRUE;
		IFS_Strcpy( GFS_WebConfig.home_page, FS_DEFAULT_HOME_PAGE );
		GFS_WebConfig.max_network_timer = FS_DEFAULT_MAX_NETWORK_TIMER;

		FS_WebConfigShortCutSetDefault();
	}
}

void FS_WebConfigSave( void )
{
	FS_FileWrite( FS_DIR_WEB, FS_WEB_CFG_FILE, 0, &GFS_WebConfig, sizeof(FS_WebConfig) );
}

void FS_WebConfigSetDefault( void )
{
	FS_FileDelete( FS_DIR_WEB, FS_WEB_CFG_FILE );
	FS_WebConfigInit( );
}

#endif	//FS_MODULE_WEB


