#include "inc\util\FS_Mime.h"
#include "inc\inte\FS_Inte.h"
#include "inc\util\FS_File.h"
#include "inc\util\FS_Util.h"
#include "inc\util\FS_MemDebug.h"

#define FS_MAX_EXT	3

typedef struct FS_MimeName_Tag
{
	FS_CHAR *mime;
	FS_CHAR *ext[FS_MAX_EXT];	// max for 3 ext
}FS_MimeName;

typedef struct FS_BinMime_Tag
{
	FS_UINT2 code;
	FS_CHAR *mime;
}FS_BinMime;

typedef struct FS_MimeMediaName_Tag
{
	FS_CHAR *			name;
	FS_MimeMediaType	type;
}FS_MimeMediaName;

typedef struct FS_EncodeName_Tag
{
	FS_CHAR * 			name;
	FS_EncodeType		enc;
}FS_EncodeName;

static FS_EncodeName GFS_EncodeTable[] = {
	{"7bit", 				FS_ENC_7BIT},
	{"8bit", 				FS_ENC_8BIT},
	{"binary", 				FS_ENC_BINARY},
	{"quoted-printable", 	FS_ENC_QUOTED_PRINTABLE},
	{"base64", 				FS_ENC_BASE64},
	{FS_NULL, 					FS_ENC_UNKNOW},
};

static FS_MimeMediaName GFS_MimeMediaTable[] = {
	{"text", 			FS_MIME_TEXT},
	{"image", 			FS_MIME_IMAGE},
	{"audio", 			FS_MIME_AUDIO},
	{"video", 			FS_MIME_VIDEO},
	{"application", 	FS_MIME_APPLICATION},
	{"message", 		FS_MIME_MESSAGE},
	{"multipart", 		FS_MIME_MULTIPART},
	{FS_NULL, 			FS_MIME_UNKNOW},
};

static FS_MimeName GFS_MimeTable [] =
{
	// image
	{"image/bmp", 				"bmp", 		FS_NULL, 		FS_NULL	},
	{"image/gif", 				"gif",		"ifm", 			FS_NULL	},
	{"image/jpeg", 				"jpg",		"jpeg",			FS_NULL	},
	{"image/png", 				"png",		"pnz",			FS_NULL	},
	{"image/vnd.wap.wbmp",		"wbmp", 	FS_NULL, 		FS_NULL	}, 
	{"image/x-icon", 			"ico", 		FS_NULL, 		FS_NULL	}, 
	{"image/x-up-wpng", 		"wpng", 	FS_NULL, 		FS_NULL	},  
	// audio
	{"audio/basic", 			"au", 		"snd", 		FS_NULL},
	{"audio/midi", 				"mid", 		"midi", 	FS_NULL },
	{"audio/x-epac",			"pae",		FS_NULL, 	FS_NULL },    
	{"audio/x-imy",				"imy",		FS_NULL, 	FS_NULL },     
	{"audio/x-mio",				"mio",		FS_NULL, 	FS_NULL },      
	{"audio/x-mod",				"mod", 		"s3z", 		"xm"	},
	{"audio/x-mpeg", 			"mp2", 		"mp3 ", 	FS_NULL },      
	{"audio/x-mpegurl", 		"m3u",		FS_NULL, 	FS_NULL },      
	{"audio/x-ms-wax", 			"wax",		FS_NULL, 	FS_NULL },       
	{"audio/x-ms-wma" 			"wma",		FS_NULL, 	FS_NULL },        
	{"audio/x-pn-realaudio" 	"ra", 		"ram", 		"rm"	}, 
	{"audio/x-pac" 				"pac",		FS_NULL, 	FS_NULL },         
	{"audio/x-wav", 			"wav",		FS_NULL, 	FS_NULL },         
	{"audio/amr", 				"amr",		FS_NULL,	FS_NULL },		   
	{"audio/x-imy",				"imy",		FS_NULL,	FS_NULL },
	{"audio/i-melody",			"imy",		FS_NULL,	FS_NULL },	
	{"audio/e-melody",			"emy",		FS_NULL,	FS_NULL },	
	// vedio
	{"video/3gpp",				"3gp",		FS_NULL,	FS_NULL },			
	{"video/mpeg",				"mpeg", 	"mpg",		"mpe"	}, 
	{"video/msvideo",			"avi",		FS_NULL,	FS_NULL },
	{"video/quicktime", 		"qt",		"mov",		FS_NULL },
	{"video/vdo",				"vdo",		FS_NULL,	FS_NULL },
	{"video/vivo",				"viv",		"vivo", 	FS_NULL },
	{"video/x-mng", 			"mng",		FS_NULL,	FS_NULL },
	{"video/x-ms-wmv",			"wmv",		FS_NULL,	FS_NULL },
	{"video/x-ms-wmx",			"wmx",		FS_NULL,	FS_NULL },
	{"video/x-ms-wvx",			"wvx",		FS_NULL,	FS_NULL },
	{"video/mp4",				"mp4",		"mpg4", 	FS_NULL },
	{"video/x-sgi-movie",		"movie",	FS_NULL,	FS_NULL },
	// text
	{"text/css",				"css",				FS_NULL,		FS_NULL		}, 
	{"text/html", 				"htm",				"html",			FS_NULL		},  
	{"text/plain", 				"txt",				FS_NULL,		FS_NULL		}, 
	{"text/richtext", 			"rtx",				FS_NULL,		FS_NULL		},  
	{"text/vnd.sun.j2me.app-descriptor",	"jad",	FS_NULL,		FS_NULL		}, 
	{"text/vnd.wap.wml", 		"wml", 				FS_NULL,		FS_NULL		},  
	{"text/vnd.wap.wmlscript", 	"wmls", 			FS_NULL,		FS_NULL		},  
	{"text/xml", 				"xml",				"xsl",			FS_NULL		},   
	{"text/x-vCard", 			"vcf",				FS_NULL,		FS_NULL		}, 
	{"text/x-vCalendar", 		"vcs",				FS_NULL,		FS_NULL		}, 	
	
	{"message/rfc822",						"eml",		FS_NULL,		FS_NULL		}, 	
	{"application/smil",					"smil",		FS_NULL,		FS_NULL 	},	
	{"application/java-archive",			"jar",		FS_NULL,		FS_NULL 	},	
	{"application/vnd.wap.mms-message",		"mms",		FS_NULL,		FS_NULL 	},		
	{"application/vnd.oma.drm.message", 	"dm",		FS_NULL,		FS_NULL 	},		
	{"application/vnd.oma.drm.content", 	"dcf",		FS_NULL,		FS_NULL 	},		
	{"application/vnd.oma.drm.rights+xml", 	"dr",		FS_NULL,		FS_NULL 	},		
	{"application/vnd.oma.drm.rights+wbxml","drc",		FS_NULL,		FS_NULL 	},		
	// last
	{FS_NULL,					FS_NULL,	FS_NULL,		FS_NULL		}, 	
};

static FS_BinMime GFS_BinMimeTable [] = 
{
	{ 0x02, "text/html" },
	{ 0x03, "text/plain" },		
	{ 0x06, "text/x-vCalendar" },
	{ 0x07, "text/x-vCard" },
	{ 0x08, "text/vnd.wap.wml" },
	{ 0x09, "text/vnd.wap.wmlscript" },
	{ 0x28, "text/xml" },
	
	{ 0x0C, "Multipart/mixed" },	
	{ 0x0F, "multipart/alternative" },
	{ 0x14, "application/vnd.wap.wmlc" },
	{ 0x15, "application/vnd.wap.wmlscriptc" },
	{ 0x27, "application/xml" },
	{ 0x29, "application/vnd.wap.wbxml" },

	{ 0x1D, "image/gif" },
	{ 0x1E, "image/jpeg" },
	{ 0x1F, "image/tiff" },
	{ 0x20, "image/png" },
	{ 0x21, "image/vnd.wap.wbmp" },

	{ 0xFF, FS_NULL }
};

//-----------------------------------------------------------------------
// local function defined here

// return mime base on binary code( WSP 1.0 )
FS_CHAR * FS_GetMimeFromCode( FS_UINT2 code )
{
	FS_BinMime *pname;	
	for( pname = GFS_BinMimeTable; pname->mime != FS_NULL; pname ++ )
	{
		if( pname->code == code )
			return pname->mime;
	}
	// default
	return "application/octet-stream";
}

FS_UINT2 FS_GetMimeCodeFromMime( FS_CHAR *mime )
{
	FS_BinMime *pname;	
	for( pname = GFS_BinMimeTable; pname->mime != FS_NULL; pname ++ )
	{
		if( IFS_Stricmp(mime, pname->mime) == 0 )
			return pname->code;
	}
	// default
	return 0;
}

FS_CHAR * FS_GetExtFromMimeCode( FS_UINT2 code )
{
	FS_CHAR *mime;

	mime = FS_GetMimeFromCode( code );
	if( mime )
	{
		return FS_GetExtFromMime( mime );
	}
	else
	{
		return FS_NULL;
	}
}

FS_SINT4 FS_GetTypeFromMime( FS_CHAR *mime )
{
	FS_SINT4 ret = FS_MIME_UNKNOW;
	
	if( FS_STR_NI_EQUAL(mime, "image/", 6) )
		return FS_MIME_IMAGE;
	else if( FS_STR_NI_EQUAL(mime, "audio/", 6) )
		return FS_MIME_AUDIO;
	else if( FS_STR_NI_EQUAL(mime, "video/", 6) )
		return FS_MIME_VIDEO;
	
	return ret;
}

FS_SINT4 FS_GetTypeFromMimeCode( FS_UINT2 code )
{
	FS_CHAR *mime;
	FS_SINT4 ret = FS_MIME_UNKNOW;
	
	mime = FS_GetMimeFromCode( code );
	return FS_GetTypeFromMime( mime );
}

FS_UINT2 FS_GetMimeCodeFromExt( FS_CHAR *ext )
{
	FS_CHAR *mime;

	mime = FS_GetMimeFromExt( ext );
	return FS_GetMimeCodeFromMime( mime );
}

//-----------------------------------------------------------------------
// return mime base on file name ext
FS_CHAR * FS_GetMimeFromExt( FS_CHAR *filename )
{
	FS_CHAR * ext = FS_GetFileExt( filename );
	FS_MimeName *pname;	
	FS_SINT4 i;

	if( ext )
	{
		for( pname = GFS_MimeTable; pname->mime != FS_NULL; pname ++ )
		{
			for( i = 0; i < FS_MAX_EXT; i ++ )
			{
				if( pname->ext[i] )
					if( IFS_Stricmp(ext, pname->ext[i]) == 0 )
						return pname->mime;
			}
		}
	}
	// default
	return "application/octet-stream";
}
//-----------------------------------------------------------------------
// return ext base on ext
FS_CHAR * FS_GetExtFromMime( FS_CHAR *mime )
{
	FS_MimeName *pname;	
	for( pname = GFS_MimeTable; pname->mime != FS_NULL; pname ++ )
	{
		if( IFS_Stricmp(mime, pname->mime) == 0 )
			return pname->ext[0];
	}
	// default
	return FS_NULL;
}

FS_MimeMediaType FS_GetMimeMediaType( FS_CHAR *str )
{
	FS_MimeMediaType ret = FS_MIME_UNKNOW;
	FS_MimeMediaName *pType = GFS_MimeMediaTable;
	for( pType = GFS_MimeMediaTable; pType->name != FS_NULL; pType ++ )
	{
		if( 0 == IFS_Strnicmp(pType->name, str, IFS_Strlen(pType->name)) )
		{
			ret = pType->type;
			break;
		}
	}
	return ret;
}

FS_CHAR * FS_GenMimeTypeName( FS_CHAR * mime, FS_MimeMediaType type, FS_CHAR * subtype )
{
	FS_MimeMediaName *pType = GFS_MimeMediaTable;
	for( pType = GFS_MimeMediaTable; pType->name != FS_NULL; pType ++ )
	{
		if( type == pType->type )
		{
			if( subtype )
			{
				IFS_Strcpy( mime, pType->name );
				IFS_Strcat( mime, "/" );
				IFS_Strcat( mime, subtype );
				return mime;
			}
		}
	}
	IFS_Strcpy( mime, "application/octet-stream" );
	return mime;
}

FS_EncodeType FS_GetEncodeType( FS_CHAR *str )
{
	FS_EncodeType ret = FS_ENC_UNKNOW;
	FS_EncodeName *pen = GFS_EncodeTable;
	for( pen = GFS_EncodeTable; pen->name != FS_NULL; pen ++ )
	{
		if( ! IFS_Strnicmp(pen->name, str, IFS_Strlen(pen->name)) )
		{
			ret = pen->enc;
			break;
		}
	}
	return ret;
}

FS_CHAR * FS_GetParam( FS_List *params, FS_CHAR *name )
{
	FS_MimeParam *param;
	FS_List *node = params->next;
	FS_SINT4 nlen = IFS_Strlen( name );
	while( node != params )
	{
		param = FS_ListEntry( node, FS_MimeParam, list );
		if( param->name && IFS_Strnicmp(param->name, name, nlen) == 0 )
			return param->val;
		node = node->next;
	}
	return FS_NULL;
}

FS_MimeEntry * FS_NewMimeEntry( void )
{
	FS_MimeEntry *ret = IFS_Malloc( sizeof(FS_MimeEntry) );
	if( ret )
	{
		IFS_Memset( ret, 0, sizeof(FS_MimeEntry) );
		FS_ListInit( &ret->list );
		FS_ListInit( &ret->params );
	}
	return ret;
}

void FS_FreeMimeEntry( FS_MimeEntry *entry )
{
	if( entry )
	{
		FS_MimeParam *param;
		FS_List *node = entry->params.next;
		if( entry->file_name )
		{
			if( entry->temp_file )
				FS_FileDelete( FS_DIR_TMP, entry->file_name );
			IFS_Free( entry->file_name );
			entry->file_name = FS_NULL;
		}
		if( entry->disp_name )
		{
			IFS_Free( entry->disp_name );
			entry->disp_name = FS_NULL;
		}
		if( entry->subtype )
		{
			IFS_Free( entry->subtype );
			entry->subtype = FS_NULL;
		}
		while( node != &entry->params )
		{
			param = FS_ListEntry( node, FS_MimeParam, list );
			node = node->next;
			FS_ListDel( &param->list );
			if( param->name )
				IFS_Free( param->name );
			if( param->val )
				IFS_Free( param->val );
			IFS_Free( param );
		}
	}
}

/*---------------------------------MIME parser utilities---------------------------------------*/
/* remove trailing return code */
static FS_CHAR *FS_DelTrailCrlf( FS_CHAR *str )
{
	FS_CHAR *s;

	if (!*str) return str;
	s = str + IFS_Strlen(str) - 1;
	for ( ; s >= str && (*s == '\n' || *s == '\r'); s-- )
		*s = '\0';

	return str;
}

FS_SINT4 FS_GetOneField( FS_CHAR *buf, FS_SINT4 len, FS_CHAR ** data, FS_HeadField *hentry )
{
	FS_SINT4 nexthead;
	FS_SINT4 hnum = 0;
	FS_SINT4 ret = -1, i, buflen;

	/* skip non-required headers */
	do {
		do {
			if( FS_GetLine( buf, len, data ) == FS_NULL )
				return -1;
			if( buf[0] == '\r' || buf[0] == '\n' )
				return -1;	
		} while( buf[0] == ' ' || buf[0] == '\t' );

		for ( i = 0; hentry[i].name != FS_NULL; i ++ )
		{
			if ( ! IFS_Strnicmp( hentry[i].name, buf, IFS_Strlen( hentry[i].name )))
			{
				ret = i;
				break;
			}
		}
	} while ( ret == -1 );

	/* unfold line */
	while( ret != -1 )
	{
		nexthead = **data;
		/* ([*WSP CRLF] 1*WSP) */
		if (nexthead == ' ' || nexthead == '\t')
		{
			FS_DelTrailCrlf( buf );

			buflen = IFS_Strlen( buf );
			
			/* concatenate next line */
			if ( (len - buflen) > 2 )
			{
				if( FS_GetLine( buf + buflen, len - buflen, data ) == FS_NULL)
					break;
			}
			else
			{
				break;
			}
		}
		else
		{
			/* remove trailing new line */
			FS_DelTrailCrlf( buf );
			break;
		}
	}

	return ret;
}

void FS_ProcessContentType( FS_MimeEntry *entry, FS_CHAR * str )
{
	FS_CHAR *p;
	FS_SINT4 len;
	entry->type = FS_GetMimeMediaType( str );
	str = IFS_Strchr( str, '/' );
	p = IFS_Strchr( str, ';' );
	if( p )
		len = p - str - 1;
	else
		len = IFS_Strlen( str + 1);
	entry->subtype = FS_Strndup( str + 1, len );
	if( p )
		FS_ProcessParams( &entry->params, p + 1 );
}

void FS_ProcessParams( FS_List *paramList, FS_CHAR * str )
{
	FS_CHAR *pval, *pname;
	FS_MimeParam *params;
	pname = str;
	pval = IFS_Strchr( str, '=' );
	while( pval )
	{
		params = IFS_Malloc( sizeof(FS_MimeParam) );
		if( params )
		{
			while( *pname == ' ' || *pname == '\t' ) pname ++;
			params->name = FS_Strndup( pname, pval - pname );
			pval ++;
			while( *pval == ' ' || *pval == '\t' ) pval ++;
			if( *pval == '"' )
			{
				FS_ExtractQuote( pval, '"' );
				pname = pval + IFS_Strlen( pval ) + 1;
				pname = IFS_Strchr( pname, ';' );
			}
			else
				pname = IFS_Strchr( pval, ';' );
			if( pname ) // another params
			{
				params->val = FS_Strndup( pval, pname - pval );
				pname ++;
				pval = IFS_Strchr( pname, '=' );
			}
			else	// no other params
			{
				params->val = IFS_Strdup( pval );
				pval = FS_NULL;
			}
			FS_InsertParams( paramList, params );
		}
	}
}

void FS_InsertParams( FS_List *paramList, FS_MimeParam *params )
{
	FS_List *node = paramList->next;
	FS_MimeParam *pa;
	while( node != paramList )
	{
		pa = FS_ListEntry( node, FS_MimeParam, list );
		// duplicate check
		if( pa->name && params->name && (IFS_Strcmp(pa->name, params->name) == 0) )
		{
			IFS_Free( params->name );
			if( params->val )
				IFS_Free( params->val );
			IFS_Free( params );
			return;
		}
		node = node->next;
	}
	FS_ListAdd( paramList, &params->list );
}


