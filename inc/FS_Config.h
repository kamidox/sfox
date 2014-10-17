#ifndef _FS_HEAD_H_
#define _FS_HEAD_H_

/* platform select is defined in project file */

/* software feature list */
#define FS_MODULE_WEB
#define FS_MODULE_MMS
#define FS_MODULE_EML
#define FS_MODULE_DCD
#define FS_MODULE_STK
#define FS_MODULE_SNS

#define FS_MODULE_MM
#define FS_MODULE_FONT
#define FS_MODULE_IMAGE
#define FS_MODULE_SSL
#define FS_MODULE_QVGA_RES

#define FS_DEBUG_
#define FS_TRACE_

/* platform config */
#ifdef FS_PLT_WIN
#undef FS_MODULE_MM
#undef FS_MODULE_QVGA_RES

#undef FS_MODULE_SSL

#undef FS_MODULE_DCD
#undef FS_MODULE_WEB
#undef FS_MODULE_MMS
#undef FS_MODULE_EML
#undef FS_MODULE_STK

#endif

#ifdef FS_PLT_VIENNA
#undef FS_MODULE_FONT
#undef FS_MODULE_MM
#undef FS_DEBUG_
#undef FS_TRACE_
#endif

#ifdef FS_PLT_SPT
#undef FS_DEBUG_
#undef FS_TRACE_

#undef FS_MODULE_FONT
#undef FS_MODULE_MM
#undef FS_MODULE_IMAGE
#undef FS_MODULE_SSL

#undef FS_MODULE_DCD
#undef FS_MODULE_WEB
#undef FS_MODULE_MMS
#undef FS_MODULE_EML
#undef FS_MODULE_STK

#define FS_USE_NATIVE_DRAW_CHAR

#endif

#include "FS_Export.h"

#endif
