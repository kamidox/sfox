#ifndef _FS_GUI_H_
#define _FS_GUI_H_

#include "inc\FS_Config.h"
#include "inc\util\FS_List.h"
#include "inc\util\FS_Image.h"
#include "inc\util\FS_File.h"

#define FS_BAR_W				10				// scroll bar width
#define FS_WIN_BTN_X			71				// headline button begin position

#define FS_WIN_KEY_NUM			3

#define FS_MSG_BOX_TIME			3000

#define FS_DEFAULT_EDIT_LEN 			1024
#define FS_DETAULT_SCROLL_EDIT_LEN		(16 * 1024)

struct FS_ScrollPane_Tag;
struct FS_Window_Tag;
struct FS_WebWgt_Tag;

typedef struct FS_GuiSkin_Tag
{
	FS_COLOR		fg;
	FS_COLOR		bg;
	FS_COLOR		focus_bg;
	FS_COLOR		focus_text;
	FS_COLOR		focus_link_bg;
	FS_COLOR		focus_link;
	FS_BOOL 		effect_3d;
}FS_GuiSkin;

typedef void ( * FS_WidgetEventHandler )( struct FS_Window_Tag *win );

typedef enum FS_WidgetType_Tag
{
	FS_WGT_AUTO = 0,	// auto type when it's add to window
	FS_WGT_BUTTON,
	FS_WGT_MENUITEM,
	FS_WGT_LISTITEM,
	FS_WGT_EDIT_BOX,
	FS_WGT_COMBO_BOX,
	FS_WGT_LABEL,
	FS_WGT_SCROLLER,
	FS_WGT_RADIO_BOX,
	FS_WGT_CHECK_BOX,
	FS_WGT_PROGRASS,
	FS_WGT_IMAGE,
	FS_WGT_KEY
}FS_WidgetType;

// GUI type
typedef enum FS_WIN_TYPE_TAG
{
	FS_WT_WINDOW = 1,
	FS_WT_WEB,
	FS_WT_MENU,
	FS_WT_POPUP_MENU,
	FS_WT_DIALOG,
}FS_WinType;

// message box style
typedef enum FS_MsgBoxStyle_Tag
{
	FS_MS_OK = 1,
	FS_MS_ALERT,
	FS_MS_INFO_CANCEL,
	FS_MS_YES_NO,
	FS_MS_NONE		/* modle, no softkey */
}FS_MsgBoxStyle;

typedef enum FS_WndMess_Tag
{
	FS_WM_COMMAND = 0,
	FS_WM_DESTROY,
	FS_WM_SETFOCUS,			// window get focus
	FS_WM_LOSTFOCUS,
	FS_WM_PAINT,
	FS_WM_NUM = 0x8000
}FS_WndMess;

typedef enum FS_Event_Tag
{
	FS_EV_OK = 0x8000,
	FS_EV_CANCEL,
	FS_EV_YES,
	FS_EV_NO,
	FS_EV_ITEM_VALUE_CHANGE,
	FS_EV_TAB_FOCUS_CHANGE,
}FS_Event;

#define FS_WGT_CAN_WRITE			0x01
#define FS_WGT_MARK_CHAR			0x02
#define FS_WGT_MULTI_LINE			0x04
#define FS_WGT_ENABLE				0x08
#define FS_WGT_FOCUS				0x10
#define FS_WGT_CHECK				0x20
#define FS_WGT_SUB_MENU_FLAG		0x40
#define FS_WGT_NO_INDEX_MENU_FLAG	0x80
#define FS_WGT_RED_FLAG				0x100
#define FS_WGT_GREEN_FLAG			0x200
#define FS_WGT_FORCE_NO_STATUS_BAR	0x400
#define FS_WGT_FORCE_MULTI_LINE		0x800
#define FS_WGT_SHARE_HEIGHT			0x1000		/* share the same height to previous widget, not only available for list item */
#define FS_WGT_DRAW_BORDER			0x2000
#define FS_WGT_ALIGN_CENTER			0x4000

#define FS_WGT_SET_CAN_WRITE(wgt)			((wgt)->flag |= FS_WGT_CAN_WRITE)
#define FS_WGT_GET_CAN_WRITE(wgt)			(((wgt)->flag) & FS_WGT_CAN_WRITE)
#define FS_WGT_CLR_CAN_WRITE(wgt)			((wgt)->flag &= (~FS_WGT_CAN_WRITE))
#define FS_WGT_SET_MARK_CHAR(wgt)			((wgt)->flag |= FS_WGT_MARK_CHAR)
#define FS_WGT_GET_MARK_CHAR(wgt)			(((wgt)->flag) & FS_WGT_MARK_CHAR)
#define FS_WGT_SET_MULTI_LINE(wgt)			((wgt)->flag |= FS_WGT_MULTI_LINE)
#define FS_WGT_GET_MULTI_LINE(wgt)			(((wgt)->flag) & FS_WGT_MULTI_LINE)
#define FS_WGT_CLR_MULTI_LINE(wgt)			((wgt)->flag &= (~FS_WGT_MULTI_LINE))
#define FS_WGT_SET_ENABLE(wgt)				((wgt)->flag |= FS_WGT_ENABLE)
#define FS_WGT_GET_ENABLE(wgt)				(((wgt)->flag) & FS_WGT_ENABLE)
#define FS_WGT_SET_FOCUS(wgt)				((wgt)->flag |= FS_WGT_FOCUS)
#define FS_WGT_GET_FOCUS(wgt)				(((wgt)->flag) & FS_WGT_FOCUS)
#define FS_WGT_CLR_FOCUS(wgt)				((wgt)->flag &= (~FS_WGT_FOCUS))
#define FS_WGT_SET_CHECK(wgt)				((wgt)->flag |= FS_WGT_CHECK)
#define FS_WGT_GET_CHECK(wgt)				(((wgt)->flag) & FS_WGT_CHECK)
#define FS_WGT_CLR_CHECK(wgt)				((wgt)->flag &= (~FS_WGT_CHECK))
#define FS_WGT_SET_SUB_MENU_FLAG(wgt)		((wgt)->flag |= FS_WGT_SUB_MENU_FLAG)
#define FS_WGT_GET_SUB_MENU_FLAG(wgt)		(((wgt)->flag) & FS_WGT_SUB_MENU_FLAG)
#define FS_WGT_SET_NO_INDEX_MENU_FLAG(wgt)	((wgt)->flag |= FS_WGT_NO_INDEX_MENU_FLAG)
#define FS_WGT_GET_NO_INDEX_MENU_FLAG(wgt)	(((wgt)->flag) & FS_WGT_NO_INDEX_MENU_FLAG)
#define FS_WGT_SET_RED_FLAG(wgt)			((wgt)->flag |= FS_WGT_RED_FLAG)
#define FS_WGT_GET_RED_FLAG(wgt)			(((wgt)->flag) & FS_WGT_RED_FLAG)
#define FS_WGT_CLR_RED_FLAG(wgt)			((wgt)->flag &= (~FS_WGT_RED_FLAG))
#define FS_WGT_SET_GREEN_FLAG(wgt)			((wgt)->flag |= FS_WGT_GREEN_FLAG)
#define FS_WGT_GET_GREEN_FLAG(wgt)			(((wgt)->flag) & FS_WGT_GREEN_FLAG)
#define FS_WGT_CLR_GREEN_FLAG(wgt)			((wgt)->flag &= (~FS_WGT_GREEN_FLAG))
#define FS_WGT_GET_FORCE_NO_STATUS_BAR(wgt)		(((wgt)->flag) & FS_WGT_FORCE_NO_STATUS_BAR)
#define FS_WGT_SET_FORCE_NO_STATUS_BAR(wgt)		((wgt)->flag |= FS_WGT_FORCE_NO_STATUS_BAR)
#define FS_WGT_GET_FORCE_MULTI_LINE(wgt)		(((wgt)->flag) & FS_WGT_FORCE_MULTI_LINE)
#define FS_WGT_SET_FORCE_MULTI_LINE(wgt)		((wgt)->flag |= FS_WGT_FORCE_MULTI_LINE)
#define FS_WGT_GET_SHARE_HEIGHT(wgt)			(((wgt)->flag) & FS_WGT_SHARE_HEIGHT)
#define FS_WGT_SET_SHARE_HEIGHT(wgt)			((wgt)->flag |= FS_WGT_SHARE_HEIGHT)
#define FS_WGT_GET_DRAW_BORDER(wgt)				(((wgt)->flag) & FS_WGT_DRAW_BORDER)
#define FS_WGT_SET_DRAW_BORDER(wgt)				((wgt)->flag |= FS_WGT_DRAW_BORDER)
#define FS_WGT_GET_ALIGN_CENTER(wgt)				(((wgt)->flag) & FS_WGT_ALIGN_CENTER)
#define FS_WGT_SET_ALIGN_CENTER(wgt)				((wgt)->flag |= FS_WGT_ALIGN_CENTER)

// structure for button and menu item
typedef struct FS_Widget_Tag
{
	FS_List						list;		// internal use, link each button
	FS_WidgetType				type;		// widget type, editbox, menuitem, button, softkey etc.
	FS_CHAR *					text;	
	FS_CHAR *					sub_cap;	// for listitem the second line text
	FS_CHAR *					file;
	FS_EditParam				edit_param;		// edit param for edit box
	FS_CHAR *					extra_text;		// for list item to show extra infomation. will free this field
	FS_CHAR *					tip;
	FS_Bitmap *					icon;
	FS_Rect						rect;
	FS_WidgetEventHandler		handle;	
	FS_ImHandle					im_handle;	/* for image widget */

	FS_UINT4					flag;
	FS_SINT4					prg_val;	/* prograss bar cur value */
	FS_SINT4					prg_max;	/* prograss bar max value */
	
	FS_UINT4					id;
	FS_CHAR *					data;		// will free when widget destroy
	FS_UINT4					private_data;
	struct FS_ScrollPane_Tag *	pane;		// widget belong to this pane
}FS_Widget;

typedef struct FS_ScrollBar_Tag
{
	FS_Rect			rect;
	FS_SINT4		box_height;
	FS_SINT4		box_top;
}FS_ScrollBar;

// scroll pane
typedef struct FS_ScrollPane_Tag
{
	FS_Rect				rect;			// scroll pane rect
	FS_Rect				view_port;		// scroll pane view port
	FS_List				widget_list;	// widgets list
	FS_Widget *			focus_widget;	// which widget own the focus in widget list

	struct FS_WebWgt_Tag *	focus_wwgt;

	FS_BOOL				cyc;			// circal scroll
	
	FS_BOOL				draw_scroll_bar;		// whether to draw scroll bar
	FS_ScrollBar		bar;

	struct FS_Window_Tag *win;
}FS_ScrollPane;

typedef struct FS_TabSheet_Tag
{
	FS_List				list;
	FS_UINT4			id;
	FS_CHAR *			title;
	FS_ScrollPane		pane;
	FS_BOOL				draw_status_bar;
	FS_Bitmap *			icon;
}FS_TabSheet;

typedef struct FS_WebSound_Tag
{
	FS_BOOL			is_playing;
	FS_CHAR			dname[FS_FILE_NAME_LEN];
	FS_CHAR			file[FS_FILE_NAME_LEN];
}FS_WebSound;

// window event handler
typedef FS_BOOL ( * FS_WndProc )( void *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam );

/* web win context */
typedef struct FS_WebContext_Tag
{
	FS_List 			form_list;
	FS_List				task_list;
	FS_List				image_list;
	FS_WebSound			bgsound;		/* background sound */
	FS_BOOL				url_var;		/* url contains var. for wml doc */
	FS_BOOL				is_start_page;
	FS_CHAR *			title;
	/* for layout */
	FS_SINT4			line_h;			/* max line height. for layout */
	FS_SINT4			cur_x;
	FS_SINT4			cur_y;
}FS_WebContext;

typedef struct FS_Window_Tag
{
	// private members, internal use
	FS_List				list;		// internal use, for window mamange

	FS_WinType			type;			// GUI type, window or menu or dialog etc.
	FS_Rect				rect;			// position

	// public members
	FS_CHAR *			title;			// window title
	FS_List				btn_list;		// window headline button list
	FS_Widget *			focus_btn;		// for window, current focus button
	
	// window content
	FS_Rect				client_rect;	// window client rect
	FS_ScrollPane		pane;			// window content

	FS_List				tab_book;		// tab book widget, for eml edit etc. store FS_TabSheet widget
	FS_TabSheet *		focus_sheet;	// focus tab sheet
	FS_SINT4			sheets_per_page;// sheets in one page
	FS_BOOL				draw_sheet_arrows;
	FS_WebContext		context;		/* web win context */
	
	FS_BOOL				disable_func;	/* only allow key3 to handle event */
	// window status bar
	FS_BOOL				draw_status_bar;
	FS_Rect				status_bar;
	// window list index
	FS_BOOL				show_index;
	FS_SINT4			item_index;
	FS_SINT4			item_total;
	// window softkeys
	FS_Widget *			softkeys[FS_WIN_KEY_NUM];	// window softkey button list
	FS_UINT4			id;				// GUI id
	FS_WndProc			proc;
	FS_UINT4			private_data;	// private data
	FS_UINT4			private_data2;		// other private data
}FS_Window;

FS_Widget * FS_CreateWidget( FS_WidgetType type, FS_UINT4 id, FS_CHAR *text, 
	FS_CHAR *sub_cap, FS_CHAR *file, FS_SINT2 iconid, FS_UINT1 nLines, FS_EditParam *eParam );

void FS_WidgetSetExtraText( FS_Widget *wgt, FS_CHAR *txt );

void FS_WidgetSetHandler( FS_Widget *btn, FS_WidgetEventHandler handle );

void FS_WidgetSetTip( FS_Widget *btn, FS_CHAR *tip );

void FS_WidgetSetCheck( FS_Widget *wgt, FS_BOOL bCheck );

void FS_WidgetSetText( FS_Widget *btn, FS_CHAR *txt );

void FS_WidgetSetSubText( FS_Widget *btn, FS_CHAR *txt );

void FS_WidgetSetImage( FS_Widget *wgt, FS_CHAR *file );

void FS_WidgetSetFocus( FS_Window *win, FS_Widget *wgt );

void FS_WidgetSetIcon( FS_Widget *wgt, FS_SINT2 icon );

void FS_WidgetSetIconFile( FS_Widget *wgt, FS_CHAR *file );

void FS_WindowSetTitle( FS_Window *win, FS_CHAR *title );

void FS_WindowSetViewPort( FS_Window *win, FS_SINT4 vp );

void FS_WindowSetSoftkey( FS_Window *win, FS_BYTE pos, FS_CHAR *text, FS_WidgetEventHandler handle );

void FS_MenuSetSoftkey( FS_Window *win );

void FS_RedrawSoftkeys( FS_Window *win );

void FS_RedrawWinTitle( FS_Window *win );

void FS_RedrawWinStatusBar( FS_Window *win );

void FS_DestroyWindow( FS_Window *win );

void FS_DestroyWindowByID( FS_SINT4 winId );

void FS_ShowWindow( FS_Window *win );

FS_BOOL FS_SendMessage( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam );

FS_Window * FS_CreateWindow( FS_UINT4 id, FS_CHAR * title, FS_WndProc proc );

FS_Window * FS_CreateMenu( FS_UINT4 id, FS_SINT4 nItems );

FS_Window * FS_CreatePopUpMenu( FS_UINT4 id, FS_Rect *curRect, FS_SINT4 nItems );

void FS_MenuAddItem( FS_Window *win, FS_Widget *item );

FS_List * FS_WindowGetListItems( FS_Window *win );
	
void FS_WindowDelWidget( FS_Window *win, FS_Widget *item );

FS_BOOL FS_WindowIsTopMost( FS_UINT4 id );

FS_Window *FS_GetTopFullScreenWindow( void );

void FS_WindowReturnToId( FS_UINT4 id );

FS_CHAR * FS_WindowGetWidgetText( FS_Window *win, FS_UINT4 index );

FS_Widget * FS_WindowGetWidget( FS_Window *win, FS_UINT4 index );

FS_Widget * FS_WindowGetWidgetByPrivateData( FS_Window *win, FS_UINT4 private_data );

FS_List * FS_WindowGetTabSheetWidgetList( FS_Window *win, FS_UINT4 index );

void FS_WindowDelTabSheetWidgetList( FS_Window *win, FS_UINT4 tab_id );

FS_TabSheet * FS_WindowGetTabSheet( FS_Window *win, FS_UINT4 id );

void FS_WindowDelWidgetList( FS_Window *win );

FS_Widget * FS_WindowGetFocusItem( FS_Window *win );

FS_Window * FS_WindowFindId( FS_UINT4 id );

FS_Window * FS_WindowFindPtr( FS_Window *pWin );

void FS_WindowMoveTop( FS_Window *win );

void FS_WindowMoveBottom( FS_Window *win );

FS_TabSheet * FS_CreateTabSheet( FS_UINT4 id, FS_CHAR *title, FS_SINT2 icon_id, FS_BOOL bShowStatusBar );

void FS_WidgetCalcHeight( FS_Widget *wgt );

void FS_WindowAddTabSheet( FS_Window *win, FS_TabSheet *sheet );

void FS_WindowSetFocusTab( FS_Window *win, FS_UINT4 id );

void FS_SendKeyEvent( FS_SINT4 event );

void FS_SendMouseEvent( FS_SINT4 x, FS_SINT4 y );

void FS_InvalidateRect( FS_Window *win, FS_Rect *pRect );

void FS_RedrawWidget( FS_Window *win, FS_Widget *wgt );

FS_Rect FS_GetWidgetDrawRect( FS_Widget *wgt );

// standard gui object or handler
void FS_StandardKey3Handler( FS_Window *win );

void FS_StandardKey2Handler( FS_Window *win );

void FS_StandardMenuKey1Handler( FS_Window *win );

void FS_StdEditBoxHandler( FS_Window *win );

void FS_StdShowDetail( FS_CHAR *title, FS_CHAR *info );

void FS_StdEditBoxViewHandler( FS_Window *win );

void FS_StdTimerCbDestoryMsgBox( void *param );

FS_Window * FS_MessageBox( FS_MsgBoxStyle style, FS_CHAR * txt, FS_WndProc proc, FS_BOOL autoclose );

void FS_MsgBoxSetText( FS_Window *win, FS_CHAR *str );

FS_Window *FS_PrograssDialog( FS_CHAR *title, FS_CHAR *txt, FS_SINT4 prg_max, FS_WndProc proc );

void FS_PrgDlgSetValue( FS_Window *win, FS_SINT4 val );

void FS_PrgDlgSetMax( FS_Window *win, FS_SINT4 val );

void FS_PrgDlgSetText( FS_Window *win, FS_CHAR * txt );

void FS_StdExitWithoutSaveCnfHandler( FS_Window *win );

void FS_WindowAddWidget( FS_Window *win, FS_Widget *wgt );

void FS_TabSheetAddWidget( FS_TabSheet *sheet, FS_Widget *wgt );

void FS_WindowSetListIndex( FS_Window *win, FS_SINT4 index, FS_SINT4 total );

void FS_ThemeSetting_UI( FS_Window *win );

void FS_GuiInit( void );

void FS_GuiDeinit( void );

void FS_GuiExit( void );

FS_Window * FS_GetTopMostWindow( void );

void FS_WindowSetSheetCountPerPage( FS_Window *win, FS_SINT4 count );

/*-------------------------------------------------------------------------------------*/
#define FS_CreateMenuItem( id, text )	\
	FS_CreateWidget( FS_WGT_MENUITEM, id, text, FS_NULL, FS_NULL, 0, 1, FS_NULL )
	
#define FS_CreateEditBox( id, text, icon, lines, eParam )	\
	FS_CreateWidget( FS_WGT_EDIT_BOX, id, text, FS_NULL, FS_NULL, icon, lines, eParam )
	
#define FS_CreateLabel( id, text, icon, lines )	\
	FS_CreateWidget( FS_WGT_LABEL, id, text, FS_NULL, FS_NULL, icon, lines, FS_NULL )
	
#define FS_CreateScroller( id, text )	\
	FS_CreateWidget( FS_WGT_SCROLLER, id, text, FS_NULL, FS_NULL, FS_NULL, 0, FS_NULL )
	
#define FS_CreateButton( id, text, icon, lines )	\
	FS_CreateWidget( FS_WGT_BUTTON, id, text, FS_NULL, FS_NULL, icon, lines, FS_NULL )

#define FS_CreateListItem( id, text, sub_cap, icon, lines )	\
	FS_CreateWidget( FS_WGT_LISTITEM, id, text, sub_cap, FS_NULL, icon, lines, FS_NULL )

#define FS_CreateComboBox( id, text, icon )	\
	FS_CreateWidget( FS_WGT_COMBO_BOX, id, text, FS_NULL, FS_NULL, icon, 1, FS_NULL )

#define FS_CreateKey( id, text )	\
	FS_CreateWidget( FS_WGT_KEY, id, text, FS_NULL, FS_NULL, FS_NULL, 1, FS_NULL )
	
#define FS_CreateCheckBox( id, text ) \
		FS_CreateWidget( FS_WGT_CHECK_BOX, id, text, FS_NULL, FS_NULL, FS_NULL, 1, FS_NULL )
		
#define FS_CreateRadioBox( id, text ) \
		FS_CreateWidget( FS_WGT_RADIO_BOX, id, text, FS_NULL, FS_NULL, FS_NULL, 1, FS_NULL )

#define FS_CreatePrograss( id, text ) \
		FS_CreateWidget( FS_WGT_PROGRASS, id, text, FS_NULL, FS_NULL, FS_NULL, 1, FS_NULL )

#define FS_CreateImage( id, file ) \
		FS_CreateWidget( FS_WGT_IMAGE, id, FS_NULL, FS_NULL, file, FS_NULL, 1, FS_NULL )
	
#define FS_ComboBoxSetTitle( wgt, title ) FS_WidgetSetExtraText( wgt, title )

#endif
