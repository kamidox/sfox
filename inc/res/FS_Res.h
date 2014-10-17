#ifndef _FS_RES_H_
#define _FS_RES_H_

#include "inc/FS_Config.h"
#include "inc/gui/FS_GrpLib.h"
#include "inc/res/FS_TimerID.h"
#include "inc/res/FS_TextID.h"
#include "inc/res/FS_IconID.h"
#include "inc/res/FS_WidgetID.h"

FS_Bitmap * FS_Icon( FS_SINT4 id );
void FS_ReleaseIcon( FS_Bitmap *pBmp );
FS_CHAR * FS_Text( FS_SINT4 id );
void FS_SetLanguage( FS_UINT4 lan );
FS_UINT1 FS_GetLanguage( void );

#endif
