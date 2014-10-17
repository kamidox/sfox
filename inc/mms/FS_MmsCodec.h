#ifndef _FS_MMS_CODEC_H_
#define  _FS_MMS_CODEC_H_

#include "inc/FS_Config.h"
#include "inc/util/FS_List.h"
#include "inc/util/FS_File.h"

/* message-class value */
#define FS_MMS_H_V_CLASS_PERSONAL		0x00
#define FS_MMS_H_V_CLASS_ADDS			0x01	/* Advertisement */
#define FS_MMS_H_V_CLASS_INFO			0x02	/* Informational */
#define FS_MMS_H_V_CLASS_AUTO			0x03

/* priority field value */
#define FS_MMS_H_V_PRIORITY_LOW			0x00
#define FS_MMS_H_V_PRIORITY_NORMAL		0x01
#define FS_MMS_H_V_PRIORITY_HIGH			0x02

/* retrieve-status field value */
#define FS_MMS_H_V_RETR_STS_OK							0x00	/* other value indicate error. see [MMS-ENC] */

/* read-status field value */
#define FS_MMS_H_V_READ									0x00
#define FS_MMS_H_V_DEL_WITHOUT_READ						0x01

/* mms message type value */
#define FS_M_SEND_REQ				0x00
#define FS_M_SEND_CONF				0x01
#define FS_M_NOTIFICATION_IND		0x02
#define FS_M_NOTIFY_RESP_IND		0x03
#define FS_M_RETRIEVE_CONF			0x04
#define FS_M_ACKNOWLEDGE_IND		0x05
#define FS_M_DELIVERY_IND			0x06
#define FS_M_READ_REC_IND			0x07
#define FS_M_READ_ORIG_IND			0x08

/* mms status */
#define FS_MMS_H_V_STATUS_EXPIRED		0x00
#define FS_MMS_H_V_STATUS_RETRIEVED		0x01
#define FS_MMS_H_V_STATUS_REJECTED		0x02
#define FS_MMS_H_V_STATUS_DEFERRED		0x03
#define FS_MMS_H_V_STATUS_UNRECOGNISED	0x04
#define FS_MMS_H_V_STATUS_INDETERMINATE	0x05
#define FS_MMS_H_V_STATUS_FORWARDED		0x06
#define FS_MMS_H_V_STATUS_UNREACHABLE	0x00

/* delivery report value */
#define FS_MMS_H_V_DELIVERY_REPORT_YES		0x00
#define FS_MMS_H_V_DELIVERY_REPORT_NO		0x01

/* read report value */
#define FS_MMS_H_V_READ_REPORT_YES		0x00
#define FS_MMS_H_V_READ_REPORT_NO		0x01

/* mms codec content type */
#define FS_MMS_H_V_MULTIPART_RELATED		0x33
#define FS_MMS_H_V_MULTIPART_MIXED			0x23

#define FS_MMS_TID_LEN		16
typedef struct FS_MmsEncHead_Tag
{
	FS_UINT1			message_type;
	FS_CHAR *			tid;
	FS_UINT1			mms_version;
	FS_CHAR *			subject;
	FS_UINT4			date;
	FS_UINT4			expiry;
	FS_UINT1			delivery_report;
	FS_UINT1			read_report;
	FS_CHAR *			from;
	FS_CHAR *			to;
	FS_CHAR *			cc;
	FS_CHAR *			bcc;
	FS_UINT1			message_class;
	FS_UINT4			message_size;
	FS_CHAR *			message_id;
	FS_CHAR *			content_location;
	/* content-type field. may contain START and TYPE parameters */
	FS_CHAR *			content_type;
	FS_CHAR *			param_start;
	FS_CHAR *			param_type;
	FS_UINT2			ct_code;		/* binary value of content type. see [WSP] */
	
	FS_UINT1			priority;
	FS_UINT1			status;
	FS_UINT1			delivery_report_allow;
	FS_UINT1			retrieve_status;	/* use for retrieve mms */
	FS_UINT1			read_status;		/* use for read report */
}FS_MmsEncHead;

/* binary format multipart entry. see [WSP] section 8.5 */
typedef struct FS_MmsEncEntry_Tag
{
	FS_List				list;
	FS_CHAR *			content_type;
	FS_UINT2			ct_code;
	FS_CHAR *			param_name;
	FS_UINT1			param_charset;
	
	FS_CHAR *			content_id;
	FS_CHAR *			content_location;
	FS_SINT4			data_len;	
	FS_CHAR	*			file;
	FS_BOOL				temp_file;
}FS_MmsEncEntry;

typedef struct FS_MmsEncMultipart_Tag
{
	FS_SINT4			count;
	FS_List				entry_list;		/* here. store a FS_MmsEncEntry structure list */	
}FS_MmsEncMultipart;

typedef struct FS_MmsEncData_Tag
{
	FS_MmsEncHead		head;
	FS_MmsEncMultipart	body;
}FS_MmsEncData;

/* return mms header len. 0 on error */
FS_SINT4 FS_MmsCodecDecodeHead( FS_MmsEncHead *pHead, FS_BYTE *buf, FS_SINT4 len );

/* return FS_TRUE on success. or return FS_FALSE */
FS_BOOL FS_MmsCodecDecodeFile( FS_MmsEncData * pMms, FS_CHAR * file );

void FS_MmsCodecFreeData( FS_MmsEncData *pMms );

void FS_MmsCodecFreeHead( FS_MmsEncHead * pHead );

FS_BOOL FS_MmsCodecEncodeFile( FS_CHAR *file, FS_MmsEncData *pMms );

/* return content-id */
FS_CHAR * FS_MmsCodecCreateEntry( FS_MmsEncData *pData, FS_CHAR *file, FS_SINT4 filesize );

void FS_MmsCodecUpdateEntry( FS_MmsEncData *pData, FS_CHAR *cid, FS_CHAR *file, FS_SINT4 filesize );

void FS_MmsCodecDeleteEntry( FS_MmsEncData *pData, FS_CHAR *cid );

FS_BOOL FS_MmsCodecDecodeFileHead( FS_MmsEncHead *pHead, FS_CHAR *file );

FS_SINT4 FS_MmsCodecEncodeHead( FS_BYTE *buf, FS_MmsEncHead *pHead );

#endif
