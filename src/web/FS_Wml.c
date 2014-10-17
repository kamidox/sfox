#include "inc/FS_Config.h"

#ifdef FS_MODULE_WEB

#include "inc/util/FS_File.h"
#include "inc/util/FS_Util.h"
#include "inc/web/FS_WebDoc.h"
#include "inc/inte/FS_Inte.h"
#include "inc/util/FS_MemDebug.h"

void FS_WmlNewOption( FS_WebDoc *doc )
{
	if( doc->input == FS_NULL || doc->input->type != FS_WWT_SELECT )
		return;
	
	if( doc->option == FS_NULL )
	{
		doc->option = IFS_Malloc( sizeof(FS_WebOption) );
		if( doc->option )
		{
			IFS_Memset( doc->option, 0, sizeof(FS_WebOption) );
		}
	}
}

static void FS_WmlOptionAttr( FS_WebDoc *doc, FS_CHAR *name, FS_CHAR *value )
{
	if( doc->option == FS_NULL ) return;
	
	if( IFS_Stricmp(name, "value") == 0 )
	{
		FS_COPY_TEXT( doc->option->value, value );
	}
	else if( IFS_Stricmp(name, "title") == 0 )
	{
		FS_COPY_TEXT( doc->option->label, value );
	}
	else if( IFS_Stricmp(name, "onpick") == 0 )
	{
		FS_COPY_TEXT( doc->option->on_pick, value );
	}
}

static void FS_WmlStartElement( FS_WebDoc *doc, FS_CHAR *ename )
{
	if( ! ename ) return;
	
	if( ! doc->start ) return;
	
	/* <br> */
	if( FS_CHAR_EQUAL( ename[0], 'b') && FS_CHAR_EQUAL( ename[1], 'r') && ename[2] == 0 )
	{
		doc->new_line = 1;
	}
	/* <tr> */
	else if( FS_CHAR_EQUAL( ename[0], 't') && FS_CHAR_EQUAL( ename[1], 'r') && ename[2] == 0 )
	{
		doc->new_line = 1;
	}
	/* <p> */
	else if( FS_CHAR_EQUAL( ename[0], 'p') && ename[1] == 0 )
	{
		doc->new_line = 1;
	}
	/* <a> element */
	else if( FS_CHAR_EQUAL(ename[0], 'a') && ename[1] == 0 )
	{
		FS_WebDocNewWebWgt( doc, FS_WWT_LINK );
	}
	/* <anchor> element */
	else if( IFS_Stricmp(ename, "anchor") == 0 
		|| IFS_Stricmp(ename, "do") == 0 )
	{
		FS_WebDocNewWebWgt( doc, FS_WWT_ANCHOR );
	}
	/* <input> element */
	else if( IFS_Stricmp(ename, "input") == 0 )
	{
		FS_WebDocNewWebWgt( doc, FS_WWT_TEXT );
	}
	/* <onevent> element */
	else if( IFS_Stricmp( ename, "onevent" ) == 0 )
	{
		FS_WebDocNewTask( doc, FS_WTSK_GOTO );
	}
	/* <go> element */
	else if( IFS_Stricmp( ename, "go" ) == 0 )
	{
		FS_WebDocNewForm( doc, FS_WFT_ANCHOR );
	}
	/* <postfield> element */
	else if( IFS_Stricmp( ename, "postfield" ) == 0 )
	{
		FS_WebDocNewPostField( doc );
	}
	/* <img> element */
	else if( IFS_Stricmp(ename, "img") == 0 )
	{
		if( ! doc->input )
			FS_WebDocNewWebWgt( doc, FS_WWT_IMAGE );
		else
			doc->input->type = FS_WWT_IMAGE;
	}
	/* <select> */
	else if( IFS_Stricmp( ename, "select" ) == 0 )
	{
		FS_WebDocNewWebWgt( doc, FS_WWT_SELECT );
	}
	/* <option> */
	else if( IFS_Stricmp( ename, "option" ) == 0 )
	{
		FS_WmlNewOption( doc );
	}
}

static void FS_WmlStartElementEnd( FS_WebDoc * doc, FS_CHAR *ename )
{
	if( ! ename ) return;
	
	if( ! doc->start ) return;

	if( IFS_Stricmp(ename, "postfield") == 0 )
	{
		if( doc->form && doc->post_field )
		{
			FS_ListAdd( &doc->form->input_list, &doc->post_field->list );
			doc->post_field = FS_NULL;	/* form owns the post field */
		}
		else if( doc->post_field )
		{
			FS_SAFE_FREE( doc->post_field->name );
			FS_SAFE_FREE( doc->post_field->value );
			IFS_Free( doc->post_field );
			doc->post_field = FS_NULL;
		}
	}
	/* <input> element */
	else if( IFS_Stricmp(ename, "input") == 0 )
	{
		FS_WebDocAddWebWgt( doc );
	}
	/* <img> element */
	else if( IFS_Stricmp(ename, "img") == 0 )
	{
		if( doc->input && doc->input->link == FS_NULL )
			FS_WebDocAddWebWgt( doc );
	}	
}

static void FS_WmlEndElement( FS_WebDoc * doc, FS_CHAR *ename )
{
	if( ! ename ) return;
	
	if( IFS_Stricmp( ename, "wml" ) == 0 )
	{
		/*
			we assume that, document parse end. use that to avoid stupid waiting
			for server further data.
			because some HTTP server may not send Content-Length header field.
		*/
		FS_WebDocEndTag( doc ); 	
	}
	
	if( ! doc->start ) return;
	
	if( IFS_Stricmp(ename, "a") == 0 )
	{
		FS_WebDocAddWebWgt( doc );
	}
	else if( IFS_Stricmp(ename, "go") == 0 )
	{
		if( doc->input && doc->input->type == FS_WWT_ANCHOR )
		{
			/* a anchor or do task <go> */
			doc->input->form = doc->form;
			doc->form = FS_NULL;	/* anchor own the form */
		}
		else if( doc->task )
		{
			doc->task->form = doc->form;
			doc->form = FS_NULL;	/* onevent own the form */
		}
		else if( doc->form )
		{
			FS_FreeWebForm( doc->form );
			IFS_Free( doc->form );
			doc->form = FS_NULL;
		}
	}
	else if( IFS_Stricmp(ename, "anchor") == 0 
		|| IFS_Stricmp(ename, "do") == 0 )
	{
		FS_WebDocAddWebWgt( doc );
	}
	else if( IFS_Stricmp(ename, "onevent") == 0 )
	{
		FS_WebDocAddTask( doc );
	}
	else if( IFS_Stricmp( ename, "select" ) == 0 )
	{
		FS_WebDocAddWebWgt( doc );
	}
	else if( IFS_Stricmp( ename, "option" ) == 0 )
	{
		FS_WebDocAddOption( doc );
	}
	else if( IFS_Stricmp( ename, "card" ) == 0 )
	{
		doc->start = FS_FALSE;
	}
}

static void FS_WmlElementText( FS_WebDoc * doc, FS_CHAR *ename, FS_CHAR *str, FS_SINT4 slen )
{
	FS_CHAR *text;
	FS_SINT4 olen = 0;
	FS_WebWgt *wwgt;
	
	if( ! doc->start ) return;
	
	if( ename && str && slen > 0 )
	{		
		text = FS_ProcessCharset( str, slen, doc->charset, &olen );
		if( text == FS_NULL && str )
		{
			text = FS_Strndup( str, slen );
			olen = slen;
		}
		
		if( ! text ) return;
		
		FS_ProcessEsc( text, olen );

		/* hyperlink */
		if( doc->input && FS_CHAR_EQUAL(ename[0], 'a') && ename[1] == 0 )
		{
			FS_COPY_TEXT( doc->input->text, text );
		}
		else if( doc->input && IFS_Stricmp(ename, "anchor") == 0 )
		{
			FS_COPY_TEXT( doc->input->text, text );
		}
		else if( doc->input && IFS_Stricmp(ename, "do") == 0 )
		{
			FS_COPY_TEXT( doc->input->text, text );
		}
		else if( IFS_Stricmp(ename, "option") == 0 )
		{
			if( doc->option && doc->option->label == FS_NULL )
			{
				FS_COPY_TEXT( doc->option->label, text );
			}
		}
		else
		{
			wwgt = FS_WwCreateText( text );
			FS_WebWinAddWebWgt( doc->win, wwgt, doc->new_line );
			doc->new_line = 0;
		}
		
		IFS_Free( text );
	}
}

static void FS_WmlTaskGoAttr( FS_WebDoc * doc, FS_CHAR *name, FS_CHAR *value )
{
	if( ! doc->form ) return;
	
	if( IFS_Stricmp( name, "href" ) == 0 )
	{
		FS_COPY_TEXT( doc->form->action, value );
	}
	else if( IFS_Stricmp( name, "sendreferer" ) == 0 )
	{
		if( IFS_Stricmp(value, "true") == 0 )
			doc->form->send_referer = FS_TRUE;
	}
	else if( IFS_Stricmp( name, "method" ) == 0 )
	{
		FS_COPY_TEXT( doc->form->method, value );
	}
	/* ignore other attribute */
}

static void FS_WmlPostFieldAttr( FS_WebDoc * doc, FS_CHAR *name, FS_CHAR *value )
{
	FS_CHAR *tstr;
	
	if( ! doc->post_field ) return;
	
	if( IFS_Stricmp( name, "name" ) == 0 )
	{
		FS_COPY_TEXT( doc->post_field->name, value );
	}
	else if( IFS_Stricmp( name, "value" ) == 0 )
	{
		/* WML Variables */
		if( value[0] == '$' )
		{
			doc->post_field->is_var = FS_TRUE;
			if( value[1] == '(' )
			{
				tstr = IFS_Strchr( value, ')' );
				if( tstr )
				{
					FS_SAFE_FREE( doc->post_field->value );
					doc->post_field->value = FS_Strndup( value + 2, tstr - value - 2 );
				}
				else
				{
					doc->post_field->is_var = FS_FALSE;
					FS_COPY_TEXT( doc->post_field->value, value );
				}
			}
			else
			{
				FS_COPY_TEXT( doc->post_field->value, value + 1 );
			}

			if( doc->post_field->is_var )
			{
				tstr = IFS_Strchr( doc->post_field->value, ':' );
				if( tstr )
				{
					if( IFS_Stricmp( tstr, ":noesc" ) == 0 )
					{
						*tstr = 0;
					}
					else if( IFS_Stricmp( tstr, ":escape" ) == 0 )
					{
						*tstr = 0;
					}
					else if( IFS_Stricmp( tstr, ":unesc" ) == 0 )
					{
						*tstr = 0;
					}
				}
			}
		}
		else
		{
			doc->post_field->is_var = FS_FALSE;
			FS_COPY_TEXT( doc->post_field->value, value );
		}
	}
}

static void FS_WmlInputAttr( FS_WebDoc * doc, FS_CHAR *name, FS_CHAR *value )
{
	if( ! doc->input ) return;
	
	if( IFS_Stricmp( name, "name" ) == 0 )
	{
		FS_COPY_TEXT( doc->input->name, value );
	}
	else if( IFS_Stricmp( name, "iname" ) == 0 )
	{
		FS_WWGT_SET_IVALUE( doc->input );
		FS_COPY_TEXT( doc->input->name, value );
	}
	else if( IFS_Stricmp( name, "value" ) == 0 || IFS_Stricmp( name, "ivalue" ) == 0)
	{
		FS_COPY_TEXT( doc->input->text, value );
		FS_COPY_TEXT( doc->input->value, value );
	}
	else if( IFS_Stricmp( name, "type" ) == 0 )
	{
		if( IFS_Stricmp( value, "password" ) == 0 )
			doc->input->type = FS_WWT_PWD;
		else
			doc->input->type = FS_WWT_TEXT;
	}
	else if( FS_STR_I_EQUAL(name, "multiple") && (value == FS_NULL || FS_STR_I_EQUAL(value, "true")) )
	{
		FS_WWGT_SET_MULTI( doc->input );
	}
	else if( IFS_Stricmp(name, "format") == 0 )
	{
		if( doc->input->type == FS_WWT_TEXT || doc->input->type == FS_WWT_PWD )
		{
			if( IFS_Strchr(value, 'X') || IFS_Strchr(value, 'x') 
				|| IFS_Strchr(value, 'A') || IFS_Strchr(value, 'a') )
			{
				doc->input->im_method = FS_IM_ABC;
			}
			else if( IFS_Strchr(value, 'N') || IFS_Strchr(value, 'n') )
			{
				doc->input->im_method = FS_IM_123;
			}
			else
			{
				doc->input->im_method = FS_IM_CHI;
			}
		}
	}
	else if( IFS_Stricmp(name, "maxlenght") == 0 )
	{
		if( doc->input->type == FS_WWT_TEXT || doc->input->type == FS_WWT_PWD )
		{
			doc->input->max_len = IFS_Atoi( value );
		}
	}
	/* ignore other attr */
}

static void FS_WmlElementAttr( FS_WebDoc * doc, FS_CHAR *ename, FS_CHAR *name, FS_CHAR *value )
{
	FS_CHAR *vtext;
	if( ! doc->start )
	{
		if( FS_STR_I_EQUAL(ename, "card") 
			&& FS_STR_I_EQUAL(name, "id") && FS_STR_I_EQUAL(value, doc->card_name) )
		{
			doc->start = FS_TRUE;
		}
		return;
	}
	
	if( ename && name )
	{
		vtext = FS_ProcessCharset( value, -1, doc->charset, FS_NULL );
		if( vtext == FS_NULL )
		{
			vtext = value;
		}

		if( vtext ) FS_ProcessEsc( vtext, -1 );
		
		/* hypelink */
		if( vtext && IFS_Stricmp( ename, "a" ) == 0 )
		{
			/* prechar # is jump to doc part. donot support now */
			if( doc->input && IFS_Stricmp( name, "href" ) == 0 )
			{
				FS_COPY_TEXT( doc->input->link, vtext );
			}
		}
		/* image */
		else if( vtext && IFS_Stricmp( ename, "img" ) == 0 )
		{
			if( doc->input && IFS_Stricmp(name, "src") == 0 )
			{
				FS_COPY_TEXT( doc->input->src, vtext );
			}
		}
		/* <go> attr */
		else if( vtext && name && IFS_Stricmp( ename, "go" ) == 0 )
		{
			FS_WmlTaskGoAttr( doc, name, vtext );
		}
		/* <postfield> attr */
		else if( vtext && name && IFS_Stricmp( ename, "postfield" ) == 0 )
		{
			FS_WmlPostFieldAttr( doc, name, vtext );
		}
		/* <input> attr */
		else if( vtext && name && (IFS_Stricmp(ename, "input") == 0 || IFS_Stricmp(ename, "select") == 0) )
		{
			FS_WmlInputAttr( doc, name, vtext );
		}
		/* <card> attr */
		else if( vtext && name && IFS_Stricmp( ename, "card" ) == 0 )
		{
			if( IFS_Stricmp( name, "onenterforward" ) == 0 )
			{
				FS_WebDocNewTask( doc, FS_WTSK_GOTO );
				if( doc->task )
				{
					doc->task->url = IFS_Strdup( vtext );
					FS_WebDocAddTask( doc );
				}
			}
			else if( IFS_Stricmp( name, "ontimer" ) == 0 )
			{
				FS_WebDocNewTask( doc, FS_WTSK_TIMER );
				if( doc->task )
				{
					doc->task->url = IFS_Strdup( vtext );
					FS_WebDocAddTask( doc ); 
				}
			}
			else if( IFS_Stricmp( name, "title" ) == 0 )
			{
				FS_WebDocSetTitle( doc, vtext );
			}
		}
		/* <onevent> attr */
		else if( doc->task && vtext && name && IFS_Stricmp( ename, "onevent" ) == 0 )
		{
			if( IFS_Stricmp( name, "type" ) == 0 )
			{
				if( IFS_Stricmp( vtext, "onenterforward" ) == 0 )
					doc->task->type = FS_WTSK_GOTO;
				else if( IFS_Stricmp( vtext, "ontimer" ) == 0 )
					doc->task->type = FS_WTSK_TIMER;
			}
		}
		/* <select attr> */
		else if( IFS_Stricmp(ename, "select") == 0 && IFS_Stricmp(name, "value") == 0 && value )
		{
			if( doc->input ) FS_COPY_TEXT( doc->input->value, value );
		}
		/* <option attr> */
		else if( IFS_Stricmp(ename, "option") == 0 )
		{
			FS_WmlOptionAttr( doc, name, vtext );
		}
		/* <timer> attr */
		else if( IFS_Stricmp(ename, "timer") == 0 )
		{
			if( IFS_Stricmp( name, "value" ) == 0 && vtext )
			{
				FS_WebDocTimerTaskSetValue( doc, IFS_Atoi(vtext) );
			}
		}
		if( vtext && vtext != value )
			IFS_Free( vtext );
	}
}

static void FS_WmlDocNote( FS_WebDoc * doc, FS_CHAR *version, FS_CHAR *encoding )
{
	if( encoding )
		FS_COPY_TEXT( doc->charset, encoding );
}

FS_SaxHandle FS_WmlProcessFile( FS_Window *win, FS_CHAR *file, FS_CHAR *card, 
	FS_SaxDataRequest dataReq )
{
	FS_SaxHandle hsax = FS_NULL;
	FS_WebDoc *doc;

	doc = IFS_Malloc( sizeof(FS_WebDoc) );
	if( doc )
	{
		IFS_Memset( doc, 0, sizeof(FS_WebDoc) );
		win->context.url_var = FS_TRUE;
		doc->win = win;
		doc->charset = IFS_Strdup( "UTF-8" );	/* wml doc default to utf-8 charset */
		if( card )
		{
			doc->card_name = IFS_Strdup( card );
			doc->start = FS_FALSE;
		}
		else
		{
			doc->start = FS_TRUE;
		}
		
		hsax = FS_CreateSaxHandler( doc );
		if( dataReq == FS_NULL )
		{
			doc->file = IFS_Strdup( file );
			FS_SaxSetDataRequest( hsax, FS_WebDocFileRead );
		}
		else
		{
			FS_SaxSetDataRequest( hsax, dataReq );
		}

		FS_SaxSetStartElementHandler( hsax, FS_WmlStartElement );
		FS_SaxSetStartElementEndHandler( hsax, FS_WmlStartElementEnd );
		FS_SaxSetEndElementHandler( hsax, FS_WmlEndElement );
		FS_SaxSetElementTextHandler( hsax, FS_WmlElementText );
		FS_SaxSetAttributeHandler( hsax, FS_WmlElementAttr );
		FS_SaxSetCompleteHandler( hsax, FS_WebDocComplete );
		FS_SaxSetXmlNoteHandler( hsax, FS_WmlDocNote );
		FS_SaxProcXmlDoc( hsax );
	}
	return hsax;
}

#endif	//FS_MODULE_WEB


