#include "inc/inte/FS_Inte.h"
#include "inc/inte/FS_ISocket.h"
#include "inc/web/FS_Wtp.h"
#include "inc/util/FS_List.h"
#include "inc/util/FS_Util.h"
#include "inc/res/FS_TimerID.h"
#include "inc/util/FS_MemDebug.h"

#define FS_WSP_PORT				8502

#define FS_WTP_PDU_T_MARK		0x78	/* 0111 1000 */
#define FS_WTP_TRAILER_MARK		0x06	/* 0000 0110 */
#define FS_WTP_CONTINUE_MARK	0x80	/* 1000 0000 */
#define FS_WTP_RID_MARK			0x01	/* 0000 0001 */
#define FS_WTP_TVE_MARK 		0x04	/* 0000 0100 */
#define FS_WTP_ABORT_T_MARK		0x07	/* 0000 0111 */
#define FS_WTP_ACK_MARK		 	0x10	/* 0001 0000 */
#define FS_WTP_TID_MARK 		0x7FFF	/* 0111 1111 */

#define FS_WTP_TPI_LEN_FLAG_MARK		0x04	/* 0000 0100 */
#define FS_WTP_TPI_LEN_MARK				0x03	/* 0000 0011 */

#define FS_WTP_INVOKE		0x01
#define FS_WTP_RESULT		0x02
#define FS_WTP_ACK			0x03
#define FS_WTP_ABORT		0x04
#define FS_WTP_SEG_INVOKE	0x05
#define FS_WTP_SEG_RESULT	0x06
#define FS_WTP_NACK			0x07

#define FS_WTP_LAST_PKG_OF_MSG		0x01
#define FS_WTP_LAST_PKG_OF_GRP		0x02
#define FS_WTP_NULL_PKG_OF_GRP		0x00

/* base on SEPC-WTP-19990611 suggest base timer interval and counter value IP */
#define FS_WTP_RCR_MAX			8		/* Re-transmission Counter */
#define FS_WTP_AEC_MAX			6		/* Acknowledgment Expiration Counter */
#define FS_WTP_INTERVAL_B_A		2000	/* base A timer interval, for Hold on acknowledgement, CLASS 2 */
#define FS_WTP_INTERVAL_S_A 	1000	/* short A timer interval, for Last acknowledgement, CLASS 1 */
#define FS_WTP_INTERVAL_L_A 	4000	/* long A timer interval, for Last acknowledgement, CLASS 2 */
#define FS_WTP_INTERVAL_B_R		5000	/* base R timer interval, for Invoke message CLASS 2 */
#define FS_WTP_INTERVAL_S_R 	4000	/* short R timer interval, for Invoke message CLASS 1 */
#define FS_WTP_INTERVAL_L_R 	7000	/* long R timer interval, for Result message, CLASS 2 */
#define FS_WTP_INTERVAL_G_R 	3000	/* package group R timer interval, for Last packet of packet group CLASS 1,2 */
#define FS_WTP_INTERVAL_W	 	40000	/* wait timer interval 40s */

#define FS_WTP_STS_NULL					0
#define FS_WTP_STS_RESULT_WAIT			1
#define FS_WTP_STS_RESULT_RESP_WAIT		2

/* abort type */
#define FS_WTP_ABORT_USER			0x01
#define FS_WTP_ABORT_PROVIDER		0x00

/* SAR param */
#define FS_NET_MTU					1460
#define FS_WTP_MTU					1400
#define FS_WTP_GROUP_PKG			3

/* WTP PDU operation macro. use to encoding a WTP PDU */
#define FS_WtpPduSetType( pdu, type )	\
	( (pdu)[0] |= (( (type) << 3 ) & FS_WTP_PDU_T_MARK) )

#define FS_WtpPduSetTrailerFlag( pdu, flag )	\
	( (pdu)[0] |= (( (flag) << 1 ) & FS_WTP_TRAILER_MARK) )

#define FS_WtpPduSetAck( pdu )	\
	( (pdu)[3] |= FS_WTP_ACK_MARK )

#define FS_WtpPduSetClass( pdu, xlass )	\
	( (pdu)[3] |= (xlass) )

#define FS_WtpPduSetRetransFlag( pdu )	\
	( (pdu)[0] |= FS_WTP_RID_MARK )

#define FS_WtpPduSetTok( pdu )	\
	( (pdu)[0] |= FS_WTP_TVE_MARK )
	
#define FS_WtpPduSetPkgSeqNumber( pdu, seq )	\
		( (pdu)[3] = (seq) )
		
#define FS_WtpPduSetNumberOfMissingPkg( pdu, num )	\
		( (pdu)[3] = (num) )
		
#define FS_WtpPduSetSeqNumberOfMissingPkg( pdu, i, seq )	\
		( (pdu)[4 + (i)] = (seq) )
		
#define FS_WtpPduSetContinueFlag( pdu )	( (pdu)[0] |= FS_WTP_CONTINUE_MARK )

#define FS_WtpPduSetAbortType( pdu, type )	( (pdu)[0] |= (type) )

#define FS_WtpPduSetAbortReason( pdu, reason )	( (pdu)[3] = (reason) )

/* PSN TPI : CON = 0x00, TPI Identity = 0x03, Length = 0x01. so 0x19 */
#define FS_WtpPduSetPsnTpi( tpi, psn )				\
	do {											\
		(tpi)[0] = 0x19;							\
		(tpi)[1] = (psn);							\
	} while( 0 )

#define FS_WtpPduType( pdu )	\
	( (FS_BYTE)((pdu[0] & FS_WTP_PDU_T_MARK) >> 3) )
	
#define FS_WtpContinueFlag( pdu )	( (pdu)[0] & FS_WTP_CONTINUE_MARK )

#define FS_WtpTve( pdu )		\
	( (FS_BYTE)( (pdu)[0] & FS_WTP_TVE_MARK ) )

#define FS_WtpAbortReason( pdu )	( (pdu)[3] )

#define FS_WtpTrailerFlag( pdu )	\
	( (FS_BYTE)(((pdu)[0] & FS_WTP_TRAILER_MARK) >> 1) )
	
#define FS_WtpNumberOfMissingPkg( pdu )	( (pdu)[3] )

#define FS_WtpSeqNumberOfMissingPkg( pdu, i ) ( (pdu)[4 + i] )

#define FS_WtpPkgSeqNumber( pdu )	( (pdu)[3] )

typedef struct FS_WtpPackage_Tag
{
	FS_List			list;
	FS_BOOL			group_flag;
	FS_BYTE			seq_number;
	FS_BYTE	*		data;
	FS_SINT4		data_len;
}FS_WtpPackage;

typedef struct FS_WtpTrans_Tag
{
	FS_UINT4		socket;
	FS_SockAddr		addr;
	FS_UINT4		timer;
	FS_BYTE			state;
	
	FS_SINT1		retrans_counter;
	FS_SINT1		ack_exprie_counter;
	
	FS_UINT2		send_tid;
	FS_UINT2		recv_tid;
	FS_BYTE			xlass;
	
	FS_BOOL			hold_on_ack;
	FS_BOOL			user_ack;
	
	FS_BYTE			pending_pdu[FS_NET_MTU];
	FS_SINT4		pdu_length;

	/* segmentation information */
	FS_BYTE			pkg_index;
	FS_BYTE			pkg_group;
	FS_BYTE *		invoke_data;
	FS_SINT4		invoke_data_len;

	/* re-assemble information */
	FS_List			msg_pkg_list;
	
	/* callback information */
	void *				user_data;
	FS_WtpEventFunc		event_func;

	FS_BOOL				in_use;
}FS_WtpTrans;

static FS_WtpTrans GFS_WtpTrans;

/* local function decleare here */
static FS_BOOL FS_WtpReSendSegInvokePkg( FS_WtpTrans *wtp, FS_BYTE seqNum, FS_BOOL bTtr );

static void FS_WtpProcessErrPdu( FS_WtpTrans *wtp );

static void FS_WtpTimerExpired( FS_WtpTrans *wtp );

static void FS_WtpSegmentInvoke( FS_WtpTrans *wtp, FS_BOOL bReTransFlag );

static FS_BOOL FS_WtpGrpPkgAllRecv( FS_WtpTrans *wtp );

static FS_BOOL FS_WtpGetGrpMissPkg( FS_WtpTrans *wtp, FS_BYTE *misPkgs, FS_BYTE *misNums );

static void FS_WtpSendAbortPdu( FS_WtpTrans *wtp, FS_BYTE abort_type, FS_BYTE reason );

static void FS_WtpSendInvokePdu( FS_WtpTrans *wtp );

static FS_UINT2 FS_WtpTid( FS_BYTE *pdu )
{
	FS_UINT2 tid = 0;
	tid = (FS_UINT2)(pdu[1] << 8);
	tid |= pdu[2];
	
	return tid & FS_WTP_TID_MARK;
}

static void FS_WtpPduSetTid( FS_BYTE * pdu, FS_UINT2 tid )
{
	tid = tid & FS_WTP_TID_MARK;
	pdu[1] = (FS_BYTE)(tid >> 8);
	pdu[2] = (FS_BYTE)tid;
}

static FS_BYTE FS_WtpTpiLength( FS_BYTE *tpi )
{
	FS_BYTE len;
	if( tpi[0] & FS_WTP_TPI_LEN_FLAG_MARK )
	{
		/* Long TPI structure */
		len = tpi[1];
	}
	else
	{
		len = tpi[0] & FS_WTP_TPI_LEN_MARK;
	}
	return len;
}

static FS_UINT2 FS_WtpGenTid( void )
{
	static FS_UINT2 s_GenTid = 0;
	FS_SINT4 secs;
	
	if( s_GenTid == 0 )
	{
		secs = FS_GetSeconds( 0 );
		s_GenTid = (FS_UINT2)(secs & FS_WTP_TID_MARK);
	}
	else
	{
		s_GenTid ++;
	}
	return s_GenTid;
}

static void FS_WtpFreeMsgPkgList( FS_WtpTrans *wtp )
{
	FS_WtpPackage *pkg;
	FS_List *node = wtp->msg_pkg_list.next;
	while( node != &wtp->msg_pkg_list )
	{
		pkg = FS_ListEntry( node, FS_WtpPackage, list );
		node = node->next;
		
		FS_ListDel( &pkg->list );
		FS_SAFE_FREE( pkg->data );
		IFS_Free( pkg );
	}
}

static void FS_WtpReportError( FS_WtpTrans *wtp, FS_SINT4 error )
{
	wtp->state = FS_WTP_STS_NULL;
	wtp->event_func( wtp->user_data, wtp, error, 0 );
}

static void FS_WtpStartTimer( FS_WtpTrans *wtp, FS_BYTE type )
{
	if( ! wtp->in_use )
		return;
	
	if( type == FS_WTP_INVOKE )
	{
		if( wtp->xlass == FS_WTP_CLASS_1 )
			wtp->timer = IFS_StartTimer( FS_TIMER_ID_WTP, FS_WTP_INTERVAL_S_R, FS_WtpTimerExpired, wtp );
		else
			wtp->timer = IFS_StartTimer( FS_TIMER_ID_WTP, FS_WTP_INTERVAL_B_R, FS_WtpTimerExpired, wtp );
	}
	else if( type == FS_WTP_SEG_RESULT )
	{
		wtp->timer = IFS_StartTimer( FS_TIMER_ID_WTP, FS_WTP_INTERVAL_G_R, FS_WtpTimerExpired, wtp );
	}
	else if( type == FS_WTP_RESULT )
	{
		wtp->timer = IFS_StartTimer( FS_TIMER_ID_WTP, FS_WTP_INTERVAL_L_A, FS_WtpTimerExpired, wtp );
	}
}

static void FS_WtpTimerExpired( FS_WtpTrans *wtp )
{
	/* may abort before timer expired */
	if( ! wtp->in_use )
		return;

	wtp->timer = 0;
	if( wtp->state == FS_WTP_STS_RESULT_WAIT )
	{
		if( wtp->retrans_counter < FS_WTP_RCR_MAX )
		{
			wtp->retrans_counter ++;
			/* segment invoke */
			if( wtp->invoke_data && wtp->pkg_index > 0 )
			{
				/* 
					If the sender has not received an acknowledgment when the re-transmission timer expires, 
					only the GTR/TTR packet is retransmitted, not the entire packet group. --- SPEC-WTP-19990611
				*/
				FS_BYTE seqNum = wtp->pkg_index + wtp->pkg_group - 1;
				FS_WtpReSendSegInvokePkg( wtp, seqNum, FS_TRUE );
				FS_WtpStartTimer( wtp, FS_WTP_INVOKE );
			}
			else
			{
				FS_WtpPduSetRetransFlag( wtp->pending_pdu );
				IFS_SocketSendTo( wtp->socket, wtp->pending_pdu, wtp->pdu_length, &wtp->addr );
				FS_WtpStartTimer( wtp, FS_WTP_INVOKE );
			}
		}
		else
		{
			FS_WtpReportError( wtp, FS_WTP_ERR_NET ); 
		}
	}
	else if( wtp->state == FS_WTP_STS_RESULT_RESP_WAIT )
	{
		if( wtp->user_ack )
		{
			if( wtp->ack_exprie_counter < FS_WTP_AEC_MAX )
			{
				wtp->ack_exprie_counter ++;
				FS_WtpStartTimer( wtp, FS_WTP_RESULT );
			}
			else
			{
				FS_WtpSendAbortPdu( wtp, FS_WTP_ABORT_PROVIDER, FS_WTP_ABORT_NORESPONSE );
			}
		}
	}
}

static FS_BYTE * FS_WtpReadNetData( FS_WtpTrans *wtp, FS_SINT4 *olen )
{
	FS_SINT4 rlen = -1;
	FS_BYTE *buf = IFS_Malloc( FS_SOCKET_BUF_LEN );
	if( buf )
	{
		rlen = IFS_SocketRecvFrom( wtp->socket, buf, FS_SOCKET_BUF_LEN, FS_NULL );
		if( rlen <= 0 )
		{
			IFS_Free( buf );
			buf = FS_NULL;
			FS_WtpReportError( wtp, FS_WTP_ERR_NET ); 
		}
	}
	else
	{
		FS_WtpReportError( wtp, FS_WTP_ERR_MEMORY );
	}
	*olen = rlen;
	return buf;
}

static void FS_WtpSendAckPdu( FS_WtpTrans *wtp, FS_BOOL bTok, FS_BYTE psnTpi )
{
	IFS_Memset( wtp->pending_pdu, 0, sizeof(wtp->pending_pdu) );
	wtp->pdu_length = 3;
	
	FS_WtpPduSetType( wtp->pending_pdu, FS_WTP_ACK );
	if( bTok )
		FS_WtpPduSetTok( wtp->pending_pdu );
	if( psnTpi > 0 )
	{
		FS_WtpPduSetContinueFlag( wtp->pending_pdu );
		FS_WtpPduSetPsnTpi( wtp->pending_pdu + 3, psnTpi );
		wtp->pdu_length += 2;
	}
	
	FS_WtpPduSetTid( wtp->pending_pdu, wtp->send_tid );
	IFS_SocketSendTo( wtp->socket, wtp->pending_pdu, wtp->pdu_length, &wtp->addr );
}

static void FS_WtpSendAbortPdu( FS_WtpTrans *wtp, FS_BYTE abort_type, FS_BYTE reason )
{
	FS_BYTE pdu[4];
	IFS_Memset( pdu, 0, 4 );
	FS_WtpPduSetType( pdu, FS_WTP_ABORT );
	FS_WtpPduSetAbortType( pdu, abort_type );
	FS_WtpPduSetTid( pdu, wtp->send_tid );
	FS_WtpPduSetAbortReason( pdu, reason );
	
	IFS_SocketSendTo( wtp->socket, pdu, 4, &wtp->addr );
}

static void FS_WtpSendNackPdu( FS_WtpTrans *wtp )
{
	FS_BYTE misPkgs[256], misNum, i;
	
	FS_WtpGetGrpMissPkg( wtp, misPkgs, &misNum );
	
	IFS_Memset( wtp->pending_pdu, 0, sizeof(wtp->pending_pdu) );
	wtp->pdu_length = 4 + misNum;
	
	FS_WtpPduSetType( wtp->pending_pdu, FS_WTP_NACK );
	FS_WtpPduSetTid( wtp->pending_pdu, wtp->send_tid );
	FS_WtpPduSetNumberOfMissingPkg( wtp->pending_pdu, misNum );
	for( i = 0; i < misNum; i ++ )
	{
		FS_WtpPduSetSeqNumberOfMissingPkg( wtp->pending_pdu, i, misPkgs[i] );
	}
	
	IFS_SocketSendTo( wtp->socket, wtp->pending_pdu, wtp->pdu_length, &wtp->addr );
}

static FS_BOOL FS_WtpAddSegPackage( FS_WtpTrans *wtp, FS_BYTE *pdu, FS_SINT4 sdu_len )
{
	FS_BOOL ret = FS_TRUE;
	FS_List *node;
	FS_BYTE tpiLen = 0;
	FS_WtpPackage *pkgPrev, *pkgNext, *pkg = FS_NEW( FS_WtpPackage );
	if( pkg )
	{
		IFS_Memset( pkg, 0, sizeof(FS_WtpPackage) );
		if( pdu == FS_NULL )
		{
			pkg->seq_number = 0;
			pkg->group_flag = FS_WTP_LAST_PKG_OF_GRP;
			pkg->data_len = sdu_len;
		}
		else
		{
			pkg->seq_number = FS_WtpPkgSeqNumber( pdu );
			pkg->group_flag = FS_WtpTrailerFlag(pdu);
			if( FS_WtpContinueFlag(pdu) )
				tpiLen = FS_WtpTpiLength( pdu + 4 );
			pkg->data_len = sdu_len - 4 - tpiLen;
			pkg->data = IFS_Malloc( pkg->data_len );
			if( pkg->data )
				IFS_Memcpy( pkg->data, pdu + 4 + tpiLen, pkg->data_len );
		}
	}
	else
	{
		FS_WtpReportError( wtp, FS_WTP_ERR_MEMORY );
		return FS_FALSE;
	}
	
	/* will add in the packet group sort by packet sequent number */
	if( FS_ListIsEmpty( &wtp->msg_pkg_list ) )
	{
		FS_ListAdd( &wtp->msg_pkg_list, &pkg->list );
	}
	else
	{
		node = wtp->msg_pkg_list.next;
		while( node != &wtp->msg_pkg_list )
		{
			pkgPrev = FS_ListEntry( node, FS_WtpPackage, list );

			/* duplicate package. ignore it. and if package is a group package. we send ack */
			if( pkg->seq_number == pkgPrev->seq_number )
			{
				FS_SAFE_FREE( pkg->data );
				IFS_Free( pkg );
				if( pkgPrev->group_flag && pkg->seq_number != 0 )
					FS_WtpSendAckPdu( wtp, FS_FALSE, pkgPrev->seq_number );
				ret = FS_FALSE;
				break;
			}
			
			if( node->next != &wtp->msg_pkg_list )
				pkgNext = FS_ListEntry( node->next, FS_WtpPackage, list );
			else
				pkgNext = FS_NULL;
			
			if( (pkgNext && (pkg->seq_number > pkgPrev->seq_number) && (pkg->seq_number < pkgNext->seq_number))
				|| (pkgNext == FS_NULL && (pkg->seq_number > pkgPrev->seq_number)) )
			{
				FS_ListAdd( node, &pkg->list );
				break;
			}
			else if( pkgNext == FS_NULL && (pkg->seq_number < pkgPrev->seq_number) )
			{
				FS_ListAddTail( node, &pkg->list );
				break;
			}

			node = node->next;
			
		}
	}
	return ret;
}

static FS_BOOL FS_WtpIsGrpMissPkg( FS_WtpTrans *wtp, FS_BYTE seqNum )
{
	FS_WtpPackage *pkg;
	FS_List *node = wtp->msg_pkg_list.next;
	while( node != &wtp->msg_pkg_list )
	{
		pkg = FS_ListEntry( node, FS_WtpPackage, list );
		node = node->next;
		
		if( pkg->seq_number == seqNum )
			return FS_FALSE;
	}
	return FS_TRUE;
}

static FS_BOOL FS_WtpGetGrpMissPkg( FS_WtpTrans *wtp, FS_BYTE *misPkgs, FS_BYTE *misNums )
{
	FS_List *node;
	FS_WtpPackage *pkg;
	FS_BYTE i, startSeqNum = 0, endSeqNum, off;
	
	*misNums = 0;
	
	node = wtp->msg_pkg_list.prev;
	pkg = FS_ListEntry( node, FS_WtpPackage, list );
	endSeqNum = pkg->seq_number;

	/* the last pkg must have group_flag. that means must be last pkg of group or last pkg or msg */
	if( !pkg->group_flag )
		return FS_FALSE;

	while( node != &wtp->msg_pkg_list )
	{
		node = node->prev;
		pkg = FS_ListEntry( node, FS_WtpPackage, list );
				
		if( pkg->group_flag )	/* reach to prev group */
		{
			startSeqNum = pkg->seq_number;
			break;
		}
	}
	
	off = 0;
	for( i = startSeqNum; i < endSeqNum; i ++ )
	{
		if( FS_WtpIsGrpMissPkg( wtp, i ) )
		{
			misPkgs[off ++] = i;
		}
	}
	*misNums = off;
	return FS_TRUE;
}

static FS_BOOL FS_WtpGrpPkgAllRecv( FS_WtpTrans *wtp )
{
	FS_List *node;
	FS_WtpPackage *pkg;
	FS_BYTE seqNum;
	
	node = wtp->msg_pkg_list.prev;
	pkg = FS_ListEntry( node, FS_WtpPackage, list );
	seqNum = pkg->seq_number;

	/* the last pkg must have group_flag. that means must be last pkg of group or last pkg or msg */
	if( !pkg->group_flag )
		return FS_FALSE;

	while( node != &wtp->msg_pkg_list )
	{
		seqNum --;
		node = node->prev;
		pkg = FS_ListEntry( node, FS_WtpPackage, list );
				
		if( pkg->seq_number != seqNum )
			return FS_FALSE;

		if( pkg->group_flag )	/* reach to prev group */
			return FS_TRUE;
	}

	return FS_TRUE;
}

/* return the pkg which has group flag */
static FS_WtpPackage * FS_WtpReAssemblePackage( FS_WtpTrans *wtp, FS_WtpResultData *rstData )
{
	FS_List *node;
	FS_WtpPackage *pkg, *grpPkg = FS_NULL;
	FS_SINT4 offset;
	FS_BOOL bFirstPkg;
	
	rstData->len = 0;
	rstData->data = FS_NULL;
	node = wtp->msg_pkg_list.prev;
	bFirstPkg = FS_TRUE;
	while( node != &wtp->msg_pkg_list )
	{
		pkg = FS_ListEntry( node, FS_WtpPackage, list );
		node = node->prev;
		if( pkg->group_flag && ! bFirstPkg )
		{
			break;
		}
		else
		{
			if( pkg->group_flag && bFirstPkg )
			{
				grpPkg = pkg;
				bFirstPkg = FS_FALSE;
			}
			rstData->len += pkg->data_len;
		}
	}

	rstData->data = IFS_Malloc( rstData->len );
	if( rstData->data )
	{
		node = wtp->msg_pkg_list.prev;
		offset = rstData->len;
		bFirstPkg = FS_TRUE;
		while( node != &wtp->msg_pkg_list )
		{
			pkg = FS_ListEntry( node, FS_WtpPackage, list );
			node = node->prev;
			if( pkg->group_flag && ! bFirstPkg )
			{
				break;
			}
			else
			{
				bFirstPkg = FS_FALSE;
				if( pkg->data )
				{
					offset -= pkg->data_len;
					IFS_Memcpy( rstData->data + offset, pkg->data, pkg->data_len );
					/* here, to save memory */
					IFS_Free( pkg->data );
					pkg->data = FS_NULL;
				}
			}
		}
	}
	return grpPkg;
}

static void FS_WtpProcessAck( FS_WtpTrans *wtp, FS_BYTE *pdu )
{
	IFS_StopTimer( wtp->timer );
	wtp->timer = 0;
	
	if( FS_WtpTve(pdu) )
	{
		wtp->retrans_counter ++;
		wtp->recv_tid = FS_WtpTid( pdu );
		FS_WtpSendAckPdu( wtp, FS_TRUE, 0 );
		FS_WtpStartTimer( wtp, FS_WTP_INVOKE );
	}
	else
	{
		/* here, we recv segmentation invoke ack */
		if( wtp->invoke_data )
		{
			if( wtp->pkg_index == 0 )
				wtp->pkg_index ++;
			else
				wtp->pkg_index += wtp->pkg_group;

			if( wtp->pkg_index < ( (wtp->invoke_data_len + FS_WTP_MTU - 1) / FS_WTP_MTU ) )
			{
				wtp->event_func( wtp->user_data, wtp, FS_WTP_EV_SEG_INVOKE_CNF, FS_WTP_MTU * wtp->pkg_index );
				/* have more package to send */
				FS_WtpSegmentInvoke( wtp, FS_FALSE );
			}
			else
			{
				/* segment package send complete */
				wtp->event_func( wtp->user_data, wtp, FS_WTP_EV_SEG_INVOKE_CNF, wtp->invoke_data_len );
				IFS_Free( wtp->invoke_data );
				wtp->invoke_data = FS_NULL;
				wtp->invoke_data_len = 0;
				wtp->event_func( wtp->user_data, wtp, FS_WTP_EV_INVOKE_CNF, 0 );
			}
		}
		else
		{
			if( wtp->xlass == FS_WTP_CLASS_2 )
				wtp->hold_on_ack = FS_TRUE;
			wtp->event_func( wtp->user_data, wtp, FS_WTP_EV_INVOKE_CNF, 0 );
		}
	}
}

static void FS_WtpProcessAbort( FS_WtpTrans *wtp, FS_BYTE *pdu )
{
	if( wtp->send_tid == FS_WtpTid( pdu ) )
	{
		/* if not this tid's abort. we ignore it */
		wtp->state = FS_WTP_STS_NULL;
		wtp->event_func( wtp->user_data, wtp, FS_WTP_EV_ABORT_IND, FS_WtpAbortReason(pdu) );
	}
}

static void FS_WtpPrepareNackPdu( FS_WtpTrans *wtp )
{
	IFS_Memset( wtp->pending_pdu, 0, sizeof(wtp->pending_pdu) );
	wtp->pdu_length = 4;
	FS_WtpPduSetType( wtp->pending_pdu, FS_WTP_NACK );
	FS_WtpPduSetTid( wtp->pending_pdu, wtp->send_tid );
	FS_WtpPduSetNumberOfMissingPkg( wtp->pending_pdu, 0 );
	FS_WtpStartTimer( wtp, FS_WTP_SEG_RESULT );
}

static void FS_WtpProcessResult( FS_WtpTrans *wtp, FS_BYTE *pdu, FS_SINT4 sdu_len )
{
	FS_SINT4 tpiLen = 0;
	FS_WtpResultData rstData;
	/* other state ignore RESULT */
	if( wtp->state == FS_WTP_STS_RESULT_WAIT && wtp->xlass == FS_WTP_CLASS_2 )
	{
		IFS_StopTimer( wtp->timer );
		wtp->timer = 0;
		
		if( ! wtp->hold_on_ack )
		{
			wtp->event_func( wtp->user_data, wtp, FS_WTP_EV_INVOKE_CNF, 0 );
		}

		if( FS_WtpContinueFlag(pdu) )
			tpiLen = FS_WtpTpiLength( pdu + 3 );
		rstData.data = pdu + 3 + tpiLen;
		rstData.len = sdu_len - 3 - tpiLen;

		/* if is last package of message. send ack */
		if( FS_WtpTrailerFlag(pdu) == FS_WTP_LAST_PKG_OF_MSG )
		{
			wtp->state = FS_WTP_STS_RESULT_RESP_WAIT;
			FS_WtpSendAckPdu( wtp, FS_FALSE, 0 );
			rstData.done = FS_TRUE;
			wtp->hold_on_ack = FS_FALSE;
			wtp->event_func( wtp->user_data, wtp, FS_WTP_EV_RESULT_IND, (FS_UINT4)&rstData );
			FS_WtpStartTimer( wtp, FS_WTP_RESULT );
		}
		else
		{
			if( FS_WtpAddSegPackage( wtp, FS_NULL, rstData.len ) )
			{
				rstData.done = FS_FALSE;
				wtp->event_func( wtp->user_data, wtp, FS_WTP_EV_RESULT_IND, (FS_UINT4)&rstData );
				FS_WtpPrepareNackPdu( wtp );
			}
		}
	}
}

static void FS_WtpProcessSegResult( FS_WtpTrans *wtp, FS_BYTE *pdu, FS_SINT4 sdu_len )
{
	FS_SINT4 tpiLen = 0;
	FS_WtpResultData rstData;
	FS_WtpPackage *grpPkg;
	/* other state ignore RESULT */
	if( wtp->state == FS_WTP_STS_RESULT_WAIT && wtp->xlass == FS_WTP_CLASS_2 )
	{
		IFS_StopTimer( wtp->timer );
		wtp->timer = 0;
		
		/* add segment package ok. means no duplicate package */
		if( FS_WtpAddSegPackage( wtp, pdu, sdu_len ) )
		{
			/* check if packages in group are all recv */
			if( FS_WtpGrpPkgAllRecv( wtp ) )
			{
				grpPkg = FS_WtpReAssemblePackage( wtp, &rstData );
				if( rstData.data != FS_NULL )
				{
					if( grpPkg->group_flag == FS_WTP_LAST_PKG_OF_GRP )
					{
						/* send group ACK pdu */
						FS_WtpSendAckPdu( wtp, FS_FALSE, grpPkg->seq_number );
						rstData.done = FS_FALSE;
						wtp->event_func( wtp->user_data, wtp, FS_WTP_EV_SEG_RESULT_IND, (FS_UINT4)&rstData );
						FS_WtpPrepareNackPdu( wtp );
					}
					else
					{
						wtp->state = FS_WTP_STS_RESULT_RESP_WAIT;
						FS_WtpSendAckPdu( wtp, FS_FALSE, grpPkg->seq_number );
						rstData.done = FS_TRUE;
						wtp->hold_on_ack = FS_FALSE;
						FS_WtpFreeMsgPkgList( wtp );
						wtp->event_func( wtp->user_data, wtp, FS_WTP_EV_SEG_RESULT_IND, (FS_UINT4)&rstData );
						FS_WtpStartTimer( wtp, FS_WTP_RESULT );
					}
					IFS_Free( rstData.data );
				}
			}
			else if( FS_WtpTrailerFlag(pdu) != FS_WTP_NULL_PKG_OF_GRP )
			{
				FS_WtpSendNackPdu( wtp );
				FS_WtpStartTimer( wtp, FS_WTP_SEG_RESULT );
			}
		}
	}
}

static FS_BOOL FS_WtpReSendSegInvokePkg( FS_WtpTrans *wtp, FS_BYTE seqNum, FS_BOOL bTtr )
{
	FS_BOOL ret = FS_TRUE;
	FS_SINT4 len;
	IFS_Memset( wtp->pending_pdu, 0, 4 );
	FS_WtpPduSetType( wtp->pending_pdu, FS_WTP_SEG_INVOKE );
	FS_WtpPduSetTid( wtp->pending_pdu, wtp->send_tid );
	FS_WtpPduSetPkgSeqNumber( wtp->pending_pdu, seqNum );
	FS_WtpPduSetRetransFlag( wtp->pending_pdu );
	
	len = FS_MIN( FS_WTP_MTU,  wtp->invoke_data_len - (FS_WTP_MTU * seqNum) );
	IFS_Memcpy( wtp->pending_pdu + 4, wtp->invoke_data + (FS_WTP_MTU * seqNum), len );

	if( bTtr )
	{
		if( wtp->invoke_data_len > (FS_WTP_MTU * (seqNum + 1)) )
		{
			FS_WtpPduSetTrailerFlag( wtp->pending_pdu, FS_WTP_LAST_PKG_OF_GRP );
		}
		else
		{
			FS_WtpPduSetTrailerFlag( wtp->pending_pdu, FS_WTP_LAST_PKG_OF_MSG );
		}
	}
	
	wtp->pdu_length = len + 4;
	/* donot raise error here */
	if( IFS_SocketSendTo( wtp->socket, wtp->pending_pdu, wtp->pdu_length, &wtp->addr ) != wtp->pdu_length )
	{
		FS_WtpReportError( wtp, FS_WTP_ERR_NET ); 
		ret = FS_FALSE;
	}
	return ret;
}

static void FS_WtpProcessNack( FS_WtpTrans *wtp, FS_BYTE *pdu )
{
	FS_BYTE i, seqNum, misNum;
	misNum = FS_WtpNumberOfMissingPkg( pdu );
	if( misNum > 0 && misNum < wtp->pkg_group )
	{
		for( i = 0; i < misNum; i ++ )
		{
			seqNum = FS_WtpSeqNumberOfMissingPkg( pdu, i );
			if( ! FS_WtpReSendSegInvokePkg( wtp, seqNum, FS_FALSE ) )
				break;
		}
	}
	else if( misNum == 0 )
	{
		FS_WtpSegmentInvoke( wtp, FS_TRUE );
	}
	else
	{
		FS_WtpProcessErrPdu( wtp );
	}
}

static void FS_WtpProcessErrPdu( FS_WtpTrans *wtp )
{
	FS_WtpSendAbortPdu( wtp, FS_WTP_ABORT_PROVIDER, FS_WTP_ABORT_PROTOERR );
	wtp->state = FS_WTP_STS_NULL;
	wtp->event_func( wtp->user_data, wtp, FS_WTP_EV_ABORT_IND, FS_WTP_ABORT_PROTOERR );
}

static void FS_WtpProcessData( FS_WtpTrans *wtp )
{
	FS_SINT4 len;
	FS_BYTE pduType;
	FS_BYTE *buf = FS_WtpReadNetData( wtp, &len );
	if( buf )
	{
		if( wtp->xlass != FS_WTP_CLASS_0 )
		{
			pduType = FS_WtpPduType( buf );
			if( pduType == FS_WTP_ACK && len >= 3 )
				FS_WtpProcessAck( wtp, buf );
			else if( pduType == FS_WTP_ABORT && len >= 4 )
				FS_WtpProcessAbort( wtp, buf );
			else if( pduType == FS_WTP_RESULT && len >= 3 )
				FS_WtpProcessResult( wtp, buf, len );
			else if( pduType == FS_WTP_SEG_RESULT && len >= 4 )
				FS_WtpProcessSegResult( wtp, buf, len );
			else if( pduType == FS_WTP_NACK && len >= 4 )
				FS_WtpProcessNack( wtp, buf );
			else
				FS_WtpProcessErrPdu( wtp );
		}
		IFS_Free( buf );
	}
}

static void FS_WtpSockEventHandler( FS_UINT4 socket, FS_ISockEvent ev, FS_UINT4 param )
{
	FS_WtpTrans *wtp = &GFS_WtpTrans;
	if( wtp->in_use )
	{
		if( ev == FS_ISOCK_READ )
		{
			FS_WtpProcessData( wtp );
		}
		else if( ev == FS_ISOCK_CONNECT )
		{
			if( wtp->invoke_data )
			{
				FS_WtpSendInvokePdu( wtp );
				FS_WtpStartTimer( wtp, FS_WTP_INVOKE );
			}
		}
		else if( ev == FS_ISOCK_ERROR )
		{
			FS_WtpReportError( wtp, FS_WTP_ERR_NET ); 
		}
	}
}

static void FS_WtpSegmentInvoke( FS_WtpTrans *wtp, FS_BOOL bReTransFlag )
{
	/* first package. use INVOKE */
	if( wtp->pkg_index == 0 )
	{
		IFS_Memset( wtp->pending_pdu, 0, 4 );
		FS_WtpPduSetType( wtp->pending_pdu, FS_WTP_INVOKE );
		FS_WtpPduSetTrailerFlag( wtp->pending_pdu, FS_WTP_LAST_PKG_OF_GRP );
		FS_WtpPduSetTid( wtp->pending_pdu, wtp->send_tid );
		FS_WtpPduSetAck( wtp->pending_pdu );
		FS_WtpPduSetClass( wtp->pending_pdu, wtp->xlass );
		if( bReTransFlag )
			FS_WtpPduSetRetransFlag( wtp->pending_pdu );
		
		IFS_Memcpy( wtp->pending_pdu + 4, wtp->invoke_data, FS_WTP_MTU );
		
		wtp->pdu_length = FS_WTP_MTU + 4;
		if( IFS_SocketSendTo( wtp->socket, wtp->pending_pdu, wtp->pdu_length, &wtp->addr ) != wtp->pdu_length )
		{
			FS_WtpReportError( wtp, FS_WTP_ERR_NET ); 
		}
	}
	else
	{
		FS_SINT4 i, len;
		FS_BOOL bDone = FS_FALSE;
		for( i = 0; i < wtp->pkg_group; i ++ )
		{
			IFS_Memset( wtp->pending_pdu, 0, 4 );
			FS_WtpPduSetType( wtp->pending_pdu, FS_WTP_SEG_INVOKE );
			FS_WtpPduSetTid( wtp->pending_pdu, wtp->send_tid );
			FS_WtpPduSetPkgSeqNumber( wtp->pending_pdu, wtp->pkg_index + i );
			if( bReTransFlag )
				FS_WtpPduSetRetransFlag( wtp->pending_pdu );

			len = FS_MIN( FS_WTP_MTU,  wtp->invoke_data_len - (FS_WTP_MTU * (wtp->pkg_index + i)) );
			IFS_Memcpy( wtp->pending_pdu + 4, wtp->invoke_data + (FS_WTP_MTU * (wtp->pkg_index + i)), len );
			
			if( i == (wtp->pkg_group - 1) && wtp->invoke_data_len > (FS_WTP_MTU * (wtp->pkg_index + i + 1)) )
			{
				FS_WtpPduSetTrailerFlag( wtp->pending_pdu, FS_WTP_LAST_PKG_OF_GRP );
				bDone = FS_TRUE;
			}
			else if( wtp->invoke_data_len <= (FS_WTP_MTU * (wtp->pkg_index + i + 1)) )
			{
				FS_WtpPduSetTrailerFlag( wtp->pending_pdu, FS_WTP_LAST_PKG_OF_MSG );
				bDone = FS_TRUE;
			}
			
			wtp->pdu_length = len + 4;
			if( IFS_SocketSendTo( wtp->socket, wtp->pending_pdu, wtp->pdu_length, &wtp->addr ) != wtp->pdu_length )
			{
				FS_WtpReportError( wtp, FS_WTP_ERR_NET ); 
				break;
			}

			if( bDone ) break;
			
		}		
	}
}

static void FS_WtpSendInvokePdu( FS_WtpTrans *wtp )
{
	/* need segmentation */
	if( wtp->invoke_data_len > FS_WTP_MTU )
	{
		wtp->pkg_index = 0;
		wtp->pkg_group = FS_WTP_GROUP_PKG;
		FS_WtpSegmentInvoke( wtp, FS_FALSE );
	}
	else
	{
		IFS_Memset( wtp->pending_pdu, 0, 4 );
		FS_WtpPduSetType( wtp->pending_pdu, FS_WTP_INVOKE );
		FS_WtpPduSetTrailerFlag( wtp->pending_pdu, FS_WTP_LAST_PKG_OF_MSG );
		FS_WtpPduSetTid( wtp->pending_pdu, wtp->send_tid );
		FS_WtpPduSetAck( wtp->pending_pdu );
		FS_WtpPduSetClass( wtp->pending_pdu, wtp->xlass );

		if( wtp->invoke_data )
			IFS_Memcpy( wtp->pending_pdu + 4, wtp->invoke_data, wtp->invoke_data_len );

		wtp->pdu_length = wtp->invoke_data_len + 4;
		FS_SAFE_FREE( wtp->invoke_data );
		wtp->invoke_data = FS_NULL;
		wtp->invoke_data_len = 0;
		if( IFS_SocketSendTo( wtp->socket, wtp->pending_pdu, wtp->pdu_length, &wtp->addr ) != wtp->pdu_length )
		{
			FS_WtpReportError( wtp, FS_WTP_ERR_NET ); 
		}
	}
}

FS_WtpTransHandle FS_WtpCreateHandle( void *user_data, FS_WtpEventFunc handler )
{
	FS_WtpTrans *wtp = &GFS_WtpTrans;
	if( ! wtp->in_use )
	{
		IFS_Memset( wtp, 0, sizeof(FS_WtpTrans) );
		wtp->in_use = FS_TRUE;
		wtp->user_data = user_data;
		wtp->event_func = handler;
		FS_ListInit( &wtp->msg_pkg_list );
	}
	return wtp;
}

void FS_WtpInvokeReq( FS_WtpTransHandle hWtp, FS_WtpInvokeData *iData )
{
	FS_WtpTrans *wtp = ( FS_WtpTrans * )hWtp;

	wtp->send_tid = FS_WtpGenTid( );
	wtp->retrans_counter = 0;
	wtp->ack_exprie_counter = 0;
	wtp->user_ack = iData->ack;
	wtp->xlass = iData->xlass;
	if( iData->xlass != FS_WTP_CLASS_0 )
		wtp->state = FS_WTP_STS_RESULT_WAIT;

	if( wtp->addr.host )
		IFS_Free( wtp->addr.host );

	wtp->addr.host = IFS_Strdup( iData->host );
	wtp->addr.port = iData->port;
	if( wtp->timer )
	{
		IFS_StopTimer( wtp->timer );
		wtp->timer = 0;
	}
	
	if( wtp->invoke_data )
	{
		IFS_Free( wtp->invoke_data );
		wtp->invoke_data = FS_NULL;
		wtp->invoke_data_len = 0;
	}
	
	/* copy invoke data */
	if( iData->file )
		wtp->invoke_data = IFS_Malloc( iData->len + iData->size );
	else
		wtp->invoke_data = IFS_Malloc( iData->len );
	if( wtp->invoke_data )
	{
		IFS_Memcpy( wtp->invoke_data, iData->data, iData->len );
		wtp->invoke_data_len = iData->len;
		if( iData->file )
		{
			FS_FileRead( -1, iData->file, 0, wtp->invoke_data + iData->len, iData->size );
			wtp->invoke_data_len += iData->size;
		}
		if( wtp->socket == 0 )
		{
			/* UDP do not need to connect. but this operate play some extra init for socket */
			if( ! IFS_SocketCreate( &wtp->socket, FS_FALSE, FS_WtpSockEventHandler ) 
				|| ! IFS_SocketConnect( wtp->socket, FS_NULL, FS_WSP_PORT ) )
			{
				FS_WtpReportError( wtp, FS_WTP_ERR_NET ); 
			}
		}
		else
		{
			FS_WtpSendInvokePdu( wtp );
			FS_WtpStartTimer( wtp, FS_WTP_INVOKE );
		}
	}
	else
	{
		FS_WtpReportError( wtp, FS_WTP_ERR_MEMORY );
	}
}

void FS_WtpAbortReq( FS_WtpTransHandle hWtp, FS_BYTE reason )
{
	FS_WtpTrans *wtp = ( FS_WtpTrans * )hWtp;
	if( wtp->state != FS_WTP_STS_NULL && wtp->timer )
	{
		IFS_StopTimer( wtp->timer );
		wtp->timer = 0;
	}
	wtp->state = FS_WTP_STS_NULL;
	FS_WtpFreeMsgPkgList( wtp );
	if( wtp->invoke_data )
	{
		IFS_Free( wtp->invoke_data );
		wtp->invoke_data = FS_NULL;
		wtp->invoke_data_len = 0;
	}
	FS_WtpSendAbortPdu( wtp, FS_WTP_ABORT_USER, reason );
}

void FS_WtpDestroyHandle( FS_WtpTransHandle hWtp )
{
	FS_WtpTrans *wtp = ( FS_WtpTrans * )hWtp;
	if( wtp == &GFS_WtpTrans && wtp->in_use )
	{
		if( wtp->socket ) IFS_SocketClose( wtp->socket );
		FS_SAFE_FREE( wtp->addr.host );
		if( wtp->timer )
		{
			IFS_StopTimer( wtp->timer );
			wtp->timer = 0;
		}
		
		if( wtp->invoke_data )
		{
			IFS_Free( wtp->invoke_data );
			wtp->invoke_data = FS_NULL;
			wtp->invoke_data_len = 0;
		}

		IFS_Memset( wtp, 0, sizeof(FS_WtpTrans) );
	}
}

