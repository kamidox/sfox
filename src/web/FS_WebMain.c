#include "inc/FS_Config.h"

#ifdef FS_MODULE_WEB

#include "inc/gui/FS_WebGui.h"
#include "inc/web/FS_WebDoc.h"
#include "inc/web/FS_WebUtil.h"
#include "inc/web/FS_History.h"
#include "inc/web/FS_WebConfig.h"
#include "inc/web/FS_Cookie.h"
#include "inc/web/FS_Push.h"
#include "inc/inte/FS_Inte.h"
#include "inc/res/FS_Res.h"
#include "inc/util/FS_File.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_Charset.h"
#include "inc/util/FS_NetConn.h"
#include "inc/util/FS_MemDebug.h"

#define FS_ITEM_DETAIL_LEN			2048
#define FS_MAX_BOOKMARK_NUM		100
#define FS_MAX_BOOKMARK_DIR_NUM	6

static void FS_BookmarkListMenu_UI( FS_Window *win );
static FS_CHAR *FS_WebGetSCDetail( FS_WebShortCut sc, FS_WidgetEventHandler *pEvHandler );
static void FS_WebStartPage( FS_Window *win );
void FS_WebMainMenu_UI( FS_Window *win );
void FS_HistoryBack_CB( FS_Window *win );

static FS_BOOL FS_WebSysInit( void )
{
	FS_SystemInit( );
	FS_CacheInit( );
	FS_WebConfigInit( );
	
	return FS_TRUE;
}

static void FS_WebExit_CB( FS_Window *win )
{
	FS_CacheDeinit( );
	FS_BookmarkDeinit( );
	FS_WebUtilDeinit( );
	FS_PushDeinit( );
	FS_CookieClear( );
	FS_DeactiveApplication( FS_APP_WEB );
	FS_DestroyWindowByID( FS_W_WebMainFrm );
	FS_DestroyWindowByID( FS_W_WebMainMenu );
	FS_NetDisconnect( FS_APP_WEB );
	if( ! FS_HaveActiveApplication() )
	{
		FS_GuiExit( );
		IFS_SystemExit( );
	}
}

static FS_Window *FS_WebGetMainWin( void )
{
	FS_Window *win;

	win = FS_WindowFindId( FS_W_WebMainFrm );
	if( win == FS_NULL )
	{
		win = FS_WebCreateWin( FS_W_WebMainFrm, FS_Text(FS_T_WEB), FS_NULL );
		FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_MENU), FS_WebMainMenu_UI );
		FS_WindowSetSoftkey( win, 3, FS_Text(FS_T_FALL_BACK), FS_HistoryBack_CB );
	}
	return win;
}

static void FS_ViewHistoryFile_CB( FS_Window *win )
{
	FS_HistoryEntry *he;
	FS_Widget *wgt;
	FS_Window *lwin = FS_WindowFindId( FS_W_WebHistoryListFrm );
	FS_Window *vwin = FS_WebGetMainWin( );
	
	wgt = FS_WindowGetFocusItem( lwin );
	if( wgt && wgt->private_data )
	{
		he = (FS_HistoryEntry *)wgt->private_data;
		FS_HistorySetCurrent( he->url );
		FS_SetCurWebPageUrl( he->url );
		FS_LoadWebDoc( vwin, he->file, FS_NULL, FS_FALSE, FS_TRUE );
		FS_DestroyWindow( lwin );
	}
	
	if( win && ( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_HistoryItemDetail_UI( FS_Window *win )
{
	FS_Window *lwin = FS_WindowFindId( FS_W_WebHistoryListFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( lwin );
	FS_CHAR *info;
	FS_SINT4 offset;
	FS_HistoryEntry *he;
	
	if( wgt && wgt->private_data )
	{
		he = (FS_HistoryEntry *)wgt->private_data;
		if( he )
		{
			info = IFS_Malloc( FS_ITEM_DETAIL_LEN );
			offset = 0;
			if( info )
			{
				IFS_Sprintf( info + offset, "%s: %s\n", FS_Text(FS_T_SUBJECT), he->title );
				offset += IFS_Strlen( info + offset );

				IFS_Sprintf( info + offset, "%s: %s\n", FS_Text(FS_T_WEB_URL), he->url );
				offset += IFS_Strlen( info + offset );

				IFS_Sprintf( info + offset, "%s: %d-%d-%d %d:%d:%d\n", FS_Text(FS_T_DATE), 
					he->date.year, he->date.month, he->date.day, he->date.hour, he->date.min, he->date.sec );
				offset += IFS_Strlen( info + offset );

				IFS_Sprintf( info + offset, "%s: %s", FS_Text(FS_T_WEB_CACHE), he->file );
				offset += IFS_Strlen( info + offset );
				
				FS_StdShowDetail( FS_Text(FS_T_DETAIL), info );
				IFS_Free( info );
			}
		}
	}

	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_GotoUrl_CB( FS_Window *win )
{
	FS_Window *mwin = FS_WebGetMainWin( );
	FS_CHAR *text = FS_FindWebWgtValue( win, "url" );
	if( text )
	{
		FS_CHAR *url = FS_NULL;
		if( IFS_Strnicmp(text, "http", 4) != 0 )
		{
			url = FS_StrConCat( "http://", text, FS_NULL, FS_NULL );
			FS_SetCurWebPageUrl( url );
			FS_WebGoToUrl( mwin, url, FS_FALSE );
			IFS_Free( url );
		}
		else
		{
			FS_SetCurWebPageUrl( text );
			FS_WebGoToUrl( mwin, text, FS_FALSE );
		}
	}
	FS_DestroyWindowByID( FS_W_WebUrlInput );
}

static void FS_GotoUrl_UI( FS_Window *win )
{
	FS_Window *ewin = FS_WebCreateWin( FS_W_WebUrlInput, FS_Text(FS_T_GOTO_URL), FS_NULL );
	FS_WebWgt *wwgt;
	FS_CHAR *url;

	url = FS_GetCurWebPageUrl( );
	if( url == FS_NULL ) url = FS_WebConfigGetHomePage( );
	/* url input box */
	wwgt = FS_WwCreateInput( "url", url );
	FS_WebWinAddWebWgt( ewin, wwgt, 1 );
	/* insert text */
	wwgt = FS_WwCreateLink( "http://", "sfox::urlinput(http://)" );
	FS_WebWinAddWebWgt( ewin, wwgt, 1 );
	wwgt = FS_WwCreateText( "|" );
	FS_WebWinAddWebWgt( ewin, wwgt, 0 );
	wwgt = FS_WwCreateLink( "wap.", "sfox::urlinput(wap.)" );
	FS_WebWinAddWebWgt( ewin, wwgt, 0 );
	wwgt = FS_WwCreateText( "|" );
	FS_WebWinAddWebWgt( ewin, wwgt, 0 );
	wwgt = FS_WwCreateLink( "www.", "sfox::urlinput(www.)" );
	FS_WebWinAddWebWgt( ewin, wwgt, 0 );
	/* append text */
	wwgt = FS_WwCreateLink( ".com", "sfox::urlinput(.com)" );
	FS_WebWinAddWebWgt( ewin, wwgt, 1 );
	wwgt = FS_WwCreateText( "|" );
	FS_WebWinAddWebWgt( ewin, wwgt, 0 );
	wwgt = FS_WwCreateLink( ".cn", "sfox::urlinput(.cn)" );
	FS_WebWinAddWebWgt( ewin, wwgt, 0 );
	wwgt = FS_WwCreateText( "|" );
	FS_WebWinAddWebWgt( ewin, wwgt, 0 );
	wwgt = FS_WwCreateLink( ".com.cn", "sfox::urlinput(.com.cn)" );
	FS_WebWinAddWebWgt( ewin, wwgt, 0 );
	wwgt = FS_WwCreateText( "|" );
	FS_WebWinAddWebWgt( ewin, wwgt, 0 );
	wwgt = FS_WwCreateLink( ".net", "sfox::urlinput(.net)" );
	FS_WebWinAddWebWgt( ewin, wwgt, 0 );
	/* delete text */
	wwgt = FS_WwCreateLink( FS_Text(FS_T_DEL_ALL), "sfox::urlinput(clear)" );
	FS_WebWinAddWebWgt( ewin, wwgt, 1 );
	
	FS_WindowSetSoftkey( ewin, 1, FS_Text(FS_T_OK), FS_GotoUrl_CB );
	FS_WindowSetSoftkey( ewin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	FS_ShowWindow( ewin );
	FS_DestroyWindowByID( FS_W_WebMainMenu );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_WebSettingSave_CB( FS_Window *win )
{
	FS_UINT2 port = 0;
	FS_Widget *wgt;
	FS_Window *swin;
	FS_SINT4 i, maxTimer = 0;
	
	swin = FS_WindowFindId( FS_W_WebConnSetFrm );
	if( swin )
	{
		wgt = FS_WindowGetWidget( swin, FS_W_WebConfigProtoHttp );
		if( FS_WGT_GET_CHECK(wgt) )
		{
			FS_WebConfigSetProtocol( FS_WEB_HTTP );
			port = 80;
		}
		else
		{
			FS_WebConfigSetProtocol( FS_WEB_WSP );
			port = 9201;
		}

		wgt = FS_WindowGetWidget( swin, FS_W_WebConfigUseProxy );
		FS_WebConfigSetUseProxy( FS_WGT_GET_CHECK(wgt) );

		wgt = FS_WindowGetWidget( swin, FS_W_WebConfigProxyAddr );
		FS_WebConfigSetProxyAddr( wgt->text );
			
		wgt = FS_WindowGetWidget( swin, FS_W_WebConfigProxyPort );
		if( wgt->text )
			port = ( FS_UINT2 )IFS_Atoi( wgt->text );
		FS_WebConfigSetProxyPort( port );

		wgt = FS_WindowGetWidget( swin, FS_W_WebConfigApn );
		FS_WebConfigSetApn( wgt->text );

		wgt = FS_WindowGetWidget( swin, FS_W_WebConfigUserName );
		FS_WebConfigSetUserName( wgt->text );

		wgt = FS_WindowGetWidget( swin, FS_W_WebConfigPasswd);
		FS_WebConfigSetPassword( wgt->text );

		/* if we change the net setting. reset net transfer */
		FS_QuitTransSession( );
	}

	swin = FS_WindowFindId( FS_W_WebGeneralSetFrm );
	if( swin )
	{
		wgt = FS_WindowGetWidget( swin, FS_W_WebGeneralSetShowImage );
		FS_WebConfigSetShowImageFlag( FS_WGT_GET_CHECK(wgt) );

		wgt = FS_WindowGetWidget( swin, FS_W_WebGeneralSetPlayAudio );
		FS_WebConfigSetPlayAudioFlag( FS_WGT_GET_CHECK(wgt) );
		
		wgt = FS_WindowGetWidget( swin, FS_W_WebGeneralSetUseCacheImage );
		FS_WebConfigSetUseCacheImageFlag( FS_WGT_GET_CHECK(wgt) );
		
		wgt = FS_WindowGetWidget( swin, FS_W_WebGeneralSetHomePage );
		FS_WebConfigSetHomePage( wgt->text );

		wgt = FS_WindowGetWidget( swin, FS_W_WebGeneralSetMaxTimer );
		if( wgt->text )
			maxTimer = IFS_Atoi( wgt->text );
		FS_WebConfigSetMaxNetworkTimer( maxTimer );
	}

	swin = FS_WindowFindId( FS_W_WebSCSetFrm );
	if( swin )
	{
		wgt = FS_WindowGetWidget( swin, FS_W_WebSCEnable );
		FS_WebConfigSetShortCutEnableFlag( FS_WGT_GET_CHECK(wgt) );
		for( i = 0; i < FS_SC_MAX_NUM; i ++ )
		{
			wgt = FS_WindowGetWidget( swin, FS_W_WebSCSetKey0 + i );
			FS_WebConfigSetShortCut( (FS_WebShortCut)wgt->private_data, FS_KEY_0 + i );
		}
	}

	FS_WebConfigSave( );
	FS_MessageBox( FS_MS_OK, FS_Text(FS_T_CONFIG_UPDATED), FS_NULL, FS_FALSE );
	FS_DestroyWindow( win );
}

static FS_BOOL FS_WebConfigWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	FS_Widget *wgt;
	
	if( cmd == FS_WM_COMMAND && wparam == FS_EV_ITEM_VALUE_CHANGE )
	{
		wgt = ( FS_Widget *)lparam;
		if( wgt->id == FS_W_WebConfigProtoHttp || wgt->id == FS_W_WebConfigProtoWsp )
		{
			wgt = FS_WindowGetWidget( win, FS_W_WebConfigProtoHttp );
			if( FS_WGT_GET_CHECK(wgt) )
			{
				wgt = FS_WindowGetWidget( win, FS_W_WebConfigProxyPort );
				FS_WidgetSetText( wgt, "80" );
			}
			else
			{
				wgt = FS_WindowGetWidget( win, FS_W_WebConfigProxyPort );
				FS_WidgetSetText( wgt, "9201" );

				wgt = FS_WindowGetWidget( win, FS_W_WebConfigUseProxy );
				FS_WidgetSetCheck( wgt, FS_TRUE );
			}
			ret = FS_TRUE;
		}
	}
	return ret;
}

static void FS_WebGeneralSetting_UI( FS_Window *win )
{
	FS_Window *gwin;
	FS_Widget *wDownImage, *wPlayAudio, *wCacheImgFirst, *wHomePage, *wMaxTimer;
	FS_CHAR str[16];
	FS_EditParam eParam = { FS_IM_ABC, FS_IM_ABC | FS_IM_123, FS_URL_LEN - 1 };
	
	IFS_Itoa( FS_WebConfigGetMaxNetworkTimer(), str, 10 );
	gwin = FS_CreateWindow( FS_W_WebGeneralSetFrm, FS_Text(FS_T_WEB_GENERAL), FS_NULL );
	wHomePage = FS_CreateEditBox( FS_W_WebGeneralSetHomePage, FS_WebConfigGetHomePage(), FS_I_HOME, 1, &eParam );
	wDownImage = FS_CreateCheckBox( FS_W_WebGeneralSetShowImage, FS_Text(FS_T_WEB_DOWN_IMAGE) );
	wPlayAudio = FS_CreateCheckBox( FS_W_WebGeneralSetPlayAudio, FS_Text(FS_T_PLAY_BGSOUND) );
	wCacheImgFirst = FS_CreateCheckBox( FS_W_WebGeneralSetUseCacheImage, FS_Text(FS_T_WEB_USE_CACHE_IMAGE) );
	eParam.preferred_method = FS_IM_123;
	eParam.allow_method = FS_IM_123;
	eParam.max_len = 12;
	wMaxTimer = FS_CreateEditBox( FS_W_WebGeneralSetMaxTimer, str, FS_I_EDIT, 1, &eParam );
	
	FS_WidgetSetCheck( wDownImage, FS_WebConfigGetShowImageFlag() );
	FS_WidgetSetCheck( wPlayAudio, FS_WebConfigGetPlayAudioFlag() );
	FS_WidgetSetCheck( wCacheImgFirst, FS_WebConfigGetUseCacheImageFlag() );
	wCacheImgFirst->tip = FS_Text( FS_T_WEB_CACHE_IMAGE_FIRST_TIP );
	wHomePage->tip = FS_Text( FS_T_WEB_SET_HOME_PAGE_TIP );
	wMaxTimer->tip = FS_Text( FS_T_MAX_WEB_TIMER );
	
	FS_WindowAddWidget( gwin, wHomePage );
	FS_WindowAddWidget( gwin, wDownImage );
	FS_WindowAddWidget( gwin, wPlayAudio );
	FS_WindowAddWidget( gwin, wCacheImgFirst );
	FS_WindowAddWidget( gwin, wMaxTimer );

	FS_WindowSetSoftkey( gwin, 1, FS_Text(FS_T_SAVE), FS_WebSettingSave_CB );
	FS_WindowSetSoftkey( gwin, 3, FS_Text(FS_T_CANCEL), FS_StandardKey3Handler );
	FS_ShowWindow( gwin );
		
	FS_DestroyWindowByID( FS_W_WebMainMenu );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_WebSCSelect_CB( FS_Window *win )
{
	FS_Window *swin = FS_WindowFindId( FS_W_WebSCSetFrm );
	FS_Widget *wkey = FS_WindowGetFocusItem( swin );
	FS_Widget *wSc = FS_WindowGetFocusItem( win );
	FS_WebShortCut sc = (FS_WebShortCut)wSc->private_data;

	/* now, update the setting UI */
	FS_WidgetSetText( wkey, FS_WebGetSCDetail(sc, FS_NULL) );
	wkey->private_data = sc;
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );	
}

static void FS_WebSCSelect_UI( FS_Window *win )
{
	FS_Window *menu;
	FS_Widget *wgt;
	FS_Widget *focusWgt = FS_WindowGetFocusItem( win );
	FS_Rect rect = FS_GetWidgetDrawRect( focusWgt );
	FS_SINT4 i;
	
	menu = FS_CreatePopUpMenu( FS_W_WebSCSelectMenu, &rect, FS_WSC_COUNT );
	for( i = 0; i < FS_WSC_COUNT; i ++ )
	{
		wgt = FS_CreateMenuItem( i, FS_WebGetSCDetail(FS_WSC_NONE + i, FS_NULL) );
		wgt->private_data = FS_WSC_NONE + i;
		FS_WidgetSetHandler( wgt, FS_WebSCSelect_CB );
		FS_MenuAddItem( menu, wgt );
	}
	
	FS_MenuSetSoftkey( menu );

	FS_ShowWindow( menu );
}

static FS_BOOL FS_WebRestoreDefaultSettingCnf_CB( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;

	if( wparam == FS_EV_YES )
	{
		FS_WebConfigSetDefault( );
		ret = FS_TRUE;
	}
	return ret;
}

static void FS_WebRestoreDefaultSetting_CB( FS_Window *win )
{
	FS_DestroyWindowByID( FS_W_WebMainMenu );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
	
	FS_MessageBox( FS_MS_YES_NO, FS_Text(FS_T_CONFIRM_OPERATION), FS_WebRestoreDefaultSettingCnf_CB, FS_FALSE );
}

static void FS_WebShortCutSetting_UI( FS_Window *win )
{
	FS_SINT4 i;
	FS_Widget *wgt;
	FS_WebShortCut sc;
	FS_Window *twin = FS_CreateWindow( FS_W_WebSCSetFrm, FS_Text(FS_T_WEB_SC), FS_WebConfigWndProc );

	wgt = FS_CreateCheckBox( FS_W_WebSCEnable, FS_Text(FS_T_WEB_SC_ENABLE) );
	FS_WidgetSetCheck( wgt, FS_WebConfigGetShortCutEnableFlag() );
	FS_WindowAddWidget( twin, wgt );
	for( i = 0; i < FS_SC_MAX_NUM; i ++ )
	{
		sc = FS_WebConfigGetShortCut( FS_KEY_0 + i );
		wgt = FS_CreateComboBox( FS_W_WebSCSetKey0 + i, FS_WebGetSCDetail(sc, FS_NULL), 0 );
		wgt->private_data = sc;
		FS_ComboBoxSetTitle( wgt, FS_Text((FS_SINT2)(FS_T_KEY0 + i)) );
		FS_WidgetSetHandler( wgt, FS_WebSCSelect_UI );

		FS_WindowAddWidget( twin, wgt );
	}
	
	FS_WindowSetSoftkey( twin, 1, FS_Text(FS_T_SAVE), FS_WebSettingSave_CB );
	FS_WindowSetSoftkey( twin, 3, FS_Text(FS_T_CANCEL), FS_StandardKey3Handler );

	FS_ShowWindow( twin );
	
	FS_DestroyWindowByID( FS_W_WebMainMenu );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_WebConnSetting_UI( FS_Window *win )
{
	FS_CHAR strPort[16], *strProxy, *strApn, *strUser, *strPasswd;
	FS_Widget *wProtHttp, *wProtWsp, *wUseProxy, *wProxyAddr, *wProxyPort, *wApn, *wUser, *wPasswd;
	FS_Window *twin = FS_CreateWindow( FS_W_WebConnSetFrm, FS_Text(FS_T_CONN_SET), FS_WebConfigWndProc );
	FS_EditParam eParam = { FS_IM_ABC, FS_IM_ABC | FS_IM_123, FS_URL_LEN - 1 };
	
	/* use id as skin's index. must follow up GFS_Skins define */
	IFS_Itoa( FS_WebConfigGetProxyPort(), strPort, 10 );
	strProxy = FS_WebConfigGetProxyAddr();
	strApn = FS_WebConfigGetApn( );
	strUser = FS_WebConfigGetUserName( );
	strPasswd = FS_WebConfigGetPassword( );
	
	wProtHttp = FS_CreateRadioBox( FS_W_WebConfigProtoHttp, FS_Text(FS_T_PROTO_HTTP) );
	wProtWsp = FS_CreateRadioBox( FS_W_WebConfigProtoWsp, FS_Text(FS_T_PROTO_WSP) );
	wUseProxy = FS_CreateCheckBox( FS_W_WebConfigUseProxy, FS_Text(FS_T_USE_PROXY) );
	wProxyAddr = FS_CreateEditBox( FS_W_WebConfigProxyAddr, strProxy, FS_I_EDIT, 1, &eParam );
	eParam.preferred_method = FS_IM_123;
	eParam.allow_method = FS_IM_123;
	eParam.max_len = 12;
	wProxyPort = FS_CreateEditBox( FS_W_WebConfigProxyPort, strPort, FS_I_EDIT, 1, &eParam );
	eParam.preferred_method = FS_IM_ABC;
	eParam.allow_method = FS_IM_ALL;
	eParam.max_len = FS_URL_LEN - 1;
	wApn = FS_CreateEditBox( FS_W_WebConfigApn, strApn, FS_I_EDIT, 1, &eParam );
	eParam.preferred_method = FS_IM_ABC;
	eParam.allow_method = FS_IM_ALL;
	eParam.max_len = FS_URL_LEN - 1;
	wUser = FS_CreateEditBox( FS_W_WebConfigUserName, strUser, FS_I_EDIT, 1, &eParam );
	eParam.preferred_method = FS_IM_ABC;
	eParam.allow_method = FS_IM_ALL;
	eParam.max_len = FS_URL_LEN - 1;
	wPasswd = FS_CreateEditBox( FS_W_WebConfigPasswd, strPasswd, FS_I_EDIT, 1, &eParam );

	
	wProxyAddr->tip = FS_Text(FS_T_PROXY_ADDR);
	wProxyPort->tip = FS_Text(FS_T_PROXY_PORT);
	wApn->tip = FS_Text(FS_T_APN);
	wUser->tip = FS_Text(FS_T_USER_NAME);
	wPasswd->tip = FS_Text(FS_T_PASSWORD);
	
	twin->draw_status_bar = FS_TRUE;
	twin->pane.view_port.height -= IFS_GetLineHeight( );
	
	FS_WindowAddWidget( twin, wProtHttp );
	FS_WindowAddWidget( twin, wProtWsp );
	FS_WindowAddWidget( twin, wUseProxy );
	FS_WindowAddWidget( twin, wProxyAddr );
	FS_WindowAddWidget( twin, wProxyPort );
	FS_WindowAddWidget( twin, wApn );
	FS_WindowAddWidget( twin, wUser );
	FS_WindowAddWidget( twin, wPasswd );
	
	if( FS_WebConfigGetProtocol() == FS_WEB_HTTP )
		FS_WidgetSetCheck( wProtHttp, FS_TRUE );
	else
		FS_WidgetSetCheck( wProtWsp, FS_TRUE );

	FS_WidgetSetCheck( wUseProxy, FS_WebConfigGetUseProxy() );
	
	FS_WindowSetSoftkey( twin, 1, FS_Text(FS_T_SAVE), FS_WebSettingSave_CB );
	FS_WindowSetSoftkey( twin, 3, FS_Text(FS_T_CANCEL), FS_StandardKey3Handler );
	
	FS_ShowWindow( twin );
	
	FS_DestroyWindowByID( FS_W_WebMainMenu );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static FS_BOOL FS_DelCachCnf_CB( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	FS_Window *mwin = FS_WebGetMainWin( );
	if( wparam == FS_EV_YES )
	{
		FS_CacheDeleteAll( );
		if( mwin->context.is_start_page )
		{
			FS_WebStartPage(  mwin );
		}
		ret = FS_TRUE;
	}
	return ret;
}

static void FS_DelCache_CB( FS_Window *win )
{
	FS_DestroyWindowByID( FS_W_WebMainMenu );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
	{
		FS_DestroyWindow( win );
	}
	FS_MessageBox( FS_MS_YES_NO, FS_Text(FS_T_CONFIRM_DEL), FS_DelCachCnf_CB, FS_FALSE );
}

static void FS_WebPageProperty_UI( FS_Window *win )
{
	
	FS_DestroyWindowByID( FS_W_WebMainMenu );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_WebSetting_UI( FS_Window *win )
{
	FS_Widget *itemConnSet, *itemGenSet, *itemAbout, *itemShortCut, *iSetDefault, *iDelHistory, *wgt;
	FS_Window *pMenu, *mwin;
	FS_Rect rect = { 0 };

	rect.top = IFS_GetScreenHeight( ) / 2;
	rect.width = IFS_GetScreenWidth( );
	rect.height = IFS_GetLineHeight( );
	mwin = FS_WindowFindId( FS_W_WebMainMenu );
	if( mwin )
	{
		wgt = FS_WindowGetFocusItem( mwin );
		rect = FS_GetWidgetDrawRect( wgt );
	}
	
	itemGenSet = FS_CreateMenuItem( 0, FS_Text(FS_T_WEB_GENERAL) );
	itemConnSet = FS_CreateMenuItem( 0, FS_Text(FS_T_CONN_SET) );
	iSetDefault = FS_CreateMenuItem( 0, FS_Text(FS_T_SET_DEFAULT) );
	itemShortCut = FS_CreateMenuItem( 0, FS_Text(FS_T_WEB_SC) );
	itemAbout = FS_CreateMenuItem( 0, FS_Text(FS_T_ABOUT) );
	iDelHistory = FS_CreateMenuItem( 0, FS_Text(FS_T_WEB_DEL_CACHE) );

	pMenu = FS_CreatePopUpMenu( FS_W_WebConfigMenu, &rect, 6 );
	FS_MenuAddItem( pMenu, itemGenSet );
	FS_MenuAddItem( pMenu, itemConnSet );
	FS_MenuAddItem( pMenu, iSetDefault );
	FS_MenuAddItem( pMenu, itemShortCut );
	FS_MenuAddItem( pMenu, iDelHistory );
	FS_MenuAddItem( pMenu, itemAbout );
	
	FS_WidgetSetHandler( itemGenSet, FS_WebGeneralSetting_UI );
	FS_WidgetSetHandler( itemConnSet, FS_WebConnSetting_UI );
	FS_WidgetSetHandler( itemShortCut, FS_WebShortCutSetting_UI );
	FS_WidgetSetHandler( iSetDefault, FS_WebRestoreDefaultSetting_CB );
	FS_WidgetSetHandler( iDelHistory, FS_DelCache_CB );
	FS_WidgetSetHandler( itemAbout, FS_ThemeSetting_UI );

	FS_MenuSetSoftkey( pMenu );
	FS_ShowWindow( pMenu );
}

static void FS_WebPushGoto_CB( FS_Window *win )
{
	FS_Widget *wgt;
	FS_Window *lwin = FS_WindowFindId( FS_W_WebPushListFrm );
	FS_Window *vwin = FS_WebGetMainWin( );
	FS_PushMsg *pMsg;
	
	wgt = FS_WindowGetFocusItem( lwin );
	if( wgt && wgt->private_data )
	{
		pMsg = (FS_PushMsg *)wgt->private_data;
		if( ! FS_PUSHMSG_GET_FLAG(pMsg, FS_PUSHMSG_READ) )
		{
			FS_PUSHMSG_SET_FLAG( pMsg, FS_PUSHMSG_READ );
			FS_PushSaveList( );
			FS_WidgetSetIcon( wgt, FS_I_PUSH_MSG );
		}
		FS_SetCurWebPageUrl( pMsg->url );
		FS_WebGoToUrl( vwin, pMsg->url, FS_FALSE );
		FS_DestroyWindow( lwin );
	}
	
	if( win && ( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_WebPushDel_CB( FS_Window *win )
{
	FS_Widget *wgt;
	FS_Window *lwin = FS_WindowFindId( FS_W_WebPushListFrm );
	FS_PushMsg *pMsg;
	
	wgt = FS_WindowGetFocusItem( lwin );
	if( wgt && wgt->private_data )
	{
		pMsg = (FS_PushMsg *)wgt->private_data;
		
		FS_WindowDelWidget( lwin, wgt );
		FS_PushDelItem( pMsg );
	}
	
	if( win && ( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
	
}

static FS_BOOL FS_WebPushDelAllCnf_CB( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	if( wparam == FS_EV_YES )
	{
		FS_Window *lwin = FS_WindowFindId( FS_W_WebPushListFrm );
		if( lwin )
		{
			FS_DestroyWindow( lwin );
		}
		FS_PushDelAll( );
		ret = FS_TRUE;
	}
	return ret;
}

static void FS_WebPushDelAll_CB( FS_Window *win )
{
	FS_Window *lwin = FS_WindowFindId( FS_W_WebPushListFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( lwin );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );

	if( wgt )
	{
		FS_MessageBox( FS_MS_YES_NO, FS_Text(FS_T_CONFIRM_DEL), FS_WebPushDelAllCnf_CB, FS_FALSE );
	}
}

static void FS_WebPushDetail_UI( FS_Window *win )
{
	FS_Window *lwin = FS_WindowFindId( FS_W_WebPushListFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( lwin );
	FS_CHAR *info, *strType;
	FS_SINT4 offset;
	FS_PushMsg *pMsg;
	
	if( wgt && wgt->private_data )
	{
		pMsg = (FS_PushMsg *)wgt->private_data;
		info = IFS_Malloc( FS_ITEM_DETAIL_LEN );
		offset = 0;
		if( info )
		{
			IFS_Sprintf( info + offset, "%s: %s\n", FS_Text(FS_T_WEB_URL), pMsg->url );
			offset = IFS_Strlen( info + offset );

			IFS_Sprintf( info + offset, "%s: %s\n", FS_Text(FS_T_SUBJECT), pMsg->subject );
			offset += IFS_Strlen( info + offset );

			IFS_Sprintf( info + offset, "%s: ", FS_Text(FS_T_DATE) );
			offset += IFS_Strlen( info + offset );
			FS_DateStruct2DispStr( info + offset, &pMsg->date );
			IFS_Strcat( info + offset, "\n" );
			offset += IFS_Strlen( info + offset );
			
			if( FS_PUSHMSG_GET_FLAG( pMsg, FS_PUSHMSG_TYPE_SL) )
				strType = FS_Text( FS_T_PUSH_SL );
			else
				strType = FS_Text( FS_T_PUSH_SI );
				
			IFS_Sprintf( info + offset, "%s: %s\n", FS_Text(FS_T_TYPE), strType );
			offset += IFS_Strlen( info + offset );
			
			FS_StdShowDetail( FS_Text(FS_T_DETAIL), info );
			IFS_Free( info );

			if( ! FS_PUSHMSG_GET_FLAG(pMsg, FS_PUSHMSG_READ) )
			{
				FS_PUSHMSG_SET_FLAG( pMsg, FS_PUSHMSG_READ );
				FS_PushSaveList( );
				FS_WidgetSetIcon( wgt, FS_I_PUSH_MSG );
			}
		}
	}
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_WebPushListMenu_UI( FS_Window *win )
{
	FS_Widget *wGoto, *wDelete, *wDetail, *wDelAll;
	FS_Window *pMenu;
	
	pMenu = FS_CreateMenu( FS_W_PushMsgListMenu, 4 );
	
	wGoto = FS_CreateMenuItem( 0, FS_Text(FS_T_GOTO) );
	wDetail = FS_CreateMenuItem( 0, FS_Text(FS_T_DETAIL) );	
	wDelete = FS_CreateMenuItem( 0, FS_Text(FS_T_DEL) ); 		
	wDelAll = FS_CreateMenuItem( 0, FS_Text(FS_T_DEL_ALL) ); 
	
	FS_MenuAddItem( pMenu, wDetail );
	FS_MenuAddItem( pMenu, wGoto );
	FS_MenuAddItem( pMenu, wDelete );
	FS_MenuAddItem( pMenu, wDelAll );
	
	FS_WidgetSetHandler( wGoto, FS_WebPushGoto_CB );
	FS_WidgetSetHandler( wDetail, FS_WebPushDetail_UI );
	FS_WidgetSetHandler( wDelete, FS_WebPushDel_CB );
	FS_WidgetSetHandler( wDelAll, FS_WebPushDelAll_CB );
	
	FS_MenuSetSoftkey( pMenu );
	FS_ShowWindow( pMenu );
}

static void FS_WebPushListBuild( FS_Window * lwin )
{
	FS_PushMsg *pMsg;
	FS_List *head, *node;
	FS_Widget *wgt;
	FS_CHAR *title;
	
	FS_WindowDelWidgetList( lwin );
	head = FS_PushGetList( );
	node = head->prev;
	while( node != head )
	{
		pMsg = FS_ListEntry( node, FS_PushMsg, list );
		node = node->prev;

		if( pMsg->subject[0] )
			title = pMsg->subject;
		else
			title = pMsg->url;
		if( FS_PUSHMSG_GET_FLAG( pMsg, FS_PUSHMSG_READ ) )
			wgt = FS_CreateListItem( 0, title, FS_NULL, FS_I_PUSH_MSG, 1 );
		else
			wgt = FS_CreateListItem( 0, title, FS_NULL, FS_I_NEW_NTF, 1 );
		
		wgt->private_data = (FS_UINT4)pMsg;
		FS_WidgetSetHandler( wgt, FS_WebPushGoto_CB );
		FS_WindowAddWidget( lwin, wgt );
	}
}

static void FS_WebPushList_UI( FS_Window *win )
{
	FS_Window *lwin;
	
	lwin = FS_CreateWindow( FS_W_WebPushListFrm, FS_Text(FS_T_WEB_PUSH), FS_NULL );
	lwin->show_index = FS_TRUE;

	FS_WebPushListBuild( lwin );
	
	FS_WindowSetSoftkey( lwin, 1, FS_Text(FS_T_MENU), FS_WebPushListMenu_UI );
	FS_WindowSetSoftkey( lwin, 2, FS_Text(FS_T_GOTO), FS_WebPushGoto_CB );
	FS_WindowSetSoftkey( lwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	
	FS_DestroyWindowByID( FS_W_WebMainMenu );
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
	
	FS_ShowWindow( lwin );
}

static void FS_OpenBookmark_CB( FS_Window *win )
{
	FS_Widget *wgt;
	FS_Window *lwin;
	FS_Window *vwin = FS_WebGetMainWin( );
	FS_Bookmark *bmk;

	lwin = FS_WindowFindId( FS_W_WebBookmarkDirFrm );
	if( lwin == FS_NULL ) lwin = FS_WindowFindId( FS_W_WebBookmarkListFrm );
	
	wgt = FS_WindowGetFocusItem( lwin );
	if( wgt && wgt->private_data )
	{
		bmk = (FS_Bookmark *)wgt->private_data;
		FS_SetCurWebPageUrl( bmk->url );
		FS_WebGoToUrl( vwin, bmk->url, FS_FALSE );
		FS_DestroyWindowByID( FS_W_WebBookmarkListFrm );
		FS_DestroyWindowByID( FS_W_WebBookmarkDirFrm );
	}
	
	if( win && ( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_OpenBookmarkDir_UI( FS_Window *win )
{
	FS_Widget *wgt;
	FS_Window *lwin = FS_WindowFindId( FS_W_WebBookmarkListFrm );
	FS_Window *dwin;
	FS_Bookmark *bmk, *dir;
	FS_List *head, *node;
	FS_CHAR *title;

	wgt = FS_WindowGetFocusItem( lwin );
	if( wgt && wgt->private_data )
	{
		dir = (FS_Bookmark *)wgt->private_data;
		
		dwin = FS_CreateWindow( FS_W_WebBookmarkDirFrm, FS_Text(FS_T_FAVORITE), FS_NULL );
		dwin->show_index = FS_TRUE;
		
		head = FS_BookmarkGetList( );
		node = head->next;
		while( node != head )
		{
			bmk = FS_ListEntry( node, FS_Bookmark, list );
			node = node->next;
			if( !FS_BMK_IS_DIR(bmk) && FS_BMK_OWN_TO_DIR(bmk, dir) )
			{
				if( bmk->title[0] )
					title = bmk->title;
				else
					title = bmk->url;
				
				wgt = FS_CreateListItem( 0, title, FS_NULL, FS_I_FILE, 1 );
				FS_WidgetSetHandler( wgt, FS_OpenBookmark_CB );
				wgt->private_data = (FS_UINT4)bmk;
				FS_WindowAddWidget( dwin, wgt );
			}
		}
		
		FS_WindowSetSoftkey( dwin, 1, FS_Text(FS_T_MENU), FS_BookmarkListMenu_UI );
		FS_WindowSetSoftkey( dwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
		FS_ShowWindow( dwin );
	}
	
	if( win && ( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_SaveBookmark_CB( FS_Window *win )
{
	FS_Window *lwin, *ewin = FS_WindowFindId( FS_W_WebBmkEditFrm );
	FS_CHAR *title, *url;
	FS_Bookmark *bmk;
	FS_Widget *wgt = FS_NULL;
	FS_SINT4 id = 0;
	
	title = FS_WindowGetWidgetText( ewin, FS_W_WebBmkTitle );
	url = FS_WindowGetWidgetText( ewin, FS_W_WebBmkUrl );

	if( FS_WindowGetWidget( ewin, FS_W_WebBmkUrl ) != (void *)FS_NULL )
	{
		if( url == FS_NULL || url[0] == 0 || IFS_Strchr(url, '.') == FS_NULL )
		{
			FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_INPUT_ALL_DATA_PLS), FS_NULL, FS_FALSE );
			return;
		}
	}
	else
	{
		if( title == FS_NULL || title[0] == 0 )
		{
			FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_INPUT_ALL_DATA_PLS), FS_NULL, FS_FALSE );
			return;
		}
	}
	
	bmk = ( FS_Bookmark * )ewin->private_data;
	if( bmk )
	{
		/* update a exist bookmark */
		lwin = FS_WindowFindId( FS_W_WebBookmarkDirFrm );
		if( lwin == FS_NULL ) lwin = FS_WindowFindId( FS_W_WebBookmarkListFrm );
		if( lwin ) wgt = FS_WindowGetFocusItem( lwin );

		if( url )
		{
			IFS_Strncpy( bmk->url, url, sizeof(bmk->url) - 2 );
		}
		if( title )
		{
			IFS_Strncpy( bmk->title, title, sizeof(bmk->title) - 2 );
			if( wgt ) FS_WidgetSetText( wgt, bmk->title );
		}
		else
		{
			bmk->title[0] = 0;
			if( wgt ) FS_WidgetSetText( wgt, bmk->url );
		}
		FS_BookmarkSave( );
	}
	else
	{
		lwin = FS_WindowFindId( FS_W_WebBookmarkDirFrm );
		if( lwin )
		{
			/* in a special bookmark directory */
			lwin = FS_WindowFindId( FS_W_WebBookmarkListFrm );
			wgt = FS_WindowGetFocusItem( lwin );
			if( wgt && wgt->private_data )
			{
				id = ((FS_Bookmark *)(wgt->private_data))->id;
			}
		}
		
		bmk = FS_BookmarkAddItem( title, url, id );
		lwin = FS_WindowFindId( FS_W_WebBookmarkDirFrm );
		if( lwin == FS_NULL ) lwin = FS_WindowFindId( FS_W_WebBookmarkListFrm );
		if( lwin )
		{
			/* if bookmark list win exist. we must refresh ui */
			if( url != FS_NULL )
			{
				if( title && title[0] )
					wgt = FS_CreateListItem( 0, title, FS_NULL, FS_I_FILE, 1 );
				else
					wgt = FS_CreateListItem( 0, url, FS_NULL, FS_I_FILE, 1 );
				FS_WidgetSetHandler( wgt, FS_OpenBookmark_CB );
			}
			else
			{
				wgt = FS_CreateListItem( 0, title, FS_NULL, FS_I_DIR, 1 );
				FS_WidgetSetHandler( wgt, FS_OpenBookmarkDir_UI );
			}
			wgt->private_data = (FS_UINT4)bmk;
			FS_WindowAddWidget( lwin, wgt );
		}
	}

	wgt = FS_WindowGetWidget( ewin, FS_W_WebBmkSetHome );
	if( wgt && FS_WGT_GET_CHECK(wgt) )
	{
		FS_WebConfigSetHomePage( url );
		FS_WebConfigSave( );
	}
	
	FS_DestroyWindow( ewin );
}

static void FS_BookmarkEdit_UI( FS_Bookmark *bmk, FS_BOOL bNew, FS_BOOL bDir )
{
	FS_Widget *eTitle, *eUrl, *wSetHome;
	FS_Window *editWin;
	FS_CHAR *title = FS_NULL, *url = "http://";
	FS_EditParam eParam = { FS_IM_CHI, FS_IM_ALL, FS_BM_TITLE_LEN - 1 };
	
	if( bmk )
	{
		title = bmk->title;
		url = bmk->url;
	}
	
	if( bNew )
	{
		if( bDir && FS_BookmarkGetDirCount( ) >= FS_MAX_BOOKMARK_DIR_NUM )
		{
			FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_DIR_FULL), FS_NULL, FS_FALSE );
			return;			
		}
		if( !bDir && FS_BookmarkGetCount( ) >= FS_MAX_BOOKMARK_NUM )
		{
			FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_LIST_FULL), FS_NULL, FS_FALSE );
			return;
		}
	}
	editWin = FS_CreateWindow( FS_W_WebBmkEditFrm, FS_Text(FS_T_FAVORITE), FS_NULL );
	eTitle = FS_CreateEditBox( FS_W_WebBmkTitle, title, FS_I_EDIT, 1, &eParam );
	FS_WindowAddWidget( editWin, eTitle );
	if( !bDir )
	{
		wSetHome = FS_CreateCheckBox( FS_W_WebBmkSetHome, FS_Text(FS_T_SET_HOME_PAGE) );
		eParam.preferred_method = FS_IM_ABC;
		eParam.allow_method = FS_IM_123 | FS_IM_ABC;
		eParam.max_len = FS_URL_LEN - 1;
		eUrl = FS_CreateEditBox( FS_W_WebBmkUrl, url, FS_I_EDIT, 3, &eParam );
		eTitle->tip = FS_Text(FS_T_WEB_BMK_TITLE);
		eUrl->tip = FS_Text(FS_T_WEB_BMK_URL);
		wSetHome->tip = FS_Text(FS_T_SET_HOME_PAGE);
		FS_WindowAddWidget( editWin, eUrl );
		FS_WindowAddWidget( editWin, wSetHome );
	}
	else
	{
		eTitle->tip = FS_Text(FS_T_DIR_NAME);
	}
	
	FS_WindowSetSoftkey( editWin, 1, FS_Text(FS_T_SAVE), FS_SaveBookmark_CB );
	FS_WindowSetSoftkey( editWin, 3, FS_Text(FS_T_BACK), FS_StdExitWithoutSaveCnfHandler );

	if( ! bNew ) editWin->private_data = (FS_UINT4)bmk;
	
	FS_ShowWindow( editWin );	
}

static void FS_NewBookmark_CB( FS_Window *win )
{
	if( win && ( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );

	FS_BookmarkEdit_UI( FS_NULL, FS_TRUE, FS_FALSE );
}

static void FS_NewBookmarkDir_CB( FS_Window *win )
{
	if( win && ( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );

	FS_BookmarkEdit_UI( FS_NULL, FS_TRUE, FS_TRUE );
}

static void FS_BookmarkMoveToFolder_CB( FS_Window *win )
{
	FS_Window *lwin;
	FS_Widget *wgt, *wSelect;
	FS_Bookmark *bmk;
	
	lwin = FS_WindowFindId( FS_W_WebBookmarkDirFrm );
	if( lwin == FS_NULL ) lwin = FS_WindowFindId( FS_W_WebBookmarkListFrm );
	wgt = FS_WindowGetFocusItem( lwin );
	bmk = (FS_Bookmark *)(wgt->private_data);
	
	wSelect = FS_WindowGetFocusItem( win );
	bmk->id = wSelect->id;
	FS_BookmarkSave( );
	
	FS_WindowDelWidget( lwin, wgt );
	lwin = FS_WindowFindId( FS_W_WebBookmarkListFrm );
	if( wSelect->id == 0 && lwin )	/* move to root folder */
	{
		if( bmk->title[0] )
			wgt = FS_CreateListItem( 0, bmk->title, FS_NULL, FS_I_FILE, 1 );
		else
			wgt = FS_CreateListItem( 0, bmk->url, FS_NULL, FS_I_FILE, 1 );
		wgt->private_data = (FS_UINT4)(bmk);
		FS_WidgetSetHandler( wgt, FS_OpenBookmark_CB );
		FS_WindowAddWidget( lwin,  wgt );
	}
	
	FS_DestroyWindowByID( FS_W_BookmarkListMenu );
	if( win && ( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );	
}

static void FS_BookmarkMoveToFolder_UI( FS_Window *win )
{
	FS_Widget *iFolder, *wgt;
	FS_Window *pMenu, *lwin;
	FS_Rect rect = { 0 };
	FS_SINT4 nItems;
	FS_UINT4 dirID;
	FS_Bookmark *bmk;
	FS_List *head, *node;
	
	rect.top = IFS_GetScreenHeight( ) / 2;
	rect.width = IFS_GetScreenWidth( );
	rect.height = IFS_GetLineHeight( );
	wgt = FS_WindowGetFocusItem( win );
	rect = FS_GetWidgetDrawRect( wgt );

	nItems = FS_BookmarkGetDirCount( );

	lwin = FS_WindowFindId( FS_W_WebBookmarkDirFrm );
	if( lwin == FS_NULL ) lwin = FS_WindowFindId( FS_W_WebBookmarkListFrm );
	wgt = FS_WindowGetFocusItem( lwin );
	
	if( wgt == FS_NULL || nItems <= 0 ) return;
	
	dirID = ((FS_Bookmark *)(wgt->private_data))->id;
	
	pMenu = FS_CreatePopUpMenu( 0, &rect, nItems );
	head = FS_BookmarkGetList( );
	node = head->next;
	while( node != head )
	{
		bmk = FS_ListEntry( node, FS_Bookmark, list );
		node = node->next;

		if( FS_BMK_IS_DIR(bmk) && bmk->id != dirID )
		{
			iFolder = FS_CreateMenuItem( bmk->id, bmk->title );
			FS_MenuAddItem( pMenu, iFolder );
			FS_WidgetSetHandler( iFolder, FS_BookmarkMoveToFolder_CB );
		}
	}
	if( dirID != 0 )	/* current not in root dir */
	{
		iFolder = FS_CreateMenuItem( 0, FS_Text(FS_T_ROOT_DIR) );
		FS_MenuAddItem( pMenu, iFolder );
		FS_WidgetSetHandler( iFolder, FS_BookmarkMoveToFolder_CB );
	}
	FS_MenuSetSoftkey( pMenu );
	FS_ShowWindow( pMenu );
}

static void FS_EditBookmark_CB( FS_Window *win )
{
	FS_Window *lwin;
	FS_Widget *focusWgt;
	FS_Bookmark *bmk = FS_NULL;

	lwin = FS_WindowFindId( FS_W_WebBookmarkDirFrm );
	if( lwin == FS_NULL ) lwin = FS_WindowFindId( FS_W_WebBookmarkListFrm );
	focusWgt = FS_WindowGetFocusItem( lwin );
	if( focusWgt )
		bmk = (FS_Bookmark *)focusWgt->private_data;
	
	if( win && ( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );

	if( bmk )
	{
		FS_BookmarkEdit_UI( bmk, FS_FALSE, FS_BMK_IS_DIR(bmk) );
	}
}

static void FS_BookmarkSetHome_CB( FS_Window *win )
{
	FS_Bookmark *bmk = FS_NULL;
	FS_Widget *wgt;
	FS_Window *lwin;

	lwin = FS_WindowFindId( FS_W_WebBookmarkDirFrm );
	if( lwin == FS_NULL ) lwin = FS_WindowFindId( FS_W_WebBookmarkListFrm );
	
	wgt = FS_WindowGetFocusItem( lwin );
	if( wgt && wgt->private_data )
	{
		bmk = (FS_Bookmark *)wgt->private_data;
		if( ! FS_BMK_IS_DIR(bmk) )
		{
			FS_WebConfigSetHomePage( bmk->url);
			FS_WebConfigSave( );
		}
	}
	
	if( win && ( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );

	if( bmk && ! FS_BMK_IS_DIR(bmk) )
	{
		FS_MessageBox( FS_MS_OK, FS_Text(FS_T_SUCCESS), FS_NULL, FS_TRUE );
	}
}

static FS_BOOL FS_DeleteBookmarkCnf_CB( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_Widget *wgt;
	FS_Window *lwin;
	FS_BOOL ret = FS_FALSE;

	if( wparam == FS_EV_YES )
	{
		lwin = FS_WindowFindId( FS_W_WebBookmarkDirFrm );
		if( lwin == FS_NULL ) lwin = FS_WindowFindId( FS_W_WebBookmarkListFrm );

		wgt = FS_WindowGetFocusItem( lwin );
		if( wgt && wgt->private_data )
		{
			FS_BookmarkDelItem( (FS_Bookmark *)wgt->private_data );
			FS_WindowDelWidget( lwin, wgt );
		}
		ret = FS_TRUE;
	}
	return ret;
}

static void FS_DeleteBookmark_CB( FS_Window *win )
{
	FS_Widget *wgt;
	FS_Window *lwin;
	
	if( win && ( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
	
	lwin = FS_WindowFindId( FS_W_WebBookmarkDirFrm );
	if( lwin == FS_NULL ) lwin = FS_WindowFindId( FS_W_WebBookmarkListFrm );

	wgt = FS_WindowGetFocusItem( lwin );
	if( wgt && wgt->private_data )
	{
		FS_MessageBox( FS_MS_YES_NO, FS_Text(FS_T_CONFIRM_DEL), FS_DeleteBookmarkCnf_CB, FS_FALSE );
	}
}

static void FS_BookmarkListMenu_UI( FS_Window *win )
{
	FS_Widget *wEdit, *wDelete, *wNew, *wSetHome = FS_NULL, *wNewDir = FS_NULL, *wMove = FS_NULL, *wgt;
	FS_Window *pMenu;
	FS_SINT4 nItems = 3;
	
	wDelete = FS_CreateMenuItem( 0, FS_Text(FS_T_DEL) ); 
	wNew = FS_CreateMenuItem( 0, FS_Text(FS_T_NEW_BMK) );	

	wgt = FS_WindowGetFocusItem( win );
	if( wgt && wgt->private_data && !FS_BMK_IS_DIR((FS_Bookmark *)(wgt->private_data)) )
	{
		nItems += 2;	/* plus two op: set home and move to folder */
		wEdit = FS_CreateMenuItem( 0, FS_Text(FS_T_EDIT) );
		wSetHome = FS_CreateMenuItem( 0, FS_Text(FS_T_SET_HOME_PAGE) );	
		wMove = FS_CreateMenuItem( 0, FS_Text(FS_T_MOVE_TO_FOLDER) );	
		FS_WGT_SET_SUB_MENU_FLAG( wMove );
	}
	else
	{
		wEdit = FS_CreateMenuItem( 0, FS_Text(FS_T_RENAME) );
	}
	
	if( win->id == FS_W_WebBookmarkListFrm )
	{
		nItems ++;	/* plus one op: new folder */
		wNewDir = FS_CreateMenuItem( 0, FS_Text(FS_T_NEW_DIR) );	
	}
	pMenu = FS_CreateMenu( FS_W_BookmarkListMenu, nItems );
	
	FS_MenuAddItem( pMenu, wNew );
	if( wNewDir ) FS_MenuAddItem( pMenu, wNewDir );
	if( wMove ) FS_MenuAddItem( pMenu, wMove );
	if( wSetHome ) FS_MenuAddItem( pMenu, wSetHome);
	FS_MenuAddItem( pMenu, wEdit );
	FS_MenuAddItem( pMenu, wDelete );

	if( wNewDir ) FS_WidgetSetHandler( wNewDir, FS_NewBookmarkDir_CB );
	if( wMove ) FS_WidgetSetHandler( wMove, FS_BookmarkMoveToFolder_UI );
	if( wSetHome ) FS_WidgetSetHandler( wSetHome, FS_BookmarkSetHome_CB );
	FS_WidgetSetHandler( wNew, FS_NewBookmark_CB );
	FS_WidgetSetHandler( wEdit, FS_EditBookmark_CB );
	FS_WidgetSetHandler( wDelete, FS_DeleteBookmark_CB );

	FS_MenuSetSoftkey( pMenu );
	FS_ShowWindow( pMenu );
}

static void FS_WebFavoriteList_UI( FS_Window *win )
{
	FS_Window *lwin;
	FS_Widget *wgt;
	FS_Bookmark *bmk;
	FS_List *head, *node;
	FS_CHAR *title;
	
	lwin = FS_CreateWindow( FS_W_WebBookmarkListFrm, FS_Text(FS_T_FAVORITE), FS_NULL );
	lwin->show_index = FS_TRUE;
	head = FS_BookmarkGetList( );
	node = head->next;
	while( node != head )
	{
		bmk = FS_ListEntry( node, FS_Bookmark, list );
		node = node->next;
		if( bmk->title[0] )
			title = bmk->title;
		else
			title = bmk->url;
		if( FS_BMK_IS_DIR(bmk) )
		{
			wgt = FS_CreateListItem( 0, title, FS_NULL, FS_I_DIR, 1 );
			FS_WidgetSetHandler( wgt, FS_OpenBookmarkDir_UI );
			wgt->private_data = (FS_UINT4)bmk;
			FS_WindowAddWidget( lwin, wgt );
		}
		else if(bmk->id == 0 )	/* not own to any directory */
		{
			wgt = FS_CreateListItem( 0, title, FS_NULL, FS_I_FILE, 1 );
			FS_WidgetSetHandler( wgt, FS_OpenBookmark_CB );
			wgt->private_data = (FS_UINT4)bmk;
			FS_WindowAddWidget( lwin, wgt );
		}
	}
	
	FS_WindowSetSoftkey( lwin, 1, FS_Text(FS_T_MENU), FS_BookmarkListMenu_UI );
	FS_WindowSetSoftkey( lwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	FS_ShowWindow( lwin );

	FS_DestroyWindowByID( FS_W_WebMainMenu );	
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_WebHistoryList_UI( FS_Window *win )
{
	FS_Window *lwin;
	FS_Widget *wgt;
	FS_HistoryEntry *he;
	FS_List *head, *node;
	FS_CHAR * ext, *title;
	
	lwin = FS_CreateWindow( FS_W_WebHistoryListFrm, FS_Text(FS_T_WEB_HISTORY), FS_NULL );
	lwin->show_index = FS_TRUE;
	head = FS_CacheGetList( );
	node = head->prev;
	while( node != head )
	{
		he = FS_ListEntry( node, FS_HistoryEntry, list );
		node = node->prev;
		ext = FS_GetFileExt( he->file );
		if( ext && (IFS_Stricmp(ext, "htm") == 0 || IFS_Stricmp(ext, "html") == 0
			|| IFS_Stricmp(ext, "wml") == 0 || IFS_Stricmp(ext, "bwml") == 0) )
		{
			if( he->title[0] )
				title = he->title;
			else
				title = he->url;
			wgt = FS_CreateListItem( 0, title, FS_NULL, FS_I_FILE, 1 );
			wgt->private_data = (FS_UINT4)he;
			FS_WidgetSetHandler( wgt, FS_HistoryItemDetail_UI );
			FS_WindowAddWidget( lwin, wgt );
		}
	}

	FS_WindowSetSoftkey( lwin, 1, FS_Text(FS_T_OPEN), FS_ViewHistoryFile_CB );
	FS_WindowSetSoftkey( lwin, 2, FS_Text(FS_T_DETAIL), FS_HistoryItemDetail_UI );
	FS_WindowSetSoftkey( lwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	
	FS_ShowWindow( lwin );
	FS_DestroyWindowByID( FS_W_WebMainMenu );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_OpenRecommandUrl_CB( FS_Window *win )
{
	FS_Widget *wgt;
	FS_Window *lwin;
	FS_Window *vwin = FS_WebGetMainWin( );
	FS_RecommandUrl *url;

	lwin = FS_WindowFindId( FS_W_WebRecmdDirFrm );
	
	wgt = FS_WindowGetFocusItem( lwin );
	if( wgt && wgt->private_data )
	{
		url = (FS_RecommandUrl *)wgt->private_data;
		FS_SetCurWebPageUrl( url->url );
		FS_WebGoToUrl( vwin, url->url, FS_FALSE );
		FS_DestroyWindowByID( FS_W_WebRecmdListFrm );
		FS_DestroyWindowByID( FS_W_WebRecmdDirFrm );
	}
	
	if( win && ( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_OpenRecommandDir_UI( FS_Window *win )
{
	FS_Widget *wgt;
	FS_Window *lwin = FS_WindowFindId( FS_W_WebRecmdListFrm );
	FS_Window *dwin;
	FS_RecommandUrlDir *dir;
	FS_RecommandUrl *url;
	FS_List *node;
	
	wgt = FS_WindowGetFocusItem( lwin );
	if( wgt && wgt->private_data )
	{
		dir = (FS_RecommandUrlDir *)wgt->private_data;
		
		dwin = FS_CreateWindow( FS_W_WebRecmdDirFrm, FS_Text(FS_T_RECOMMAND), FS_NULL );
		dwin->show_index = FS_TRUE;	
		
		FS_ListForEach( node, &dir->url_list ){
			url = FS_ListEntry( node, FS_RecommandUrl, list );
			if( url->title && url->url )
			{				
				wgt = FS_CreateListItem( 0, url->title, FS_NULL, FS_I_FILE, 1 );
				FS_WidgetSetHandler( wgt, FS_OpenRecommandUrl_CB );
				wgt->private_data = (FS_UINT4)url;
				FS_WindowAddWidget( dwin, wgt );
			}
		}

		FS_WindowSetSoftkey( dwin, 1, FS_Text(FS_T_CONNECT), FS_OpenRecommandUrl_CB );
		FS_WindowSetSoftkey( dwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
		FS_ShowWindow( dwin );
	}
}

static void FS_WebRecommandList_UI( FS_Window *win )
{
	FS_Window *lwin;
	FS_Widget *wgt;
	FS_RecommandUrlDir *dir;
	FS_List *dir_list, *node;
	
	lwin = FS_CreateWindow( FS_W_WebRecmdListFrm, FS_Text(FS_T_RECOMMAND), FS_NULL );
	lwin->show_index = FS_TRUE;
	
	dir_list = FS_RecmdUrlGetList( );
	FS_ListForEach( node, dir_list ){
		dir = FS_ListEntry( node, FS_RecommandUrlDir, list );
		if( dir->title )
		{
			wgt = FS_CreateListItem( 0, dir->title, FS_NULL, FS_I_DIR, 1 );
			FS_WidgetSetHandler( wgt, FS_OpenRecommandDir_UI );
			wgt->private_data = (FS_UINT4)dir;
			FS_WindowAddWidget( lwin, wgt );
		}
	}
	
	FS_WindowSetSoftkey( lwin, 1, FS_Text(FS_T_OPEN), FS_OpenRecommandDir_UI );
	FS_WindowSetSoftkey( lwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	FS_ShowWindow( lwin );

	FS_DestroyWindowByID( FS_W_WebMainMenu );	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_WebHomePage_CB( FS_Window *win )
{
#if 1
	FS_CHAR *url;
	FS_Window *mwin = FS_WebGetMainWin( );
	
	url = FS_WebConfigGetHomePage( );
	if( url[0] )
	{
		FS_SetCurWebPageUrl( url );
		FS_WebGoToUrl( mwin, url, FS_FALSE );	
	}
	else
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_PLS_SETHOMEPAGE), FS_NULL, FS_FALSE );
	}
#else
	FS_LoadWebDoc( FS_WebGetMainWin( ), "test.html", FS_NULL, FS_FALSE, FS_FALSE );
#endif
	FS_DestroyWindowByID( FS_W_WebMainMenu );
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_WebOpen_UI( FS_Window *win )
{
	FS_Widget *iHome, *iUrl, *iFavorite, *iHistory, *iRecommand, *iPush, *wgt;
	FS_Window *pMenu, *mwin;
	FS_Rect rect = { 0 };

	rect.top = IFS_GetScreenHeight( ) / 2;
	rect.width = IFS_GetScreenWidth( );
	rect.height = IFS_GetLineHeight( );
	mwin = FS_WindowFindId( FS_W_WebMainMenu );
	if( mwin )
	{
		wgt = FS_WindowGetFocusItem( mwin );
		rect = FS_GetWidgetDrawRect( wgt );
	}
	
	iHome = FS_CreateMenuItem( 0, FS_Text(FS_T_WEB_HOME_PAGE) );
	iUrl = FS_CreateMenuItem( 0, FS_Text(FS_T_GOTO_URL) );
	iFavorite = FS_CreateMenuItem( 0, FS_Text(FS_T_FAVORITE) );
	iRecommand = FS_CreateMenuItem( 0, FS_Text(FS_T_RECOMMAND) );
	iHistory = FS_CreateMenuItem( 0, FS_Text(FS_T_WEB_HISTORY) );
	iPush = FS_CreateMenuItem( 0, FS_Text(FS_T_WEB_PUSH) );

	pMenu = FS_CreatePopUpMenu( 0, &rect, 6 );
	
	FS_MenuAddItem( pMenu, iHome );
	FS_MenuAddItem( pMenu, iUrl );
	FS_MenuAddItem( pMenu, iFavorite );
	FS_MenuAddItem( pMenu, iHistory );
	FS_MenuAddItem( pMenu, iRecommand );
	FS_MenuAddItem( pMenu, iPush );
	
	FS_WidgetSetHandler( iHome, FS_WebHomePage_CB );
	FS_WidgetSetHandler( iUrl, FS_GotoUrl_UI );
	FS_WidgetSetHandler( iFavorite, FS_WebFavoriteList_UI );
	FS_WidgetSetHandler( iHistory, FS_WebHistoryList_UI );
	FS_WidgetSetHandler( iRecommand, FS_WebRecommandList_UI );
	FS_WidgetSetHandler( iPush, FS_WebPushList_UI );

	FS_MenuSetSoftkey( pMenu );
	FS_ShowWindow( pMenu );
}

static FS_BOOL FS_WebExitCnf_CB( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	if( wparam == FS_EV_YES )
	{
		FS_WebExit_CB( FS_NULL );
	}
	return ret;
}

void FS_WebPageGoStart_CB( FS_Window *win )
{
	FS_Window *mwin = FS_WebGetMainWin( );
	FS_WindowSetViewPort( mwin, IFS_GetWinTitleHeight() );
	FS_SetFocusToEyeableWebWgt( mwin );
	FS_InvalidateRect( win, &win->client_rect );
	FS_DestroyWindowByID( FS_W_WebMainMenu );
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

void FS_WebPageGoEnd_CB( FS_Window *win )
{
	FS_Window *mwin = FS_WebGetMainWin( );
	FS_WindowSetViewPort( mwin, -1 );
	FS_SetFocusToEyeableWebWgt( mwin );
	FS_DestroyWindowByID( FS_W_WebMainMenu );
	FS_InvalidateRect( win, &win->client_rect );
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

void FS_HistoryBack_CB( FS_Window *win )
{
	FS_Window *mwin = FS_WebGetMainWin( );
	FS_CHAR *file = FS_HistoryBack( );

	if( file )
	{
		FS_SetCurWebPageUrl( FS_HistoryCurUrl( ) );
		FS_LoadWebDoc( mwin, file, FS_NULL, FS_TRUE, FS_FALSE );
	}
	else
	{
		if( mwin->context.is_start_page )
		{
			FS_MessageBox( FS_MS_YES_NO, FS_Text(FS_T_CONFIRM_EXIT), FS_WebExitCnf_CB, FS_FALSE );
		}
		else
		{
			FS_WebStartPage( mwin );
			FS_InvalidateRect( mwin, FS_NULL );
		}
	}
	
	FS_DestroyWindowByID( FS_W_WebMainMenu );
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_HistoryForward_CB( FS_Window *win )
{
	FS_Window *mwin = FS_WebGetMainWin( );
	FS_CHAR *file;

	if( mwin->context.is_start_page )
	{
		file = FS_HistoryCurFile( );
	}
	else
	{
		file = FS_HistoryForward( );
	}
	
	if( file )
	{
		FS_SetCurWebPageUrl( FS_HistoryCurUrl( ) );
		FS_LoadWebDoc( mwin, file, FS_NULL, FS_TRUE, FS_FALSE );
	}

	FS_DestroyWindowByID( FS_W_WebMainMenu );
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_WebRefresh_CB( FS_Window *win )
{
	FS_Window *mwin = FS_WebGetMainWin( );
	FS_CHAR *url = FS_GetCurWebPageUrl( );

	if( url ) FS_WebGoToUrl( mwin, url, FS_FALSE );

	
	FS_DestroyWindowByID( FS_W_WebMainMenu );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_WebNavigate_UI( FS_Window *win )
{
	FS_Widget *iPrev, *iRefresh, *iBack, *iGoHome, *iGoEnd, *wgt;
	FS_Window *pMenu, *mwin;
	FS_Rect rect = { 0 };

	rect.top = IFS_GetScreenHeight( ) / 2;
	rect.width = IFS_GetScreenWidth( );
	rect.height = IFS_GetLineHeight( );
	mwin = FS_WindowFindId( FS_W_WebMainMenu );
	if( mwin )
	{
		wgt = FS_WindowGetFocusItem( mwin );
		rect = FS_GetWidgetDrawRect( wgt );
	}

	iPrev = FS_CreateMenuItem( 0, FS_Text(FS_T_WEB_PREV) );
	iRefresh = FS_CreateMenuItem( 0, FS_Text(FS_T_REFRESH) );
	iBack = FS_CreateMenuItem( 0, FS_Text(FS_T_FALL_BACK) );
	iGoHome = FS_CreateMenuItem( 0, FS_Text(FS_T_GO_HOME) );
	iGoEnd = FS_CreateMenuItem( 0, FS_Text(FS_T_GO_END) );
	
	pMenu = FS_CreatePopUpMenu( 0, &rect, 5 );
	
	FS_MenuAddItem( pMenu, iPrev );
	FS_MenuAddItem( pMenu, iRefresh );
	FS_MenuAddItem( pMenu, iBack );
	FS_MenuAddItem( pMenu, iGoHome );
	FS_MenuAddItem( pMenu, iGoEnd );
	
	FS_WidgetSetHandler( iPrev, FS_HistoryForward_CB );
	FS_WidgetSetHandler( iRefresh, FS_WebRefresh_CB );
	FS_WidgetSetHandler( iBack, FS_HistoryBack_CB );
	FS_WidgetSetHandler( iGoHome, FS_WebPageGoStart_CB );
	FS_WidgetSetHandler( iGoEnd, FS_WebPageGoEnd_CB );

	FS_MenuSetSoftkey( pMenu );
	FS_ShowWindow( pMenu );
}

static void FS_SaveCurPageUrl_CB( FS_Window *win )
{
	FS_Bookmark bmk;
	FS_CHAR *curUrl = FS_GetCurWebPageUrl( );
	FS_Window *mwin = FS_WebGetMainWin( );

	FS_DestroyWindowByID( FS_W_WebMainMenu );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );

	if( curUrl && mwin )
	{
		IFS_Memset( &bmk, 0, sizeof(FS_Bookmark) );
		if(mwin->context.title) 
			IFS_Strncpy( bmk.title, mwin->context.title, sizeof(bmk.title) - 2 );
		IFS_Strncpy( bmk.url, curUrl, sizeof(bmk.url) - 1 );
		FS_BookmarkEdit_UI( &bmk, FS_TRUE, FS_FALSE );
	}
}

static void FS_WebSavePageHandle( FS_CHAR * path, void *param )
{
	FS_BOOL ret;
	FS_SINT4 len;
	FS_CHAR *str, *file = FS_NULL, *url = FS_GetCurWebPageUrl( );
	
	if( url ) file = FS_CacheFindFile( url );

	if( path && path[0] )
	{
		len = IFS_Strlen( path );
		ret = FS_FileCopy( FS_DIR_WEB, file, -1, path );
		if( ret )
		{
			str = IFS_Malloc( len + 32 );
			IFS_Sprintf( str, "%s %s", FS_Text(FS_T_SAVED_TO), path );
			FS_MessageBox( FS_MS_OK, str, FS_NULL, FS_TRUE );
			IFS_Free( str );
		}
		else
		{
			str = FS_Text( FS_T_SAVE_FILE_FAILED );
			FS_MessageBox( FS_MS_ALERT, str, FS_NULL, FS_TRUE );
		}
	}
}

static void FS_SaveCurPage_CB( FS_Window *win )
{
	FS_CHAR *file = FS_NULL, *url = FS_GetCurWebPageUrl( );
	FS_DestroyWindowByID( FS_W_WebMainMenu );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );

	if( url ) file = FS_CacheFindFile( url );
	
	if( file ) IFS_FileDialogSave( file, FS_WebSavePageHandle, FS_NULL );
}

static void FS_SaveImage_HD( FS_CHAR * path, void *param )
{
	FS_SINT4 len;
	FS_BOOL ret;
	FS_Window *lwin = FS_WindowFindId( FS_W_WebImageListFrm );
	FS_Widget *wgt;
	FS_CHAR *str;
	
	wgt = FS_WindowGetFocusItem( lwin );
	
	if( path && path[0] )
	{
		len = IFS_Strlen( path );
		ret = FS_FileCopy( FS_DIR_WEB, wgt->data, -1, path );
		if( ret )
		{
			str = IFS_Malloc( len + 32 );
			IFS_Sprintf( str, "%s %s", FS_Text(FS_T_SAVED_TO), path );
			FS_MessageBox( FS_MS_OK, str, FS_NULL, FS_FALSE );
			IFS_Free( str );
		}
		else
		{
			str = FS_Text( FS_T_SAVE_FILE_FAILED );
			FS_MessageBox( FS_MS_ALERT, str, FS_NULL, FS_FALSE );
		}
	}
}

static void FS_SaveImage_CB( FS_Window *win )
{
	FS_Window *lwin = FS_WindowFindId( FS_W_WebImageListFrm );
	FS_Widget *wgt;

	wgt = FS_WindowGetFocusItem( lwin );
	if( wgt )
	{
		IFS_FileDialogSave( wgt->text, FS_SaveImage_HD, FS_NULL );
	}
}

static void FS_ViewImage_UI( FS_Window *win )
{
	FS_Window *vwin;
	FS_Window *lwin = FS_WindowFindId( FS_W_WebImageListFrm );
	FS_Widget *wgt;
	FS_WebWgt *wwgt;
	FS_CHAR filename[FS_MAX_PATH_LEN];
	
	wgt = FS_WindowGetFocusItem( lwin );
	if( wgt )
	{
		vwin = FS_WebCreateWin( FS_W_WebImageViewFrm, FS_Text(FS_T_VIEW), FS_NULL );
		wwgt = FS_CreateWebWgt( FS_WWT_IMAGE, FS_NULL, FS_NULL, FS_NULL, FS_NULL );
		FS_GetAbsFileName( FS_DIR_WEB, wgt->data, filename );
		wwgt->file = IFS_Strdup( filename );
		FS_WWGT_SET_IMAGE(wwgt);
		FS_WebWinAddWebWgt( vwin, wwgt, 1 );

		FS_WindowSetSoftkey( vwin, 1, FS_Text(FS_T_SAVE), FS_SaveImage_CB );
		FS_WindowSetSoftkey( vwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
		FS_ShowWindow( vwin );
	}
}

static void FS_WebImageList_UI( FS_Window *win )
{
	FS_Window *lwin, *mwin;
	FS_Widget *wgt;
	FS_List *node;
	FS_WebImage *img;

	FS_DestroyWindowByID( FS_W_WebMainMenu );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );

	mwin = FS_WebGetMainWin( );
	if( FS_ListIsEmpty( &mwin->context.image_list ) )
	{
		FS_MessageBox( FS_MS_OK, FS_Text(FS_T_NO_IMAGE), FS_NULL, FS_TRUE );
		return;
	}
	
	lwin = FS_CreateWindow( FS_W_WebImageListFrm, FS_Text(FS_T_SAVE_IMAGE), FS_NULL );
	node = mwin->context.image_list.next;
	while( node != &mwin->context.image_list )
	{
		img = FS_ListEntry( node, FS_WebImage, list );
		node = node->next;
		if( img->dname[0] )
			wgt = FS_CreateListItem( 0, img->dname, FS_NULL, FS_I_IMAGE, 1 );
		else
			wgt = FS_CreateListItem( 0, img->file, FS_NULL, FS_I_IMAGE, 1 );
		wgt->data = IFS_Strdup( img->file );
		FS_WidgetSetHandler( wgt, FS_ViewImage_UI );
		FS_WindowAddWidget( lwin, wgt );
	}

	FS_WindowSetSoftkey( lwin, 1, FS_Text(FS_T_SAVE), FS_SaveImage_CB );
	FS_WindowSetSoftkey( lwin, 2, FS_Text(FS_T_VIEW), FS_NULL );
	FS_WindowSetSoftkey( lwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	
	FS_ShowWindow( lwin );
}

static void FS_SaveBgSound_HD( FS_CHAR * path, void *param )
{
	FS_SINT4 len;
	FS_BOOL ret;
	FS_Window *mwin;
	FS_CHAR *str;
	FS_WebSound *pSnd;
	
	if( path && path[0] )
	{
		mwin = FS_WebGetMainWin( );
		pSnd = &mwin->context.bgsound;
		len = IFS_Strlen( path );
		ret = FS_FileCopy( FS_DIR_WEB, pSnd->file, -1, path );
		if( ret )
		{
			str = IFS_Malloc( len + 32 );
			IFS_Sprintf( str, "%s %s", FS_Text(FS_T_SAVED_TO), path );
			FS_MessageBox( FS_MS_OK, str, FS_NULL, FS_FALSE );
			IFS_Free( str );
		}
		else
		{
			str = FS_Text( FS_T_SAVE_FILE_FAILED );
			FS_MessageBox( FS_MS_ALERT, str, FS_NULL, FS_FALSE );
		}
	}
}


static void FS_WebSaveBgSound_CB( FS_Window *win )
{
	FS_Window *mwin;

	FS_DestroyWindowByID( FS_W_WebMainMenu );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );

	mwin = FS_WebGetMainWin( );
	if( mwin->context.bgsound.file[0] == 0 )
	{
		FS_MessageBox( FS_MS_OK, FS_Text(FS_T_NO_AUDIO), FS_NULL, FS_TRUE );
		return;
	}
	if( mwin->context.bgsound.is_playing )
	{
		IFS_StopAudio( );
		mwin->context.bgsound.is_playing = FS_FALSE;
	}

	IFS_FileDialogSave( mwin->context.bgsound.dname, FS_SaveBgSound_HD, FS_NULL );
}

static void FS_WebSaveContent_UI( FS_Window *win )
{
	FS_Widget *wSaveUrl, *wSavePage, *wSaveImage, *wSaveAudio, *wgt;
	FS_Window *pMenu, *mwin;
	FS_Rect rect = { 0 };

	rect.top = IFS_GetScreenHeight( ) / 2;
	rect.width = IFS_GetScreenWidth( );
	rect.height = IFS_GetLineHeight( );
	mwin = FS_WindowFindId( FS_W_WebMainMenu );
	if( mwin )
	{
		wgt = FS_WindowGetFocusItem( mwin );
		rect = FS_GetWidgetDrawRect( wgt );
	}
	
	wSaveUrl = FS_CreateMenuItem( 0, FS_Text(FS_T_WEB_SAVE_URL) );
	wSavePage = FS_CreateMenuItem( 0, FS_Text(FS_T_WEB_SAVE_PAGE) );
	wSaveImage = FS_CreateMenuItem( 0, FS_Text(FS_T_SAVE_IMAGE) );
	wSaveAudio = FS_CreateMenuItem( 0, FS_Text(FS_T_SAVE_AUDIO) );
	
	pMenu = FS_CreatePopUpMenu( FS_W_WebSaveContentMenu, &rect, 4 );
	FS_MenuAddItem( pMenu, wSaveUrl );
	FS_MenuAddItem( pMenu, wSavePage );
	FS_MenuAddItem( pMenu, wSaveImage );
	FS_MenuAddItem( pMenu, wSaveAudio );
	
	FS_WidgetSetHandler( wSaveUrl, FS_SaveCurPageUrl_CB );
	FS_WidgetSetHandler( wSavePage, FS_SaveCurPage_CB );
	FS_WidgetSetHandler( wSaveImage, FS_WebImageList_UI );
	FS_WidgetSetHandler( wSaveAudio, FS_WebSaveBgSound_CB );
	
	FS_MenuSetSoftkey( pMenu );
	FS_ShowWindow( pMenu );
}

/* 显示起始页面 */
static void FS_WebStartPage( FS_Window *win )
{
	FS_WebWgt *wwgt;
	FS_CHAR fullname[128];
	FS_List *head, *node;
	FS_HistoryEntry *he;
	FS_CHAR *ext, *title;
	FS_SINT4 num = 0;
	FS_CHAR strscript[128];
	
	FS_ClearWebWinContext( win );
	
	/* logo */
	wwgt = FS_WwCreateImage( FS_NULL, FS_NULL, FS_NULL );
	FS_WWGT_SET_IMAGE(wwgt);
	FS_WWGT_SET_MID_ALIGN(wwgt);
	FS_GetAbsFileName(FS_DIR_ROOT, "sfox_logo.bmp", fullname);
	wwgt->file = IFS_Strdup( fullname );
	FS_WebWinAddWebWgt( win, wwgt, 0 );
	/* search */
	wwgt = FS_WwCreateText( FS_Text(FS_T_QUICK_SEARCH) );
	FS_WebWinAddWebWgt( win, wwgt, 1);
	wwgt = FS_WwCreateInput( "input", FS_NULL );
	wwgt->max_len = 64;
	FS_WebWinAddWebWgt( win, wwgt, 1 );
	wwgt = FS_WwCreateLink( FS_Text(FS_T_BAIDU), "sfox::baidu(input)" );
	FS_WebWinAddWebWgt( win, wwgt, 1 );
	wwgt = FS_WwCreateLink( FS_Text(FS_T_GOOGLE), "sfox::google(input)" );
	FS_WebWinAddWebWgt( win, wwgt, 0 );
	/* quick lunch */
	wwgt = FS_WwCreateText( FS_Text(FS_T_QUICK_START) );
	FS_WebWinAddWebWgt( win, wwgt, 2 );
	wwgt = FS_WwCreateLink( FS_Text(FS_T_GOTO_URL), "sfox::gotourl" );
	FS_WebWinAddWebWgt( win, wwgt, 1 );	
	wwgt = FS_WwCreateLink( FS_Text(FS_T_FAVORITE), "sfox::favorite" );
	FS_WebWinAddWebWgt( win, wwgt, 0 );
	wwgt = FS_WwCreateLink( FS_Text(FS_T_RECOMMAND), "sfox::recommand" );
	FS_WebWinAddWebWgt( win, wwgt, 0 );
	wwgt = FS_WwCreateLink( FS_Text(FS_T_WEB_HISTORY), "sfox::history" );
	FS_WebWinAddWebWgt( win, wwgt, 0 );	
	/* history list */
	wwgt = FS_WwCreateText( FS_Text(FS_T_RECENT_VISIT) );
	FS_WebWinAddWebWgt( win, wwgt, 2 );
	
	head = FS_CacheGetList( );
	node = head->prev;
	while( node != head && num < 10 )
	{
		he = FS_ListEntry( node, FS_HistoryEntry, list );
		node = node->prev;
		ext = FS_GetFileExt( he->file );
		if( ext && (IFS_Stricmp(ext, "htm") == 0 || IFS_Stricmp(ext, "html") == 0
			|| IFS_Stricmp(ext, "wml") == 0 || IFS_Stricmp(ext, "bwml") == 0) )
		{
			if( he->title[0] )
				title = he->title;
			else
				title = he->url;
			IFS_Memset( fullname, 0, sizeof(fullname) );
			IFS_Strcpy( fullname, ">>" );
			IFS_Strncpy( fullname + 2, title, sizeof(fullname) - 4 );
			IFS_Sprintf( strscript, "sfox::viewcache(%s)", he->file );
			wwgt = FS_WwCreateLink( fullname, strscript );
			FS_WebWinAddWebWgt( win, wwgt, 1 );
			num ++;
		}
	}

	FS_WindowSetTitle( win, FS_Text(FS_T_WEB) );
	win->context.is_start_page = FS_TRUE;
}

void FS_WebMainMenu_UI( FS_Window *win )
{
	FS_Widget *itemOpen, *itemOption, *itemExit, *itemNavigate, *itemSave;
	FS_Window *pMenu;
	itemOpen = FS_CreateMenuItem( 0, FS_Text(FS_T_OPEN) );
	itemOption = FS_CreateMenuItem( 0, FS_Text(FS_T_OPTION) );	
	itemExit = FS_CreateMenuItem( 0, FS_Text(FS_T_EXIT) );
	itemSave = FS_CreateMenuItem( 0, FS_Text(FS_T_SAVE) );
	itemNavigate = FS_CreateMenuItem( 0, FS_Text(FS_T_Navigate) );
	
	pMenu = FS_CreateMenu( FS_W_WebMainMenu, 5 );
	FS_MenuAddItem( pMenu, itemOpen );
	FS_MenuAddItem( pMenu, itemNavigate );
	FS_MenuAddItem( pMenu, itemSave );
	FS_MenuAddItem( pMenu, itemOption );
	FS_MenuAddItem( pMenu, itemExit );

	FS_WidgetSetHandler( itemOpen, FS_WebOpen_UI );
	FS_WGT_SET_SUB_MENU_FLAG( itemOpen );
	FS_WidgetSetHandler( itemOption, FS_WebSetting_UI );
	FS_WGT_SET_SUB_MENU_FLAG( itemOption );
	FS_WidgetSetHandler( itemExit, FS_WebExit_CB );
	FS_WidgetSetHandler( itemNavigate, FS_WebNavigate_UI );
	FS_WGT_SET_SUB_MENU_FLAG( itemNavigate );
	FS_WidgetSetHandler( itemSave, FS_WebSaveContent_UI );
	FS_WGT_SET_SUB_MENU_FLAG( itemSave );
	FS_MenuSetSoftkey( pMenu );
	FS_ShowWindow( pMenu );	
}

void FS_WebMain( void )
{
	FS_Window *win;
	
	FS_WebSysInit( );
	FS_ActiveApplication( FS_APP_WEB );
	
	win = FS_WebCreateWin( FS_W_WebMainFrm, FS_Text(FS_T_WEB), FS_NULL );
	FS_WebStartPage( win );
	
	FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_MENU), FS_WebMainMenu_UI );
	FS_WindowSetSoftkey( win, 3, FS_Text(FS_T_FALL_BACK), FS_HistoryBack_CB );
	FS_ShowWindow( win );
}

void FS_WebExit( void )
{
	FS_WebExit_CB( FS_NULL );
}

static FS_CHAR *FS_WebGetSCDetail( FS_WebShortCut sc, FS_WidgetEventHandler *pEvHandler )
{
	FS_WidgetEventHandler evh;
	FS_CHAR *ret;
	switch( sc )
	{
		case FS_WSC_HOME_PAGE:
			ret = FS_Text(FS_T_WEB_HOME_PAGE);
			evh = FS_WebHomePage_CB;
			break;
		case FS_WSC_FAVORITE:
			ret = FS_Text(FS_T_FAVORITE);
			evh = FS_WebFavoriteList_UI;
			break;
		case FS_WSC_RECOMMAND:
			ret = FS_Text(FS_T_RECOMMAND);
			evh = FS_WebRecommandList_UI;
			break;
		case FS_WSC_GOTO_URL:
			ret = FS_Text( FS_T_GOTO_URL );
			evh = FS_GotoUrl_UI;
			break;
		case FS_WSC_HISTORY:
			ret = FS_Text( FS_T_WEB_HISTORY );
			evh = FS_WebHistoryList_UI;
			break;
		case FS_WSC_SAVE_URL:
			ret = FS_Text( FS_T_WEB_SAVE_URL );
			evh = FS_SaveCurPageUrl_CB;
			break;
		case FS_WSC_GEN_SETTING:
			ret = FS_Text( FS_T_WEB_GENERAL );
			evh = FS_WebGeneralSetting_UI;
			break;
		case FS_WSC_NET_SETTING:
			ret = FS_Text( FS_T_CONN_SET );
			evh = FS_WebConnSetting_UI;
			break;
		case FS_WSC_SHORTCUT_SETTING:
			ret = FS_Text( FS_T_WEB_SC );
			evh = FS_WebShortCutSetting_UI;
			break;
		case FS_WSC_FORWARD:
			ret = FS_Text( FS_T_WEB_PREV );
			evh = FS_HistoryForward_CB;
			break;
		case FS_WSC_REFRESH:
			ret = FS_Text( FS_T_REFRESH );
			evh = FS_WebRefresh_CB;
			break;
		case FS_WSC_BACKWARD:
			ret = FS_Text( FS_T_WEB_BACK );
			evh = FS_HistoryBack_CB;
			break;
		case FS_WSC_PAGE_START:
			ret = FS_Text( FS_T_GO_HOME );
			evh = FS_WebPageGoStart_CB;
			break;
		case FS_WSC_PAGE_END:
			ret = FS_Text( FS_T_GO_END );
			evh = FS_WebPageGoEnd_CB;
			break;
		default:
			ret = FS_Text( FS_T_NONE );
			evh = FS_NULL;
			break;
	}

	if( pEvHandler ) *pEvHandler = evh;
	return ret;
}

FS_BOOL FS_WebHandleShortCutKey( FS_Window *win, FS_SINT4 vkeyCode )
{
	FS_WebShortCut sc;
	FS_WidgetEventHandler evHandler = FS_NULL;
	/* when is requesting a web page. we will ignore the shortcut key */
	if( FS_WebNetBusy( ) ) return FS_FALSE;
	if( ! FS_WebConfigGetShortCutEnableFlag() ) return FS_FALSE;
	
	sc = FS_WebConfigGetShortCut( vkeyCode );
	FS_WebGetSCDetail( sc, &evHandler );
	if( evHandler ) evHandler( win );
	return FS_TRUE;
}

static FS_WebSearch( FS_Window *win, FS_CHAR *strscript, FS_CHAR *input )
{
	FS_SINT4 len;
	FS_CHAR *url = IFS_Malloc( 2000 );	
	FS_CHAR *word = FS_FindWebWgtValue( win, input );
	if( word != FS_NULL )
	{
		if( IFS_Strnicmp(strscript, "baidu", 5) == 0 )
		{
			IFS_Strcpy( url, "http://wap.baidu.com/baidu?word=" );
			len = IFS_Strlen(url);
			FS_UrlEncode( url + len, 2000 - len, word, -1 );
			FS_SetCurWebPageUrl( url );
			FS_WebGoToUrl( win, url, FS_NULL );
		}
		else if( IFS_Strnicmp(strscript, "google", 6) == 0 )
		{
			IFS_Strcpy( url, "http://www.google.cn/wml/search?q=" );
			len = IFS_Strlen( url );
			FS_UrlEncode( url + len, 2000 - len, word, -1 );
			FS_SetCurWebPageUrl( url );
			FS_WebGoToUrl( win, url, FS_NULL );
		}
	}
	IFS_Free( url );
}

static void FS_UrlInputScript( FS_CHAR *str )
{
	FS_Window *win = FS_WindowFindId( FS_W_WebUrlInput );
	FS_WebWgt *wwgt = FS_FindWebWgt( win, "url" );
	FS_CHAR *url;
	if( IFS_Strcmp( str, "clear" ) != 0 )
	{
		url = FS_StrConCat( wwgt->text, str, FS_NULL, FS_NULL );
		FS_COPY_TEXT( wwgt->text, url );
		IFS_Free( url );
	}
	else
	{
		FS_SAFE_FREE( wwgt->text );
	}
	FS_RedrawWebWgt( wwgt );
}

FS_BOOL FS_SFoxWebScript( FS_CHAR *strscript )
{
	FS_CHAR *str1, *str2;
	FS_CHAR str[64];
	FS_HistoryEntry *he;
	FS_Window *win = FS_WindowFindId( FS_W_WebMainFrm );
	if( IFS_Strnicmp(strscript, "sfox::", 6) != 0 || win == FS_NULL ) return FS_FALSE;

	strscript = strscript + 6;
	if( IFS_Strnicmp(strscript, "favorite", 8) == 0 )
	{
		FS_WebFavoriteList_UI( win );
	}
	else if( IFS_Strnicmp(strscript, "gotourl", 8) == 0 )
	{
		FS_GotoUrl_UI( win );
	}
	else if( IFS_Strnicmp(strscript, "recommand", 9) == 0 )
	{
		FS_WebRecommandList_UI( win);
	}
	else if( IFS_Strnicmp(strscript, "history", 7) == 0 )
	{
		FS_WebHistoryList_UI( win );
	}
	else if( IFS_Strnicmp(strscript, "viewcache", 9) == 0 )
	{
		str1 = IFS_Strchr( strscript, '(' );
		str1 ++;
		str2 = IFS_Strchr( strscript, ')' );
		IFS_Memset( str, 0, sizeof(str) );
		IFS_Strncpy( str, str1, str2 - str1 );
		he = FS_CacheFindEntryByFile( str );
		if( he )
		{
			FS_HistoryDeinit( );
			FS_HistorySetCurrent( he->url );
			FS_SetCurWebPageUrl( he->url );
			FS_LoadWebDoc( win, str, FS_NULL, FS_FALSE, FS_TRUE );
		}
	}
	else if( IFS_Strnicmp(strscript, "baidu", 5) == 0 || IFS_Strnicmp(strscript, "google", 6) == 0 )
	{
		str1 = IFS_Strchr( strscript, '(' );
		str1 ++;
		str2 = IFS_Strchr( strscript, ')' );
		IFS_Memset( str, 0, sizeof(str) );
		IFS_Strncpy( str, str1, str2 - str1 );
		FS_WebSearch( win, strscript, str );
	}
	else if( IFS_Strnicmp(strscript, "urlinput", 8) == 0 )
	{
		str1 = IFS_Strchr( strscript, '(' );
		str1 ++;
		str2 = IFS_Strchr( strscript, ')' );
		IFS_Memset( str, 0, sizeof(str) );
		IFS_Strncpy( str, str1, str2 - str1 );
		FS_UrlInputScript( str );
	}
	return FS_TRUE;
}

void FS_WebOpenPushList( void )
{
	FS_WebSysInit( );
	FS_ActiveApplication( FS_APP_WEB );

	FS_WebPushList_UI( FS_NULL );
}

void FS_WebOpenBookmarkList( void )
{
	FS_WebSysInit( );
	FS_ActiveApplication( FS_APP_WEB );

	FS_WebFavoriteList_UI( FS_NULL );
}

void FS_WebOpenUrl( FS_CHAR *url )
{
	FS_Window *win;
	
	FS_WebSysInit( );
	FS_ActiveApplication( FS_APP_WEB );

	win = FS_WebCreateWin( FS_W_WebMainFrm, FS_Text(FS_T_WEB), FS_NULL );	
	FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_MENU), FS_WebMainMenu_UI );
	FS_WindowSetSoftkey( win, 3, FS_Text(FS_T_FALL_BACK), FS_HistoryBack_CB );
	FS_ShowWindow( win );
	
	FS_SetCurWebPageUrl( url );
	FS_WebGoToUrl( win, url, FS_FALSE );
}

static FS_BOOL FS_WebLoadWebDocWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	if( cmd == FS_WM_DESTROY )
	{
		FS_CHAR *docFile = (FS_CHAR *)win->private_data;
		FS_ASSERT( docFile );
		if( docFile ){
			FS_FileDelete( FS_DIR_WEB, docFile );
			IFS_Free( docFile );
		}
		ret = FS_TRUE;
	}
	return ret;
}

void FS_WebLoadWebDoc( FS_CHAR *file )
{
	FS_Window *win;
	FS_CHAR docFile[FS_FILE_NAME_LEN], *ext;
	
	ext = FS_GetFileExt( file );
	if( ! FS_STR_I_EQUAL(ext, "wml") && ! FS_STR_I_EQUAL(ext, "htm") && ! FS_STR_I_EQUAL(ext, "html") ){
		return;
	}
	FS_GetGuid( docFile );
	IFS_Strcat( docFile, "." );
	IFS_Strcat( docFile, ext );
	if( ! FS_FileCopy( -1, file, FS_DIR_WEB, docFile ) ){
		return;
	}
	
	FS_WebSysInit( );
	FS_ActiveApplication( FS_APP_WEB );

	win = FS_WebCreateWin( FS_W_WebMainFrm, FS_Text(FS_T_WEB), FS_WebLoadWebDocWndProc );	
	FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_MENU), FS_WebMainMenu_UI );
	FS_WindowSetSoftkey( win, 3, FS_Text(FS_T_FALL_BACK), FS_HistoryBack_CB );
	FS_ShowWindow( win );

	win->private_data = (FS_UINT4)IFS_Strdup( docFile );
	FS_LoadWebDoc( win, docFile, FS_NULL, FS_FALSE, FS_FALSE );
}

void FS_WebOpenHomePage( void )
{	
	FS_Window *win;
	
	FS_WebSysInit( );
	FS_ActiveApplication( FS_APP_WEB );
	
	win = FS_WebCreateWin( FS_W_WebMainFrm, FS_Text(FS_T_WEB), FS_NULL );	
	FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_MENU), FS_WebMainMenu_UI );
	FS_WindowSetSoftkey( win, 3, FS_Text(FS_T_FALL_BACK), FS_HistoryBack_CB );
	FS_ShowWindow( win );
	
	FS_WebHomePage_CB( FS_NULL );
}

/* we receive a new push msg. update push msg list or any, and inform user */
void FS_NewPushMsgNotify( FS_PushMsg *pMsg )
{
	/* rebuild push msg list if any */
	FS_Window *win;

	win = FS_WindowFindId( FS_W_WebPushListFrm );
	if( win )
	{
		FS_WebPushListBuild( win );
		FS_GuiRepaint( );
		IFS_PushNotify( pMsg->subject, pMsg->url );
	}
}

#endif	//FS_MODULE_WEB


