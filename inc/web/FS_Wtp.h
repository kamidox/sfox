#include "inc/FS_Config.h"

#define FS_WTP_CLASS_0		0x00
#define FS_WTP_CLASS_1		0x01
#define FS_WTP_CLASS_2		0x02

#define FS_WTP_EV_INVOKE_CNF			1
#define FS_WTP_EV_ABORT_IND				2
#define FS_WTP_EV_RESULT_IND			3	/* param is FS_WtpResultData struct point */
#define FS_WTP_EV_SEG_RESULT_IND		4	/* param is FS_WtpResultData struct point */
#define FS_WTP_EV_SEG_INVOKE_CNF		5	/* when post data. use segment invoke. param is size sended to server */

#define FS_WTP_ERR_NET				-1
#define FS_WTP_ERR_MEMORY			-2
#define FS_WTP_ERR_UNKNOW_PDU		-3

/* wtp provider abort reason */
#define FS_WTP_ABORT_PROTOERR				0x01	/* The received PDU could not be interpreted. The structure MAY be wrong. */
#define FS_WTP_ABORT_INVALIDTID 			0x02	/* Only used by the Initiator as a negative result to the TID verification. */
#define FS_WTP_ABORT_NOTIMPLEMENTEDCL2 		0x03	/* The transaction could not be completed since the Responder does not support Class 2 transactions. */
#define FS_WTP_ABORT_NOTIMPLEMENTEDSAR		0x04	/* The transaction could not be completed since the Responder does not support SAR. */
#define FS_WTP_ABORT_NOTIMPLEMENTEDUACK		0x05	/* The transaction could not be completed since the Responder does not support User acknowledgements. */
#define FS_WTP_ABORT_WTPVERSIONZERO		 	0x06	/* Current version is 0. The initiator requested a different version that is not supported. */
#define FS_WTP_ABORT_CAPTEMPEXCEEDED 		0x07	/* Due to an overload situation the transaction can not be completed. */
#define FS_WTP_ABORT_NORESPONSE				0x08	/* A User acknowledgement was requested but the WTP user did not respond */
#define FS_WTP_ABORT_MESSAGETOOLARGE		0x09	/* Due to a message size bigger than the capabilities of the receiver the transaction cannot be completed. */

typedef void * FS_WtpTransHandle;

typedef void ( * FS_WtpEventFunc)( void *user_data, FS_WtpTransHandle hWtp, FS_SINT4 event, FS_UINT4 param );

typedef struct FS_WtpInvokeData_Tag
{
	FS_CHAR *		host;
	FS_UINT2		port;
	
	FS_BYTE		 	xlass;
	FS_BOOL			ack;
	FS_BYTE *		data;
	FS_SINT4		len;
	FS_CHAR *		file;	/* invoke application data is a file */
	FS_SINT4		size;	/* file size */
}FS_WtpInvokeData;

typedef struct FS_WtpResultData_Tag
{
	FS_BYTE *		data;
	FS_SINT4		len;
	FS_BOOL			done;
}FS_WtpResultData;

FS_WtpTransHandle FS_WtpCreateHandle( void *user_data, FS_WtpEventFunc handler );

void FS_WtpInvokeReq( FS_WtpTransHandle hWtp, FS_WtpInvokeData *iData );

void FS_WtpAbortReq( FS_WtpTransHandle hWtp, FS_BYTE reason );

void FS_WtpDestroyHandle( FS_WtpTransHandle hWtp );

