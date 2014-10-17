#ifndef _FS_DCD_LIB_H_
#define _FS_DCD_LIB_H_
#include "inc/FS_Config.h"
#include "inc/util/FS_List.h"
#include "inc/dcd/FS_DcdPkg.h"

#define FS_DCD_OK					0
#define FS_DCD_ERR_OK				FS_DCD_OK
#define FS_DCD_ERR_MEMORY			-1
#define FS_DCD_ERR_NET_READY		-2
#define FS_DCD_ERR_CONN				-3
#define FS_DCD_ERR_HTTP_BUSY		-4
#define FS_DCD_ERR_HTTP				-5
#define FS_DCD_ERR_FILE				-6
#define FS_DCD_ERR_UNKNOW			-7
#define FS_DCD_ERR_FULL				-8

typedef enum 
{
	FS_DCD_IDLE_SLW = 0,
	FS_DCD_IDLE_MID,
	FS_DCD_IDLE_QCK,
	
	FS_DCD_IDLE_SPEED_CNT
}FS_DcdIdleSpeed;

typedef enum
{
	FS_DCD_NET_ALWAYS_ON = 0,
	FS_DCD_NET_ALWAYS_OFF,
	FS_DCD_NET_ROAMING_OFF,
	
	FS_DCD_NET_MODE_CNT
}FS_DcdNetMode;

typedef struct FS_DcdConfig_Tag
{
	FS_BYTE			on;
	FS_BYTE			net_mode;
	FS_BYTE			idle_display;
	FS_BYTE			idle_speed;
}FS_DcdConfig;

/* DCD«Î«Û¿‡–Õ */
typedef enum{
	FS_DCD_REQ_TTL = 0,
	FS_DCD_REQ_MAN,
	FS_DCD_REQ_SVR,
	FS_DCD_REQ_MC,
	FS_DCD_REQ_MR,
	FS_DCD_REQ_SIM,
	
	FS_DCD_REQ_DATA,
	FS_DCD_REQ_DATA2,
	FS_DCD_REQ_DATA3,
	
	FS_DCD_REQ_MAX
}FS_DCD_REQ_TYPE;

FS_SINT4 FS_DcdInit( void );

void FS_DcdDeinit( void );

FS_SINT4 FS_DcdUpdate( FS_DCD_REQ_TYPE type);

FS_List *FS_DcdGetChannelList( void );

FS_DcdEntry *FS_DcdGetIdleEntry( void );

FS_DcdChannelFeed *FS_DcdGetIdleChannel( void );

FS_SINT4 FS_DcdGetChannelCount( void );

FS_SINT4 FS_DcdGetIdleChannelIndex( void );

void FS_DcdResumeIdleTimer( void );

void FS_DcdPauseIdleTimer( void );

void FS_DcdSaveChannelList( void );

FS_SINT4 FS_DcdSnapshotAddEntry( FS_DcdEntry *entry );

FS_SINT4 FS_DcdSnapshotDelEntry( FS_DcdEntry *entry );

FS_SINT4 FS_DcdSnaphotDelAll( void );

FS_List *FS_DcdGetSnapshotList( void );

FS_DcdConfig FS_DcdGetConfig( void );

void FS_DcdSaveConfig( FS_DcdConfig cfg );

#endif

