#ifndef _FS_WEB_DOC_H_
#define _FS_WEB_DOC_H_

#include "inc/FS_Config.h"
#include "inc/gui/FS_WebGui.h"
#include "inc/util/FS_Sax.h"

typedef struct FS_WebDoc_Tag
{
	FS_Window * 		win;

	FS_UINT1			new_line;
	FS_BOOL				start;		/* for wml to load a individual card */
	FS_WebForm *		form;
	FS_WebTask *		task;
	FS_WebWgt * 		input;
	FS_WebOption * 		option;
	FS_PostField *		post_field;
	FS_CHAR *			charset;
	FS_CHAR *			card_name;	/* for wml */
	
	FS_CHAR *			meta_name;
	FS_CHAR *			meta_content;
	
	FS_CHAR *			file;
	FS_SINT4			offset;
}FS_WebDoc;

/*--------------------------------- extern interface --------------------------------*/
FS_SaxHandle FS_HtmlProcessFile( FS_Window *win, FS_CHAR *file, FS_CHAR *charset, FS_SaxDataRequest dataReq );

FS_SaxHandle FS_WmlProcessFile( FS_Window *win, FS_CHAR *file, FS_CHAR *card, 
	FS_SaxDataRequest dataReq );

void FS_BinWmlProcessFile( FS_Window *win, FS_CHAR *file, FS_CHAR *card );

/*----------------------- share between vary web document type ----------------------*/
void FS_WebDocFileRead( FS_WebDoc *doc, FS_SaxHandle hsax );
void FS_WebDocComplete( FS_WebDoc *doc, FS_SaxHandle hsax );
void FS_WebDocNewWebWgt( FS_WebDoc *doc, FS_UINT1 type );
void FS_WebDocAddWebWgt( FS_WebDoc *doc );
void FS_WebDocNewForm( FS_WebDoc *doc, FS_UINT1 type );
void FS_WebDocNewPostField( FS_WebDoc *doc );
void FS_WebDocSetTitle( FS_WebDoc *doc, FS_CHAR *title );
void FS_WebDocAddTask( FS_WebDoc *doc );
void FS_WebDocAddForm( FS_WebDoc *doc );
void FS_WebDocNewTask( FS_WebDoc *doc, FS_UINT1 tskType );
void FS_WebDocAddImageTask( FS_WebDoc *doc, FS_CHAR *src, FS_WebWgt *wwgt );
void FS_WebDocAddFrameTask( FS_WebDoc *doc, FS_CHAR *src );
void FS_WebDocEndTag( FS_WebDoc *doc );
void FS_WebDocAddOption( FS_WebDoc *doc );
void FS_WebDocTimerTaskSetValue( FS_WebDoc *doc, FS_SINT4 time );

#endif


