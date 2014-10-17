#include "inc/FS_Config.h"

#ifdef FS_MODULE_WEB

#include "inc/FS_Config.h"
#include "inc/web/FS_Wbxml.h"
#include "inc/util/FS_Charset.h"
#include "inc/util/FS_Util.h"
#include "inc/web/FS_WebDoc.h"
#include "inc/inte/FS_Inte.h"
#include "inc/util/FS_MemDebug.h"

/* Tag Tokens */
#define FS_WMLTAG_A					0x1C
#define FS_WMLTAG_ANCHOR			0x22
#define FS_WMLTAG_BR				0x26
#define FS_WMLTAG_CARD				0x27
#define FS_WMLTAG_DO				0x28
#define FS_WMLTAG_FIELDSET			0x2A
#define FS_WMLTAG_GO				0x2B
#define FS_WMLTAG_IMG				0x2E
#define FS_WMLTAG_INPUT				0x2F
#define FS_WMLTAG_META				0x30
#define FS_WMLTAG_P					0x20
#define FS_WMLTAG_POSTFIELD			0x21
#define FS_WMLTAG_PREV				0x32
#define FS_WMLTAG_ONEVENT			0x33
#define FS_WMLTAG_OPTION			0x35
#define FS_WMLTAG_REFRESH			0x36
#define FS_WMLTAG_SELECT			0x37
#define FS_WMLTAG_SETVAR			0x3E
#define FS_WMLTAG_TEMPLATE			0x3B
#define FS_WMLTAG_TIMER				0x3C
#define FS_WMLTAG_TR				0x1E
#define FS_WMLTAG_WML				0x3F

/* Attribute Start Tokens */
#define FS_WMLATTR_ALT				0x0C
#define FS_WMLATTR_HREF				0x4A
#define FS_WMLATTR_HREF_HTTP		0x4B
#define FS_WMLATTR_HREF_HTTPS		0x4C
#define FS_WMLATTR_IVALUE			0x15
#define FS_WMLATTR_INAME			0x16
#define FS_WMLATTR_LABEL			0x18
#define FS_WMLATTR_METHOD_GET		0x1B
#define FS_WMLATTR_METHOD_POST		0x1C
#define FS_WMLATTR_MULTIPLE_FALSE	0x1F
#define FS_WMLATTR_MULTIPLE_TRUE	0x20
#define FS_WMLATTR_NAME				0x21
#define FS_WMLATTR_ONENTERBACKWARD	0x25
#define FS_WMLATTR_ONENTERFORWARD	0x26
#define FS_WMLATTR_ONPICK			0x24
#define FS_WMLATTR_ONTIMER			0x27
#define FS_WMLATTR_ONENTERBACKWARD	0x25
#define FS_WMLATTR_SENDREF_FALSE	0x2F
#define FS_WMLATTR_SENDREF_TRUE		0x30
#define FS_WMLATTR_SRC			 	0x32
#define FS_WMLATTR_SRC_HTTP			0x58
#define FS_WMLATTR_SRC_HTTPS		0x59
#define FS_WMLATTR_TITLE			0x36
#define FS_WMLATTR_TYPE				0x37
#define FS_WMLATTR_TYPE_ACCEPT		0x38
#define FS_WMLATTR_TYPE_DELETE		0x39
#define FS_WMLATTR_TYPE_HELP		0x3A
#define FS_WMLATTR_TYPE_PASSWORD	0x3B
#define FS_WMLATTR_TYPE_ONPICK		0x3C
#define FS_WMLATTR_TYPE_ONENTERBACKWARD		0x3D
#define FS_WMLATTR_TYPE_ONENTERFORWARD 		0x3E
#define FS_WMLATTR_TYPE_ONTIMER		0x3F
#define FS_WMLATTR_TYPE_PREV	 	0x46
#define FS_WMLATTR_TYPE_RESET	 	0x47
#define FS_WMLATTR_TYPE_TEXT	 	0x48
#define FS_WMLATTR_VALUE			0x4D
#define FS_WMLATTR_ID				0x55

/* Attribute Value Tokens */
#define FS_WMLATTRVAL_COM						0x85	/* .com/ */
#define FS_WMLATTRVAL_EDU						0x86	/* .edu/ */
#define FS_WMLATTRVAL_NET						0x87	/* .net/ */
#define FS_WMLATTRVAL_ORG						0x88	/* .org/ */
#define FS_WMLATTRVAL_HTTP						0x8E	/* http:// */
#define FS_WMLATTRVAL_HTTP_WWW					0x8F	/* http://www. */
#define FS_WMLATTRVAL_HTTPS						0x90	/* https:// */
#define FS_WMLATTRVAL_HTTPS_WWW					0x91	/* https://www. */
#define FS_WMLATTRVAL_ONENTERBACKWARD			0x96	/* onenterbackward */
#define FS_WMLATTRVAL_ONENTERFORWARD			0x97	/* onenterforward */
#define FS_WMLATTRVAL_ONPICK					0x95	/* onpick */
#define FS_WMLATTRVAL_ONTIMER					0x98	/* ontimer */
#define FS_WMLATTRVAL_PASSWORD					0x9A	/* password */
#define FS_WMLATTRVAL_RESET						0x9B	/* reset */
#define FS_WMLATTRVAL_TEXT	 					0x9D	/* text */
#define FS_WMLATTRVAL_WWW						0x9D	/* www. */

#define FS_MAX_WML_TEXT_LEN 		16384	/* 16 KB */
#define FS_WML_TMP_BUF( doc )		((FS_BYTE *)((doc)->file))
#define FS_WML_TMP_BUF_LEN( doc )	((doc)->offset)

extern void FS_WmlNewOption( FS_WebDoc *doc );

static FS_WbxmlAttrVal GFS_WmlAttrValTable[] = 
{
	{ FS_WMLATTRVAL_COM, ".com/" },
	{ FS_WMLATTRVAL_EDU, ".edu/" },
	{ FS_WMLATTRVAL_NET, ".net/" },
	{ FS_WMLATTRVAL_ORG, ".org/" },
	{ FS_WMLATTRVAL_HTTP, "http://" },
	{ FS_WMLATTRVAL_HTTP_WWW, "http://www." },
	{ FS_WMLATTRVAL_HTTPS, "https://" },
	{ FS_WMLATTRVAL_HTTPS_WWW, "https://www." },
	{ FS_WMLATTRVAL_ONENTERBACKWARD, "onenterbackward" },
	{ FS_WMLATTRVAL_ONENTERFORWARD, "onenterforward" },	
	{ FS_WMLATTRVAL_ONPICK, "onpick" },
	{ FS_WMLATTRVAL_ONTIMER, "ontimer" },	
	{ FS_WMLATTRVAL_PASSWORD, "password" },
	{ FS_WMLATTRVAL_RESET, "reset" },
	{ FS_WMLATTRVAL_TEXT, "text" },
	{ FS_WMLATTRVAL_WWW, "www." },
	
	{ 0, FS_NULL }
};

static FS_CHAR *FS_BinWmlAttrValue( FS_UINT1 tok )
{
	FS_UINT1 i = 0;
	
	while( GFS_WmlAttrValTable[i].value )
	{
		if( GFS_WmlAttrValTable[i].value == tok )
			return GFS_WmlAttrValTable[i].string;
		i ++;
	}
	return FS_NULL;
}

static void FS_BinWmlGetHrefValue( FS_CHAR **out, FS_WebDoc *doc, FS_WbxmlAttr *pAttr, FS_CHAR *in )
{
	/* <a> link */
	if( FS_WMLATTR_HREF == pAttr->attr_start )
	{
		FS_COPY_TEXT( *out, in );
	}
	else if( FS_WMLATTR_HREF_HTTP == pAttr->attr_start )
	{
		FS_SAFE_FREE( *out );
		*out = IFS_Malloc( 16 + IFS_Strlen(in) );
		if( *out )
		{
			IFS_Strcpy( *out, "http://" );
			IFS_Strcat( *out, in );
		}
	}
	else if( FS_WMLATTR_HREF_HTTPS == pAttr->attr_start )
	{
		FS_SAFE_FREE( *out );
		*out = IFS_Malloc( 16 + IFS_Strlen(in) );
		if( *out )
		{
			IFS_Strcpy( *out, "https://" );
			IFS_Strcat( *out, in );
		}
	}
}

static void FS_BinWmlCheckVarFlag( FS_PostField *postField, FS_WbxmlAttr *pAttr )
{
	FS_WbxmlToken *pTok;
	FS_List *node;

	node = pAttr->attr_value.next;
	while( node != &pAttr->attr_value )
	{
		pTok = FS_ListEntry( node, FS_WbxmlToken, list );
		node = node->next;

		if( pTok->value_type == 0x40 || pTok->value_type == 0x41 || pTok->value_type == 0x42
			|| pTok->value_type == 0x80 || pTok->value_type == 0x81 || pTok->value_type == 0x82 )
		{
			postField->is_var = FS_TRUE;
			return;
		}
	}
}

static void FS_BinWmlHandler( void *user_data, FS_UINT1 event, FS_UINT4 param )
{
	FS_WebDoc *doc = (FS_WebDoc *)user_data;
	FS_WbxmlAttr *pAttr;
	FS_WbxmlContent *pCnt;
	FS_CHAR *value, *tstr;
	FS_WebWgt *wwgt;
	FS_UINT4 stag;

	/* start element */
	if( event == FS_WBXML_EV_STAG )
	{
		if( ! doc->start ) return;
		
		if( param == FS_WMLTAG_BR || param == FS_WMLTAG_TR || param == FS_WMLTAG_P )
		{
			doc->new_line = 1;
		}
		else if( param == FS_WMLTAG_A )
		{
			FS_WebDocNewWebWgt( doc, FS_WWT_LINK );
		}
		else if( param == FS_WMLTAG_ANCHOR )
		{
			FS_WebDocNewWebWgt( doc, FS_WWT_ANCHOR );
		}
		else if( param == FS_WMLTAG_INPUT )
		{
			FS_WebDocNewWebWgt( doc, FS_WWT_TEXT );
		}
		else if( param == FS_WMLTAG_ONEVENT )
		{
			FS_WebDocNewTask( doc, FS_WTSK_GOTO );
		}
		else if( param == FS_WMLTAG_GO )
		{
			FS_WebDocNewForm( doc, FS_WFT_ANCHOR );
		}
		else if( param == FS_WMLTAG_POSTFIELD )
		{
			FS_WebDocNewPostField( doc );
		}
		else if( param == FS_WMLTAG_IMG )
		{
			if( ! doc->input )
				FS_WebDocNewWebWgt( doc, FS_WWT_IMAGE );
			else
				doc->input->type = FS_WWT_IMAGE;
		}
		else if( param == FS_WMLTAG_SELECT )
		{
			FS_WebDocNewWebWgt( doc, FS_WWT_SELECT );
		}
		else if( param == FS_WMLTAG_OPTION )
		{
			FS_WmlNewOption( doc );
		}
	}
	/* element attribute */
	else if( event == FS_WBXML_EV_ATTR )
	{
		pAttr = ( FS_WbxmlAttr * )param;
		IFS_Memset( FS_WML_TMP_BUF(doc), 0, FS_WML_TMP_BUF_LEN(doc) );
		FS_WbxmlAttrTokenValue( FS_WML_TMP_BUF(doc), FS_WML_TMP_BUF_LEN(doc), &pAttr->attr_value, FS_BinWmlAttrValue );
		value = FS_ProcessCharset( FS_WML_TMP_BUF(doc), -1, doc->charset, FS_NULL );
		if( value == FS_NULL )
		{
			value = FS_WML_TMP_BUF( doc );
		}
		
		if( value ) FS_ProcessEsc( value, -1 );;

		if( ! doc->start )
		{
			if( FS_WMLTAG_CARD == pAttr->stag && FS_WMLATTR_ID == pAttr->attr_start && FS_STR_I_EQUAL(value, doc->card_name) )
			{
				doc->start = FS_TRUE;
			}
			
			if( value && value != (FS_CHAR *)FS_WML_TMP_BUF(doc) )
				IFS_Free( value );
			
			return;
		}
		
		if( FS_WMLTAG_A == pAttr->stag && doc->input )
		{
			if( FS_WMLATTR_HREF == pAttr->attr_start 
				|| FS_WMLATTR_HREF_HTTP == pAttr->attr_start
				|| FS_WMLATTR_HREF_HTTPS == pAttr->attr_start )
			{
				FS_BinWmlGetHrefValue( &doc->input->link, doc, pAttr, value );
			}
		}
		else if( FS_WMLTAG_IMG == pAttr->stag && doc->input )
		{
			/* <img> src */
			if( pAttr->attr_start == FS_WMLATTR_SRC )
			{
				FS_COPY_TEXT(doc->input->src, value );
			}
			else if( pAttr->attr_start == FS_WMLATTR_SRC_HTTP )
			{
				FS_SAFE_FREE( doc->input->src );
				doc->input->src = IFS_Malloc( 16 + IFS_Strlen(value) );
				if( doc->input->src )
				{
					IFS_Strcpy( doc->input->src, "http://" );
					IFS_Strcat( doc->input->src, value );
				}
			}
			else if( pAttr->attr_start == FS_WMLATTR_SRC_HTTPS )
			{
				FS_SAFE_FREE( doc->input->src );
				doc->input->src = IFS_Malloc( 16 + IFS_Strlen(value) );
				if( doc->input->src )
				{
					IFS_Strcpy( doc->input->src, "https://" );
					IFS_Strcat( doc->input->src, value );
				}
			}
		}
		else if( FS_WMLTAG_GO == pAttr->stag && doc->form )
		{
			if( FS_WMLATTR_HREF == pAttr->attr_start 
				|| FS_WMLATTR_HREF_HTTP == pAttr->attr_start
				|| FS_WMLATTR_HREF_HTTPS == pAttr->attr_start )
			{
				FS_BinWmlGetHrefValue( &doc->form->action, doc, pAttr, value );
			}
			else if( FS_WMLATTR_SENDREF_TRUE == pAttr->attr_start )
			{
				doc->form->send_referer = FS_TRUE;
			}
			else if( FS_WMLATTR_METHOD_GET == pAttr->attr_start )
			{
				doc->form->method = IFS_Strdup( "GET" );
			}
			else if( FS_WMLATTR_METHOD_POST == pAttr->attr_start )
			{
				doc->form->method = IFS_Strdup( "POST" );
			}
		}
		else if( FS_WMLTAG_POSTFIELD == pAttr->stag && doc->post_field )
		{
			/* <postfield> attr */
			if( FS_WMLATTR_NAME == pAttr->attr_start )
			{
				FS_COPY_TEXT( doc->post_field->name, value );
			}
			else if( FS_WMLATTR_VALUE == pAttr->attr_start )
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
				}
				else
				{
					doc->post_field->is_var = FS_FALSE;
					FS_COPY_TEXT( doc->post_field->value, value );
				}

				/* wbxml use EXT_I_X and EXT_T_X as a var flag. so we check the var flag again */
				if( ! doc->post_field->is_var )
					FS_BinWmlCheckVarFlag( doc->post_field, pAttr );
			}
		}
		else if( doc->input && (FS_WMLTAG_INPUT == pAttr->stag || FS_WMLTAG_SELECT == pAttr->stag) )
		{
			/* <input> or <select> attr */
			if( FS_WMLATTR_NAME == pAttr->attr_start )
			{
				FS_COPY_TEXT( doc->input->name, value );
			}
			else if( FS_WMLATTR_INAME == pAttr->attr_start )
			{
				FS_WWGT_SET_IVALUE( doc->input );
				FS_COPY_TEXT( doc->input->name, value );
			}
			else if( FS_WMLATTR_VALUE == pAttr->attr_start || FS_WMLATTR_IVALUE == pAttr->attr_start )
			{
				FS_COPY_TEXT( doc->input->text, value );
			}
			else if( FS_WMLATTR_TYPE == pAttr->attr_start && FS_WMLTAG_INPUT == pAttr->stag )
			{
				if( IFS_Stricmp( value, "password") == 0 )
					doc->input->type = FS_WWT_PWD;
				else
					doc->input->type = FS_WWT_TEXT;
			}
			else if( FS_WMLATTR_TYPE_PASSWORD == pAttr->attr_start && FS_WMLTAG_INPUT == pAttr->stag )
			{
				doc->input->type = FS_WWT_PWD;
			}
			else if( FS_WMLATTR_TYPE_TEXT == pAttr->attr_start && FS_WMLTAG_INPUT == pAttr->stag )
			{
				doc->input->type = FS_WWT_TEXT;
			}
			else if( FS_WMLATTR_MULTIPLE_TRUE == pAttr->attr_start && FS_WMLTAG_SELECT == pAttr->stag )
			{
				FS_WWGT_SET_MULTI( doc->input );
			}
		}
		else if( FS_WMLTAG_CARD == pAttr->stag )
		{
			/* <card> attr */
			if( FS_WMLATTR_ONENTERFORWARD == pAttr->attr_start || FS_WMLATTR_ONTIMER == pAttr->attr_start )
			{
				if( FS_WMLATTR_ONENTERFORWARD == pAttr->attr_start )
					FS_WebDocNewTask( doc, FS_WTSK_GOTO );
				else
					FS_WebDocNewTask( doc, FS_WTSK_TIMER );
				if( doc->task )
				{
					doc->task->url = IFS_Strdup( value );
					FS_WebDocAddTask( doc );
				}
			}
			else if( FS_WMLATTR_TITLE == pAttr->attr_start )
			{
				FS_WebDocSetTitle( doc, value );
			}
		}
		else if( FS_WMLTAG_ONEVENT == pAttr->stag && doc->task )
		{
			if( FS_WMLATTR_TYPE == pAttr->attr_start )
			{
				if( IFS_Stricmp( value, "onenterforward" ) == 0 )
					doc->task->type = FS_WTSK_GOTO;
				else if( IFS_Stricmp( value, "ontimer" ) == 0 )
					doc->task->type = FS_WTSK_TIMER;
			}
			else if( FS_WMLATTR_TYPE_ONENTERFORWARD == pAttr->attr_start )
			{
				doc->task->type = FS_WTSK_GOTO;
			}
			else if( FS_WMLATTR_TYPE_ONTIMER == pAttr->attr_start )
			{
				doc->task->type = FS_WTSK_TIMER;
			}
		}
		else if( FS_WMLTAG_OPTION == pAttr->stag && doc->option )
		{
			/* <option attr> */ 
			if( FS_WMLATTR_VALUE == pAttr->attr_start )
			{
				FS_COPY_TEXT( doc->option->value, value );
			}
			else if( FS_WMLATTR_TITLE == pAttr->attr_start )
			{
				FS_COPY_TEXT( doc->option->label, value );
			}
			else if( FS_WMLATTR_TYPE_ONPICK == pAttr->attr_start )
			{
				FS_COPY_TEXT( doc->option->on_pick, value );
			}
		}
		else if( FS_WMLTAG_TIMER == pAttr->stag )
		{
			if( FS_WMLATTR_VALUE == pAttr->attr_start && value )
			{
				FS_WebDocTimerTaskSetValue( doc, IFS_Atoi(value) );
			}
		}
		
		if( value && value != (FS_CHAR *)FS_WML_TMP_BUF(doc) )
			IFS_Free( value );
	}
	else if( FS_WBXML_EV_CONTENT == event )
	{
		if( ! doc->start ) return;
		
		pCnt = ( FS_WbxmlContent * )param;
		IFS_Memset( FS_WML_TMP_BUF(doc), 0, FS_WML_TMP_BUF_LEN(doc) );
		FS_WbxmlAttrTokenValue( FS_WML_TMP_BUF(doc), FS_WML_TMP_BUF_LEN(doc), &pCnt->content, FS_BinWmlAttrValue );
		value = FS_ProcessCharset( FS_WML_TMP_BUF(doc), -1, doc->charset, FS_NULL );
		if( value == FS_NULL )
		{
			value = FS_WML_TMP_BUF(doc);
		}
		
		FS_ProcessEsc( value, -1 );

		/* hyperlink */
		if( doc->input && FS_WMLTAG_A == pCnt->stag )
		{
			FS_COPY_TEXT( doc->input->text, value );
		}
		else if( doc->input && FS_WMLTAG_ANCHOR == pCnt->stag )
		{
			FS_COPY_TEXT( doc->input->text, value );
		}
		else if( doc->option && FS_WMLTAG_OPTION == pCnt->stag )
		{
			if( doc->option->label == FS_NULL )
			{
				FS_COPY_TEXT( doc->option->label, value );
			}
		}
		else
		{
			wwgt = FS_WwCreateText( value );
			FS_WebWinAddWebWgt( doc->win, wwgt, doc->new_line );
			doc->new_line = 0;
		}
		
		if( value && value != (FS_CHAR *)FS_WML_TMP_BUF(doc) )
			IFS_Free( value );
	}
	else if( FS_WBXML_EV_TAGEND == event )
	{
		if( ! doc->start ) return;
		
		stag = param;
		if( FS_WMLTAG_POSTFIELD == stag )
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
		else if( FS_WMLTAG_INPUT == stag )
		{
			FS_WebDocAddWebWgt( doc );
		}
		/* <img> element */
		else if( FS_WMLTAG_IMG == stag )
		{
			if( doc->input && doc->input->link == FS_NULL )
				FS_WebDocAddWebWgt( doc );
		}
		else if( FS_WMLTAG_A == stag )
		{
			FS_WebDocAddWebWgt( doc );
		}
		else if( FS_WMLTAG_GO == stag )
		{
			if( doc->input && doc->input->type == FS_WWT_ANCHOR )
			{
				/* a anchor task <go> */
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
		else if( FS_WMLTAG_ANCHOR == stag )
		{
			FS_WebDocAddWebWgt( doc );
		}
		else if( FS_WMLTAG_ONEVENT == stag )
		{
			FS_WebDocAddTask( doc );
		}
		else if( FS_WMLTAG_SELECT == stag )
		{
			FS_WebDocAddWebWgt( doc );
		}
		else if( FS_WMLTAG_OPTION == stag )
		{
			FS_WebDocAddOption( doc );
		}
		else if( FS_WMLTAG_CARD == stag )
		{
			doc->start = FS_FALSE;	/* a card completed */
		}
	}
}

void FS_BinWmlProcessFile( FS_Window *win, FS_CHAR *file, FS_CHAR *card )
{
	FS_WebDoc *doc;

	doc = IFS_Malloc( sizeof(FS_WebDoc) );
	if( doc )
	{
		IFS_Memset( doc, 0, sizeof(FS_WebDoc) );
		win->context.url_var = FS_TRUE;
		doc->win = win;
		doc->charset = IFS_Strdup( "UTF-8" );	/* wml doc default to utf-8 charset */
		
		/* this field act as a temp buffer - will free when webdoc destroyed */
		doc->file = IFS_Malloc( FS_MAX_WML_TEXT_LEN );
		doc->offset = FS_MAX_WML_TEXT_LEN;
		
		if( card )
		{
			doc->card_name = IFS_Strdup( card );
			doc->start = FS_FALSE;
		}
		else
		{
			doc->start = FS_TRUE;
		}
		
		if( doc->file )
		{
			FS_WbxmlProcessFile( FS_DIR_WEB, file, FS_BinWmlHandler, doc );
		}

		FS_SAFE_FREE( doc->file );
		FS_SAFE_FREE( doc->charset );
		IFS_Free( doc );
		
		FS_InvalidateRect( win, FS_NULL );
	}
}

#endif	//FS_MODULE_WEB


