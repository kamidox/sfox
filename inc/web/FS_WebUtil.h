#ifndef _FS_WEB_UTIL_H_
#define _FS_WEB_UTIL_H_

#include "inc/FS_Config.h"
#include "inc/gui/FS_Gui.h"

void FS_WebGoToUrl( FS_Window *win, FS_CHAR *url, FS_BOOL send_ref );

/* post simple data. use application/x-www-form-urlencoded content-type */
void FS_WebPostData( FS_Window *win, FS_CHAR *url, FS_CHAR *data, FS_SINT4 dlen, FS_BOOL send_referer );

void FS_QuitTransSession( void );

void FS_WebUtilDeinit( void );

void FS_LoadWebDoc( FS_Window *win, FS_CHAR *file, FS_CHAR *card, 
	FS_BOOL bSetTop, FS_BOOL bPushHistory );

void FS_SetCurWebPageUrl( FS_CHAR *url );

FS_CHAR *FS_GetCurWebPageUrl( void );

void FS_SetCurRequestUrl( FS_CHAR *url );

FS_CHAR *FS_GetCurRequestUrl( void );

FS_BOOL FS_WebNetBusy( void );

FS_BOOL FS_SFoxWebScript( FS_CHAR *strscript );

#endif
