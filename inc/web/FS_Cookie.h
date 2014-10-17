#ifndef _FS_COOKIE_H_
#define _FS_COOKIE_H_

#include "inc/FS_Config.h"

FS_SINT4 FS_CookieGet( FS_CHAR *out, FS_SINT4 olen, FS_CHAR *url );

void FS_CookieClear( void );

void FS_CookieSave( FS_CHAR *url, FS_CHAR *cookies );

FS_SINT4 FS_CookieGetSize( void );

#endif
