#include "inc/FS_Config.h"

#ifdef FS_MODULE_MMS

#include "inc/mms/FS_MmsCodec.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_Mime.h"
#include "inc/util/FS_Charset.h"
#include "inc/util/FS_MemDebug.h"

#define FS_MAX_MMS_HEAD_LEN			8192
#define FS_MAX_CID_LEN				32
#define FS_MAX_MMS_TEXT_LEN			16384

#define FS_MMS_H_BCC						0x01
#define FS_MMS_H_CC							0x02
#define FS_MMS_H_CONTENT_LOCATION			0x03
#define FS_MMS_H_CONTENT_TYPE				0x04
#define FS_MMS_H_DATE						0x05
#define FS_MMS_H_DELIVERY_REPORT			0x06
#define FS_MMS_H_EXPIRY						0x08
#define FS_MMS_H_FROM						0x09
#define FS_MMS_H_MESSAGE_CLASS				0x0A
#define FS_MMS_H_MESSAGE_ID					0x0B
#define FS_MMS_H_MESSAGE_TYPE				0x0C
#define FS_MMS_H_VERSION					0x0D
#define FS_MMS_H_MESSAGE_SIZE				0x0E
#define FS_MMS_H_PRIORITY					0x0F
#define FS_MMS_H_READ_REPORT				0x10
#define FS_MMS_H_DELIVERY_REPORT_ALLOW		0x11
#define FS_MMS_H_SENDER_VISABLE				0x14
#define FS_MMS_H_STATUS						0x15
#define FS_MMS_H_SUBJECT					0x16
#define FS_MMS_H_TO							0x17
#define FS_MMS_H_TID						0x18
#define FS_MMS_H_RETRIEVE_STATUS			0x19
#define FS_MMS_H_READ_STATUS				0x1B

#define FS_WSP_H_CONTENT_ID					0x40
#define FS_WSP_H_CONTENT_LOCATION			0x0E

/* content-type field's parameter. see [WSP] */
#define FS_MMS_H_P_NAME						0x05
#define FS_MMS_H_P_START					0x0A
#define FS_MMS_H_P_TYPE						0x09
#define FS_MMS_H_P_CHARSET					0x01
/* from-value */
#define FS_MMS_H_V_ADDR					0x00
#define FS_MMS_H_V_INSERT_ADDR			0x01

#define FS_MMS_H_V_ABS_DATE				0x00
#define FS_MMS_H_V_REL_DATE				0x01

#define FS_MMS_H_V_VERSION				0x12	/* version 1.2 */

/* extern from FS_Wsp.c to process mms header fields. */
extern FS_SINT4 FS_WspHeaderName( FS_BYTE *hCode, FS_CHAR **hText, FS_BYTE *pdu, FS_SINT4 len );
extern FS_SINT4 FS_WspHeaderValue( FS_BYTE *hCode, FS_CHAR **hText, FS_SINT4 *hLen, FS_BYTE *pdu, FS_SINT4 len );

static FS_SINT4 FS_MmsCodecReadTextFile( FS_CHAR *mmsFile, FS_SINT4 offset, FS_SINT4 dir, FS_CHAR *txtFile )
{
	FS_CHAR *buf;
	FS_SINT4 len = FS_FileGetSize( dir, txtFile );

	if( len > 0 )
	{
		buf = IFS_Malloc( len + 1 );
		if( buf )
		{
			IFS_Memset( buf, 0, len + 1 );
			FS_FileRead( dir, txtFile, 0, buf, len );
			if( mmsFile )
				FS_FileWrite( FS_DIR_MMS, mmsFile, offset, buf, len );
			
			IFS_Free( buf );
		}
	}
	return len;
}

static void FS_MmsCodecWriteTextFile( FS_CHAR *file, FS_CHAR *text, FS_SINT4 len, FS_UINT1 charset )
{
	FS_CHAR *str = text;
	FS_SINT4 tlen = len;
	if( charset == 0 || charset == 0x6A )
	{
		/* the default charset is UTF-8 */
		str = FS_ProcessCharset( text, len, "UTF-8", &tlen );
		if( str == FS_NULL ) str = text;
	}

	if( str )
	{
		FS_FileWrite( FS_DIR_TMP, file, 0, str, tlen );
		if( str != text ) IFS_Free( str );
	}
}

static FS_CHAR *FS_MmsCodecDecodeSubject( FS_BYTE *value, FS_SINT4 len )
{
	FS_CHAR *sub = FS_NULL;

	if( value[0] == 0xEA )
	{
		if( value[1] )
		{
			if( value[1] == 0x7F )
			{
				/* I do not know why subject always contain 0x7F in first bytes */
				value ++;
				len --;
			}
			sub = FS_ProcessCharset( value + 1, len - 1, "UTF-8", FS_NULL );
			if( sub == FS_NULL ) sub = FS_Strndup( value + 1, len - 1 );
		}
	}
	else if( IFS_Strstr( value, "=?" ) )
	{
		sub = IFS_Malloc( len + 1 );
		if( sub )
		{
			FS_Rfc2047DecodeString( sub, value );
		}
	}
	else
	{
		sub = FS_Strndup( value + 1, len - 1 );	
	}
	return sub;
}

static FS_SINT4 FS_MmsEncodeValueLength( FS_BYTE *buf, FS_UINT4 len )
{
	FS_SINT4 offset = 0;
	if( len <= 30 )
	{
		buf[offset ++] = (FS_UINT1)len;
	}
	else
	{
		buf[offset ++] = 31;
		offset += FS_UInt4ToUIntVar( buf + offset, len );
	}
	return offset;
}

static FS_SINT4 FS_MmsEncodeCommaSplitAddr( FS_BYTE *buf, FS_CHAR *addr, FS_UINT1 hName )
{
	FS_CHAR *str, *p;
	FS_SINT4 offset = 0, tlen;

	str = addr;
	p = IFS_Strchr( str, ',' );
	while( 1 )
	{
		buf[offset ++] = hName + 0x80;
		if( p )
		{
			tlen = p - str;
			while( str[tlen - 1] <= 0x20 ) tlen --;	/* skip tail white space */
			IFS_Memcpy( buf + offset, str, tlen );
			if( IFS_Strchr( buf + offset, '/' ) == FS_NULL && IFS_Strchr( buf + offset, '@' ) == FS_NULL )
			{
				offset += tlen;
				IFS_Memcpy( buf + offset, "/TYPE=PLMN", 10 );
				tlen = 10;
			}
			offset += tlen;
			buf[offset ++] = 0;
			
			str = p + 1;
			while( *str <= 0x20 ) str ++;	/* skip head white space */
			p = IFS_Strchr( str, ',' );
		}
		else
		{
			tlen = IFS_Strlen( str );
			IFS_Memcpy( buf + offset, str, tlen );
			if( IFS_Strchr( buf + offset, '/' ) == FS_NULL && IFS_Strchr( buf + offset, '@' ) == FS_NULL )
			{
				offset += tlen;
				IFS_Memcpy( buf + offset, "/TYPE=PLMN", 10 );
				tlen = 10;
			}
			offset += tlen;
			buf[offset ++] = 0;
			break;
		}
	}
	return offset;
}

static FS_UINT4 FS_MmsCodecDecodeLongInteger( FS_BYTE *buf, FS_SINT4 len )
{
	FS_UINT4 value = 0;
	FS_SINT4 i;

	len = FS_MIN( len, 4 );
	for( i = len; i > 0; i -- )
	{
		/* bit-endian to little-endian */
		value |= (FS_UINT1)(buf[len - i]) << ((i - 1) * 8);
	}
	return value;
}

static void FS_MmsCodecFreeBody( FS_MmsEncMultipart *pBody )
{
	FS_List *node;
	FS_MmsEncEntry *pEntry;

	node = pBody->entry_list.next;
	while( node != &pBody->entry_list )
	{
		pEntry = FS_ListEntry( node, FS_MmsEncEntry, list );
		node = node->next;

		FS_ListDel( &pEntry->list );
		FS_SAFE_FREE( pEntry->content_id );
		FS_SAFE_FREE( pEntry->param_name );
		FS_SAFE_FREE( pEntry->content_type );
		FS_SAFE_FREE( pEntry->content_location );
		if( pEntry->file && pEntry->temp_file )
		{
			FS_FileDelete( FS_DIR_TMP, pEntry->file );
		}
		FS_SAFE_FREE( pEntry->file );

		IFS_Free( pEntry );
	}
	pBody->count = 0;
}

/* see document [WSP] section 8.5 [Multipart Data] */
static void FS_MmsCodecDecodeBody( FS_MmsEncMultipart *pBody, FS_BYTE *buf, FS_SINT4 len )
{
	FS_UINT4 uval;
	FS_SINT4 offset = 0;
	FS_SINT4 headersLen, hLen, tlen, clen;
	FS_MmsEncEntry *pEntry;
	FS_BYTE hCode, hByte, *hValue;
	FS_CHAR *tstr, *ext;

	IFS_Memset( pBody, 0, sizeof(FS_MmsEncMultipart) );
	FS_ListInit( &pBody->entry_list );
	
	/* multipart header: nEntities */
	offset += FS_UIntVarToUInt4( &uval, buf +offset );
	pBody->count = (FS_SINT4)uval;

	while( offset < len )
	{
		/* Multipart entry: headersLen DataLen ContentType Headers Data */
		pEntry = FS_NEW( FS_MmsEncEntry );
		if( ! pEntry )	return;

		IFS_Memset( pEntry, 0, sizeof(FS_MmsEncEntry) );
		/* headersLen */
		offset += FS_UIntVarToUInt4( &headersLen, buf +offset );
		/* datalen */
		offset += FS_UIntVarToUInt4( &pEntry->data_len, buf +offset );
		/* content-type */
		clen = FS_WspHeaderValue( &hCode, &hValue, &hLen, buf + offset, len - offset );
		
		if( headersLen >= len || headersLen <= 0 )
		{
			FS_SAFE_FREE( hValue );
			IFS_Free( pEntry );
			return;
		}

		pEntry->ct_code = hCode;
		if( hValue )
		{
			/* deal with content-type field. this field may contain START and TYPE parameters */
			if( hValue[0] >= 0x80 )
			{
				pEntry->ct_code = hValue[0] - 0x80;
				tlen = 1;
			}
			else
			{
				pEntry->content_type = IFS_Strdup( hValue );
				tlen = IFS_Strlen( hValue ) + 1;
			}
			
			while( tlen < hLen )
			{
				tlen += FS_WspHeaderName( &hCode, FS_NULL, hValue + tlen, hLen - tlen );
				tlen += FS_WspHeaderValue( &hByte, &tstr, FS_NULL, hValue + tlen, hLen - tlen );
				if( FS_MMS_H_P_NAME == hCode && tstr )
				{
					pEntry->param_name = tstr;
					tstr = FS_NULL;
				}
				else if( FS_MMS_H_P_CHARSET == hCode )
				{
					pEntry->param_charset = hByte;
				}
				FS_SAFE_FREE( tstr );
			}
			IFS_Free( hValue );
		}

		/* headers: headersLen is combined conteyt-type len and headers len */
		tlen = clen;
		while( tlen < headersLen )
		{
			tlen += FS_WspHeaderName( &hCode, &tstr, buf + offset + tlen, len - offset - tlen );
			tlen += FS_WspHeaderValue( FS_NULL, &hValue, &hLen, buf + offset + tlen, len - offset - tlen );
			if( hValue && (hCode == FS_WSP_H_CONTENT_ID || (tstr && IFS_Stricmp(tstr, "Content-ID") == 0) ))
			{
				pEntry->content_id = hValue;
				hValue = FS_NULL;
			}
			else if( hValue && (hCode == FS_WSP_H_CONTENT_LOCATION || (tstr && IFS_Stricmp(tstr, "Content-Location") == 0)))
			{
				pEntry->content_location = hValue;
				hValue = FS_NULL;
			}
			FS_SAFE_FREE( tstr );
			FS_SAFE_FREE( hValue );
		}

		/* data: we save data as a file in the disk */
		offset += headersLen;
		pEntry->file = IFS_Malloc( FS_FILE_NAME_LEN );
		IFS_Memset( pEntry->file, 0, FS_FILE_NAME_LEN );
		FS_GetLuid( pEntry->file );
		ext = FS_NULL;
		if( ext == FS_NULL && pEntry->param_name )
			ext = FS_GetFileExt( pEntry->param_name );
		
		if( ext == FS_NULL && pEntry->content_location )
			ext = FS_GetFileExt( pEntry->content_location );
		
		if( ext == FS_NULL && pEntry->ct_code )
			ext = FS_GetExtFromMimeCode( pEntry->ct_code );
		else if( ext == FS_NULL && pEntry->content_type )
			ext = FS_GetExtFromMime( pEntry->content_type );
		
		if( ext )
		{
			IFS_Strcat( pEntry->file, "." );
			tlen = IFS_Strlen( pEntry->file );
			IFS_Strncpy( pEntry->file + tlen, ext, FS_FILE_NAME_LEN - tlen - 1 );
		}
		if( FS_STR_I_EQUAL( ext, "txt" ) )
		{
			FS_MmsCodecWriteTextFile( pEntry->file, buf + offset, pEntry->data_len, pEntry->param_charset );
			pEntry->param_charset = 0;
		}
		else
		{
			FS_FileWrite( FS_DIR_TMP, pEntry->file, 0, buf + offset, pEntry->data_len );
		}
		pEntry->temp_file = FS_TRUE;
		offset += pEntry->data_len;

		/* add to list */
		FS_ListAddTail( &pBody->entry_list, &pEntry->list );
	}
}

static void FS_MmsCodecEncodeBody( FS_CHAR *file, FS_SINT4 headLen, FS_MmsEncMultipart *pBody )
{
	FS_BYTE *buf;
	FS_SINT4 offset = 0, tlen, slen, dir;
	FS_List *node;
	FS_MmsEncEntry *pEntry;
	FS_BYTE sbuf[16];
	
	buf = IFS_Malloc( FS_MAX_MMS_HEAD_LEN + FS_FILE_BLOCK );
	if( buf )
	{
		/* nEntries uintvar */
		tlen = FS_UInt4ToUIntVar( sbuf, (FS_UINT4)pBody->count );
		FS_FileWrite( FS_DIR_MMS, file, headLen, sbuf, tlen );
		headLen += tlen;
		node = pBody->entry_list.next;
		while( node != &pBody->entry_list )
		{
			IFS_Memset( buf, 0, FS_MAX_MMS_HEAD_LEN + FS_FILE_BLOCK );
			offset = 0;
			pEntry = FS_ListEntry( node, FS_MmsEncEntry, list );
			node = node->next;

			if( pEntry->temp_file )
				dir = FS_DIR_TMP;
			else
				dir = -1;

			/* Content-Type: param name */
			if( pEntry->param_name )
			{
				tlen = IFS_Strlen( pEntry->param_name ) + 2;	/* content-type + param_name + str term char */
				if( (FS_BYTE)(pEntry->param_name[0]) >= 0x80 ) tlen ++;	/* a Quote character must precede it. */
			}
			else
			{
				tlen = 0;
			}

			/* Content-Type: param charset for mms text body */
			if( pEntry->ct_code == FS_WCT_TEXT_PLAIN )
			{
				tlen += 2;
			}
			
			/* content-type len */
			if( pEntry->content_type )
				tlen += IFS_Strlen( pEntry->content_type ) + 1;
			else
				tlen += 1;

			/*
				Value-length = Short-length | (Length-quote Length)
				; Value length is used to indicate the length of the value to follow
				Short-length = <Any octet 0-30>
				Length-quote = <Octet 31>
				Length = Uintvar-integer
			*/
			if( tlen > 1 )
			{
				offset += FS_MmsEncodeValueLength( buf + offset, tlen );
			}
			
			if( pEntry->content_type )
			{
				tlen = IFS_Strlen( pEntry->content_type ) + 1;
				IFS_Memcpy( buf + offset, pEntry->content_type, tlen );
				offset += tlen;
			}
			else
			{
				buf[offset ++] = (FS_UINT1)(pEntry->ct_code + 0x80);
			}
			
			if( pEntry->ct_code == FS_WCT_TEXT_PLAIN ) 
			{
				buf[offset ++] = 0x80 + FS_MMS_H_P_CHARSET;
				buf[offset ++] = 0xEA;
			}
			
			if( pEntry->param_name )
			{
				buf[offset ++] = FS_MMS_H_P_NAME + 0x80;
				if( (FS_BYTE)(pEntry->param_name[0]) >= 0x80 ) buf[offset ++] = 0x7F;	/* a Quote character must precede it. */
				tlen = IFS_Strlen( pEntry->param_name ) + 1;
				IFS_Memcpy( buf + offset, pEntry->param_name, tlen );
				offset += tlen;
			}
			
			/* headers */
			if( pEntry->content_id )
			{
				buf[offset ++] = FS_WSP_H_CONTENT_ID + 0x80;
				tlen = IFS_Strlen( pEntry->content_id) + 1;
				IFS_Memcpy( buf + offset, pEntry->content_id, tlen );
				offset += tlen;
			}
			if( pEntry->content_location )
			{
				buf[offset ++] = FS_WSP_H_CONTENT_LOCATION + 0x80;
				if( (FS_BYTE)(pEntry->content_location[0]) >= 0x80 ) buf[offset ++] = 0x7F;	/* a Quote character must precede it. */
				tlen = IFS_Strlen( pEntry->content_location) + 1;
				IFS_Memcpy( buf + offset, pEntry->content_location, tlen );
				offset += tlen;
			}
			
			tlen = FS_UInt4ToUIntVar( sbuf, (FS_UINT4)offset );	/* headers len */
			if( pEntry->data_len <= 0 )
				pEntry->data_len = FS_FileGetSize( dir, pEntry->file );
			/* data len */
			if( pEntry->ct_code == FS_WCT_TEXT_PLAIN )
			{
				/* if is a text. we must calculate the UTF-8 text length. here we just calc the UTF-8 len */
				slen = FS_MmsCodecReadTextFile( FS_NULL, 0, dir, pEntry->file );
				tlen += FS_UInt4ToUIntVar( sbuf + tlen, (FS_UINT4)slen );
			}
			else
			{
				tlen += FS_UInt4ToUIntVar( sbuf + tlen, (FS_UINT4)pEntry->data_len );
			}
			FS_FileWrite( FS_DIR_MMS, file, headLen, sbuf, tlen );
			headLen += tlen;
			/* content-type and headers */
			FS_FileWrite( FS_DIR_MMS, file, headLen, buf, offset );
			headLen += offset;
			/* data */
			offset = 0;
			if( pEntry->ct_code == FS_WCT_TEXT_PLAIN )
			{
				/* text file. we need convert to UTF-8 charset */
				headLen += FS_MmsCodecReadTextFile( file, headLen, dir, pEntry->file );
			}
			else
			{
				/* other file. just copy to mms file */
				while( (tlen = FS_FileRead(dir, pEntry->file, offset, buf, FS_FILE_BLOCK)) > 0 )
				{
					FS_FileWrite( FS_DIR_MMS, file, headLen, buf, tlen );
					offset += tlen;
					headLen += tlen;
				}
			}
		}
		IFS_Free( buf );
	}
}

FS_SINT4 FS_MmsCodecDecodeHead( FS_MmsEncHead *pHead, FS_BYTE *buf, FS_SINT4 len )
{
	FS_SINT4 offset = 0, hLen, tlen;
	FS_BYTE hCode, hName;
	FS_CHAR *tstr;
	FS_BYTE *hValue;
	
	IFS_Memset( pHead, 0, sizeof(FS_MmsEncHead) );
	offset += FS_WspHeaderName( &hCode, FS_NULL, buf + offset, len - offset );
	if( hCode != FS_MMS_H_MESSAGE_TYPE )
	{
		/* mms header's first field MUST be x-mms-message-type */
		return 0;
	}
	offset += FS_WspHeaderValue( &hCode, FS_NULL, FS_NULL, buf + offset, len - offset );
	pHead->message_type = hCode;

	/*
		In the encoding of the header fields, the order of the fields is not significant, except that X-Mms-Message-Type, 
		XMms-Transaction-ID (when present) and X-Mms-MMS-Version MUST be at the beginning of the message headers,
		in that order, and if the PDU contains a message body the Content Type MUST be the last header field, followed by
		message body.	--------[WSP-ENC]
	*/
	while( offset < len )
	{
		offset += FS_WspHeaderName( &hName, FS_NULL, buf + offset, len - offset );
		offset += FS_WspHeaderValue( &hCode, &hValue, &hLen, buf + offset, len - offset );
		if( FS_MMS_H_VERSION == hName )
		{
			/* x-mms-version */
			pHead->mms_version = hCode;
		}
		else if( FS_MMS_H_BCC == hName && hValue )
		{
			if( pHead->bcc )
			{
				tstr = pHead->bcc;
				pHead->bcc = FS_StrConCat( tstr, ",", hValue, FS_NULL );
				IFS_Free( tstr );
			}
			else
			{
				pHead->bcc = hValue;
				hValue = FS_NULL;
			}
		}
		else if( FS_MMS_H_CC == hName  && hValue )
		{
			if( pHead->cc )
			{
				tstr = pHead->cc;
				pHead->cc = FS_StrConCat( tstr, ",", hValue, FS_NULL );
				IFS_Free( tstr );
			}
			else
			{
				pHead->cc = hValue;
				hValue = FS_NULL;
			}
		}
		else if( FS_MMS_H_TO == hName  && hValue )
		{
			if( pHead->to )
			{
				tstr = pHead->to;
				pHead->to = FS_StrConCat( tstr, ",", hValue, FS_NULL );
				IFS_Free( tstr );
			}
			else
			{
				pHead->to = hValue;
				hValue = FS_NULL;
			}
		}
		else if( FS_MMS_H_CONTENT_LOCATION == hName  && hValue )
		{
			FS_SAFE_FREE( pHead->content_location );
			pHead->content_location = hValue;
			hValue = FS_NULL;
		}
		else if( FS_MMS_H_FROM == hName  && hValue )
		{
			if( pHead->from == FS_NULL && hCode == FS_MMS_H_V_ADDR )
				pHead->from = IFS_Strdup( hValue + 1 );
		}
		else if( FS_MMS_H_CONTENT_TYPE == hName )
		{
			pHead->ct_code = hCode;
			if( hValue )
			{
				/* deal with content-type field. this field may contain START and TYPE parameters */
				if( hValue[0] >= 0x80 )
				{
					pHead->ct_code = hValue[0] - 0x80;
					tlen = 1;
				}
				else
				{
					pHead->content_type = IFS_Strdup( hValue );
					tlen = IFS_Strlen( hValue ) + 1;					
				}

				while( tlen < hLen )
				{
					tlen += FS_WspHeaderName( &hCode, FS_NULL, hValue + tlen, hLen - tlen );
					tlen += FS_WspHeaderValue( FS_NULL, &tstr, FS_NULL, hValue + tlen, hLen - tlen );
					if( FS_MMS_H_P_START == hCode && tstr )
					{
						pHead->param_start = tstr;
						tstr = FS_NULL;
					}
					else if( FS_MMS_H_P_TYPE == hCode && tstr )
					{
						pHead->param_type = tstr;
						tstr = FS_NULL;
					}

					FS_SAFE_FREE( tstr );
				}
			}
			
			FS_SAFE_FREE( hValue );
			/* content-type is the end of header. */
			return offset;
		}
		else if( FS_MMS_H_DATE == hName && hValue )
		{
			/* long-interger as to DATE. see [MMS-ENC] */
			pHead->date = FS_MmsCodecDecodeLongInteger( hValue, hLen );
		}
		else if( FS_MMS_H_DELIVERY_REPORT == hName )
		{
			pHead->delivery_report = hCode;
		}
		else if( FS_MMS_H_MESSAGE_CLASS == hName )
		{
			pHead->message_class = hCode;
		}
		else if( FS_MMS_H_MESSAGE_ID == hName && hValue )
		{
			if( pHead->message_id == FS_NULL )
			{
				pHead->message_id = hValue;
				hValue = FS_NULL;
			}
		}
		else if( FS_MMS_H_MESSAGE_SIZE == hName && hValue )
		{
			/* long-interger. see [MMS-ENC] */
			pHead->message_size = FS_MmsCodecDecodeLongInteger( hValue, hLen );
		}
		else if( FS_MMS_H_PRIORITY == hName )
		{
			pHead->priority = hCode;
		}
		else if( FS_MMS_H_READ_REPORT == hName )
		{
			pHead->read_report = hCode;
		}
		else if( FS_MMS_H_SUBJECT == hName && hValue )
		{
			if( pHead->subject == FS_NULL )
			{
				pHead->subject = FS_MmsCodecDecodeSubject( hValue, hLen );
			}
		}
		else if( FS_MMS_H_TID == hName && hValue )
		{
			if( pHead->tid == FS_NULL )
			{
				pHead->tid = hValue;
				hValue = FS_NULL;
			}
		}
		else if( FS_MMS_H_RETRIEVE_STATUS == hName )
		{
			pHead->retrieve_status = hCode;			
		}
		else if( FS_MMS_H_READ_STATUS == hName )
		{
			pHead->read_status = hCode;			
		}
		else if( FS_MMS_H_EXPIRY == hName && hValue )
		{
			pHead->expiry = FS_MmsCodecDecodeLongInteger( hValue + 1, hLen - 1 );
			if( hCode == FS_MMS_H_V_REL_DATE )
			{
				/*
				 * !!!!!!!!!!!!
				 * ugly cmcc mms gatway. the expiry is 50504427 seconds. its 584 days.
				 * here, we will handle this ugly bug. if expiry is more than 100 days, it will div to 1000.
				*/
				if( pHead->expiry > (3600 * 24 * 100) )
					pHead->expiry = pHead->expiry / 1000;
				if( pHead->date > 0 )
					pHead->expiry += pHead->date;
				else
					pHead->expiry += FS_GetSeconds( IFS_GetTimeZone() );
			}
		}
		FS_SAFE_FREE( hValue );
	}

	return offset;
}

void FS_MmsCodecFreeHead( FS_MmsEncHead *pHead )
{
	FS_SAFE_FREE( pHead->bcc );
	FS_SAFE_FREE( pHead->cc );
	FS_SAFE_FREE( pHead->content_location );
	FS_SAFE_FREE( pHead->content_type );
	FS_SAFE_FREE( pHead->from );
	FS_SAFE_FREE( pHead->message_id );
	FS_SAFE_FREE( pHead->param_start );
	FS_SAFE_FREE( pHead->param_type );
	FS_SAFE_FREE( pHead->subject );
	FS_SAFE_FREE( pHead->tid );
	FS_SAFE_FREE( pHead->to );
	IFS_Memset( pHead, 0, sizeof(FS_MmsEncHead) );
}

FS_BOOL FS_MmsCodecDecodeFile( FS_MmsEncData *pMms, FS_CHAR *file )
{
	FS_Handle hFile;
	FS_SINT4 size, hlen;
	FS_BYTE *buf;
	FS_BOOL ret = FS_FALSE;
	
	if( FS_FileOpen( &hFile, FS_DIR_MMS, file, FS_OPEN_READ ) )
	{
		size = IFS_FileGetSize( hFile );
		if( size > 0 )
		{
			buf = IFS_Malloc( size );
			if( buf )
			{
				IFS_FileRead( hFile, buf, size );
				hlen = FS_MmsCodecDecodeHead( &pMms->head, buf, size );
				if( hlen > 0 )
				{
					FS_MmsCodecDecodeBody( &pMms->body, buf + hlen, size - hlen );
					ret = FS_TRUE;
				}
				IFS_Free( buf );
			}
		}
		IFS_FileClose( hFile );
	}

	if( ret == FS_FALSE )
	{
		FS_MmsCodecFreeHead( &pMms->head );
		FS_MmsCodecFreeBody( &pMms->body );
	}
	return ret;
}

void FS_MmsCodecFreeData( FS_MmsEncData *pMms )
{
	FS_MmsCodecFreeHead( &pMms->head );
	FS_MmsCodecFreeBody( &pMms->body );
}

FS_BOOL FS_MmsCodecEncodeFile( FS_CHAR *file, FS_MmsEncData *pMms )
{
	FS_SINT4 offset = 0;
	FS_BOOL ret = FS_FALSE;
	FS_Handle hFile;
	FS_BYTE *buf;
	
	if( FS_FileCreate( &hFile, FS_DIR_MMS, file, FS_OPEN_WRITE ) )
	{
		buf = IFS_Malloc( FS_MAX_MMS_HEAD_LEN );
		if( buf )
		{
			IFS_Memset( buf, 0, FS_MAX_MMS_HEAD_LEN );
			offset = FS_MmsCodecEncodeHead( buf, &pMms->head );
			if( offset > 0 ) IFS_FileWrite( hFile, buf, offset );
			IFS_Free( buf );
		}
		IFS_FileClose( hFile );
	}
	
	if( offset > 0 )
	{
		FS_MmsCodecEncodeBody( file, offset, &pMms->body );
		ret = FS_TRUE;
	}
	
	return ret;
}

/* return content-id */
FS_CHAR * FS_MmsCodecCreateEntry( FS_MmsEncData *pData, FS_CHAR *file, FS_SINT4 filesize )
{
	FS_MmsEncEntry *pEntry;
	FS_CHAR *str = FS_NULL;
	
	pEntry = FS_NEW( FS_MmsEncEntry );
	if( pEntry )
	{
		IFS_Memset( pEntry, 0, sizeof(FS_MmsEncEntry) );
		/* file */
		pEntry->file = IFS_Strdup( file );
		/* generate a content-id */
		pEntry->content_id = IFS_Malloc( FS_MAX_CID_LEN );
		IFS_Strcpy( pEntry->content_id, "<id-" );
		FS_GetLuid( pEntry->content_id + 4 );
		IFS_Strcat( pEntry->content_id, ">" );
		/* get content-type from file extension */
		pEntry->ct_code = (FS_UINT1)FS_GetMimeCodeFromExt( file );
		if( pEntry->ct_code == 0 )
		{
			str = FS_GetMimeFromExt( file );
			pEntry->content_type = IFS_Strdup( str );
		}
		/* content-location */
		str = FS_GetFileNameFromPath( file );
		pEntry->content_location = IFS_Strdup( str );
		if( filesize > 0 )
			pEntry->data_len = filesize;
		else
			pEntry->data_len = FS_FileGetSize( -1, file );
		/* judge if a temp file */
		filesize = FS_FileGetSize( FS_DIR_TMP, file );
		if( filesize > 0 )
		{
			pEntry->data_len = filesize;
			pEntry->temp_file = FS_TRUE;
		}

		FS_ListAddTail( &pData->body.entry_list, &pEntry->list );
		pData->body.count ++;
		
		str = pEntry->content_id;
	}

	return str;
}

void FS_MmsCodecUpdateEntry( FS_MmsEncData *pData, FS_CHAR *cid, FS_CHAR *file, FS_SINT4 filesize )
{
	FS_List *node;
	FS_MmsEncEntry *pEntry;
	FS_CHAR *str;
	
	node = pData->body.entry_list.next;
	while( node != &pData->body.entry_list )
	{
		pEntry = FS_ListEntry( node, FS_MmsEncEntry, list );
		node = node->next;

		if( FS_ContentIdEqual( pEntry->content_id, cid ) )
		{
			/* reset old data */
			pEntry->ct_code = 0;
			FS_SAFE_FREE( pEntry->content_type );
			pEntry->content_type = FS_NULL;
			FS_SAFE_FREE( pEntry->content_location );
			pEntry->content_location = FS_NULL;
			if( pEntry->file && pEntry->temp_file )
			{
				FS_FileDelete( FS_DIR_TMP, pEntry->file );
			}
			pEntry->temp_file = FS_FALSE;
			FS_SAFE_FREE( pEntry->file );

			/* file */
			pEntry->file = IFS_Strdup( file );
			/* get content-type from file extension */
			pEntry->ct_code = (FS_UINT1)FS_GetMimeCodeFromExt( file );
			if( pEntry->ct_code == 0 )
			{
				str = FS_GetMimeFromExt( file );
				pEntry->content_type = IFS_Strdup( str );
			}
			/* content-location */
			str = FS_GetFileNameFromPath( file );
			pEntry->content_location = IFS_Strdup( str );
			if( filesize > 0 )
				pEntry->data_len = filesize;
			else
				pEntry->data_len = FS_FileGetSize( -1, file );
			
			filesize = FS_FileGetSize( FS_DIR_TMP, file );
			if( filesize > 0 )
			{
				pEntry->data_len = filesize;
				pEntry->temp_file = FS_TRUE;
			}
		}
	}
}

void FS_MmsCodecDeleteEntry( FS_MmsEncData *pData, FS_CHAR *cid )
{
	FS_List *node;
	FS_MmsEncEntry *pEntry;
	
	node = pData->body.entry_list.next;
	while( node != &pData->body.entry_list )
	{
		pEntry = FS_ListEntry( node, FS_MmsEncEntry, list );
		node = node->next;

		if( FS_ContentIdEqual( pEntry->content_id, cid ) )
		{
			FS_ListDel( &pEntry->list );
			FS_SAFE_FREE( pEntry->content_id );
			FS_SAFE_FREE( pEntry->param_name );
			FS_SAFE_FREE( pEntry->content_type );
			FS_SAFE_FREE( pEntry->content_location );
			if( pEntry->file && pEntry->temp_file )
			{
				FS_FileDelete( FS_DIR_TMP, pEntry->file );
			}
			FS_SAFE_FREE( pEntry->file );
			
			IFS_Free( pEntry );
			pData->body.count --;
			return;
		}
	}
}

FS_BOOL FS_MmsCodecDecodeFileHead( FS_MmsEncHead *pHead, FS_CHAR *file )
{
	FS_BYTE *buf;
	FS_SINT4 len = 0;

	IFS_Memset( pHead, 0, sizeof(FS_MmsEncHead) );
	buf = IFS_Malloc( FS_MAX_MMS_HEAD_LEN );
	if( buf )
	{
		len = FS_FileRead( FS_DIR_MMS, file, 0, buf, FS_MAX_MMS_HEAD_LEN );
		if( len > 0 )
			len = FS_MmsCodecDecodeHead( pHead, buf, len );
		IFS_Free( buf );
	}
	return (FS_BOOL)( len > 0 );
}

FS_SINT4 FS_MmsCodecEncodeHead( FS_BYTE *buf, FS_MmsEncHead *pHead )
{
	FS_SINT4 offset, tlen, len;

	offset = 0;
	/* message type */
	buf[offset ++] = FS_MMS_H_MESSAGE_TYPE + 0x80;
	buf[offset ++] = pHead->message_type + 0x80;
	/* tid if any */
	if( pHead->tid )
	{
		buf[offset ++] = FS_MMS_H_TID + 0x80;
		tlen = IFS_Strlen( pHead->tid ) + 1;
		IFS_Memcpy( buf + offset, pHead->tid, tlen );
		offset += tlen;
	}
	/* version */
	buf[offset ++] = FS_MMS_H_VERSION + 0x80;
	buf[offset ++] = FS_MMS_H_V_VERSION + 0x80;
	/* messge-id if any */
	if( pHead->message_id )
	{
		buf[offset ++] = FS_MMS_H_MESSAGE_ID+ 0x80;
		tlen = IFS_Strlen( pHead->message_id ) + 1;
		IFS_Memcpy( buf + offset, pHead->message_id, tlen );
		offset += tlen;
	}
	/* FS_M_NOTIFY_RESP_IND PDU */
	if( pHead->message_type == FS_M_NOTIFY_RESP_IND )
	{
		buf[offset ++] = FS_MMS_H_STATUS + 0x80;
		buf[offset ++] = pHead->status + 0x80;
		buf[offset ++] = FS_MMS_H_DELIVERY_REPORT_ALLOW + 0x80;
		buf[offset ++] = pHead->delivery_report_allow + 0x80;
		return offset;
	}
	/* date */
	if( pHead->date > 0 )
	{
		buf[offset ++] = FS_MMS_H_DATE + 0x80;
		buf[offset ++] = 0x04;
		buf[offset ++] = (FS_UINT1)((pHead->date >> 24) & 0xFF);
		buf[offset ++] = (FS_UINT1)((pHead->date >> 16) & 0xFF);
		buf[offset ++] = (FS_UINT1)((pHead->date >> 8) & 0xFF);
		buf[offset ++] = (FS_UINT1)(pHead->date & 0xFF);
	}
	/* from */
	if( pHead->message_type == FS_M_SEND_REQ || pHead->message_type == FS_M_READ_REC_IND )
	{
		buf[offset ++] = FS_MMS_H_FROM + 0x80;
		if( pHead->from )
		{
			tlen = IFS_Strlen( pHead->from ) + 2;
			offset += FS_MmsEncodeValueLength( buf + offset, tlen );
			buf[offset ++] = FS_MMS_H_V_ADDR + 0x80;
			IFS_Memcpy( buf + offset, pHead->from, tlen - 1 );
			offset += (tlen - 1);
		}
		else
		{
			buf[offset ++] = 0x01;
			buf[offset ++] = FS_MMS_H_V_INSERT_ADDR + 0x80;
		}
	}
	/* to */
	if( pHead->to )
	{
		offset += FS_MmsEncodeCommaSplitAddr( buf + offset, pHead->to, FS_MMS_H_TO );
	}
	/* FS_M_READ_REC_IND PDU */
	if( pHead->message_type == FS_M_READ_REC_IND )
	{
		buf[offset ++] = FS_MMS_H_READ_STATUS + 0x80;
		buf[offset ++] = pHead->read_status + 0x80;
		return offset;
	}
	/* cc */
	if( pHead->cc )
	{
		offset += FS_MmsEncodeCommaSplitAddr( buf + offset, pHead->cc, FS_MMS_H_CC );
	}
	/* bcc */
	if( pHead->bcc )
	{
		offset += FS_MmsEncodeCommaSplitAddr( buf + offset, pHead->bcc, FS_MMS_H_BCC );
	}
	/* subject */
	if( pHead->subject )
	{
		/* Encoded-string-value = Text-string | Value-length Char-set Text-string */
		buf[offset ++] = FS_MMS_H_SUBJECT + 0x80;
	
		len = offset + 5;
		tlen = IFS_Strlen( pHead->subject );
		/* add two bytes for charset flag and str term char */
		offset += FS_MmsEncodeValueLength( buf + offset, tlen + 2 );
		buf[offset ++] = 0xEA;	/* charset is UTF-8 */
		IFS_Memcpy( buf + offset, pHead->subject, tlen );
		offset += tlen;
		buf[offset ++] = 0; /* string term char */
	}
	if( pHead->message_type == FS_M_SEND_REQ )
	{
		/* X-Mms-Message-Class */
		buf[offset ++] = FS_MMS_H_MESSAGE_CLASS + 0x80;
		buf[offset ++] = pHead->message_class + 0x80;
		/* X-Mms-Priority */
		buf[offset ++] = FS_MMS_H_PRIORITY + 0x80;
		buf[offset ++] = pHead->priority + 0x80;
		/* X-Mms-Sender-Visibility on ly m-send-req message type */ 
		buf[offset ++] = FS_MMS_H_SENDER_VISABLE + 0x80;
		buf[offset ++] = 0x81;
		/* delivery report */
		buf[offset ++] = FS_MMS_H_DELIVERY_REPORT + 0x80;
		buf[offset ++] = pHead->delivery_report + 0x80;
		/* read report */
		buf[offset ++] = FS_MMS_H_READ_REPORT + 0x80;
		buf[offset ++] = pHead->read_report + 0x80;
	
		/* Content-Type must be the last of header fields */
		buf[offset ++] = FS_MMS_H_CONTENT_TYPE + 0x80;
		if( pHead->param_start && pHead->param_type )
		{
			tlen = IFS_Strlen( pHead->param_start ) + 1;
			tlen += IFS_Strlen( pHead->param_type ) + 1;
			tlen += 3;
			offset += FS_MmsEncodeValueLength( buf + offset, tlen );
		}
		buf[offset ++] = (FS_UINT1)(pHead->ct_code + 0x80); /* we just use mime code */
		if( pHead->param_start && pHead->param_type )
		{
			buf[offset ++] = 0x80 + FS_MMS_H_P_START;
			tlen = IFS_Strlen( pHead->param_start ) + 1;
			IFS_Memcpy( buf + offset, pHead->param_start, tlen );
			offset += tlen;
		
			buf[offset ++] = 0x80 + FS_MMS_H_P_TYPE;
			tlen = IFS_Strlen( pHead->param_type ) + 1;
			IFS_Memcpy( buf + offset, pHead->param_type, tlen );
			offset += tlen;
		}
	}
	return offset;
}

#ifdef FS_DEBUG_
void FS_MmsCodecDecodeTest( void )
{
	FS_MmsEncData mms;

	IFS_Memset( &mms, 0, sizeof(FS_MmsEncData) );
	FS_ListInit( &mms.body.entry_list );
	
	FS_MmsCodecDecodeFile( &mms, "SmartFox.mms" );

	FS_MmsCodecFreeData( &mms );
}

void FS_MmsCodecEncodeTest( void )
{
	FS_MmsEncData mms;
	FS_MmsEncEntry *pEntry;
	
	IFS_Memset( &mms, 0, sizeof(FS_MmsEncData) );
	FS_ListInit( &mms.body.entry_list );

	mms.head.ct_code = FS_MMS_H_V_MULTIPART_MIXED;
	mms.head.from = IFS_Strdup( "+8613859901820/TYPE=PLMN" );
	mms.head.to = IFS_Strdup( "+8613859901820/TYPE=PLMN" );
	mms.head.cc = IFS_Strdup( "+8613859901920/TYPE=PLMN, +8613859901935/TYPE=PLMN" );
	mms.head.subject = IFS_Strdup( "Áéºü²ÊÐÅ(SmartFox)" );
	mms.head.date = FS_GetSeconds( IFS_GetTimeZone() );
	mms.head.delivery_report = FS_MMS_H_V_DELIVERY_REPORT_NO;
	mms.head.read_report = FS_MMS_H_V_READ_REPORT_NO;
	mms.head.message_class = FS_MMS_H_V_CLASS_PERSONAL;
	mms.head.message_type = FS_M_SEND_REQ;
	mms.head.priority = FS_MMS_H_V_PRIORITY_NORMAL;
	mms.head.tid = IFS_Malloc( FS_MMS_TID_LEN );
	FS_GetLuid( mms.head.tid );	
	
	mms.body.count = 2;
	pEntry = IFS_Malloc( sizeof(FS_MmsEncEntry) );
	IFS_Memset( pEntry, 0, sizeof(FS_MmsEncEntry) );
	pEntry->content_type = IFS_Strdup( "text/plain" );
	pEntry->content_id = IFS_Malloc( FS_MAX_CID_LEN );
	IFS_Strcpy( pEntry->content_id, "<" );
	FS_GetLuid( pEntry->content_id + 1 );
	IFS_Strcat( pEntry->content_id, ">" );
	pEntry->param_name = IFS_Strdup( "text.txt" );
	pEntry->content_location = IFS_Strdup( "text.txt" );
	pEntry->file = IFS_Strdup( "C:\\F_SOFT\\mms\\text.txt" );
	pEntry->data_len = FS_FileGetSize( -1, pEntry->file );
	
	FS_ListAddTail( &mms.body.entry_list, &pEntry->list );

	pEntry = IFS_Malloc( sizeof(FS_MmsEncEntry) );
	IFS_Memset( pEntry, 0, sizeof(FS_MmsEncEntry) );
	pEntry->ct_code = FS_WCT_IMAGE_JPEG;
	pEntry->content_id = IFS_Malloc( FS_MAX_CID_LEN );
	IFS_Strcpy( pEntry->content_id, "<" );
	FS_GetLuid( pEntry->content_id + 1 );
	IFS_Strcat( pEntry->content_id, ">" );
	
	pEntry->param_name = IFS_Strdup( "image.jpg" );
	pEntry->content_location = IFS_Strdup( "image.jpg" );
	pEntry->file = IFS_Strdup( "C:\\F_SOFT\\mms\\image.jpg" );
	pEntry->data_len = FS_FileGetSize( -1, pEntry->file );

	FS_ListAddTail( &mms.body.entry_list, &pEntry->list );

	FS_MmsCodecEncodeFile( "mms_codec_encode_test.mms", &mms );

	FS_MmsCodecFreeData( &mms );
}

void FS_MmsCodecNtfTest( void )
{
	FS_MmsEncHead head;
	FS_BYTE *buf;
	FS_SINT4 size;

	size = FS_FileGetSize( FS_DIR_MMS, "test.ntf" );
	if( size > 0 )
	{
		buf = IFS_Malloc( size );
		FS_FileRead( FS_DIR_MMS, "test.ntf", 0, buf, size );
		FS_MmsCodecDecodeHead( &head, buf, size );
	}

	FS_MmsCodecFreeHead( &head );
}
#endif

#endif	//FS_MODULE_MMS


