#ifndef _FS_MMS_CONFIG_H_
#define _FS_MMS_CONFIG_H_

#include "inc/FS_Config.h"
/* protocol */
#define FS_MMS_WSP			0
#define FS_MMS_HTTP 		1

/*---------------------------- getters -----------------------------------*/
FS_SINT4 FS_MmsConfigGetProtocol( void );

FS_CHAR *FS_MmsConfigGetProxyAddr( void );

FS_UINT2 FS_MmsConfigGetProxyPort( void );

FS_CHAR *FS_MmsConfigGetMmsCenterUrl( void );

FS_BOOL FS_MmsConfigGetAutoRecvFlag( void );

FS_BOOL FS_MmsConfigGetAllowAdsFlag( void );

FS_BOOL FS_MmsConfigGetAllowDeliveryReportFlag( void );

FS_BOOL FS_MmsConfigGetAllowReadReportFlag( void );

FS_BOOL FS_MmsConfigGetSaveSendFlag( void );

FS_CHAR *FS_MmsConfigGetApn( void );

FS_CHAR * FS_MmsConfigGetUserName( void );

FS_CHAR * FS_MmsConfigGetPassword( void );

/*---------------------------- setters -----------------------------------*/

void FS_MmsConfigSetProtocol( FS_UINT1 protocol );

void FS_MmsConfigSetProxyAddr( FS_CHAR *proxy );

void FS_MmsConfigSetProxyPort( FS_UINT2 port );

void FS_MmsConfigSetAutoRecvFlag( FS_BOOL bOK );

void FS_MmsConfigSetAllowAdsFlag( FS_BOOL bOK );

void FS_MmsConfigSetAllowDeliveryReportFlag( FS_BOOL bOK );

void FS_MmsConfigSetAllowReadReportFlag( FS_BOOL bOK );

void FS_MmsConfigSetSaveSendFlag( FS_BOOL value );

void FS_MmsConfigSetMmsCenterUrl( FS_CHAR * url );

void FS_MmsConfigSave( void );

void FS_MmsConfigInit( void );

void FS_MmsConfigSetApn( FS_CHAR * apn );

void FS_MmsConfigSetUserName( FS_CHAR * user_name );

void FS_MmsConfigSetPassword( FS_CHAR * password );

#endif

