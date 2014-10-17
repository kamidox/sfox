#include "inc/FS_Config.h"

#ifdef FS_MODULE_STK

#include "inc/inte/FS_Inte.h"
#include "inc/res/FS_Res.h"
#include "inc/gui/FS_Gui.h"
#include "inc/stk/FS_Stock.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_MemDebug.h"

#define FS_STK_DETAIL_LEN		1024

static void FS_StkBuildList( FS_Window *lwin );
static FS_CHAR *FS_StkFormatDetailText( FS_Stock *stk );
static void FS_StkManUpdate_CB( FS_Window *win );

static void FS_StkSysInit( void )
{
	FS_SystemInit( );
	FS_StockInit( );
}

/* 
 * some ugly platform's sprintf did not support "%.2f" format 
 * 一些不规范的平台不支持sprintf的"%.2f"格式的输出，所以用这个函数来代替 
*/
static FS_CHAR * FS_DoubleToString( FS_CHAR *str, double dval )
{
	int num, i = 0, start = 0;
	FS_SINT4 factor = 1000000000;
	FS_SINT4 ival, pval, tval;
	
	if( dval >= -0.00001 && dval <= 0.00001 ){
		IFS_Strcpy( str, "0.00" );
		return str;
	}
	
	if( dval < 0 ){
		*str ++ = '-';
		dval = -dval;
	}
	ival = (FS_SINT4)dval;
	pval = (FS_SINT4)((dval - ival) * 100);
	/* 处理四舍五入 */
	tval = (FS_SINT4)((dval - ival) * 1000);
	if( (tval % 10) > 5 )
		pval ++;
	
	/* 整数部分 */
	if( ival == 0 ){
		*str ++ = '0';
	}else{
		while( factor > 0 ){
			if( ival >= factor && ! start ){
				start = 1;
			}
			if( start ){
				num = ival / factor;
				ival -= num * factor;
				*str ++ = num + '0';
			}
			factor /= 10;
		}
	}
	/* 小数部分，保留两位 */
	*str ++ = '.';
	if( pval < 10 ){
		*str ++ = '0';
	}else{
		num = pval / 10;
		pval -= num * 10;
		*str ++ = num + '0';
	}
	*str ++ = pval + '0';

	*str = 0;
	return str;
}

static FS_Stock *FS_StkUIListGetCurItem( void )
{
	FS_Window *lwin = FS_WindowFindId( FS_W_StkListFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( lwin );
	FS_Stock *ret = FS_NULL;
	
	if( wgt ){
		ret = (FS_Stock *)wgt->private_data;
	}
	return ret;
}

static void FS_StkListUpdateData( FS_Window *lwin )
{
	FS_SINT4 idx, cnt;
	FS_Widget *wgt;
	FS_Stock *stk;
	FS_CHAR txt[64], str1[32], str2[32];

	cnt = FS_StockGetListCount( );
	for( idx = 0; idx < cnt; idx ++ ){
		wgt = FS_WindowGetWidget( lwin, idx );
		if( wgt == FS_NULL ) continue;		
		FS_WGT_CLR_GREEN_FLAG( wgt );
		FS_WGT_CLR_RED_FLAG( wgt );
		
		stk = (FS_Stock *)wgt->private_data;
		if( stk->cur_price > 0 ){
			/* some ugly platform's sprintf did not support "%.2f" format */
			FS_DoubleToString( str1, (double)stk->cur_price / 100 );
			FS_DoubleToString( str2, (double)stk->markup_rate / 100 );
			IFS_Sprintf( txt, "%s(%s%%)", str1, str2 );
		}else{
			IFS_Strcpy( txt, "-- -- --" );
		}
		if( stk->markup_val > 0 ){
			FS_WGT_SET_RED_FLAG( wgt );
		}
		else if( stk->markup_val < 0 ){
			FS_WGT_SET_GREEN_FLAG( wgt );
		}
		FS_COPY_TEXT( wgt->sub_cap, txt );
		if( stk->name[0] ){
			FS_WidgetSetText( wgt, stk->name );
		}
	}
}

static void FS_StkUIRealtimeUpdateData( FS_Window *rwin )
{
	FS_Stock *stk;
	FS_CHAR absFile[FS_MAX_PATH_LEN] = {0};
	FS_Widget *wgt;	

	wgt = FS_WindowGetWidget( rwin, FS_W_StkRealTimeImage );
	stk = FS_StkUIListGetCurItem( );
	
	if( wgt && stk && stk->realtime[0] ){
		FS_GetAbsFileName( FS_DIR_STK, stk->realtime, absFile );
		FS_WidgetSetImage( wgt, absFile );
	}	
}

static void FS_StkUIDetailUpdateData( FS_Window *dwin )
{
	FS_CHAR *str;
	FS_Stock *stk;
	FS_Widget *wgt;
	
	wgt = FS_WindowGetWidget( dwin, FS_W_StkDetailFrmText );
	stk = FS_StkUIListGetCurItem( );
	str = FS_StkFormatDetailText( stk );
	if( stk && str ){
		FS_WidgetSetText( wgt, str );
		FS_WidgetCalcHeight( wgt );
	}
	FS_SAFE_FREE( str );
}

void FS_StkUIRealtimeUpdate( void )
{
	FS_Window *win;
	
	if( FS_WindowIsTopMost( FS_W_StkRealTimeFrm ) ){
		win = FS_WindowFindId( FS_W_StkRealTimeFrm );
		FS_StkUIRealtimeUpdateData( win );
		FS_InvalidateRect( win, &win->client_rect );
	}
	FS_DestroyWindowByID( FS_W_StkUpdateWaitingFrm );
}

void FS_StkUIDetailUpdate( void )
{
	FS_Window *dwin;
	
	if( FS_WindowIsTopMost( FS_W_StkDetailFrm ) )
	{
		dwin = FS_WindowFindId( FS_W_StkDetailFrm );
		FS_StkUIDetailUpdateData( dwin );
		FS_InvalidateRect( dwin, &dwin->client_rect );
	}
	FS_DestroyWindowByID( FS_W_StkUpdateWaitingFrm );
}

void FS_StkUIListUpdate( void )
{
	FS_Window *lwin;

	if( FS_WindowIsTopMost( FS_W_StkListFrm ) )
	{
		lwin = FS_WindowFindId( FS_W_StkListFrm );
		FS_StkListUpdateData( lwin );
		FS_InvalidateRect( lwin, &lwin->client_rect );
	}
	FS_DestroyWindowByID( FS_W_StkUpdateWaitingFrm );
}

void FS_StkUIUpdateFailed( void )
{
	FS_DestroyWindowByID( FS_W_StkUpdateWaitingFrm );
	if( FS_WindowFindId( FS_W_StkListFrm ) ){
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
	}
}

static void FS_StkAddStock_CB( FS_Window *win )
{
	FS_Window *ewin;
	FS_CHAR *str, *stkId;
	FS_Widget *wgt;
	
	ewin = FS_WindowFindId( FS_W_StkAddFrm );
	wgt = FS_WindowGetFocusItem( ewin );
	if( wgt->data == FS_NULL ){
		str = FS_WindowGetWidgetText( ewin, FS_W_StkAddFrmStkId );
	}else{
		str = wgt->data;
	}
	if( ! FS_STR_NI_EQUAL(str, "000", 3) && ! FS_STR_NI_EQUAL(str, "002", 3) 
		&& ! FS_STR_NI_EQUAL(str, "600", 3) && ! FS_STR_NI_EQUAL(str, "601", 3)
		&& ! FS_STR_NI_EQUAL(str, "sh", 2) && ! FS_STR_NI_EQUAL(str, "sz", 2) ){
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_INVALID_STK_ID), FS_NULL, FS_FALSE );
		return;
	}
	if( FS_STR_NI_EQUAL(str, "600", 3) || FS_STR_NI_EQUAL(str, "601", 3)){
		stkId = FS_StrConCat( "sh", str, FS_NULL, FS_NULL );
	}else if( FS_STR_NI_EQUAL(str, "002", 3) || FS_STR_NI_EQUAL(str, "000", 3)){
		stkId = FS_StrConCat( "sz", str, FS_NULL, FS_NULL );
	}else{
		stkId = IFS_Strdup( str );
	}
	
	FS_StockAdd( stkId );
	IFS_Free( stkId );
	FS_StkBuildList( FS_WindowFindId(FS_W_StkListFrm) );
	FS_MessageBox( FS_MS_OK, FS_Text(FS_T_SUCCESS), FS_NULL, FS_FALSE );
	FS_DestroyWindow( ewin );
}

static void FS_StkAddStock_UI( FS_Window *win )
{
	FS_Window *ewin;
	FS_Widget *wEdit, *wTip, *wExp;
	FS_EditParam eParam = { FS_IM_123, FS_IM_123, FS_STOCK_ID_LEN, FS_FALSE };
	FS_SINT4 cnt;
	FS_StockExponent *exp;
	
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );

	cnt = FS_StockGetListCount( );
	if( cnt >= FS_STOCK_MAX_STOCK_NUM ){
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_LIST_FULL), FS_NULL, FS_FALSE );
		return;
	}
	ewin = FS_CreateWindow( FS_W_StkAddFrm, FS_Text(FS_T_ADD_STOCK), FS_NULL );
	wTip = FS_CreateLabel( 0, FS_Text(FS_T_STK_PLS_INPUT), FS_I_INFO, 1 );
	wEdit = FS_CreateEditBox( FS_W_StkAddFrmStkId, FS_NULL, FS_I_EDIT, 1, &eParam );
	FS_WindowAddWidget( ewin, wTip );
	FS_WindowAddWidget( ewin, wEdit );
	
	/* exponent list */
	exp = FS_StockGetExponentList( );
	while( exp->id != FS_NULL ){
		wExp = FS_CreateListItem( 0, exp->name, FS_NULL, FS_I_FILE, 1 );
		wExp->data = IFS_Strdup( exp->id );
		FS_WidgetSetHandler( wExp, FS_StkAddStock_CB );
		FS_WindowAddWidget( ewin, wExp );

		exp ++;
	}
	
	FS_WindowSetSoftkey( ewin, 1, FS_Text(FS_T_OK), FS_StkAddStock_CB );
	FS_WindowSetSoftkey( ewin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );

	FS_ShowWindow( ewin );
}

static FS_BOOL FS_StkDelStockCnf_CB( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	if( wparam == FS_EV_YES )	// exit without save account data
	{
		FS_Window *lwin = FS_WindowFindId( FS_W_StkListFrm );
		FS_Widget *wgt = FS_WindowGetFocusItem( lwin );
		if( wgt ){
			FS_StockDelete( (FS_Stock *)wgt->private_data );
			FS_WindowDelWidget( lwin, wgt );
		}
		ret = FS_TRUE;
	}
	return ret;
}

static void FS_StkDelStock_CB( FS_Window *win )
{
	FS_Window *lwin = FS_WindowFindId( FS_W_StkListFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( lwin );

	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
	
	if( wgt ){
		FS_MessageBox( FS_MS_YES_NO, FS_Text(FS_T_CONFIRM_DEL), FS_StkDelStockCnf_CB, FS_FALSE ); 
	}
}

static FS_CHAR *FS_StkFormatDetailText( FS_Stock *stk )
{
	FS_CHAR *str, txt[32];
	FS_SINT4 offset = 0;

	str = IFS_Malloc( FS_STK_DETAIL_LEN );
	FS_ASSERT( str != FS_NULL );
	IFS_Memset( str, 0, FS_STK_DETAIL_LEN );
	offset += IFS_Sprintf( str + offset, "%s(%s)\r\n", stk->name, stk->id + 2 );
	FS_DoubleToString( txt, (double)stk->markup_rate / 100 );
	offset += IFS_Sprintf( str + offset, "%s: %s%%\r\n", FS_Text(FS_T_MARKUP_RATE), txt );
	FS_DoubleToString( txt, (double)stk->markup_val / 100 );
	offset += IFS_Sprintf( str + offset, "%s: %s\r\n", FS_Text(FS_T_MARKUP_VAL), txt );
	if( stk->closing_price > 0 ){
		FS_DoubleToString( txt, (double)(stk->highest_price - stk->lowest_price) * 100 / stk->closing_price );
	}else{
		IFS_Strcpy( txt, "0.00" );
	}
	offset += IFS_Sprintf( str + offset, "%s: %s%%\r\n", FS_Text(FS_T_RANGE_RATE), txt );
	FS_DoubleToString( txt, (double)stk->closing_price / 100 );
	offset += IFS_Sprintf( str + offset, "%s: %s\r\n", FS_Text(FS_T_CLOSING_PRICE), txt );
	FS_DoubleToString( txt, (double)stk->opening_price / 100 );
	offset += IFS_Sprintf( str + offset, "%s: %s\r\n", FS_Text(FS_T_OPENING_PRICE), txt );
	FS_DoubleToString( txt, (double)stk->highest_price / 100 );
	offset += IFS_Sprintf( str + offset, "%s: %s\r\n", FS_Text(FS_T_HIGHEST_PRICE), txt );
	FS_DoubleToString( txt, (double)stk->lowest_price / 100 );
	offset += IFS_Sprintf( str + offset, "%s: %s\r\n", FS_Text(FS_T_LOWEST_PRICE), txt );
	FS_DoubleToString( txt, (double)stk->cur_price / 100 );
	offset += IFS_Sprintf( str + offset, "%s: %s\r\n", FS_Text(FS_T_CUR_PRICE), txt );
	offset += IFS_Sprintf( str + offset, "%s: %d(%s)\r\n", FS_Text(FS_T_TRADE_QUANTUM), stk->total_quantum, FS_Text(FS_T_HAND) );
	offset += IFS_Sprintf( str + offset, "%s: %d(%s)\r\n", FS_Text(FS_T_TRADE_MONEY), stk->total_money, FS_Text(FS_T_TEN_KILO) );

	return str;
}

static FS_BOOL FS_StkDetailWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	if( cmd == FS_WM_DESTROY ){
		FS_StockDetailOff( );
		ret = FS_TRUE;
	}else if( cmd == FS_WM_PAINT ){
		FS_StkUIDetailUpdateData( win );
	}
	return ret;
}

static void FS_StkStockDetail_UI( FS_Window *win )
{
	FS_Window *dwin;
	FS_Widget *wInfo;
	FS_Stock *stk;

	stk = FS_StkUIListGetCurItem( );
	if( stk ){
		dwin = FS_CreateWindow( FS_W_StkDetailFrm, FS_Text(FS_T_DETAIL), FS_StkDetailWndProc );

		wInfo = FS_CreateScroller( FS_W_StkDetailFrmText, FS_NULL );
		
		FS_WindowAddWidget( dwin, wInfo );
		FS_WindowSetSoftkey( dwin, 1, FS_Text(FS_T_REFRESH), FS_StkManUpdate_CB );
		FS_WindowSetSoftkey( dwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
		FS_StockDetailOn( stk->id );
		FS_ShowWindow( dwin );
	}
	
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static FS_BOOL FS_StkRealTimeWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	if( cmd == FS_WM_DESTROY ){
		FS_StockRealTimeOff( );
		ret = FS_TRUE;
	}else if( cmd == FS_WM_PAINT ){
		FS_StkUIRealtimeUpdateData( win );
	}
	return ret;
}

static void FS_StkShowRealTime( FS_Stock *stk )
{
	FS_Window *win;
	FS_Widget *wImg, *wId;
	FS_CHAR str[32];

	win = FS_CreateWindow( FS_W_StkRealTimeFrm, FS_Text(FS_T_REAL_TIME), FS_StkRealTimeWndProc );
	wImg = FS_CreateImage( FS_W_StkRealTimeImage, FS_NULL );
	IFS_Sprintf( str, "%s(%s)", stk->name, stk->id + 2 );
	wId = FS_CreateLabel( 0, str, 0, 1 );

	FS_WindowAddWidget( win, wId );
	FS_WindowAddWidget( win, wImg );

	FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_REFRESH), FS_StkManUpdate_CB );
	FS_WindowSetSoftkey( win, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	FS_StockRealTimeOn( stk->id );
	FS_ShowWindow( win );
}

static void FS_StkRealTime_UI( FS_Window *win )
{
	FS_Window *lwin = FS_WindowFindId( FS_W_StkListFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( lwin );
	FS_Stock *stk;
	
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );

	if( wgt == FS_NULL ) return;
	stk = (FS_Stock *)wgt->private_data;
	FS_StkShowRealTime( stk );
}

static void FS_StkSettingSave_CB( FS_Window *win )
{
	FS_Widget *wgt;
	FS_CHAR *str;
	FS_StockConfig config;

	IFS_Memset( &config, 0, sizeof(FS_StockConfig) );
	str = FS_WindowGetWidgetText( win, FS_W_StkConfigApn );
	if( str ) IFS_Strncpy( config.apn, str, sizeof(config.apn) - 1 );
	str = FS_WindowGetWidgetText( win, FS_W_StkConfigUserName );
	if( str ) IFS_Strncpy( config.user, str, sizeof(config.user) - 1 );
	str = FS_WindowGetWidgetText( win, FS_W_StkConfigPasswd );
	if( str ) IFS_Strncpy( config.pass, str, sizeof(config.pass) - 1 );
	wgt = FS_WindowGetWidget( win, FS_W_StkConfigUseProxy );
	config.use_proxy = FS_WGT_GET_CHECK( wgt );
	str = FS_WindowGetWidgetText( win, FS_W_StkConfigProxyAddr );
	if( str ) IFS_Strncpy( config.proxy_addr, str, sizeof(config.proxy_addr) - 1 );
	str = FS_WindowGetWidgetText( win, FS_W_StkConfigProxyPort );
	if( str ) config.proxy_port = IFS_Atoi( str );
	str = FS_WindowGetWidgetText( win, FS_W_StkConfigRefreshTime );
	if( str ) config.refresh_time = IFS_Atoi( str );

	FS_StockSetConfig( &config );
	
	FS_MessageBox( FS_MS_OK, FS_Text(FS_T_CONFIG_UPDATED), FS_NULL, FS_FALSE );
	FS_DestroyWindow( win );
}

static void FS_StkManUpdate_CB( FS_Window *win )
{
	FS_SINT4 ret;
	
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
	
	ret = FS_StockUpdate( );
	if( ret == FS_STOCK_ERR_OK ){
		win = FS_MessageBox( FS_MS_OK, FS_Text(FS_T_BG_UPDATING), FS_NULL, FS_FALSE );
		win->id = FS_W_StkUpdateWaitingFrm;
	}else if( ret == FS_STOCK_ERR_SERV_BUSY){
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_BUSY), FS_NULL, FS_FALSE );
	}else{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_UNKNOW_ERR), FS_NULL, FS_FALSE );
	}
}
static void FS_StkMoveTop_CB( FS_Window *win )
{
	FS_Stock *stk = FS_StkUIListGetCurItem( );
	FS_Window *lwin = FS_WindowFindId( FS_W_StkListFrm );
	if( stk && lwin ){
		FS_StockMoveTop( stk );
		FS_StkBuildList( lwin );
	}
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
	
}

static void FS_StkSysSetting_UI( FS_Window *win )
{	
	FS_Widget *wRefreshTime, *wUseProxy, *wProxyAddr, *wProxyPort, *wApn, *wUser, *wPasswd;
	FS_Window *twin;
	FS_EditParam eParam = { FS_IM_ABC, FS_IM_ABC | FS_IM_123, FS_URL_LEN - 1 };
	FS_StockConfig *config = FS_StockGetConfig( );
	FS_CHAR str[16];
		
	twin = FS_CreateWindow( FS_W_StkConnSetFrm, FS_Text(FS_T_CONN_SET), FS_NULL );
	wUseProxy = FS_CreateCheckBox( FS_W_StkConfigUseProxy, FS_Text(FS_T_USE_PROXY) );
	FS_WidgetSetCheck( wUseProxy, config->use_proxy );
	wProxyAddr = FS_CreateEditBox( FS_W_StkConfigProxyAddr, config->proxy_addr, FS_I_EDIT, 1, &eParam );
	eParam.preferred_method = FS_IM_123;
	eParam.allow_method = FS_IM_123;
	eParam.max_len = 12;
	IFS_Itoa( config->proxy_port, str, 10 );
	wProxyPort = FS_CreateEditBox( FS_W_StkConfigProxyPort, str, FS_I_EDIT, 1, &eParam );
	eParam.preferred_method = FS_IM_ABC;
	eParam.allow_method = FS_IM_ALL;
	eParam.max_len = FS_URL_LEN - 1;
	wApn = FS_CreateEditBox( FS_W_StkConfigApn, config->apn, FS_I_EDIT, 1, &eParam );
	eParam.preferred_method = FS_IM_ABC;
	eParam.allow_method = FS_IM_ALL;
	eParam.max_len = FS_URL_LEN - 1;
	wUser = FS_CreateEditBox( FS_W_StkConfigUserName, config->user, FS_I_EDIT, 1, &eParam );
	eParam.preferred_method = FS_IM_ABC;
	eParam.allow_method = FS_IM_ALL;
	eParam.max_len = FS_URL_LEN - 1;
	wPasswd = FS_CreateEditBox( FS_W_StkConfigPasswd, config->pass, FS_I_EDIT, 1, &eParam );
	eParam.preferred_method = FS_IM_123;
	eParam.allow_method = FS_IM_123;
	eParam.max_len = 3;
	IFS_Itoa( config->refresh_time, str, 10 );
	wRefreshTime = FS_CreateEditBox( FS_W_StkConfigRefreshTime, str, FS_I_EDIT, 1, &eParam );

	wRefreshTime->tip = FS_Text(FS_T_REFRESH_TIME);
	wProxyAddr->tip = FS_Text(FS_T_PROXY_ADDR);
	wProxyPort->tip = FS_Text(FS_T_PROXY_PORT);
	wApn->tip = FS_Text(FS_T_APN);
	wUser->tip = FS_Text(FS_T_USER_NAME);
	wPasswd->tip = FS_Text(FS_T_PASSWORD);
	
	twin->draw_status_bar = FS_TRUE;
	twin->pane.view_port.height -= IFS_GetLineHeight( );

	FS_WindowAddWidget( twin, wRefreshTime );
	FS_WindowAddWidget( twin, wUseProxy );
	FS_WindowAddWidget( twin, wProxyAddr );
	FS_WindowAddWidget( twin, wProxyPort );
	FS_WindowAddWidget( twin, wApn );
	FS_WindowAddWidget( twin, wUser );
	FS_WindowAddWidget( twin, wPasswd );
	
	FS_WindowSetSoftkey( twin, 1, FS_Text(FS_T_SAVE), FS_StkSettingSave_CB );
	FS_WindowSetSoftkey( twin, 3, FS_Text(FS_T_CANCEL), FS_StandardKey3Handler );
	
	FS_ShowWindow( twin );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_StkListMenu_UI( FS_Window *win )
{
	FS_Widget *iAdd, *iDel, *iDetail, *iRealTime, *iUpdate, *iSetting, *iMoveTop;
	FS_Window *pMenu;

	iAdd = FS_CreateMenuItem( 0, FS_Text(FS_T_ADD_STOCK) );
	iDel = FS_CreateMenuItem( 0, FS_Text(FS_T_DEL) );
	iDetail = FS_CreateMenuItem( 0, FS_Text(FS_T_DETAIL) );
	iRealTime = FS_CreateMenuItem( 0, FS_Text(FS_T_REAL_TIME) );
	iUpdate = FS_CreateMenuItem( 0, FS_Text(FS_T_MAN_UPDATE) );
	iSetting = FS_CreateMenuItem( 0, FS_Text(FS_T_SYS_SETTING) );
	iMoveTop = FS_CreateMenuItem( 0, FS_Text(FS_T_MOVE_UP) );
	
	pMenu = FS_CreateMenu( 0, 7 );
	FS_MenuAddItem( pMenu, iAdd );
	FS_MenuAddItem( pMenu, iDel );
	FS_MenuAddItem( pMenu, iDetail );
	FS_MenuAddItem( pMenu, iRealTime );
	FS_MenuAddItem( pMenu, iUpdate );
	FS_MenuAddItem( pMenu, iMoveTop );
	FS_MenuAddItem( pMenu, iSetting );

	FS_WidgetSetHandler( iAdd, FS_StkAddStock_UI );
	FS_WidgetSetHandler( iDel, FS_StkDelStock_CB );
	FS_WidgetSetHandler( iDetail, FS_StkStockDetail_UI );
	FS_WidgetSetHandler( iRealTime, FS_StkRealTime_UI );
	FS_WidgetSetHandler( iUpdate, FS_StkManUpdate_CB );
	FS_WidgetSetHandler( iMoveTop, FS_StkMoveTop_CB );
	FS_WidgetSetHandler( iSetting, FS_StkSysSetting_UI );

	FS_MenuSetSoftkey( pMenu );
	FS_ShowWindow( pMenu );	
}

static void FS_StkBuildList( FS_Window *lwin )
{
	FS_List *head, *node;
	FS_Stock *stk;
	FS_CHAR *title;
	FS_Widget *wgt;
	FS_SINT4 idx = 0;
	
	FS_WindowDelWidgetList( lwin );

	head = FS_StockGetList( );
	FS_ListForEach( node, head ){
		stk = FS_ListEntry( node, FS_Stock, list );
		
		title = stk->name;
		if( title[0] == 0 ) title = stk->id;
		wgt = FS_CreateListItem( idx ++, title, "-- -- --", FS_I_FILE, 2 );
		wgt->private_data = (FS_UINT4)stk;
		if( stk->markup_val > 0 )
			FS_WGT_SET_RED_FLAG( wgt );
		else if( stk->markup_val < 0 )
			FS_WGT_SET_GREEN_FLAG( wgt );
		
		FS_WidgetSetHandler( wgt, FS_StkStockDetail_UI );
		FS_WindowAddWidget( lwin, wgt );
	}	
}

static FS_BOOL FS_StkMainWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	if( cmd == FS_WM_PAINT ){
		FS_StkListUpdateData( win );
	}else if( cmd == FS_WM_DESTROY ){
		FS_StockDeinit( );
		FS_DeactiveApplication( FS_APP_STK );
		ret = FS_TRUE;
	}else if( cmd == FS_WM_COMMAND ){
		if( wparam == FS_KEY_5 ){
			FS_StkManUpdate_CB( win );
		}else if( wparam == FS_KEY_4 ){
			FS_StkRealTime_UI( win );
		}
	}
	return ret;
}

void FS_StkMain( void )
{
	FS_Window *lwin;
	
	FS_StkSysInit( );
	FS_ActiveApplication( FS_APP_STK );
	
	lwin = FS_CreateWindow( FS_W_StkListFrm, FS_Text(FS_T_STK_LIST), FS_StkMainWndProc );
	lwin->show_index = FS_TRUE;

	FS_StkBuildList( lwin );
	FS_WindowSetSoftkey( lwin, 1, FS_Text(FS_T_MENU), FS_StkListMenu_UI );
	FS_WindowSetSoftkey( lwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	FS_ShowWindow( lwin );
}

#endif

