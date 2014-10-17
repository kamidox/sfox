#include "inc/util/FS_Image.h"
#include "inc/util/FS_File.h"
#include "inc/util/FS_List.h"
#include "inc/util/FS_Util.h"
#include "inc/inte/FS_Inte.h"
#include "inc/util/FS_MemDebug.h"

#define FS_IM_MAX_TO_KEEP			(16 * 1024)
#define FS_IM_TO_BIG_TOO_KEEP(img)	(((img)->bitmap.pitch * (img)->bitmap.height) > FS_IM_MAX_TO_KEEP )

typedef struct FS_ImageData_Tag
{
	FS_List			list;
	
	FS_CHAR *		file;
	FS_UINT4		hImage;
	FS_Bitmap 		bitmap;
	FS_SINT4		w;
	FS_SINT4		h;
	FS_BOOL			need_release;
	
	FS_ImCallback	callback;
	void *			context;
	FS_CHAR			raw_bitmap[FS_FILE_NAME_LEN];
}FS_ImageData;

static FS_List GFS_ImList = { &GFS_ImList, &GFS_ImList };

static FS_ImageData *FS_ImFind( FS_UINT4 hImage )
{
	FS_List *node;
	FS_ImageData *img;
	
	node = GFS_ImList.next;
	while( node != &GFS_ImList )
	{
		img = FS_ListEntry( node, FS_ImageData, list );
		node = node->next;
		if( img->hImage == hImage )
			return img;
	}
	return FS_NULL;
}

FS_ImHandle FS_ImCreate( FS_CHAR *file, FS_ImCallback cb, void *context )
{
	FS_ImageData *img = IFS_Malloc( sizeof(FS_ImageData) );
	if( img )
	{
		IFS_Memset( img, 0, sizeof(FS_ImageData) );
		img->file = IFS_Strdup( file );
		img->callback = cb;
		img->context = context;
		FS_ListAdd( &GFS_ImList, &img->list );
	}
	return img;
}

FS_BOOL FS_ImGetSize( FS_ImHandle hImg, FS_SINT4 *w, FS_SINT4 *h )
{
	FS_ImageData *img = (FS_ImageData *)hImg;
	if( img )
		return FS_ImageGetSize( img->file, w, h );
	else
		return FS_FALSE;
}

FS_Bitmap * FS_ImDecode( FS_ImHandle hImg, FS_SINT4 w, FS_SINT4 h )
{
	FS_SINT4 size;
	FS_ImageData *img = (FS_ImageData *)hImg;

	if( img == FS_NULL )
		return FS_NULL;
	
	if( img->w == 0 || img->h == 0 )
	{
		/* image is dirty, decode image file */
		img->hImage = FS_ImageDecode( img->file, w, h, &img->bitmap );
		if( img->bitmap.bits )
		{
			/* decode ok */
			img->need_release = FS_TRUE;
		}
	}

	if( img->bitmap.bits )
	{
		img->w = w;
		img->h = h;
	}
	else if( img->raw_bitmap[0] )
	{
		/* image raw data is write to file */
		size = FS_FileGetSize( FS_DIR_TMP, img->raw_bitmap );
		if( size > 0 )
		{
			img->bitmap.bits = IFS_Malloc( size );
			if( img->bitmap.bits )
			{
				FS_FileRead( FS_DIR_TMP, img->raw_bitmap, 0, img->bitmap.bits, size );
			}
		}
	}
	
	if( img->bitmap.bits )
		return &img->bitmap;
	else
		return FS_NULL;
}

void FS_ImRelease( FS_ImHandle hImg )
{
	FS_ImageData *img = (FS_ImageData *)hImg;
	
	if( img == FS_NULL )
		return;
	
	if( img->bitmap.bits )
	{
		if( FS_IM_TO_BIG_TOO_KEEP(img) )
		{
			/* too big. write raw data to file */
			
			if( img->raw_bitmap[0] == 0 )
			{
				FS_GetLuid( img->raw_bitmap );
				IFS_Strcat( img->raw_bitmap, ".raw" );
				FS_FileWrite( FS_DIR_TMP, img->raw_bitmap, 0, img->bitmap.bits, img->bitmap.pitch * img->bitmap.height );
			}
			
			if( img->need_release )
			{
				img->need_release = FS_FALSE;
				FS_ImageRelease( img->hImage, &img->bitmap );
			}
			else
			{
				IFS_Free( img->bitmap.bits );
			}
			img->bitmap.bits = FS_NULL;
		}
	}
}

void FS_ImDestroy( FS_ImHandle hImg )
{
	FS_ImageData *img = (FS_ImageData *)hImg;

	FS_ListDel( &img->list );
	if( img->bitmap.bits )
	{
		if( img->need_release )
			FS_ImageRelease( img->hImage, &img->bitmap );
		else
			IFS_Free( img->bitmap.bits );
	}
	if( img->raw_bitmap[0] )
		FS_FileDelete( FS_DIR_TMP, img->raw_bitmap );
	if( img->hImage )
		FS_ImageDestroy( img->hImage );
	FS_SAFE_FREE( img->file );
	
	IFS_Free( hImg );
}

void FS_ImageFrameInd( FS_UINT4 hImage, FS_Bitmap *pBmp )
{
	FS_ImageData *img =  FS_ImFind( hImage );
	
	if( img == FS_NULL )
		return;
	
	if( img->bitmap.bits )
	{
		IFS_Memcpy( img->bitmap.bits, pBmp->bits, img->bitmap.pitch * img->bitmap.height );
	}
	else
	{
		FS_FileWrite( FS_DIR_TMP, img->raw_bitmap, 0, pBmp->bits, img->bitmap.pitch * img->bitmap.height );
	}

	img->callback( img->context, pBmp );
}

