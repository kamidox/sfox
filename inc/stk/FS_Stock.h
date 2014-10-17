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
	FS_SINT4	purchase_price;		/* ����۸񣬼۸� * 100����22.81�򱣴�Ϊ2281 */
	FS_SINT4	purchase_quantum;	/* ������� */
	
	FS_SINT4	closing_price;		/* ��λ: �� */
	FS_SINT4	opening_price;		/* ��λ: �� */
	FS_SINT4	highest_price;		/* ��λ: �� */
	FS_SINT4	lowest_price;		/* ��λ: �� */
	FS_SINT4	cur_price;			/* ��λ: �� */
	FS_SINT4	markup_val;			/* ��λ: �� */
	FS_SINT4	markup_rate;		/* ��λ: ���֮һ */
	FS_UINT4	total_quantum;		/* �ɽ�������λ�֣���100�� */
	FS_UINT4	total_money;		/* �ɽ�����λ��Ԫ */

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
	FS_SINT4	refresh_time;		/* ˢ��ʱ��Ƶ�ʣ���λΪ�� */
	
	FS_SINT4	stamp_tax;			/* ӡ��˰������ * 10000����1/1000���򱣴�Ϊ10 */
	FS_SINT4	commision;			/* Ӷ�𣬱��� * 10000����1.5/1000���򱣴�Ϊ15 */
	FS_SINT4	others_fee;			/* �������� */
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
