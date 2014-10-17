#include "inc/res/FS_Res.h"
#include "inc/inte/FS_Inte.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_Charset.h"
#include "inc/gui/FS_Gui.h"
#include "inc/util/FS_MemDebug.h"

// define support language number
#define FS_LNG_NUM			2

//----------------------------------------------------------
// bitmap resource
typedef struct FS_ResBitmap_Tag
{
	FS_SINT4		id;
	const FS_BYTE *	data;
}FS_ResBitmap;
//----------------------------------------------------------
// string resource
typedef struct FS_ResString_Tag
{
	FS_SINT4		id;
	FS_CHAR *		str[FS_LNG_NUM];
}FS_ResString;
//----------------------------------------------------------
// string resource cache, for UTF-8
typedef struct FS_ResUTF8_Tag
{
	FS_SINT4		id;
	FS_CHAR *		str;
}FS_ResUTF8;

static FS_UINT4 GFS_Language = 0;	// 0: English; 1: Chinese
static FS_ResUTF8 *GFS_ResUTF8 = FS_NULL;

void FS_ResDeinit( void );

#ifdef FS_MODULE_QVGA_RES
#include "FS_ResIcon_240x320.c"
#else
#include "FS_ResIcon_176x220.c"
#endif

#include "FS_ResText.c"

//--------------------------------------------------------------
// all bitmap resource define here 
static FS_ResBitmap GFS_ResBitmap [] =
{
	{ FS_I_ACCOUNT, 			GFS_Res_Bmp_Data_EmlAccount },
	{ FS_I_ALERT, 				GFS_Res_Bmp_Data_Alert },
	{ FS_I_QUESTION, 			GFS_Res_Bmp_Data_Question},
	{ FS_I_INFO, 				GFS_Res_Bmp_Data_Info},
	{ FS_I_LOGO, 				GFS_Res_Bmp_Logo},
	{ FS_I_NEW_MSG,				GFS_Res_Bmp_NewMsg},
	{ FS_I_NEW_NTF, 			GFS_Res_Bmp_NewNtf},
	{ FS_I_PUSH_MSG, 			GFS_Res_Bmp_PushMsg},
	{ FS_I_READED_MSG,			GFS_Res_Bmp_ReadedMsg},
	{ FS_I_CHECK,				GFS_Res_Bmp_Data_Flat_Check},
	{ FS_I_UNCHECK,				GFS_Res_Bmp_Data_Flat_Uncheck},
	{ FS_I_RADIO_CHECK,			GFS_Res_Bmp_Data_Radio_Check},
	{ FS_I_RADIO_UNCHECK,			GFS_Res_Bmp_Data_Radio_Uncheck},
	{ FS_I_LEFT,				GFS_Res_Bmp_Data_Left},
	{ FS_I_RIGHT,				GFS_Res_Bmp_Data_Right},
	{ FS_I_HOME,				GFS_Res_Bmp_Data_Home},
	{ FS_I_EDIT,				GFS_Res_Bmp_Data_Edit},
	{ FS_I_COMBO,				GFS_Res_Bmp_Data_Combo},
	{ FS_I_IMAGE, 				GFS_Res_Bmp_Data_Image},
	{ FS_I_VIDEO,				GFS_Res_Bmp_Data_Video},
	{ FS_I_FILE,				GFS_Res_Bmp_Data_file},
	{ FS_I_AUDIO,				GFS_Res_Bmp_Data_audio},
	{ FS_I_DIR,				GFS_Res_Bmp_Data_dir},
	{ FS_I_FROM, 				GFS_Res_Bmp_Data_From},
	{ FS_I_TO, 				GFS_Res_Bmp_Data_To},
	{ FS_I_CC, 				GFS_Res_Bmp_Data_Cc},
	{ FS_I_BCC, 				GFS_Res_Bmp_Data_Bcc},
	{ FS_I_SUBJECT, 			GFS_Res_Bmp_Data_Subject},
	{ FS_I_FACEBOOK, 			GFS_Res_Bmp_Data_Facebook},
	{ FS_I_ISYNC, 				GFS_Res_Bmp_Data_iSync},
	{ FS_I_TWITTER, 			GFS_Res_Bmp_Data_Twitter},
	{ FS_I_SINA, 				GFS_Res_Bmp_Data_Sina},
	{ FS_I_REPLY,				GFS_Res_Bmp_Data_Reply },
	{ FS_I_UPDATES,			GFS_Res_Bmp_Data_Updates },
	{ FS_I_COMPOSE,			GFS_Res_Bmp_Data_Compose },
	{ FS_I_FRIENDS,			GFS_Res_Bmp_Data_Friends },
	{ FS_I_EML,			GFS_Res_Bmp_Data_EmlL },
	{ FS_I_RSS,			GFS_Res_Bmp_Data_RssL },
	{ FS_I_ISYNC_L,			GFS_Res_Bmp_Data_ISyncL },
	{ FS_I_BIND_L,			GFS_Res_Bmp_Data_BindL },
	{ FS_I_EXT_L,			GFS_Res_Bmp_Data_ExtL },
	{ FS_I_SET_L,			GFS_Res_Bmp_Data_SetL },
	{ FS_I_ARTICLE,			GFS_Res_Bmp_Data_Article },
	{ FS_I_CHANNEL,			GFS_Res_Bmp_Data_Channel },
	{ FS_I_BIND,			GFS_Res_Bmp_Data_Bind },
	{ FS_I_CONTACT,			GFS_Res_Bmp_Data_Contact },
	{ FS_I_EML_ACT,			GFS_Res_Bmp_Data_EmlAct },
	// the last one
	{ -1, FS_NULL }	
};

FS_Bitmap * FS_Icon( FS_SINT4 id )
{
	int i = 0;
	FS_Bitmap * ret = IFS_Malloc( sizeof(FS_Bitmap) );		// will free in FS_ReleaseIcon
	while( GFS_ResBitmap[i].id != -1 )
	{
		if( id == GFS_ResBitmap[i].id )
		{
			const FS_BYTE *data = GFS_ResBitmap[i].data;
			ret->width = FS_LE_BYTE_TO_UINT4(data);
			ret->height = FS_LE_BYTE_TO_UINT4(data + 4);
			ret->bpp = FS_LE_BYTE_TO_UINT4(data + 8);
			ret->pitch = FS_LE_BYTE_TO_UINT4(data + 12);
			ret->bits = (FS_BYTE *)(GFS_ResBitmap[i].data + sizeof(FS_Bitmap) - 4);
			return ret;
		}
		i ++;
	}
	IFS_Free( ret );
	return FS_NULL;
}

void FS_ReleaseIcon( FS_Bitmap *pBmp )
{
	if( pBmp )
	{
		IFS_Free( pBmp );
	}
}

FS_CHAR * FS_GetResString( FS_SINT4 id )
{
	FS_SINT4 i = 0;
	while( GFS_ResString[i].id != -1 )
	{
		if( id == GFS_ResString[i].id )
		{
			return GFS_ResString[i].str[GFS_Language];
		}
		i ++;
	}
	return FS_NULL;
}

FS_CHAR * FS_Text( FS_SINT4 id )
{
	FS_CHAR *str;
	FS_CHAR *utf8;
	FS_SINT4 len;
	
	if( GFS_ResUTF8 == FS_NULL ){
		/* alloc utf8 string table */
		GFS_ResUTF8 = IFS_Malloc( sizeof(FS_ResUTF8) * FS_T_MAX_COUNT );
		FS_ASSERT( GFS_ResUTF8 != FS_NULL );
		IFS_Memset( GFS_ResUTF8, 0, sizeof(FS_ResUTF8) * FS_T_MAX_COUNT );
	}

	if( GFS_ResUTF8[id - 1].str == FS_NULL ){
		/* find res string. convert it to utf8 */
		str = FS_GetResString( id );
		if( str == FS_NULL ) return FS_NULL;
		len = IFS_Strlen( str );
		utf8 = IFS_Malloc( (len * 3 / 2) + 1 );
		FS_ASSERT( utf8 != FS_NULL );
		FS_CnvtGBKToUtf8( str, len, utf8, FS_NULL );
		GFS_ResUTF8[id - 1].str = utf8;
	}
	return GFS_ResUTF8[id - 1].str;
}

void FS_SetLanguage( FS_UINT4 lan )
{
	FS_UINT4 old_lan = GFS_Language;

	if( lan > FS_LNG_NUM - 1 )
		GFS_Language = FS_LNG_NUM - 1;
	else
		GFS_Language = lan;

	if ( old_lan != GFS_Language ) {
		if ( FS_GetTopMostWindow() == FS_NULL ) {
			FS_ResDeinit( );
		}
	}
}

FS_UINT1 FS_GetLanguage( void )
{
	return GFS_Language;
}

void FS_ResDeinit( void )
{
	FS_SINT4 i;
	if( GFS_ResUTF8 ){
		for( i = 0; i < FS_T_MAX_COUNT; i ++ ){
			if( GFS_ResUTF8[i].str ){
				IFS_Free( GFS_ResUTF8[i].str );
				GFS_ResUTF8[i].str = FS_NULL;
			}
		}
		IFS_Free( GFS_ResUTF8 );
		GFS_ResUTF8 = FS_NULL;
	}
}

