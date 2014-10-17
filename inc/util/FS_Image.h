#ifndef _FS_IMAGE_H_
#define _FS_IMAGE_H_

#include "inc/FS_Config.h"

typedef void * FS_ImHandle;

typedef void (*FS_ImCallback) ( void *context, FS_Bitmap *newBmp );

FS_ImHandle FS_ImCreate( FS_CHAR *file, FS_ImCallback cb, void *context );

FS_BOOL FS_ImGetSize( FS_ImHandle hImg, FS_SINT4 *w, FS_SINT4 *h );

FS_Bitmap * FS_ImDecode( FS_ImHandle hImg, FS_SINT4 w, FS_SINT4 h );

void FS_ImRelease( FS_ImHandle hImg );

void FS_ImDestroy( FS_ImHandle hImg );

void FS_ImageScale( FS_SINT4 *w, FS_SINT4 *h, FS_SINT4 max_w );

FS_BOOL FS_ImageGetSize( char *filename, int *w, int *h );

FS_UINT4 FS_ImageDecode( FS_CHAR *file, FS_SINT4 w, FS_SINT4 h, FS_Bitmap *pBmp );

void FS_ImageRelease( FS_UINT4 hImage, FS_Bitmap *pBmp );

void FS_ImageDestroy( FS_UINT4 hImage );

#endif

