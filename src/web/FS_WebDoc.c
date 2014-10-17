#include "inc/FS_Config.h"

#ifdef FS_MODULE_WEB

#include "inc/web/FS_WebDoc.h"
#include "inc/web/FS_WebConfig.h"
#include "inc/inte/FS_Inte.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_File.h"
#include "inc/util/FS_Charset.h"
#include "inc/util/FS_MemDebug.h"

extern FS_CHAR *FS_GetCurRequestUrl( void );
extern FS_CHAR *FS_GetCurWebPageUrl( void );

static void FS_WebDocFreeInput( FS_WebDoc *doc )
{
	if( doc->input )
	{
		FS_FreeWebWgt( doc->input );
		IFS_Free( doc->input );
		doc->input = FS_NULL;
	}
}

static void FS_ResetWebForm( FS_WebDoc *doc )
{
	FS_List *node, *head;
	FS_WebWgt *wwgt;
	
	head = &doc->win->pane.widget_list;
	node = head->next;
	while( node != head )
	{
		wwgt = FS_ListEntry( node, FS_WebWgt, list );
		node = node->next;
		
		if( wwgt->form == doc->form )
			wwgt->form = FS_NULL;
	}
}

void FS_WebDocFileRead( FS_WebDoc *doc, FS_SaxHandle hsax )
{
	FS_SINT4 rlen;
	FS_BOOL bDone = FS_FALSE;
	FS_BYTE *buf = IFS_Malloc( FS_FILE_BLOCK );
	if( buf )
	{
		rlen = FS_FileRead( FS_DIR_WEB, doc->file, doc->offset, buf, FS_FILE_BLOCK );
		if( rlen < FS_FILE_BLOCK )
			bDone = FS_TRUE;
		doc->offset += rlen;
		FS_SaxDataFeed( hsax, buf, rlen, bDone );
		IFS_Free( buf );
	}
}

void FS_WebDocComplete( FS_WebDoc *doc, FS_SaxHandle hsax )
{
	FS_FreeSaxHandler( hsax );
	FS_InvalidateRect( doc->win, FS_NULL );
	FS_SAFE_FREE( doc->file );
	FS_SAFE_FREE( doc->charset );
	FS_SAFE_FREE( doc->card_name );
	
	if( doc->form )
	{
		FS_ResetWebForm( doc );	/* reset all widget connect to this form */
		FS_FreeWebForm( doc->form );
		IFS_Free( doc->form );
		doc->form = FS_NULL;
	}
	if( doc->task )
	{
		FS_FreeWebTask( doc->task );
		IFS_Free( doc->task );
		doc->task = FS_NULL;
	}
	if( doc->input )
	{
		FS_WebDocFreeInput( doc );
	}
	if( doc->option )
	{
		FS_FreeWebOption( doc->option );
		IFS_Free( doc->option );
		doc->option = FS_NULL;
	}
	if( doc->post_field )
	{
		FS_SAFE_FREE( doc->post_field->name );
		FS_SAFE_FREE( doc->post_field->value );
		IFS_Free( doc->post_field );
	}
	IFS_Free( doc );
}

extern void FS_WebDocParseEnd( void );
/*
	when parser encouter a end doc tag, eg. </html> etc.
	use this to avoid stupid waiting for some server that 
	did not send Content-Lenght header fields.
*/
void FS_WebDocEndTag( FS_WebDoc *doc )
{
	/* to inform net to cancel stupid waiting. extern from FS_WebUtil.c */
	FS_WebDocParseEnd( );
}

void FS_WebDocNewWebWgt( FS_WebDoc *doc, FS_UINT1 type )
{
	if( doc->input )
		FS_WebDocFreeInput( doc );

	doc->input = FS_NEW( FS_WebWgt );
	if( doc->input )
	{
		IFS_Memset( doc->input, 0, sizeof(FS_WebWgt) );
		doc->input->type = type;
		FS_ListInit( &doc->input->options );
	}
}

void FS_WebDocNewTask( FS_WebDoc *doc, FS_UINT1 tskType )
{
	if( doc->task )
	{
		FS_FreeWebTask( doc->task );
		IFS_Free( doc->task );
		doc->task = FS_NULL;
	}
	
	doc->task = FS_NEW( FS_WebTask );
	if( doc->task )
	{
		IFS_Memset( doc->task, 0, sizeof(FS_WebTask) );
		doc->task->type = tskType;
	}
}

void FS_WebDocNewForm( FS_WebDoc *doc, FS_UINT1 type )
{
	if( doc->form )
	{
		FS_FreeWebForm( doc->form );
		IFS_Free( doc->form );
		doc->form = FS_NULL;
	}

	doc->form = FS_NEW( FS_WebForm );
	if( doc->form )
	{
		IFS_Memset( doc->form, 0, sizeof(FS_WebForm) );
		doc->form->type = type;
		FS_ListInit( &doc->form->input_list );
	}
}

void FS_WebDocAddForm( FS_WebDoc *doc )
{
	if( doc->form )
	{
		FS_ListAdd( &doc->win->context.form_list, &doc->form->list );
		doc->form = FS_NULL;
	}
}

void FS_WebDocNewPostField( FS_WebDoc *doc )
{
	if( doc->post_field )
	{
		FS_SAFE_FREE( doc->post_field->name );
		FS_SAFE_FREE( doc->post_field->value );
		IFS_Free( doc->post_field );
		doc->post_field = FS_NULL;
	}

	doc->post_field = FS_NEW( FS_PostField );
	if( doc->post_field )
	{
		IFS_Memset( doc->post_field, 0, sizeof(FS_PostField) );
	}
}

void FS_WebDocAddWebWgt( FS_WebDoc *doc )
{
	FS_WebWgt *wwgt, *wLink = FS_NULL;
	
	if( ! doc->input )
		return;

	wwgt = FS_CreateWebWgt( doc->input->type, doc->input->name, 
		doc->input->text, doc->input->link, doc->input->src );
	wwgt->flag |= doc->input->flag;
	wwgt->im_method = doc->input->im_method;
	wwgt->max_len = doc->input->max_len;
	
	if( doc->input->form && doc->input->form->accept_charset == FS_NULL
		&& doc->charset && IFS_Stricmp(doc->charset, "UTF-8") == 0 )
	{
		doc->input->form->accept_charset = IFS_Strdup( doc->charset );
	}
	
	if( doc->input->type != FS_WWT_IMAGE && doc->input->type != FS_WWT_LINK && doc->input->type != FS_WWT_STR )
	{	
		if( doc->input->type == FS_WWT_ANCHOR )
		{
			wwgt->form = doc->input->form;
			doc->input->form = FS_NULL;
		}
		else
		{
			wwgt->form = doc->form;
			if( doc->form )
				FS_ListAdd( &doc->form->input_list, &wwgt->container );
		}
	}
	/* copy select options */
	if( wwgt->type == FS_WWT_SELECT )
	{
		FS_ListCon( &wwgt->options, &doc->input->options );
		FS_ListInit( &doc->input->options );
		FS_WebWgtSelectOption( wwgt );
	}

	if( wwgt->type == FS_WWT_IMAGE && wwgt->src )
	{
		/* add image task */
		FS_WebDocAddImageTask( doc, wwgt->src, wwgt );
		if( wwgt->type == FS_WWT_IMAGE && wwgt->link && wwgt->text && wwgt->text[0] )
		{
			/* we split into two widget: image and link */
			wLink = FS_CreateWebWgt( FS_WWT_LINK, FS_NULL, wwgt->text, wwgt->link, FS_NULL );
			FS_WWGT_CLR_CAN_FOCUS( wwgt );
			IFS_Free( wwgt->link );
			wwgt->link = FS_NULL;
			IFS_Free( wwgt->text );
			wwgt->text = FS_NULL;
		}
	}
	FS_WebWinAddWebWgt( doc->win, wwgt, doc->new_line );

	if( wLink )
	{
		FS_WebWinAddWebWgt( doc->win, wLink, 0 );
	}
	
	FS_WebDocFreeInput( doc );
	doc->new_line = 0;
}

void FS_WebDocSetTitle( FS_WebDoc *doc, FS_CHAR *title )
{
	FS_COPY_TEXT( doc->win->context.title, title );
	FS_WindowSetTitle( doc->win, title );
}

void FS_WebDocAddTask( FS_WebDoc *doc )
{
	if( doc->task )
	{
		if( doc->task->type == FS_WTSK_SOUND && ! FS_WebConfigGetPlayAudioFlag() )
		{
			FS_FreeWebTask( doc->task );
			IFS_Free( doc->task );
			doc->task = FS_NULL;
			return;
		}
		
		FS_ListAddTail( &doc->win->context.task_list, &doc->task->list );
		doc->task = FS_NULL;
	}
}

void FS_WebDocTimerTaskSetValue( FS_WebDoc *doc, FS_SINT4 time )
{
	FS_List *node;
	FS_WebTask *task;

	if( doc->task && doc->task->type == FS_WTSK_TIMER )
	{
		doc->task->delay = time;
		return;
	}
	
	node = doc->win->context.task_list.prev;
	while( node != &doc->win->context.task_list )
	{
		task = FS_ListEntry( node, FS_WebTask, list );
		node = node->prev;

		if( task->type == FS_WTSK_TIMER )
		{
			task->delay = time / 10;
			return;
		}
	}
}

void FS_WebDocAddOption( FS_WebDoc *doc )
{
	if( doc->option )
	{
		if( doc->input && doc->input->type == FS_WWT_SELECT )
		{
			FS_ListAddTail( &doc->input->options, &doc->option->list );
		}
		else
		{
			FS_FreeWebOption( doc->option );
			IFS_Free( doc->option );	
		}
		doc->option = FS_NULL;
	}
}

void FS_WebDocAddImageTask( FS_WebDoc *doc, FS_CHAR *src, FS_WebWgt *wwgt )
{
	FS_WebTask *task;
	
	/* did not show image. so didnot download it */
	if( ! FS_WebConfigGetShowImageFlag() )
		return;
	
	task = FS_NEW( FS_WebTask );
	if( task )
	{
		IFS_Memset( task, 0, sizeof(FS_WebTask) );
		task->type = FS_WTSK_IMAGE;
		task->is_embed = FS_TRUE;
		/* use abs url. avoid when download a frameset web page may cause confuse of url */
		//task->url = FS_ComposeAbsUrl( FS_GetCurRequestUrl(), src );
		task->url = IFS_Strdup( src );
		task->wwgt = wwgt;
		FS_ListAddTail( &doc->win->context.task_list, &task->list );
	}
}

void FS_WebDocAddFrameTask( FS_WebDoc *doc, FS_CHAR *src )
{
#if 0
	FS_WebTask *task;
	
	task = FS_NEW( FS_WebTask );
	if( task )
	{
		IFS_Memset( task, 0, sizeof(FS_WebTask) );
		task->type = FS_WTSK_FRAME;
		task->url = FS_ComposeAbsUrl( FS_GetCurWebPageUrl(), src );
		//task->url = IFS_Strdup( src );
		FS_ListAddTail( &doc->win->context.task_list, &task->list );
	}
#else
	/* 暂时不支持网页框架，框架的各个url列出，供用户点击链接 */
	FS_WebWgt *wwgt;
	FS_CHAR *link = FS_ComposeAbsUrl( FS_GetCurWebPageUrl(), src );
	wwgt = FS_CreateWebWgt( FS_WWT_LINK, FS_NULL, link, link, FS_NULL );
	if( wwgt )
	{
		FS_WebWinAddWebWgt( doc->win, wwgt, 1 );
	}
	FS_SAFE_FREE( link );
#endif
}

#endif	//FS_MODULE_WEB


