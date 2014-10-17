#include "inc/FS_Config.h"

#ifdef FS_MODULE_WEB

#include "inc/util/FS_File.h"
#include "inc/util/FS_Util.h"
#include "inc/web/FS_WebDoc.h"
#include "inc/inte/FS_Inte.h"
#include "inc/util/FS_MemDebug.h"

/*--------------------------------------------macro define------------------------*/

/*--------------------------------------------type define------------------------*/

static void FS_HtmlFormAttr( FS_WebDoc *doc, FS_CHAR *name, FS_CHAR *value )
{
	if( doc->form == FS_NULL ) return;
	
	if( IFS_Stricmp(name, "method") == 0 )
	{
		FS_COPY_TEXT( doc->form->method, value );
	}
	else if( IFS_Stricmp(name, "action") == 0 )
	{
		FS_COPY_TEXT( doc->form->action, value );
	}
	else if( IFS_Stricmp(name, "name") == 0 )
	{
		FS_COPY_TEXT( doc->form->name, value );
	}
	/* TODO other form attribute */
}

static void FS_HtmlInputType( FS_WebWgt *wwgt, FS_CHAR *type )
{
	wwgt->type = FS_WWT_TEXT;
	if( type )
	{
		if( IFS_Stricmp(type, "text") == 0 )
			wwgt->type = FS_WWT_TEXT;
		else if( IFS_Stricmp(type, "password") == 0 )
			wwgt->type = FS_WWT_PWD;
		else if( IFS_Stricmp(type, "checkbox") == 0 )
			wwgt->type = FS_WWT_CHECK;
		else if( IFS_Stricmp(type, "radio") == 0 )
			wwgt->type = FS_WWT_RADIO;
		else if( IFS_Stricmp(type, "submit") == 0 )
			wwgt->type = FS_WWT_SUBMIT;
		else if( IFS_Stricmp(type, "reset") == 0 )
			wwgt->type = FS_WWT_RESET;
		else if( IFS_Stricmp(type, "hidden") == 0 )
			wwgt->type = FS_WWT_HIDDEN;
		else if( IFS_Stricmp(type, "select") == 0 )
			wwgt->type = FS_WWT_SELECT;
		else if( IFS_Stricmp(type, "textarea") == 0 )
			wwgt->type = FS_WWT_TEXTAREA;
		else if( IFS_Stricmp(type, "img") == 0 )
			wwgt->type = FS_WWT_IMAGE;
		else
			wwgt->type = FS_WWT_STR;	/* all unsupport widget just diaplay its text */
	}
}

static void FS_HtmlInputAttr( FS_WebDoc *doc, FS_CHAR *name, FS_CHAR *value )
{
	if( doc->input == FS_NULL ) return;
	
	if( IFS_Stricmp(name, "name") == 0 )
	{
		FS_COPY_TEXT( doc->input->name, value );
	}
	else if( IFS_Stricmp(name, "type") == 0 )
	{
		FS_HtmlInputType( doc->input, value );
	}
	else if( IFS_Stricmp(name, "value") == 0 )
	{
		FS_COPY_TEXT( doc->input->text, value );
	}
	else if( IFS_Stricmp(name, "checked") == 0 )
	{
		FS_WWGT_SET_CHECK( doc->input );
	}
	else if( IFS_Stricmp(name, "multiple") == 0 )
	{
		FS_WWGT_SET_MULTI( doc->input );
	}
	else if( IFS_Stricmp(name, "src") == 0 )
	{
		FS_COPY_TEXT( doc->input->src, value );
	}
	else if( IFS_Stricmp(name, "alt") == 0 )
	{
		FS_COPY_TEXT( doc->input->text, value );
	}
	else if( IFS_Stricmp(name, "href") == 0 )
	{
		FS_COPY_TEXT( doc->input->link, value );
	}
	else if( IFS_Stricmp(name, "title") == 0 && doc->input->type == FS_WWT_LINK )
	{
		FS_COPY_TEXT( doc->input->text, value );
	}
	else if( IFS_Stricmp(name, "format") == 0 )
	{
		if( doc->input->type == FS_WWT_TEXT || doc->input->type == FS_WWT_PWD || doc->input->type == FS_WWT_TEXTAREA )
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
	/* TODO other INPUT attribute */
}

static void FS_HtmlOptionAttr( FS_WebDoc *doc, FS_CHAR *name, FS_CHAR *value )
{
	if( doc->input == FS_NULL || doc->input->type != FS_WWT_SELECT )
		return;
	
	if( doc->option == FS_NULL )
	{
		doc->option = IFS_Malloc( sizeof(FS_WebOption) );
		if( doc->option )
			IFS_Memset( doc->option, 0, sizeof(FS_WebOption) );
		else
			return;
	}
	
	if( IFS_Stricmp(name, "value") == 0 )
	{
		FS_COPY_TEXT( doc->option->value, value );
	}
	else if( IFS_Stricmp(name, "selected") == 0 )
	{
		doc->option->selected = FS_TRUE;
	}
	else if( IFS_Stricmp(name, "label") == 0 )
	{
		FS_COPY_TEXT( doc->option->label, value );
	}
}

static void FS_HtmlContentType( FS_WebDoc *doc, FS_CHAR *content )
{
	FS_CHAR *p;

	/* 
		!!!
		some ugly html doc may contain two <meta> element, and inform two conflicted charset.
		we just use the first one.

		or we get charset from http header. use the header content-type charset first.
	*/
	if( doc->charset != FS_NULL )
		return;
	
	if( doc && content )
	{
		p = IFS_Strstr( content, "charset" );
		if( p )
		{
			while( *p && *p != '=' ) p ++;
			if( *p == '=' )
			{
				p ++;
				while( *p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') )
					p ++;
				if( *p )
				{
					FS_COPY_TEXT( doc->charset, p );
				}
			}
		}
	}
}

static void FS_HtmlRefreshTask( FS_WebDoc *doc, FS_CHAR *content )
{
	FS_SINT4 delay;
	FS_CHAR *url;

	delay = IFS_Atoi( content );
	
	url = IFS_Strstr( content, "url" );
	if( url == FS_NULL ) return;
	while( *url && *url != '=' ) url ++;
	if( *url == 0 ) return;
	url ++;
	while( *url && (*url == ' ' || *url == '\t' || *url == '\r' || *url == '\n') )
		url ++;
	if( *url == 0 ) return;
	
	FS_WebDocNewTask( doc, FS_WTSK_TIMER );
	if( doc->task )
	{
		doc->task->url = IFS_Strdup( url );
		doc->task->delay = delay;
		FS_WebDocAddTask( doc );
	}
}

static void FS_HtmlStartElement( void * userData, FS_CHAR *ename )
{
	FS_WebDoc *doc = (FS_WebDoc *)userData;

	if( ename == FS_NULL ) return;
	
	/* h1, h2, h ... h9 */
	if( FS_CHAR_EQUAL( ename[0], 'h') )
	{
		if( ename[1] >= '1' && ename[1] <= '9' )
			doc->new_line = 2;
	}
	/* <p> */
	else if( FS_CHAR_EQUAL( ename[0], 'p') && ename[1] == 0 )
	{
		doc->new_line = 1;
	}
	/* <tr> */
	else if( FS_CHAR_EQUAL( ename[0], 't') && FS_CHAR_EQUAL( ename[1], 'r') && ename[2] == 0 )
	{
		doc->new_line = 1;
	}
	/* <br> */
	else if( FS_CHAR_EQUAL( ename[0], 'b') && FS_CHAR_EQUAL( ename[1], 'r') && ename[2] == 0 )
	{
		doc->new_line = 1;
	}
	/* <li> */
	else if( FS_CHAR_EQUAL( ename[0], 'l') && FS_CHAR_EQUAL( ename[1], 'i') && ename[2] == 0 )
	{
		doc->new_line = 1;
	}
	/* hypelink */
	else if( IFS_Stricmp( ename, "a" ) == 0 )
	{
		FS_WebDocNewWebWgt( doc, FS_WWT_LINK );
	}
	/* <form> */
	else if( IFS_Stricmp( ename, "form" ) == 0 )
	{
		FS_WebDocNewForm( doc, FS_WFT_HTML );
	}
	/* <input> */
	else if( IFS_Stricmp( ename, "input" ) == 0 )
	{
		FS_WebDocNewWebWgt( doc, FS_WWT_TEXT );
	}
	/* <button> */
	else if( IFS_Stricmp( ename, "button" ) == 0 )
	{
		FS_WebDocNewWebWgt( doc, FS_WWT_STR );	/* we did not support button yet */
	}
	/* <select> */
	else if( IFS_Stricmp( ename, "select" ) == 0 )
	{
		FS_WebDocNewWebWgt( doc, FS_WWT_SELECT );
	}
	/* <img> */
	else if( IFS_Stricmp( ename, "img" ) == 0 )
	{
		if( ! doc->input )
			FS_WebDocNewWebWgt( doc, FS_WWT_IMAGE );
		else
			doc->input->type = FS_WWT_IMAGE;
	}
	/* <bgsound> */
	else if( IFS_Stricmp( ename, "bgsound" ) == 0 )
	{
		FS_WebDocNewTask( doc, FS_WTSK_SOUND );
	}
	/* <textarea> */
	else if( IFS_Stricmp( ename, "textarea" ) == 0 )
	{
		FS_WebDocNewWebWgt( doc, FS_WWT_TEXTAREA );
	}
	/* <option> */
	else if( IFS_Stricmp( ename, "option" ) == 0 )
	{
		FS_WebDocAddOption( doc );
	}
}

static void FS_HtmlStartElementEnd( void * userData, FS_CHAR *ename )
{
	FS_WebDoc *doc = (FS_WebDoc *)userData;
	
	if( ename == FS_NULL )
		return;

	if( doc->input && IFS_Stricmp(ename, "input") == 0 )
	{
		FS_WebDocAddWebWgt( doc );
	}
	else if( IFS_Stricmp( ename, "img" ) == 0 )
	{
		if( doc->input && doc->input->link == FS_NULL )
			FS_WebDocAddWebWgt( doc );
	}
	else if( FS_STR_I_EQUAL( ename, "meta" ) )
	{
		if( FS_STR_I_EQUAL(doc->meta_name, "Content-Type") )
		{
			FS_HtmlContentType( doc, doc->meta_content );
		}
		else if( FS_STR_I_EQUAL(doc->meta_name, "Refresh") )
		{
			FS_HtmlRefreshTask( doc, doc->meta_content );
		}
		FS_SAFE_FREE( doc->meta_name );
		FS_SAFE_FREE( doc->meta_content );
	}
}

static void FS_HtmlEndElement( void * userData, FS_CHAR *ename )
{
	FS_WebDoc *doc = (FS_WebDoc *)userData;

	if( ename == FS_NULL ) return;
	
	if( FS_CHAR_EQUAL(ename[0], 'a') && ename[1] == 0 )
	{
		FS_WebDocAddWebWgt( doc );
	}
	else if( IFS_Stricmp( ename, "form" ) == 0 )
	{
		FS_WebDocAddForm( doc );
	}
	else if( IFS_Stricmp( ename, "select" ) == 0 )
	{
		FS_WebDocAddOption( doc );
		FS_WebDocAddWebWgt( doc );
	}
	else if( (IFS_Stricmp( ename, "textarea" ) == 0)
		|| (IFS_Stricmp( ename, "button" ) == 0) )
	{
		FS_WebDocAddWebWgt( doc );
	}
	else if( IFS_Stricmp( ename, "html" ) == 0 )
	{
		/*
			we assume that, document parse end. use that to avoid stupid waiting
			for server further data.
			because some HTTP server may not send Content-Length header field.
		*/
		FS_WebDocEndTag( doc );
	}
}

static void FS_HtmlElementText( void * userData, FS_CHAR *ename, FS_CHAR *str, FS_SINT4 slen )
{
	FS_WebDoc *doc = (FS_WebDoc *)userData;
	FS_WebWgt *wwgt;
	FS_CHAR *text;
	FS_SINT4 olen = 0;
	
	if( ename && str && slen > 0 )
	{		
		text = FS_ProcessCharset( str, slen, doc->charset, &olen );
		if( text == FS_NULL )
		{
			text = FS_Strndup( str, slen );
			olen = slen;
		}
		
		if( text ) FS_ProcessEsc( text, olen );
		
		/* hypelink */
		if( (doc->input && IFS_Stricmp( ename, "a" ) == 0) || (doc->input && doc->input->type == FS_WWT_LINK) )
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
		else if( IFS_Stricmp(ename, "textarea") == 0 )
		{
			if( doc->input && doc->input->type == FS_WWT_TEXTAREA )
			{
				FS_COPY_TEXT( doc->input->text, text );
			}
		}
		else if( IFS_Stricmp(ename, "title") == 0 )
		{
			FS_WebDocSetTitle( doc, text );
		}
		else if( IFS_Stricmp(ename, "style") != 0 && IFS_Stricmp(ename, "script") != 0 )
		{
			wwgt = FS_WwCreateText( text );
			FS_WebWinAddWebWgt( doc->win, wwgt, doc->new_line );
			doc->new_line = 0;
		}
		IFS_Free( text );
	}
}

static void FS_HtmlElementAttr( void * userData, FS_CHAR *ename, FS_CHAR *name, FS_CHAR *value )
{
	FS_WebDoc *doc = (FS_WebDoc *)userData;
	FS_CHAR *vtext;

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
			FS_HtmlInputAttr( doc, name, vtext );
		}
		/* <form attr> */
		else if( IFS_Stricmp(ename, "form") == 0 )
		{
			FS_HtmlFormAttr( doc, name, vtext );
		}
		/* <input attr>, <button attr>, <seelct attr> */
		else if( ( IFS_Stricmp(ename, "input") == 0 ) 
			|| ( IFS_Stricmp(ename, "button") == 0 )
			|| ( IFS_Stricmp(ename, "select") == 0 )
			|| ( IFS_Stricmp(ename, "img") == 0 ) 
			|| ( IFS_Stricmp(ename, "textarea") == 0 ) )
		{
			FS_HtmlInputAttr( doc, name, vtext );
		}
		/* <option attr> */
		else if( IFS_Stricmp(ename, "option") == 0 )
		{
			FS_HtmlOptionAttr( doc, name, vtext );
		}
		/* <meta attr> */
		else if( IFS_Stricmp(ename, "meta") == 0 )
		{
			if( FS_STR_I_EQUAL(name, "http-equiv") )
			{
				FS_COPY_TEXT( doc->meta_name, vtext );
			}
			else if( FS_STR_I_EQUAL(name, "content") )
			{
				FS_COPY_TEXT( doc->meta_content, vtext );
			}
		}
		else if( FS_STR_I_EQUAL(ename, "frame") )
		{
			if( IFS_Stricmp(name, "src") == 0 && vtext ) 
				FS_WebDocAddFrameTask( doc, vtext );
		}
		else if( FS_STR_I_EQUAL(ename, "bgsound") )
		{
			if( FS_STR_I_EQUAL(name, "src") && vtext && doc->task ) 
			{
				doc->task->url = IFS_Strdup( vtext );
				doc->task->is_embed = FS_TRUE;
				FS_WebDocAddTask( doc );
			}
		}
		if( vtext != value )
			IFS_Free( vtext );
	}
}

static void FS_HtmlDocNote( FS_WebDoc * doc, FS_CHAR *version, FS_CHAR *encoding )
{
	if( encoding )
		FS_COPY_TEXT( doc->charset, encoding );
}

FS_SaxHandle FS_HtmlProcessFile( FS_Window *win, FS_CHAR *file, FS_CHAR *charset, FS_SaxDataRequest dataReq )
{
	FS_SaxHandle hsax = FS_NULL;
	FS_WebDoc *doc;

	doc = IFS_Malloc( sizeof(FS_WebDoc) );
	if( doc )
	{
		IFS_Memset( doc, 0, sizeof(FS_WebDoc) );
		doc->win = win;
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

		if( charset && charset[0] ) doc->charset = IFS_Strdup( charset );
		FS_SaxSetStartElementHandler( hsax, FS_HtmlStartElement );
		FS_SaxSetStartElementEndHandler( hsax, FS_HtmlStartElementEnd );
		FS_SaxSetEndElementHandler( hsax, FS_HtmlEndElement );
		FS_SaxSetElementTextHandler( hsax, FS_HtmlElementText );
		FS_SaxSetAttributeHandler( hsax, FS_HtmlElementAttr );
		FS_SaxSetXmlNoteHandler( hsax, FS_HtmlDocNote );
		FS_SaxSetCompleteHandler( hsax, FS_WebDocComplete );
		FS_SaxProcXmlDoc( hsax );
	}
	return hsax;
}

#endif	//FS_MODULE_WEB


