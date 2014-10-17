#include "inc\FS_Config.h"
#include "inc\gui\FS_GrpLib.h"
#include "inc\res\FS_Res.h"
#include "inc\gui\FS_Gui.h"
#include "inc\gui\FS_Font.h"
#include "inc\gui\FS_WebGui.h"
#include "inc\inte\FS_Inte.h"
#include "inc\util\FS_Util.h"
#include "inc\util\FS_File.h"
#include "inc\util\FS_NetConn.h"
#include "inc\util\FS_MemDebug.h"

#define FS_MAX_SCROLL		0x0FFFFFFF

//--------------------------------------------------------------------------
#define FS_TAB_NUM				3		// show 3 tab sheet's title
#define FS_THEME_FILE			"theme.bin"

#define FS_WGT_CAN_FOCUS(wgt)	(((wgt)->type) != FS_WGT_IMAGE && ((wgt)->type) != FS_WGT_LABEL )
#define FS_WGT_PART_ABOVE_VIEW_PORT(wgt)	((wgt)->rect.top < (wgt)->pane->view_port.top)
#define FS_WGT_PART_BELOW_VIEW_PORT(wgt)	\
	((wgt)->rect.top + (wgt)->rect.height > (wgt)->pane->view_port.top + (wgt)->pane->view_port.height)

//---------------------------------------------------------------
// global var define here
FS_GuiSkin GFS_Skins[5];

FS_UINT1 GFS_SkinIndex = 0;

static FS_List GFS_WinList = { &(GFS_WinList), &(GFS_WinList) };

static FS_SINT4 GFS_MaxMenuItemWidth = 0;
static FS_SINT4 GFS_MenuItemIndex = 0;

static void FS_DrawWinStatusBar( FS_Window *win );
static void FS_Draw3DText( FS_CHAR *txt, FS_SINT4 x, FS_SINT4 y);
static void FS_ReleaseWindow( FS_Window *win );
static void FS_Draw3DBackgroud( FS_Rect *rect );

#ifndef FS_MODULE_WEB
// dummy function to make complier happy
void FS_DrawWebWgtList( FS_List *wwgtlist ){}
void FS_ClearWebWinContext( FS_Window *win ){}
FS_BOOL FS_WebDefWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam ){return FS_FALSE;}
void FS_SetFocusToEyeableWebWgt( FS_Window *win ){}
FS_BOOL FS_WebPaneMouseEvent( FS_Window *win, FS_SINT4 x, FS_SINT4 y ) {return FS_FALSE;}
#endif	//FS_MODULE_WEB

static void FS_UpdateListItemIndex( FS_Window *win )
{
	FS_List *node;
	FS_Widget *wgt;
	
	win->item_index = 0;
	node = win->pane.widget_list.next;
	while( node != &win->pane.widget_list )
	{
		wgt = FS_ListEntry( node, FS_Widget, list );
		node = node->next;
		win->item_index ++;
		if( FS_WGT_GET_FOCUS(wgt) )
			return;
	}
}

//---------------------------------------------------------------
// get top most window which has the focus
FS_Window * FS_GetTopMostWindow( void )
{
	FS_List *node = GFS_WinList.prev;
	if( ! FS_ListIsEmpty(node) )
		return FS_ListEntry( node, FS_Window, list );
	else
		return FS_NULL;
}

static FS_Window * FS_GetParentWindow( FS_Window * win )
{
	FS_List *node = win->list.prev;
	if( node != &GFS_WinList )
		win = FS_ListEntry( node, FS_Window, list );
	else
		win = FS_NULL;
	return win;
}

FS_Window * FS_WindowFindPtr( FS_Window *pWin )
{
	FS_Window *win = FS_NULL;
	FS_List *node = GFS_WinList.prev;
	while( node != &GFS_WinList )
	{
		win = FS_ListEntry( node, FS_Window, list );
		node = node->prev;
		
		if( win == pWin )
			return win;
	}
	return FS_NULL;
}

FS_Window *FS_GetTopFullScreenWindow( void )
{
	FS_Window *win;
	FS_List *node = GFS_WinList.prev;
	while( node != &GFS_WinList )
	{
		win = FS_ListEntry( node, FS_Window, list );
		node = node->prev;
		
		if( win->type == FS_WT_WINDOW || win->type == FS_WT_WEB )
			return win;
	}
	return FS_NULL;
}

//---------------------------------------------------------------
// get the widget redraw rect base on its view port
FS_Rect FS_GetWidgetDrawRect( FS_Widget *wgt )
{
	FS_Rect rect = wgt->rect;
	if( wgt->pane )
	{
		rect.top = wgt->rect.top - wgt->pane->view_port.top + wgt->pane->rect.top;
	}
	return rect;
}

//---------------------------------------------------------------
// get the pane redraw rect base on its view port
FS_Rect FS_GetPaneDrawRect( FS_ScrollPane *pane )
{
	FS_Rect redrawRect = pane->rect;
	FS_SINT4 span = IFS_GetWidgetSpan( );
	if( pane->cyc )	// circal scroll
	{	
		if( pane->view_port.top < pane->rect.top )
		{
			if( pane->view_port.height > pane->rect.height )
				pane->view_port.top = pane->rect.top;
			else
				pane->view_port.top = pane->rect.top + pane->rect.height - pane->view_port.height;
		}
		else if( pane->view_port.top + pane->view_port.height > pane->rect.top + pane->rect.height )
			pane->view_port.top = pane->rect.top;
	}
	else
	{
		if( pane->view_port.top < pane->rect.top )
			pane->view_port.top = pane->rect.top;
		else if( pane->view_port.top + pane->view_port.height > pane->rect.top + pane->rect.height
			&& pane->rect.height > pane->view_port.height )
			pane->view_port.top = pane->rect.top + pane->rect.height - pane->view_port.height;
	}
	redrawRect.height = pane->view_port.height;
	return redrawRect;
}

static FS_Widget *FS_PaneFindFirstEyeableWidget( FS_ScrollPane *pane )
{
	FS_Widget *wgt;
	FS_List *node = pane->widget_list.next;
	while( node != &pane->widget_list )
	{
		wgt = FS_ListEntry( node, FS_Widget, list );
		if( wgt->rect.top >= pane->view_port.top 
			&& wgt->rect.top + wgt->rect.height <= pane->view_port.top + pane->view_port.height
			&& FS_WGT_CAN_FOCUS(wgt) )
			return wgt;
		node = node->next;
	}
	return FS_NULL;
}

static void FS_PaneSetEyeableFocus( FS_Window *win, FS_ScrollPane *pane )
{
	FS_Widget *wgt;
	if( win->type == FS_WT_WEB )
	{
		FS_SetFocusToEyeableWebWgt( win );
	}
	else
	{
		/* focus widget is out of view port, set view port top most widget focus */
		if( pane->focus_widget 
			&& FS_WGT_GET_FOCUS(pane->focus_widget) )
		{
			if( pane->focus_widget->rect.top < pane->view_port.top
				|| pane->focus_widget->rect.top >= pane->view_port.top + pane->view_port.height )
			{
				wgt = FS_PaneFindFirstEyeableWidget( pane );
				if( wgt )
				{
					FS_WGT_CLR_FOCUS( pane->focus_widget );
					FS_RedrawWidget( win, pane->focus_widget );
					FS_WGT_SET_FOCUS( wgt );
					pane->focus_widget = wgt;
					FS_RedrawWidget( win, wgt );
				}
			}
		}
	}
}

//---------------------------------------------------------------
// scroll the pane's view port, scroll > 0 : scroll down; scroll < 0 : scroll up
void FS_PaneScrollVertical( FS_Window *win, FS_ScrollPane *pane, FS_SINT4 scroll )
{
	if( win && pane )
	{
		FS_Rect redrawRect;
		pane->view_port.top += scroll;
		redrawRect = FS_GetPaneDrawRect( pane );
		FS_InvalidateRect( win, &redrawRect );
		if( win->pane.draw_scroll_bar )
		{
			FS_RedrawWinStatusBar( win );
		}
	}
}

FS_BOOL FS_ItemListNavigate( FS_Window *win, FS_List *itemList, FS_BOOL bPrev )
{
	FS_List *select;
	FS_BOOL ret = FS_FALSE, bRollBack = FS_FALSE, bNeedRedraw = FS_TRUE;
	FS_Widget **focusWidget, *wgt;
	FS_Rect wgtRect, vpRect;
	FS_SINT4 scroll = 0;
	if( itemList == FS_NULL || FS_ListIsEmpty( itemList ) )
		return FS_FALSE;
	
	// get focus widget
	if( itemList == &(win->pane.widget_list) )
		focusWidget = &(win->pane.focus_widget);
	else if( itemList == &win->btn_list )
		focusWidget = &(win->focus_btn);
	else if( itemList == &(win->focus_sheet->pane.widget_list) )
		focusWidget = &(win->focus_sheet->pane.focus_widget);
	/* handle scroller widget scroll */
	if( *focusWidget && (*focusWidget)->type == FS_WGT_SCROLLER )
	{
		wgt = *focusWidget;
		if( bPrev )
		{
			if( wgt->pane->view_port.top < wgt->rect.top )
			{
				if( FS_ListCount( itemList ) > 1 )
					goto NAVI_TO_NEXT;
			}
			wgt->pane->cyc = FS_FALSE;
			if( wgt->pane->rect.height > wgt->pane->view_port.height )
				FS_PaneScrollVertical( win, wgt->pane, -IFS_GetLineHeight( ) );
		}
		else
		{
			if( wgt->rect.top + wgt->rect.height <= wgt->pane->view_port.top + wgt->pane->view_port.height )
			{
				if( FS_ListCount( itemList ) > 1 )
					goto NAVI_TO_NEXT;
			}
			wgt->pane->cyc = FS_FALSE;
			if( wgt->pane->rect.height > wgt->pane->view_port.height )
				FS_PaneScrollVertical( win, wgt->pane, IFS_GetLineHeight( ) );
		}
		wgt->pane->cyc = FS_TRUE;
		return FS_TRUE;
	}

NAVI_TO_NEXT:	
	/* handle items navigate */
	if( bPrev )
	{
		if( FS_NULL == *focusWidget )
		{
			wgt = FS_NULL;
			*focusWidget = FS_ListEntry( itemList->prev, FS_Widget, list );
		}
		else
		{
			wgt = (*focusWidget);
			select = (&(*focusWidget)->list)->prev; 	
			if( select == itemList )
			{
				bRollBack = FS_TRUE;
				select = itemList->prev;
			}
			*focusWidget = FS_ListEntry( select, FS_Widget, list );
			if( ! FS_WGT_CAN_FOCUS(*focusWidget) )
			{
				/* widget cannot have focus */
				if( FS_WGT_PART_ABOVE_VIEW_PORT( *focusWidget ) )
				{
					FS_PaneScrollVertical( win, (*focusWidget)->pane, -IFS_GetLineHeight( ) );
					*focusWidget = wgt;
					bNeedRedraw = FS_FALSE;
				}
				else
				{
					if( select->prev != itemList )
					{
						*focusWidget = FS_ListEntry( select->prev, FS_Widget, list );
					}
					else
					{
						(*focusWidget) = FS_ListEntry( itemList->prev, FS_Widget, list );
						bRollBack = FS_TRUE;
					}
				}
			}
		}
		
		if( bNeedRedraw )
		{
			if( wgt && wgt != *focusWidget )
			{
				FS_WGT_CLR_FOCUS( wgt );
				FS_RedrawWidget( win, wgt );
			}
			FS_WGT_SET_FOCUS(*focusWidget);
			if( (*focusWidget)->pane )
			{
				wgtRect = (*focusWidget)->pane->focus_widget->rect;
				vpRect =  (*focusWidget)->pane->view_port;
				scroll = wgtRect.top - vpRect.top;
				// scroll view port up
				if( scroll < 0 )						
				{
					FS_PaneScrollVertical( win, (*focusWidget)->pane, scroll );
					bNeedRedraw = FS_FALSE;
				}
				if( bRollBack ) // scroll back to first item
				{
					FS_PaneScrollVertical( win, (*focusWidget)->pane, -FS_MAX_SCROLL );
					bNeedRedraw = FS_FALSE;
				}
			}
		}
		
		if( bNeedRedraw )
		{
			FS_RedrawWidget( win, *focusWidget );
		}
		ret = FS_TRUE;
	}
	else
	{
		if( FS_NULL == *focusWidget )
		{
			wgt = FS_NULL;
			*focusWidget = FS_ListEntry( itemList->next, FS_Widget, list );
		}
		else
		{
			wgt = (*focusWidget);
			select = (&(*focusWidget)->list)->next; 	
			if( select == itemList )
			{
				bRollBack = FS_TRUE;
				select = itemList->next;
			}
			*focusWidget = FS_ListEntry( select, FS_Widget, list );
			if( ! FS_WGT_CAN_FOCUS(*focusWidget) )
			{
				/* widget cannot have focus */
				if( FS_WGT_PART_BELOW_VIEW_PORT( *focusWidget ) )
				{
					FS_PaneScrollVertical( win, (*focusWidget)->pane, IFS_GetLineHeight( ) );
					*focusWidget = wgt;
					bNeedRedraw = FS_FALSE;
				}
				else
				{
					if( select->next != itemList )
					{
						*focusWidget = FS_ListEntry( select->next, FS_Widget, list );
					}
					else
					{
						(*focusWidget) = FS_ListEntry( itemList->next, FS_Widget, list );
						bRollBack = FS_TRUE;
					}
				}
			}
		}
		
		if( bNeedRedraw )
		{
			if( wgt && wgt != *focusWidget )
			{
				FS_WGT_CLR_FOCUS( wgt );
				FS_RedrawWidget( win, wgt );
			}
			FS_WGT_SET_FOCUS(*focusWidget);
			if( (*focusWidget)->pane )
			{
				wgtRect = (*focusWidget)->pane->focus_widget->rect;
				vpRect =  (*focusWidget)->pane->view_port;
				scroll = (wgtRect.top + wgtRect.height) - (vpRect.top + vpRect.height);
				scroll = FS_MIN( scroll, (wgtRect.top - vpRect.top) );
				// scroll view port up
				if( scroll > 0 )						
				{
					FS_PaneScrollVertical( win, (*focusWidget)->pane, scroll );
					bNeedRedraw = FS_FALSE;
				}
				if( bRollBack ) // scroll back to first item
				{
					FS_PaneScrollVertical( win, (*focusWidget)->pane, FS_MAX_SCROLL );
					bNeedRedraw = FS_FALSE;
				}
			}
		}
		
		if( bNeedRedraw )
		{
			FS_RedrawWidget( win, *focusWidget );
		}
		ret = FS_TRUE;
	}

	if( win->draw_status_bar || (win->focus_sheet && win->focus_sheet->draw_status_bar) )
	{
		FS_RedrawWinStatusBar( win );
	}
	return ret;
}

void FS_RedrawWinStatusBar( FS_Window *win )
{
	if( FS_WindowIsTopMost( win->id ) )
	{
		if( win->draw_status_bar || (win->focus_sheet && win->focus_sheet->draw_status_bar) )
		{
			FS_FillRect( &win->status_bar, GFS_Skins[GFS_SkinIndex].bg );
			FS_DrawWinStatusBar( win );
			IFS_InvalidateRect( &win->status_bar );
		}
	}
}

static void FS_DrawListItemIndex( FS_Window *win )
{
	if( win->show_index )
	{
		FS_CHAR str[16];
		FS_SINT4 len;
		FS_Rect rect;
		
		rect.left = IFS_GetScreenWidth() / 3 * 2;
		rect.top = 1;
		rect.width = IFS_GetScreenWidth() / 3;
		rect.height = IFS_GetWinTitleHeight() - 2;

		IFS_Sprintf( str, "%d/%d", win->item_index, win->item_total );
		len = FS_StringWidth( str );
		if( GFS_Skins[GFS_SkinIndex].effect_3d ){
			FS_Draw3DBackgroud( &rect );
		}else{
			FS_FillRect( &rect, GFS_Skins[GFS_SkinIndex].focus_bg );
		}
		FS_Draw3DText( str, IFS_GetScreenWidth( ) - len - IFS_GetWidgetSpan(), (IFS_GetWidgetSpan( ) >> 1) );
		IFS_InvalidateRect( &rect );
	}
}

void FS_WindowSetListIndex( FS_Window *win, FS_SINT4 index, FS_SINT4 total )
{
	win->item_index = index;
	win->item_total = total;
}

static FS_BOOL FS_DefWindowProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	FS_List *wgtList = FS_NULL;
	FS_Widget *wgt = FS_NULL;
	FS_ScrollPane *pane = FS_NULL;

	if( ! FS_ListIsEmpty(&(win->pane.widget_list)) )
		pane = & win->pane;
	else if( ! FS_ListIsEmpty(&(win->tab_book)) )
		pane = & win->focus_sheet->pane;

	if( pane == FS_NULL ) return FS_FALSE;
	
	if( FS_SOFTKEY2 == wparam )
	{
		if( win->focus_btn )
		{
			if( win->focus_btn->handle )
				win->focus_btn->handle( win );
			ret = FS_TRUE;
		}
		else
		{
			if( win->pane.focus_widget )
				wgt = win->pane.focus_widget;
			else if( win->focus_sheet )
				wgt =  win->focus_sheet->pane.focus_widget;

			if( wgt && wgt->type == FS_WGT_EDIT_BOX && ! FS_WGT_GET_CAN_WRITE(wgt) )
				wgt->handle = FS_StdEditBoxViewHandler;
			if( wgt && wgt->handle )
			{
				wgt->handle( win );
				ret = FS_TRUE;
			}
		}
	}
	else if( FS_KEY_LEFT == wparam )
	{
		if( win->type == FS_WT_POPUP_MENU )
		{
			FS_DestroyWindow( win );
			ret = FS_TRUE;
		} 
		else if( ! FS_ListIsEmpty(&win->btn_list))
		{
			ret = FS_ItemListNavigate( win, &win->btn_list, FS_FALSE );
		}
		else if( ! FS_ListIsEmpty(&win->tab_book) )
		{
			wgtList = win->focus_sheet->list.prev;
			if( wgtList == &win->tab_book )
				wgtList = wgtList->prev;
			win->focus_sheet = FS_ListEntry( wgtList, FS_TabSheet, list );
			FS_InvalidateRect( win, &win->client_rect );
			FS_SendMessage( win, FS_WM_COMMAND, FS_EV_TAB_FOCUS_CHANGE, (FS_UINT4)win->focus_sheet );
			ret = FS_TRUE;
		}
		else
		{
			if( pane->rect.height > pane->view_port.height && pane->rect.top < pane->view_port.top )
			{
				pane->cyc = FS_FALSE;
				FS_PaneScrollVertical( win, pane, (IFS_GetLineHeight( ) - pane->view_port.height) );
				FS_PaneSetEyeableFocus( win, pane );
				pane->cyc = FS_TRUE;
			} else {
				ret = FS_ItemListNavigate( win, &pane->widget_list, FS_TRUE );
			}
		}
	}
	else if( FS_KEY_RIGHT == wparam )
	{
		if( win->type == FS_WT_MENU && FS_WGT_GET_SUB_MENU_FLAG( win->pane.focus_widget ) )
		{
			win->pane.focus_widget->handle( win );
			ret = FS_TRUE;
		}
		else if( ! FS_ListIsEmpty(&win->btn_list))
		{
			ret = FS_ItemListNavigate( win, &win->btn_list, FS_TRUE );
		}
		else if( ! FS_ListIsEmpty(&win->tab_book) )
		{
			wgtList = win->focus_sheet->list.next;
			if( wgtList == &win->tab_book )
				wgtList = wgtList->next;
			win->focus_sheet = FS_ListEntry( wgtList, FS_TabSheet, list );
			FS_InvalidateRect( win, &win->client_rect );
			FS_SendMessage( win, FS_WM_COMMAND, FS_EV_TAB_FOCUS_CHANGE, (FS_UINT4)win->focus_sheet );
			ret = FS_TRUE;
		}
		else
		{
			if( pane->rect.height > pane->view_port.height
				&& (pane->rect.top + pane->rect.height) > (pane->view_port.top + pane->view_port.height ) )
			{
				pane->cyc = FS_FALSE;
				FS_PaneScrollVertical( win, pane, (pane->view_port.height - IFS_GetLineHeight( )) );
				FS_PaneSetEyeableFocus( win, pane );
				pane->cyc = FS_TRUE;
			} else {
				ret = FS_ItemListNavigate( win, &pane->widget_list, FS_FALSE );
			}
		}
	}
	else if( FS_KEY_UP == wparam )
	{
		if( win->focus_btn )
		{
			FS_WGT_CLR_FOCUS( win->focus_btn );
			FS_InvalidateRect( win, &win->focus_btn->rect );
			win->focus_btn = FS_NULL;
		}
		ret = FS_ItemListNavigate( win, &pane->widget_list, FS_TRUE );
	}
	else if( FS_KEY_DOWN == wparam )
	{
		if( win->focus_btn )
		{
			FS_WGT_CLR_FOCUS( win->focus_btn );
			FS_InvalidateRect( win, &win->focus_btn->rect );
			win->focus_btn = FS_NULL;
		}
		ret = FS_ItemListNavigate( win, &pane->widget_list, FS_FALSE );
	}
	else if( FS_KEY_PGUP == wparam && pane )
	{
		if( pane->rect.height > pane->view_port.height && pane->rect.top < pane->view_port.top )
		{
			pane->cyc = FS_FALSE;
			FS_PaneScrollVertical( win, pane, (IFS_GetLineHeight( ) - pane->view_port.height) );
			FS_PaneSetEyeableFocus( win, pane );
			pane->cyc = FS_TRUE;
		}
	}
	else if( FS_KEY_PGDOWN == wparam && pane )
	{
		if( pane->rect.height > pane->view_port.height
			&& (pane->rect.top + pane->rect.height) > (pane->view_port.top + pane->view_port.height ) )
		{
			pane->cyc = FS_FALSE;
			FS_PaneScrollVertical( win, pane, (pane->view_port.height - IFS_GetLineHeight( )) );
			FS_PaneSetEyeableFocus( win, pane );
			pane->cyc = FS_TRUE;
		}
	}

	if( wparam != FS_SOFTKEY2 && win->type == FS_WT_WINDOW )
	{
		FS_UpdateListItemIndex( win );
		FS_DrawListItemIndex( win );
	}
	
	return ret;
}

static FS_UINT1 FS_GetInputNumber( FS_SINT4 event )
{
	FS_UINT1 num = 0;
	if( event >= FS_KEY_0 && event <= FS_KEY_9 )
		num = ( FS_UINT1 )(event - FS_KEY_0);
	
	return num;
}

static FS_BOOL FS_MenuHandleNumberKey( FS_Window * win, FS_SINT4 event )
{
	FS_List *node;
	FS_Widget *wgt;
	FS_BOOL ret = FS_FALSE;
	FS_UINT1 num = FS_GetInputNumber( event );

	if( num > 0 )
	{
		node = win->pane.widget_list.next;
		while( num > 0 && node != &win->pane.widget_list )
		{
			wgt = FS_ListEntry( node, FS_Widget, list );
			node = node->next;
			num --;
		}
		ret = FS_TRUE;
		if( wgt && num == 0 )
		{
			FS_WGT_SET_FOCUS( wgt );
			if( wgt != win->pane.focus_widget && win->pane.focus_widget )
			{
				FS_WGT_CLR_FOCUS( win->pane.focus_widget );
				win->pane.focus_widget = wgt;
			}
			wgt->handle( win );
		}
	}

	return ret;
}

//---------------------------------------------------------------
// handle key event
static void FS_WindowKeyEvent( FS_Window * win, FS_SINT4 event )
{
	FS_BOOL ret = FS_FALSE;
	// search for window softkey to handle the event
	if( event == FS_SOFTKEY1 )
	{
		if( win->softkeys[0] && win->softkeys[0]->handle )
		{
			win->softkeys[0]->handle( win );
			return;
		}
	}
	else if( event == FS_SOFTKEY3 )
	{
		if( win->softkeys[2] && win->softkeys[2]->handle )
		{
			win->softkeys[2]->handle( win );
			return;
		}
	}

	// no widget want the event, window try to handle the event
	ret = FS_SendMessage( win, FS_WM_COMMAND, event, 0 );
	if( ! ret )
	{
		if( win->type != FS_WT_WEB )
		{
			ret = FS_DefWindowProc( win, FS_WM_COMMAND, event, 0 );
			if( ! ret && FS_WindowFindPtr( win ) && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
			{
				FS_MenuHandleNumberKey( win, event );
			}
		}
		else
		{
			FS_WebDefWndProc( win, FS_WM_COMMAND, event, 0 );
		}
	}
	return;
}

static FS_BOOL FS_HitRect( FS_Rect *rect, FS_SINT4 x, FS_SINT4 y )
{
	if( rect->left < x &&
		rect->top < y &&
		rect->left + rect->width > x &&
		rect->top + rect->height > y)
		return FS_TRUE;
	else
		return FS_FALSE;
}

static FS_SINT4 FS_CalcStringHeight( FS_Widget *wgt, FS_SINT4 width )
{
	FS_CHAR *str = wgt->text;
	FS_SINT4 w = wgt->rect.width;
	FS_SINT4 h = IFS_GetLineHeight( );

	if( width > 0 )
		w = width;
	if( wgt->icon )
		w -= IFS_GetLineHeight( );
	w -= (IFS_GetWidgetSpan( ) << 1);
	
	if( wgt->type == FS_WGT_SCROLLER )
		w -= (FS_BAR_W - IFS_GetWidgetSpan( ));
	
	if( str )
	{
		h = FS_StringHeight( str, 0, w, FS_NULL );
		h = ((h + IFS_GetLineHeight( ) - 1) / IFS_GetLineHeight( )) * IFS_GetLineHeight( );
	}
	
	return FS_MAX( IFS_GetLineHeight( ), h );
}

//---------------------------------------------------------------
// check if the mouse click hit the button
static FS_BOOL FS_HitButton( FS_Widget * btn, FS_SINT4 x, FS_SINT4 y )
{
	FS_Rect rect;
	if( btn->type == FS_WGT_SCROLLER )
	{
		rect = FS_GetWidgetDrawRect( btn );
	}
	else
	{
		rect = FS_GetWidgetDrawRect( btn );
	}
	return FS_HitRect( &rect, x, y );
}

//---------------------------------------------------------------
// bal book title handle mouse event to scroll left/right
static FS_BOOL FS_TabBookHandleMouseEvent( FS_Window *win, FS_SINT4 x, FS_SINT4 y )
{
	FS_BOOL ret = FS_FALSE;
	FS_List *sheet;
	FS_Rect left, right;
	
	left.left = 0;
	left.top = IFS_GetWinTitleHeight( );
	left.width = IFS_GetLineHeight( );
	left.height = IFS_GetLineHeight( );

	right.left = IFS_GetScreenWidth( ) - IFS_GetLineHeight( );
	right.top = IFS_GetWinTitleHeight( );
	right.width = IFS_GetLineHeight( );
	right.height = IFS_GetLineHeight( );
	
	if( FS_HitRect( &left, x, y ) )
	{
		sheet = win->focus_sheet->list.prev;
		if( sheet == &win->tab_book )
			sheet = sheet->prev;
		win->focus_sheet = FS_ListEntry( sheet, FS_TabSheet, list );
		FS_InvalidateRect( win, &win->client_rect );
		ret = FS_TRUE;
	}
	else if( FS_HitRect( &right, x, y ) )
	{
		sheet = win->focus_sheet->list.next;
		if( sheet == &win->tab_book )
			sheet = sheet->next;
		win->focus_sheet = FS_ListEntry( sheet, FS_TabSheet, list );
		FS_InvalidateRect( win, &win->client_rect );
		ret = FS_TRUE;
	}
	return ret;
}

//---------------------------------------------------------------
// scroll bar handle mouse event to scroll up/down
static FS_BOOL FS_ScrollBarHandleMouseEvent( FS_Window *win, FS_SINT4 x, FS_SINT4 y )
{
	FS_BOOL ret = FS_FALSE;
	FS_ScrollPane *pane = FS_NULL;
	FS_Rect area1, area2, area3;
	if( ! FS_ListIsEmpty(&win->pane.widget_list) )
		pane = &win->pane;
	else if( ! FS_ListIsEmpty(&win->tab_book) )
		pane = &win->focus_sheet->pane;
	
	if( pane == FS_NULL )
		return FS_FALSE;

	area1 = pane->bar.rect;
	area1.height = area1.width;
	area2 = pane->bar.rect;
	area2.top = pane->bar.rect.top + area1.height;
	area2.height = pane->bar.rect.height - (area1.height << 1);
	area3 = area1;
	area3.top = area2.top + area2.height;
	if( FS_HitRect( &area1, x, y ) )
	{
		if( pane->bar.box_top > 0 )
		{
			pane->cyc = FS_FALSE;
			FS_PaneScrollVertical( win, pane, -IFS_GetLineHeight( ) );
			FS_PaneSetEyeableFocus( win, pane );
			pane->cyc = FS_TRUE;
		}
		ret = FS_TRUE;
	}
	else if( FS_HitRect( &area2, x, y ) )
	{
		FS_SINT4 page = pane->view_port.height - IFS_GetLineHeight( );
		if( y < pane->bar.box_top )	// scroll up one page
		{
			pane->cyc = FS_FALSE;
			FS_PaneScrollVertical( win, pane, -page );
			FS_PaneSetEyeableFocus( win, pane );
			pane->cyc = FS_TRUE;
		}
		else if( y > pane->bar.box_top + pane->bar.box_height )	// scroll down one page
		{
			pane->cyc = FS_FALSE;
			FS_PaneScrollVertical( win, pane, page );
			FS_PaneSetEyeableFocus( win, pane );
			pane->cyc = FS_TRUE;
		}
		ret = FS_TRUE;
	}
	else if( FS_HitRect( &area3, x, y ) )
	{
		if( pane->bar.box_top < area2.top + area2.height - pane->bar.box_height )
		{
			pane->cyc = FS_FALSE;
			FS_PaneScrollVertical( win, pane, IFS_GetLineHeight( ) );
			FS_PaneSetEyeableFocus( win, pane );
			pane->cyc = FS_TRUE;
		}
		ret = FS_TRUE;
	}
	return ret;
}

static FS_BOOL FS_WindowPaneMouseEvent( FS_Window *win, FS_SINT4 x, FS_SINT4 y )
{
	FS_List *head, *node;
	FS_Widget *btn;
	
	// search for list items to handle the event
	head = &win->pane.widget_list;
	if( ! FS_ListIsEmpty(&win->tab_book) )
		head = &win->focus_sheet->pane.widget_list;
	node = head->next;
	while( node != head )
	{
		btn = FS_ListEntry( node, FS_Widget, list );
		if( FS_HitButton( btn, x, y ) )
		{
			// clear focus from button if any
			if( win->focus_btn )
			{
				FS_WGT_CLR_FOCUS( win->focus_btn );
				FS_RedrawWidget( win, win->focus_btn );
			}
			// set focus to correct item
			if( win->pane.focus_widget && win->pane.focus_widget != btn )
			{
				FS_WGT_CLR_FOCUS( win->pane.focus_widget );
				FS_RedrawWidget( win, win->pane.focus_widget );
			}
			else if( win->focus_sheet && win->focus_sheet->pane.focus_widget != btn )
			{
				FS_WGT_CLR_FOCUS( win->focus_sheet->pane.focus_widget );
				FS_RedrawWidget( win, win->focus_sheet->pane.focus_widget );
			}
			
			if( win->pane.focus_widget && win->pane.focus_widget != btn )
			{
				win->pane.focus_widget = btn;
				FS_WGT_SET_FOCUS( btn );
				FS_RedrawWidget( win, btn );
			}
			else if( win->focus_sheet && win->focus_sheet->pane.focus_widget != btn )
			{
				win->focus_sheet->pane.focus_widget = btn;
				FS_WGT_SET_FOCUS( btn );
				FS_RedrawWidget( win, btn );
			}
			
			if( win->draw_status_bar || (win->focus_sheet && win->focus_sheet->draw_status_bar) )
				FS_RedrawWinStatusBar( win );		
			if( btn->handle )	
			{
				btn->handle( win ); 	// call the handler
				return FS_TRUE;
			}
		}
		node = node->next;
	}
	return FS_FALSE;
}

//---------------------------------------------------------------
// handle mouse event
static void FS_WindowMouseEvent( FS_Window * win, FS_SINT4 x, FS_SINT4 y )
{
	FS_List *node;
	FS_Widget *btn;
	FS_SINT4 i;
	FS_Rect screen = { 0, 0, 0, 0 };

	screen.width = IFS_GetScreenWidth( );
	screen.height = IFS_GetScreenHeight( );
	/* out side the window area, destroy this window */
	if( ! FS_HitRect( &win->rect, x, y ) && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
	{
		/* must in my screen area */
		if( FS_HitRect( &screen, x, y ) )
		{
			FS_DestroyWindow( win );
			return;
		}
	}

	screen.left = 0;
	screen.top = 0;
	screen.width = IFS_GetScreenWidth( );
	screen.height = IFS_GetWinTitleHeight( );
	if( FS_HitRect( &screen, x, y ) )
	{
		// search for window button to handle the event
		node = win->btn_list.next;
		while( node != &win->btn_list )
		{
			btn = FS_ListEntry( node, FS_Widget, list );
			if( FS_HitButton( btn, x, y ) )
			{
				if( win->focus_btn && win->focus_btn != btn )
				{
					FS_WGT_CLR_FOCUS( win->focus_btn );
					FS_InvalidateRect( win, &win->focus_btn->rect );
				}
				if( win->focus_btn == FS_NULL || win->focus_btn != btn )
				{
					win->focus_btn = btn;
					FS_WGT_SET_FOCUS( btn );
					FS_InvalidateRect( win, &win->focus_btn->rect );
				}
				if( btn->handle )
					btn->handle( win );		// call the handler
				return;
			}
			node = node->next;
		}
		return;
	}

	screen.left = 0;
	screen.top = IFS_GetScreenHeight( ) - IFS_GetSoftkeyHeight( );
	screen.width = IFS_GetScreenWidth( );
	screen.height = IFS_GetSoftkeyHeight( );
	if( FS_HitRect( &screen, x, y ) )
	{
		// search for window softkey to handle the event
		for( i = 0; i < FS_WIN_KEY_NUM; i ++ )
		{
			btn = win->softkeys[i];
			if( btn && FS_HitButton( btn, x, y ) && btn->handle )
			{
				btn->handle( win );		// call the handler
				return;
			}
		}
		return;
	}
	
	// scroll bar handle mouse event
	if( win->pane.draw_scroll_bar || (win->focus_sheet && win->focus_sheet->pane.draw_scroll_bar) )
	{
		if( FS_ScrollBarHandleMouseEvent( win, x, y ) )
			return;
	}

	/* tab book handle */
	if( ! FS_ListIsEmpty(&win->tab_book) )
	{
		if( FS_TabBookHandleMouseEvent( win, x, y ) )
			return;
	}

	if( win->type != FS_WT_WEB  )
		FS_WindowPaneMouseEvent( win, x, y );
	else
		FS_WebPaneMouseEvent( win, x, y );
	
	return;
}

static void FS_UpdateWidgetListWidth( FS_List *widgetList, FS_SINT4 w, FS_BOOL bUpdateLeft )
{
	FS_List *node = widgetList->next;
	FS_Widget *entry;
	while( node != widgetList )
	{
		entry = FS_ListEntry( node, FS_Widget, list );
		entry->rect.width = w;
		if( bUpdateLeft )
			entry->rect.left = (IFS_GetScreenWidth( ) - w) >> 1;
		node = node->next;
	}
}

/* return if we need a scroll bar */
static FS_BOOL FS_JudgePaneScrollBar( FS_ScrollPane *pane )
{
	FS_BOOL ret = FS_FALSE;
	FS_List *node;
	FS_Widget *wgt;
	if( ! pane->draw_scroll_bar )
	{
		if( pane->rect.height > pane->view_port.height )
		{
			node = pane->widget_list.next;
			pane->draw_scroll_bar = FS_TRUE;
			pane->bar.rect = FS_GetPaneDrawRect( pane );
			pane->bar.rect.left = pane->rect.left + pane->rect.width - FS_BAR_W;
			pane->bar.rect.width = FS_BAR_W;
			// reset the edit box's width
			while( node != &pane->widget_list )
			{
				wgt = FS_ListEntry( node, FS_Widget, list );
				/* image widget's width is always short than IFS_GetScreenWidth( ) - FS_BAR_W */
				if( wgt->type != FS_WGT_IMAGE ) wgt->rect.width -= FS_BAR_W;
				node = node->next;
			}
			ret = FS_TRUE;
		}
	}
	else if( pane->draw_scroll_bar )
	{
		if( pane->rect.height <= pane->view_port.height )
		{
			node = pane->widget_list.next;
			pane->draw_scroll_bar = FS_FALSE;
			// reset the edit box's width
			while( node != &pane->widget_list )
			{
				wgt = FS_ListEntry( node, FS_Widget, list );
				/* image widget's width is always short than IFS_GetScreenWidth( ) - FS_BAR_W */
				if( wgt->type != FS_WGT_IMAGE ) wgt->rect.width += FS_BAR_W;
				node = node->next;
			}
			ret = FS_TRUE;
		}
	}
	return ret;
}

/*
	update pane widgets position
	@param[in]	 pane		pane to update
	@param[in]	 y			start y pos
	@param[in]	 h			height to grow, if h < 0, them reduce height

	@return		if we change the scroll bar status
*/
static FS_BOOL FS_UpdatePaneWidgetsTop( FS_ScrollPane *pane, FS_SINT4 y, FS_SINT4 h )
{	
	FS_List *node = pane->widget_list.next;
	FS_Widget *entry;
	while( node != &pane->widget_list )
	{
		entry = FS_ListEntry( node, FS_Widget, list );
		if( entry->rect.top > y )
			entry->rect.top += h;
		node = node->next;
	}
	pane->rect.height += h;
	/* decide to draw scroll bar */
	return FS_JudgePaneScrollBar( pane );
}

static void FS_UpdateWidgetsRectForShareHeight( FS_Widget *item )
{
	FS_Widget *wgt, *thisWgt = item;
	FS_SINT4 i, wgt_cnt = 2;
	FS_SINT4 left, width;

	if ( item->pane == FS_NULL ) return;
	if ( ! FS_WGT_GET_SHARE_HEIGHT(item) ) return;

	while ( item->list.prev != &item->pane->widget_list )
	{
		wgt = FS_ListEntry( item->list.prev, FS_Widget, list );
		if( ! FS_WGT_GET_SHARE_HEIGHT(wgt) )
		{
			/* find out the widget whom we want to share height to */
			break;
		}

		wgt_cnt ++;
		item = FS_ListEntry( item->list.prev, FS_Widget, list );
	}

	if ( item->list.prev == &item->pane->widget_list )
	{
		/* we did not find any widget who to share height with */
		return;
	}

	thisWgt->rect.top = wgt->rect.top;
	width = wgt->rect.width * (wgt_cnt - 1) / wgt_cnt;
	left = wgt->pane->rect.left;
	item = wgt;
	for ( i = 0; i < wgt_cnt; i ++ )
	{
		item->rect.left = left + (i * width);
		item->rect.width = width;
		item = FS_ListEntry(item->list.next, FS_Widget, list);
	}
}

void FS_PaneAddWidget( FS_ScrollPane *pane, FS_Widget *item )
{
	FS_SINT4 w = 0, h = IFS_GetWidgetSpan( );
	FS_List * widgetList;
	if( pane && item )
	{		
		if( item->rect.height == 0 )	/* full screen edit box */
			item->rect.height = pane->view_port.height;
		if( item->type == FS_WGT_EDIT_BOX )
			item->rect.height = FS_MIN( pane->view_port.height, item->rect.height );
		item->pane = pane;
		
		widgetList = &pane->widget_list;
		
		// the first item, set the position and focus
		if( pane->focus_widget == FS_NULL )
		{
			if( FS_WGT_CAN_FOCUS( item ) )
			{
				FS_WGT_SET_FOCUS( item );
				pane->focus_widget = item;
			}
		}
		
		if( FS_ListIsEmpty( widgetList ) )
		{
			item->rect.top = item->pane->rect.top;
			if( item->type == FS_WGT_MENUITEM ) item->rect.top += IFS_GetWidgetSpan( );
		}
		else
		{
 			FS_Widget * lastItem = FS_ListEntry( widgetList->prev, FS_Widget, list );
			item->rect.top = lastItem->rect.top + lastItem->rect.height;
		}

		/* menu item's width is dynamic, or fixed width */
		if( item->type == FS_WGT_MENUITEM )
		{
			item->rect.width = IFS_GetLineHeight( );
			if( item->icon )
			{
				item->rect.width += IFS_GetWidgetSpan( );
				item->rect.width += item->icon->width;
			}
			if( item->text )
			{
				item->rect.width += IFS_GetWidgetSpan( );
				item->rect.width += FS_StringWidth( item->text );
			}
			
			item->rect.left = pane->rect.left;
			
			item->rect.width = FS_MIN( IFS_GetScreenWidth( ) - IFS_GetLineHeight( ), item->rect.width );
			
			if( GFS_MaxMenuItemWidth < item->rect.width )
			{
				GFS_MaxMenuItemWidth = item->rect.width;
				FS_UpdateWidgetListWidth( widgetList, item->rect.width, (FS_BOOL)(pane->win->type == FS_WT_POPUP_MENU));
			}
			else
				item->rect.width = GFS_MaxMenuItemWidth;		// keep all menu items aligned
		}
		else if( item->rect.width == 0 )
		{
			item->rect.width = item->pane->rect.width;
			if( pane->draw_scroll_bar )
			{
				item->rect.width -= FS_BAR_W;
			}
		}

		FS_ListAddTail( widgetList, &item->list );
		
		if ( FS_WGT_GET_SHARE_HEIGHT(item) && item->list.prev != widgetList )
		{
			/*
			 * this widget want to share height with previous widget, 
			 * which means that they will be in the same line 
			 */
			FS_UpdateWidgetsRectForShareHeight( item );
		}
		else
		{
			pane->rect.height += item->rect.height;
		}

		if( item->type != FS_WGT_MENUITEM )
		{			
			FS_JudgePaneScrollBar( pane );
		}
	}
}

//---------------------------------------------------------------
// draw 3D border, will no limit to clip rect.
void FS_Draw3DBorder( FS_Rect *rect, FS_BOOL down )
{
	FS_Rect border = *rect;
	border.height --;
	border.width --;
	if( GFS_Skins[GFS_SkinIndex].effect_3d )
	{
		if( down )
		{
			FS_DrawLine( border.left, border.top, border.width + border.left, border.top, 0 );
			FS_DrawLine( border.left, border.top, border.left, border.top + border.height, 0 );
			FS_DrawLine( border.left + border.width, border.top, border.left + border.width,
					border.top + border.height, IFS_DDB_RGB(0xFF, 0xFF, 0xFF) );
			FS_DrawLine( border.left, border.top + border.height, border.left + border.width + 1,
					border.top + border.height, IFS_DDB_RGB(0xFF, 0xFF, 0xFF) );
		}
		else
		{
			FS_DrawLine( border.left, border.top, border.width + border.left, border.top, 
				IFS_DDB_RGB(0xFF, 0xFF, 0xFF) );
			FS_DrawLine( border.left, border.top, border.left, border.top + border.height, 
				IFS_DDB_RGB(0xFF, 0xFF, 0xFF) );
			FS_DrawLine( border.left + border.width, border.top, border.left + border.width,
					border.top + border.height, 0 );
			FS_DrawLine( border.left, border.top + border.height, border.left + border.width + 1,
					border.top + border.height, 0 );
		}
	}
	else
	{
		FS_COLOR clr;
		if( down )
			clr = GFS_Skins[GFS_SkinIndex].fg;
		else
			clr = GFS_Skins[GFS_SkinIndex].bg;
		
		FS_DrawLine( border.left, border.top, border.width + border.left, border.top, clr );
		FS_DrawLine( border.left, border.top, border.left, border.top + border.height, clr );
		FS_DrawLine( border.left + border.width, border.top, border.left + border.width,
				border.top + border.height, clr );
		FS_DrawLine( border.left, border.top + border.height, border.left + border.width + 1,
				border.top + border.height, clr );
	}
}
//---------------------------------------------------------------
// draw 3d text
static void FS_Draw3DText( FS_CHAR *txt, FS_SINT4 x, FS_SINT4 y)
{
	FS_COLOR orgFgColor, orgBgColor, orgTransColor;
#if 0
	orgFgColor = FS_SetFgColor( GFS_Skins[GFS_SkinIndex].focus_text );
	FS_DrawString( x + 3, y + 3, txt, FS_S_TRANS );
	FS_DrawString( x + 1, y + 3, txt, FS_S_TRANS );
	FS_DrawString( x + 2, y + 2, txt, FS_S_TRANS );
	FS_DrawString( x + 2, y + 4, txt, FS_S_TRANS );
	FS_SetFgColor( IFS_DDB_RGB(0xFF, 0xFF, 0xFF) );
	FS_DrawString( x + 2, y + 3, txt, FS_S_TRANS );
#else
	orgFgColor = FS_SetFgColor( GFS_Skins[GFS_SkinIndex].focus_text );
	orgBgColor = FS_SetBgColor( GFS_Skins[GFS_SkinIndex].focus_bg );
	orgTransColor = FS_SetTransColor( GFS_Skins[GFS_SkinIndex].focus_bg );
	FS_DrawString( x, y + 2, txt, FS_S_BOLD | FS_S_TRANS );
#endif
	FS_SetFgColor( orgFgColor );
	FS_SetBgColor( orgBgColor );
	FS_SetTransColor( orgTransColor );
}

//---------------------------------------------------------------
// draw a button on the screen
static void FS_DrawWidget( FS_Widget *pBtn )
{
	FS_Rect drawRect;
	FS_SINT4 x, y, w;
	FS_COLOR origFg, origBg = -1, origTrans;
	FS_SINT4 span = IFS_GetWidgetSpan( ) >> 1;
	FS_Rect fill;
	FS_Bitmap *icon;
	FS_UINT1 cType = FS_CLIP_NONE;
	FS_Rect clipText;
	FS_CHAR szMark[32] = {0};

	drawRect = FS_GetWidgetDrawRect( pBtn );
	if( (drawRect.top + drawRect.height <= 0) || (drawRect.top >= IFS_GetScreenHeight( )) )
		return;		// out of view port, return
	if( drawRect.height <= 0 )
		return;
	if( drawRect.top >= IFS_GetScreenHeight( ) - IFS_GetSoftkeyHeight( ) && pBtn->type != FS_WGT_KEY )
		return;
	
	if( pBtn->type != FS_WGT_IMAGE )
	{
		drawRect.left += span;
		drawRect.width -= IFS_GetWidgetSpan( );
		drawRect.height -= span;
		if( pBtn->type == FS_WGT_MENUITEM )
		{
			drawRect.left += span;
			drawRect.width -= IFS_GetWidgetSpan( );
		}
	}
	x = drawRect.left;
	y = drawRect.top;

	if( pBtn->type == FS_WGT_IMAGE )
	{
		drawRect.width = IFS_GetScreenWidth( ) - FS_BAR_W;
	}

	if( pBtn->pane )
	{
		if( pBtn->pane->win->draw_status_bar )
			cType = FS_CLIP_CWSB;
		else
			cType = FS_CLIP_CLIENT;
	}
	
	if( FS_PushClipRect( &drawRect, cType ) )
	{
		// draw high light feature, for menu items
		if( pBtn->type == FS_WGT_MENUITEM && FS_WGT_GET_FOCUS(pBtn))
		{
			FS_FillRect( &drawRect, GFS_Skins[GFS_SkinIndex].bg );
			FS_Draw3DBorder( &drawRect, FS_TRUE );
		}
		/* draw high light feature for list item */
		if( pBtn->type == FS_WGT_LISTITEM && FS_WGT_GET_FOCUS(pBtn) )
		{
			FS_FillRect( &drawRect, GFS_Skins[GFS_SkinIndex].focus_bg );
			FS_Draw3DBorder( &drawRect, FS_FALSE );
		}
		if ( pBtn->type == FS_WGT_BUTTON && FS_WGT_GET_FOCUS(pBtn) )
		{
			fill = drawRect;
			if ( fill.width > fill.height ) {
				fill.left += (fill.width - fill.height) >> 1;
				fill.width = fill.height;
			}
			FS_FillRect( &fill, GFS_Skins[GFS_SkinIndex].focus_bg );
			FS_Draw3DBorder( &fill, FS_FALSE );
		}
		/* draw high light feature for list item */
		if( (pBtn->type == FS_WGT_CHECK_BOX || pBtn->type == FS_WGT_RADIO_BOX) && FS_WGT_GET_FOCUS(pBtn) )
		{
			FS_FillRect( &drawRect, GFS_Skins[GFS_SkinIndex].focus_bg );
			FS_Draw3DBorder( &drawRect, FS_FALSE );
		}
		
		// draw icon
		if( pBtn->icon )
		{
			if( pBtn->type != FS_WGT_BUTTON ) {
				x += span;
			} else {
				if ( drawRect.width > pBtn->icon->width ) {
					x += (drawRect.width - pBtn->icon->width) >> 1;
				}
			}
			origFg = FS_SetTransColor( IFS_DDB_RGB(0xFF, 0, 0xFF) );
			if( pBtn->type != FS_WGT_BUTTON ) {
				y += ((drawRect.height - pBtn->icon->height) >> 1);
			} else {
				y += (span << 1);
			}
			FS_DrawBitmap( x, y, pBtn->icon, FS_S_TRANS );
			FS_SetTransColor( origFg );
			
			if( pBtn->type != FS_WGT_BUTTON ) {
				x += pBtn->icon->width + IFS_GetWidgetSpan( );
				y = drawRect.top;
			} else {
				x = drawRect.left;
				y += pBtn->icon->height;
				if ( pBtn->text ) {
					w = FS_StringWidth( pBtn->text );
					if ( drawRect.width > w ) {
						x += ((drawRect.width - w) >> 1) + span;
					}
				}
			}
		}
		// draw combobox's pre text
		if( (pBtn->type == FS_WGT_EDIT_BOX || pBtn->type == FS_WGT_COMBO_BOX) && pBtn->extra_text )
		{
			origBg = FS_SetBgColor( GFS_Skins[GFS_SkinIndex].bg );
			origFg = FS_SetFgColor( GFS_Skins[GFS_SkinIndex].fg );
			FS_DrawString( x, y + span, pBtn->extra_text, FS_S_NONE );
			x += FS_StringWidth( pBtn->extra_text ) + IFS_GetWidgetSpan( );
			FS_SetBgColor( origBg );
			FS_SetFgColor( origFg );
		}
		/* draw editbox's focus feature */
		if(  FS_WGT_GET_FOCUS(pBtn) && (pBtn->type == FS_WGT_EDIT_BOX || pBtn->type == FS_WGT_COMBO_BOX) )
		{
			fill = drawRect;
			if( pBtn->icon )
				fill.left += pBtn->icon->width + span;
			if( pBtn->extra_text )
				fill.left += FS_StringWidth( pBtn->extra_text ) + span;
			FS_FillRect( &fill, GFS_Skins[GFS_SkinIndex].focus_bg );
		}
		// draw text
		if( pBtn->text )
		{
			if( (pBtn->type == FS_WGT_LISTITEM && FS_WGT_GET_FOCUS(pBtn) )
				|| (pBtn->type == FS_WGT_MENUITEM && ! FS_WGT_GET_FOCUS(pBtn)) 
				|| (pBtn->type == FS_WGT_EDIT_BOX && FS_WGT_GET_FOCUS(pBtn)) 
				|| (pBtn->type == FS_WGT_COMBO_BOX && FS_WGT_GET_FOCUS(pBtn))
				|| (pBtn->type == FS_WGT_CHECK_BOX && FS_WGT_GET_FOCUS(pBtn))
				|| (pBtn->type == FS_WGT_RADIO_BOX && FS_WGT_GET_FOCUS(pBtn))
				|| (pBtn->type == FS_WGT_BUTTON && FS_WGT_GET_FOCUS(pBtn)))
			{
				origBg = FS_SetBgColor( GFS_Skins[GFS_SkinIndex].focus_bg );
			}
			else if( pBtn->type != FS_WGT_KEY )
			{
				origBg = FS_SetBgColor( GFS_Skins[GFS_SkinIndex].bg );
			}
			
			if( pBtn->icon == FS_NULL )
				x += span;
			y += span;
			if( pBtn->type == FS_WGT_MENUITEM 
				|| pBtn->type == FS_WGT_LISTITEM
				|| pBtn->type == FS_WGT_CHECK_BOX
				|| pBtn->type == FS_WGT_RADIO_BOX 
				|| pBtn->type == FS_WGT_BUTTON )
			{
				if( FS_WGT_GET_FOCUS(pBtn) )	// for menu item, focus color fg, reverse
					origFg = FS_SetFgColor( GFS_Skins[GFS_SkinIndex].focus_text);
				else				// or, black
					origFg = FS_SetFgColor( GFS_Skins[GFS_SkinIndex].fg );
				if( FS_WGT_GET_RED_FLAG(pBtn) )
					FS_SetFgColor( IFS_DDB_RGB(0xFF, 0, 0) );
				if( FS_WGT_GET_GREEN_FLAG(pBtn) )
					FS_SetFgColor( IFS_DDB_RGB(0, 128, 0) );
			}
			/* draw submenu flag */
			if( pBtn->type == FS_WGT_MENUITEM  && FS_WGT_GET_SUB_MENU_FLAG(pBtn) )
			{
				fill = drawRect;
				fill.top  = fill.top + (fill.height >> 2);
				fill.height = (fill.height >> 1);
				fill.left = fill.left + fill.width - (IFS_GetLineHeight( ) >> 1);
				fill.width = (IFS_GetLineHeight( ) >> 1);
				if( FS_WGT_GET_FOCUS(pBtn) )
					FS_FillTrigon( &fill, FS_DIR_RIGHT, GFS_Skins[GFS_SkinIndex].focus_text );
				else
					FS_FillTrigon( &fill, FS_DIR_RIGHT, GFS_Skins[GFS_SkinIndex].fg );
			}
			
			if( pBtn->type == FS_WGT_KEY )
			{
				FS_Draw3DText( pBtn->text, x, y );
			}
			else
			{
				if( FS_WGT_GET_MULTI_LINE(pBtn) )
				{
					clipText = drawRect;
					clipText.left = x;
					clipText.width -= x;
					clipText.top = y;
					if( pBtn->type == FS_WGT_SCROLLER )
					{
						clipText.left -= span;
						clipText.width = pBtn->rect.width - IFS_GetWidgetSpan( );
					}
					FS_DrawMultiLineString( clipText.left, &clipText, pBtn->text, FS_S_NONE );
				} else {
					if( FS_WGT_GET_MARK_CHAR(pBtn) ){
						IFS_Strncpy( szMark, "************************", IFS_Strlen(pBtn->text) );
						if(pBtn->text && pBtn->text[0]) FS_DrawString( x, y, szMark, FS_S_NONE );
					} else {
						if ( FS_WGT_GET_ALIGN_CENTER(pBtn) ) {
							w = FS_StringWidth( pBtn->text );
							if ( drawRect.width > w ) {
								x += ((drawRect.width - w) >> 1);
							}
						}
						FS_DrawString( x, y, pBtn->text, FS_S_NONE );
					}
				}
			}

			if( pBtn->type == FS_WGT_LISTITEM )
			{
				if( pBtn->extra_text )
				{
					FS_SINT4 infoLen = FS_StringWidth( pBtn->extra_text );
					FS_DrawString( pBtn->rect.left + pBtn->rect.width - infoLen - IFS_GetWidgetSpan( ), 
						y, pBtn->extra_text, FS_S_NONE );
				}
				if( pBtn->sub_cap )
				{
					y = drawRect.top + IFS_GetLineHeight( ) - span;
					FS_DrawString( x , y, pBtn->sub_cap, FS_S_NONE );
				}
				if ( FS_WGT_GET_DRAW_BORDER(pBtn) )
				{
					FS_Draw3DBorder( &drawRect, FS_TRUE );
				}
			}
			if( pBtn->type == FS_WGT_MENUITEM 
				|| pBtn->type == FS_WGT_LISTITEM
				|| pBtn->type == FS_WGT_CHECK_BOX 
				|| pBtn->type == FS_WGT_RADIO_BOX
				|| pBtn->type == FS_WGT_BUTTON)
			{
				FS_SetFgColor( origFg );
			}
			if( pBtn->type != FS_WGT_KEY )
				FS_SetBgColor( origBg );
		}

		/* draw border */
		if( pBtn->type == FS_WGT_SCROLLER && FS_WGT_GET_FOCUS(pBtn) )
		{
			fill = drawRect;
			fill.width = span;
			fill.top += span;
			fill.height = FS_GetCharMaxHeight( );
			
			if( FS_WGT_GET_CAN_WRITE( pBtn ) )	/* draw cursor */		
				FS_FillRect( &fill, IFS_DDB_RGB(0xFF, 0, 0) );
		}
		else if( pBtn->type == FS_WGT_EDIT_BOX || pBtn->type == FS_WGT_COMBO_BOX )	// for edit box
		{
			if( pBtn->icon )
			{
				drawRect.left += (pBtn->icon->width + span);
				drawRect.width -= (pBtn->icon->width + span);
			}
			if( pBtn->extra_text )
			{
				drawRect.left += FS_StringWidth( pBtn->extra_text ) + span;
				drawRect.width -= FS_StringWidth( pBtn->extra_text ) + span;
			}
			FS_Draw3DBorder( &drawRect, FS_TRUE );
			if( pBtn->type == FS_WGT_COMBO_BOX )	// draw combo box flag
			{
				origTrans = FS_SetTransColor( IFS_DDB_RGB(0xFF, 0, 0xFF) );
				icon = FS_Icon( FS_I_COMBO );
				x = drawRect.left + drawRect.width - icon->width - span;
				y = drawRect.top + ((IFS_GetLineHeight() - icon->height) >> 1);
				FS_DrawBitmap( x, y, icon, FS_S_TRANS );
				FS_ReleaseIcon( icon );
				FS_SetTransColor( origTrans );
			}
		}
		else if( pBtn->type == FS_WGT_PROGRASS )
		{
			fill = drawRect;
			if( pBtn->text )
			{
				fill.top += IFS_GetLineHeight( );
				fill.height = IFS_GetLineHeight( ) - IFS_GetWidgetSpan( );
			}
			fill.left = IFS_GetLineHeight( );
			fill.width = IFS_GetScreenWidth( ) - (IFS_GetLineHeight( ) << 1);
			FS_DrawRect( &fill, 0 );
			if( pBtn->prg_val > 0 && pBtn->prg_max > 0 )
			{
				fill.width = fill.width * pBtn->prg_val / pBtn->prg_max;
				FS_FillRect( &fill, GFS_Skins[GFS_SkinIndex].focus_bg );
			}
		}
		else if( pBtn->type == FS_WGT_IMAGE )
		{
			if( pBtn->file && pBtn->rect.width > 0 )
			{
				icon = FS_ImDecode( pBtn->im_handle, pBtn->rect.width, pBtn->rect.height );
				if( icon )
				{
					icon->width = pBtn->rect.width;
					icon->height = pBtn->rect.height;
					FS_DrawBitmap( (IFS_GetScreenWidth( ) - FS_BAR_W - pBtn->rect.width) >> 1, y, icon, FS_S_NONE );
					FS_ImRelease( pBtn->im_handle );
				}
			}
		}
		FS_PopClipRect( );
	}
}

static void FS_Draw3DBackgroud( FS_Rect *rect )
{
	FS_COLOR focus_bg = GFS_Skins[GFS_SkinIndex].focus_bg;
	FS_BYTE r, g, b, rr, gg, bb;
	FS_COLOR clr;
	FS_SINT4 step = 2, i;

	r = IFS_DDB_RED( focus_bg );
	g = IFS_DDB_GREEN( focus_bg );
	b = IFS_DDB_BLUE( focus_bg );
	
	for( i = 0; i < rect->height; i ++ )
	{
		rr = (FS_BYTE)FS_MAX(r - (step * i), 0);
		gg = (FS_BYTE)FS_MAX(g - (step * i), 0);
		bb = (FS_BYTE)FS_MAX(b - (step * i), 0);
		clr = IFS_DDB_RGB( rr, gg, bb );
		FS_DrawLine( rect->left, rect->top + i, rect->left + rect->width, rect->top + i, clr );
	}
}

//---------------------------------------------------------------
// draw the window headline, include the headline buttons and window title
static void FS_DrawWinHead( FS_Window *win )
{
	FS_CHAR *pTitle = win->title;
	FS_List *btnLst = &win->btn_list;
	// draw the headline backgound
	FS_SINT4 x = win->rect.left, y = win->rect.top;
	FS_List *node;
	FS_Rect rect = { 0, 0, 0, 0 };
	FS_Bitmap *logo, *bgBmp;
	FS_COLOR orgColor;
	// draw header bg

	rect.width = IFS_GetScreenWidth( );
	rect.height = IFS_GetWinTitleHeight( );
	rect.top = y;

	bgBmp = IFS_GetWinTitleBgImg( );
	if( bgBmp == FS_NULL ){
		if( GFS_Skins[GFS_SkinIndex].effect_3d ){
			FS_Draw3DBackgroud( &rect );
		}else{
			FS_FillRect( &rect, GFS_Skins[GFS_SkinIndex].focus_bg );		
		}
	}else{
		FS_DrawBitmap( win->rect.left, win->rect.top, bgBmp, FS_S_NONE );
		IFS_ReleaseWinTitleBgImg( bgBmp );
	}
	
	// draw the headline logo
#ifdef FS_MODULE_SNS
	logo = FS_Icon(FS_I_ISYNC);
#else
	logo = FS_Icon(FS_I_LOGO);
#endif
	x = win->rect.left;
	y = win->rect.top + ((IFS_GetWinTitleHeight() - logo->height) >> 1);
	orgColor = FS_SetTransColor( IFS_DDB_RGB(0xFF, 0, 0xFF) );
	FS_DrawBitmap( x, y, logo, FS_S_TRANS );
	FS_SetTransColor( orgColor );
	x += logo->width;
	FS_ReleaseIcon( logo );
	// draw the window title
	if( pTitle )
	{
		FS_Draw3DText( pTitle, x, y + 1 );
	}
	// draw the window headline button if any
	node = btnLst->next;			// first node in the list
	while( node != btnLst )
	{
		FS_Widget *pBtn = FS_ListEntry(node, FS_Widget, list);
		FS_DrawWidget( pBtn );
		node = node->next;
	}
}
//---------------------------------------------------------------
static void FS_DrawWinBottom( FS_Window *win )
{
	// draw the bottom line background
	FS_Widget *pBtn;
	FS_SINT4 i, x = 0, y = IFS_GetScreenHeight( ) - IFS_GetSoftkeyHeight( );
	FS_Rect rect = { 0 };
	FS_Bitmap *pSBarBg;
	
	rect.left = 0;
	rect.top = IFS_GetScreenHeight( ) - IFS_GetSoftkeyHeight( );
	rect.width = IFS_GetScreenWidth( );
	rect.height = IFS_GetSoftkeyHeight( );

	pSBarBg = IFS_GetSoftkeyBarBgImg( );
	if( pSBarBg == FS_NULL ){
		if( GFS_Skins[GFS_SkinIndex].effect_3d ){
			FS_Draw3DBackgroud( &rect );
		}else{
			FS_FillRect( &rect, GFS_Skins[GFS_SkinIndex].focus_bg );		
		}
	}else{
		FS_DrawBitmap( rect.left, rect.top, pSBarBg, FS_S_NONE );
		IFS_ReleaseSoftkeyBarBgImg( pSBarBg );
	}
	// draw the softkey button
	for( i = 0; i < FS_WIN_KEY_NUM; i ++ )
	{
		pBtn = win->softkeys[i];
		if( pBtn )
			FS_DrawWidget( pBtn );
	}
}

//---------------------------------------------------------------
// draw the menu items
static void FS_DrawWidgetList( FS_List *items )
{
	FS_List *node = items->next;
	while( node != items )
	{
		FS_Widget *pItem = FS_ListEntry( node, FS_Widget, list );
		FS_DrawWidget( pItem );
		node = node->next;
	}
}
//---------------------------------------------------------------
// draw scroll bar if needed
static void FS_DrawPaneScrollBar( FS_ScrollPane *pane )
{
	if( pane && pane->draw_scroll_bar && pane->rect.height > 0 )
	{
		FS_Rect area1, area2, area3;
		/* calculate bar box height */
		pane->bar.box_height = pane->view_port.height * (pane->view_port.height - (FS_BAR_W * 2)) / pane->rect.height;
		if( pane->bar.box_height < (FS_BAR_W << 1) )
			pane->bar.box_height = FS_BAR_W << 1;
		
		area1 = pane->bar.rect;
		area1.height = area1.width;
		area2 = pane->bar.rect;
		area2.top = pane->bar.rect.top + area1.height;
		area2.height = pane->bar.rect.height - (area1.height << 1);
		area3 = area1;
		area3.top = area2.top + area2.height;
		
		FS_FillRect( &area1,  GFS_Skins[GFS_SkinIndex].focus_bg );
		FS_FillTrigon( &area1,  FS_DIR_UP, GFS_Skins[GFS_SkinIndex].bg );
		FS_FillRect( &area3,  GFS_Skins[GFS_SkinIndex].focus_bg );
		FS_FillTrigon( &area3, FS_DIR_DOWN, GFS_Skins[GFS_SkinIndex].bg );
		if( pane->rect.height > pane->view_port.height )	// we need to draw scroll pane
		{
			FS_Rect area = area1;
			FS_SINT4 pos = (pane->view_port.top - pane->rect.top) * ( area2.height - pane->bar.box_height) / (pane->rect.height - pane->view_port.height);
			pane->bar.box_top = pos + area2.top;
			area.top = pos + area2.top;
			area.height = pane->bar.box_height;
			if( area.top > area2.top + area2.height - area.height )
				area.top = area2.top + area2.height - area.height;
			FS_FillRect( &area, GFS_Skins[GFS_SkinIndex].focus_bg );
			FS_DrawRect( &area, GFS_Skins[GFS_SkinIndex].fg );
			area.top += (area.height >> 1) - 2;
			FS_DrawLine( area.left + 2, area.top, area.left + area.width - 3, 
				area.top, GFS_Skins[GFS_SkinIndex].bg );
			area.top += 3;
			FS_DrawLine( area.left + 2, area.top, area.left + area.width - 3, 
				area.top, GFS_Skins[GFS_SkinIndex].bg );
			/* draw border */
			area = pane->bar.rect;
			FS_DrawRect( &area, GFS_Skins[GFS_SkinIndex].fg );
		}
	}
}

static void FS_DrawWinStatusBar( FS_Window *win )
{
	FS_Widget *wgt;
	FS_COLOR origBg;

	if( win->draw_status_bar || (win->focus_sheet && win->focus_sheet->draw_status_bar) )
	{
		if( ! FS_ListIsEmpty( &win->pane.widget_list ) )
			wgt = win->pane.focus_widget;
		else if( ! FS_ListIsEmpty( &win->tab_book ) )
			wgt = win->focus_sheet->pane.focus_widget;
		// draw tip
		if( win && wgt && wgt->tip )
		{
			FS_CHAR *txt = FS_Text(FS_T_TIP);
			FS_FillRect( &win->status_bar, GFS_Skins[GFS_SkinIndex].bg );
			origBg = FS_SetBgColor( GFS_Skins[GFS_SkinIndex].bg );
			FS_DrawLine( win->status_bar.left, win->status_bar.top + 1, 
				win->status_bar.left + win->status_bar.width,
				win->status_bar.top + 1, GFS_Skins[GFS_SkinIndex].focus_bg );
			FS_DrawString( win->status_bar.left + IFS_GetWidgetSpan( ), win->status_bar.top + IFS_GetWidgetSpan( ), 
				txt, FS_S_NONE );
			FS_DrawString( win->status_bar.left + IFS_GetWidgetSpan( ) + FS_StringWidth(txt), 
				win->status_bar.top + IFS_GetWidgetSpan( ), wgt->tip, FS_S_NONE );
			FS_SetBgColor( origBg );
		}
	}
}
//---------------------------------------------------------------
// draw tab book's title area
static void FS_DrawTabSheetTitle( FS_Window *win )
{
	FS_Rect rect = { 0 };
	FS_CHAR *title;
	FS_TabSheet *sheet;
	FS_List *node, *first;
	FS_SINT4 i;
	FS_COLOR origColor, origBg;
	FS_Bitmap *bmp;
	FS_SINT4 span = IFS_GetWidgetSpan( ) >> 1;
	FS_Rect t, t2;
	
	if ( win->draw_sheet_arrows )
	{
		origColor = FS_SetTransColor( IFS_DDB_RGB(0xFF, 0, 0xFF) );
		bmp = FS_Icon( FS_I_LEFT );
		FS_DrawBitmap( span, IFS_GetWinTitleHeight( ) + span, bmp, FS_S_TRANS );
		FS_ReleaseIcon( bmp );
		bmp = FS_Icon( FS_I_RIGHT );
		FS_DrawBitmap( IFS_GetScreenWidth( ) - bmp->width - span, IFS_GetWinTitleHeight( ) + span, bmp, FS_S_TRANS );
		FS_ReleaseIcon( bmp );
		FS_SetTransColor( origColor );

		rect.left = IFS_GetLineHeight( );
		rect.top = IFS_GetWinTitleHeight( );
		rect.height = IFS_GetLineHeight( );
		rect.width = (IFS_GetScreenWidth( ) - (IFS_GetLineHeight( ) << 1)) / win->sheets_per_page;
	} else {
		rect.left = span;
		rect.top = IFS_GetWinTitleHeight( );
		rect.height = IFS_GetLineHeight( );
		rect.width = (IFS_GetScreenWidth( ) - IFS_GetWidgetSpan( ) ) / win->sheets_per_page;
	}
	
	/* find the first tab sheet title to draw */
	sheet = win->focus_sheet;
	node = &sheet->list;
	for( i = 1; i < win->sheets_per_page && node->prev != &win->tab_book; i ++ )
		node = node->prev;
	// now , we found the first tab sheet to draw, draw it
	first = node;
	origBg = FS_SetBgColor( GFS_Skins[GFS_SkinIndex].bg );
	// draw tab sheet title in one page
	for( i = 0; i < win->sheets_per_page; i ++ )
	{
		t = rect;
		sheet = FS_ListEntry( node, FS_TabSheet, list );
		title = sheet->title;
		if( FS_PushClipRect( &rect, FS_CLIP_NONE ) )
		{
			FS_SINT4 strW = FS_StringWidth( title );
			if( sheet == win->focus_sheet )
			{
				t2 = t;
				t2.top += span;
				t2.height -= IFS_GetWidgetSpan( ) - 1;
				FS_FillRect( &t2, GFS_Skins[GFS_SkinIndex].focus_bg );
				
				if ( sheet->icon )
				{
					origColor = FS_SetTransColor( IFS_DDB_RGB(0xFF, 0, 0xFF) );
					if ( sheet->icon->width < t.width )
						FS_DrawBitmap( t.left + ((t.width - sheet->icon->width) >> 1), t.top + span, sheet->icon, FS_S_TRANS );	
					else
						FS_DrawBitmap( t.left + span, t.top + IFS_GetWidgetSpan( ), sheet->icon, FS_S_TRANS );	
					FS_SetTransColor( origColor );
				}
				else
				{
					origColor = FS_SetFgColor( GFS_Skins[GFS_SkinIndex].focus_text );
					if( strW < t.width )
						FS_DrawString( t.left + ((t.width - strW) >> 1), t.top + IFS_GetWidgetSpan( ), title, FS_S_TRANS );
					else
						FS_DrawString( t.left + span, t.top + IFS_GetWidgetSpan( ), title, FS_S_TRANS );
					FS_SetFgColor( origColor );
				}
				t.top += span;
				t.height -= IFS_GetWidgetSpan( );
				t.width --;
				FS_DrawLine( t.left, t.top, t.left + t.width, t.top, GFS_Skins[GFS_SkinIndex].focus_text );
				FS_DrawLine( t.left, t.top, t.left, t.top + t.height, GFS_Skins[GFS_SkinIndex].focus_text );
				FS_DrawLine( t.left + t.width, t.top, t.left + t.width, t.top + t.height, GFS_Skins[GFS_SkinIndex].focus_text );
			}
			else
			{
				if ( sheet->icon )
				{
					origColor = FS_SetTransColor( IFS_DDB_RGB(0xFF, 0, 0xFF) );
					if ( sheet->icon->width < t.width )
						FS_DrawBitmap( t.left + ((t.width - sheet->icon->width) >> 1), t.top + span, sheet->icon, FS_S_TRANS );	
					else
						FS_DrawBitmap( t.left + span, t.top + IFS_GetWidgetSpan( ), sheet->icon, FS_S_TRANS );	
					FS_SetTransColor( origColor );
				}
				else
				{
					if( strW < t.width )
						FS_DrawString( t.left + ((t.width - strW) >> 1), t.top + IFS_GetWidgetSpan( ), title, FS_S_NONE );
					else
						FS_DrawString( t.left + span, t.top + IFS_GetWidgetSpan( ), title, FS_S_NONE );
				}
				t.top += span;
				t.height -= IFS_GetWidgetSpan( );
				t.width --;
				
				FS_DrawLine( t.left, t.top, t.left + t.width, t.top, GFS_Skins[GFS_SkinIndex].focus_bg );
				if( node == first )
					FS_DrawLine( t.left, t.top, t.left, t.top + t.height, GFS_Skins[GFS_SkinIndex].focus_bg );
				FS_DrawLine( t.left + t.width, t.top, t.left + t.width, t.top + t.height, GFS_Skins[GFS_SkinIndex].focus_bg );
			}
			FS_PopClipRect( );
		}
		if( sheet == win->focus_sheet )
		{
			if( node == first )
				FS_DrawLine( 0, t.top + t.height, t.left + 1, t.top + t.height, GFS_Skins[GFS_SkinIndex].focus_bg );
			else
				FS_DrawLine( 0, t.top + t.height, t.left, t.top + t.height, GFS_Skins[GFS_SkinIndex].focus_bg );
			FS_DrawLine( t.left + t.width, t.top + t.height, IFS_GetScreenWidth( ), t.top + t.height, GFS_Skins[GFS_SkinIndex].focus_bg );
		}
		rect.left += rect.width;
		node = node->next;
		if( node == first || node == &win->tab_book )
			break;
	}
	FS_SetBgColor( origBg );
}

/*
	draw a pane
*/
static void FS_DrawScrollPane( FS_ScrollPane *pane )
{
	FS_Rect rect;	
	rect = FS_GetPaneDrawRect( pane );
	if( FS_PushClipRect( &rect, FS_CLIP_NONE ) )
	{
		if( pane->win->type != FS_WT_WEB )
			FS_DrawWidgetList( &(pane->widget_list) );
		else
			FS_DrawWebWgtList( &(pane->widget_list) );
		
		FS_DrawPaneScrollBar( pane ); 
		FS_PopClipRect( );
	}
}

//---------------------------------------------------------------
// draw window content area
static void FS_DrawWinContent( FS_Window *win )
{
	// simple frame window content
	if( ! FS_ListIsEmpty(&win->pane.widget_list) )
	{
		FS_DrawScrollPane( &win->pane );
	}
	// tab book, complex window content
	else if( ! FS_ListIsEmpty(&win->tab_book) )
	{
		FS_List *node = win->tab_book.next;
		FS_TabSheet *sheet;
		while( node != &win->tab_book )
		{
			sheet = FS_ListEntry( node, FS_TabSheet, list );
			if( sheet == win->focus_sheet )
				break;
			node = node->next;
		}
		FS_DrawTabSheetTitle( win );
		FS_DrawScrollPane( &sheet->pane );
	}
}

//---------------------------------------------------------------
// draw the window
static void FS_DrawWindow( FS_Window *win )
{
	if( win->type == FS_WT_WINDOW || win->type == FS_WT_DIALOG || win->type == FS_WT_WEB )
	{
		FS_DrawWinHead( win );
		FS_FillRect( &win->client_rect, GFS_Skins[GFS_SkinIndex].bg );
		if( ! FS_SendMessage(win, FS_WM_PAINT, 0, 0) )
		{
			FS_DrawWinContent( win );
		}
		FS_DrawWinStatusBar( win );
		FS_DrawWinBottom( win );
		FS_DrawListItemIndex( win );
	}
	else if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU)
	{
		FS_FillRect( &win->client_rect, GFS_Skins[GFS_SkinIndex].focus_bg );
		FS_DrawScrollPane( &win->pane );
		FS_Draw3DBorder( &win->client_rect, FS_FALSE );
		FS_DrawWinBottom( win );
	}
}

static void FS_DeleteWidget( FS_Widget *wgt )
{
	FS_Widget *btn = wgt;
	FS_ListDel( &btn->list );
	if( btn->icon && btn->im_handle == FS_NULL )
		FS_ReleaseIcon( btn->icon );
	if( btn->text )
		IFS_Free( btn->text );
	if( btn->sub_cap)
		IFS_Free( btn->sub_cap );
	if( btn->extra_text )
		IFS_Free( btn->extra_text );
	if( btn->data )
		IFS_Free( btn->data );
	if( btn->file )
		IFS_Free( btn->file );
	if( btn->im_handle )
		FS_ImDestroy( btn->im_handle );
	IFS_Free( btn );
}

static void FS_CheckBoxHandler( FS_Window *win )
{
	FS_Widget *wgt = FS_WindowGetFocusItem( win );
	if( wgt )
	{
		FS_WidgetSetCheck( wgt, (FS_BOOL)(! FS_WGT_GET_CHECK(wgt)) );
		FS_RedrawWidget( win,  wgt );
		FS_SendMessage( win, FS_WM_COMMAND, FS_EV_ITEM_VALUE_CHANGE, (FS_UINT4)wgt );
	}
}

static void FS_RadioBoxHandler( FS_Window *win )
{
	FS_Widget *wgt = FS_WindowGetFocusItem( win );
	if( wgt )
	{
		FS_WidgetSetCheck( wgt, FS_TRUE );
		FS_SendMessage( win, FS_WM_COMMAND, FS_EV_ITEM_VALUE_CHANGE, (FS_UINT4)wgt );
		FS_InvalidateRect( win, &win->client_rect );
	}
}

static void FS_ThemeSetSkin_CB( FS_Window *win )
{
	FS_Widget *wgt;
	FS_RadioBoxHandler( win );
	wgt = FS_WindowGetFocusItem( win );
	GFS_SkinIndex = wgt->id;
	wgt = FS_WindowGetWidget( win, FS_W_Theme3D );
	GFS_Skins[GFS_SkinIndex].effect_3d = FS_WGT_GET_CHECK(wgt);
	FS_InvalidateRect( win, FS_NULL );
}

static void FS_ThemeSet3DEffect_CB( FS_Window *win )
{
	FS_Widget *wgt;
	FS_CheckBoxHandler( win );
	wgt = FS_WindowGetFocusItem( win );
	GFS_Skins[GFS_SkinIndex].effect_3d = FS_WGT_GET_CHECK(wgt);
	FS_InvalidateRect( win, FS_NULL );	
}

//---------------------------------------------------------------
// remove the button list of a window
// !!!NOTE : will free the button memory
static void FS_RemoveWidgetList( FS_List *head )
{
	FS_List *node = head->next;
	FS_Widget *btn;
	while( node != head )
	{
		btn = FS_ListEntry( node, FS_Widget, list );
		node = node->next;
		FS_DeleteWidget( btn );
	}
}

static void FS_RemoveTabSheet( FS_List *head )
{
	FS_List *node = head->next;
	FS_TabSheet *sheet;
	while( node != head )
	{
		sheet = FS_ListEntry( node, FS_TabSheet, list );
		node = node->next;
		FS_ListDel( &sheet->list );
		FS_RemoveWidgetList( &sheet->pane.widget_list );
		if ( sheet->icon )
			FS_ReleaseIcon( sheet->icon );
		IFS_Free( sheet );
	}
}
//---------------------------------------------------------------
// create a window
// !!!NOTE : will not copy the memory of param pass in
FS_Window * FS_CreateWindow( FS_UINT4 id, FS_CHAR * title, FS_WndProc proc )
{
	FS_Window *ret;

	ret = FS_WindowFindId( id );
	if( ret )
	{
		FS_ReleaseWindow( ret );
	}
	
	ret = IFS_Malloc( sizeof(FS_Window) );
	if( ret )
	{
		IFS_Memset( ret, 0, sizeof(FS_Window) );
		ret->proc = proc;
		ret->title = IFS_Strdup( title );
		FS_ListInit( &ret->btn_list );
		FS_ListInit( &ret->list );
		FS_ListInit( &ret->pane.widget_list );
		FS_ListInit( &ret->tab_book );
		ret->id = id;
		ret->type = FS_WT_WINDOW;
		ret->rect.width = IFS_GetScreenWidth( );
		ret->rect.height = IFS_GetScreenHeight( );
		ret->client_rect.top = IFS_GetWinTitleHeight( );
		ret->client_rect.width = IFS_GetScreenWidth( );
		ret->client_rect.height = IFS_GetScreenHeight( ) - IFS_GetWinTitleHeight( ) - IFS_GetSoftkeyHeight( );
		// default pane to window client
		ret->pane.rect.top = IFS_GetWinTitleHeight( );
		ret->pane.rect.width = IFS_GetScreenWidth( );
		// default view port to window client
		ret->pane.view_port = ret->pane.rect;
		ret->pane.view_port.height = IFS_GetScreenHeight( ) - IFS_GetWinTitleHeight( ) - IFS_GetSoftkeyHeight( );
		ret->pane.cyc = FS_TRUE;
		ret->status_bar.width = IFS_GetScreenWidth( );
		ret->status_bar.top = IFS_GetScreenHeight( ) - IFS_GetSoftkeyHeight( ) - IFS_GetLineHeight( );
		ret->status_bar.height = IFS_GetLineHeight( );
		ret->sheets_per_page = FS_TAB_NUM;

		ret->pane.win = ret;
		FS_ListAddTail( &GFS_WinList, &ret->list );
	}
	return ret;
}
//---------------------------------------------------------------
// create a window
// !!!NOTE : will not copy the memory of param pass in
FS_Window * FS_CreateMenu( FS_UINT4 id, FS_SINT4 nItems )
{
	FS_Window *ret;

	ret = FS_WindowFindId( id );
	if( ret )
	{
		FS_ReleaseWindow( ret );
	}

	ret = IFS_Malloc( sizeof(FS_Window) );
	if( ret )
	{
		IFS_Memset( ret, 0, sizeof(FS_Window) );
		FS_ListInit( &ret->btn_list );
		FS_ListInit( &ret->pane.widget_list );
		FS_ListInit( &ret->list );
		FS_ListInit( &ret->tab_book );
		ret->type = FS_WT_MENU;
		ret->rect.height = nItems * IFS_GetLineHeight( ) + (IFS_GetWidgetSpan( ) << 1);
		ret->rect.top = IFS_GetScreenHeight( ) - IFS_GetSoftkeyHeight( ) - ret->rect.height;
		// for menu, pane rect default to its window's rect
		ret->pane.view_port = ret->rect;
		ret->pane.rect = ret->rect;
		ret->client_rect = ret->rect;
		ret->pane.rect.height = 0;
		ret->pane.cyc = FS_TRUE;
		ret->id = id;
		ret->pane.win = ret;
		FS_ListAddTail( &GFS_WinList, &ret->list );
		GFS_MaxMenuItemWidth = IFS_GetLineHeight( ) << 2;
		GFS_MenuItemIndex = 1;
	}
	return ret;
}
//---------------------------------------------------------------

/*
	create pop up menu, menu's position is base on the rect pass in
	!!!NOTE : will not copy the memory of param pass in
	param:
			wgt			in			the widget where pop up menu will pop up
			nItems		in			the pop up menu will hold how menu items
*/
#if 0
FS_Window * FS_CreatePopUpMenu( FS_UINT4 id, FS_Rect *curRect, FS_SINT4 nItems )
{
	FS_SINT4 h = nItems * IFS_GetLineHeight( ) + (IFS_GetWidgetSpan( ) << 1);
	FS_Rect rect = *curRect;
	FS_Window *ret;

	ret = FS_WindowFindId( id );
	if( ret )
	{
		FS_ReleaseWindow( ret );
	}

	ret = IFS_Malloc( sizeof(FS_Window) );
	if( ret )
	{
		IFS_Memset( ret, 0, sizeof(FS_Window) );
		ret->id = id;
		FS_ListInit( &ret->btn_list );
		FS_ListInit( &ret->pane.widget_list );
		FS_ListInit( &ret->list );
		FS_ListInit( &ret->tab_book );
		ret->type = FS_WT_POPUP_MENU;
		if( rect.top + rect.height + h > IFS_GetScreenHeight( ) - IFS_GetSoftkeyHeight( ) )
			ret->rect.top = IFS_GetScreenHeight( ) - IFS_GetSoftkeyHeight( ) - h;
		else
			ret->rect.top = rect.top;
		ret->rect.left = (IFS_GetScreenWidth( ) - (IFS_GetLineHeight( ) << 2)) >> 1;
		ret->rect.height = h;
		// for menu, pane rect default to its window's rect
		ret->pane.rect = ret->rect;
		ret->pane.view_port = ret->rect;
		ret->pane.rect.height = 0;
		ret->client_rect = ret->rect;
		ret->pane.cyc = FS_TRUE;
		ret->pane.win = ret;
		FS_ListAddTail( &GFS_WinList, &ret->list );

		GFS_MaxMenuItemWidth = IFS_GetLineHeight( ) << 2;
	}

	return ret;
}
#else
FS_Window * FS_CreatePopUpMenu( FS_UINT4 id, FS_Rect *curRect, FS_SINT4 nItems )
{
	FS_SINT4 h = nItems * IFS_GetLineHeight( ) + (IFS_GetWidgetSpan( ) << 1);
	FS_Rect rect = *curRect;
	FS_Window *ret;

	ret = FS_WindowFindId( id );
	if( ret )
	{
		FS_ReleaseWindow( ret );
	}

	ret = IFS_Malloc( sizeof(FS_Window) );
	if( ret )
	{
		IFS_Memset( ret, 0, sizeof(FS_Window) );
		ret->id = id;
		FS_ListInit( &ret->btn_list );
		FS_ListInit( &ret->pane.widget_list );
		FS_ListInit( &ret->list );
		FS_ListInit( &ret->tab_book );
		ret->type = FS_WT_POPUP_MENU;
		if( rect.top + h > IFS_GetScreenHeight() - IFS_GetSoftkeyHeight() - IFS_GetLineHeight() )
			ret->rect.top = FS_MAX( IFS_GetScreenHeight() - IFS_GetSoftkeyHeight() - IFS_GetLineHeight() - h, IFS_GetWinTitleHeight() );
		else
			ret->rect.top = rect.top;
		ret->rect.left = (IFS_GetScreenWidth() - (IFS_GetLineHeight() << 2)) >> 1;
		ret->rect.height = h;
		// for menu, pane rect default to its window's rect
		ret->pane.rect = ret->rect;
		ret->pane.rect.height = IFS_GetWidgetSpan( ) << 1;
		ret->pane.view_port = ret->rect;
		ret->pane.view_port.height = FS_MIN( ret->pane.view_port.height, IFS_GetScreenHeight( ) - IFS_GetWinTitleHeight( ) - IFS_GetSoftkeyHeight( ) - IFS_GetLineHeight( ) );
		ret->client_rect = ret->rect;
		ret->client_rect.height = ret->pane.view_port.height;
		ret->pane.cyc = FS_TRUE;
		ret->pane.win = ret;
		FS_ListAddTail( &GFS_WinList, &ret->list );

		GFS_MaxMenuItemWidth = IFS_GetLineHeight( ) << 2;
		GFS_MenuItemIndex = 1;
	}

	return ret;
}
#endif
//---------------------------------------------------------------
/*
	create a dialog
	!!!NOTE : will not copy the memory of param pass in
	param:
			id			in			the dialog id, can be 0
			nLines		in			the lines of the dialog, one line is IFS_GetLineHeight( ) height
*/
FS_Window * FS_CreateDialog( FS_UINT4 id, FS_CHAR *title, FS_SINT4 nLines, FS_WndProc proc )
{
	FS_Window *ret;

	ret = FS_WindowFindId( id );
	if( ret )
	{
		FS_ReleaseWindow( ret );
	}

	ret = IFS_Malloc( sizeof(FS_Window) );
	if( ret )
	{
		IFS_Memset( ret, 0, sizeof(FS_Window) );
		ret->title = title;
		ret->proc = proc;
		FS_ListInit( &ret->btn_list );
		FS_ListInit( &ret->list );
		FS_ListInit( &ret->pane.widget_list );
		FS_ListInit( &ret->tab_book );
		ret->id = id;
		ret->type = FS_WT_DIALOG;
		ret->rect.left = 0;
		ret->rect.width = IFS_GetScreenWidth( );
		if( nLines <= 0 )
			ret->rect.height = IFS_GetScreenHeight( );
		else
			ret->rect.height = nLines * IFS_GetLineHeight( ) + IFS_GetWinTitleHeight( ) + IFS_GetSoftkeyHeight( );
		if( ret->rect.height > IFS_GetScreenHeight( ) )
			ret->rect.height = IFS_GetScreenHeight( );
		
		ret->rect.top = IFS_GetScreenHeight( ) - ret->rect.height;
		ret->client_rect.top = ret->rect.top + IFS_GetWinTitleHeight( );
		ret->client_rect.width = IFS_GetScreenWidth( );
		ret->client_rect.height = ret->rect.height - IFS_GetWinTitleHeight( ) - IFS_GetSoftkeyHeight( );
		// default pane to window client
		ret->pane.rect.top = ret->rect.top + IFS_GetWinTitleHeight( );
		ret->pane.rect.width = IFS_GetScreenWidth( );
		// default view port to window client
		ret->pane.view_port = ret->client_rect;
		ret->pane.cyc = FS_TRUE;
		ret->pane.win = ret;

		FS_ListAddTail( &GFS_WinList, &ret->list );
	}
	return ret;
	
}

//---------------------------------------------------------------
// add a menu item to a menu
void FS_MenuAddItem( FS_Window *win, FS_Widget *item )
{
	FS_CHAR *idxText;
	FS_SINT4 len;
	if( win && item )
	{
		/* add menuitem index number */
		len = IFS_Strlen( item->text );
		if( ! FS_WGT_GET_NO_INDEX_MENU_FLAG(item) )
		{
			idxText = IFS_Malloc( len + 16 );
			if( idxText )
			{
				IFS_Sprintf( idxText, "%d. %s", GFS_MenuItemIndex ++, item->text );
				IFS_Free( item->text );
				item->text = idxText;
			}
		}
		// will calculate item width base on its string width
		FS_PaneAddWidget( &win->pane, item );
		win->rect.width = item->rect.width;
		win->pane.rect.width = win->rect.width;
		win->client_rect.width = win->rect.width;

		if( win->type == FS_WT_POPUP_MENU )
		{
			win->rect.left = (IFS_GetScreenWidth( ) - win->rect.width) >> 1;
			win->pane.rect.left = win->rect.left;
			win->client_rect.left = win->rect.left;
			item->rect.left = win->rect.left;
		}
	}
}

FS_List * FS_WindowGetListItems( FS_Window *win )
{
	return &win->pane.widget_list;
}

static FS_Widget *FS_PaneGetNextFocusWidget( FS_ScrollPane *pane, FS_Widget *curFocus )
{
	FS_Widget *nextFocus = FS_NULL;
	FS_List *node = &curFocus->list;
	if( ! FS_ListIsEmpty(&pane->widget_list) )
	{
		if( node->next != &pane->widget_list) 
			nextFocus = FS_ListEntry( node->next, FS_Widget, list );
		else if( node->prev != &pane->widget_list) 
			nextFocus = FS_ListEntry( node->prev, FS_Widget, list );
		else
			nextFocus = FS_NULL;
	}
	return nextFocus;
}

/* if item is in this pane, and delete it, return FS_TRUE, or FS_FALSE */
FS_BOOL FS_PaneDelWidget( FS_ScrollPane *pane, FS_Widget *item )
{
	FS_BOOL ret = FS_FALSE;
	FS_Widget *wgt, *nextFocus = FS_NULL;
	FS_SINT4 y = item->rect.top, h = item->rect.height;
	FS_List *node = pane->widget_list.next;
	while( node != &pane->widget_list )
	{
		wgt = FS_ListEntry( node, FS_Widget, list );
		if( wgt == item )
		{
			if( (pane->view_port.top + pane->view_port.height) >= (pane->rect.top + pane->rect.height) )
				pane->view_port.top -= item->rect.height;
			if( FS_WGT_GET_FOCUS(wgt) )
				nextFocus = FS_PaneGetNextFocusWidget( pane, wgt );
			FS_DeleteWidget( wgt );
			ret = FS_TRUE;
			break;
		}
		node = node->next;
	}
	if( nextFocus )
	{
		FS_WGT_SET_FOCUS(nextFocus);
		pane->focus_widget = nextFocus;
	}
	else
	{
		pane->focus_widget = FS_NULL;
	}

	if( ret ) FS_UpdatePaneWidgetsTop( pane, y, -h );
	return ret;
}

//---------------------------------------------------------------
// add list item to window
void FS_WindowDelWidget( FS_Window *win, FS_Widget *item )
{
	FS_List *node;
	FS_TabSheet *sheet;

	if( item == FS_NULL || win == FS_NULL ) return;
	
	if( FS_PaneDelWidget( &win->pane, item ) )
	{
		FS_UpdateListItemIndex( win );
		win->item_total --;
		return;
	}

	node = win->tab_book.next;
	while( node != &win->tab_book )
	{
		sheet = FS_ListEntry( node, FS_TabSheet, list );
		if( FS_PaneDelWidget( &sheet->pane, item ) )
			return;
		node = node->next;
	}
}

//---------------------------------------------------------------
// remove list item from window
void FS_WindowDelWidgetList( FS_Window *win )
{
	win->item_index = 0;
	win->item_total = 0;
	win->pane.focus_widget = FS_NULL;
	win->pane.rect.height = 0;
	FS_RemoveWidgetList( &(win->pane.widget_list) );
}

FS_Widget * FS_WindowGetWidget( FS_Window *win, FS_UINT4 index )
{
	FS_List *node = win->pane.widget_list.next;
	FS_Widget *wgt;
	while( node != &win->pane.widget_list )
	{
		wgt = FS_ListEntry( node, FS_Widget, list );
		node = node->next;
		if( wgt->id == index )
			return wgt;
	}

	node = win->tab_book.next;
	while( node != &win->tab_book )
	{
		FS_List *wnode;
		FS_TabSheet *sheet;
		sheet = FS_ListEntry( node, FS_TabSheet, list );
		node = node->next;
		
		wnode = sheet->pane.widget_list.next;
		while( wnode != &sheet->pane.widget_list )
		{
			wgt = FS_ListEntry( wnode, FS_Widget, list );
			if( wgt->id == index )
				return wgt;
			wnode = wnode->next;
		}
	}
	return FS_NULL;
}

FS_Widget * FS_WindowGetWidgetByPrivateData( FS_Window *win, FS_UINT4 private_data )
{
	FS_List *node = win->pane.widget_list.next;
	FS_Widget *wgt;
	while( node != &win->pane.widget_list )
	{
		wgt = FS_ListEntry( node, FS_Widget, list );
		node = node->next;
		if( wgt->private_data == private_data )
			return wgt;
	}

	node = win->tab_book.next;
	while( node != &win->tab_book )
	{
		FS_List *wnode;
		FS_TabSheet *sheet;
		sheet = FS_ListEntry( node, FS_TabSheet, list );
		wnode = sheet->pane.widget_list.next;
		
		node = node->next;
		while( wnode != &sheet->pane.widget_list )
		{
			wgt = FS_ListEntry( wnode, FS_Widget, list );
			wnode = wnode->next;
			if( wgt->private_data == private_data )
				return wgt;
		}
	}
	return FS_NULL;
}


//---------------------------------------------------------------
// get window widget's text, index : widget index in window's pane widget list
FS_CHAR * FS_WindowGetWidgetText( FS_Window *win, FS_UINT4 index )
{
	FS_CHAR *ret = FS_NULL;
	FS_Widget *wgt = FS_WindowGetWidget( win, index );
	if( wgt )
		ret = wgt->text;
	return ret;
}

//---------------------------------------------------------------
// show the window, must be full screen window
void FS_ShowWindow( FS_Window *win )
{
	FS_Window *parent;

	parent = FS_GetParentWindow( win );
	if( parent ) FS_SendMessage( parent, FS_WM_LOSTFOCUS, 0, 0 );
	FS_SendMessage( win, FS_WM_SETFOCUS, 0, 0 );
	FS_DrawWindow( win );
	IFS_InvalidateRect( FS_NULL );
	// if not a full screen window, we need to redraw the softkey line
	if( win->type != FS_WT_WINDOW )		
	{
		FS_Rect softkeyRect = { 0 };
		softkeyRect.left = 0;
		softkeyRect.top = IFS_GetScreenHeight( ) - IFS_GetSoftkeyHeight( );
		softkeyRect.width = IFS_GetScreenWidth( );
		softkeyRect.height = IFS_GetSoftkeyHeight( );
		IFS_InvalidateRect( &softkeyRect );
	}
}

//---------------------------------------------------------------
// redraw the window soft key line
void FS_RedrawSoftkeys( FS_Window *win )
{
	FS_Rect rect = { 0 };
	rect.left = 0;
	rect.top = IFS_GetScreenHeight( ) - IFS_GetSoftkeyHeight( );
	rect.width = IFS_GetScreenWidth( );
	rect.height = IFS_GetSoftkeyHeight( );
	if( win && FS_GetTopMostWindow() == win )
	{
		FS_DrawWinBottom( win );
		IFS_InvalidateRect( &rect );
	}
}

void FS_RedrawWinTitle( FS_Window *win )
{
	FS_Rect rect = { 0 };
	rect.width = IFS_GetScreenWidth( );
	rect.height = IFS_GetWinTitleHeight( );
	rect.top = win->rect.top;;
	if( win && FS_GetTopMostWindow() == win )
	{
		FS_DrawWinHead( win );
		IFS_InvalidateRect( &rect );
	}
}

//---------------------------------------------------------------
// redraw the window rect
void FS_InvalidateRect( FS_Window *win, FS_Rect *pRect )
{
	FS_Rect rect;
	if( FS_WindowFindPtr(win) == FS_NULL ) return;

	if( pRect )
		rect = *pRect;
	else
		rect = win->rect;

	if( FS_GetTopMostWindow()== win )
	{
		FS_DrawWindow( win );
		IFS_InvalidateRect( &rect );
	}
	else
	{
		FS_GuiRepaint( );
	}
}

FS_BOOL FS_SendMessage( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	if( win && win->proc )
		return win->proc( win, cmd, wparam, lparam );
	else
		return FS_FALSE;
}

static void FS_ReleaseWindow( FS_Window *win )
{
	FS_SINT4 i;
	
	FS_SendMessage( win, FS_WM_DESTROY, 0, 0 );
	FS_RemoveWidgetList( &win->btn_list );
	for( i = 0; i < FS_WIN_KEY_NUM; i ++ )
	{
		if( win->softkeys[i] )
			FS_DeleteWidget( win->softkeys[i] );
	}
	
	if( win->type != FS_WT_WEB )
	{
		FS_RemoveWidgetList( &(win->pane.widget_list) );
	}
	else
	{
		FS_ClearWebWinContext( win );
	}
	FS_RemoveTabSheet( &win->tab_book );
	FS_SAFE_FREE( win->title );
	FS_ListDel( &win->list );
	IFS_Free( win );
}

#if 1
//---------------------------------------------------------------
// destroy window, will free the button list and key list if any
void FS_DestroyWindow( FS_Window *win )
{
	FS_BOOL redraw = FS_FALSE;
	FS_Window *topfullwin;
	
	/* invalid window. donot exist in window stack */
	if( FS_WindowFindPtr( win ) == FS_NULL )
		return;
	
	if( win == FS_GetTopMostWindow( ) )	// destroy a topmost window, need redraw
		redraw = FS_TRUE;
	
	if( win )
	{
		FS_ReleaseWindow( win );
	}

	// show the top most win
	if( redraw )
	{
		topfullwin = FS_GetTopFullScreenWindow( );
		win = FS_GetTopMostWindow( );
		
		if( win )
		{
			if( topfullwin == win )
			{
				FS_SendMessage( win, FS_WM_SETFOCUS, 0, 0 );
			}
			else
			{
				FS_SendMessage( win, FS_WM_SETFOCUS, 0, 0 );
			}
			FS_GuiRepaint( );
		}
		else
		{
			FS_GuiExit( );
			FS_NetDisconnect( FS_APP_ALL );
			IFS_SystemExit( );
			FS_DeactiveApplication( FS_APP_ALL );
		}
	}
}

#else

//---------------------------------------------------------------
/*
	destroy window, will free the button list and key list if any
	partial reflesh version DestroyWindow
	NOTE: 	use partial reflesh, when a top not full screen window destryed.
			and this window case his parent window to change some content out 
			of its rect. here, may cause some problem.
	
	!!!		当正在接收邮件的窗口破坏时，要引起主窗口中各个信箱里的邮件数变化
			而此时，用局部刷新会有问题，不会实现在Paint消息里自动刷新
*/
void FS_DestroyWindow( FS_Window *win )
{
	FS_BOOL redraw = FS_FALSE;
	FS_BOOL redrawSoftkey = FS_FALSE;
	FS_Rect rect;
	FS_SINT4 i;
	FS_Window * topWin = FS_GetTopMostWindow( );
	/* destroy a topmost window, or topmost window is not a full screen window, need redraw */	
	if( (win == topWin) || (win != topWin && topWin->type != FS_WT_WINDOW) ) 
	{
		redraw = FS_TRUE;
	}

	if( win->type != FS_WT_WINDOW )
	{
		redrawSoftkey = FS_TRUE;
	}
	
	if( win )
	{
		rect = win->rect;
		FS_SendMessage( win, FS_WM_DESTROY, 0, 0 );
		FS_RemoveWidgetList( &win->btn_list );
		for( i = 0; i < FS_WIN_KEY_NUM; i ++ )
		{
			if( win->softkeys[i] )
				FS_DeleteWidget( win->softkeys[i] );
		}
		FS_RemoveWidgetList( &(win->pane.widget_list) );
		FS_RemoveTabSheet( &win->tab_book );
		FS_ListDel( &win->list );
		IFS_Free( win );
	}

	// show the top most win
	if( redraw )
	{
		win = FS_GetTopMostWindow( );
		topWin = FS_GetTopFullScreenWindow( );
		if( win && topWin )
		{
			FS_InvalidateRect( topWin, &rect );
			if( win != topWin )
			{
				FS_InvalidateRect( win, FS_NULL );
			}
			if( redrawSoftkey )
				FS_RedrawSoftkeys( win );
		}
	}
}
#endif

void FS_DestroyWindowByID( FS_SINT4 winId )
{
	FS_Window *win = FS_WindowFindId( winId );
	if( win ) FS_DestroyWindow( win );
}
//---------------------------------------------------------------

void FS_WindowSetSoftkey( FS_Window *win, FS_BYTE pos, FS_CHAR *text, FS_WidgetEventHandler handle )
{
	FS_Widget *key;

	if( win && text )
	{
		key = win->softkeys[pos - 1];
		if( key == FS_NULL )
		{
			key = FS_CreateKey( 0, text );
			win->softkeys[pos - 1] = key;
			key->handle = handle;
			key->rect.width = FS_StringWidth( key->text ) + (IFS_GetWidgetSpan( ) << 1);
		}
		else
		{
			key->handle = handle;
			FS_COPY_TEXT( key->text, text );
			key->rect.width = FS_StringWidth( key->text ) + (IFS_GetWidgetSpan( ) << 1);
		}
		// the first button, set the position
		if( pos == 1 )
		{
			key->rect.left = 0;
			key->rect.top = IFS_GetScreenHeight( ) - IFS_GetSoftkeyHeight( );
		}
		else if( pos == 2 )
		{
			key->rect.left = (IFS_GetScreenWidth( ) - key->rect.width) >> 1;
			key->rect.top = IFS_GetScreenHeight( ) - IFS_GetSoftkeyHeight( );
			key->handle = FS_StandardKey2Handler;
		}
		else if( pos == 3 )
		{
			key->rect.left = IFS_GetScreenWidth( ) - key->rect.width + (IFS_GetWidgetSpan( ) >> 1);
			key->rect.top = IFS_GetScreenHeight( ) - IFS_GetSoftkeyHeight( );
		}
	}
	else if( win && win->softkeys[pos - 1] )
	{
		FS_DeleteWidget( win->softkeys[pos - 1] );
		win->softkeys[pos - 1] = FS_NULL;
	}
}

void FS_MenuSetSoftkey( FS_Window *win )
{
	FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_SELECT), FS_StandardMenuKey1Handler );
	FS_WindowSetSoftkey( win, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
}

//---------------------------------------------------------------
// set window title
void FS_WindowSetTitle( FS_Window *win, FS_CHAR *title )
{
	if( win )
	{
		FS_COPY_TEXT( win->title, title );
	}
}

void FS_WindowSetViewPort( FS_Window *win, FS_SINT4 vp )
{
	if( win )
	{
		if( vp >= 0 )
			win->pane.view_port.top = vp;
		else
			win->pane.view_port.top = win->pane.rect.top + win->pane.rect.height - win->pane.view_port.height;
	}
}

FS_Widget * FS_WindowGetFocusItem( FS_Window *win )
{
	if( win && win->pane.focus_widget )
		return win->pane.focus_widget;
	else if( win && win->focus_sheet && win->focus_sheet->pane.focus_widget )
		return win->focus_sheet->pane.focus_widget;
	else 
		return FS_NULL;
}

FS_Window * FS_WindowFindId( FS_UINT4 id )
{
	FS_List *node = GFS_WinList.next;
	FS_Window *win;
	while( node != &GFS_WinList )
	{
		win = FS_ListEntry( node, FS_Window, list );
		if( win->id == id )
			return win;
		node = node->next;
	}
	return FS_NULL;
}

void FS_WindowMoveTop( FS_Window *win )
{
	if( win )
	{
		FS_ListDel( &win->list );
		FS_ListAddTail( &GFS_WinList, &win->list );
	}
}

void FS_WindowMoveBottom( FS_Window *win )
{
	if( win )
	{
		FS_ListDel( &win->list );
		FS_ListAdd( &GFS_WinList, &win->list );
	}
}

static void FS_WidgetDrawImage_CB( FS_Widget *wgt, FS_Bitmap *pBmp )
{
	if( FS_WindowIsTopMost( wgt->pane->win->id ) )
	{
		FS_RedrawWidget( wgt->pane->win, wgt );
	}
}

//---------------------------------------------------------------
// create a new widget
FS_Widget * FS_CreateWidget( FS_WidgetType type, FS_UINT4 id, FS_CHAR *text, 
	FS_CHAR *sub_cap, FS_CHAR *file, FS_SINT2 iconid, FS_UINT1 nLines, FS_EditParam *eParam )
{
	FS_Widget *ret = IFS_Malloc( sizeof(FS_Widget) );
	if( ret )
	{
		IFS_Memset( ret, 0, sizeof(FS_Widget) );
		FS_ListInit( &ret->list );
		if( text && IFS_Strlen(text) > 0 )
			ret->text = IFS_Strdup( text );
		if( file && IFS_Strlen(file) > 0 )
			ret->file = IFS_Strdup( file );
		if( sub_cap && IFS_Strlen(sub_cap) > 0 )
			ret->sub_cap = IFS_Strdup( sub_cap );
		ret->type = type;
		ret->rect.height = IFS_GetLineHeight( ) * nLines;
		if( iconid > 0 )
			ret->icon = FS_Icon( iconid );
		
		ret->id = id;
		if( type == FS_WGT_EDIT_BOX )
		{
			FS_WGT_SET_MULTI_LINE( ret );
			ret->handle = FS_StdEditBoxHandler;
			FS_WGT_SET_CAN_WRITE( ret );
			if( ret->text )
				ret->rect.height = FS_CalcStringHeight( ret, IFS_GetScreenWidth( ) );
			
			if( eParam )
			{
				ret->edit_param = *eParam;
			}
			else
			{
				ret->edit_param.allow_method = FS_IM_ALL;
				ret->edit_param.preferred_method = FS_IM_ABC;
				ret->edit_param.max_len = FS_DEFAULT_EDIT_LEN;
			}
		}
		else if( type == FS_WGT_SCROLLER )
		{
			ret->rect.width = IFS_GetScreenWidth( );
			if( ret->text )
				ret->rect.height = FS_CalcStringHeight( ret, 0 );
			else
				ret->rect.height = IFS_GetLineHeight( );
			
			FS_WGT_SET_MULTI_LINE( ret );
			ret->edit_param.max_len = FS_DETAULT_SCROLL_EDIT_LEN;
		}
		else if( type == FS_WGT_CHECK_BOX )
		{
			ret->handle = FS_CheckBoxHandler;
			ret->icon = FS_Icon( FS_I_UNCHECK );
		}
		else if( type == FS_WGT_RADIO_BOX )
		{
			ret->handle = FS_RadioBoxHandler;
			ret->icon = FS_Icon( FS_I_RADIO_UNCHECK );
		}
		else if( type == FS_WGT_LABEL )
		{
			FS_WGT_SET_MULTI_LINE( ret );
			if( ret->text )
				ret->rect.height = FS_CalcStringHeight( ret, IFS_GetScreenWidth( ) );
		}
		else if( type == FS_WGT_PROGRASS )
		{
			if( FS_WGT_GET_CAN_WRITE(ret) )
				ret->rect.height = (IFS_GetLineHeight( ) << 1);
		}
		else if( type == FS_WGT_IMAGE )
		{
			if( file )
				ret->im_handle = FS_ImCreate( file, FS_WidgetDrawImage_CB, ret );
			ret->rect.height = IFS_GetWidgetSpan( );
		}
	}
	return ret;
}

void FS_WidgetSetExtraText( FS_Widget *wgt, FS_CHAR *txt )
{
	if( wgt )
	{
		if( txt )
		{
			FS_COPY_TEXT( wgt->extra_text, txt );
		}
		else
		{
			FS_SAFE_FREE( wgt->extra_text );
		}
	}
}

void FS_WindowAddWidget( FS_Window *win, FS_Widget *wgt )
{
	if( win && wgt )
	{
		if( ! win->draw_status_bar && wgt->type == FS_WGT_EDIT_BOX && wgt->rect.height != 0 )
		{
			if ( ! (FS_WGT_GET_FORCE_NO_STATUS_BAR(wgt)) )
			{
				win->draw_status_bar = FS_TRUE;
				win->pane.view_port.height -= IFS_GetLineHeight( );
			}
		}
		FS_PaneAddWidget( &win->pane, wgt );		
		win->item_total ++;		
		if( win->item_index == 0 )
			win->item_index = 1;
	}
}

void FS_TabSheetAddWidget( FS_TabSheet *sheet, FS_Widget *wgt )
{
	if( sheet && wgt )
	{
		FS_PaneAddWidget( &sheet->pane, wgt );		
	}
}

//---------------------------------------------------------------
// create a tab sheet
FS_TabSheet * FS_CreateTabSheet( FS_UINT4 id, FS_CHAR *title, FS_SINT2 icon_id, FS_BOOL bShowStatusBar )
{
	FS_TabSheet *ret = IFS_Malloc( sizeof(FS_TabSheet) );
	if( ret )
	{
		IFS_Memset( ret, 0, sizeof(FS_TabSheet) );
		ret->title = title;
		ret->id = id;
		FS_ListInit( &ret->pane.widget_list ) ;
		ret->pane.rect.top = IFS_GetWinTitleHeight( ) + IFS_GetLineHeight( );	// reserver a line to draw tab sheet title
		ret->pane.rect.width = IFS_GetScreenWidth( );
		ret->pane.view_port = ret->pane.rect;
		ret->pane.view_port.height = IFS_GetScreenHeight( ) - IFS_GetWinTitleHeight( ) - IFS_GetSoftkeyHeight( ) - IFS_GetLineHeight( );
		ret->draw_status_bar = bShowStatusBar;
		ret->pane.cyc = FS_TRUE;
		if( icon_id > 0 )
			ret->icon = FS_Icon( icon_id );
		if( bShowStatusBar )
			ret->pane.view_port.height -= IFS_GetLineHeight( );
	}
	return ret;
}

void FS_WidgetCalcHeight( FS_Widget *wgt )
{
	if( wgt->text )
	{
		wgt->rect.height = FS_CalcStringHeight( wgt, 0 );
		wgt->pane->rect.height = wgt->rect.height;
		FS_JudgePaneScrollBar( wgt->pane );
	}
}

//---------------------------------------------------------------
// create a tab sheet
void FS_WindowAddTabSheet( FS_Window *win, FS_TabSheet *sheet )
{
	if( sheet && win )
	{
		if( FS_ListIsEmpty( &win->tab_book) )
		{
			win->focus_sheet = sheet;
		}
		sheet->pane.win = win;
		FS_ListAddTail( &win->tab_book, &sheet->list );
	}
}

FS_List * FS_WindowGetTabSheetWidgetList( FS_Window *win, FS_UINT4 index )
{
	FS_TabSheet *sheet;
	FS_List *node = win->tab_book.next;
	while( node != &win->tab_book )
	{
		sheet = FS_ListEntry( node, FS_TabSheet, list );
		if( sheet->id == index )
			return &sheet->pane.widget_list;
		node = node->next;
	}
	return FS_NULL;
}

void FS_WindowDelTabSheetWidgetList( FS_Window *win, FS_UINT4 id )
{
	FS_TabSheet *sheet;
	FS_List *node = win->tab_book.next;
	while( node != &win->tab_book ) {
		sheet = FS_ListEntry( node, FS_TabSheet, list );
		if( sheet->id == id ) {
			sheet->pane.focus_widget = FS_NULL;
			sheet->pane.rect.height = 0;
			sheet->pane.view_port.top = sheet->pane.rect.top;
			FS_RemoveWidgetList( &(sheet->pane.widget_list) );
			return;
		}
		node = node->next;
	}
}

FS_TabSheet * FS_WindowGetTabSheet( FS_Window *win, FS_UINT4 id )
{
	FS_TabSheet *sheet;
	if( win )
	{
		FS_List *node = win->tab_book.next;
		while( node != &win->tab_book )
		{
			sheet = FS_ListEntry( node, FS_TabSheet, list );
			if( sheet->id == id )
				return sheet;
			node = node->next;
		}
	}
	return FS_NULL;
}

void FS_WindowSetFocusTab( FS_Window *win, FS_UINT4 id )
{
	FS_TabSheet *sheet;
	FS_List *node = win->tab_book.next;
	while( node != &win->tab_book )
	{
		sheet = FS_ListEntry( node, FS_TabSheet, list );
		if( sheet->id == id )
		{
			win->focus_sheet = sheet;
			break;
		}
		node = node->next;
	}
}

void FS_WindowSetSheetCountPerPage( FS_Window *win, FS_SINT4 count )
{
	if ( win ) {
		win->sheets_per_page = count;
	}
}

//---------------------------------------------------------------
// set tooltip to button
void FS_WidgetSetTip( FS_Widget *btn, FS_CHAR *tip )
{
	if( btn )
		btn->tip = tip;
}

void FS_WidgetSetFocus( FS_Window *win, FS_Widget *wgt )
{
	FS_Widget *focusWgt = win->pane.focus_widget;

	if( focusWgt == FS_NULL && win->focus_sheet )
		focusWgt = win->focus_sheet->pane.focus_widget;

	if( focusWgt != FS_NULL && focusWgt != wgt )
	{
		FS_WGT_CLR_FOCUS( focusWgt );
		FS_WGT_SET_FOCUS( wgt );

		if( win->focus_sheet )
			win->focus_sheet->pane.focus_widget = wgt;
		else if( win->pane.focus_widget )
			win->pane.focus_widget = wgt;
	}
}

FS_BOOL FS_WindowIsTopMost( FS_UINT4 id )
{
	FS_Window *win = FS_GetTopMostWindow( );
	if( win && win->id == id )
		return FS_TRUE;
	else
		return FS_FALSE;
}

void FS_WindowReturnToId( FS_UINT4 id )
{
	FS_Window *win;

	if( FS_WindowFindId(id) == FS_NULL ) return;
	
	win = FS_GetTopMostWindow( );
	while( win && win->id != id ){
		FS_DestroyWindow( win );
		win = FS_GetTopMostWindow( );
	}
}

//---------------------------------------------------------------
// set text to widget , this function will malloc memory
void FS_WidgetSetText( FS_Widget *btn, FS_CHAR *txt )
{
	if( btn )
	{
		if( btn->type != FS_WGT_EDIT_BOX && btn->type != FS_WGT_SCROLLER )
		{
			if( txt && txt[0] )
			{
				FS_COPY_TEXT( btn->text, txt );
			}
			else
			{
				FS_SAFE_FREE( btn->text );
			}
		}
		else
		{	
			FS_SINT4 h;
			FS_SAFE_FREE( btn->text );
			
			h = btn->rect.height;
			if( txt && txt[0] )
			{
				btn->text = IFS_Strdup( txt );
				btn->rect.height = FS_CalcStringHeight( btn, 0 );
				if ( FS_WGT_GET_FORCE_MULTI_LINE(btn) )
				{
					if ( btn->rect.height < 2 * IFS_GetLineHeight() )
					{
						btn->rect.height = 2 * IFS_GetLineHeight( );
					}
				}
			}
			else
			{
				if ( FS_WGT_GET_FORCE_MULTI_LINE(btn) )
				{
					/* have multi line flag, at least need two line */
					btn->rect.height = 2 * IFS_GetLineHeight( );
				}
				else
				{
					btn->rect.height = IFS_GetLineHeight( );
				}
			}
			if ( btn->pane )
			{
				if ( btn->type == FS_WGT_EDIT_BOX )
				{
					btn->rect.height = FS_MIN( btn->pane->view_port.height, btn->rect.height );
				}
				FS_UpdatePaneWidgetsTop( btn->pane, btn->rect.top, btn->rect.height - h );
			}
		}
	}
}

void FS_WidgetSetSubText( FS_Widget *btn, FS_CHAR *txt )
{
	if( btn )
	{
		if( txt && txt[0] )
		{
			FS_COPY_TEXT( btn->sub_cap, txt );
		}
		else
		{
			FS_SAFE_FREE( btn->text );
		}
	}
}

void FS_WidgetSetImage( FS_Widget *wgt, FS_CHAR *file )
{
	FS_SINT4 w = 0, h = 0;

	FS_COPY_TEXT( wgt->file, file );
	if( wgt->im_handle )
	{
		FS_ImDestroy( wgt->im_handle );
		wgt->im_handle = 0;
	}
	if( wgt->file )
	{
		wgt->im_handle = FS_ImCreate( file, FS_WidgetDrawImage_CB, wgt );
		FS_ImGetSize( wgt->im_handle, &w, &h );
		FS_ImageScale( &w, &h, IFS_GetScreenWidth( ) - FS_BAR_W - IFS_GetWidgetSpan( ) );
	}
	
	if( h == 0 )
	{
		h = IFS_GetWidgetSpan( );
	}
	
	if ( wgt->pane )
	{
		FS_UpdatePaneWidgetsTop( wgt->pane, wgt->rect.top, h - wgt->rect.height );
	}
	wgt->rect.height = h;
	wgt->rect.width = w;
}

void FS_WidgetSetIcon( FS_Widget *wgt, FS_SINT2 icon )
{
	if( wgt )
	{
		if( wgt->icon )
			FS_ReleaseIcon( wgt->icon );
		wgt->icon = FS_Icon( icon );
	}
}

void FS_WidgetSetIconFile( FS_Widget *wgt, FS_CHAR *file )
{
	FS_SINT4 w = 0, h = 0;
	
	FS_COPY_TEXT( wgt->file, file );
	if( wgt->im_handle )
	{
		FS_ImDestroy( wgt->im_handle );
		wgt->im_handle = 0;
	}
	if( wgt->file )
	{
		wgt->im_handle = FS_ImCreate( file, FS_WidgetDrawImage_CB, wgt );
		FS_ImGetSize( wgt->im_handle, &w, &h );
		FS_ImageScale( &w, &h, wgt->rect.height - IFS_GetWidgetSpan() );
		if ( wgt->icon ) {
			FS_ReleaseIcon( wgt->icon );
			wgt->icon = FS_NULL;
		}
		wgt->icon = FS_ImDecode( wgt->im_handle, w, h );
	}
}


void FS_WidgetSetCheck( FS_Widget *wgt, FS_BOOL bCheck )
{
	if( wgt && wgt->type == FS_WGT_CHECK_BOX )
	{
		if( bCheck ) 
			FS_WGT_SET_CHECK(wgt);
		else
			FS_WGT_CLR_CHECK(wgt);
		FS_ReleaseIcon( wgt->icon );
		if( bCheck )
			wgt->icon = FS_Icon( FS_I_CHECK );
		else
			wgt->icon = FS_Icon( FS_I_UNCHECK );
	}
	else if( wgt && wgt->type == FS_WGT_RADIO_BOX )
	{
		FS_List *node;
		FS_Widget *item;
		node = wgt->pane->widget_list.next;
		while( node != &wgt->pane->widget_list )
		{
			item = FS_ListEntry( node, FS_Widget, list );
			if( item->type == FS_WGT_RADIO_BOX && FS_WGT_GET_CHECK(item) )
			{
				FS_ReleaseIcon( item->icon );
				item->icon = FS_Icon( FS_I_RADIO_UNCHECK );
				FS_WGT_CLR_CHECK( item );
			}
			node = node->next;
		}
		FS_WGT_SET_CHECK(wgt);
		FS_ReleaseIcon( wgt->icon );
		wgt->icon = FS_Icon( FS_I_RADIO_CHECK );
	}
}

void FS_RedrawWidget( FS_Window *win, FS_Widget *wgt )
{
	FS_Rect rect = FS_GetWidgetDrawRect( wgt );
	FS_UINT1 cType = FS_CLIP_NONE;
	
	if( wgt->pane )
	{
		if( wgt->pane->win->draw_status_bar )
			cType = FS_CLIP_CWSB;
		else
			cType = FS_CLIP_CLIENT;
	}
	
	if( wgt->type == FS_WGT_MENUITEM )
	{
		FS_InvalidateRect( win, &rect );
	}
	else
	{
		if( FS_PushClipRect( &rect, cType ) )
		{
			FS_FillRect( &rect, GFS_Skins[GFS_SkinIndex].bg );
			FS_PopClipRect( );
		}
		FS_DrawWidget( wgt );
		if( wgt->type == FS_WGT_IMAGE )
		{
			/* image is middle align. so we must expand the rect */
			rect.left = 0;
			rect.width = IFS_GetScreenWidth( ) - FS_BAR_W;
		}
		IFS_InvalidateRect( &rect );
	}
}

//---------------------------------------------------------------
// set button event handler
void FS_WidgetSetHandler( FS_Widget *btn, FS_WidgetEventHandler handle )
{
	if( btn )
		btn->handle = handle;
}

void FS_StandardKey3Handler( FS_Window *win )
{
	FS_DestroyWindow( win );
}

void FS_StandardKey2Handler( FS_Window *win )
{
	FS_Widget *wgt = FS_WindowGetFocusItem( win );
	if( wgt && wgt->handle )
		wgt->handle( win );
}

void FS_StandardMenuKey1Handler( FS_Window *win )
{
	FS_Widget *wgt = win->pane.focus_widget;
	if( wgt )
	{
		if( wgt->handle )
			wgt->handle( win );
	}
}

static void FS_StdEditBox_CB( FS_CHAR * text, FS_SINT4 len, void *param )
{
	FS_Window *win = ( FS_Window * )param;
	FS_Widget *txtBox = FS_NULL;
	FS_SINT4 h;
	FS_ScrollPane *pane;
	FS_BOOL change = FS_FALSE;
	
	if( win )
	{
		if( ! FS_ListIsEmpty( &win->pane.widget_list ) )
		{
			txtBox = win->pane.focus_widget;
			pane = &win->pane;
		}
		else if( ! FS_ListIsEmpty( &win->tab_book ) )
		{
			txtBox = win->focus_sheet->pane.focus_widget;
			pane = &win->focus_sheet->pane;
		}
		if( txtBox )
		{
			if( txtBox->text )
			{
				IFS_Free( txtBox->text );
				txtBox->text = FS_NULL;
			}
			
			h = txtBox->rect.height;
			if( text && text[0] )
			{
				txtBox->text = IFS_Strdup( text );
				txtBox->rect.height = FS_CalcStringHeight( txtBox, 0 );
				if ( FS_WGT_GET_FORCE_MULTI_LINE(txtBox) )
				{
					if ( txtBox->rect.height < 2 * IFS_GetLineHeight() )
					{
						txtBox->rect.height = 2 * IFS_GetLineHeight( );
					}
				}
			}
			else
			{
				if ( FS_WGT_GET_FORCE_MULTI_LINE(txtBox) )
				{
					/* have multi line flag, at least need two line */
					txtBox->rect.height = 2 * IFS_GetLineHeight( );
				}
				else
				{
					txtBox->rect.height = IFS_GetLineHeight( );
				}
			}
			if( txtBox->type == FS_WGT_EDIT_BOX )
				txtBox->rect.height = FS_MIN( txtBox->pane->view_port.height, txtBox->rect.height );
			change = FS_UpdatePaneWidgetsTop( pane, txtBox->rect.top, txtBox->rect.height - h );

			if( h == txtBox->rect.height )
				FS_RedrawWidget( win, txtBox );
			else
				FS_InvalidateRect( win, &win->client_rect );

			FS_SendMessage( win, FS_WM_COMMAND, FS_EV_ITEM_VALUE_CHANGE, (FS_UINT4)txtBox );
		}
	}
}

void FS_StdEditBoxHandler( FS_Window *win )
{
	FS_Widget *wgt;
	FS_SINT4 h;
	
	if( ! FS_ListIsEmpty( &win->pane.widget_list ) )
		wgt = win->pane.focus_widget;
	else if( ! FS_ListIsEmpty( &win->tab_book ) )
		wgt = win->focus_sheet->pane.focus_widget;
	
	if( wgt && FS_WGT_GET_CAN_WRITE(wgt) )
	{
		wgt->edit_param.rect = FS_GetWidgetDrawRect( wgt );
		if( wgt->edit_param.rect.top < IFS_GetWinTitleHeight( ) )
		{
			FS_WindowSetViewPort( win, wgt->rect.top );
			FS_InvalidateRect( win, FS_NULL );
			wgt->edit_param.rect = FS_GetWidgetDrawRect( wgt );
		}
		h = FS_MAX( 0, wgt->rect.top - wgt->pane->view_port.top );
		wgt->edit_param.rect.height = FS_MIN(wgt->edit_param.rect.height, wgt->pane->view_port.height - h );

		wgt->edit_param.rect.left += 1;
		wgt->edit_param.rect.width -= IFS_GetWidgetSpan();
		wgt->edit_param.rect.top += 1;
		wgt->edit_param.rect.height -= IFS_GetWidgetSpan();
		if( wgt->icon )
		{
			wgt->edit_param.rect.left += IFS_GetLineHeight( );
			wgt->edit_param.rect.width -= IFS_GetLineHeight( );
		}

		IFS_InputDialog( wgt->text, &wgt->edit_param, FS_StdEditBox_CB, win );
	}
}

void FS_StdShowDetail( FS_CHAR *title, FS_CHAR *info )
{
	FS_Window *win;
	FS_Widget *wInfo;
	FS_CHAR *ttl;
	
	ttl = title == FS_NULL ? FS_Text( FS_T_MSG ) : title;
	wInfo = FS_CreateScroller( 0, info );
	win = FS_CreateWindow( 0, ttl, FS_NULL );
	
	FS_WindowAddWidget( win, wInfo );
	FS_WindowSetSoftkey( win, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );

	FS_ShowWindow( win );
}

/* edit box in view mode, this can view the the detail information */
void FS_StdEditBoxViewHandler( FS_Window *win )
{
	FS_Widget *wgt = FS_WindowGetFocusItem( win );
	if( wgt && wgt->text )
	{
		FS_StdShowDetail( wgt->tip, wgt->text );
	}
}

//---------------------------------------------------------------
// render event to win proc, caller can overwrite its proc to handle message box event
void FS_StdMsgBoxKey1Handler( FS_Window *win )
{
	FS_UINT4 winid = win->id;
	FS_SendMessage( win, FS_WM_COMMAND, FS_EV_YES, 0 );
	if( FS_WindowFindId(winid) == win ){
		FS_DestroyWindow( win );
	}
}

//---------------------------------------------------------------
// render event to win proc, caller can overwrite its proc to handle message box event
void FS_StdMsgBoxKey3Handler( FS_Window *win )
{
	FS_UINT4 winid = win->id;
	FS_SendMessage( win, FS_WM_COMMAND, FS_EV_NO, 0 );
	if( FS_WindowFindId(winid) == win ){
		FS_DestroyWindow( win );
	}
}

void FS_StdTimerCbDestoryMsgBox( void *param )
{
	FS_Window *win = FS_WindowFindId( FS_W_MsgBoxFrm );
	if( win )
	{
		FS_DestroyWindow( win );
	}
}

//---------------------------------------------------------------
// standard message box
FS_Window * FS_MessageBox( FS_MsgBoxStyle style, FS_CHAR * txt, FS_WndProc proc, FS_BOOL autoclose )
{
	FS_Widget *wgt;
	FS_CHAR *key3 = FS_NULL, *key1 = FS_NULL;
	FS_SINT4 lines = 3;
	FS_Window *ret;
	FS_UINT4 winid = FS_W_MsgBoxFrm;
	
	if( style == FS_MS_NONE )
	{
		/* avoid the progress form pop up twice and splash the screen. */
		winid = FS_W_ProgressFrm;
		ret = FS_GetTopMostWindow( );
		if( ret && ret->id == winid )
			return ret;
	}
	
	ret = FS_CreateDialog( winid, FS_NULL, lines, proc );
	if( style == FS_MS_OK )
	{
		key3 = FS_Text(FS_T_OK);
		wgt = FS_CreateLabel( 0, txt, FS_I_INFO, 0 );
		FS_WindowSetTitle( ret, FS_Text(FS_T_MSG) );
	}
	else if( style == FS_MS_INFO_CANCEL )
	{
		key3 = FS_Text(FS_T_CANCEL);
		wgt = FS_CreateScroller( 0, txt );
		FS_WindowSetTitle( ret, FS_Text(FS_T_MSG) );
	}
	else if( style == FS_MS_ALERT )
	{
		key3 = FS_Text(FS_T_OK);
		wgt = FS_CreateLabel( 0, txt, FS_I_ALERT, 0 );
		FS_WindowSetTitle( ret, FS_Text(FS_T_MSG) );
	}
	else if( style == FS_MS_YES_NO )
	{
		key1 = FS_Text(FS_T_OK);
		key3 = FS_Text(FS_T_CANCEL);
		wgt = FS_CreateLabel( 0, txt, FS_I_QUESTION, 0 );
		FS_WindowSetTitle( ret, FS_Text(FS_T_MSG) );
	}
	else if( style == FS_MS_NONE )
	{
		wgt = FS_CreateLabel( 0, txt, FS_I_INFO, 0 );
		FS_WindowSetTitle( ret, FS_Text(FS_T_MSG) );
	}
	if( key3 )
	{
		FS_WindowSetSoftkey( ret, 3, key3, FS_StdMsgBoxKey3Handler );
	}
	if( key1 )
	{
		FS_WindowSetSoftkey( ret, 1, key1, FS_StdMsgBoxKey1Handler );
	}
	FS_WGT_SET_MULTI_LINE( wgt );
	FS_WindowAddWidget( ret, wgt );

	if( style == FS_MS_OK || style == FS_MS_ALERT )
	{
		if( autoclose ) 
			IFS_StartTimer( FS_TIMER_ID_GUI, FS_MSG_BOX_TIME, FS_StdTimerCbDestoryMsgBox, FS_NULL );
	}
	
	FS_ShowWindow( ret );
	return ret;
}

void FS_PrgDlgSetValue( FS_Window *win, FS_SINT4 val )
{
	FS_Widget *wPrg = FS_WindowGetWidget( win, FS_W_PRG_PRG );
	if( wPrg )
	{
		wPrg->prg_val = val;
		FS_RedrawWidget( win, wPrg );
	}
}

void FS_PrgDlgSetMax( FS_Window *win, FS_SINT4 max )
{
	FS_Widget *wPrg = FS_WindowGetWidget( win, FS_W_PRG_PRG );
	if( wPrg )
		wPrg->prg_max = max;
}

void FS_PrgDlgSetText( FS_Window *win, FS_CHAR * txt )
{
	FS_Widget *wTxt = FS_WindowGetWidget( win, FS_W_PRG_TEXT );
	if( wTxt )
	{
		FS_WidgetSetText( wTxt, txt );
		FS_RedrawWidget( win, wTxt );
	}
}

FS_Window *FS_PrograssDialog( FS_CHAR *title, FS_CHAR *txt, FS_SINT4 prg_max, FS_WndProc proc )
{
	FS_Widget *wTxt, *wPrg;
	FS_Window *ret = FS_CreateDialog( 0, title, 4, proc );
	wTxt = FS_CreateLabel( FS_W_PRG_TEXT, txt, FS_I_INFO, 0 );
	wPrg = FS_CreatePrograss( FS_W_PRG_PRG, FS_NULL );
	wPrg->prg_max = prg_max;
	
	FS_WindowAddWidget( ret, wPrg );
	FS_WindowAddWidget( ret, wTxt );
	FS_WindowSetSoftkey( ret, 3, FS_Text(FS_T_CANCEL), FS_StandardKey3Handler );
	FS_ShowWindow( ret );
	return ret;
}

void FS_MsgBoxSetText( FS_Window *win, FS_CHAR *str )
{
	FS_Widget *wgt;
	if( win && str )
	{
		wgt = FS_ListEntry( win->pane.widget_list.next, FS_Widget, list );
		FS_WidgetSetText( wgt, str );
		FS_WidgetCalcHeight( wgt );
		FS_InvalidateRect( win, &win->client_rect );
	}
}

static FS_BOOL FS_StdExitCnf_CB( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	if( wparam == FS_EV_YES )	// exit without save account data
	{
		FS_Window *dform = FS_WindowFindId( win->private_data );
		FS_DestroyWindow( dform );
		ret = FS_TRUE;
	}
	return ret;
}

void FS_StdExitWithoutSaveCnfHandler( FS_Window *win )
{
	FS_Window *msgBox;
	msgBox = FS_MessageBox( FS_MS_YES_NO,
		FS_Text(FS_T_EXIT_WITHOUT_SAVE), FS_StdExitCnf_CB, FS_FALSE );
	msgBox->private_data = win->id;
}

static void FS_ThemeSettingSave( FS_Window *win )
{
	FS_FileWrite( FS_DIR_ROOT, FS_THEME_FILE, 0, GFS_Skins, sizeof(GFS_Skins) );
	FS_FileWrite( FS_DIR_ROOT, FS_THEME_FILE, sizeof(GFS_Skins), &GFS_SkinIndex, sizeof(GFS_SkinIndex) );
	FS_DestroyWindow( win );
}

static void FS_ThemeSettingRestore( FS_Window *win )
{
	if( FS_FileRead( FS_DIR_ROOT, FS_THEME_FILE, 0, GFS_Skins, sizeof(GFS_Skins) ) == sizeof(GFS_Skins) ){
		FS_FileRead( FS_DIR_ROOT, FS_THEME_FILE, sizeof(GFS_Skins), &GFS_SkinIndex, sizeof(GFS_SkinIndex) );
	}else{
		FS_GuiInit( );
	}
	FS_DestroyWindow( win );
}

void FS_ThemeSetting_UI( FS_Window *win )
{
	FS_Widget *wBlue, *wRed, *wGreen, *wGray, *wYellow, *w3D, *wRight;
	FS_Window *twin = FS_CreateWindow( 0, FS_Text(FS_T_ABOUT), FS_NULL );
	/* use id as skin's index. must follow up GFS_Skins define */
	wBlue = FS_CreateRadioBox( 0, FS_Text(FS_T_BLUE_SKY) );
	wRed = FS_CreateRadioBox( 1, FS_Text(FS_T_RED_RECALL) );
	wGreen = FS_CreateRadioBox( 2, FS_Text(FS_T_GREEN_GRASS) );
	wGray = FS_CreateRadioBox( 3, FS_Text(FS_T_GRAY_CLASSICAL) );
	wYellow = FS_CreateRadioBox( 4, FS_Text(FS_T_YELLOW_STORM) );
	w3D = FS_CreateCheckBox( FS_W_Theme3D, FS_Text(FS_T_3D_EFFECT) );
	wRight = FS_CreateLabel( 0, FS_Text(FS_T_COPY_RIGHT), 0, 1 );
	FS_WidgetSetHandler( wBlue, FS_ThemeSetSkin_CB );
	FS_WidgetSetHandler( wRed, FS_ThemeSetSkin_CB );
	FS_WidgetSetHandler( wGreen, FS_ThemeSetSkin_CB );
	FS_WidgetSetHandler( wGray, FS_ThemeSetSkin_CB );
	FS_WidgetSetHandler( wYellow, FS_ThemeSetSkin_CB );
	FS_WidgetSetHandler( w3D, FS_ThemeSet3DEffect_CB );
	
	FS_WindowAddWidget( twin, wBlue );
	FS_WindowAddWidget( twin, wRed );
	FS_WindowAddWidget( twin, wGreen );
	FS_WindowAddWidget( twin, wGray );
	FS_WindowAddWidget( twin, wYellow );
	FS_WindowAddWidget( twin, w3D );
	FS_WindowAddWidget( twin, wRight );
	
	if( GFS_SkinIndex == 0 )
		FS_WidgetSetCheck( wBlue, FS_TRUE );
	else if( GFS_SkinIndex == 1 )
		FS_WidgetSetCheck( wRed, FS_TRUE );
	else if( GFS_SkinIndex == 2 )
		FS_WidgetSetCheck( wGreen, FS_TRUE );
	else if( GFS_SkinIndex == 3 )
		FS_WidgetSetCheck( wGray, FS_TRUE );
	else
		FS_WidgetSetCheck( wYellow, FS_TRUE );
	
	FS_WidgetSetCheck( w3D, GFS_Skins[GFS_SkinIndex].effect_3d );
	
	FS_WindowSetSoftkey( twin, 1, FS_Text(FS_T_SAVE), FS_ThemeSettingSave );
	FS_WindowSetSoftkey( twin, 3, FS_Text(FS_T_CANCEL), FS_ThemeSettingRestore );
	FS_ShowWindow( twin );
	
	FS_DestroyWindowByID( FS_W_WebMainMenu );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

//---------------------------------------------------------------
// INTERFACE, call by platform, to handle key event
void FS_SendKeyEvent( FS_SINT4 event )
{
	FS_Window * win = FS_GetTopMostWindow( );
	if( win )
	{
		if( win->disable_func )
		{
			if( event != FS_SOFTKEY1 && event != FS_SOFTKEY2 )
				FS_WindowKeyEvent( win, event );
		}
		else
		{
			FS_WindowKeyEvent( win, event );
		}
	}
}
//---------------------------------------------------------------
// INTERFACE, call by platform, to handle mouse event
void FS_SendMouseEvent( FS_SINT4 x, FS_SINT4 y )
{
	FS_Window * win = FS_GetTopMostWindow( );
	if( win )
	{
		if( y > IFS_GetScreenHeight( ) ) return;
		
		if( win->disable_func )
		{
			if( win->softkeys[2] && FS_HitButton(win->softkeys[2], x, y) )
				FS_WindowMouseEvent( win, x, y );
		}
		else
		{
			FS_WindowMouseEvent( win, x, y );
		}
	}
}

void FS_GuiRepaint( void )
{
	FS_BOOL repaint = FS_FALSE;
	FS_List *node;
	FS_Window * win = FS_GetTopMostWindow( );
	FS_Window *fullWin = FS_GetTopFullScreenWindow( );
	if( win )
	{
		if( fullWin )
		{
			if( FS_PushClipRect( &fullWin->rect, FS_CLIP_NONE ) )
			{
				FS_DrawWindow( fullWin );
				FS_PopClipRect( );
			}
			repaint = FS_TRUE;
		}

		if( win != fullWin )
		{
			node = fullWin->list.next;
			while( node != &GFS_WinList ){
				win = FS_ListEntry( node, FS_Window, list );
				node = node->next;
				
				FS_DrawWindow( win );
			}
			repaint = FS_TRUE;
		}

		if( repaint )
		{
			IFS_InvalidateRect( FS_NULL );
		}
	}
}

void FS_GuiExit( void )
{
	FS_Window *win;
	while( (win = FS_GetTopMostWindow() ) )
	{
		FS_DestroyWindow( win );
	}
}

void FS_GuiDeinit( void )
{
	FS_DeinitGrpLib( );		
}

void FS_GuiInit( void )
{
	FS_InitGrpLib( );
	
	if( FS_FileRead( FS_DIR_ROOT, FS_THEME_FILE, 0, GFS_Skins, sizeof(GFS_Skins) ) != sizeof(GFS_Skins) 
		|| FS_FileRead( FS_DIR_ROOT, FS_THEME_FILE, sizeof(GFS_Skins), &GFS_SkinIndex, sizeof(GFS_SkinIndex) ) != sizeof(GFS_SkinIndex))
	{
		/* blue */
		GFS_Skins[0].fg = IFS_DDB_RGB(0x00, 0x00, 0x00);
		GFS_Skins[0].bg = IFS_DDB_RGB(0xFF, 0xFF, 0xFF);
		GFS_Skins[0].focus_bg = IFS_DDB_RGB(122, 153, 242);
		GFS_Skins[0].focus_text = IFS_DDB_RGB(12, 56, 88);
		GFS_Skins[0].focus_link = IFS_DDB_RGB(0xFF, 0xFF, 0xFF);
		GFS_Skins[0].focus_link_bg = IFS_DDB_RGB(0, 0, 0xFF);
		GFS_Skins[0].effect_3d = FS_TRUE;
		
		/* pink */
		GFS_Skins[1].fg = IFS_DDB_RGB(0x00, 0x00, 0x00);
		GFS_Skins[1].bg = IFS_DDB_RGB(0xFF, 0xFF, 0xFF);
		GFS_Skins[1].focus_bg = IFS_DDB_RGB(255, 146, 163);
		GFS_Skins[1].focus_text = IFS_DDB_RGB(0x00, 0x80, 0x40);
		GFS_Skins[1].focus_link = IFS_DDB_RGB(0xFF, 0xFF, 0xFF);
		GFS_Skins[1].focus_link_bg = IFS_DDB_RGB(0, 0, 0xFF);
		GFS_Skins[1].effect_3d = FS_FALSE;
		
		/* green */
		GFS_Skins[2].fg = IFS_DDB_RGB(0x00, 0x00, 0x00);
		GFS_Skins[2].bg = IFS_DDB_RGB(0xFF, 0xFF, 0xFF);
		GFS_Skins[2].focus_bg = IFS_DDB_RGB(192, 231, 161);
		GFS_Skins[2].focus_text = IFS_DDB_RGB(12, 56, 88);
		GFS_Skins[2].focus_link = IFS_DDB_RGB(0xFF, 0xFF, 0xFF);
		GFS_Skins[2].focus_link_bg = IFS_DDB_RGB(0, 0, 0xFF);
		GFS_Skins[2].effect_3d = FS_FALSE;
		
		/* gray */
		GFS_Skins[3].fg = IFS_DDB_RGB(0x00, 0x00, 0x00);
		GFS_Skins[3].bg = IFS_DDB_RGB(0xFF, 0xFF, 0xFF);
		GFS_Skins[3].focus_bg = IFS_DDB_RGB(204, 204, 204);
		GFS_Skins[3].focus_text = IFS_DDB_RGB(0x00, 0x80, 0x40);
		GFS_Skins[3].focus_link = IFS_DDB_RGB(0xFF, 0xFF, 0xFF);
		GFS_Skins[3].focus_link_bg = IFS_DDB_RGB(0x00, 0x00, 0xFF);
		GFS_Skins[3].effect_3d = FS_FALSE;

		/* orange */
		GFS_Skins[4].fg = IFS_DDB_RGB(0x00, 0x00, 0x00);
		GFS_Skins[4].bg = IFS_DDB_RGB(0xFF, 0xFF, 0xFF);
		GFS_Skins[4].focus_bg = IFS_DDB_RGB(248, 181, 121);
		GFS_Skins[4].focus_text = IFS_DDB_RGB(0x00, 0x80, 0x40);
		GFS_Skins[4].focus_link = IFS_DDB_RGB(0xFF, 0xFF, 0xFF);
		GFS_Skins[4].focus_link_bg = IFS_DDB_RGB(0, 0, 0xFF);
		GFS_Skins[4].effect_3d = FS_FALSE;
		
		GFS_SkinIndex = 2;
	}
}

