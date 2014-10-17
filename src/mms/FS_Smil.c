#include "inc/FS_Config.h"

#ifdef FS_MODULE_MMS

#include "inc/mms/FS_Smil.h"
#include "inc/util/FS_Sax.h"
#include "inc/util/FS_Util.h"
#include "inc/inte/FS_Inte.h"
#include "inc/util/FS_MemDebug.h"

#define FS_MMS_SMIL_TEXT_REGION		"Text"
#define FS_MMS_SMIL_IMAGE_REGION 	"Image"

#define FS_MAX_MMS_SMIL_LEN			8192

typedef struct FS_SmilParam_Tag
{
	FS_MmsSmil *		smil;
	FS_CHAR *			file;

	FS_SmilRegion *		region;
	FS_SmilPar *		par;
	FS_SmilObject *		obj;
}FS_SmilParam;

static FS_CHAR *FS_SmilFormatCid( FS_CHAR * cid )
{
	FS_SINT4 size = IFS_Strlen( cid );
	FS_CHAR *ret = IFS_Malloc( size + 6 );

	if( cid[0] == '<' )
		cid ++;
	IFS_Strcpy( ret, "cid:" );
	IFS_Strcat( ret, cid );
	size = IFS_Strlen( ret ) - 1;
	if( ret[size] == '>' )
		ret[size] = 0;

	return ret;
}

static void FS_SmilFreeObject( FS_SmilObject *obj )
{
	FS_SAFE_FREE( obj->src );
	FS_SAFE_FREE( obj->region );
	IFS_Free( obj );
}

static void FS_SmilFreePar( FS_SmilPar *par )
{
	if( par->text ) FS_SmilFreeObject( par->text );
	if( par->image ) FS_SmilFreeObject( par->image );
	if( par->audio ) FS_SmilFreeObject( par->audio );
	if( par->video ) FS_SmilFreeObject( par->video );
	if( par->ref ) FS_SmilFreeObject( par->ref );
	
	IFS_Free( par );
}

static FS_SmilPar *FS_SmilGetPar( FS_MmsSmil *pSmil, FS_SINT4 frameNum )
{
	FS_List *node;
	FS_SmilPar *par;

	if( pSmil )
	{
		node = pSmil->par_list.next;
		while( node != &pSmil->par_list )
		{
			par = FS_ListEntry( node, FS_SmilPar, list );
			node = node->next;
			frameNum --;
			
			if( frameNum == 0 )
			{
				/* we found it */
				return par;
			}
		}
	}
	return FS_NULL;
}

static FS_SmilRegion *FS_SmilGetRegion( FS_MmsSmil *pSmil, FS_CHAR *regname )
{
	FS_List *node;
	FS_SmilRegion *region;

	node = pSmil->region_list.next;
	while( node != &pSmil->region_list )
	{
		region = FS_ListEntry( node, FS_SmilRegion, list );
		node = node->next;

		if( FS_STR_I_EQUAL(region->id, regname) )
			return region;
	}
	return FS_NULL;
}

void FS_SmilFileRead( FS_SmilParam *param, FS_SaxHandle hsax )
{
	FS_SINT4 rlen;
	FS_BYTE *buf;

	rlen = FS_FileGetSize( -1, param->file );
	if( rlen > 0 )
	{
		buf = IFS_Malloc( rlen );		
		if( buf )
		{
			IFS_Memset( buf, 0, rlen );
			FS_FileRead( -1, param->file, 0, buf, rlen );
			FS_SaxDataFeed( hsax, buf, rlen, FS_TRUE );
			IFS_Free( buf );
		}
	}
}

static void FS_SmilStartElement( FS_SmilParam *param, FS_CHAR *ename )
{
	if( IFS_Stricmp(ename, "region") == 0 && param->region == FS_NULL )
	{
		param->region = IFS_Malloc( sizeof(FS_SmilRegion) );
		if( param->region ) IFS_Memset( param->region, 0, sizeof(FS_SmilRegion) );
	}
	else if( IFS_Stricmp(ename, "par") == 0 && param->par == FS_NULL )
	{
		param->par = IFS_Malloc( sizeof(FS_SmilPar) );
		if( param->par ) IFS_Memset( param->par, 0, sizeof(FS_SmilPar) );
	}
	else if( IFS_Stricmp(ename, "text") == 0
		|| IFS_Stricmp(ename, "img") == 0
		|| IFS_Stricmp(ename, "audio") == 0
		|| IFS_Stricmp(ename, "video") == 0
		|| IFS_Stricmp(ename, "ref") == 0)
	{
		if( param->par && param->obj == FS_NULL )
		{
			param->obj = IFS_Malloc( sizeof(FS_SmilObject) );
			if( param->obj ) IFS_Memset( param->obj, 0, sizeof(FS_SmilObject) );
		}
	}
}

static void FS_SmilElementAttr( FS_SmilParam *param, FS_CHAR *ename, FS_CHAR *name, FS_CHAR *value )
{
	FS_MmsSmil *pSmil = param->smil;

	if( ename == FS_NULL || name == FS_NULL || value == FS_NULL )
		return;

	if( IFS_Stricmp(ename, "root-layout") == 0 )
	{
		if( IFS_Stricmp(name, "width") == 0 )
		{
			pSmil->root_width = IFS_Atoi( value );
		}
		else if( IFS_Stricmp(name, "height") == 0 )
		{
			pSmil->root_height = IFS_Atoi( value );
		}
	}
	else if( param->region && IFS_Stricmp(ename, "region") == 0 )
	{
		if( IFS_Stricmp(name, "id") == 0 && param->region->id == FS_NULL )
		{
			param->region->id = IFS_Strdup( value );
		}
		else if( IFS_Stricmp(name, "left") == 0 )
		{
			param->region->left = IFS_Atoi( value );
		}
		else if( IFS_Stricmp(name, "top") == 0 )
		{
			param->region->top = IFS_Atoi( value );
		}
		else if( IFS_Stricmp(name, "width") == 0 )
		{
			param->region->width = IFS_Atoi( value );
		}
		else if( IFS_Stricmp(name, "height") == 0 )
		{
			param->region->height = IFS_Atoi( value );
		}
	}
	else if( param->par && IFS_Stricmp(ename, "par") == 0 )
	{
		if( IFS_Stricmp(name, "dur") == 0 )
			param->par->dur = IFS_Atoi( value );
	}
	else if( param->par && IFS_Stricmp(ename, "text") == 0
		|| param->par && IFS_Stricmp(ename, "img") == 0
		|| param->par && IFS_Stricmp(ename, "audio") == 0
		|| param->par && IFS_Stricmp(ename, "video") == 0
		|| param->par && IFS_Stricmp(ename, "ref") == 0)
	{
		if( param->obj )
		{
			if( IFS_Stricmp(name, "src") == 0 )
			{
				param->obj->src = IFS_Strdup( value );
			}
			else if( IFS_Stricmp(name, "region") == 0 )
			{
				param->obj->region = IFS_Strdup( value );
			}
		}
	}
}

static void FS_SmilEndElement( FS_SmilParam *param, FS_CHAR *ename )
{
	if( IFS_Stricmp(ename, "region") == 0 && param->region )
	{
		FS_ListAddTail( &param->smil->region_list, &param->region->list );
		param->region = FS_NULL;
	}
	else if( IFS_Stricmp(ename, "par") == 0 && param->par )
	{
		FS_ListAddTail( &param->smil->par_list, &param->par->list );
		param->par = FS_NULL;
	}
	else if( IFS_Stricmp(ename, "text") == 0 && param->par && param->obj )
	{
		if( param->par->text == FS_NULL )			
			param->par->text = param->obj;
		else
			FS_SmilFreeObject( param->obj );
		
		param->obj = FS_NULL;
	}
	else if( IFS_Stricmp(ename, "img") == 0 && param->par && param->obj )
	{
		if( param->par->image == FS_NULL )			
			param->par->image = param->obj;
		else
			FS_SmilFreeObject( param->obj );
		
		param->obj = FS_NULL;
	}
	else if( IFS_Stricmp(ename, "audio") == 0 && param->par && param->obj )
	{
		if( param->par->audio == FS_NULL )			
			param->par->audio = param->obj;
		else
			FS_SmilFreeObject( param->obj );
		
		param->obj = FS_NULL;
	}
	else if( IFS_Stricmp(ename, "video") == 0 && param->par && param->obj )
	{
		if( param->par->video == FS_NULL )			
			param->par->video = param->obj;
		else
			FS_SmilFreeObject( param->obj );
		
		param->obj = FS_NULL;
	}
	else if( IFS_Stricmp(ename, "ref") == 0 && param->par && param->obj )
	{
		if( param->par->ref == FS_NULL )			
			param->par->ref = param->obj;
		else
			FS_SmilFreeObject( param->obj );
		
		param->obj = FS_NULL;
	}
}

/* absFile : must be the smil file's abs path */
FS_MmsSmil * FS_SmilDecodeFile( FS_CHAR *absFile )
{
	FS_MmsSmil *pSmil = FS_NULL;
	FS_SaxHandle hsax = FS_NULL;
	FS_SmilParam *param;

	param = IFS_Malloc( sizeof(FS_SmilParam) );
	if( param )
	{
		IFS_Memset( param, 0, sizeof(FS_SmilParam) );
		param->file = IFS_Strdup( absFile );
		pSmil = IFS_Malloc( sizeof(FS_MmsSmil) );
		param->smil = pSmil;
		if( pSmil )
		{
			IFS_Memset( pSmil, 0, sizeof(FS_MmsSmil) );
			FS_ListInit( &pSmil->region_list );
			FS_ListInit( &pSmil->par_list );
			
			hsax = FS_CreateSaxHandler( param );
			FS_SaxSetDataRequest( hsax, FS_SmilFileRead );
			FS_SaxSetStartElementHandler( hsax, FS_SmilStartElement );
			FS_SaxSetAttributeHandler( hsax, FS_SmilElementAttr );
			FS_SaxSetEndElementHandler( hsax, FS_SmilEndElement );
			FS_SaxProcXmlDoc( hsax );
		}
		if( param->par ) FS_SmilFreePar( param->par );
		if( param->obj ) FS_SmilFreeObject( param->obj );
		IFS_Free( param->file );
		IFS_Free( param );
		if( hsax ) FS_FreeSaxHandler( hsax );
	}
	
	return pSmil;
}

void FS_FreeMmsSmil( FS_MmsSmil *pSmil )
{
	FS_List *node;
	FS_SmilRegion *region;
	FS_SmilPar *par;
	
	node = pSmil->region_list.next;
	while( node != &pSmil->region_list )
	{
		region = FS_ListEntry( node, FS_SmilRegion, list );
		node = node->next;
		
		FS_ListDel( &region->list );
		FS_SAFE_FREE( region->id );
		IFS_Free( region );
	}

	node = pSmil->par_list.next;
	while( node != &pSmil->par_list )
	{
		par = FS_ListEntry( node, FS_SmilPar, list );
		node = node->next;
		
		FS_ListDel( &par->list );
		FS_SmilFreePar( par );
	}

	IFS_Free( pSmil );
}

FS_MmsSmil *FS_CreateDefaultMmsSmil( void )
{
	FS_MmsSmil *pSmil;
	FS_SmilRegion *region;
	
	pSmil = FS_NEW( FS_MmsSmil );
	if( pSmil )
	{
		IFS_Memset( pSmil, 0, sizeof(FS_MmsSmil) );
		FS_ListInit( &pSmil->region_list );
		FS_ListInit( &pSmil->par_list );

		pSmil->root_width = 100;
		pSmil->root_height = 100;
		/* text region */
		region = FS_NEW( FS_SmilRegion );
		if( region )
		{
			IFS_Memset( region, 0, sizeof(FS_SmilRegion) );
			region->id = IFS_Strdup( FS_MMS_SMIL_TEXT_REGION );
			region->left = 0;
			region->top = 0;
			region->height = 50;
			region->width = 100;
			FS_ListAddTail( &pSmil->region_list, &region->list );
		}
		/* image region */
		region = FS_NEW( FS_SmilRegion );
		if( region )
		{
			IFS_Memset( region, 0, sizeof(FS_SmilRegion) );
			region->id = IFS_Strdup( FS_MMS_SMIL_IMAGE_REGION );
			region->left = 0;
			region->top = 50;
			region->height = 50;
			region->width = 100;
			FS_ListAddTail( &pSmil->region_list, &region->list );
		}
	}
	return pSmil;
}

void FS_SmilAddMmsFrame( FS_MmsSmil *pSmil, FS_SINT4 index )
{
	FS_SmilPar *par;
	FS_List *head, *node;

	if( pSmil == FS_NULL ) return;
	
	if( index > 0 )
	{
		node = pSmil->par_list.next;
		while( node != &pSmil->par_list )
		{
			node = node->next;
			head = node;
			index --;
			if( index == 0 )
				break;
		}
	}
	else
	{
		head = &pSmil->par_list;
	}
	
	par = FS_NEW( FS_SmilPar );
	if( par )
	{
		IFS_Memset( par, 0, sizeof(FS_SmilPar) );
		par->dur = 30000;	/* default to 30 second per frame */
		FS_ListAddTail( head, &par->list );
	}
}

void FS_SmilDelMmsFrame( FS_MmsSmil *pSmil, FS_SINT4 frameNum )
{
	FS_SmilPar *par;
	
	par = FS_SmilGetPar( pSmil, frameNum );
	if( par )
	{
		FS_ListDel( &par->list );
		FS_SmilFreePar( par );
	}
}

void FS_SmilEncodeFile( FS_CHAR *absFile, FS_MmsSmil *pSmil )
{
	FS_CHAR *buf;
	FS_SINT4 offset = 0;
	FS_List *node;
	FS_SmilRegion *region;
	FS_SmilPar *par;
	
	buf = IFS_Malloc( FS_MAX_MMS_SMIL_LEN );
	if( buf )
	{
		IFS_Memset( buf, 0, FS_MAX_MMS_SMIL_LEN );
		IFS_Sprintf( buf + offset, "<smil>\n\t<head>\n\t\t<layout>\n\t\t\t<root-layout width=\"%d%%\" height=\"%d%%\" />", pSmil->root_width, pSmil->root_height );
		offset += IFS_Strlen( buf + offset );

		node = pSmil->region_list.next;
		while( node != &pSmil->region_list )
		{
			region = FS_ListEntry( node, FS_SmilRegion, list );
			node = node->next;
			
			IFS_Sprintf( buf + offset, "\n\t\t\t\t<region id=\"%s\" left=\"%d%%\" top=\"%d%%\" width=\"%d%%\" height=\"%d%%\" />",
				region->id, region->left, region->top, region->width, region->height );
			offset += IFS_Strlen( buf + offset );
		}
		IFS_Strcpy( buf + offset, "\n\t\t</layout>\n\t</head>\n\t<body>" );
		offset += IFS_Strlen( buf + offset );
		
		node = pSmil->par_list.next;
		while( node != &pSmil->par_list )
		{
			par = FS_ListEntry( node, FS_SmilPar, list );
			node = node->next;

			/* must contain a media object */
			if( par->text || par->image || par->audio || par->video || par->ref )
			{
				IFS_Sprintf( buf + offset, "\n\t\t<par dur=\"%dms\">", par->dur );
				offset += IFS_Strlen( buf + offset );

				if( par->text )
				{
					IFS_Sprintf( buf + offset, "\n\t\t\t<text src=\"%s\" region=\"%s\" />", par->text->src, par->text->region );
					offset += IFS_Strlen( buf + offset );
				}
				if( par->image )
				{
					IFS_Sprintf( buf + offset, "\n\t\t\t<img src=\"%s\" region=\"%s\" />", par->image->src, par->image->region );
					offset += IFS_Strlen( buf + offset );
				}
				if( par->audio )
				{
					IFS_Sprintf( buf + offset, "\n\t\t\t<audio src=\"%s\" region=\"%s\" />", par->audio->src, par->audio->region );
					offset += IFS_Strlen( buf + offset );
				}
				if( par->video )
				{
					IFS_Sprintf( buf + offset, "\n\t\t\t<video src=\"%s\" region=\"%s\" />", par->video->src, par->video->region );
					offset += IFS_Strlen( buf + offset );
				}
				if( par->ref )
				{
					IFS_Sprintf( buf + offset, "\n\t\t\t<ref src=\"%s\" region=\"%s\" />", par->ref->src, par->ref->region );
					offset += IFS_Strlen( buf + offset );
				}
				
				IFS_Strcpy( buf + offset, "\n\t\t</par>" );
				offset += IFS_Strlen( buf + offset );
			}
		}

		IFS_Strcpy( buf + offset, "\n\t</body>\n</smil>" );
		offset += IFS_Strlen( buf + offset );

		FS_FileWrite( -1, absFile, 0, buf, offset );
		IFS_Free( buf );
	}
}

void FS_SmilAddFrameAudio( FS_MmsSmil *pSmil, FS_SINT4 frameNum, FS_CHAR *cid )
{
	FS_SmilPar *par = FS_SmilGetPar( pSmil, frameNum );

	if( par )
	{
		/* we found it */
		if( par->audio == FS_NULL )
		{
			par->audio = IFS_Malloc( sizeof(FS_SmilObject) );
			if( par->audio )
				IFS_Memset( par->audio, 0, sizeof(FS_SmilObject) );
		}

		if( par->audio )
		{
			FS_SAFE_FREE( par->audio->src );
			par->audio->src = FS_SmilFormatCid( cid );
		}
	}	
}

void FS_SmilAddFrameText( FS_MmsSmil *pSmil, FS_SINT4 frameNum, FS_CHAR *cid )
{
	FS_SmilPar *par = FS_SmilGetPar( pSmil, frameNum );

	if( par )
	{
		/* we found it */
		if( par->text == FS_NULL )
		{
			par->text = IFS_Malloc( sizeof(FS_SmilObject) );
			if( par->text )
				IFS_Memset( par->text, 0, sizeof(FS_SmilObject) );
		}

		if( par->text )
		{
			FS_SAFE_FREE( par->text->src );
			FS_SAFE_FREE( par->text->region );
			par->text->src = FS_SmilFormatCid( cid );
			par->text->region = IFS_Strdup( FS_MMS_SMIL_TEXT_REGION );
		}
	}	
}

void FS_SmilAddFrameImage( FS_MmsSmil *pSmil, FS_SINT4 frameNum, FS_CHAR *cid )
{
	FS_SmilPar *par = FS_SmilGetPar( pSmil, frameNum );
	
	if( par )
	{
		/* we found it */
		if( par->image == FS_NULL )
		{
			par->image = IFS_Malloc( sizeof(FS_SmilObject) );
			if( par->image )
				IFS_Memset( par->image, 0, sizeof(FS_SmilObject) );
		}

		if( par->image )
		{
			FS_SAFE_FREE( par->image->src );
			FS_SAFE_FREE( par->image->region );
			par->image->src = FS_SmilFormatCid( cid );
			par->image->region = IFS_Strdup( FS_MMS_SMIL_IMAGE_REGION );
		}
	}
}

void FS_SmilAddFrameVideo( FS_MmsSmil *pSmil, FS_SINT4 frameNum, FS_CHAR *cid )
{
	FS_SmilPar *par = FS_SmilGetPar( pSmil, frameNum );
	
	if( par )
	{
		/* we found it */
		if( par->video == FS_NULL )
		{
			par->video = IFS_Malloc( sizeof(FS_SmilObject) );
			if( par->video )
				IFS_Memset( par->video, 0, sizeof(FS_SmilObject) );
		}

		if( par->video )
		{
			FS_SAFE_FREE( par->video->src );
			FS_SAFE_FREE( par->video->region );
			par->video->src = FS_SmilFormatCid( cid );
			par->video->region = IFS_Strdup( FS_MMS_SMIL_IMAGE_REGION );
		}
	}
}

void FS_SmilDelFrameImage( FS_MmsSmil *pSmil, FS_SINT4 frameNum )
{
	FS_SmilPar *par = FS_SmilGetPar( pSmil, frameNum );
	if( par )
	{
		if( par->image )
		{
			FS_SmilFreeObject( par->image );
			par->image = FS_NULL;
		}
		if( par->video )
		{
			FS_SmilFreeObject( par->video );
			par->video = FS_NULL;
		}
	}
}

void FS_SmilDelFrameAudio( FS_MmsSmil *pSmil, FS_SINT4 frameNum )
{
	FS_SmilPar *par = FS_SmilGetPar( pSmil, frameNum );
	if( par )
	{
		if( par->audio )
		{
			FS_SmilFreeObject( par->audio );
			par->audio = FS_NULL;
		}
	}
}

void FS_SmilDelFrameVideo( FS_MmsSmil *pSmil, FS_SINT4 frameNum )
{
	FS_SmilPar *par = FS_SmilGetPar( pSmil, frameNum );
	if( par )
	{
		if( par->video )
		{
			FS_SmilFreeObject( par->video );
			par->video = FS_NULL;
		}
	}
}

void FS_SmilDelFrameText( FS_MmsSmil *pSmil, FS_SINT4 frameNum )
{
	FS_SmilPar *par = FS_SmilGetPar( pSmil, frameNum );
	if( par )
	{
		if( par->text )
		{
			FS_SmilFreeObject( par->text );
			par->text = FS_NULL;
		}
	}
}

FS_CHAR *FS_SmilGetImageCid( FS_MmsSmil *pSmil, FS_SINT4 frameNum )
{
	FS_SmilPar *par = FS_SmilGetPar( pSmil, frameNum );

	if( par && par->image )
		return par->image->src;
	else
		return FS_NULL;
}

FS_CHAR *FS_SmilGetVideoCid( FS_MmsSmil *pSmil, FS_SINT4 frameNum )
{
	FS_SmilPar *par = FS_SmilGetPar( pSmil, frameNum );
	
	if( par && par->video )
		return par->video->src;
	else
		return FS_NULL;
}

FS_CHAR *FS_SmilGetAudioCid( FS_MmsSmil *pSmil, FS_SINT4 frameNum )
{
	FS_SmilPar *par = FS_SmilGetPar( pSmil, frameNum );
	
	if( par && par->audio )
		return par->audio->src;
	else
		return FS_NULL;
}

FS_CHAR *FS_SmilGetTextCid( FS_MmsSmil *pSmil, FS_SINT4 frameNum )
{
	FS_SmilPar *par = FS_SmilGetPar( pSmil, frameNum );
	
	if( par && par->text )
		return par->text->src;
	else
		return FS_NULL;
}

FS_SINT4 FS_SmilGetFrameCount( FS_MmsSmil *pSmil )
{
	if( pSmil )
	{
		return FS_ListCount( &pSmil->par_list );
	}
	else
	{
		return 0;
	}
}

/* return seconds */
FS_SINT4 FS_SmilGetFrameDur( FS_MmsSmil *pSmil, FS_SINT4 frameNum )
{
	FS_SmilPar *par;
	FS_SINT4 ret = 0;

	par = FS_SmilGetPar( pSmil, frameNum );
	if( par )
	{
		ret = par->dur;
	}
	return ret / 1000;
}
/* dur is seconds */
void FS_SmilSetFrameDur( FS_MmsSmil *pSmil, FS_SINT4 frameNum, FS_SINT4 dur )
{
	FS_SmilPar *par;
	
	par = FS_SmilGetPar( pSmil, frameNum );
	if( par )
	{
		par->dur = dur * 1000;
	}
}

FS_BOOL FS_SmilLayoutImageFirst( FS_MmsSmil *pSmil )
{
	FS_BOOL ret = FS_FALSE;
	FS_SmilRegion *rTxt, *rImg;
	if( pSmil )
	{
		rTxt = FS_SmilGetRegion( pSmil, FS_MMS_SMIL_TEXT_REGION );
		rImg = FS_SmilGetRegion( pSmil, FS_MMS_SMIL_IMAGE_REGION );

		if( rTxt && rImg && rTxt->top > rImg->top )
			ret = FS_TRUE;
	}
	return ret;
}

void FS_SmilLayoutSetImageFirst( FS_MmsSmil *pSmil, FS_BOOL bImgFirst )
{
	FS_SmilRegion *rTxt, *rImg;
	rTxt = FS_SmilGetRegion( pSmil, FS_MMS_SMIL_TEXT_REGION );
	rImg = FS_SmilGetRegion( pSmil, FS_MMS_SMIL_IMAGE_REGION );

	/* see FS_CreateDefaultMmsSmil */
	if( bImgFirst )
	{
		rTxt->top = 50;
		rImg->top = 0;
	}
	else
	{
		rTxt->top = 0;
		rImg->top = 50;
	}
}

#ifdef FS_DEBUG_
void FS_SmilTest( void )
{
	FS_MmsSmil *pSmil = FS_SmilDecodeFile( "C:\\F_SOFT\\mms\\test.smil" );
	FS_SmilEncodeFile( "C:\\F_SOFT\\mms\\SmartFox.smil", pSmil );
	if( pSmil ) FS_FreeMmsSmil( pSmil );
}
#endif

#endif	//FS_MODULE_MMS

