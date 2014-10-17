#include "inc/web/FS_Wsp.h"
#include "inc/web/FS_Wtp.h"
#include "inc/inte/FS_Inte.h"
#include "inc/util/FS_List.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_MemDebug.h"

#define _WSP_DEBUG

#ifdef _WSP_DEBUG
#define FS_WSP_TRACE0(a)				FS_TRACE0( "[WSP]" a "\r\n")
#define FS_WSP_TRACE1(a,b)				FS_TRACE1( "[WSP]" a "\r\n", b)
#define FS_WSP_TRACE2(a,b,c)			FS_TRACE2( "[WSP]" a "\r\n", b, c)
#define FS_WSP_TRACE3(a,b,c,d)			FS_TRACE3( "[WSP]" a "\r\n", b, c, d)
#else
#define FS_WSP_TRACE0(a)
#define FS_WSP_TRACE1(a,b)
#define FS_WSP_TRACE2(a,b,c)
#define FS_WSP_TRACE3(a,b,c,d)
#endif

#define FS_WSP_HEADER_LEN		2048

#define FS_WSP_CUR_VERSION		0x10

#define FS_WSP_SDU_SIZE			512000

/* wsp session state */
#define FS_WSP_STS_NULL			0x00
#define FS_WSP_STS_CONNECTING	0x01
#define FS_WSP_STS_CONNECTED	0x02
#define FS_WSP_STS_SUSPENDED	0x03
#define FS_WSP_STS_RESUMING		0x04

#define FS_WSP_MSTS_NULL		0x00
#define FS_WSP_MSTS_REQUESTING	0x01
#define FS_WSP_MSTS_WAITING		0x02
#define FS_WSP_MSTS_RESPONSE	0x03
#define FS_WSP_MSTS_COMPLETE	0x04

/* wsp pdu type */
#define FS_WSP_CONNECT			0x01
#define FS_WSP_CONNECT_REPLY	0x02
#define FS_WSP_REDIRECT			0x03
#define FS_WSP_REPLY			0x04
#define FS_WSP_DISCONNECT		0x05
#define FS_WSP_PUSH				0x06
#define FS_WSP_CONFIRMED_PUSH	0x07
#define FS_WSP_SUSPEND			0x08
#define FS_WSP_RESUME			0x09
#define FS_WSP_GET				0x40
#define FS_WSP_POST				0x60

/* Wsp Capability IDentifier */
#define FS_WCID_CLIENT_SDU		0x00
#define FS_WCID_SERVER_SDU		0x01
#define FS_WCID_PROTOCOL_OPT	0x02
#define FS_WCID_METHOD_MOR		0x03
#define FS_WCID_PUSH_MOR		0x04
#define FS_WCID_HEADER_CODEPAGE	0x06

/* protocol option: Push Facility + Session Resume Facility + Acknowledgement Headers */
#define FS_WSP_DEF_PROTOCOL_OPTION		0x70	

/* wsp header field name */
#define FS_WH_ACCEPT			0x00
#define FS_WH_ACCEPT_CHARSET	0x01
#define FS_WH_CONTENT_LENGTH	0x0D
#define FS_WH_CONTENT_TYPE		0x11
#define FS_WH_LOCATION			0x1C
#define FS_WH_USER_AGENT		0x29
#define FS_WH_REFERER			0x24
#define FS_WH_APPLICATION_ID	0x2F
#define FS_WH_UAPROFILE		0x35

/* wsp abort reason code */
#define FS_WSP_ABORT_PROTOERR			0xE0
#define FS_WSP_ABORT_DISCONNECT 		0xE1
#define FS_WSP_ABORT_USERREQ			0xEA

/* wsp pdu header operation */
#define FS_WspSetPduType( pdu, type )	( (pdu)[0] = (type) )

#define FS_WspSetCurVersion( pdu )	( (pdu)[1] = FS_WSP_CUR_VERSION )

/* wsp pdu information get */
#define FS_WspPduType( pdu )	( (pdu)[0] )

#define FS_WspCapsId( caps )	( (caps)[0] - 0x80 )

#define FS_WspReplyStatus( pdu )	( (pdu)[0] )

typedef struct FS_WspSession_Tag
{
	/* call back event notify */
	void *						user_data;
	FS_WspEventFunc				event_func;

	/* wtp trans */
	FS_WtpTransHandle			hWtp;
	FS_SockAddr					addr;

	/* method param */
	FS_UINT1					method;
	FS_CHAR *					url;
	FS_WspPostDataStruct		post_data;
	
	/* capability */
	FS_UINT4					client_sdu;
	FS_UINT4					server_sdu;	

	/* method invoke reply data */
	FS_BYTE						reply_status;
	FS_UINT2					content_type;		/* compact encoding format content type */
	FS_CHAR *					mime_content;		/* string format content type */
	FS_CHAR *					location;

	/* push releated */
	FS_UINT1					app_id_code;
	FS_CHAR *					app_id_value;
	
	/* internal session data */
	FS_BYTE						state;
	FS_BYTE						method_state;
	
	FS_UINT4					client_session_id;
	FS_UINT4					server_session_id;

	FS_BOOL						in_use;
}FS_WspSession;

static FS_WspSession GFS_WspSession;

static FS_BYTE * FS_WspGetCapability( FS_WspSession *wsp, FS_SINT4 *capLen )
{
	static FS_BYTE s_cap[64];
	
	FS_BYTE uintvar[8];
	FS_SINT1 vlen, offset = 0;

	/* client sdu capability */
	vlen = FS_UInt4ToUIntVar( uintvar, wsp->client_sdu );	/* client sdu */
	s_cap[offset ++] = vlen + 1;							/* cap len */
	s_cap[offset ++] = FS_WCID_CLIENT_SDU + 0x80;			/* cap id */
	IFS_Memcpy( s_cap + offset, uintvar, vlen );			/* cap param */
	offset += vlen;

	/* server sdu capability */
	vlen = FS_UInt4ToUIntVar( uintvar, wsp->server_sdu );	/* client sdu */
	s_cap[offset ++] = vlen + 1;							/* cap len */
	s_cap[offset ++] = FS_WCID_SERVER_SDU + 0x80;			/* cap id */
	IFS_Memcpy( s_cap + offset, uintvar, vlen );			/* cap param */
	offset += vlen;

	/* protocol option capability */
	s_cap[offset ++] = 0x02;								/* cap len */
	s_cap[offset ++] = FS_WCID_PROTOCOL_OPT + 0x80;			/* cap id */
	s_cap[offset ++] = FS_WSP_DEF_PROTOCOL_OPTION;			/* cap param */

	/* 
		other capabilitys user default value and is omitted: (SPEC-WSP-19990528)
		aliases: none
		extended methods: none
		header code page: none
		method mor: 1
		push mor: 1
	*/
	*capLen = offset;
	return s_cap;
}

static FS_BYTE *FS_WspGetHeaders( FS_WspSession *wsp, FS_SINT4 *headLen )
{
	static FS_BYTE s_header[FS_WSP_HEADER_LEN];
	FS_CHAR *audio = "audio/*", *user_agent, *ua_profile;
	FS_BYTE accept_content[] = { 0x01, 0x10, 0x1C, 0x00, 0xFF };	/* text.*, application.*, image.*, *.* */
	FS_SINT4 offset = 0, i = 0, tlen;

	/* accept */
	while( accept_content[i] != 0xFF )
	{
		s_header[offset ++] = FS_WH_ACCEPT + 0x80;
		s_header[offset ++] = accept_content[i] + 0x80;
		i ++;
	}
	/* audio is not in code page define in SPEC-WSP-1999 */
	s_header[offset ++] = FS_WH_ACCEPT + 0x80;
	tlen = IFS_Strlen( audio );
	IFS_Memcpy( s_header + offset, audio, tlen + 1);
	offset += tlen + 1;
	
	/* user-agent */
	user_agent = IFS_GetUserAgent( );
	tlen = IFS_Strlen( user_agent );
	s_header[offset ++] = FS_WH_USER_AGENT + 0x80;
	IFS_Memcpy( s_header + offset, user_agent, tlen + 1);
	offset += tlen + 1;

	/* ua_profile */
	ua_profile = IFS_GetUaProfile( );
	if( ua_profile )
	{
		tlen = IFS_Strlen( ua_profile );
		s_header[offset ++] = FS_WH_UAPROFILE + 0x80;
		IFS_Memcpy( s_header + offset, ua_profile, tlen + 1);
		offset += tlen + 1;
	}
	
	*headLen = offset;
	return s_header;
}

static void FS_WspSendConnectPdu( FS_WspSession *wsp, FS_BOOL bResume )
{
	FS_BYTE *pdu = IFS_Malloc( FS_WSP_HEADER_LEN );
	FS_SINT4 capLen, headLen, offset = 0;
	FS_BYTE *cap, *headers;
	FS_WtpInvokeData iData;
	if( pdu )
	{
		IFS_Memset( pdu, 0, FS_WSP_HEADER_LEN );
		if( bResume )
		{
			FS_WspSetPduType( pdu, FS_WSP_RESUME );	/* pdu type */
			offset = 1;
			offset += FS_UInt4ToUIntVar( pdu, wsp->server_session_id );
			wsp->state = FS_WSP_STS_RESUMING;
		}
		else
		{
			FS_WspSetPduType( pdu, FS_WSP_CONNECT );	/* pdu type */
			FS_WspSetCurVersion( pdu );					/* wsp version */
			offset = 2;
			wsp->state = FS_WSP_STS_CONNECTING;
		}
		cap = FS_WspGetCapability( wsp, &capLen );
		offset += FS_UInt4ToUIntVar( pdu + offset, capLen );		/* capability len */
		
		headers = FS_WspGetHeaders( wsp, &headLen );
		offset += FS_UInt4ToUIntVar( pdu + offset, headLen );		/* headers len */

		IFS_Memcpy( pdu + offset, cap, capLen );
		offset += capLen;
		IFS_Memcpy( pdu + offset, headers, headLen );
		offset += headLen;

		IFS_Memset( &iData, 0, sizeof(FS_WtpInvokeData) );
		iData.ack = FS_FALSE;
		iData.host = wsp->addr.host;
		iData.port = wsp->addr.port;
		iData.xlass = FS_WTP_CLASS_2;
		iData.data = pdu;
		iData.len = offset;
		FS_WSP_TRACE1( "FS_WspSendConnectPdu pdu len = %d", offset );
		FS_WtpInvokeReq( wsp->hWtp, &iData );
		IFS_Free( pdu );
	}
}

static void FS_WspSendGetPdu( FS_WspSession *wsp, FS_CHAR *url )
{
	FS_BYTE *pdu = IFS_Malloc( FS_WSP_HEADER_LEN );
	FS_SINT4 urlLen, offset;
	FS_WtpInvokeData iData;
	if( pdu )
	{
		IFS_Memset( pdu, 0, FS_WSP_HEADER_LEN );
		FS_WspSetPduType( pdu, FS_WSP_GET );
		urlLen = IFS_Strlen( url );
		offset = 1;
		offset += FS_UInt4ToUIntVar( pdu + 1, urlLen );
		IFS_Memcpy( pdu + offset, url, urlLen );
		offset += urlLen;

		IFS_Memset( &iData, 0, sizeof(FS_WtpInvokeData) );
		iData.ack = FS_FALSE;
		iData.host = wsp->addr.host;
		iData.port = wsp->addr.port;
		iData.xlass = FS_WTP_CLASS_2;
		iData.data = pdu;
		iData.len = offset;
		FS_WSP_TRACE1( "FS_WspSendGetPdu pdu len = %d", offset );
		FS_WtpInvokeReq( wsp->hWtp, &iData );

		wsp->method_state = FS_WSP_MSTS_REQUESTING;
		IFS_Free( pdu );
	}
}

static FS_SINT4 FS_WspEncodePostHeaders( FS_BYTE *out, FS_WspPostDataStruct *pData )
{
	FS_SINT4 offset = 0, tlen;
	if( pData->content_type )
	{
		tlen = IFS_Strlen( pData->content_type ) + 1;
		IFS_Memcpy( out, pData->content_type, tlen );
		offset = tlen;
	}
	else
	{
		out[0] = pData->int_enc_content + 0x80;
		offset = 1;
	}

	if( pData->referer )
	{
		out[offset ++] = FS_WH_REFERER + 0x80;
		tlen = IFS_Strlen( pData->referer ) + 1;
		IFS_Memcpy( out + offset, pData->referer, tlen );
		offset += tlen;
	}

	return offset;
}

/*
	post pdu struct
	urlLen  			[uintvar]
	headerslen			[uintvar] including the content-type len and headers len
	url					[urlLen octet]
	content-type		[multi octet]
	headers				[headerlen - content-type len octet]
	data				[multi octet]
*/
static void FS_WspSendPostPdu( FS_WspSession *wsp, FS_CHAR *url, FS_WspPostDataStruct *pData )
{
	FS_BYTE *pdu;
	FS_SINT4 urlLen, headersLen, offset, hpos, tlen;
	FS_WtpInvokeData iData;

	tlen = FS_WSP_HEADER_LEN;
	if( ! pData->data_is_file )
		tlen += pData->data_len;
	pdu = IFS_Malloc( tlen );
	if( pdu )
	{
		IFS_Memset( pdu, 0, tlen );
		FS_WspSetPduType( pdu, FS_WSP_POST );
		urlLen = IFS_Strlen( url );
		hpos = FS_WSP_HEADER_LEN / 2;
		headersLen = FS_WspEncodePostHeaders( pdu + hpos, pData );
		offset = 1;
		/* url len */
		offset += FS_UInt4ToUIntVar( pdu + offset, urlLen );
		/* headers len*/
		offset += FS_UInt4ToUIntVar( pdu + offset, headersLen );
		/* url */
		IFS_Memcpy( pdu + offset, url, urlLen );
		offset += urlLen;
		/* content-type + headers */
		IFS_Memcpy( pdu + offset, pdu + hpos, headersLen );
		offset += headersLen;
		/* data */
		if( ! pData->data_is_file )
		{
			if( pData->data && pData->data_len > 0 )
			{
				IFS_Memcpy( pdu + offset, pData->data, pData->data_len );
				offset += pData->data_len;
			}
		}

		IFS_Memset( &iData, 0, sizeof(FS_WtpInvokeData) );
		iData.ack = FS_FALSE;
		iData.host = wsp->addr.host;
		iData.port = wsp->addr.port;
		iData.xlass = FS_WTP_CLASS_2;
		iData.data = pdu;
		iData.len = offset;
		if( pData->data_is_file )
		{
			iData.file = pData->data;
			iData.size = pData->data_len;
		}
		FS_WSP_TRACE1( "FS_WspSendPostPdu pdu len = %d", offset );
		FS_WtpInvokeReq( wsp->hWtp, &iData );

		wsp->method_state = FS_WSP_MSTS_REQUESTING;
		IFS_Free( pdu );
	}
}

static void FS_WspProcessCapability( FS_WspSession *wsp, FS_BYTE *caps, FS_SINT4 len )
{
	FS_SINT4 offset = 0;
	FS_UINT4 capLen;
	FS_BYTE capsId, *capsParam;

	while( offset < len )
	{
		offset += FS_UIntVarToUInt4( &capLen, caps );
		capsId = FS_WspCapsId( caps + offset );
		capsParam = caps += offset + 1;
		
		if( capsId == FS_WCID_CLIENT_SDU )
		{
			FS_UIntVarToUInt4( &wsp->client_sdu, capsParam );
		}
		else if( capsId == FS_WCID_SERVER_SDU )
		{
			FS_UIntVarToUInt4( &wsp->server_sdu, capsParam );
		}
		offset += capLen;
	}
}

static void FS_WspProcessConnectReply( FS_WspSession *wsp, FS_BYTE *pdu, FS_SINT4 len )
{
	FS_UINT4 capLen, headLen;
	FS_SINT4 offset = 0;
	offset += FS_UIntVarToUInt4( &wsp->server_session_id, pdu );
	offset += FS_UIntVarToUInt4( &capLen, pdu + offset );
	offset += FS_UIntVarToUInt4( &headLen, pdu + offset );

	/* process negociation capability */
	if( capLen > 0 )
	{
		FS_WspProcessCapability( wsp, pdu + offset, capLen );
	}
	wsp->state = FS_WSP_STS_CONNECTED;
	/* we did not care about the headers return by ConnectReply */
	wsp->event_func( wsp->user_data, wsp, FS_WSP_EV_CONNECT_CNF, 0 );

	/* send method invoke pdu if any */
	if( wsp->method == FS_WSP_GET )
	{
		FS_WspSendGetPdu( wsp, wsp->url );
		wsp->method = 0;
		IFS_Free( wsp->url );
		wsp->url = FS_NULL;
	}
	else if( wsp->method == FS_WSP_POST )
	{
		FS_WspSendPostPdu( wsp, wsp->url, &wsp->post_data );
		wsp->method = 0;
		IFS_Free( wsp->url );
		wsp->url = FS_NULL;
		FS_SAFE_FREE( wsp->post_data.data );
		FS_SAFE_FREE( wsp->post_data.referer );
		FS_SAFE_FREE( wsp->post_data.content_type );
		IFS_Memset( &wsp->post_data, 0, sizeof(FS_WspPostDataStruct) );
	}
}

FS_SINT4 FS_WspHeaderName( FS_BYTE *hCode, FS_CHAR **hText, FS_BYTE *pdu, FS_SINT4 len )
{
	if( hCode ) *hCode = 0;
	if( hText ) *hText = FS_NULL;
	
	if( pdu[0] >= 128 )
	{
		/* integer encoding */
		if( hCode )
		{
			*hCode = pdu[0] - 0x80;
		}
		return 1;
	}
	else
	{
		/* text encoding */
		if( hText )
		{
			*hText = IFS_Strdup( pdu );
		}
		return IFS_Strlen( pdu ) + 1;
	}
}

FS_SINT4 FS_WspHeaderValue( FS_BYTE *hCode, FS_CHAR **hText, FS_SINT4 *hLen, FS_BYTE *pdu, FS_SINT4 len )
{
	FS_SINT4 rlen;
	FS_BYTE bval = pdu[0];

	if( hCode ) *hCode = 0;
	if( hLen ) *hLen = 0;
	if( hText ) *hText = FS_NULL;
	
	if( bval >= 0 && bval <= 30 )
	{
		rlen = bval + 1;	/* include the 1 byte len */
		
		if( pdu[1] > 0x80 && hCode ) *hCode = pdu[1] - 0x80;
		
		if( hText )
		{
			*hText = IFS_Malloc( bval );
			if( *hText ) IFS_Memcpy( *hText, pdu + 1, bval );
		}
		
		if( hLen ) *hLen = bval;
	}
	else if( bval == 31 )
	{
		/* uintvar field value length */
		bval = FS_UIntVarToUInt4( &rlen, pdu + 1 );
		if( pdu[bval + 1] > 0x80 && hCode ) *hCode = pdu[bval + 1] - 0x80;

		if( hText )
		{
			*hText = IFS_Malloc( rlen );
			if( *hText ) IFS_Memcpy( *hText, pdu + bval + 1, rlen );
		}

		if( hLen ) *hLen = rlen;

		rlen += (1 + bval);	/* one byte for 31 and bval */
	}
	else if( bval >= 32 && bval <= 127 )
	{
		/* The value is a text string, terminated by a zero octet (NUL character) */
		rlen = IFS_Strlen( (FS_CHAR *)pdu ) + 1;
		if( hText )
		{
			/* The Quote is not part of the contents. */
			if( bval == 127 ) pdu += 1;
			*hText = IFS_Strdup( (FS_CHAR *)pdu );
		}
	}
	else
	{
		/* It is an encoded 7-bit value; this header has no more data */
		rlen = 1;
		if( hCode ) *hCode = bval - 0x80;
	}

	return rlen;
}

static FS_SINT4 FS_WspProcessContentType( FS_WspSession *wsp, FS_BYTE *pdu, FS_SINT4 len )
{
	FS_BYTE bval = 0;
	FS_SINT4 ret;

	if( wsp->mime_content )
	{
		IFS_Free( wsp->mime_content );
		 wsp->mime_content  = FS_NULL;
	}
	ret = FS_WspHeaderValue( &bval, &wsp->mime_content, FS_NULL, pdu, len );

	wsp->content_type = bval;
	return ret;
}

static FS_SINT4 FS_WspProcessHeaders( FS_WspSession *wsp, FS_BYTE *pdu, FS_SINT4 len )
{
	FS_SINT4 offset;
	FS_BYTE	hName;
	
	offset = FS_WspProcessContentType( wsp, pdu, len );
	
	while( offset < len )
	{
		offset += FS_WspHeaderName( &hName, FS_NULL, pdu + offset, len - offset );
		if( hName == FS_WH_LOCATION )
		{
			if( wsp->location )
			{
				IFS_Free( wsp->location );
				wsp->location = FS_NULL;
			}
			offset += FS_WspHeaderValue( FS_NULL, &wsp->location, FS_NULL, pdu + offset, len - offset );
		}
		else if( hName == FS_WH_APPLICATION_ID )
		{
			if( wsp->app_id_value == FS_NULL )
			{
				offset += FS_WspHeaderValue( &wsp->app_id_code, &wsp->app_id_value, FS_NULL, pdu + offset, len - offset );
			}
		}
		/* other field. ignore it */
		else
		{
			offset += FS_WspHeaderValue( FS_NULL, FS_NULL, FS_NULL, pdu + offset, len - offset );
		}
	}

	return offset;
}

static void FS_WspProcessReply( FS_WspSession *wsp, FS_BYTE *pdu, FS_SINT4 len, FS_BOOL bDone )
{
	FS_SINT4 offset;
	FS_UINT4 headersLen;
	FS_WspResultData rData;
	
	wsp->reply_status = FS_WspReplyStatus( pdu );
	offset = 1;

	offset += FS_UIntVarToUInt4( &headersLen, pdu + offset );
	/* process content type. and we do not care about the headers in reply PDU */
	FS_WspProcessHeaders( wsp, pdu + offset, headersLen );

	if( len - offset - headersLen > 0 )
	{
		rData.data = pdu + offset + headersLen;
		rData.len = len - offset - headersLen;
	}
	else
	{
		rData.data = FS_NULL;
		rData.len = 0;
	}
	rData.done = bDone;
	rData.status = wsp->reply_status;
	rData.content_type = wsp->content_type;
	rData.mime_content = wsp->mime_content;
	rData.location = wsp->location;
	if( bDone )
		wsp->method_state = FS_WSP_MSTS_COMPLETE;
	else
		wsp->method_state = FS_WSP_MSTS_RESPONSE;
	
	wsp->event_func( wsp->user_data, wsp, FS_WSP_EV_RESULT_IND, (FS_UINT4)&rData );
}

static void FS_WspSavePostData( FS_WspSession *wsp, FS_WspPostDataStruct *postData )
{
	FS_SAFE_FREE( wsp->post_data.data );
	FS_SAFE_FREE( wsp->post_data.referer );
	FS_SAFE_FREE( wsp->post_data.content_type );
	IFS_Memset( &wsp->post_data, 0, sizeof(FS_WspPostDataStruct) );
	
	wsp->post_data.int_enc_content = postData->int_enc_content;
	if( postData->content_type )
		wsp->post_data.content_type = IFS_Strdup( postData->content_type );
	if( postData->data && postData->data_len > 0 )
	{
		wsp->post_data.data_len = postData->data_len;
		wsp->post_data.data_is_file = postData->data_is_file;
		if( postData->data_is_file )
		{
			wsp->post_data.data = IFS_Strdup( postData->data );
		}
		else
		{
			wsp->post_data.data = IFS_Malloc( postData->data_len );
			if( wsp->post_data.data )
				IFS_Memcpy( wsp->post_data.data, postData->data, postData->data_len );
		}
	}

	if( postData->referer )
		wsp->post_data.referer = IFS_Strdup( postData->referer );
}

static void FS_WtpEventHandler( void *user_data, FS_WtpTransHandle hWtp, FS_SINT4 event, FS_UINT4 param )
{
	FS_WspSession *wsp = ( FS_WspSession *)user_data;
	FS_WtpResultData *rData;
	FS_WspResultData wspData;
	FS_UINT4 sessionId;

	if( wsp != &GFS_WspSession || ! wsp->in_use )
		return;
	
	FS_WSP_TRACE2( "FS_WtpEventHandler wsp state = %d, event = %d", wsp->state, event );
	
	if( event == FS_WTP_EV_RESULT_IND )
	{
		rData = (FS_WtpResultData *)param;
		if( wsp->state == FS_WSP_STS_CONNECTING && FS_WspPduType(rData->data) == FS_WSP_CONNECT_REPLY )
		{
			FS_WspProcessConnectReply( wsp, rData->data + 1, rData->len - 1 );
		}
		else if(FS_WspPduType(rData->data) == FS_WSP_REPLY )
		{ 
			if( wsp->state == FS_WSP_STS_CONNECTED && wsp->method_state == FS_WSP_MSTS_WAITING )
			{
				FS_WspProcessReply( wsp, rData->data + 1, rData->len - 1, rData->done );
			}
			else if( wsp->state == FS_WSP_STS_RESUMING )
			{
				if( FS_WspReplyStatus( rData->data + 1 ) == FS_WSP_STATUS_OK )
				{
					wsp->state = FS_WSP_STS_CONNECTED;
					wsp->event_func( wsp->user_data, wsp, FS_WSP_EV_CONNECT_CNF, 0 );
				}
				else
				{
					wsp->state = FS_WSP_STS_NULL;
					wsp->method_state = FS_WSP_MSTS_NULL;
					wsp->event_func( wsp->user_data, wsp, FS_WSP_EV_DISCONNECT_IND, 0 );
				}
			}
		}
		else if( FS_WspPduType(rData->data) == FS_WSP_DISCONNECT )
		{
			FS_UIntVarToUInt4( &sessionId, rData->data + 1 );
			if( sessionId == wsp->server_session_id )
			{
				wsp->state = FS_WSP_STS_NULL;
				wsp->method_state = FS_WSP_MSTS_NULL;
				wsp->event_func( wsp->user_data, wsp, FS_WSP_EV_DISCONNECT_IND, 0 );
			}
		}
		else if( wsp->state == FS_WSP_STS_CONNECTED && FS_WspPduType(rData->data) == FS_WSP_SUSPEND )
		{
			FS_UIntVarToUInt4( &sessionId, rData->data + 1 );
			if( sessionId == wsp->server_session_id )
			{
				wsp->state = FS_WSP_STS_SUSPENDED;
				wsp->method_state = FS_WSP_MSTS_NULL;
				wsp->event_func( wsp->user_data, wsp, FS_WSP_EV_SUSPEND_IND, 0 );
			}
		}
	}
	else if( event == FS_WTP_EV_SEG_INVOKE_CNF )
	{
		wsp->event_func( wsp->user_data, wsp, FS_WSP_EV_INVOKE_IND, param );
	}
	else if( event == FS_WTP_EV_INVOKE_CNF )
	{
		if( wsp->method_state == FS_WSP_MSTS_REQUESTING )
			wsp->method_state = FS_WSP_MSTS_WAITING;
	}
	else if( event == FS_WTP_EV_SEG_RESULT_IND )
	{
		rData = (FS_WtpResultData *)param;
		if( wsp->method_state == FS_WSP_MSTS_RESPONSE )
		{
			wspData.content_type = wsp->content_type;
			wspData.mime_content = wsp->mime_content;
			wspData.status = wsp->reply_status;
			wspData.data = rData->data;
			wspData.len = rData->len;
			wspData.done = rData->done;
			
			wsp->event_func( wsp->user_data, wsp, FS_WSP_EV_RESULT_IND, (FS_UINT4)&wspData );
		}
	}
	else if( event == FS_WTP_EV_ABORT_IND )
	{
		wsp->state = FS_WSP_STS_NULL;
		wsp->method_state = FS_WSP_MSTS_NULL;
		wsp->event_func( wsp->user_data, wsp, FS_WSP_ERR_NET, 0 );
	}
	else if( event < 0 )
	{
		wsp->state = FS_WSP_STS_NULL;
		wsp->method_state = FS_WSP_MSTS_NULL;
		if( event == FS_WTP_ERR_NET )
		{
			FS_WtpAbortReq( wsp->hWtp, FS_WSP_ABORT_DISCONNECT );
			wsp->event_func( wsp->user_data, wsp, FS_WSP_ERR_NET, 0 );
		}
		else if( event == FS_WTP_ERR_MEMORY )
		{
			wsp->event_func( wsp->user_data, wsp, FS_WSP_ERR_MEMORY, 0 );
		}
		else
		{
			wsp->event_func( wsp->user_data, wsp, FS_WSP_ERR_UNKNOW, 0 );
		}
	}
}

FS_WspHandle FS_WspCreateHandle( FS_SockAddr *addr, void *user_data, FS_WspEventFunc handler )
{
	FS_WspSession *wsp = &GFS_WspSession;
	if( ! wsp->in_use )
	{
		IFS_Memset( wsp, 0, sizeof( FS_WspSession ) );
		wsp->in_use = FS_TRUE;
		wsp->user_data = user_data;
		wsp->event_func = handler;
		wsp->addr.port = addr->port;
		wsp->addr.host = IFS_Strdup( addr->host );
		wsp->client_sdu = FS_WSP_SDU_SIZE;
		wsp->server_sdu = FS_WSP_SDU_SIZE;
	}
	else
	{
		wsp = FS_NULL;
	}
	return wsp;
}

static void FS_WspConnectReq( FS_WspSession * wsp )
{
	if( wsp->hWtp == FS_NULL )
		wsp->hWtp = FS_WtpCreateHandle( wsp, FS_WtpEventHandler );
	FS_WspSendConnectPdu( wsp, FS_FALSE );
}

void FS_WspPushInd( FS_BYTE *pdu, FS_SINT4 len, FS_WspEventFunc evFunc )
{
	FS_WspSession wsp;
	FS_WspPushData rData;
	FS_SINT4 hlen, offset = 1;	/* skip tid */
	FS_UINT1 tid;

	tid = pdu[0];
	if( FS_WspPduType(pdu + offset) == FS_WSP_PUSH )
	{
		IFS_Memset( &rData, 0, sizeof(FS_WspPushData) );
		IFS_Memset( &wsp, 0, sizeof( FS_WspSession ) );
		offset ++;		
		/* headers len */
		offset += FS_UIntVarToUInt4( (FS_UINT4 *)(&hlen), pdu + offset );
		
		FS_WspProcessHeaders( &wsp, pdu + offset, hlen );

		rData.data = pdu + offset + hlen;
		rData.len = len - offset - hlen;
		rData.content_type = wsp.content_type;
		rData.mime_content = wsp.mime_content;
		rData.app_id_code = wsp.app_id_code;
		rData.app_id_value = wsp.app_id_value;
		rData.tid = tid;
		evFunc( FS_NULL, FS_NULL, FS_WSP_EV_PUSH_IND, (FS_UINT4)(&rData) );
		FS_SAFE_FREE( rData.mime_content );
		FS_SAFE_FREE( rData.app_id_value );
	}
	else
	{
		evFunc( FS_NULL, FS_NULL, FS_WSP_ERR_UNKNOW, FS_WspPduType(pdu + offset) );
	}
}

void FS_WspGetReq( FS_WspHandle hWsp, FS_CHAR *url )
{
	FS_WspSession *wsp = ( FS_WspSession *)hWsp;
	FS_WSP_TRACE2( "FS_WspGetReq url = %s, wsp state = %d", url, wsp->state);
	if( wsp->state == FS_WSP_STS_NULL )
	{
		wsp->method = FS_WSP_GET;
		FS_COPY_TEXT( wsp->url, url );
		FS_WspConnectReq( wsp );
	}
	else
	{
		FS_WspSendGetPdu( wsp, url );
	}
}

void FS_WspPostReq( FS_WspHandle hWsp, FS_CHAR *url, FS_WspPostDataStruct *postData )
{
	FS_WspSession *wsp = ( FS_WspSession *)hWsp;
	FS_WSP_TRACE2( "FS_WspPostReq url = %s, wsp state = %d", url, wsp->state);
	if( wsp->state == FS_WSP_STS_NULL )
	{
		wsp->method = FS_WSP_POST;
		FS_COPY_TEXT( wsp->url, url );
		FS_WspSavePostData( wsp, postData );
		FS_WspConnectReq( wsp );
	}
	else
	{
		FS_WspSendPostPdu( wsp, url, postData );
	}
}

void FS_WspAbortReq( FS_WspHandle hWsp, FS_BOOL callback )
{
	FS_WspSession *wsp = ( FS_WspSession *)hWsp;
	FS_WSP_TRACE2( "FS_WspAbortReq wsp state = %d, callback = %d", wsp->state, callback );
	FS_WtpAbortReq( wsp->hWtp, FS_WSP_ABORT_USERREQ );
	if( wsp->state != FS_WSP_STS_CONNECTED )
		wsp->state = FS_WSP_STS_NULL;
	if( callback )
		wsp->event_func( wsp->user_data, wsp, FS_WSP_ERR_USER_ABORT, 0 );
}

void FS_WspResumeReq( FS_WspHandle hWsp )
{
	FS_WspSession *wsp = ( FS_WspSession *)hWsp;
	if( wsp->state == FS_WSP_STS_SUSPENDED )
	{
		FS_WspSendConnectPdu( wsp, FS_TRUE );
	}
}

void FS_WspSuspendReq( FS_WspHandle hWsp )
{
	FS_WspSession *wsp = ( FS_WspSession *)hWsp;
	if( wsp->state == FS_WSP_STS_SUSPENDED )
	{
		wsp->event_func( wsp->user_data, wsp, FS_WSP_EV_SUSPEND_IND, 0 );
	}
	else if( wsp->server_session_id )
	{
		FS_BYTE pdu[8];
		FS_SINT4 len;
		FS_WtpInvokeData iData;
		FS_WspSetPduType( pdu, FS_WSP_SUSPEND );
		len = 1;
		len += FS_UInt4ToUIntVar( pdu + len, wsp->server_session_id );

		wsp->state = FS_WSP_STS_SUSPENDED;
		wsp->event_func( wsp->user_data, wsp, FS_WSP_EV_SUSPEND_IND, 0 );

		IFS_Memset( &iData, 0, sizeof(FS_WtpInvokeData) );
		iData.ack = FS_FALSE;
		iData.host = wsp->addr.host;
		iData.port = wsp->addr.port;
		iData.xlass = FS_WTP_CLASS_2;
		iData.data = pdu;
		iData.len = len;
		FS_WtpInvokeReq( wsp->hWtp, &iData );
	}
}

void FS_WspDestroyHandle( FS_WspHandle hWsp )
{
	FS_WspSession *wsp = ( FS_WspSession *)hWsp;

	if( wsp == &GFS_WspSession && wsp->in_use )
	{
		FS_SAFE_FREE( wsp->mime_content );
		FS_SAFE_FREE( wsp->addr.host );
		FS_SAFE_FREE( wsp->location );
		FS_SAFE_FREE( wsp->post_data.data );
		FS_SAFE_FREE( wsp->post_data.referer );
		FS_SAFE_FREE( wsp->post_data.content_type );
		FS_SAFE_FREE( wsp->url );
		
		FS_WtpDestroyHandle( wsp->hWtp );
		IFS_Memset( wsp, 0, sizeof(FS_WspSession) );
	}
}

