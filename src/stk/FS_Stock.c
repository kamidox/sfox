#include "inc/FS_Config.h"

#ifdef FS_MODULE_STK
#include "inc/stk/FS_Stock.h"
#include "inc/res/FS_TimerID.h"
#include "inc/web/FS_Http.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_File.h"
#include "inc/util/FS_NetConn.h"
#include "inc/util/FS_MemDebug.h"

#define _STK_DEBUG

#ifdef _STK_DEBUG
#define FS_STK_TRACE0(a)				FS_TRACE0( "[STK]" a "\r\n")
#define FS_STK_TRACE1(a,b)				FS_TRACE1( "[STK]" a "\r\n", b)
#define FS_STK_TRACE2(a,b,c)			FS_TRACE2( "[STK]" a "\r\n", b, c)
#define FS_STK_TRACE3(a,b,c,d)			FS_TRACE3( "[STK]" a "\r\n", b, c, d)
#else
#define FS_STK_TRACE0(a)
#define FS_STK_TRACE1(a,b)
#define FS_STK_TRACE2(a,b,c)
#define FS_STK_TRACE3(a,b,c,d)
#endif

#define FS_STK_NET_GATWAY		"10.0.0.172"
#define FS_STK_NET_HOST			"http://hq.sinajs.cn/list="
#define FS_STK_RT_HOST			"http://image.sinajs.cn/newchart/small/b"
#define FS_STK_RT_IMG_FMT		".gif"

#define FS_STOCK_LIST_FILE		"stocks.bin"
#define FS_STK_CFG_FILE			"stk_cfg.bin"
#define FS_STK_UPDATE_FILE		"stk_update.txt"
#define FS_STK_DETAIL_FILE		"stk_detail.txt"

#define FS_STK_REFRESH_TIME			10		/* s */
#define FS_STK_REALTIME_REF_TIME		600		/* 600s for realtime auto update */

#define FS_STK_STATE_IDLE			0	/* 空闲 */
#define FS_STK_STATE_UPDATE			1	/* 更新 */
#define FS_STK_STATE_REALTIME		2	/* 获取实时走势 */
#define FS_STK_STATE_DETAIL			3	/* 获取个股详情 */

typedef struct FS_StockSession_tag
{
	FS_SINT4				state;
	FS_SINT4				orig_state;	/* backup state. for auto update */
	
	FS_HttpHandle			hHttp;
	FS_List					stock_list;
	FS_StockConfig			config;
	FS_UINT4				timer_id;
	
	FS_CHAR					file[FS_FILE_NAME_LEN];
	FS_SINT4				content_len;
	FS_SINT4				offset;

	FS_CHAR					realtime_id[FS_STOCK_ID_LEN];
}FS_StockSession;

static FS_StockSession *GFS_StockSes;

static FS_StockExponent GFS_StockExps[] =
{
	{ "sh000001", "\xE4\xB8\x8A\xE8\xAF\x81\xE7\xBB\xBC\xE6\x8C\x87" },		/* 上证综指 */
	{ "sz399001", "\xE6\xB7\xB1\xE8\xAF\x81\xE6\x88\x90\xE6\x8C\x87" },		/* 深证成指 */
	{ "sh000300", "\xE6\xB2\xAA\xE6\xB7\xB1\x33\x30\x30" },			/* 沪深300 */
	{ "sh000017", "\xE6\x96\xB0\xE7\xBB\xBC\xE6\x8C\x87" },			/* 新综指 */

	/* last item */
	{ FS_NULL, FS_NULL }
};

void FS_StkUIListUpdate( void );
void FS_StkUIRealtimeUpdate( void );
void FS_StkUIDetailUpdate( void );
void FS_StkUIUpdateFailed( void );

static void FS_StkTimerCallback( FS_StockSession *stkSes );

static FS_SINT4 FS_StkReadFloatValue( FS_CHAR *str )
{
	FS_CHAR *p, *p2;
	FS_SINT4 price, i;
	FS_SINT4 minus = 1;
	
	if( *str == '-' ){
		str ++;
		minus = -1;
	}
	price = IFS_Atoi( str ) * 100;
	p = IFS_Strchr( str, '.' );
	if( p == FS_NULL ){
		return price;
	}
	p = p + 1;
	p2 = IFS_Strchr( str, ',' );
	/* 在这种情况下，说明没有小数点 */
	if( p2 != FS_NULL && p > p2 ){
		return price;
	}
	/* 此时，p指向小数点第一位。计算小数点后面两位 */
	i = 0;
	if(p[0] >= '0' && p[0] <= '9'){
		i += (p[0] - '0') * 10;
	}
	if(p[1] >= '0' && p[1] <= '9'){
		i += p[1] - '0';
	}
	/* 四舍五入 */
	if(p[2] >= '0' && p[2] <= '9'){
		if( p[2] >= '5' ) i ++;
	}
	return minus * (price + i);
}

static void FS_StkWriteStockList( FS_StockSession *stkSes )
{
	FS_SINT4 cnt, len;
	FS_BYTE *buf, *ptr;
	FS_List *node;
	FS_Stock *stock;
	
	cnt = FS_ListCount( &stkSes->stock_list );
	len = cnt * sizeof(FS_Stock) + 4;
	buf = IFS_Malloc( len );
	FS_ASSERT( buf != FS_NULL );
	if( buf == FS_NULL ) return;
	IFS_Memset( buf, 0, len );
	ptr = buf;
	/* write stock count */
	FS_UINT4_TO_LE_BYTE( cnt, ptr );
	/* write stock data */
	ptr += 4;
	cnt = sizeof( FS_Stock );
	FS_ListForEach( node, &stkSes->stock_list ){
		stock = FS_ListEntry( node, FS_Stock, list );
		IFS_Memcpy( ptr, stock, cnt );
		ptr += cnt;
	}
	FS_FileWrite( FS_DIR_STK, FS_STOCK_LIST_FILE, 0, buf, len );
	IFS_Free( buf );
}

static void FS_StkReadStockList( FS_StockSession *stkSes )
{
	FS_SINT4 len;
	FS_BYTE *buf, *ptr;
	FS_UINT4 cnt, i;
	FS_Stock *stock;
	
	len = FS_FileGetSize( FS_DIR_STK, FS_STOCK_LIST_FILE );
	if( len <= 0 ) return;
	buf = IFS_Malloc( len );
	FS_ASSERT( buf != FS_NULL );
	if( buf == FS_NULL ) return;
	FS_FileRead( FS_DIR_STK, FS_STOCK_LIST_FILE, 0, buf, len );

	cnt = FS_LE_BYTE_TO_UINT4( buf );
	ptr = buf + 4;
	len = sizeof(FS_Stock);
	for( i = 0; i < cnt; i ++ ){
		stock = IFS_Malloc( len );
		FS_ASSERT( stock != FS_NULL );
		IFS_Memcpy( stock, ptr, len );
		ptr += len;

		FS_ListAddTail( &stkSes->stock_list, &stock->list );
	}
	
	IFS_Free( buf );
}

static FS_BOOL FS_StkIsExponent( FS_CHAR *id )
{
	FS_StockExponent *exp;

	exp = GFS_StockExps;
	while( exp->id != FS_NULL ){
		if( FS_STR_I_EQUAL(id, exp->id) ){
			return FS_TRUE;
		}
		exp ++;
	}
	return FS_FALSE;
}

static void FS_StkParseSimple( FS_StockSession *stkSes )
{
	FS_CHAR *buf, *ptr, *ptmp, *pend, *pname, id[9];
	FS_SINT4 len;
	FS_Stock *stock;
	
	len = FS_FileGetSize( FS_DIR_STK, stkSes->file );
	buf = IFS_Malloc( len + 1 );
	FS_ASSERT( buf != FS_NULL );
	if( buf == FS_NULL ) return;
	IFS_Memset( buf, 0, len + 1 );
	if( len != FS_FileRead( FS_DIR_STK, stkSes->file, 0, buf, len ) ){
		goto STK_DONE;
	}

	ptr = buf;
	while( ptr != FS_NULL && ptr <= buf + len ){
		/* prefix */
		ptr = IFS_Strstr( ptr, "var hq_str_s_" );
		if( ptr == FS_NULL ){
			goto STK_DONE;
		}
		ptr += 13;
		pend = IFS_Strchr( ptr, ';' );
		
		/* stock id */
		IFS_Memset( id, 0, sizeof(id) );
		IFS_Strncpy( id, ptr, 8 );
		ptr += 8;
		/* find stock id */
		stock = FS_StockFindById( id );
		if( stock == FS_NULL ){
			continue;
		}
		if( ! FS_STR_NI_EQUAL( ptr, "=\"", 2) ){
			goto STK_DONE;
		}
		ptr += 2;
		/* stock name */
		ptmp = IFS_Strchr( ptr, ',' );
		if( ptmp == FS_NULL || ptmp > pend || ((FS_UINT4)(ptmp - ptr)) > sizeof(stock->name) ){
			continue;
		}
		if( stock->name[0] == 0 ){
			pname = FS_ProcessCharset( ptr, ptmp - ptr, "GB2312", FS_NULL );
			if( pname ){
				IFS_Strncpy( stock->name, pname, sizeof(stock->name) - 1 );
				IFS_Free( pname );
			}else{
				IFS_Strncpy( stock->name, ptr, ptmp - ptr );
			}
		}
		ptr = ptmp + 1;
		/* cur price */
		stock->cur_price = FS_StkReadFloatValue( ptr );
		ptmp = IFS_Strchr( ptr, ',' );
		if( ptmp == FS_NULL || ptmp > pend ){
			continue;
		}
		ptr = ptmp + 1;
		/* markup value */
		stock->markup_val = FS_StkReadFloatValue( ptr );
		ptmp = IFS_Strchr( ptr, ',' );
		if( ptmp == FS_NULL || ptmp > pend ){
			continue;
		}
		ptr = ptmp + 1;
		/* markup rate */
		stock->markup_rate = FS_StkReadFloatValue( ptr );
		ptmp = IFS_Strchr( ptr, ',' );
		if( ptmp == FS_NULL || ptmp > pend ){
			continue;
		}
		ptr = ptmp + 1;
		/* total quantum. simple data里，单位已经为手 */
		stock->total_quantum = (FS_UINT4)IFS_Atoi( ptr );
		ptmp = IFS_Strchr( ptr, ',' );
		if( ptmp == FS_NULL || ptmp > pend ){
			continue;
		}
		ptr = ptmp + 1;
		/* total money. simple data里，单位已经为万元 */
		stock->total_money = (FS_UINT4)IFS_Atoi( ptr );
	}
	
STK_DONE:
	IFS_Free( buf );
	return;
}

static void FS_StkParseDetail( FS_StockSession *stkSes )
{
	FS_CHAR *buf, *ptr, *ptmp, *ptmp2, *pend, id[9];
	FS_SINT4 len;
	FS_Stock *stock;
	
	len = FS_FileGetSize( FS_DIR_STK, stkSes->file );
	buf = IFS_Malloc( len + 1 );
	FS_ASSERT( buf != FS_NULL );
	if( buf == FS_NULL ) return;
	IFS_Memset( buf, 0, len + 1 );
	if( len != FS_FileRead( FS_DIR_STK, stkSes->file, 0, buf, len ) ){
		goto STK_DONE;
	}

	ptr = buf;
	while( ptr != FS_NULL && ptr <= buf + len ){
		/* prefix */
		ptr = IFS_Strstr( ptr, "var hq_str_" );
		if( ptr == FS_NULL ){
			goto STK_DONE;
		}
		ptr += 11;
		pend = IFS_Strchr( ptr, ';' );
		
		/* stock id */
		IFS_Memset( id, 0, sizeof(id) );
		IFS_Strncpy( id, ptr, 8 );
		ptr += 8;
		/* find stock id */
		stock = FS_StockFindById( id );
		if( stock == FS_NULL ){
			continue;
		}
		if( ! FS_STR_NI_EQUAL( ptr, "=\"", 2) ){
			goto STK_DONE;
		}
		ptr += 2;
		/* stock name */
		ptmp = IFS_Strchr( ptr, ',' );
		if( ptmp == FS_NULL || ptmp > pend || ((FS_UINT4)(ptmp - ptr)) > sizeof(stock->name) ){
			continue;
		}
		if( stock->name[0] == 0 ){
			IFS_Strncpy( stock->name, ptr, ptmp - ptr );
		}
		ptr = ptmp + 1;
		/* opening price */
		stock->opening_price = FS_StkReadFloatValue( ptr );
		ptmp = IFS_Strchr( ptr, ',' );
		if( ptmp == FS_NULL || ptmp > pend ){
			continue;
		}
		ptr = ptmp + 1;
		/* closing price */
		stock->closing_price = FS_StkReadFloatValue( ptr );
		ptmp = IFS_Strchr( ptr, ',' );
		if( ptmp == FS_NULL || ptmp > pend ){
			continue;
		}
		ptr = ptmp + 1;
		/* cur price */
		stock->cur_price = FS_StkReadFloatValue( ptr );
		ptmp = IFS_Strchr( ptr, ',' );
		if( ptmp == FS_NULL || ptmp > pend ){
			continue;
		}
		ptr = ptmp + 1;
		/* highest price */
		stock->highest_price = FS_StkReadFloatValue( ptr );
		ptmp = IFS_Strchr( ptr, ',' );
		if( ptmp == FS_NULL || ptmp > pend ){
			continue;
		}
		ptr = ptmp + 1;
		/* lowest price */
		stock->lowest_price = FS_StkReadFloatValue( ptr );
		ptmp = IFS_Strchr( ptr, ',' );
		if( ptmp == FS_NULL || ptmp > pend ){
			continue;
		}
		ptr = ptmp + 1;
		/* skip two price */		
		ptmp = IFS_Strchr( ptr, ',' );
		if( ptmp == FS_NULL || ptmp > pend ){
			continue;
		}
		ptr = ptmp + 1;
		ptmp = IFS_Strchr( ptr, ',' );
		if( ptmp == FS_NULL || ptmp > pend ){
			continue;
		}
		ptr = ptmp + 1;
		/* total quantum */
		ptmp = IFS_Strchr( ptr, ',' );
		ptmp2 = IFS_Strchr( ptr, '.' );
		if( ptmp2 ) ptmp = FS_MIN( ptmp, ptmp2 );
		if( ptmp == FS_NULL || ptmp > pend ){
			continue;
		}
		/* 数据源里，指数的单位为手，个股的单位为股，这里统一转化为手 */
		if( ! FS_StkIsExponent(id) ){
			*(ptmp - 2) = '.';
		}
		stock->total_quantum = (FS_UINT4)IFS_Atoi( ptr );
		ptr = ptmp + 1;
		/* total money */
		ptmp = IFS_Strchr( ptr, ',' );
		ptmp2 = IFS_Strchr( ptr, '.' );
		if( ptmp2 ) ptmp = FS_MIN( ptmp, ptmp2 );
		if( ptmp == FS_NULL || ptmp > pend ){
			continue;
		}
		*(ptmp - 4) = '.';	/* 单位转化为万元 */
		stock->total_money = (FS_UINT4)IFS_Atoi( ptr );

		/* 计算涨跌额和涨跌幅 */
		if( stock->cur_price > 0 )
			stock->markup_val = stock->cur_price - stock->closing_price;
		else
			stock->markup_val = 0;
		if( stock->closing_price > 0 ){
			stock->markup_rate = stock->markup_val * 10000 / stock->closing_price;
		}else{
			stock->markup_rate = 0;
		}
	}
	
STK_DONE:
	IFS_Free( buf );
	return;
}

static void FS_StkResetNetwork( FS_StockSession *stkSes )
{
	FS_SINT4 msec;
	
	stkSes->orig_state = stkSes->state;
	stkSes->state = FS_STK_STATE_IDLE;
	if( stkSes->file[0] ){
		IFS_Memset( stkSes->file, 0, sizeof(stkSes->file) );
	}
	stkSes->offset = 0;
	stkSes->content_len = 0;
	if( stkSes->hHttp ){
		FS_HttpRequestCancel( stkSes->hHttp, FS_FALSE );
		FS_HttpDestroyHandle( stkSes->hHttp );
		stkSes->hHttp = FS_NULL;		
	}
	if(stkSes->config.refresh_time > 0 ){
		if( stkSes->orig_state == FS_STK_STATE_REALTIME ){
			msec = FS_STK_REALTIME_REF_TIME * 1000;
		}else{
			msec = stkSes->config.refresh_time * 1000;
		}
		
		stkSes->timer_id = IFS_StartTimer( FS_TIMER_ID_STOCK, msec, FS_StkTimerCallback, stkSes );
	}
}

static void FS_StkUpdateResult( FS_StockSession *stkSes, FS_SINT4 err_code )
{
	if( err_code == FS_STOCK_ERR_OK ){
		if( stkSes->state == FS_STK_STATE_UPDATE ){
			FS_StkParseSimple( stkSes );
			FS_StkWriteStockList( stkSes );
			/* inform stock ui to update new data */
			FS_StkUIListUpdate( );
		}else if( stkSes->state == FS_STK_STATE_REALTIME ){
			FS_StkUIRealtimeUpdate( );
		}else if( stkSes->state == FS_STK_STATE_DETAIL ){
			FS_StkParseDetail( stkSes );
			/* inform stock ui to update detail */
			FS_StkUIDetailUpdate( );
		}
	}else{
		FS_StkUIUpdateFailed( );
	}
	FS_StkResetNetwork( stkSes );
}

static void FS_StkHttpResponseStart( FS_StockSession *stkSes, FS_HttpHandle hHttp, FS_SINT4 status, FS_HttpHeader *headers )
{	
	FS_Stock *stock;
	
	if( stkSes->state == FS_STK_STATE_IDLE )
	{
		FS_STK_TRACE0( "FS_StkHttpResponseStart state error." );
		return;
	}
	if( status != FS_HTTP_OK ){
		FS_STK_TRACE1( "FS_StkHttpResponseStart status = %d", status );
		FS_StkUpdateResult( stkSes, FS_STOCK_ERR_HTTP );
		return;
	}
	FS_STK_TRACE2( "FS_StkHttpResponseStart content_type = %s, content_len = %d", headers->content_type, headers->content_length );

	/* 生成临时的媒体文件名 */
	if( stkSes->state == FS_STK_STATE_UPDATE ){
		IFS_Strcpy( stkSes->file, FS_STK_UPDATE_FILE );
	}else if( stkSes->state == FS_STK_STATE_DETAIL ){
		IFS_Strcpy( stkSes->file, FS_STK_DETAIL_FILE );
	}else if( stkSes->state == FS_STK_STATE_REALTIME ){
		if( ! FS_STR_NI_EQUAL(headers->content_type, "image", 5) ){
			FS_StkUpdateResult( stkSes, FS_STOCK_ERR_HTTP );
			return;
		}
		
		stock = FS_StockFindById( stkSes->realtime_id );
		if( stock == FS_NULL ){
			FS_StkUpdateResult( stkSes, FS_STOCK_ERR_UNKNOW );
			return;
		}
		if( stock->realtime[0] == 0 ){
			IFS_Strcpy( stock->realtime, stock->id );
			IFS_Strcat( stock->realtime, FS_STK_RT_IMG_FMT );
		}
		IFS_Strcpy( stkSes->file, stock->realtime );
	}
	stkSes->content_len = headers->content_length;
	stkSes->offset = 0;
}

static void FS_StkHttpResponseData( FS_StockSession *stkSes, FS_HttpHandle hHttp, FS_BYTE *rdata, FS_SINT4 rdata_len )
{
	if( stkSes->state == FS_STK_STATE_IDLE )
	{
		FS_STK_TRACE0( "FS_StkHttpResponseData state error." );
		return;
	}
	
	FS_STK_TRACE3( "FS_StkHttpResponseData len = %d, offset = %d, rlen = %d", stkSes->content_len,  stkSes->offset, rdata_len );
	if( rdata_len == FS_FileWrite( FS_DIR_STK, stkSes->file, stkSes->offset, rdata, rdata_len ) )
	{
		stkSes->offset += rdata_len;
	}
	else
	{
		FS_STK_TRACE0( "FS_StkHttpResponseData FS_FileWrite error." );
		FS_StkUpdateResult( stkSes, FS_STOCK_ERR_FILE );
	}
}

static void FS_StkHttpResponseEnd( FS_StockSession *stkSes, FS_HttpHandle hHttp, FS_SINT4 error_code )
{
	FS_STK_TRACE0( "FS_StkHttpResponseEnd" );
	if( error_code == FS_HTTP_ERR_OK ){
		FS_StkUpdateResult( stkSes, FS_STOCK_ERR_OK );
	} else {
		FS_StkUpdateResult( stkSes, FS_STOCK_ERR_HTTP );
	}
}

static FS_BOOL FS_StkNeedUpdate( FS_StockSession *stkSes )
{
	FS_DateTime dt;
	FS_SINT4 offset;

	return FS_TRUE;
	
	IFS_GetDateTime( &dt );
	if( dt.week_day >= 1 && dt.week_day <= 5 ){
		/* minute offset from 9:30 */
		offset = (dt.hour - 9) * 60 + dt.min - 30;	
		/* 9:20 - 11:35, 12:55 - 13:05 */
		if( (offset >= -10 && offset <= 125) || (offset >= 205 && offset <= 335) )
			return FS_TRUE;
	}
	
	return FS_FALSE;
}

static void FS_StkTimerCallback( FS_StockSession *stkSes )
{
	FS_SINT4 msec;
	
	FS_ASSERT( stkSes == GFS_StockSes );
	if( GFS_StockSes == FS_NULL ) return;
	stkSes->timer_id = 0;

	if( stkSes->state == FS_STK_STATE_IDLE && FS_StkNeedUpdate( stkSes ) ){
		FS_StockUpdate( );
	}else{
		if( stkSes->config.refresh_time > 0 ){
			if( stkSes->orig_state == FS_STK_STATE_REALTIME ){
				msec = FS_STK_REALTIME_REF_TIME * 1000;
			}else{
				msec = stkSes->config.refresh_time * 1000;
			}
			
			stkSes->timer_id = IFS_StartTimer( FS_TIMER_ID_STOCK, msec, FS_StkTimerCallback, stkSes );
		}
	}
}

static FS_CHAR *FS_StkFormatRealtimeUrl( FS_StockSession *stkSes )
{
	FS_CHAR *url;
	url = FS_StrConCat( FS_STK_RT_HOST, stkSes->realtime_id, FS_STK_RT_IMG_FMT, FS_NULL );
	return url;
}

static FS_CHAR *FS_StkFormatDetailUrl( FS_StockSession *stkSes )
{
	FS_CHAR *url;
	url = FS_StrConCat( FS_STK_NET_HOST, stkSes->realtime_id, FS_NULL, FS_NULL );
	return url;
}

static FS_CHAR *FS_StkFormatUpdateUrl( FS_StockSession *stkSes )
{
	FS_CHAR *url;
	FS_List *node;
	FS_Stock *stock;
	FS_SINT4 len;
	
	url = IFS_Malloc( FS_URL_LEN + FS_STOCK_MAX_STOCK_NUM * 16 );
	FS_ASSERT( url != FS_NULL );
	IFS_Strcpy( url, FS_STK_NET_HOST );

	node = stkSes->stock_list.next;
	while( node != &stkSes->stock_list ){
		stock = FS_ListEntry( node, FS_Stock, list );
		node = node->next;
		
		IFS_Strcat( url, "s_" );
		IFS_Strcat( url, stock->id );
		IFS_Strcat( url, "," );
	}
	len = IFS_Strlen( url );
	FS_ASSERT( url[len - 1] == ',' );
	url[len - 1] = 0;	/* delete trail ',' */
	return url;
}

static void FS_StkNetConnCallback( FS_StockSession *stkSes, FS_BOOL bOK )
{
	FS_CHAR *url = FS_NULL;
	FS_SockAddr addr;

	FS_STK_TRACE1( "FS_StkNetConnCallback ok = %d", bOK );
	if( !bOK ){
		FS_StkUpdateResult( stkSes, FS_STK_ERR_CONN );
		return;
	}
	stkSes->hHttp = FS_HttpCreateHandle( stkSes, FS_StkHttpResponseStart, FS_StkHttpResponseData, FS_StkHttpResponseEnd );
	if( stkSes->hHttp == FS_NULL ){
		FS_StkUpdateResult( stkSes, FS_STOCK_ERR_HTTP_BUSY );
		return;
	}

	/* format header */
	if( stkSes->state == FS_STK_STATE_UPDATE ){
		url = FS_StkFormatUpdateUrl( stkSes );
	}else if( stkSes->state == FS_STK_STATE_REALTIME ){
		url = FS_StkFormatRealtimeUrl( stkSes );
	}else if( stkSes->state == FS_STK_STATE_DETAIL ){
		url = FS_StkFormatDetailUrl( stkSes );
	}
	
	if( url == FS_NULL ){
		FS_StkUpdateResult( stkSes, FS_STOCK_ERR_UNKNOW );
		return;
	}
	/* send request */
	if( stkSes->config.use_proxy ){
		addr.host = stkSes->config.proxy_addr;
		addr.port = stkSes->config.proxy_port;
		FS_HttpRequest( stkSes->hHttp, &addr, "GET", url, FS_NULL, FS_NULL );
	}else{
		FS_HttpRequest( stkSes->hHttp, FS_NULL, "GET", url, FS_NULL, FS_NULL );
	}
	FS_STK_TRACE1( "FS_StkNetConnCallback url = %s", url );
	IFS_Free( url );
}

static void FS_StkConfigInit( FS_StockSession *stkSes )
{
	if( sizeof(FS_StockConfig) != FS_FileRead(FS_DIR_STK, FS_STK_CFG_FILE, 0, &stkSes->config, sizeof(FS_StockConfig)) ){
		IFS_Strcpy( stkSes->config.apn, "CMWAP" );
		IFS_Strcpy( stkSes->config.proxy_addr, "10.0.0.172" );
		stkSes->config.proxy_port = 80;
		stkSes->config.use_proxy = FS_TRUE;
		stkSes->config.refresh_time = FS_STK_REFRESH_TIME;
	}
}

FS_Stock *FS_StockFindById( FS_CHAR *id )
{
	FS_Stock *stock;
	FS_List *node;
	FS_StockSession *stkSes = GFS_StockSes;

	FS_ASSERT( stkSes != FS_NULL );
	if( stkSes == FS_NULL ) return FS_NULL;

	FS_ListForEach( node, &stkSes->stock_list ){
		stock = FS_ListEntry( node, FS_Stock, list );
		if( FS_STR_I_EQUAL(stock->id, id) ){
			return stock;
		}
	}
	return FS_NULL;
}

FS_SINT4 FS_StockUpdate( void )
{
	FS_StockSession *stkSes = GFS_StockSes;
	FS_SINT4 cnt;
	
	FS_ASSERT( stkSes != FS_NULL );

	cnt = FS_StockGetListCount( );
	if( cnt <= 0 ){
		return FS_STOCK_ERR_EMPTY;
	}
	if( stkSes->state != FS_STK_STATE_IDLE ){
		return FS_STOCK_ERR_SERV_BUSY;
	}
	if( stkSes->timer_id ){
		IFS_StopTimer( stkSes->timer_id );
		stkSes->timer_id = 0;
	}
	
	stkSes->state = stkSes->orig_state;
	FS_NetConnect( stkSes->config.apn, stkSes->config.user, stkSes->config.pass, 
		FS_StkNetConnCallback, FS_APP_STK, FS_FALSE, stkSes );
	return FS_STOCK_OK;
}

FS_SINT4 FS_StockInit( void )
{
	FS_StockSession *stkSes;
	
	if( GFS_StockSes != FS_NULL )
		return FS_STOCK_OK;
	stkSes = IFS_Malloc( sizeof(FS_StockSession) );
	if( stkSes == FS_NULL )
		return FS_STOCK_ERR_MEMORY;
	IFS_Memset( stkSes, 0, sizeof(FS_StockSession) );
	stkSes->state = FS_STK_STATE_IDLE;
	stkSes->orig_state = FS_STK_STATE_UPDATE;
	FS_ListInit( &stkSes->stock_list );
	FS_StkReadStockList( stkSes );
	FS_StkConfigInit( stkSes );
	stkSes->timer_id = IFS_StartTimer( FS_TIMER_ID_STOCK, FS_STK_REFRESH_TIME * 1000, FS_StkTimerCallback, stkSes );
	GFS_StockSes = stkSes;
	return FS_STOCK_OK;
}

void FS_StockDeinit( void )
{
	FS_StockSession *stkSes = GFS_StockSes;
	FS_Stock *stock;
	FS_List *node;
	
	if( stkSes == FS_NULL ) return;
	GFS_StockSes = FS_NULL;
	if( stkSes->timer_id ){
		IFS_StopTimer( stkSes->timer_id );
		stkSes->timer_id = 0;
	}
	if( stkSes->file[0] ){
		IFS_Memset( stkSes->file, 0, sizeof(stkSes->file) );
	}
	if( stkSes->hHttp ){
		FS_HttpRequestCancel( stkSes->hHttp, FS_FALSE );
		FS_HttpDestroyHandle( stkSes->hHttp );
		stkSes->hHttp = FS_NULL;		
	}
	FS_NetDisconnect( FS_APP_STK );

	node = stkSes->stock_list.next;
	while( node != &stkSes->stock_list ){
		stock = FS_ListEntry( node, FS_Stock, list );
		node = node->next;
		
		FS_ListDel( &stock->list );
		IFS_Free( stock );
	}

	IFS_Free( stkSes );
}

FS_List *FS_StockGetList( void )
{
	FS_ASSERT( GFS_StockSes != FS_NULL );
	if( GFS_StockSes == FS_NULL ) return FS_NULL;
	return &GFS_StockSes->stock_list;
}

FS_SINT4 FS_StockGetListCount( void )
{
	FS_List *head = FS_StockGetList( );
	if( head ){
		return FS_ListCount( head );
	}else{
		return 0;
	}
}

FS_StockConfig *FS_StockGetConfig( void )
{
	FS_ASSERT( GFS_StockSes != FS_NULL );
	if( GFS_StockSes == FS_NULL ) return FS_NULL;
	return &GFS_StockSes->config;
}

void FS_StockSetConfig( FS_StockConfig *config )
{
	FS_SINT4 orig_time;
	FS_ASSERT( GFS_StockSes != FS_NULL );
	if( GFS_StockSes == FS_NULL ) return;
	orig_time = GFS_StockSes->config.refresh_time;
	IFS_Memcpy( &GFS_StockSes->config, config, sizeof(FS_StockConfig) );
	FS_FileWrite( FS_DIR_STK, FS_STK_CFG_FILE, 0, config, sizeof(FS_StockConfig) );

	if( orig_time == 0 && config->refresh_time > 0 ){
		FS_StockUpdate( );
	}
}

void FS_StockAdd( FS_CHAR *stkId )
{
	FS_StockSession *stkSes = GFS_StockSes;
	FS_Stock *stock;

	FS_ASSERT( stkSes );
	if( stkSes == FS_NULL ) return;
	
	stock = IFS_Malloc( sizeof(FS_Stock) );
	FS_ASSERT( stock != FS_NULL );
	IFS_Memset( stock, 0, sizeof(FS_Stock) );
	IFS_Strncpy( stock->id, stkId, sizeof(stock->id) - 1 );
	
	FS_ListAddTail( &stkSes->stock_list, &stock->list );

	FS_StockUpdate( );
}

void FS_StockDelete( FS_Stock *stock )
{
	FS_StockSession *stkSes = GFS_StockSes;
	FS_ASSERT( stkSes != FS_NULL );
	if( stock == FS_NULL ) return;

	FS_ListDel( &stock->list );
	if(stock->realtime[0]){
		FS_FileDelete( FS_DIR_STK, stock->realtime );
	}
	IFS_Free( stock );
	FS_StkWriteStockList( stkSes );
}

void FS_StockMoveTop( FS_Stock *stock )
{
	FS_StockSession *stkSes = GFS_StockSes;
	FS_ASSERT( stkSes );
	if( stkSes == FS_NULL || stock == FS_NULL ) return;
	
	FS_ListDel( &stock->list );
	FS_ListAdd( &stkSes->stock_list, &stock->list );
}

void FS_StockRealTimeOff( void )
{
	FS_StockSession *stkSes = GFS_StockSes;
	FS_ASSERT( stkSes != FS_NULL );
	if( stkSes == FS_NULL ) return;
	
	IFS_Memset( stkSes->realtime_id, 0, sizeof(stkSes->realtime_id) );
	stkSes->state = FS_STK_STATE_UPDATE;
	FS_StkResetNetwork( stkSes );
}

FS_SINT4 FS_StockRealTimeOn( FS_CHAR *id )
{
	FS_StockSession *stkSes = GFS_StockSes;
	FS_ASSERT( stkSes != FS_NULL );
	if( stkSes == FS_NULL || id == FS_NULL ){
		return FS_STOCK_ERR_UNKNOW;
	}
	if( FS_StockFindById( id ) == FS_NULL ){
		return FS_STOCK_ERR_UNKNOW;
	}
	
	if( stkSes->state != FS_STK_STATE_IDLE ){
		/* 获取实时走势的优先级比自动更新高 */
		FS_StkResetNetwork( stkSes );
	}

	if( stkSes->timer_id ){
		IFS_StopTimer( stkSes->timer_id );
		stkSes->timer_id = 0;
	}
	IFS_Memset( stkSes->realtime_id, 0, sizeof(stkSes->realtime_id) );
	IFS_Strncpy( stkSes->realtime_id, id, sizeof(stkSes->realtime_id) - 1);
	stkSes->state = FS_STK_STATE_REALTIME;
	FS_NetConnect( stkSes->config.apn, stkSes->config.user, stkSes->config.pass, 
		FS_StkNetConnCallback, FS_APP_STK, FS_FALSE, stkSes );
	return FS_STOCK_OK;
}

void FS_StockDetailOff( void )
{
	FS_StockSession *stkSes = GFS_StockSes;
	FS_ASSERT( stkSes != FS_NULL );
	if( stkSes == FS_NULL ) return;
	
	IFS_Memset( stkSes->realtime_id, 0, sizeof(stkSes->realtime_id) );
	stkSes->state = FS_STK_STATE_UPDATE;
	FS_StkResetNetwork( stkSes );
}

FS_SINT4 FS_StockDetailOn( FS_CHAR *id )
{
	FS_StockSession *stkSes = GFS_StockSes;
	FS_ASSERT( stkSes != FS_NULL );
	if( stkSes == FS_NULL || id == FS_NULL ){
		return FS_STOCK_ERR_UNKNOW;
	}
	if( FS_StockFindById( id ) == FS_NULL ){
		return FS_STOCK_ERR_UNKNOW;
	}
	
	if( stkSes->state != FS_STK_STATE_IDLE ){
		/* 获取详情的优先级比自动更新高 */
		FS_StkResetNetwork( stkSes );
	}

	if( stkSes->timer_id ){
		IFS_StopTimer( stkSes->timer_id );
		stkSes->timer_id = 0;
	}
	IFS_Memset( stkSes->realtime_id, 0, sizeof(stkSes->realtime_id) );
	IFS_Strncpy( stkSes->realtime_id, id, sizeof(stkSes->realtime_id) - 1);
	stkSes->state = FS_STK_STATE_DETAIL;
	FS_NetConnect( stkSes->config.apn, stkSes->config.user, stkSes->config.pass, 
		FS_StkNetConnCallback, FS_APP_STK, FS_FALSE, stkSes );
	return FS_STOCK_OK;
}

FS_StockExponent *FS_StockGetExponentList( void )
{
	return GFS_StockExps;
}

#endif
