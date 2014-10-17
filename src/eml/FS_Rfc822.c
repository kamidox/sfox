#include "inc/FS_Config.h"

#ifdef FS_MODULE_EML

#include "inc\eml\FS_Rfc822.h"
#include "inc\inte\FS_Inte.h"
#include "inc\util\FS_Base64.h"
#include "inc\util\FS_Mime.h"
#include "inc\util\FS_MemDebug.h"

//-----------------------------------------------------------------------------
// local function declare
static void FS_ProcessMimeEntry( FS_MimeEntry *entry );

static void FS_AddParams( FS_MimeEntry *entry, FS_CHAR *name, FS_CHAR *value )
{
	FS_MimeParam *param = IFS_Malloc( sizeof(FS_MimeParam) );
	if( param )
	{
		param->name = IFS_Strdup( name );
		param->val = IFS_Strdup( value );
		FS_InsertParams( &entry->params, param );
	}
}

static void FS_EmlParseDate( FS_DateTime *date, FS_CHAR *str )
{
	FS_DateStr2Struct( date, str );
}

static void FS_EmlParseAddr( FS_EmlAddr *out, FS_CHAR *str )
{
	FS_CHAR *addr, *p = str;

	IFS_Memset( out, 0, sizeof(FS_EmlAddr) );
	FS_ListInit( &out->list );
	if( *p == '"' )
	{
		FS_ExtractQuote( p, '"' );
		IFS_Strncpy( out->name, p, FS_EML_NAME_LEN - 1 );
		p += IFS_Strlen( p ) + 1;
		p = IFS_Strchr( p, '<' );
		if( p ) p ++;
	}
	else
	{
		addr = p;
		p = IFS_Strchr( p, '<' );
		if( p )
		{
			*p ++ = '\0';
			IFS_Strncpy( out->name, addr, FS_EML_NAME_LEN - 1 );
		}
	}

	if( p )	
	{
		addr = IFS_Strchr( p, '>' );
		if( addr )
			*addr = '\0';
		IFS_Strncpy( out->addr, p, FS_EML_ADDR_LEN - 1 );
	}
	else	/* treat hold as email address */
	{
		IFS_Strncpy( out->addr, str, FS_EML_ADDR_LEN - 1 );
	}
}

static void FS_ProcessContentDisposition( FS_MimeEntry *entry, FS_CHAR * str )
{
	if( ! IFS_Strnicmp( str, "attachment", 10 ) )
		entry->disposition = FS_DPS_ATTACHMENT;
	else if( ! IFS_Strnicmp( str, "inline", 6 ) )
		entry->disposition = FS_DPS_INLINE;
	
	str = IFS_Strchr( str, ';' );
	if( str )
		FS_ProcessParams( &entry->params, str + 1 );
}

/*
	parse email head
	emlHead 		out 		parse result place here
	data			in			input char buffer

	return	FS_TRUE if header finished, of FS_FALSE
*/
void FS_GetHeaderFields( FS_HeadField *hentry, FS_CHAR ** data )
{
	FS_CHAR * buf;
	FS_HeadField *hp;
	FS_SINT4 hnum;
	FS_CHAR *p;

	buf = IFS_Malloc( FS_MIME_HEAD_FIELD_MAX_LEN );
	if( buf )
	{
		while ((hnum = FS_GetOneField(buf, FS_MIME_HEAD_FIELD_MAX_LEN, data, hentry) ) != -1) {
			hp = hentry + hnum;
		
			p = buf + IFS_Strlen( hp->name );
			while (*p == ' ' || *p == '\t') p++;
		
			if (hp->value == FS_NULL)
				hp->value = IFS_Strdup( p );
			else if ( IFS_Strnicmp(hp->name, "To", 2) == 0 ||
				 IFS_Strnicmp(hp->name, "Cc", 2) == 0)
			{
				FS_CHAR *tp = hp->value;
				hp->value = FS_StrConCat( tp, ", ", p, FS_NULL );
				IFS_Free( tp );
			}
		}
		IFS_Free( buf );
	}
}

void FS_EmlParseAddrList( FS_List *head, FS_CHAR *str )
{
	FS_CHAR *val, *p, *pStr = FS_NULL;
	FS_EmlAddr *addr;
	
	if( str && IFS_Strlen(str) > 0 )
		pStr = IFS_Strdup( str );
	
	if( pStr )
	{
		p = pStr;
		while( p )
		{
			while( *p == ' ' || *p == '\t' ) p ++;
			val = p;
			p = IFS_Strchr( p, ',' );
			if( p )
			{
				*p = '\0';
				p = p ++;
			}
			addr = IFS_Malloc( sizeof(FS_EmlAddr) );
			if( addr )
			{
				FS_EmlParseAddr( addr, val );
				FS_ListAdd( head, &addr->list );
			}
		}
		IFS_Free( pStr );
	}
}

/*
	parse email head
	emlHead 		out 		parse result place here
	data			in			input char buffer
*/
FS_BOOL FS_EmlParseTop( FS_EmlHead *emlHead, FS_CHAR ** data )
{
	FS_SINT4 hnum = 0;
	FS_CHAR * buf, *hp, *val;
	
	typedef enum FS_FS_EmlHeadField_Tag
	{
		FS_H_DATE = 0,
		FS_H_FROM,
		FS_H_SUBJECT,
		FS_H_X_PRIORITY,
	}FS_EmlHeadField;
	
	FS_HeadField hentry[] = 
	{
		{ "Date:",						FS_NULL },
		{ "From:",						FS_NULL },
		{ "Subject:",					FS_NULL },
		{ "X-Priority:",				FS_NULL },
		// last item
		{ FS_NULL,						FS_NULL }
	};
	
	buf = IFS_Malloc( FS_MIME_HEAD_FIELD_MAX_LEN );
	if( buf )
	{
		val = IFS_Malloc( FS_MIME_HEAD_FIELD_MAX_LEN );
		if( val )
		{
			while(( hnum = FS_GetOneField( buf, FS_MIME_HEAD_FIELD_MAX_LEN, data, hentry )) != -1 )
			{
				hp = buf + IFS_Strlen( hentry[hnum].name );
				while (*hp == ' ' || *hp == '\t')
					hp++;
	
				switch (hnum)
				{
				case FS_H_DATE:
					FS_DateStr2Struct( &emlHead->date, hp );
					break;
				case FS_H_FROM:
					FS_Rfc2047DecodeString( val, hp );
					FS_EmlParseAddr( &emlHead->from, val );
					break;
				case FS_H_SUBJECT:
					FS_Rfc2047DecodeString( val, hp );
					if( val[0] != '\0' )
						emlHead->subject = IFS_Strdup( val );
					break;
				default:
					break;
				}
			}
			IFS_Free( val );
		}
		IFS_Free( buf );
	}
	return FS_TRUE;
}

/*
	parse email head
	emlHead			out			parse result place here
	data			in			input char buffer
*/
FS_BOOL FS_EmlParseHead( FS_EmlFile *emlFile, FS_CHAR ** data )
{
	FS_SINT4 hnum = 0;
	FS_CHAR * buf, *hp, *val;

	typedef enum FS_EmlHeadField_Tag
	{
		FS_H_DATE = 0,
		FS_H_FROM,
		FS_H_TO,
		FS_H_CC,
		FS_H_BCC,
		FS_H_SUBJECT,
		FS_H_CONTENT_TYPE,
		FS_H_CONTENT_TRANSFER_ENCODING,
		FS_H_X_PRIORITY,
	}FS_EmlHeadField;
	
	FS_HeadField hentry[] = 
	{
		{ "Date:",						FS_NULL },
		{ "From:",						FS_NULL },
		{ "To:",						FS_NULL },
		{ "Cc:",						FS_NULL },
		{ "Bcc:",						FS_NULL },
		{ "Subject:",					FS_NULL },
		{ "Content-Type:",				FS_NULL },
		{ "Content-Transfer-Encoding:", FS_NULL },
		{ "X-Priority:",				FS_NULL },
		// last item
		{ FS_NULL,						FS_NULL }
	};

	buf = IFS_Malloc( FS_MIME_HEAD_FIELD_MAX_LEN );
	if( buf )
	{
		val = IFS_Malloc( FS_MIME_HEAD_FIELD_MAX_LEN );
		if( val )
		{
			while(( hnum = FS_GetOneField( buf, FS_MIME_HEAD_FIELD_MAX_LEN, data, hentry )) != -1 )
			{
				hp = buf + IFS_Strlen( hentry[hnum].name );
				while (*hp == ' ' || *hp == '\t')
					hp++;

				switch (hnum)
				{
				case FS_H_DATE:
					FS_DateStr2Struct( &emlFile->date, hp );
					break;
				case FS_H_FROM:
					FS_Rfc2047DecodeString( val, hp );
					FS_EmlParseAddr( &emlFile->from, val );
					break;
				case FS_H_TO:
					FS_Rfc2047DecodeString( val, hp );
					FS_EmlParseAddrList( &emlFile->to, val );
					break;
				case FS_H_CC:
					FS_Rfc2047DecodeString( val, hp );
					FS_EmlParseAddrList( &emlFile->cc, val );
					break;
				case FS_H_BCC:
					FS_Rfc2047DecodeString( val, hp );
					FS_EmlParseAddrList( &emlFile->bcc, val );
					break;
				case FS_H_SUBJECT:
					if( ! emlFile->subject )
					{
						FS_Rfc2047DecodeString( val, hp );
						emlFile->subject = IFS_Strdup( val );
					}
					break;
				case FS_H_CONTENT_TYPE:
					if( emlFile->body.type == FS_MIME_UNKNOW )
					{
						FS_Rfc2047DecodeString( val, hp );
						FS_ProcessContentType( &emlFile->body, val );
					}
					break;
				case FS_H_CONTENT_TRANSFER_ENCODING:
					emlFile->body.encode = FS_GetEncodeType( hp );
					break;
				default:
					break;
				}
			}
			IFS_Free( val );
		}
		IFS_Free( buf );
	}
	return FS_TRUE;
}

static void FS_ProcessMimeMultipart( FS_MimeEntry *entry )
{
	FS_CHAR * bndry = FS_GetParam( &entry->params, "boundary" );
	FS_CHAR *buf, *data, *inp;
	FS_SINT4 len, offset, bndrylen;
	FS_MimeEntry *new_entry = FS_NULL;
	FS_BOOL bFinish = FS_FALSE;
	FS_HeadField hentry[] = 
	{
		{ "Content-Type:",				FS_NULL },
		{ "Content-Transfer-Encoding:", FS_NULL },
		{ "Content-Disposition:", 		FS_NULL },		
		// last item
		{ FS_NULL,						FS_NULL }
	};
	data = IFS_Malloc( FS_EML_HEAD_MAX_LEN );
	if( data )
	{
		bndrylen = IFS_Strlen( bndry );
		buf = IFS_Malloc( FS_MIME_HEAD_FIELD_MAX_LEN );
		if( buf )
		{
			offset = entry->offset;
			while((len = FS_FileReadXLine( entry->file_name, offset, data, FS_EML_HEAD_MAX_LEN )) > 0 )
			{
				inp = data;
				while( inp - data < len )
				{
					FS_GetLine( buf, FS_MIME_HEAD_FIELD_MAX_LEN, &inp );
					if( FS_IS_BOUNDARY(buf, bndry, bndrylen) )
					{
						// end boundary
						if( buf[2 + bndrylen] == '-' && buf[2 + bndrylen + 1] == '-' )
						{
							bFinish = FS_TRUE;
							break;
						}
						new_entry = FS_NewMimeEntry( );
						FS_GetHeaderFields( hentry, &inp );
						if( hentry[0].value )
						{
							FS_Rfc2047DecodeString( buf, hentry[0].value );
							IFS_Free( hentry[0].value );
							hentry[0].value = FS_NULL;
							FS_ProcessContentType( new_entry, buf );
						}
						if( hentry[1].value )
						{
							new_entry->encode = FS_GetEncodeType( hentry[1].value );
							IFS_Free( hentry[1].value );
							hentry[1].value = FS_NULL;
						}
						if( hentry[2].value )
						{
							FS_Rfc2047DecodeString( buf, hentry[2].value );
							IFS_Free( hentry[2].value );
							hentry[2].value = FS_NULL;
							FS_ProcessContentDisposition( new_entry, buf );
						}
						new_entry->file_name = IFS_Strdup( entry->file_name );
						new_entry->offset = offset + inp - data;
						FS_AddParams( new_entry, "boundary", bndry );
						FS_ListAdd( &entry->list, &new_entry->list );
						FS_ProcessMimeEntry( new_entry );
					}
				}
				if( bFinish )
					break;
				offset += len;
			}
			IFS_Free( buf );
		}
		IFS_Free( data );
	}
}
//------------------------------------------------------------------------------
// process mime single part, create a temp file and save data to it
void FS_ProcessMimePart( FS_MimeEntry *entry )
{
	FS_CHAR tmp_file[FS_FILE_NAME_LEN], dname[FS_FILE_NAME_LEN];
	FS_CHAR *ext, *file;
	FS_SINT4 rlen, dlen, soff, doff;
	FS_CHAR *buf, *decbuf, *cend, *bndry, *charset, *str = FS_NULL;
	FS_BOOL bend = FS_FALSE;
	
	/* check the content type, if is not exist, set to default */
	if( entry->type == FS_MIME_UNKNOW && entry->subtype == FS_NULL )
	{
		entry->type = FS_MIME_TEXT;
		entry->subtype = IFS_Strdup( "plain" );
	}
	/* get charset if exist */
	charset = FS_GetParam( &entry->params, "charset" );
	/* get file name if exist, or generate a file name */
	file = FS_GetParam( &entry->params, "filename" );
	if( file == FS_NULL )
		file = FS_GetParam( &entry->params, "name" );
	if( file == FS_NULL )
	{
		FS_CHAR mime[FS_MIME_TYPE_NAME_LEN];
		file = FS_GetGuid( dname );
		FS_GenMimeTypeName( mime, entry->type, entry->subtype );
		ext = FS_GetExtFromMime( mime );
		if( ext )
		{
			IFS_Strcat( dname, "." );
			IFS_Strcat( dname, ext );
		}
	}

	entry->disp_name = IFS_Strdup( file );
	FS_GetLuid( tmp_file );
	ext = FS_GetFileExt( file );
	if( ext )
	{
		IFS_Strcat( tmp_file, "." );
		IFS_Strcat( tmp_file, ext );
	}
	buf = IFS_Malloc( FS_FILE_BLOCK + 1 );
	if( buf )
	{
		IFS_Memset( buf, 0, FS_FILE_BLOCK + 1 );
		decbuf = IFS_Malloc( FS_FILE_BLOCK );
		if( decbuf )
		{
			void *dec = FS_NULL;
			if( entry->encode == FS_ENC_BASE64 )
				dec = FS_B64NewDecoder( );
			soff = entry->offset;
			doff = 0;
			bndry = FS_GetParam( &entry->params, "boundary" );
			while((rlen = FS_FileReadXLine(entry->file_name, soff, buf, FS_FILE_BLOCK)) > 0)
			{
				if( rlen < FS_FILE_BLOCK )
					IFS_Memset( buf + rlen, 0, FS_FILE_BLOCK - rlen );
				if( bndry )
				{	// reach to end boundary
					cend = IFS_Strstr( buf, bndry );
					if( cend )
					{
						bend = FS_TRUE;
						IFS_Memset( cend, 0, FS_FILE_BLOCK - (cend - buf) );
					}
				}
				if( entry->encode == FS_ENC_BASE64 )
				{
					dlen = FS_B64DecoderDecode( dec, buf, (FS_BYTE *)decbuf );
					if( charset && entry->type == FS_MIME_TEXT )
						str = FS_ProcessCharset( decbuf, dlen, charset, &dlen );
					if( str )
					{
						FS_FileWrite( FS_DIR_TMP, tmp_file, doff, str, dlen );
						IFS_Free( str );
						str = FS_NULL;
					}
					else
					{
						FS_FileWrite( FS_DIR_TMP, tmp_file, doff, decbuf, dlen );
					}
				}
				else if( entry->encode == FS_ENC_QUOTED_PRINTABLE )
				{
					dlen = FS_QPDecode( (FS_BYTE *)decbuf, buf, rlen );
					if( charset && entry->type == FS_MIME_TEXT )
						str = FS_ProcessCharset( decbuf, dlen, charset, &dlen );
					if( str )
					{
						FS_FileWrite( FS_DIR_TMP, tmp_file, doff, str, dlen );
						IFS_Free( str );
						str = FS_NULL;
					}
					else
					{
						FS_FileWrite( FS_DIR_TMP, tmp_file, doff, decbuf, dlen );
					}
				}
				else	// write data directly
				{
					if( charset && entry->type == FS_MIME_TEXT )
						str = FS_ProcessCharset( buf, rlen, charset, &dlen );
					if( str )
					{
						FS_FileWrite( FS_DIR_TMP, tmp_file, doff, str, dlen );
						IFS_Free( str );
						str = FS_NULL;
					}
					else
					{
						FS_FileWrite( FS_DIR_TMP, tmp_file, doff, buf, rlen );
						dlen = rlen;
					}
				}
				if( bend )
					break;
				doff += dlen;
				soff += rlen;
			}
			if( dec )
				FS_B64FreeDecoder( dec );
			IFS_Free( decbuf );

			// remenber the temp file name
			if( entry->file_name )
				IFS_Free( entry->file_name );
			entry->file_name = IFS_Strdup( tmp_file );
			entry->temp_file = FS_TRUE;
			entry->offset = 0;
			entry->length = FS_FileGetSize( FS_DIR_TMP, tmp_file );
		}
		IFS_Free( buf );
	}
}

//------------------------------------------------------------------------------
// process mime entry
static void FS_ProcessMimeEntry( FS_MimeEntry *entry )
{
	if( FS_MIME_MULTIPART == entry->type )
		FS_ProcessMimeMultipart( entry );
	else
		FS_ProcessMimePart( entry );	// TODO here, we can use lazy copy
}

void FS_EmlParseFile( FS_EmlFile *emlFile, FS_CHAR *file )
{
	FS_Handle hFile;
	FS_CHAR *buf, *str;
	FS_SINT4 len, size, offset = 0;
	FS_BOOL headok = FS_FALSE;
	FS_MimeEntry *entry;
	FS_List *node;
	FS_MimeParam *param;
	
	FS_InitEmlFile( emlFile );
	if( IFS_FileOpen( &hFile, file, FS_OPEN_READ ) )
	{
		buf = IFS_Malloc( FS_EML_HEAD_MAX_LEN );
		if( buf )
		{
			IFS_Memset( buf, 0, FS_EML_HEAD_MAX_LEN );
			size = IFS_FileGetSize( hFile );
			len = size > (FS_EML_HEAD_MAX_LEN - 1) ? (FS_EML_HEAD_MAX_LEN - 1) : size;
			str = buf;
			IFS_FileRead( hFile, str, len );
			FS_EmlParseHead( emlFile, &str );
			offset = str - buf;
			IFS_Free( buf );
		}
		IFS_FileClose( hFile );
		// here we begin to process email body
		emlFile->body.offset = offset;
		emlFile->body.file_name = IFS_Strdup( file );
		if( emlFile->body.type == FS_MIME_MULTIPART )
			FS_ProcessMimeEntry( &emlFile->body );
		else
		{
			entry = FS_NewMimeEntry( );
			entry->file_name = IFS_Strdup( file );
			entry->offset = offset;
			entry->type = emlFile->body.type;
			if( emlFile->body.subtype )
				entry->subtype = IFS_Strdup( emlFile->body.subtype );
			entry->encode = emlFile->body.encode;
			node = emlFile->body.params.next;
			while( node != &emlFile->body.params )
			{
				param = FS_ListEntry( node, FS_MimeParam, list );
				node = node->next;

				FS_ListDel( &param->list );
				FS_ListAdd( &entry->params, &param->list );
			}
			FS_ListAdd( &emlFile->body.list, &entry->list );
			FS_ProcessMimePart( entry );
		}

		/* set email text */
		node = emlFile->body.list.next;
		while( node != &emlFile->body.list )
		{
			entry = FS_ListEntry( node, FS_MimeEntry, list );
			node = node->next;
			if( entry->type == FS_MIME_TEXT && FS_STR_I_EQUAL(entry->subtype, "plain" ) 
				&& entry->disposition != FS_DPS_ATTACHMENT && emlFile->text == FS_NULL )
			{
				buf = IFS_Malloc( FS_EML_TEXT_MAX_LEN );
				if( buf )
				{
					len = FS_FileRead(FS_DIR_TMP, entry->file_name, 0, buf, FS_EML_TEXT_MAX_LEN - 1);
					buf[len] = '\0';
					/* here, malloc memory of buf. not to use SetText to avoid memory realloc */
					emlFile->text = buf;

					/* delete this mime entry */
					FS_ListDel( &entry->list );
					FS_FreeMimeEntry( entry );
					IFS_Free( entry );
					return;
				}
			}
		}
	}
}

/*
	@param	[out]	rcpt, parse result add to here.
	@param	[in]	file, eml file to parse

	NOTE:	this function will alloc memory for rcpts, parse TO, CC, BCC fields
*/
void FS_EmlParseRcpt( FS_List *rcpt, FS_CHAR *file )
{
	FS_CHAR *buf, *p;
	FS_SINT4 rlen;
	FS_EmlFile emlFile;
	buf = IFS_Malloc( FS_EML_HEAD_MAX_LEN );
	if( buf )
	{
		FS_InitEmlFile( &emlFile );
		rlen = FS_FileRead( FS_DIR_EML, file, 0, buf, FS_EML_HEAD_MAX_LEN - 1 );
		if( rlen > 0 )
		{
			p = buf;
			FS_EmlParseHead( &emlFile, &p );
		}
		IFS_Free( buf );
		if( ! FS_ListIsEmpty( &emlFile.to ) )
			FS_ListCon( rcpt, &emlFile.to );
		if( ! FS_ListIsEmpty( &emlFile.cc ) )
			FS_ListCon( rcpt, &emlFile.cc );
		if( ! FS_ListIsEmpty( &emlFile.bcc) )
			FS_ListCon( rcpt, &emlFile.bcc );
		/* avoid FS_FreeEmlFile to free the rcpts */	
		FS_ListInit( &emlFile.to );	
		FS_ListInit( &emlFile.cc );
		FS_ListInit( &emlFile.bcc );

		FS_FreeEmlFile( &emlFile );
	}
}

/*---------------------------------------------------------------------*/
/*--------------------rfc822 message encode----------------------------*/

static FS_SINT4 FS_GenDate( FS_CHAR *buf )
{
	FS_DateTime dt;
	
	IFS_GetDateTime( &dt );
	FS_DateTimeFormatText( buf, &dt );
	return IFS_Strlen( buf );
}

static FS_SINT4 FS_GenBoundary( FS_CHAR *boundary )
{
	FS_SINT4 rlen;
	IFS_Strcpy( boundary, "==F_SOFT_EML_CLIENT==" );
	rlen = IFS_Strlen( boundary );
	FS_GetLuid( boundary + rlen );
	IFS_Strcat( boundary, "==" );
	return IFS_Strlen( boundary );
}

static FS_SINT4 FS_EncodeAddrList( FS_CHAR *out, FS_List *rcpt )
{
	FS_EmlAddr *addr;
	FS_List *node = rcpt->next;
	FS_CHAR *buf = out;
	while( node != rcpt )
	{
		addr = FS_ListEntry( node, FS_EmlAddr, list );
		node = node->next;
		if( IFS_Strlen(addr->name) )
		{	
			buf += FS_Rfc2047EncodeString( buf, addr->name, FS_NULL );
			*buf ++ = ' ';
		}
		*buf ++ = '<';
		IFS_Strcat( buf, addr->addr );
		buf += IFS_Strlen( addr->addr );
		*buf ++ = '>';

		if( node != rcpt )
		{
			IFS_Strcat( buf, ",\r\n\t" );
			buf += 4;
		}
	}
	return (FS_SINT4)(buf - out);
}

static FS_SINT4 FS_EncodeHead( FS_EmlFile *emlFile, FS_CHAR *pBuf, FS_SINT4 len, FS_CHAR *boundary )
{	
	FS_SINT4 offset = 0;
	FS_CHAR *buf = pBuf;
	// first header field
	IFS_Strcpy( buf, "MIME-Version: 1.0\r\n" );
	offset += IFS_Strlen( buf + offset );
	if( FS_ListIsEmpty(&emlFile->body.list) )
	{
		// email text encode in base64 any way
		// content-type text/plain
		IFS_Strcat( buf + offset, "Content-Type: text/plain; charset=\"UTF-8\"\r\n" );
		offset += IFS_Strlen( buf + offset );
		// Content-transfer-encoding
		IFS_Strcat( buf + offset, "Content-Transfer-Encoding: base64\r\n" );
		offset += IFS_Strlen( buf + offset );
	}
	else
	{
		// content-type multipart/mixed
		IFS_Strcat( buf + offset, "Content-Type: multipart/mixed;\r\n\t" );
		offset += IFS_Strlen( buf + offset );
		// boundary
		FS_GenBoundary( boundary );
		IFS_Strcat( buf + offset, "boundary=\"" );
		IFS_Strcat( buf + offset, boundary );
		IFS_Strcat( buf + offset, "\"\r\n" );
		offset += IFS_Strlen( buf + offset );
	}
	// date
	IFS_Strcat( buf + offset, "Date: " );
	offset += IFS_Strlen( buf + offset );
	FS_GenDate( buf + offset );
	IFS_Strcat( buf + offset, "\r\n" );
	offset += IFS_Strlen( buf + offset );
	// from
	IFS_Strcat( buf + offset, "From: " );
	offset += IFS_Strlen( buf + offset );
	if( IFS_Strlen(emlFile->from.name) > 0 )
	{	
		buf[offset ++]  = '"';
		offset += FS_Rfc2047EncodeString( buf + offset, emlFile->from.name, FS_NULL );
		buf[offset ++]  = '"';
		buf[offset ++]  = ' ';
	}
	buf[offset ++] = '<';
	IFS_Strcat( buf + offset, emlFile->from.addr );
	offset += IFS_Strlen( buf + offset );
	buf[offset ++] = '>';
	IFS_Strcat( buf + offset, "\r\n" );
	offset += IFS_Strlen( buf + offset );
	// to
	if( ! FS_ListIsEmpty(&emlFile->to) )
	{
		IFS_Strcat( buf + offset, "To: " );
		offset += IFS_Strlen( buf + offset );
		FS_EncodeAddrList( buf + offset, &emlFile->to );
		IFS_Strcat( buf + offset, "\r\n" );
		offset += IFS_Strlen( buf + offset );
	}
	// cc
	if( ! FS_ListIsEmpty(&emlFile->cc) )
	{
		IFS_Strcat( buf + offset, "Cc: " );
		offset += IFS_Strlen( buf + offset );
		FS_EncodeAddrList( buf + offset, &emlFile->cc );
		IFS_Strcat( buf + offset, "\r\n" );
		offset += IFS_Strlen( buf + offset );
	}
	// bcc
	if( ! FS_ListIsEmpty(&emlFile->bcc) )
	{
		IFS_Strcat( buf + offset, "Bcc: " );
		offset += IFS_Strlen( buf + offset );
		FS_EncodeAddrList( buf + offset, &emlFile->bcc );
		IFS_Strcat( buf + offset, "\r\n" );
		offset += IFS_Strlen( buf + offset );
	}
	// subject
	if( emlFile->subject )
	{
		IFS_Strcat( buf + offset, "Subject: " );
		offset += IFS_Strlen( buf + offset );
		FS_Rfc2047EncodeString( buf + offset, emlFile->subject, FS_NULL );
		IFS_Strcat( buf + offset, "\r\n" );
		offset += IFS_Strlen( buf + offset );
	}
	IFS_Strcat( buf+ offset, "\r\n" );
	offset += IFS_Strlen( buf + offset );
	
	return offset;
}

static FS_SINT4 FS_EncodeText( FS_CHAR *text, FS_CHAR *bndry, FS_CHAR *pBuf, FS_SINT4 len )
{
	FS_SINT4 ret = 0;
	FS_CHAR *buf = pBuf;
	// start boundary
	IFS_Strcpy( buf, "--" );
	IFS_Strcat( buf, bndry );
	IFS_Strcat( buf, "\r\n" );
	// content-type
	IFS_Strcat( buf, "Content-Type: text/plain; charset=\"UTF-8\"\r\n" );
	IFS_Strcat( buf, "Content-Transfer-Encoding: base64\r\n\r\n" );
	ret = IFS_Strlen( buf );
	// text
	ret += FS_Base64Encode( buf + ret, (FS_BYTE *)text, -1 );
	return ret;
}

static FS_SINT4 FS_EncodeAttachFile( FS_MimeEntry *attach, FS_CHAR *bndry, FS_CHAR * dstFile, FS_SINT4 d_off )
{
	FS_SINT4 ret = 0;
	FS_CHAR *buf;
	FS_CHAR attFileName[FS_MAX_PATH_LEN];
	FS_CHAR emlFileName[FS_MAX_PATH_LEN];
	
	if( attach->temp_file ){
		FS_GetAbsFileName( FS_DIR_TMP, attach->file_name, attFileName );
	}else{
		IFS_Memset( attFileName, 0, sizeof(attFileName) );
		IFS_Strncpy( attFileName, attach->file_name, sizeof(attFileName) - 1 );
	}
	FS_GetAbsFileName( FS_DIR_EML, dstFile, emlFileName );
	
	buf = IFS_Malloc( FS_EML_HEAD_MAX_LEN );

	FS_ASSERT( buf != FS_NULL );
	if( buf == FS_NULL ) return 0;
	
	IFS_Memset( buf, 0, FS_EML_HEAD_MAX_LEN );
	// start boundary
	IFS_Strcpy( buf + ret, "--" );
	IFS_Strcat( buf + ret, bndry );
	IFS_Strcat( buf + ret, "\r\n" );
	ret += IFS_Strlen( buf + ret );
	// content-type
	IFS_Strcat( buf + ret, "Content-Type: " );
	IFS_Strcat( buf + ret, FS_GetMimeFromExt(attach->disp_name) );
	IFS_Strcat( buf + ret, "; name=\"" );
	ret += IFS_Strlen( buf + ret );
	FS_Rfc2047EncodeString( buf + ret, attach->disp_name, FS_NULL );
	IFS_Strcat( buf + ret, "\"\r\n" );
	IFS_Strcat( buf + ret, "Content-Transfer-Encoding: base64\r\n" );
	ret += IFS_Strlen( buf + ret );
	// Content-Disposition
	IFS_Strcat( buf + ret, "Content-Disposition: attachment; " );
	IFS_Strcat( buf + ret, "filename=\"" );
	ret += IFS_Strlen( buf + ret );
	FS_Rfc2047EncodeString( buf + ret, attach->disp_name, FS_NULL );
	IFS_Strcat( buf + ret, "\"\r\n\r\n" );
	ret += IFS_Strlen( buf + ret );
	// write head info to file	
	FS_FileWrite( FS_DIR_EML, dstFile, d_off, buf, ret );
	d_off += ret;
	IFS_Free( buf );
	
	// encode the file, write it to dst file directly
	ret += FS_Base64EncodeFile( attFileName, 0, emlFileName, d_off );

	return ret;
}

/*
	encode the email data, save to the disk, return the file name of the email
	@param [in]		emlFile		email file data
	@param [in/out]	file		email file name
	if file is not empty, then it will overwrite the email file
*/
void FS_PackRfc822Message( FS_EmlFile *emlFile, FS_CHAR *file )
{
	FS_CHAR filename[FS_MAX_PATH_LEN], dstFile[FS_FILE_NAME_LEN];
	FS_BYTE *buf;
	FS_SINT4 len, offset;
	FS_List *node;
	FS_MimeEntry *entry;
	FS_CHAR boundary[64];
	
	IFS_Memset( boundary, 0, 64 );
	if( IFS_Strlen(file) == 0 )
	{
		FS_GetGuid( dstFile );
		IFS_Strcat( dstFile, ".eml" );
		IFS_Strcpy( file, dstFile );
	}
	else
	{
		IFS_Strcpy( dstFile, file );
	}
	// encode email head
	offset = 0;
	buf = IFS_Malloc( FS_EML_HEAD_MAX_LEN );
	FS_ASSERT( buf != FS_NULL );
	if( buf )
	{
		IFS_Memset( buf, 0, FS_EML_HEAD_MAX_LEN );
		len = FS_EncodeHead( emlFile, (FS_CHAR *)buf, FS_EML_HEAD_MAX_LEN, boundary );
		FS_ASSERT( len < FS_EML_HEAD_MAX_LEN );
		FS_FileWrite( FS_DIR_EML, dstFile, offset, buf, len );
		offset += len;
		IFS_Free( buf );
	}
	// encode email text if any
	if( emlFile->text )
	{
		buf = IFS_Malloc( FS_EML_TEXT_MAX_BUF );
		FS_ASSERT( buf != FS_NULL );
		if( buf )
		{
			IFS_Memset( buf, 0, FS_EML_TEXT_MAX_BUF );
			if( ! FS_ListIsEmpty(&emlFile->body.list) )	// multipart/mixed here
				len = FS_EncodeText( emlFile->text, boundary, (FS_CHAR *)buf, FS_EML_TEXT_MAX_BUF );
			else	// plain/text here
				len = FS_Base64Encode( (FS_CHAR *)buf, (FS_BYTE *)emlFile->text, -1 );
			FS_ASSERT( len < FS_EML_TEXT_MAX_BUF );
			FS_FileWrite( FS_DIR_EML, dstFile, offset, buf, len );
			offset += len;
			IFS_Free( buf );			
		}
	}
	// encode attach file if any
	node = emlFile->body.list.next;
	while( node != &emlFile->body.list )
	{
		entry = FS_ListEntry( node, FS_MimeEntry, list );
		node = node->next;

		if( entry->type == FS_MIME_MULTIPART )
			continue;
		
		if( entry->temp_file ){
			FS_GetAbsFileName( FS_DIR_TMP, entry->file_name, filename );
		}else{
			IFS_Memset( filename, 0, sizeof(filename) );
			IFS_Strncpy( filename, entry->file_name, sizeof(filename) - 1 );
		}
		
		len = FS_FileGetSize( -1, filename );
		if( len > 0 )
		{
			offset += FS_EncodeAttachFile( entry, boundary, dstFile, offset );
		}
	}
	
	if( ! FS_ListIsEmpty(&emlFile->body.list) )
	{
		IFS_Strcpy( filename, "--" );
		IFS_Strcat( filename, boundary );
		IFS_Strcat( filename, "--\r\n\r\n" );
		FS_FileWrite( FS_DIR_EML, dstFile, offset, filename, IFS_Strlen(filename) );
	}
}

void FS_EmlFreeAddrList( FS_List *head )
{
	FS_EmlAddr *addr;
	FS_List *node = head->next;
	while( node != head )
	{
		addr = FS_ListEntry( node, FS_EmlAddr, list );
		node = node->next;
		FS_ListDel( &addr->list );
		IFS_Free( addr );
	}
}

void FS_InitEmlFile( FS_EmlFile *emlFile )
{
	IFS_Memset( emlFile, 0, sizeof(FS_EmlFile) );
	FS_ListInit( &emlFile->body.list );
	FS_ListInit( &emlFile->body.params );
	FS_ListInit( &emlFile->to );
	FS_ListInit( &emlFile->cc );
	FS_ListInit( &emlFile->bcc );
	FS_ListInit( &emlFile->from.list );
}

void FS_FreeEmlFile( FS_EmlFile *emlFile )
{
	if( emlFile )
	{
		FS_MimeEntry *entry;
		FS_List *node;
		FS_EmlFreeAddrList( &emlFile->to );
		FS_EmlFreeAddrList( &emlFile->cc );
		FS_EmlFreeAddrList( &emlFile->bcc );
		FS_SAFE_FREE( emlFile->subject );
		FS_SAFE_FREE( emlFile->text );
		
		node = emlFile->body.list.next;
		while( node != &emlFile->body.list )
		{
			entry = FS_ListEntry( node, FS_MimeEntry, list );
			node = node->next;
			FS_ListDel( &entry->list );
			FS_FreeMimeEntry( entry );
			IFS_Free( entry );
		}
		FS_FreeMimeEntry( &emlFile->body );
	}
}

#endif	//FS_MODULE_EML

