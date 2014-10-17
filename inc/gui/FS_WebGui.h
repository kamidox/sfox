#ifndef _FS_WEB_GUI_H_
#define _FS_WEB_GUI_H_

#include "inc/gui/FS_Gui.h"
#include "inc/util/FS_File.h"
#include "inc/util/FS_Image.h"

/* script field is not defined yet */
#define FS_WFT_HTML 		0	/* normal html form */
#define FS_WFT_ANCHOR		1	/* WML anchor form	*/

typedef struct FS_WebForm_Tag
{
	FS_List 		list;
	
	FS_UINT1		type;
	FS_CHAR *		action;
	FS_CHAR *		method;
	FS_CHAR *		enc_type;
	FS_CHAR *		accept;
	FS_CHAR *		accept_charset;
	FS_CHAR *		name;
	FS_BOOL			send_referer;
	FS_List 		input_list; 	/* all element in this form (FS_WebWgt) or FS_PostField */
}FS_WebForm;

typedef struct FS_WebOption_Tag
{
	FS_List 			list;
	FS_BOOL				selected;
	FS_BOOL				disabled;
	FS_CHAR *			value;
	FS_CHAR *			label;
	FS_CHAR *			on_pick;
}FS_WebOption;

#define FS_WWT_STR			0		/* normal text widget */
#define FS_WWT_LINK			1		/* hyper link */
#define FS_WWT_IMAGE		2		/* image */
#define FS_WWT_SUBMIT		3		/* submit button */
#define FS_WWT_RESET		4		/* reset button */
#define FS_WWT_TEXT			5		/* text input widget */
#define FS_WWT_PWD			6		/* password input widget */
#define FS_WWT_CHECK		7		/* check box */
#define FS_WWT_RADIO		8		/* radio box */
#define FS_WWT_HIDDEN		9		/* hiden field */
#define FS_WWT_SELECT		10		/* select widget */
#define FS_WWT_TEXTAREA		11		/* text area */

#define FS_WWT_ANCHOR 		12		/* anchor. @ref WML */
#define FS_WWT_BGSOUND		13		/* bgsound */

/* reduce this struct's memory as small as we can */
typedef struct FS_WebWgt_Tag
{
	FS_List 			list;
	FS_UINT1			type;
	FS_UINT1			newline;
	FS_UINT1			flag;
	FS_UINT1			im_method;
	FS_SINT4			max_len;
	FS_CHAR *			name;
	FS_CHAR *			text;
	FS_CHAR *			link;
	FS_CHAR *			src;	/* for image. url */
	FS_CHAR *			file;	/* for image. local file */
	FS_CHAR *			value;
	FS_ImHandle			im_handle;	/* for image handle */
	
	FS_Rect 			rect;
	FS_ScrollPane * 	pane;

	FS_List				options;	/* for select widget */

	FS_List				container;	/* for form to link widgets togeter */
	FS_WebForm *		form;		/* belong to which form */
}FS_WebWgt;

/* @ref WML post field of setvar element */
typedef struct FS_PostField_Tag
{
	FS_List				list;
	FS_CHAR *			name;
	FS_CHAR *			value;
	FS_BOOL				is_var;		/* value is a var */
}FS_PostField;

/* 
	web doc tasks. when web doc load complete. it must do this task.
	for example, dowmload image files or redirect etc.
*/
#define FS_WTSK_GOTO		1	/* redirect to another url. Will clear the pending tasks */
#define FS_WTSK_TIMER		2	/* timer event */
#define FS_WTSK_IMAGE		3	/* image task. download image */
#define FS_WTSK_SOUND		4	/* sound task. download background sound */

typedef struct FS_WebTask_Tag
{
	FS_List 		list;
	
	FS_UINT1		type;
	FS_CHAR *		url;
	FS_WebForm *	form;	/* task may comtain a form to submit */
	FS_WebWgt *		wwgt;	/* FS_WTSK_IMAGE may reference a web wgt */
	FS_BOOL			is_embed;	/* is embeded task belong to web doc */
	FS_SINT4		delay;	/* only for timer task */
}FS_WebTask;

typedef struct FS_WebImage_Tag
{
	FS_List			list;

	FS_CHAR			dname[FS_FILE_NAME_LEN];
	FS_CHAR			file[FS_FILE_NAME_LEN];
}FS_WebImage;

#define FS_WW_FOCUS 		0x01
#define FS_WW_CHECK 		0x02
#define FS_WW_IMAGE 		0x04	/* when set. means image is download to local */
#define FS_WW_CAN_FOCUS 	0x08	/* when set. means this web widget can own focus */
#define FS_WW_MULTI 		0x10	/* when set. means this web widget can own focus */
#define FS_WW_UTF8			0x20	/* when submit, value must convert to UTF-8 charset */
#define FS_WW_IVALUE		0x40	/* for WML. select list has index value */
#define FS_WW_MID_ALIGN		0x80	/* middle align */

#define FS_WWGT_FOCUS( wwgt )		((wwgt)->flag & FS_WW_FOCUS)
#define FS_WWGT_CHECK( wwgt )		((wwgt)->flag & FS_WW_CHECK)
#define FS_WWGT_IMAGE( wwgt )		((wwgt)->flag & FS_WW_IMAGE)
#define FS_WWGT_CAN_FOCUS( wwgt )	((wwgt)->flag & FS_WW_CAN_FOCUS)
#define FS_WWGT_MULTI( wwgt )		((wwgt)->flag & FS_WW_MULTI)
#define FS_WWGT_IVALUE( wwgt )		((wwgt)->flag & FS_WW_IVALUE)
#define FS_WWGT_MID_ALIGN( wwgt )	((wwgt)->flag & FS_WW_MID_ALIGN)

#define FS_WWGT_SET_FOCUS( wwgt )		((wwgt)->flag |= FS_WW_FOCUS)
#define FS_WWGT_CLR_FOCUS( wwgt )		((wwgt)->flag &= (~FS_WW_FOCUS))
#define FS_WWGT_SET_CHECK( wwgt )		((wwgt)->flag |= FS_WW_CHECK)
#define FS_WWGT_CLR_CHECK( wwgt )		((wwgt)->flag &= (~FS_WW_CHECK))
#define FS_WWGT_SET_IMAGE( wwgt )		((wwgt)->flag |= FS_WW_IMAGE)
#define FS_WWGT_SET_CAN_FOCUS( wwgt )	((wwgt)->flag |= FS_WW_CAN_FOCUS)
#define FS_WWGT_SET_MULTI( wwgt )		((wwgt)->flag |= FS_WW_MULTI)
#define FS_WWGT_CLR_CAN_FOCUS( wwgt )	((wwgt)->flag &= (~FS_WW_CAN_FOCUS))
#define FS_WWGT_SET_IVALUE( wwgt )		((wwgt)->flag |= FS_WW_IVALUE)
#define FS_WWGT_SET_MID_ALIGN( wwgt )	((wwgt)->flag |= FS_WW_MID_ALIGN)

#define FS_WwCreateText( text )		\
	FS_CreateWebWgt( FS_WWT_STR, FS_NULL, text, FS_NULL, FS_NULL )
#define FS_WwCreateLink( text, link )	\
	FS_CreateWebWgt( FS_WWT_LINK, FS_NULL, text, link, FS_NULL )
#define FS_WwCreateImage( text, link, src )	\
	FS_CreateWebWgt( FS_WWT_IMAGE, FS_NULL, text, link, src )
#define FS_WwCreateSmtBtn( text )	\
	FS_CreateWebWgt( FS_WWT_SUBMIT, FS_NULL, text, FS_NULL, FS_NULL )
#define FS_WwCreateRstBtn( text )	\
	FS_CreateWebWgt( FS_WWT_RESET, FS_NULL, text, FS_NULL, FS_NULL )
#define FS_WwCreateInput( name, text )	\
	FS_CreateWebWgt( FS_WWT_TEXT, name, text, FS_NULL, FS_NULL )
#define FS_WwCreatePwd( name, text )	\
	FS_CreateWebWgt( FS_WWT_PWD, name, text, FS_NULL, FS_NULL )
#define FS_WwCreateCheck( name, text )	\
	FS_CreateWebWgt( FS_WWT_CHECK, name, text, FS_NULL, FS_NULL )
#define FS_WwCreateRadio( name, text )	\
	FS_CreateWebWgt( FS_WWT_RADIO, name, text, FS_NULL, FS_NULL )

FS_WebWgt * FS_CreateWebWgt( FS_UINT1 type, FS_CHAR *name, FS_CHAR *text, FS_CHAR *link, FS_CHAR *src );

void FS_WebWinAddWebWgt( FS_Window *win, FS_WebWgt *wwgt, FS_UINT1 nNewLine );

void FS_WebWinAddForm( FS_Window *win, FS_WebForm *form );

void FS_DrawWebWgtList( FS_List *wwgtlist );

FS_Window * FS_WebCreateWin( FS_UINT4 id, FS_CHAR * title, FS_WndProc proc );

FS_BOOL FS_WebDefWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam );

FS_BOOL FS_WebPaneMouseEvent( FS_Window *win, FS_SINT4 x, FS_SINT4 y );

void FS_FreeWebWgt( FS_WebWgt *wwgt );

void FS_FreeWebOption( FS_WebOption *opt );

void FS_RemoveWebWgtList( FS_Window *win );

void FS_RemoveFormList( FS_Window *win );

void FS_WebWgtSelectOption( FS_WebWgt *wwgt );

void FS_SetFocusToEyeableWebWgt( FS_Window *win );

void FS_RedrawWebWgt( FS_WebWgt *wwgt );

void FS_FreeWebForm( FS_WebForm *form );

void FS_FreeWebTask( FS_WebTask *task );

void FS_RemoveTaskList( FS_Window *win );

void FS_ClearWebWinContext( FS_Window *win );

void FS_SubmitForm( FS_Window *win, FS_WebForm *form );

void FS_LayoutWebWin( FS_Window *win );

void FS_WebWinPlayBgSound( FS_Window *win );

void FS_WebWinStopBgSound( FS_Window *win );

FS_SINT4 FS_WebWinGetFocusWebWgtPos( FS_Window * win );

FS_CHAR *FS_FindWebWgtValue( FS_Window *win, FS_CHAR *name );

FS_WebWgt *FS_FindWebWgt( FS_Window *win, FS_CHAR *name );

#endif
