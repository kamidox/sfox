#ifndef _FS_TRUE_TYPE_H_
#define _FS_TRUE_TYPE_H_

#include "inc/FS_Config.h"

typedef void * FS_TTFontHandle;

FS_TTFontHandle FS_TTFontCreate(FS_CHAR *fontfile);

void FS_TTFontDestroy(FS_TTFontHandle hTTFont);

#endif
