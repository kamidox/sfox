#ifndef _FS_STOCK_H_
#define _FS_STOCK_H_

#include "inc/FS_Config.h"
#include "inc/util/FS_List.h"
#include "inc/util/FS_File.h"

#define FS_STOCK_OK					0
#define FS_STOCK_ERR_OK				0
#define FS_STOCK_ERR_MEMORY			-1
#define FS_STOCK_ERR_SERV_BUSY		-2
#define FS_STOCK_ERR_NET_BUSY		-3
#define FS_STK_ERR_CONN				-4
#define FS_STOCK_ERR_FILE			-5
#define FS_STOCK_ERR_HTTP			-6
#define FS_STOCK_ERR_HTTP_BUSY		-7
#define FS_STOCK_ERR_UNKNOW			-8
#define FS_STOCK_ERR_EMPTY			-9

#define FS_STOCK_MAX_COL_NUM		240
#define FS_STOCK_MAX_STOCK_NUM		20
#define FS_STOCK_NAME_LEN			16
#define FS_STOCK_ID_LEN				12

typedef struct FS_Stock_Tag
{
	FS_List		list;
	
	FS_CHAR		id[FS_STOCK_ID_LEN];
	FS_CHAR 	name[FS_STOCK_NAME_LEN];
	FS_SINT4	purchase_price;		/* 买入价格，价格 * 100，如22.81则保存为2281 */
	FS_SINT4	purchase_quantum;	/* 买入股数 */
	
	FS_SINT4	closing_price;		/* 单位: 分 */
	FS_SINT4	opening_price;		/* 单位: 分 */
	FS_SINT4	highest_price;		/* 单位: 分 */
	FS_SINT4	lowest_price;		/* 单位: 分 */
	FS_SINT4	cur_price;			/* 单位: 分 */
	FS_SINT4	markup_val;			/* 单位: 分 */
	FS_SINT4	markup_rate;		/* 单位: 万分之一 */
	FS_UINT4	total_quantum;		/* 成交量，单位手，即100股 */
	FS_UINT4	total_money;		/* 成交金额，单位万元 */

	FS_CHAR 	realtime[FS_FILE_NAME_LEN];
}FS_Stock;

typedef struct FS_StockConfig_Tag
{
	FS_CHAR 	apn[FS_URL_LEN];
	FS_CHAR		user[FS_URL_LEN];
	FS_CHAR		pass[FS_URL_LEN];
	FS_BOOL		use_proxy;
	FS_CHAR		proxy_addr[FS_URL_LEN];
	FS_SINT4	proxy_port;
	FS_SINT4	refresh_time;		/* 刷新时间频率，单位为秒 */
	
	FS_SINT4	stamp_tax;			/* 印花税，比例 * 10000，如1/1000，则保存为10 */
	FS_SINT4	commision;			/* 佣金，比例 * 10000，如1.5/1000，则保存为15 */
	FS_SINT4	others_fee;			/* 其它费用 */
}FS_StockConfig;

typedef struct FS_StockExponent_Tag
{
	FS_CHAR *		id;
	FS_CHAR *		name;
}FS_StockExponent;

FS_StockExponent *FS_StockGetExponentList( void );

FS_Stock *FS_StockFindById( FS_CHAR *id );

FS_SINT4 FS_StockUpdate( void );

FS_SINT4 FS_StockInit( void );

void FS_StockDeinit( void );

FS_List *FS_StockGetList( void );

FS_SINT4 FS_StockGetListCount( void );

FS_StockConfig *FS_StockGetConfig( void );

void FS_StockSetConfig( FS_StockConfig *config );

void FS_StockAdd( FS_CHAR *stkId );

void FS_StockDelete( FS_Stock *stock );

void FS_StockMoveTop( FS_Stock *stock );

FS_SINT4 FS_StockRealTimeOn( FS_CHAR *id );

void FS_StockRealTimeOff( void );

FS_SINT4 FS_StockDetailOn( FS_CHAR *id );

void FS_StockDetailOff( void );

#endif
