#include "inc/FS_Config.h"

#ifdef FS_MODULE_DCD

#include "inc/inte/FS_Inte.h"
#include "inc/res/FS_Res.h"
#include "inc/gui/FS_Gui.h"
#include "inc/gui/FS_WebGui.h"
#include "inc/dcd/FS_DcdPkg.h"
#include "inc/dcd/FS_DcdLib.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_MemDebug.h"

#define FS_DCD_CHANNEL_TAB_H		(IFS_DcdGetChannelTabHeight())
#define FS_DCD_ENTRY_DETAIL_H		(IFS_DcdGetIdleDetailHeight())
#define FS_DCD_IMAGE_MAX_W			64

typedef struct FS_DcdUIContext_Tag
{
	FS_List *				channel_list;
	FS_SINT4				focus_channel;
	FS_DcdChannelFeed *		cur_feed;		/* for list display */
	FS_DcdEntry *			cur_entry;		/* for list display */
	FS_ImHandle				hImage;			/* current draw image */
	FS_BOOL					show_detail_menu;

	FS_DcdEntry *			cur_snap_entry;	/* for snapshot list display */
}FS_DcdUICtx;

static void FS_DcdDrawNextSnapshot( FS_Window *win );

static FS_DcdUICtx GFS_DcdUICtx;

static void FS_DcdUICtxDeinit( void )
{
	FS_DcdUICtx *dcdCtx = &GFS_DcdUICtx;
	
	if( dcdCtx->hImage ){
		FS_ImDestroy( dcdCtx->hImage );
	}
	IFS_Memset( dcdCtx, 0, sizeof(FS_DcdUICtx) );
}

static void FS_DcdUICtxInit( void )
{
	FS_List *node;
	FS_DcdUICtx *dcdCtx = &GFS_DcdUICtx;

	dcdCtx->channel_list = FS_DcdGetChannelList( );
	dcdCtx->cur_feed = FS_DcdGetIdleChannel( );
	dcdCtx->cur_entry = FS_DcdGetIdleEntry( );
	if( dcdCtx->cur_feed == FS_NULL ){
		if( dcdCtx->channel_list && ! FS_ListIsEmpty(dcdCtx->channel_list)){
			node = dcdCtx->channel_list->next;
			dcdCtx->cur_feed = FS_ListEntry( node, FS_DcdChannelFeed, list );
			if( ! FS_ListIsEmpty( &dcdCtx->cur_feed->entry_list ) ){
				node = dcdCtx->cur_feed->entry_list.next;
				dcdCtx->cur_entry = FS_ListEntry( node, FS_DcdEntry, list );
			}
		}
	}
}

static void FS_DcdUICtxInitSnapshotEntry( void )
{
	FS_List *head;
	FS_DcdUICtx *dcdCtx = &GFS_DcdUICtx;

	head = FS_DcdGetSnapshotList( );
	if( FS_ListIsEmpty( head ) ){
		dcdCtx->cur_snap_entry = FS_NULL;
	}else{
		dcdCtx->cur_snap_entry = FS_ListEntry(head->next, FS_DcdEntry, list );
	}
}

static FS_DcdEntry *FS_DcdUICtxGetCurSnapshotEntry( void )
{
	FS_DcdUICtx *dcdCtx = &GFS_DcdUICtx;
	
	return dcdCtx->cur_snap_entry;
}

void FS_DcdUICtxUpdate( FS_BOOL ret_to_list )
{
	if( ! FS_WindowFindId(FS_W_DcdMainFrm) ){
		return;
	}
	FS_DcdUICtxDeinit( );
	FS_DcdUICtxInit( );

	if( ret_to_list ){
		FS_MessageBox( FS_MS_OK, FS_Text(FS_T_DCD_DATA_UPDATED), FS_NULL, FS_FALSE );
		FS_DestroyWindowByID( FS_W_DcdMainMenu );
		FS_DestroyWindowByID( FS_W_DcdDetailFrm );
		FS_DestroyWindowByID( FS_W_DcdDetailMenu );
		FS_DestroyWindowByID( FS_W_DcdSnapshotListFrm );
		FS_DestroyWindowByID( FS_W_DcdSnapshotListMenu );
	}
}

void FS_DcdUICtxUpdateFinish( void )
{
	FS_DestroyWindowByID( FS_W_DcdWaitingFrm );	
}

static void FS_DcdSysInit( void )
{
	FS_SystemInit( );
	FS_DcdInit( );
}

static FS_CHAR *FS_DcdGetImageFromContentList( FS_List *content_list )
{
	FS_List *node;
	FS_DcdContent *content;

	FS_ListForEach( node, content_list ){
		content = FS_ListEntry( node, FS_DcdContent, list );
		if( content->file && FS_STR_NI_EQUAL(content->type, "image/", 6) ){
			return content->file;
		}
	}
	return FS_NULL;
}

static FS_DcdEntry *FS_DcdUICtxGetCurEntry( void )
{
	FS_DcdUICtx *dcdCtx = &GFS_DcdUICtx;
	
	return dcdCtx->cur_entry ;
}

static FS_DcdChannelFeed *FS_DcdUICtxGetCurChannel( void )
{
	FS_DcdUICtx *dcdCtx = &GFS_DcdUICtx;
	
	return dcdCtx->cur_feed;
}

static void FS_DcdUICtxSetImHandler( FS_ImHandle hImage )
{
	FS_DcdUICtx *dcdCtx = &GFS_DcdUICtx;

	if( dcdCtx->hImage ){
		FS_ImDestroy( dcdCtx->hImage );
		dcdCtx->hImage = FS_NULL;
	}
	dcdCtx->hImage = hImage;
}

static void FS_DcdDrawChannelTab( FS_SINT4 top )
{
	FS_SINT4 channel_cnt, focus_channel, i, unit;
	FS_Rect rect, r2;
	FS_COLOR border_color, bg_color, focus_color;
	FS_DcdUICtx *dcdCtx = &GFS_DcdUICtx;
	
	if( dcdCtx->channel_list == FS_NULL || FS_ListIsEmpty(dcdCtx->channel_list) ){
		channel_cnt = FS_DcdGetChannelCount( );
		focus_channel = FS_DcdGetIdleChannelIndex( );
	}else{
		channel_cnt = FS_ListCount( dcdCtx->channel_list );
		focus_channel = dcdCtx->focus_channel;
	}
	
	if( channel_cnt == 0 ) return;
	
	border_color = IFS_DDB_RGB( 0, 0, 0 );
	bg_color = IFS_DDB_RGB( 0xFF, 0xFF, 0xFF );
	focus_color = IFS_DDB_RGB( 0xFF, 0, 0 );
	
	rect.left = 0;
	rect.top = top;
	rect.width = IFS_GetScreenWidth( );
	rect.height = FS_DCD_CHANNEL_TAB_H;
	FS_DrawRect( &rect, border_color );
	
	unit = rect.width / channel_cnt;
	r2.top = rect.top + 1;
	r2.height = rect.height - 1;
	for( i = 0; i < channel_cnt; i ++ ){
		r2.left = i * unit;
		r2.width = unit;
		FS_DrawRect( &r2, border_color );
		r2.left += 1;
		r2.width -= 1;
		if( i == focus_channel ){
			FS_FillRect( &r2, focus_color );
		}else{
			FS_FillRect( &r2, bg_color );
		}
	}
}

static void FS_DcdDrawEntryImage_CB( void* context, FS_Bitmap *bmp )
{
	FS_Rect rect;
	FS_SINT4 tab_h, span, top = (FS_SINT4)context;
	FS_DcdEntry *entry, *entry2;
	
	span = IFS_GetWidgetSpan( );

	if( bmp == FS_NULL )
		return;

	entry = FS_DcdUICtxGetCurEntry( );
	entry2 = FS_DcdUICtxGetCurSnapshotEntry( );
	if(entry2 && FS_WindowIsTopMost(FS_W_DcdSnapshotListFrm) && FS_DcdGetImageFromContentList(&entry2->content_list) ){
		tab_h = 0;
	}else{
		tab_h = FS_DCD_CHANNEL_TAB_H;
	}
	/*
	 * 1. draw animation in idle
	 * 2. draw animation in channel list
	 * 3. draw animation in dcd snapshot list
	*/
	if( (IFS_DcdCanDrawIdle() && entry == FS_NULL)
		|| (entry && FS_WindowIsTopMost(FS_W_DcdMainFrm) && FS_DcdGetImageFromContentList(&entry->content_list) )
		|| (entry2 && FS_WindowIsTopMost(FS_W_DcdSnapshotListFrm) && FS_DcdGetImageFromContentList(&entry2->content_list) )){
		rect.left = IFS_GetWidgetSpan( );
		rect.top = top + tab_h + span;
		rect.width = bmp->width;
		rect.height = bmp->height;
		FS_DrawBitmap( rect.left, rect.top, bmp, FS_S_NONE );
		IFS_InvalidateRect( &rect );
	}
}

static void FS_DcdDrawEntryDetail( FS_SINT4 top, FS_SINT4 tab_h, FS_DcdEntry *entry )
{
	FS_CHAR *image, *title, *summary, file[FS_MAX_PATH_LEN];	
	FS_ImHandle hImage;
	FS_SINT4 im_w = 0, im_h = 0, title_h, span, line_h, detail_h;
	FS_Rect rect, clip;
	FS_Bitmap *bmp;
	FS_COLOR orgFg;
	
	/* calculate some position */
	span = IFS_GetWidgetSpan();
	line_h = IFS_GetLineHeight( );
	detail_h = FS_DCD_ENTRY_DETAIL_H;
	/* get current focus entry data */
	if( entry == FS_NULL ){
		FS_DrawString( span, top + tab_h + span, FS_Text(FS_T_NONE), FS_S_NONE );
		return;
	}
	clip.left = 0;
	clip.top = top + tab_h;
	clip.width = IFS_GetScreenWidth( );
	clip.height = detail_h;
	FS_PushClipRect( &clip, FS_CLIP_NONE );
	
	image = FS_DcdGetImageFromContentList( &entry->content_list );
	FS_GetAbsFileName( FS_DIR_DCD, image, file );
	title = entry->title;
	summary = entry->summary;
	/* draw image */
	hImage = FS_ImCreate( file, FS_DcdDrawEntryImage_CB, (void *)top );
	FS_ImGetSize( hImage, &im_w, &im_h );
	if( im_w > 0 ){
		FS_ImageScale( &im_w, &im_h, FS_DCD_IMAGE_MAX_W );
		bmp = FS_ImDecode( hImage, im_w, im_h );
		if( bmp ){
			FS_DrawBitmap( span, top + tab_h + span, bmp, FS_S_NONE );
		}
	}
	FS_DcdUICtxSetImHandler( hImage );
	/* draw title */
	rect.left = im_w + (span << 1);
	rect.top = top + tab_h + span;
	rect.width = IFS_GetScreenWidth( ) - rect.left;
	rect.height = detail_h;
	if( ! FS_DCD_ENTRY_IS_READ(entry) ){
		orgFg = FS_SetFgColor( IFS_DDB_RGB(0, 0, 0xFF) );
	}
	title_h = FS_DrawMultiLineString( rect.left, &rect, title, FS_S_BOLD );
	if( ! FS_DCD_ENTRY_IS_READ(entry) ){
		FS_SetFgColor( orgFg );
	}
	/* draw summary */
	title_h += line_h;
	rect.top += title_h;
	rect.height -= title_h;
	if( top + tab_h + span + im_h < rect.top ){
		rect.left = span;
		rect.width = IFS_GetScreenWidth( ) - span;
	}
	FS_DrawMultiLineString( rect.left, &rect, summary, FS_S_NONE );
	/* draw border */
	rect.left = 0;
	rect.top = top + tab_h;
	rect.width = IFS_GetScreenWidth( );
	rect.height = detail_h;
	FS_DrawRect( &rect, IFS_DDB_RGB(0, 0, 0) );
	FS_PopClipRect( );
}

static void FS_DcdDrawEntryList( FS_DcdEntry *entry, FS_List *head, FS_SINT4 top_h )
{
	FS_List *node;
	FS_SINT4 top, span, line_h, client_h;
	FS_Rect rect;
	FS_COLOR border_color, orgColor;
	FS_Bitmap *icon;

	if( entry == FS_NULL ){
		return;
	}
	
	/* calculate position param */
	top = top_h + FS_DCD_ENTRY_DETAIL_H + IFS_GetWinTitleHeight( );
	span = IFS_GetWidgetSpan( );
	line_h = IFS_GetLineHeight( );
	client_h = IFS_GetScreenHeight( ) - IFS_GetSoftkeyHeight( );
	border_color = IFS_DDB_RGB( 0, 0, 0 );
	
	rect.left = 0;
	rect.top = top;
	rect.width = IFS_GetScreenWidth( );
	rect.height = line_h;
	/* draw each content */
	node = entry->list.next;
	while( node != head ){
		entry = FS_ListEntry( node, FS_DcdEntry, list );
		node = node->next;

		if( FS_DCD_ENTRY_IS_READ(entry) ){
			icon = FS_Icon( FS_I_READED_MSG );
		}else{
			icon = FS_Icon( FS_I_NEW_MSG );
		}
		orgColor = FS_SetTransColor( IFS_DDB_RGB(0xFF, 0, 0xFF) );
		FS_DrawBitmap( rect.left + span, rect.top + ((rect.height - icon->height) >> 1), icon, FS_S_TRANS );
		FS_SetTransColor( orgColor );
		
		FS_DrawString( rect.left + span + icon->width + span, rect.top + span, entry->title, FS_S_NONE );
		FS_DrawRect( &rect, border_color );
		rect.top += line_h;
		FS_ReleaseIcon( icon );
		if( rect.top + line_h > client_h ){
			break;
		}
	}
}

static void FS_DcdDrawPrevChannel( FS_Window *win )
{
	FS_List *node;
	FS_DcdChannelFeed *feed;
	FS_DcdUICtx *dcdCtx = &GFS_DcdUICtx;
	FS_Rect rect;

	rect.top = IFS_GetWinTitleHeight( );
	rect.left = 0;
	rect.width = IFS_GetScreenWidth( );
	rect.height = IFS_GetScreenHeight( ) - IFS_GetSoftkeyHeight( );

	if( dcdCtx->channel_list == FS_NULL || FS_ListIsEmpty(dcdCtx->channel_list) ){
		return;
	}
	/* switch to prev channel */
	node = dcdCtx->cur_feed->list.prev;
	if( node == dcdCtx->channel_list ){
		node = dcdCtx->channel_list->prev;
	}
	feed = FS_ListEntry( node, FS_DcdChannelFeed, list );
	dcdCtx->cur_feed = feed;
	dcdCtx->cur_entry = FS_NULL;
	if( ! FS_ListIsEmpty( &feed->entry_list ) ){
		node = feed->entry_list.next;
		dcdCtx->cur_entry = FS_ListEntry( node, FS_DcdEntry, list );
	}
	dcdCtx->focus_channel --;
	if( dcdCtx->focus_channel < 0 ){
		dcdCtx->focus_channel = FS_ListCount( dcdCtx->channel_list ) - 1;
	}
	/* redraw window */
	FS_InvalidateRect( win, &rect );
}

static void FS_DcdDrawNextChannel( FS_Window *win )
{
	FS_List *node;
	FS_DcdChannelFeed *feed;
	FS_DcdUICtx *dcdCtx = &GFS_DcdUICtx;
	FS_Rect rect;

	rect.top = IFS_GetWinTitleHeight( );
	rect.left = 0;
	rect.width = IFS_GetScreenWidth( );
	rect.height = IFS_GetScreenHeight( ) - IFS_GetSoftkeyHeight( );

	if( dcdCtx->channel_list == FS_NULL || FS_ListIsEmpty(dcdCtx->channel_list) ){
		return;
	}
	/* switch to prev channel */
	node = dcdCtx->cur_feed->list.next;
	if( node == dcdCtx->channel_list ){
		node = dcdCtx->channel_list->next;
	}
	feed = FS_ListEntry( node, FS_DcdChannelFeed, list );
	dcdCtx->cur_feed = feed;
	dcdCtx->cur_entry = FS_NULL;
	if( ! FS_ListIsEmpty( &feed->entry_list ) ){
		node = feed->entry_list.next;
		dcdCtx->cur_entry = FS_ListEntry( node, FS_DcdEntry, list );
	}
	dcdCtx->focus_channel ++;
	if( dcdCtx->focus_channel >= FS_ListCount( dcdCtx->channel_list ) ){
		dcdCtx->focus_channel = 0;
	}
	/* redraw window */
	FS_InvalidateRect( win, &rect );
}

static void FS_DcdDrawPrevEntry( FS_Window *win )
{
	FS_List *node;
	FS_DcdUICtx *dcdCtx = &GFS_DcdUICtx;
	FS_Rect rect;

	rect.top = IFS_GetWinTitleHeight( );
	rect.left = 0;
	rect.width = IFS_GetScreenWidth( );
	rect.height = IFS_GetScreenHeight( ) - IFS_GetSoftkeyHeight( );
	
	if( dcdCtx->cur_entry == FS_NULL || dcdCtx->cur_feed == FS_NULL || FS_ListIsEmpty(&dcdCtx->cur_feed->entry_list) ){
		return;
	}
	/* switch to prev entry */
	node = dcdCtx->cur_entry->list.prev;
	if( node == &dcdCtx->cur_feed->entry_list ){
		node = dcdCtx->cur_feed->entry_list.prev;
	}
	dcdCtx->cur_entry = FS_ListEntry( node, FS_DcdEntry, list );
	/* redraw window */
	FS_InvalidateRect( win, &rect );
}

static void FS_DcdDrawNextEntry( FS_Window *win )
{
	FS_List *node;
	FS_DcdUICtx *dcdCtx = &GFS_DcdUICtx;
	FS_Rect rect;

	rect.top = IFS_GetWinTitleHeight( );
	rect.left = 0;
	rect.width = IFS_GetScreenWidth( );
	rect.height = IFS_GetScreenHeight( ) - IFS_GetSoftkeyHeight( );
	
	if( dcdCtx->cur_entry == FS_NULL || dcdCtx->cur_feed == FS_NULL || FS_ListIsEmpty(&dcdCtx->cur_feed->entry_list) ){
		return;
	}
	/* switch to prev entry */
	node = dcdCtx->cur_entry->list.next;
	if( node == &dcdCtx->cur_feed->entry_list ){
		node = dcdCtx->cur_feed->entry_list.next;
	}
	dcdCtx->cur_entry = FS_ListEntry( node, FS_DcdEntry, list );
	/* redraw window */
	FS_InvalidateRect( win, &rect );
}

static FS_BOOL FS_DcdDetailWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_Rect rect;
	FS_CHAR *title;
	FS_SINT4 span = IFS_GetWidgetSpan( );
	FS_SINT4 im_w = (FS_SINT4)win->private_data2;
	FS_SINT4 title_h = IFS_GetWinTitleHeight( ), line_h = IFS_GetLineHeight( );
	
	title = (FS_CHAR *)win->private_data;
	if( cmd == FS_WM_PAINT && title ){
		if( win->pane.view_port.top <= title_h ){
			rect.top = title_h;
			rect.left = im_w + span;
			rect.width = IFS_GetScreenWidth() - rect.left - (span << 1);
			rect.height = FS_DCD_ENTRY_DETAIL_H;
			if( FS_PushClipRect( &rect, FS_CLIP_NONE ) ){
				FS_DrawMultiLineString( rect.left, &rect, title, FS_S_BOLD );
				FS_PopClipRect( );
			}
		}
		/* do not return true. to let window to draw its widget */	
	}else if( cmd == FS_WM_DESTROY && title ){
		win->private_data = 0;
		IFS_Free( title );
	}
	return FS_FALSE;
}

static void FS_DcdSaveEntry_CB( FS_Window *win )
{
	FS_SINT4 ret;
	FS_DcdEntry *entry = FS_DcdUICtxGetCurEntry( );

	ret = FS_DcdSnapshotAddEntry( entry );
	if( ret == FS_DCD_ERR_FULL ){
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_LIST_FULL), FS_NULL, FS_FALSE );
	}else if( ret == FS_DCD_OK ){
		FS_MessageBox( FS_MS_OK, FS_Text(FS_T_SAVE_OK), FS_NULL, FS_FALSE );
	}else{
		FS_MessageBox( FS_MS_OK, FS_Text(FS_T_SAVE_FAILED), FS_NULL, FS_FALSE );
	}
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );	
}

static void FS_DcdSendByMms_CB( FS_Window *win )
{
	FS_DcdEntry *entry;
	FS_CHAR file[FS_MAX_PATH_LEN], *imgFile;
	
	entry = FS_DcdUICtxGetCurSnapshotEntry( );
	if( entry == FS_NULL ){
		entry = FS_DcdUICtxGetCurEntry( );
	}
	
	if( entry ){
		imgFile = FS_DcdGetImageFromContentList( &entry->content_list );
		if( imgFile ){
			FS_GetAbsFileName( FS_DIR_DCD, imgFile, file );
			imgFile = file;
		}
		FS_MmsSendByMms( entry->title, FS_NULL, entry->summary, imgFile );
	}
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );	
}

static void FS_DcdDetailMenu_UI( FS_Window *win )
{
	FS_Widget *iSave, *iSendByMms;
	FS_Window *pMenu;
	
	iSave = FS_CreateMenuItem( 0, FS_Text(FS_T_SAVE) );
	iSendByMms = FS_CreateMenuItem( 0, FS_Text(FS_T_SEND_BY_MMS) );
	
	pMenu = FS_CreateMenu( FS_W_DcdDetailMenu, 2 );
	FS_MenuAddItem( pMenu, iSave );
	FS_MenuAddItem( pMenu, iSendByMms );

	FS_WidgetSetHandler( iSave, FS_DcdSaveEntry_CB );
	FS_WidgetSetHandler( iSendByMms, FS_DcdSendByMms_CB );
	FS_MenuSetSoftkey( pMenu );
	FS_ShowWindow( pMenu );	
}

static void FS_DcdDetail_UI( FS_CHAR *image, FS_CHAR *title, FS_CHAR *summary, FS_List *link_list )
{
	FS_CHAR file[FS_MAX_PATH_LEN];
	FS_Window *win;
	FS_WebWgt *wwgt, *wimg = FS_NULL;
	FS_List *node;
	FS_DcdLink *link;
	
	win = FS_WebCreateWin( FS_W_DcdDetailFrm, FS_Text(FS_T_DETAIL), FS_DcdDetailWndProc );
	if( GFS_DcdUICtx.show_detail_menu ){
		FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_MENU), FS_DcdDetailMenu_UI );
	}
	FS_WindowSetSoftkey( win, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	FS_ShowWindow( win );
	
	FS_ClearWebWinContext( win );
	if( image ){
		FS_GetAbsFileName( FS_DIR_DCD, image, file );
		wimg = FS_WwCreateImage( FS_NULL, FS_NULL, FS_NULL );
		FS_WWGT_SET_IMAGE( wimg );
		wimg->file = IFS_Strdup( file );
		FS_WebWinAddWebWgt( win, wimg, 0 );
	}
	if( title ){
		win->private_data = (FS_UINT4)IFS_Strdup( title );
		if( wimg ) win->private_data2 = wimg->rect.width;
	}
	if( summary ){
		wwgt = FS_WwCreateText( summary );
		FS_WebWinAddWebWgt( win, wwgt, 2 );
	}
	FS_ListForEach( node, link_list ){
		link = FS_ListEntry( node, FS_DcdLink, list );
		wwgt = FS_WwCreateLink( link->title ? link->title : FS_Text(FS_T_MORE_CONTENT), link->href );
		FS_WebWinAddWebWgt( win, wwgt, 2 );
	}

	FS_InvalidateRect( win, FS_NULL );
}

static void FS_DcdEntryDetail_CB( FS_Window * win )
{
	FS_DcdEntry *entry;

	entry = FS_DcdUICtxGetCurEntry( );
	if( entry ){
		GFS_DcdUICtx.show_detail_menu = FS_TRUE;
		if( ! FS_DCD_ENTRY_IS_READ(entry) ){
			FS_DCD_ENTRY_SET_READ(entry);
			FS_DcdSaveChannelList( );
		}
		FS_DcdDetail_UI( FS_DcdGetImageFromContentList(&entry->content_list), entry->title, entry->summary, &entry->link_list );
	}
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_DcdChannelDetail_CB( FS_Window *win )
{
	FS_DcdChannelFeed *feed;

	feed = FS_DcdUICtxGetCurChannel( );
	if( feed ){
		GFS_DcdUICtx.show_detail_menu = FS_FALSE;
		FS_DcdDetail_UI( FS_DcdGetImageFromContentList(&feed->content_list), feed->title, feed->summary, &feed->link_list );
	}
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static FS_BOOL FS_DcdSnapshotDelAllCnf_CB( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	if( cmd == FS_WM_COMMAND && wparam == FS_EV_YES )
	{
		GFS_DcdUICtx.cur_snap_entry = FS_NULL;
		FS_DcdSnaphotDelAll( );
		ret = FS_TRUE;
	}
	return ret;
}

static void FS_DcdSnapshotDelAll_CB( FS_Window *win )
{
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );		
	FS_MessageBox( FS_MS_YES_NO, FS_Text(FS_T_CONFIRM_DEL), FS_DcdSnapshotDelAllCnf_CB, FS_FALSE );
}

static void FS_DcdSnapshotDel_CB( FS_Window *win )
{
	FS_DcdEntry *entry = FS_DcdUICtxGetCurSnapshotEntry( );
	FS_DcdDrawNextSnapshot( FS_WindowFindId(FS_W_DcdSnapshotListFrm) );
	FS_DcdSnapshotDelEntry( entry );
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );	
}

static void FS_DcdSnapshotDetail_CB( FS_Window * win )
{
	FS_DcdEntry *entry;

	entry = FS_DcdUICtxGetCurSnapshotEntry( );
	if( entry ){
		GFS_DcdUICtx.show_detail_menu = FS_TRUE;
		FS_DcdDetail_UI( FS_DcdGetImageFromContentList(&entry->content_list), entry->title, entry->summary, &entry->link_list );
	}
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_DcdDrawPrevSnapshot( FS_Window *win )
{
	FS_List *node, *head;
	FS_DcdUICtx *dcdCtx = &GFS_DcdUICtx;
	FS_Rect rect;

	rect.top = IFS_GetWinTitleHeight( );
	rect.left = 0;
	rect.width = IFS_GetScreenWidth( );
	rect.height = IFS_GetScreenHeight( ) - IFS_GetSoftkeyHeight( );

	head = FS_DcdGetSnapshotList( );
	if( dcdCtx->cur_snap_entry == FS_NULL || FS_ListIsEmpty(head) ){
		return;
	}
	/* switch to prev entry */
	node = dcdCtx->cur_snap_entry->list.prev;
	if( node == head ){
		node = head->prev;
	}
	dcdCtx->cur_snap_entry = FS_ListEntry( node, FS_DcdEntry, list );
	/* redraw window */
	FS_InvalidateRect( win, &rect );
}

static void FS_DcdDrawNextSnapshot( FS_Window *win )
{
	FS_List *node, *head;
	FS_DcdUICtx *dcdCtx = &GFS_DcdUICtx;
	FS_Rect rect;

	rect.top = IFS_GetWinTitleHeight( );
	rect.left = 0;
	rect.width = IFS_GetScreenWidth( );
	rect.height = IFS_GetScreenHeight( ) - IFS_GetSoftkeyHeight( );

	head = FS_DcdGetSnapshotList( );
	if( dcdCtx->cur_snap_entry == FS_NULL || FS_ListIsEmpty(head) ){
		return;
	}
	/* switch to next entry */
	node = dcdCtx->cur_snap_entry->list.next;
	if( node == head ){
		node = head->next;
	}
	dcdCtx->cur_snap_entry = FS_ListEntry( node, FS_DcdEntry, list );
	/* redraw window */
	FS_InvalidateRect( win, &rect );
}

static FS_BOOL FS_DcdSnapshotListWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_TRUE;
	if( cmd == FS_WM_PAINT ){
		FS_DcdDrawEntryDetail( IFS_GetWinTitleHeight(), 0, FS_DcdUICtxGetCurSnapshotEntry() );
		FS_DcdDrawEntryList( FS_DcdUICtxGetCurSnapshotEntry(), FS_DcdGetSnapshotList(), 0 );
	}else if( cmd == FS_WM_DESTROY ){
		GFS_DcdUICtx.cur_snap_entry = FS_NULL;
	}else if( cmd == FS_WM_COMMAND ){ 
		if( wparam == FS_KEY_UP ){
			FS_DcdDrawPrevSnapshot( win );
		}else if( wparam == FS_KEY_DOWN ){
			FS_DcdDrawNextSnapshot( win );
		}else if( wparam == FS_SOFTKEY2 ){
			FS_DcdSnapshotDetail_CB( win );
		}else{
			ret = FS_FALSE;
		}
	}else{
		ret = FS_FALSE;
	}
	return ret;
}

static void FS_DcdSnapshotListMenu_UI( FS_Window *win )
{
	FS_Widget *iDel, *iDelAll, *iOpen, *iMmsSend;
	FS_Window *pMenu;
	
	iOpen = FS_CreateMenuItem( 0, FS_Text(FS_T_VIEW) );
	iMmsSend = FS_CreateMenuItem( 0, FS_Text(FS_T_SEND_BY_MMS) );
	iDel = FS_CreateMenuItem( 0, FS_Text(FS_T_DEL) );
	iDelAll = FS_CreateMenuItem( 0, FS_Text(FS_T_DEL_ALL) );
	
	pMenu = FS_CreateMenu( FS_W_DcdSnapshotListMenu, 4 );
	FS_MenuAddItem( pMenu, iOpen );
	FS_MenuAddItem( pMenu, iMmsSend );
	FS_MenuAddItem( pMenu, iDel );
	FS_MenuAddItem( pMenu, iDelAll );

	FS_WidgetSetHandler( iOpen, FS_DcdSnapshotDetail_CB );
	FS_WidgetSetHandler( iMmsSend, FS_DcdSendByMms_CB );
	FS_WidgetSetHandler( iDel, FS_DcdSnapshotDel_CB );
	FS_WidgetSetHandler( iDelAll, FS_DcdSnapshotDelAll_CB );

	FS_MenuSetSoftkey( pMenu );
	FS_ShowWindow( pMenu );	
}


static void FS_DcdSnapshotList_UI( FS_Window *win )
{
	FS_Window *lwin;

	FS_DcdUICtxInitSnapshotEntry( );
	
	lwin = FS_CreateWindow( FS_W_DcdSnapshotListFrm, FS_Text(FS_T_SNAPSHOT_LIST), FS_DcdSnapshotListWndProc );
	FS_WindowSetSoftkey( lwin, 1, FS_Text(FS_T_MENU), FS_DcdSnapshotListMenu_UI );
	FS_WindowSetSoftkey( lwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	FS_ShowWindow( lwin );

	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_DcdManUpdate_CB( FS_Window *win )
{
	FS_SINT4 ret;

	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
	
	ret = FS_DcdUpdate( FS_DCD_REQ_MAN );
	if( ret == FS_DCD_ERR_OK ){
		win = FS_MessageBox( FS_MS_OK, FS_Text(FS_T_BG_UPDATING), FS_NULL, FS_FALSE );
		win->id = FS_W_DcdWaitingFrm;
	}else if( ret == FS_DCD_ERR_NET_READY ){
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_BUSY), FS_NULL, FS_FALSE );
	}else{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_UNKNOW_ERR), FS_NULL, FS_FALSE );
	}
}

static void FS_DcdAddChannel_CB( FS_Window *win )
{
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
	FS_WebOpenUrl( "http://dcd.monternet.com/service/addchannel" );
}

static void FS_DcdDelChannel_CB( FS_Window *win )
{
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
	FS_WebOpenUrl( "http://dcd.monternet.com/service/deletechannel" );
}

static void FS_DcdSettingSave_CB( FS_Window *win )
{
	FS_Window *swin;
	FS_Widget *wgt;
	FS_DcdConfig cfg;
	
	swin = FS_WindowFindId( FS_W_DcdSettingFrm );
	if( swin )
	{
		wgt = FS_WindowGetWidget( swin, FS_W_DcdOnOff );
		cfg.on = FS_WGT_GET_CHECK(wgt);
		
		wgt = FS_WindowGetWidget( swin, FS_W_DcdIdleDisplay );
		cfg.idle_display = FS_WGT_GET_CHECK(wgt);
		
		wgt = FS_WindowGetWidget( swin, FS_W_DcdNetMode );
		cfg.net_mode = (FS_BYTE)(wgt->private_data);

		wgt = FS_WindowGetWidget( swin, FS_W_DcdIdleSpeed );
		cfg.idle_speed = (FS_BYTE)(wgt->private_data);
		FS_DcdSaveConfig( cfg );
		FS_MessageBox( FS_MS_OK, FS_Text(FS_T_CONFIG_UPDATED), FS_NULL, FS_FALSE );
		FS_DestroyWindow( swin );
	}
	
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static FS_CHAR *FS_DcdGetNetModeName( FS_SINT4 net_mode )
{
	if( net_mode == FS_DCD_NET_ALWAYS_ON ){
		return FS_Text( FS_T_ALWAYS_ON );
	}else if( net_mode == FS_DCD_NET_ALWAYS_OFF ){
		return FS_Text( FS_T_ALWAYS_OFF );
	}else{
		return FS_Text( FS_T_ROAMING_OFF );
	}
}

static FS_CHAR *FS_DcdGetIdleSpeedName( FS_SINT4 speed )
{
	if( speed == FS_DCD_IDLE_SLW ){
		return FS_Text( FS_T_SLOW );
	}else if( speed == FS_DCD_IDLE_MID ){
		return FS_Text( FS_T_MID );
	}else{
		return FS_Text( FS_T_QUICK );
	}
}

static void FS_DcdSelectNetMode_CB( FS_Window *win )
{
	FS_Window *swin = FS_WindowFindId( FS_W_DcdSettingFrm );
	FS_Widget *iNetMode = FS_WindowGetFocusItem( swin );
	FS_Widget *iMode = FS_WindowGetFocusItem( win );
	FS_DcdNetMode mode = (FS_DcdNetMode)iMode->private_data;

	/* now, update the setting UI */
	FS_WidgetSetText( iNetMode, FS_DcdGetNetModeName(mode) );
	iNetMode->private_data = mode;
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );	
}

static void FS_DcdSelectIdleSpeed_CB( FS_Window *win )
{
	FS_Window *swin = FS_WindowFindId( FS_W_DcdSettingFrm );
	FS_Widget *iIdleSpeed = FS_WindowGetFocusItem( swin );
	FS_Widget *iSpeed = FS_WindowGetFocusItem( win );
	FS_DcdIdleSpeed speed = (FS_DcdNetMode)iSpeed->private_data;

	/* now, update the setting UI */
	FS_WidgetSetText( iIdleSpeed, FS_DcdGetIdleSpeedName(speed) );
	iIdleSpeed->private_data = speed;
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );	
}

static void FS_DcdIdleSpeedSelect_UI( FS_Window *win )
{
	FS_Window *menu;
	FS_Widget *wgt;
	FS_Widget *focusWgt = FS_WindowGetFocusItem( win );
	FS_Rect rect = FS_GetWidgetDrawRect( focusWgt );
	FS_SINT4 i;
	
	menu = FS_CreatePopUpMenu( FS_W_DcdNetModeSelectMenu, &rect, FS_DCD_IDLE_SPEED_CNT );
	for( i = 0; i < FS_DCD_IDLE_SPEED_CNT; i ++ )
	{
		wgt = FS_CreateMenuItem( i, FS_DcdGetIdleSpeedName(i) );
		wgt->private_data = FS_DCD_IDLE_SLW + i;
		FS_WidgetSetHandler( wgt, FS_DcdSelectIdleSpeed_CB );
		FS_MenuAddItem( menu, wgt );
	}
	
	FS_MenuSetSoftkey( menu );

	FS_ShowWindow( menu );
}

static void FS_DcdNetModeSelect_UI( FS_Window *win )
{
	FS_Window *menu;
	FS_Widget *wgt;
	FS_Widget *focusWgt = FS_WindowGetFocusItem( win );
	FS_Rect rect = FS_GetWidgetDrawRect( focusWgt );
	FS_SINT4 i;
	
	menu = FS_CreatePopUpMenu( FS_W_DcdIdleSpeedSelectMenu, &rect, FS_DCD_NET_MODE_CNT );
	for( i = 0; i < FS_DCD_NET_MODE_CNT; i ++ )
	{
		wgt = FS_CreateMenuItem( i, FS_DcdGetNetModeName(i) );
		wgt->private_data = FS_DCD_NET_ALWAYS_ON + i;
		FS_WidgetSetHandler( wgt, FS_DcdSelectNetMode_CB );
		FS_MenuAddItem( menu, wgt );
	}
	
	FS_MenuSetSoftkey( menu );

	FS_ShowWindow( menu );
}

static void FS_DcdSetting_UI( FS_Window *win )
{
	FS_Window *twin;
	FS_Widget *iOnOff, *iNetMode, *iIdle, *iSpeed;
	FS_DcdConfig cfg;
	
	twin = FS_CreateWindow( FS_W_DcdSettingFrm, FS_Text(FS_T_SYS_SETTING), FS_NULL );
	iOnOff = FS_CreateCheckBox( FS_W_DcdOnOff, FS_Text(FS_T_DCD_ON) );
	iNetMode = FS_CreateComboBox( FS_W_DcdNetMode, FS_Text(FS_T_DCD_NET_MODE), 0 );
	FS_ComboBoxSetTitle( iNetMode, FS_Text(FS_T_DCD_NET_MODE) );
	iIdle = FS_CreateCheckBox( FS_W_DcdIdleDisplay, FS_Text(FS_T_DCD_IDLE_DISPLAY) );
	iSpeed = FS_CreateComboBox( FS_W_DcdIdleSpeed, FS_Text(FS_T_DCD_IDLE_SPEED), 0 );
	FS_ComboBoxSetTitle( iSpeed, FS_Text(FS_T_DCD_IDLE_SPEED) );
	
	FS_WindowAddWidget( twin, iOnOff );
	FS_WindowAddWidget( twin, iNetMode );
	FS_WindowAddWidget( twin, iIdle );
	FS_WindowAddWidget( twin, iSpeed );

	cfg = FS_DcdGetConfig( );
	FS_WidgetSetCheck( iOnOff, cfg.on );
	FS_WidgetSetCheck( iIdle, cfg.idle_display );
	FS_WidgetSetText( iNetMode, FS_DcdGetNetModeName(cfg.net_mode) );
	FS_WidgetSetHandler( iNetMode, FS_DcdNetModeSelect_UI );
	iNetMode->private_data = cfg.net_mode;
	FS_WidgetSetText( iSpeed, FS_DcdGetIdleSpeedName(cfg.idle_speed) );
	FS_WidgetSetHandler( iSpeed, FS_DcdIdleSpeedSelect_UI );
	iSpeed->private_data = cfg.idle_speed;
	
	FS_WindowSetSoftkey( twin, 1, FS_Text(FS_T_OK), FS_DcdSettingSave_CB );
	FS_WindowSetSoftkey( twin, 3, FS_Text(FS_T_CANCEL), FS_StandardKey3Handler );
	
	FS_ShowWindow( twin );

	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_DcdChannelListMenu_UI( FS_Window *win )
{
	FS_Widget *iSetting, *iUpdate, *iAdd, *iDel, *iOpen, *iDetail, *iSaves;
	FS_Window *pMenu;
	
	iOpen = FS_CreateMenuItem( 0, FS_Text(FS_T_VIEW) );
	iDetail = FS_CreateMenuItem( 0, FS_Text(FS_T_CHANNEL_DETAIL) );
	iSaves = FS_CreateMenuItem( 0, FS_Text(FS_T_SNAPSHOT_LIST) );
	iUpdate = FS_CreateMenuItem( 0, FS_Text(FS_T_MAN_UPDATE) );
	iAdd = FS_CreateMenuItem( 0, FS_Text(FS_T_ADD_CHANNEL) );	
	iDel = FS_CreateMenuItem( 0, FS_Text(FS_T_DEL_CHANNEL) );
	iSetting = FS_CreateMenuItem( 0, FS_Text(FS_T_SYS_SETTING) );
	
	pMenu = FS_CreateMenu( FS_W_DcdMainMenu, 7 );
	FS_MenuAddItem( pMenu, iOpen );
	FS_MenuAddItem( pMenu, iSaves );
	FS_MenuAddItem( pMenu, iDetail );
	FS_MenuAddItem( pMenu, iUpdate );
	FS_MenuAddItem( pMenu, iAdd );
	FS_MenuAddItem( pMenu, iDel );
	FS_MenuAddItem( pMenu, iSetting );

	FS_WidgetSetHandler( iUpdate, FS_DcdManUpdate_CB );
	FS_WidgetSetHandler( iAdd, FS_DcdAddChannel_CB );
	FS_WidgetSetHandler( iDel, FS_DcdDelChannel_CB );
	FS_WidgetSetHandler( iSetting, FS_DcdSetting_UI );
	FS_WidgetSetHandler( iOpen, FS_DcdEntryDetail_CB );
	FS_WidgetSetHandler( iDetail, FS_DcdChannelDetail_CB );
	FS_WidgetSetHandler( iSaves, FS_DcdSnapshotList_UI );
	FS_WidgetSetHandler( iAdd, FS_DcdAddChannel_CB );
	FS_WidgetSetHandler( iDel, FS_DcdDelChannel_CB );
	FS_MenuSetSoftkey( pMenu );
	FS_ShowWindow( pMenu );	
}

static FS_BOOL FS_DcdChannelListWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_TRUE;
	if( cmd == FS_WM_PAINT ){
		FS_DcdDrawChannelTab( IFS_GetWinTitleHeight() );
		FS_DcdDrawEntryDetail( IFS_GetWinTitleHeight(), FS_DCD_CHANNEL_TAB_H, FS_DcdUICtxGetCurEntry() );
		FS_DcdDrawEntryList( FS_DcdUICtxGetCurEntry(), &GFS_DcdUICtx.cur_feed->entry_list, FS_DCD_CHANNEL_TAB_H );
	}else if( cmd == FS_WM_DESTROY ){
		FS_DcdUICtxDeinit( );
	}else if( cmd == FS_WM_COMMAND ){
		if( wparam == FS_KEY_LEFT ){
			FS_DcdDrawPrevChannel( win );
		}else if( wparam == FS_KEY_RIGHT ){
			FS_DcdDrawNextChannel( win );
		}else if( wparam == FS_KEY_UP ){
			FS_DcdDrawPrevEntry( win );
		}else if( wparam == FS_KEY_DOWN ){
			FS_DcdDrawNextEntry( win );
		}else if( wparam == FS_SOFTKEY2 ){
			FS_DcdEntryDetail_CB( win );
		}else{
			ret = FS_FALSE;
		}
	}else{
		ret = FS_FALSE;
	}
	return ret;
}

void FS_DcdOpenChannelList( void )
{
	FS_Window *win;

	FS_DcdSysInit( );
	FS_DcdUICtxInit( );
	FS_ActiveApplication( FS_APP_DCD );
	
	win = FS_CreateWindow( FS_W_DcdMainFrm, FS_Text(FS_T_CHANNEL_LIST), FS_DcdChannelListWndProc );
	FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_MENU), FS_DcdChannelListMenu_UI );
	FS_WindowSetSoftkey( win, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	FS_ShowWindow( win );
}

static void FS_DcdChannelList_UI( FS_Window *win )
{
	FS_DcdOpenChannelList( );
}

static FS_BOOL FS_DcdIdleWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_TRUE;
	if( cmd == FS_WM_PAINT ){
		FS_DcdDrawIdle( );
	}else if( cmd == FS_WM_LOSTFOCUS ){
		FS_DcdPauseIdleTimer( );
	}else if( cmd == FS_WM_SETFOCUS ){
		FS_DcdResumeIdleTimer( );
	}else if( cmd == FS_WM_DESTROY ){
		FS_DcdUICtxDeinit( );
	}else{
		ret = FS_FALSE;
	}
	return ret;
}

static void FS_DcdIdleWin( void )
{
	FS_Window *win;

	win = FS_WindowFindId( FS_W_DcdIdleFrm );
	if( win != FS_NULL ) return;
	
	win = FS_CreateWindow( FS_W_DcdIdleFrm, FS_Text(FS_T_CHANNEL_LIST), FS_DcdIdleWndProc );
	FS_WindowSetSoftkey( win, 3, FS_Text(FS_T_DCD), FS_DcdChannelList_UI );
	FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_EXIT), FS_StandardKey3Handler );
	FS_ShowWindow( win );
}

void FS_DcdDrawIdle( void )
{
	FS_DcdEntry *entry;
	FS_Rect rect;
	FS_SINT4 top;
	
	FS_DcdSysInit( );

#ifdef FS_PLT_WIN
	FS_DcdIdleWin( );
	if( ! FS_WindowIsTopMost(FS_W_DcdIdleFrm) ){
		return;
	}
#endif
	top = IFS_GetScreenHeight() - IFS_GetSoftkeyHeight() - FS_DCD_CHANNEL_TAB_H - FS_DCD_ENTRY_DETAIL_H;
	entry = FS_DcdGetIdleEntry( );
	if( entry ){
		rect.top = top;
		rect.left = 0;
		rect.width = IFS_GetScreenWidth( );
		rect.height = FS_DCD_ENTRY_DETAIL_H + FS_DCD_CHANNEL_TAB_H;
		FS_FillRect( &rect, 0xFFFFFF );
		
		FS_DcdDrawChannelTab( top );
		FS_DcdDrawEntryDetail( top, FS_DCD_CHANNEL_TAB_H, entry );
		
		IFS_InvalidateRect( &rect );
	}
}

#endif //FS_MODULE_DCD

