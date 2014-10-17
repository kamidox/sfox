#include "inc/FS_Config.h"

#ifdef FS_MODULE_WEB

#include "inc/gui/FS_GrpLib.h"
#include "inc/gui/FS_WebGui.h"
#include "inc/inte/FS_Inte.h"
#include "inc/web/FS_WebUtil.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_Charset.h"
#include "inc/res/FS_Res.h"
#include "inc/util/FS_MemDebug.h"

#define FS_BROWSER_W				(IFS_GetScreenWidth( ) - FS_BAR_W)
#define FS_WEBWIN_INIT_H			(IFS_GetLineHeight( ) << 1)
#define FS_WWGT_SELECT_W			(IFS_GetLineHeight( ) << 2)
#define FS_WWGT_SELECT_H			(IFS_GetLineHeight( ) << 2)
#define FS_FORM_DATA_MAX_LEN		16384	/* 16K */

#define FS_WWGT_OVER_PAGE( wwgt )		\
	((wwgt)->rect.height > (wwgt)->pane->view_port.height)
	
#define FS_WWGT_PART_BELOW_VIEWPORT( wwgt )	\
	(((wwgt)->rect.top + (wwgt)->rect.height) \
	> ((wwgt)->pane->view_port.top + (wwgt)->pane->view_port.height))
	
#define FS_WWGT_PART_ABOVE_VIEWPORT( wwgt )	\
	((wwgt)->rect.top < (wwgt)->pane->view_port.top )
	
#define FS_WWGT_INSIDE_VIEWPORT( wwgt )	\
	( ((wwgt)->rect.top >= (wwgt)->pane->view_port.top)	\
	&& ( ((wwgt)->rect.top + (wwgt)->rect.height) < ((wwgt)->pane->view_port.top + (wwgt)->pane->view_port.height)) )

#define FS_WWGT_OUTSIDE_VIEWPORT( wwgt ) \
	( (((wwgt)->rect.top + (wwgt)->rect.height) <= (wwgt)->pane->view_port.top) \
	|| ((wwgt)->rect.top >= ((wwgt)->pane->view_port.top + (wwgt)->pane->view_port.height)) )

#define FS_PANE_VIEWPORT_IN_BOTTOM( pane )	\
	((pane)->view_port.top + (pane)->view_port.height >= (pane)->rect.top + (pane)->rect.height)
	
/*-----------------------------------------------------------------------------------------
		local functions define here
-----------------------------------------------------------------------------------------*/

/*--------------------- extern from FS_Gui.c --------------------------------------------*/
extern FS_GuiSkin GFS_Skins[3];

extern FS_UINT1 GFS_SkinIndex;

extern FS_Rect FS_GetPaneDrawRect( FS_ScrollPane *pane );
extern void FS_PaneScrollVertical( FS_Window *win, FS_ScrollPane *pane, FS_SINT4 scroll );
extern void FS_Draw3DBorder(FS_Rect * rect, FS_BOOL down);

extern FS_BOOL FS_WebHandleShortCutKey( FS_Window *win, FS_SINT4 vkeyCode );

/*--------------------- extern end --------------------------------------------------------*/

static FS_Rect FS_WebWgtGetDrawRect( FS_WebWgt *wwgt )
{
	FS_Rect rect;
	rect.left = 0;
	rect.top = wwgt->rect.top;
	rect.width = FS_BROWSER_W;
	rect.height = wwgt->rect.height;
	
	if( wwgt->pane )
	{
		rect.top = wwgt->rect.top - wwgt->pane->view_port.top + wwgt->pane->rect.top;
	}
	return rect;
}

void FS_DrawWebWgtBorder( FS_Rect *rect, FS_BOOL down )
{
	FS_Rect border = *rect;
	FS_COLOR clr = GFS_Skins[GFS_SkinIndex].fg;
	border.height --;
	border.width --;
	
	FS_DrawLine( border.left, border.top, border.width + border.left, border.top, clr );
	FS_DrawLine( border.left, border.top, border.left, border.top + border.height, clr );
	FS_DrawLine( border.left + border.width, border.top, border.left + border.width,
			border.top + border.height, clr );
	FS_DrawLine( border.left, border.top + border.height, border.left + border.width + 1,
			border.top + border.height, clr );
}

static void FS_DrawWebWgt( FS_WebWgt *wwgt )
{
	FS_COLOR orgBg = -1, orgFg = -1, orgTrans = -1;
	FS_Rect drawRect;
	FS_Bitmap *icon;
	FS_SINT4 span = IFS_GetWidgetSpan( ) >> 1;

	if( wwgt->type == FS_WWT_HIDDEN )
		return;
	
	drawRect = FS_WebWgtGetDrawRect( wwgt );
	if( drawRect.top + drawRect.height < 0 || drawRect.top > IFS_GetScreenHeight( ) )
		return; 	// out of view port, return
	
	if( wwgt->type == FS_WWT_TEXT || wwgt->type == FS_WWT_PWD 
		|| wwgt->type == FS_WWT_SELECT || wwgt->type == FS_WWT_TEXTAREA )
	{
		drawRect.width -= span;
	}

	if( FS_PushClipRect( &drawRect, FS_CLIP_CLIENT ) )
	{
		if( wwgt->type == FS_WWT_STR && wwgt->text )
		{
			orgBg = FS_SetBgColor( GFS_Skins[GFS_SkinIndex].bg );
			orgFg = FS_SetFgColor( GFS_Skins[GFS_SkinIndex].fg );
			FS_DrawMultiLineString( wwgt->rect.left, &drawRect, wwgt->text, FS_S_NONE );
		}
		else if( (wwgt->type == FS_WWT_LINK || wwgt->type == FS_WWT_ANCHOR) && wwgt->text )
		{
			if( FS_WWGT_FOCUS(wwgt) )
			{
				//orgBg = FS_SetBgColor( GFS_Skins[GFS_SkinIndex].focus_link_bg );
				//orgFg = FS_SetFgColor( GFS_Skins[GFS_SkinIndex].focus_link );
				orgBg = FS_SetBgColor( GFS_Skins[GFS_SkinIndex].focus_bg );
				orgFg = FS_SetFgColor( GFS_Skins[GFS_SkinIndex].focus_text );				
			}
			else
			{
				orgBg = FS_SetBgColor( GFS_Skins[GFS_SkinIndex].bg );
				orgFg = FS_SetFgColor( GFS_Skins[GFS_SkinIndex].focus_link_bg );
			}
			FS_DrawMultiLineString( wwgt->rect.left, &drawRect, wwgt->text, FS_S_UNDERLINE );
		}
		else if( wwgt->type == FS_WWT_TEXT || wwgt->type == FS_WWT_PWD 
			|| wwgt->type == FS_WWT_SELECT || wwgt->type == FS_WWT_TEXTAREA )
		{
			orgFg = FS_SetFgColor( GFS_Skins[GFS_SkinIndex].fg );

			if( FS_WWGT_FOCUS(wwgt) )
			{	
				orgBg = FS_SetBgColor( GFS_Skins[GFS_SkinIndex].focus_bg );
				FS_FillRect( &drawRect, GFS_Skins[GFS_SkinIndex].focus_bg );
			}
			else
			{
				orgBg = FS_SetBgColor( GFS_Skins[GFS_SkinIndex].bg );
				FS_FillRect( &drawRect, GFS_Skins[GFS_SkinIndex].bg );
			}
			FS_Draw3DBorder( &drawRect, FS_TRUE );
			if( wwgt->text )
			{
				drawRect.top ++;
				
				if( wwgt->type == FS_WWT_PWD )
					FS_DrawString( drawRect.left + IFS_GetWidgetSpan( ), drawRect.top, "********", FS_S_NONE );
				else if( wwgt->type == FS_WWT_TEXTAREA )
					FS_DrawMultiLineString( wwgt->rect.left + IFS_GetWidgetSpan( ), &drawRect, wwgt->text, FS_S_NONE );
				else
					FS_DrawString( drawRect.left + IFS_GetWidgetSpan( ), drawRect.top, wwgt->text, FS_S_NONE );

				drawRect.top --;
			}
			if( wwgt->type == FS_WWT_SELECT )
			{
				orgTrans = FS_SetTransColor( IFS_DDB_RGB(0xFF, 0, 0xFF) );
				icon = FS_Icon( FS_I_COMBO );
				FS_DrawBitmap( drawRect.left + FS_BROWSER_W - IFS_GetLineHeight( ), drawRect.top, icon, FS_S_TRANS );
				FS_ReleaseIcon( icon );
			}
		}
		else if( wwgt->type == FS_WWT_CHECK || wwgt->type == FS_WWT_RADIO )
		{
			if( FS_WWGT_CHECK(wwgt) )
			{
				if( wwgt->type == FS_WWT_CHECK )
					icon = FS_Icon( FS_I_CHECK );
				else
					icon = FS_Icon( FS_I_RADIO_CHECK );
			}
			else
			{
				if( wwgt->type == FS_WWT_CHECK )
					icon = FS_Icon( FS_I_UNCHECK );
				else
					icon = FS_Icon( FS_I_RADIO_UNCHECK );
			}
			drawRect.left = wwgt->rect.left;
			orgTrans = FS_SetTransColor( IFS_DDB_RGB(0xFF, 0, 0xFF) );
			FS_DrawBitmap( drawRect.left + span, drawRect.top, icon, FS_S_TRANS );
			FS_ReleaseIcon( icon );

			drawRect.left += span;
			drawRect.width = FS_TEXT_H;
			drawRect.height = FS_TEXT_H - 1;
			if( FS_WWGT_FOCUS(wwgt) )
			{
				FS_DrawRect( &drawRect, IFS_DDB_RGB(0, 0, 0xFF) );
			}
			else
			{
				FS_DrawRect( &drawRect, GFS_Skins[GFS_SkinIndex].bg );
			}
		}
		else if( wwgt->type == FS_WWT_RESET || wwgt->type == FS_WWT_SUBMIT )
		{
			FS_CHAR *text;
			drawRect.left = wwgt->rect.left;
			drawRect.width = wwgt->rect.width - wwgt->rect.left;
			FS_FillRect( &drawRect, IFS_DDB_RGB(0xB0, 0xB0, 0xB0) );
			if( wwgt->text == FS_NULL && wwgt->type == FS_WWT_RESET )
				text = "reset";
			else if( wwgt->text == FS_NULL && wwgt->type == FS_WWT_SUBMIT )
				text = "submit";
			else
				text = wwgt->text;
			orgBg = FS_SetBgColor( IFS_DDB_RGB(0xB0, 0xB0, 0xB0) );
			FS_DrawString( drawRect.left + IFS_GetWidgetSpan( ), drawRect.top + span, text, FS_S_NONE );
			FS_DrawWebWgtBorder( &drawRect, FS_FALSE );
			if( FS_WWGT_FOCUS(wwgt) )
			{
				drawRect.height --;
				FS_DrawRect( &drawRect, IFS_DDB_RGB(0xFF, 0, 0) );
			}
		}
		else if( wwgt->type == FS_WWT_IMAGE )
		{
			if( FS_WWGT_IMAGE(wwgt) && wwgt->im_handle )
			{
				/* decode image and draw image */
				drawRect.left = wwgt->rect.left;
				drawRect.width = wwgt->rect.width - wwgt->rect.left;
				if( drawRect.width > 0 )
				{
					icon = FS_ImDecode( wwgt->im_handle, drawRect.width, drawRect.height );
					if( icon )
					{
						drawRect.left += span;
						if( FS_WWGT_MID_ALIGN(wwgt) )
							FS_DrawBitmap( (FS_BROWSER_W - drawRect.width) >> 1, drawRect.top, icon, FS_S_NONE );
						else
							FS_DrawBitmap( drawRect.left, drawRect.top, icon, FS_S_NONE );
						FS_ImRelease( wwgt->im_handle );
					}
					if( FS_WWGT_FOCUS(wwgt) )
					{
						drawRect.height --;
						FS_DrawRect( &drawRect, IFS_DDB_RGB(0xFF, 0, 0) );
					}
				}
			}
			else
			{
				icon = FS_Icon( FS_I_IMAGE );
				orgTrans = FS_SetTransColor( IFS_DDB_RGB(0xFF, 0, 0xFF) );
				FS_DrawBitmap( wwgt->rect.left + span, drawRect.top, icon, FS_S_TRANS );
				if( wwgt->text )
				{
					if( wwgt->link && FS_WWGT_FOCUS(wwgt) )
					{
						//orgBg = FS_SetBgColor( GFS_Skins[GFS_SkinIndex].focus_link_bg );
						//orgFg = FS_SetFgColor( GFS_Skins[GFS_SkinIndex].focus_link );
						orgBg = FS_SetBgColor( GFS_Skins[GFS_SkinIndex].focus_bg );
						orgFg = FS_SetFgColor( GFS_Skins[GFS_SkinIndex].focus_text );				
					}
					else
					{
						orgBg = FS_SetBgColor( GFS_Skins[GFS_SkinIndex].bg );
						orgFg = FS_SetFgColor( GFS_Skins[GFS_SkinIndex].fg );
					}
					if( wwgt->link )
						FS_DrawMultiLineString( wwgt->rect.left + IFS_GetLineHeight( ), &drawRect, wwgt->text, FS_S_UNDERLINE );
					else
						FS_DrawMultiLineString( wwgt->rect.left + IFS_GetLineHeight( ), &drawRect, wwgt->text, FS_S_NONE );
				}
				
				drawRect.left = wwgt->rect.left + span;
				drawRect.width = icon->width;
				drawRect.height = icon->height - 1;
				if( FS_WWGT_FOCUS(wwgt) )
				{
					FS_DrawRect( &drawRect, IFS_DDB_RGB(0, 0, 0xFF) );
				}
				else
				{
					FS_DrawRect( &drawRect, GFS_Skins[GFS_SkinIndex].bg );
				}				
				FS_ReleaseIcon( icon );
			}
		}
		FS_PopClipRect( );
	}

	if( orgBg != -1 )
		FS_SetBgColor( orgBg );
	if( orgFg != -1 )
		FS_SetFgColor( orgFg );
	if( orgTrans != -1 )
		FS_SetTransColor( orgTrans );
}

void FS_WebWgtDrawImage_CB( FS_WebWgt *wwgt, FS_Bitmap *pBmp )
{
	if( FS_WindowIsTopMost( wwgt->pane->win->id ) )
	{
		FS_RedrawWebWgt( wwgt );
	}
}

static void FS_WebWgtRadioSetCheck( FS_WebWgt *wwgt )
{
	FS_List *node;
	FS_WebWgt *ww;
	if( wwgt->form && ! FS_WWGT_CHECK(wwgt) )
	{
		node = wwgt->form->input_list.next;
		while( node != &wwgt->form->input_list )
		{
			ww = FS_ListEntry( node, FS_WebWgt, container );
			node = node->next;
			if( FS_WWGT_CHECK(ww) && IFS_Stricmp(wwgt->name, ww->name) == 0 )
			{
				FS_WWGT_CLR_CHECK(ww);
				FS_RedrawWebWgt( ww );
				break;
			}
		}
		FS_WWGT_SET_CHECK( wwgt );
		FS_RedrawWebWgt( wwgt );
	}
}


static void FS_WebWgtTextInput_CB( FS_CHAR * text, FS_SINT4 len, void *param )
{
	FS_WebWgt *wwgt = (FS_WebWgt *)param;
	if( text )
	{
		FS_COPY_TEXT( wwgt->text, text );
	}
	else
	{
		if( wwgt->text )
		{
			IFS_Free( wwgt->text );
			wwgt->text = FS_NULL;
		}
	}
	FS_RedrawWebWgt( wwgt );
}

static void FS_WebWgtSelectComposeIValue( FS_WebWgt *wwgt )
{
	#define FS_WWT_SELECT_IVALUE_LEN		128
	
	FS_List *node;
	FS_WebOption *opt;
	FS_SINT4 len, index;
	
	node = wwgt->options.next;
	FS_SAFE_FREE( wwgt->value );
	wwgt->value = IFS_Malloc( FS_WWT_SELECT_IVALUE_LEN );
	if( wwgt->value )
	{
		IFS_Memset( wwgt->value, 0, FS_WWT_SELECT_IVALUE_LEN );
		index = 0;
		while( node != &wwgt->options )
		{
			opt = FS_ListEntry( node, FS_WebOption, list );
			node = node->next;
			index ++;
			if( opt->selected )
			{
				len = IFS_Strlen( wwgt->value );
				if( IFS_Strlen(wwgt->value) > 0 )
				{
					IFS_Sprintf( wwgt->value + len, ";%d", index );
				}
				else
				{
					IFS_Sprintf( wwgt->value, "%d", index );
				}
			}
		}
	}
	FS_COPY_TEXT( wwgt->text, wwgt->value );
}

static void FS_WebWgtSelectHandler( FS_Window *win )
{
	FS_Widget *wgt;
	FS_List *node;
	FS_WebWgt *wwgt;
	FS_WebOption *opt = FS_NULL;
	FS_Window *webmain = FS_WindowFindId( FS_W_WebMainFrm );
	if( webmain && win && win->id == FS_W_WebWgtSelect )
	{
		wwgt = webmain->pane.focus_wwgt;
		wgt = FS_WindowGetFocusItem( win );

		node = wwgt->options.next;
		while( node != &wwgt->options )
		{
			opt = FS_ListEntry( node, FS_WebOption, list );
			node = node->next;
			if( opt->label && IFS_Stricmp( opt->label, wgt->text ) == 0 )
			{
				/* we find the select option */
				/* multi select */
				if( FS_WWGT_MULTI(wwgt) )
				{
					opt->selected = ! opt->selected;
					break;
				}
				else
				{
					opt->selected = FS_TRUE;
					if( opt->on_pick ) break;
				}
			}
			else
			{
				if( ! FS_WWGT_MULTI(wwgt) )
					opt->selected = FS_FALSE;
			}
		}

		if( FS_WWGT_IVALUE(wwgt) )
			FS_WebWgtSelectComposeIValue( wwgt );
		else
		{
			FS_COPY_TEXT( wwgt->text, wgt->text );
			FS_COPY_TEXT( wwgt->value, wgt->data );
		}
		
		FS_RedrawWebWgt( wwgt );
	}

	if( win && win->id == FS_W_WebWgtSelect );
		FS_DestroyWindow( win );
		
	if( opt && opt->on_pick )
	{
		FS_WebGoToUrl( webmain, opt->on_pick, FS_FALSE );
	}
}

static void FS_WebWgtSelectShow_UI( FS_WebWgt * wwgt )
{
	FS_Window *win;
	FS_Widget *wgt;
	FS_List *node;
	FS_WebOption *opt;
	FS_Rect rect;
	FS_SINT4 nItems = FS_ListCount( &wwgt->options );
	if( nItems > 0 )
	{
		rect = FS_WebWgtGetDrawRect( wwgt );
		
		win = FS_CreatePopUpMenu( FS_W_WebWgtSelect, &rect, nItems );
		node = wwgt->options.next;
		while( node != &wwgt->options )
		{
			opt = FS_ListEntry( node, FS_WebOption, list );
			node = node->next;
			if( opt->label )
			{
				wgt = FS_CreateMenuItem( 0, opt->label );
				FS_COPY_TEXT( wgt->data, opt->value );
				if( FS_WWGT_MULTI(wwgt) )
				{
					if( opt->selected )
						wgt->icon = FS_Icon( FS_I_CHECK );
					else
						wgt->icon = FS_Icon( FS_I_UNCHECK );
				}
				FS_WidgetSetHandler( wgt, FS_WebWgtSelectHandler );
				FS_WGT_SET_NO_INDEX_MENU_FLAG( wgt );
				FS_MenuAddItem( win, wgt );
			}
		}
		
		FS_MenuSetSoftkey( win );
		FS_ShowWindow( win );
	}
}

static FS_CHAR *FS_WebWgtSelectFindValue( FS_WebWgt *wwgt )
{
	FS_List *node;
	FS_WebOption *opt = FS_NULL;
	node = wwgt->options.next;
	while( node != &wwgt->options )
	{
		opt = FS_ListEntry( node, FS_WebOption, list );
		node = node->next;
		if( wwgt->text && opt->label && IFS_Stricmp( wwgt->text, opt->label ) == 0 )
		{
			return opt->value;
		}
	}

	/* default, select the first option. (base on IE6.0) */
	if( ! FS_ListIsEmpty(&wwgt->options) )
	{
		opt = FS_ListEntry( wwgt->options.next, FS_WebOption, list );
		return opt->value;
	}
	else
	{
		return FS_NULL;
	}
}

FS_CHAR *FS_FindWebWgtValue( FS_Window *win, FS_CHAR *name )
{
	FS_WebWgt *wwgt;
	FS_List *node = win->pane.widget_list.next;
		
	while( node != &win->pane.widget_list )
	{
		wwgt = FS_ListEntry( node, FS_WebWgt, list );
		node = node->next;
		if( wwgt->type == FS_WWT_TEXT || wwgt->type == FS_WWT_PWD || wwgt->type == FS_WWT_SELECT )
		{
			if( wwgt->name && IFS_Stricmp(wwgt->name, name) == 0 )
			{
				if( wwgt->type == FS_WWT_SELECT )
					return wwgt->value;
				else
					return wwgt->text;
			}
		}
	}

	return FS_NULL;
}

FS_WebWgt *FS_FindWebWgt( FS_Window *win, FS_CHAR *name )
{
	FS_WebWgt *wwgt;
	FS_List *node = win->pane.widget_list.next;
		
	while( node != &win->pane.widget_list )
	{
		wwgt = FS_ListEntry( node, FS_WebWgt, list );
		node = node->next;
		if( wwgt->name && IFS_Stricmp(wwgt->name, name) == 0 )
			return wwgt;
	}

	return FS_NULL;
}

static FS_SINT4 FS_ComposeFormData( FS_Window *win, FS_WebForm *form, FS_CHAR **out_data )
{
	FS_CHAR *form_data, *value, *name;
	FS_SINT4 dlen = 0;
	FS_List *node;
	FS_PostField *postField;
	FS_WebWgt *wwgt;
	
	form_data = IFS_Malloc( FS_FORM_DATA_MAX_LEN );
	if( form_data )
	{
		IFS_Memset( form_data, 0, FS_FORM_DATA_MAX_LEN );
		/* GET method */
		if( form->method == FS_NULL || IFS_Strnicmp(form->method, "GET", 3) == 0 )
		{
			IFS_Strcpy( form_data, form->action );
			dlen = IFS_Strlen( form_data );
			if( IFS_Strchr(form->action, '?') == FS_NULL )
				form_data[dlen ++] = '?';
		}

		node = form->input_list.prev;
		while( node != &form->input_list )
		{
			if( form->type == FS_WFT_HTML )
			{
				wwgt = FS_ListEntry( node, FS_WebWgt, container );
				name = wwgt->name;
			}
			else
			{
				postField = FS_ListEntry( node, FS_PostField, list );
				name = postField->name;
				if( postField->is_var )
					value = FS_FindWebWgtValue( win, postField->value );
				else
					value = postField->value;
			}
			
			node = node->prev;
			/* Successful controls */
			if( name )
			{
				if( form->type == FS_WFT_HTML )
				{
					if( wwgt->type == FS_WWT_RADIO || wwgt->type == FS_WWT_CHECK )
					{
						if( ! FS_WWGT_CHECK(wwgt) )
							continue;
					}
					if( dlen > 0 && (wwgt->type == FS_WWT_SUBMIT || wwgt->type == FS_WWT_RESET) )
					{
						continue;
					}
				}
				
				if( dlen > 0 && form_data[dlen - 1] != '?' )
					form_data[dlen ++] = '&';
				
				dlen += FS_UrlEncode( form_data + dlen, FS_FORM_DATA_MAX_LEN - dlen, name, -1 );
				
				form_data[dlen ++] = '=';

				if( form->type == FS_WFT_HTML )
				{
					if( wwgt->type == FS_WWT_SELECT )
						value = wwgt->value;
					else
						value = wwgt->text;
				}

				dlen += FS_UrlEncode( form_data + dlen, FS_FORM_DATA_MAX_LEN - dlen, value, -1 );
			}
		}
	}

	if( dlen > 0 && form_data[dlen - 1] == '?' )
	{
		form_data[dlen - 1] = 0;
		dlen --;
	}
	*out_data = form_data;
	return dlen;
}

static void FS_ResetForm( FS_WebForm *form )
{
	FS_WebWgt *wwgt;
	FS_List *node = form->input_list.next;

	while( node != &form->input_list )
	{
		wwgt = FS_ListEntry( node, FS_WebWgt, container );
		node = node->next;

		if( wwgt->type == FS_WWT_TEXT || wwgt->type == FS_WWT_PWD )
		{
			FS_SAFE_FREE( wwgt->text );
			wwgt->text = FS_NULL;
			FS_RedrawWebWgt( wwgt );
		}
		else if( wwgt->type == FS_WWT_CHECK && FS_WWGT_CHECK(wwgt) )
		{
			FS_WWGT_CLR_CHECK( wwgt );
			FS_RedrawWebWgt( wwgt );
		}
	}
}

static FS_CHAR *FS_ProcessUrlVar( FS_Window *win, FS_CHAR *url )
{
	FS_CHAR *pVar, *p, *lastPos, *varName, *varValue;
	FS_CHAR *cnvtUrl = FS_NULL;
	FS_SINT4 offset, tlen;
	FS_SINT4 quote = 0;
	
	p = url;
	lastPos = url;
	while( 1 )
	{
		p = IFS_Strstr( p, "$" );
		if( p )
		{
			if( p[1] == '(' )
			{
				quote = 1;
			}
			else
			{
				quote = 0;
			}
			
			pVar = p + quote + 1;
			if( quote )
			{
				p = IFS_Strchr( pVar, ')' );
			}
			else
			{
				p = pVar;
				while( (*p >= '0' && *p <= '9') 
					|| (*p >= 'a' && *p <= 'z') 
					|| (*p >= 'A' && *p <= 'Z') )
				{
					p ++;
				}
			}
			
			if( p )
			{
				/*
						XXXXXparam=$(varname)XXXX
								     |      |
								     pVar   p
				*/
				varName = FS_Strndup( pVar, p - pVar );
				varValue = FS_FindWebWgtValue( win, varName );
				if( cnvtUrl == FS_NULL )
				{
					cnvtUrl = IFS_Malloc( FS_MAX_URL_LEN );
					if( cnvtUrl == FS_NULL )
					{
						IFS_Free( varName );
						break;
					}

					IFS_Memset( cnvtUrl, 0, FS_MAX_URL_LEN );
					offset = 0;
				}
				IFS_Memcpy( cnvtUrl + offset, lastPos, pVar - 1 - quote - lastPos );
				offset += (pVar - 1 - quote - lastPos);
				if( varValue )
				{
					tlen = IFS_Strlen( varValue );
					IFS_Memcpy( cnvtUrl + offset, varValue, tlen );
					offset += tlen;
				}
				IFS_Free( varName );
				p = p + quote;
				lastPos = p;
			}
			else
			{
				if( cnvtUrl )
				{
					IFS_Strcpy( cnvtUrl + offset, lastPos );
				}
				break;
			}
		}
		else
		{
			if( cnvtUrl )
			{
				IFS_Strcpy( cnvtUrl + offset, lastPos );
			}
			break;
		}
	}

	return cnvtUrl;
}

static FS_BOOL FS_WebWgtHandleSelect( FS_WebWgt *wwgt )
{	
	FS_CHAR *url, *tmp;
	FS_EditParam eParam = {0};
	if( wwgt->type == FS_WWT_CHECK )
	{
		if( FS_WWGT_CHECK(wwgt) )
			FS_WWGT_CLR_CHECK( wwgt );
		else
			FS_WWGT_SET_CHECK( wwgt );
		FS_RedrawWebWgt( wwgt );
	}
	else if( wwgt->type == FS_WWT_RADIO )
	{
		FS_WebWgtRadioSetCheck( wwgt );
	}
	else if( wwgt->type == FS_WWT_TEXT || wwgt->type == FS_WWT_PWD || wwgt->type == FS_WWT_TEXTAREA )
	{
		if( FS_WebNetBusy() ) return FS_FALSE;

		if( wwgt->max_len )
			eParam.max_len = wwgt->max_len;
		else
			eParam.max_len = 1024;
		if( wwgt->im_method )
			eParam.preferred_method = wwgt->im_method;
		else
			eParam.preferred_method = FS_IM_CHI;
		eParam.allow_method = FS_IM_ALL;
		eParam.rect = FS_WebWgtGetDrawRect( wwgt );
		if( eParam.rect.top < IFS_GetWinTitleHeight() )
		{
			FS_WindowSetViewPort( wwgt->pane->win, wwgt->rect.top );
			FS_InvalidateRect( wwgt->pane->win, FS_NULL );
			eParam.rect = FS_WebWgtGetDrawRect( wwgt );
		}
		eParam.rect.width -= IFS_GetWidgetSpan() >> 1;
		IFS_InputDialog( wwgt->text, &eParam, FS_WebWgtTextInput_CB, wwgt );
	}
	else if( wwgt->type == FS_WWT_SELECT )
	{
		FS_WebWgtSelectShow_UI( wwgt );
	}
	else if( (wwgt->type == FS_WWT_LINK && wwgt->link)
		|| (wwgt->type == FS_WWT_IMAGE && wwgt->link) )
	{
		if( wwgt->pane->win->context.url_var )
		{
			url = FS_ProcessUrlVar( wwgt->pane->win, wwgt->link );
			if( url == FS_NULL ) url = wwgt->link;
			
			FS_WebGoToUrl( wwgt->pane->win, url, FS_FALSE );

			if( url != FS_NULL && url != wwgt->link ) IFS_Free( url );
		}
		else
		{
			FS_WebGoToUrl( wwgt->pane->win, wwgt->link, FS_FALSE );
		}
	}
	else if( (wwgt->type == FS_WWT_ANCHOR || wwgt->type == FS_WWT_SUBMIT) 
		&& wwgt->form && wwgt->form->action )
	{
		if( wwgt->pane->win->context.url_var )
		{
			url = FS_ProcessUrlVar( wwgt->pane->win, wwgt->form->action );
			if( url )
			{
				tmp = wwgt->form->action;
				wwgt->form->action = url;
				url = tmp;
			}
			
			FS_SubmitForm( wwgt->pane->win, wwgt->form );

			if( url )
			{
				tmp = wwgt->form->action;
				wwgt->form->action = url;
				url = tmp;
				IFS_Free( url );
			}
		}
		else
		{
			FS_SubmitForm( wwgt->pane->win, wwgt->form );
		}
	}
	else if( wwgt->type == FS_WWT_RESET && wwgt->form )
	{
		FS_ResetForm( wwgt->form );
	}
	return FS_TRUE;
}

void FS_SetFocusToEyeableWebWgt( FS_Window *win )
{
	FS_WebWgt *wwgt;
	FS_Rect rect;
	FS_List *node = win->pane.widget_list.next;

	if( win->pane.focus_wwgt == FS_NULL )
		return;
	
	if( win->pane.focus_wwgt && FS_WWGT_INSIDE_VIEWPORT(win->pane.focus_wwgt) )
		return;
	
	while( node != &win->pane.widget_list )
	{
		wwgt = FS_ListEntry( node, FS_WebWgt, list );
		node = node->next;
		if( FS_WWGT_INSIDE_VIEWPORT(wwgt) && FS_WWGT_CAN_FOCUS(wwgt) )
		{
			if( win->pane.focus_wwgt )
			{
				FS_WWGT_CLR_FOCUS( win->pane.focus_wwgt );
				rect = FS_WebWgtGetDrawRect(  win->pane.focus_wwgt );
				FS_InvalidateRect( win, &rect );
			}
			FS_WWGT_SET_FOCUS( wwgt );
			win->pane.focus_wwgt = wwgt;
			rect = FS_WebWgtGetDrawRect(  win->pane.focus_wwgt );
			FS_InvalidateRect( win, &rect );
			return;
		}
	}
}

static void FS_WebWinSetFocusWebWgt( FS_Window *win, FS_WebWgt *wwgt )
{
	FS_WWGT_CLR_FOCUS( win->pane.focus_wwgt );
	FS_RedrawWebWgt( win->pane.focus_wwgt );
	FS_WWGT_SET_FOCUS( wwgt );
	win->pane.focus_wwgt = wwgt;
	FS_RedrawWebWgt( wwgt );
}

static FS_BOOL FS_WebWgtNavigate( FS_Window *win, FS_BOOL bDown )
{
	FS_List *node;
	FS_WebWgt *wwgt;
	FS_SINT4 line;

	if( FS_ListIsEmpty(&win->pane.widget_list) )
		return FS_FALSE;

	if( win->pane.focus_wwgt == FS_NULL )
	{
		win->pane.cyc = FS_FALSE;
		if( bDown )
		{
			if( ! FS_PANE_VIEWPORT_IN_BOTTOM(&win->pane) )
				FS_PaneScrollVertical( win, &win->pane, FS_TEXT_H );
		}
		else
		{
			if( win->pane.view_port.top > 0 )
				FS_PaneScrollVertical( win, &win->pane, -FS_TEXT_H );
		}
		
		return FS_TRUE;
	}
	
	if( bDown )
	{
		node = win->pane.focus_wwgt->list.next;
		line = FS_TEXT_H;
	}
	else
	{
		node = win->pane.focus_wwgt->list.prev;
		line = -FS_TEXT_H;
	}

	win->pane.cyc = FS_FALSE;
	wwgt = win->pane.focus_wwgt;
	/* over one page, and still full in screen. scroll down one line */
	if( /*FS_WWGT_OVER_PAGE(wwgt) &&*/ FS_WWGT_PART_BELOW_VIEWPORT(wwgt) )
	{
		FS_PaneScrollVertical( win, &win->pane, line );
		return FS_TRUE;
	}

	if( FS_WWGT_OUTSIDE_VIEWPORT(wwgt) )
	{
		FS_PaneScrollVertical( win, &win->pane, line );
		FS_SetFocusToEyeableWebWgt( win );
		return FS_TRUE;
	}
	
	while( node != &win->pane.widget_list )
	{
		wwgt = FS_ListEntry( node, FS_WebWgt, list );
		if( ! FS_WWGT_CAN_FOCUS(wwgt) )
		{
			if( bDown && ! FS_WWGT_OVER_PAGE(wwgt) && ! FS_WWGT_PART_BELOW_VIEWPORT(wwgt) )
			{
				node = node->next;
			}
			else if( ! bDown && ! FS_WWGT_OVER_PAGE(wwgt) && ! FS_WWGT_PART_ABOVE_VIEWPORT(wwgt) )
			{
				node = node->prev;
			}
			else
			{
				FS_PaneScrollVertical( win, &win->pane, line );
				FS_SetFocusToEyeableWebWgt( win );
				break;
			}
		}
		else
		{
			if( FS_WWGT_INSIDE_VIEWPORT(wwgt) )
			{
				FS_WebWinSetFocusWebWgt( win, wwgt );
			}
			else
			{
				FS_WWGT_CLR_FOCUS( win->pane.focus_wwgt );
				FS_WWGT_SET_FOCUS( wwgt );
				win->pane.focus_wwgt = wwgt;
				if( wwgt->type != FS_WWT_IMAGE )
				{
					if( line > 0 )
						line = wwgt->rect.height;
					else
						line = -wwgt->rect.height;
				}
				FS_PaneScrollVertical( win, &win->pane, line );
			}
			break;
		}
	}
	return FS_TRUE;
}

static FS_BOOL FS_WebWinScroll( FS_Window *win, FS_SINT4 scroll )
{
	win->pane.cyc = FS_FALSE;
	FS_PaneScrollVertical( win, &win->pane, scroll );
	FS_SetFocusToEyeableWebWgt( win );
	return FS_TRUE;
}

static FS_BOOL FS_HitWebWgt( FS_WebWgt *wwgt, FS_SINT4 x, FS_SINT4 y )
{
	FS_BOOL ret = FS_FALSE;
	FS_Rect rect = FS_WebWgtGetDrawRect( wwgt );
	rect.left = wwgt->rect.left;
	rect.width = wwgt->rect.width;
	if( y >= rect.top && y <= rect.top + rect.height )
	{
		/* in single line */
		if( rect.height <= FS_TEXT_H )
		{
			if( x >= rect.left && x <= rect.width )
				ret = FS_TRUE;
		}
		/* multi line */
		else
		{
			/* in first part line */
			if( y < rect.top + FS_TEXT_H )
			{
				if( x >= rect.left && x <= FS_BROWSER_W )
					ret = FS_TRUE;
			}
			/* in last part line */
			else if( y > rect.top + rect.height - FS_TEXT_H )
			{
				if( x <= rect.width )
					ret = FS_TRUE;
			}
			/* in midle line, full line */
			else
			{
				if( x <= FS_BROWSER_W )
					ret = FS_TRUE;
			}
		}
	}
	return ret;
}

/* return web widget height */
static void FS_WebWgtLayout( FS_WebWgt *wwgt, FS_WebContext *cntx )
{
	if( cntx->cur_x + IFS_GetLineHeight( ) >= FS_BROWSER_W )
	{
		cntx->cur_x = 0;
		cntx->cur_y += cntx->line_h;
	}
	wwgt->rect.left = cntx->cur_x;
	wwgt->rect.top = cntx->cur_y;
	if( wwgt->type == FS_WWT_STR || wwgt->type == FS_WWT_LINK || wwgt->type == FS_WWT_ANCHOR ){
		/* here, rect.width store the wwgt's right pos */
		if( wwgt->text ){
			wwgt->rect.height = FS_StringHeight( wwgt->text, wwgt->rect.left, FS_BROWSER_W, &wwgt->rect.width);
		}else{
			wwgt->rect.height = FS_TEXT_H;
			wwgt->rect.width = wwgt->rect.left;
		}
	}
	else if( wwgt->type == FS_WWT_TEXT || wwgt->type == FS_WWT_PWD 
		|| wwgt->type == FS_WWT_SELECT || wwgt->type == FS_WWT_TEXTAREA )
	{
		if( wwgt->rect.left != 0 )
		{
			wwgt->rect.left = 0;
			wwgt->rect.top += cntx->line_h;
		}
		
		if( wwgt->type == FS_WWT_TEXTAREA )
			wwgt->rect.height = FS_TEXT_H << 2;
		else
			wwgt->rect.height = FS_TEXT_H;
		wwgt->rect.width = FS_BROWSER_W;
	}
	else if( wwgt->type == FS_WWT_CHECK || wwgt->type == FS_WWT_RADIO )
	{
		wwgt->rect.width = wwgt->rect.left + IFS_GetLineHeight( );
		wwgt->rect.height = FS_TEXT_H;
	}
	else if( wwgt->type == FS_WWT_SUBMIT || wwgt->type == FS_WWT_RESET )
	{
		if( wwgt->text )
		{
			wwgt->rect.height = FS_StringHeight( wwgt->text, wwgt->rect.left, FS_BROWSER_W, &wwgt->rect.width );
			if( wwgt->rect.height > FS_TEXT_H )
			{
				wwgt->rect.left = 0;
				wwgt->rect.top += cntx->line_h;
				wwgt->rect.height = FS_StringHeight( wwgt->text, wwgt->rect.left, FS_BROWSER_W, &wwgt->rect.width );
				/* button's max width is in a line */
				if( wwgt->rect.height > FS_TEXT_H )
				{
					wwgt->rect.height = FS_TEXT_H;
					wwgt->rect.width = FS_BROWSER_W;
				}
			}
		}
		else
		{
			wwgt->rect.height = FS_TEXT_H;
			wwgt->rect.width = wwgt->rect.left + (IFS_GetLineHeight( ) + (IFS_GetLineHeight( ) >> 1));
		}
		wwgt->rect.width = FS_MIN( wwgt->rect.width + (IFS_GetWidgetSpan( ) << 1), FS_BROWSER_W );
	}
	else if( wwgt->type == FS_WWT_IMAGE )
	{
		if( wwgt->im_handle == FS_NULL )
		{
			if( wwgt->file && wwgt->file[0] )
				wwgt->im_handle = FS_ImCreate( wwgt->file, FS_WebWgtDrawImage_CB, wwgt );
		}
		
		if( FS_WWGT_IMAGE(wwgt) && wwgt->im_handle )
		{
			/* image is download. get image size */
			FS_SINT4 w = 0, h = 0;
			
			FS_ImGetSize( wwgt->im_handle, &w, &h );
			FS_ImageScale( &w, &h, FS_BROWSER_W - IFS_GetWidgetSpan( ) );
			if( wwgt->rect.left + w >= FS_BROWSER_W )
			{
				wwgt->rect.left = 0;
				wwgt->rect.top += cntx->line_h;
			}
			wwgt->rect.height = FS_MAX( h, FS_TEXT_H );
			wwgt->rect.width = wwgt->rect.left + w;
		}
		else
		{
			if( wwgt->rect.left + IFS_GetLineHeight( ) >= FS_BROWSER_W )
			{
				wwgt->rect.left = 0;
				wwgt->rect.top += cntx->line_h;
			}
			wwgt->rect.height = FS_TEXT_H;
			wwgt->rect.width = wwgt->rect.left + IFS_GetLineHeight( );
			if( wwgt->text )
			{
				wwgt->rect.height = FS_StringHeight( wwgt->text, wwgt->rect.left + IFS_GetLineHeight( ), FS_BROWSER_W, &wwgt->rect.width );
			}
		}
		wwgt->rect.height = FS_MAX( wwgt->rect.height, cntx->line_h );
	}
	cntx->line_h = FS_TEXT_H;
	cntx->cur_y = wwgt->rect.top + wwgt->rect.height - FS_TEXT_H;
	cntx->cur_x = wwgt->rect.width + IFS_GetWidgetSpan(); 	
}

/*-----------------------------------------------------------------------------------------
		extern functions define here
-----------------------------------------------------------------------------------------*/

FS_WebWgt * FS_CreateWebWgt( FS_UINT1 type, FS_CHAR *name, FS_CHAR *text, FS_CHAR *link, FS_CHAR *src )
{
	FS_WebWgt *wwgt = IFS_Malloc( sizeof(FS_WebWgt) );
	if( wwgt )
	{
		IFS_Memset( wwgt, 0, sizeof(FS_WebWgt) );
		wwgt->type = type;
		FS_ListInit( &wwgt->options );
		FS_ListInit( &wwgt->container );
		if( type != FS_WWT_STR && type != FS_WWT_HIDDEN )
		{
			if( type != FS_WWT_IMAGE || link != FS_NULL )
				FS_WWGT_SET_CAN_FOCUS( wwgt );
		}

		if( name )
			wwgt->name = IFS_Strdup( name );
		if( text )
			wwgt->text = IFS_Strdup( text );
		if( link )
			wwgt->link = IFS_Strdup( link );
		if( src )
			wwgt->src = IFS_Strdup( src );
	}
	return wwgt;
}

void FS_WebWinAddWebWgt( FS_Window *win, FS_WebWgt *wwgt, FS_UINT1 nNewLine )
{
	FS_WebContext *cntx = &win->context;
	FS_WebWgt *lastWwgt = FS_NULL;
	
	if( win && wwgt )
	{
		if( wwgt->type == FS_WWT_HIDDEN )
		{
			if( FS_ListIsEmpty( &win->pane.widget_list ) )
			{
				cntx->cur_x = 0;
				cntx->cur_y = IFS_GetWinTitleHeight( );
				cntx->line_h = FS_TEXT_H;
			}
			wwgt->rect.top = cntx->cur_y;
			wwgt->pane = &win->pane;
			FS_ListAddTail( &win->pane.widget_list, &wwgt->list );
			return;
		}
		
		wwgt->newline = nNewLine;
		if( FS_ListIsEmpty( &win->pane.widget_list ) )
		{
			cntx->cur_x = 0;
			cntx->cur_y = IFS_GetWinTitleHeight( );
			cntx->line_h = FS_TEXT_H;
			
			FS_WebWgtLayout( wwgt, cntx );
		}
		else
		{
			lastWwgt = FS_ListEntry( win->pane.widget_list.prev, FS_WebWgt, list );
			if( nNewLine )
			{
				nNewLine = FS_MIN( nNewLine, 3 );
				cntx->cur_x = 0;
				cntx->cur_y += nNewLine * FS_TEXT_H;
				cntx->line_h = FS_TEXT_H;			
			}else{
				if( lastWwgt->type == FS_WWT_IMAGE && wwgt->type == FS_WWT_IMAGE )
				{
					cntx->line_h = FS_MAX( FS_TEXT_H, lastWwgt->rect.height );
					cntx->cur_y = lastWwgt->rect.top;
				}				
			}

			FS_WebWgtLayout( wwgt, cntx );
		}
		
		if( win->pane.focus_wwgt == FS_NULL && FS_WWGT_CAN_FOCUS(wwgt) )
		{
			win->pane.focus_wwgt = wwgt;
			FS_WWGT_SET_FOCUS( wwgt );
		}

		wwgt->pane = &win->pane;
		win->pane.rect.height = cntx->cur_y + cntx->line_h;
		FS_ListAddTail( &win->pane.widget_list, &wwgt->list );
	}
}

void FS_WebWinAddForm( FS_Window *win, FS_WebForm *form )
{
	FS_ListAdd( &win->context.form_list, &form->list );
}

void FS_DrawWebWgtList( FS_List *wwgtlist )
{
	FS_List *node;
	FS_WebWgt *wwgt;
	node = wwgtlist->next;
	while( node != wwgtlist )
	{
		wwgt = FS_ListEntry( node, FS_WebWgt, list );
		node = node->next;
		FS_DrawWebWgt( wwgt );
	}
}

static FS_BOOL FS_WebDefaultWndProc( void *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	if( cmd == FS_WM_SETFOCUS ){
		FS_WebWinPlayBgSound( win );
	}else if( cmd == FS_WM_LOSTFOCUS ){
		FS_WebWinStopBgSound( win );
	}
	return FS_FALSE;
}

FS_Window * FS_WebCreateWin( FS_UINT4 id, FS_CHAR * title, FS_WndProc proc )
{
	FS_Window *win = FS_CreateWindow( id, title, proc );
	FS_ListInit( &win->context.form_list );
	FS_ListInit( &win->context.task_list );
	FS_ListInit( &win->context.image_list );
	win->pane.draw_scroll_bar = FS_TRUE;
	win->pane.bar.rect = FS_GetPaneDrawRect( &win->pane );
	win->pane.bar.rect.left = win->pane.rect.left + win->pane.rect.width - FS_BAR_W;
	win->pane.bar.rect.width = FS_BAR_W;
	win->pane.bar.box_height = FS_BAR_W << 1;
	win->type = FS_WT_WEB;
	win->pane.rect.height = FS_WEBWIN_INIT_H;
	if( proc == FS_NULL )
	{
		win->proc = FS_WebDefaultWndProc;
	}
	return win;
}

FS_BOOL FS_WebDefWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	FS_ScrollPane *pane = &win->pane;
		
	if( FS_SOFTKEY2 == wparam )
	{
		if( win->focus_btn )
		{
			if( win->focus_btn->handle )
				win->focus_btn->handle( win );
			ret = FS_TRUE;
		}
		else if( pane->focus_wwgt )
		{
			ret = FS_WebWgtHandleSelect( pane->focus_wwgt );
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
		ret = FS_WebWgtNavigate( win, FS_FALSE );
	}
	else if( FS_KEY_DOWN == wparam )
	{
		if( win->focus_btn )
		{
			FS_WGT_CLR_FOCUS( win->focus_btn );
			FS_InvalidateRect( win, &win->focus_btn->rect );
			win->focus_btn = FS_NULL;
		}
		ret = FS_WebWgtNavigate( win, FS_TRUE );
	}
	else if( FS_KEY_PGUP == wparam || FS_KEY_LEFT == wparam  )
	{
		if( pane->rect.height > pane->view_port.height && pane->rect.top < pane->view_port.top )
		{
			ret = FS_WebWinScroll( win, (IFS_GetLineHeight( ) - pane->view_port.height) );
		}
		else
		{
			ret = FS_WebWgtNavigate( win, FS_FALSE );
		}
	}
	else if( FS_KEY_PGDOWN == wparam || FS_KEY_RIGHT == wparam )
	{
		if( pane->rect.height > pane->view_port.height
			&& (pane->rect.top + pane->rect.height) > (pane->view_port.top + pane->view_port.height ) )
		{
			ret = FS_WebWinScroll( win, (pane->view_port.height - IFS_GetLineHeight( )) );
		}
		else
		{
			ret = FS_WebWgtNavigate( win, FS_TRUE );
		}
	}

	if( ret == FS_FALSE )
	{
		ret = FS_WebHandleShortCutKey( win, wparam );
	}
	return ret;
}

FS_BOOL FS_WebPaneMouseEvent( FS_Window *win, FS_SINT4 x, FS_SINT4 y )
{
	FS_WebWgt *wwgt;
	FS_List *node = win->pane.widget_list.next;
	while( node != &win->pane.widget_list )
	{
		wwgt = FS_ListEntry( node, FS_WebWgt, list );
		node = node->next;
		if( FS_HitWebWgt( wwgt, x, y ) )
		{
			if( FS_WWGT_CAN_FOCUS(wwgt) )
			{
				FS_WebWinSetFocusWebWgt( win, wwgt );
				FS_WebWgtHandleSelect( wwgt );
			}
			break;
		}
	}
	return FS_TRUE;
}

FS_SINT4 FS_WebWinGetFocusWebWgtPos( FS_Window * win )
{
	if( win && win->pane.focus_wwgt )
		return win->pane.focus_wwgt->rect.top;
	else
		return 0;
}

void FS_FreeWebWgt( FS_WebWgt *wwgt )
{
	FS_List *node;
	FS_WebOption *opt;
	node = wwgt->options.next;
	while( node != &wwgt->options )
	{
		opt = FS_ListEntry( node, FS_WebOption, list );
		node = node->next;
		FS_ListDel( &opt->list );
		FS_FreeWebOption( opt );
		IFS_Free( opt );
	}
	
	if( wwgt->type == FS_WWT_ANCHOR && wwgt->form )
	{
		FS_FreeWebForm( wwgt->form );
		IFS_Free( wwgt->form );
	}
	FS_SAFE_FREE( wwgt->text );
	FS_SAFE_FREE( wwgt->link );
	FS_SAFE_FREE( wwgt->name );
	FS_SAFE_FREE( wwgt->src );
	FS_SAFE_FREE( wwgt->file );
	FS_SAFE_FREE( wwgt->value );
	if( wwgt->im_handle )
	{
		FS_ImDestroy( wwgt->im_handle );
		wwgt->im_handle = 0;
	}
}

void FS_FreeWebOption( FS_WebOption *opt )
{
	FS_SAFE_FREE( opt->value );
	FS_SAFE_FREE( opt->label );
	FS_SAFE_FREE( opt->on_pick );
}

void FS_FreeWebForm( FS_WebForm *form )
{
	FS_PostField *postField;
	FS_List *node;
	if( form->type == FS_WFT_ANCHOR )
	{
		node = form->input_list.next;
		while( node != &form->input_list )
		{
			postField = FS_ListEntry( node, FS_PostField, list );
			node = node->next;
			FS_ListDel( &postField->list );
			FS_SAFE_FREE( postField->name );
			FS_SAFE_FREE( postField->value );
			IFS_Free( postField );
		}
	}
	FS_SAFE_FREE( form->action );
	FS_SAFE_FREE( form->method );
	FS_SAFE_FREE( form->enc_type );
	FS_SAFE_FREE( form->accept );
	FS_SAFE_FREE( form->accept_charset );
	FS_SAFE_FREE( form->name );
}

void FS_FreeWebTask( FS_WebTask *task )
{
	if( task->form )
	{
		FS_FreeWebForm( task->form );
		IFS_Free( task->form );
		task->form = FS_NULL;
	}

	FS_SAFE_FREE( task->url );
}

void FS_RemoveWebWgtList( FS_Window *win )
{
	FS_WebWgt *wwgt;
	FS_List *node = win->pane.widget_list.next;
	while( node != &win->pane.widget_list )
	{
		wwgt = FS_ListEntry( node, FS_WebWgt, list );
		node = node->next;
		FS_ListDel( &wwgt->list );
		FS_FreeWebWgt( wwgt );
		IFS_Free( wwgt );
	}
	win->pane.view_port.top = IFS_GetWinTitleHeight( );
	win->pane.rect.height = FS_WEBWIN_INIT_H;
	win->pane.focus_wwgt = FS_NULL;
}

void FS_RemoveFormList( FS_Window *win )
{
	FS_WebForm *form;
	FS_List *node = win->context.form_list.next;
	while( node != &win->context.form_list )
	{
		form = FS_ListEntry( node, FS_WebForm, list );
		node = node->next;
		FS_ListDel( &form->list );
		FS_FreeWebForm( form );
		IFS_Free( form );
	}
}

void FS_RemoveTaskList( FS_Window *win )
{
	FS_WebTask *task;
	FS_List *node = win->context.task_list.next;
	while( node != &win->context.task_list )
	{
		task = FS_ListEntry( node, FS_WebTask, list );
		node = node->next;
		FS_ListDel( &task->list );
		FS_FreeWebTask( task );
		IFS_Free( task );
	}
}

void FS_RemoveImageList( FS_Window *win )
{
	FS_WebImage *img;
	FS_List *node = win->context.image_list.next;
	while( node != &win->context.image_list )
	{
		img = FS_ListEntry( node, FS_WebImage, list );
		node = node->next;
		FS_ListDel( &img->list );
		IFS_Free( img );
	}
}

void FS_ClearWebWinContext( FS_Window *win )
{
	win->context.url_var = FS_FALSE;
	win->context.is_start_page = FS_FALSE;
	win->context.line_h = 0;
	win->context.cur_x = 0;
	win->context.cur_y = 0;
	FS_SAFE_FREE( win->context.title );
	if( win->context.bgsound.is_playing ){
		win->context.bgsound.is_playing = FS_FALSE;
		IFS_StopAudio( );
	}
	IFS_Memset( &win->context.bgsound, 0, sizeof(FS_WebSound) );
	FS_RemoveWebWgtList( win );
	FS_RemoveFormList( win );
	FS_RemoveTaskList( win );
	FS_RemoveImageList( win );
}

void FS_WebWgtSelectOption( FS_WebWgt *wwgt )
{
	FS_List *node;
	FS_WebOption *opt;
	node = wwgt->options.next;
	while( node != &wwgt->options )
	{
		opt = FS_ListEntry( node, FS_WebOption, list );
		node = node->next;
		if( opt->selected )
		{
			FS_COPY_TEXT( wwgt->text, opt->label );
			FS_COPY_TEXT( wwgt->value, opt->value );
			break;
		}
	}
	
	if( wwgt->value == FS_NULL ) /* still empty */
	{
		if( ! FS_ListIsEmpty(&wwgt->options) )
		{
			opt = FS_ListEntry( wwgt->options.next, FS_WebOption, list );
			FS_COPY_TEXT( wwgt->text, opt->label );
			FS_COPY_TEXT( wwgt->value, opt->value );
		}
	}
}

void FS_SubmitForm( FS_Window *win, FS_WebForm *form )
{
	FS_SINT4 dlen;
	FS_CHAR *form_data = FS_NULL;

	if( form )
	{
		dlen = FS_ComposeFormData( win, form, &form_data );
		if( form_data && dlen > 0 )
		{
			if( form->method == FS_NULL || IFS_Strnicmp( form->method, "GET", 3 ) == 0 )
				FS_WebGoToUrl( win, form_data, form->send_referer );
			else if( form->method && IFS_Strnicmp( form->method, "POST", 4 ) == 0 )
				FS_WebPostData( win, form->action, form_data, dlen, form->send_referer );
		}
		FS_SAFE_FREE( form_data );
	}
}

void FS_RedrawWebWgt( FS_WebWgt *wwgt )
{
	FS_Rect rect = FS_WebWgtGetDrawRect( wwgt );
	FS_DrawWebWgt( wwgt );
	IFS_InvalidateRect( &rect );
}

void FS_WebWinPlayBgSound( FS_Window *win )
{
	FS_WebSound *pSnd = &win->context.bgsound;
	FS_CHAR absfile[FS_MAX_PATH_LEN];

	if( pSnd->file[0] && ! pSnd->is_playing )
	{
		FS_GetAbsFileName( FS_DIR_WEB, pSnd->file, absfile );
		pSnd->is_playing = IFS_PlayAudio( absfile, 0 );
	}
}

void FS_WebWinStopBgSound( FS_Window *win )
{
	FS_WebSound *pSnd = &win->context.bgsound;
	if( pSnd->file[0] && pSnd->is_playing )
	{
		pSnd->is_playing = FS_FALSE;
		IFS_StopAudio( );
	}
}

void FS_LayoutWebWin( FS_Window *win )
{
	FS_List wwgtList, *node;
	FS_WebWgt *wwgt;

	FS_ListInit( &wwgtList );
	FS_ListCon( &wwgtList, &win->pane.widget_list );
	FS_ListInit( &win->pane.widget_list );
	/* re-layout all web widget */
	node = wwgtList.next;
	win->pane.rect.height = 0;
	while( node != &wwgtList )
	{
		wwgt = FS_ListEntry( node, FS_WebWgt, list );
		node = node->next;
		FS_ListDel( &wwgt->list );

		if( wwgt->type == FS_WWT_IMAGE && wwgt->file )
			FS_WWGT_SET_IMAGE( wwgt );
		FS_WebWinAddWebWgt( win, wwgt, wwgt->newline );
	}
	
	FS_InvalidateRect( win, FS_NULL );
	
	FS_WebWinPlayBgSound( win );
}

#endif //FS_MODULE_WEB
