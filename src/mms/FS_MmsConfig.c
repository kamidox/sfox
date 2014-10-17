#include "inc/FS_Config.h"

#ifdef FS_MODULE_MMS

#include "inc/mms/FS_MmsConfig.h"
#include "inc/inte/FS_Inte.h" 
#include "inc/util/FS_File.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_MemDebug.h"

#define FS_MMS_CFG_FILE		"MmsConfig.bin"

typedef struct FS_MmsConfig_Tag
{
	/* connection and protocol setting */
	FS_UINT1		protocol;

	FS_CHAR			proxy_addr[FS_URL_LEN];
	FS_UINT2		proxy_port;
	FS_CHAR			mms_center_url[FS_URL_LEN];

	FS_CHAR			apn[FS_URL_LEN];
	FS_CHAR			user_name[FS_URL_LEN];
	FS_CHAR			password[FS_URL_LEN];

	/* general options */
	FS_BOOL			save_send;
	FS_BOOL			allow_ads;
	FS_BOOL 		auto_recv;
	FS_BOOL			allow_delivery_report;
	FS_BOOL			allow_read_report;
}FS_MmsConfig;

static FS_MmsConfig GFS_MmsConfig;

FS_SINT4 FS_MmsConfigGetProtocol( void )
{
	return GFS_MmsConfig.protocol;
}

FS_CHAR *FS_MmsConfigGetProxyAddr( void )
{
	return GFS_MmsConfig.proxy_addr;
}

FS_UINT2 FS_MmsConfigGetProxyPort( void )
{
	return GFS_MmsConfig.proxy_port;
}

FS_CHAR *FS_MmsConfigGetMmsCenterUrl( void )
{
	return GFS_MmsConfig.mms_center_url;
}

FS_BOOL FS_MmsConfigGetAutoRecvFlag( void )
{
	return GFS_MmsConfig.auto_recv;
}

FS_BOOL FS_MmsConfigGetAllowAdsFlag( void )
{
	return GFS_MmsConfig.allow_ads;
}

FS_BOOL FS_MmsConfigGetAllowDeliveryReportFlag( void )
{
	return GFS_MmsConfig.allow_delivery_report;
}

FS_BOOL FS_MmsConfigGetAllowReadReportFlag( void )
{
	return GFS_MmsConfig.allow_read_report;
}

FS_BOOL FS_MmsConfigGetSaveSendFlag( void )
{
	return GFS_MmsConfig.save_send;
}

void FS_MmsConfigSetSaveSendFlag( FS_BOOL value )
{
	GFS_MmsConfig.save_send = value;
}

void FS_MmsConfigSetProtocol( FS_UINT1 protocol )
{
	GFS_MmsConfig.protocol = protocol;
}

void FS_MmsConfigSetProxyAddr( FS_CHAR *proxy )
{
	if( proxy )
		IFS_Strncpy( GFS_MmsConfig.proxy_addr, proxy, sizeof(GFS_MmsConfig.proxy_addr) - 1 );
	else
		GFS_MmsConfig.proxy_addr[0] = 0;
}

void FS_MmsConfigSetProxyPort( FS_UINT2 port )
{
	GFS_MmsConfig.proxy_port = port;
}

void FS_MmsConfigSetAutoRecvFlag( FS_BOOL bOK )
{
	GFS_MmsConfig.auto_recv = bOK;
}

void FS_MmsConfigSetAllowAdsFlag( FS_BOOL bOK )
{
	GFS_MmsConfig.allow_ads = bOK;
}

void FS_MmsConfigSetAllowDeliveryReportFlag( FS_BOOL bOK )
{
	GFS_MmsConfig.allow_delivery_report = bOK;
}

void FS_MmsConfigSetAllowReadReportFlag( FS_BOOL bOK )
{
	GFS_MmsConfig.allow_read_report = bOK;
}

void FS_MmsConfigSetMmsCenterUrl( FS_CHAR * url )
{
	if( url )
		IFS_Strncpy( GFS_MmsConfig.mms_center_url, url, sizeof(GFS_MmsConfig.mms_center_url) - 1 );
	else
		GFS_MmsConfig.mms_center_url[0] = 0;
}

FS_CHAR *FS_MmsConfigGetApn( void )
{
	return GFS_MmsConfig.apn;
}

void FS_MmsConfigSetApn( FS_CHAR * apn )
{
	if( apn )
		IFS_Strncpy( GFS_MmsConfig.apn, apn, sizeof(GFS_MmsConfig.apn) - 1 );
	else
		GFS_MmsConfig.apn[0] = 0;
}

FS_CHAR * FS_MmsConfigGetUserName( void )
{
	return GFS_MmsConfig.user_name;
}

void FS_MmsConfigSetUserName( FS_CHAR * user_name )
{
	if( user_name )
		IFS_Strncpy( GFS_MmsConfig.user_name, user_name, sizeof(GFS_MmsConfig.user_name) - 1 );
	else
		GFS_MmsConfig.user_name[0] = 0;
}

FS_CHAR * FS_MmsConfigGetPassword( void )
{
	return GFS_MmsConfig.password;
}

void FS_MmsConfigSetPassword( FS_CHAR * password )
{
	if( password )
		IFS_Strncpy( GFS_MmsConfig.password, password, sizeof(GFS_MmsConfig.password) - 1 );
	else
		GFS_MmsConfig.password[0] = 0;
}

void FS_MmsConfigInit( void )
{
	if( FS_FileRead( FS_DIR_MMS, FS_MMS_CFG_FILE, 0, &GFS_MmsConfig, sizeof(FS_MmsConfig) ) != sizeof(FS_MmsConfig) )
	{
		/* set to default */
		GFS_MmsConfig.protocol = FS_MMS_WSP;
		IFS_Strcpy( GFS_MmsConfig.proxy_addr, "10.0.0.172" );
		GFS_MmsConfig.proxy_port = 9201;
		IFS_Strcpy( GFS_MmsConfig.mms_center_url, "http://mmsc.monternet.com" );
		IFS_Strcpy( GFS_MmsConfig.apn, "CMWAP" );
		GFS_MmsConfig.auto_recv = FS_TRUE;
		GFS_MmsConfig.allow_delivery_report = FS_FALSE;
		GFS_MmsConfig.allow_read_report = FS_FALSE;
		GFS_MmsConfig.allow_ads = FS_FALSE;
		GFS_MmsConfig.save_send = FS_TRUE;
	}
}

void FS_MmsConfigSave( void )
{
	FS_FileWrite( FS_DIR_MMS, FS_MMS_CFG_FILE, 0, &GFS_MmsConfig, sizeof(FS_MmsConfig) );
}

#endif	//FS_MODULE_MMS


