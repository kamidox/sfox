#include "inc/FS_Config.h"

#ifdef FS_MODULE_DCD

#include "inc/inte/FS_Inte.h"
#include "inc/web/FS_Http.h"
#include "inc/dcd/FS_DcdLib.h"
#include "inc/dcd/FS_DcdPkg.h"
#include "inc/res/FS_TimerID.h"
#include "inc/util/FS_Util.h"
#include "inc/util/FS_File.h"
#include "inc/util/FS_NetConn.h"
#include "inc/util/FS_MemDebug.h"

//#define FS_DCD_TMP_FILE_DEBUG_
#define _DCD_DEBUG

#ifdef _DCD_DEBUG
#define FS_DCD_TRACE0(a)				FS_TRACE0( "[DCD]" a "\r\n")
#define FS_DCD_TRACE1(a,b)				FS_TRACE1( "[DCD]" a "\r\n", b)
#define FS_DCD_TRACE2(a,b,c)			FS_TRACE2( "[DCD]" a "\r\n", b, c)
#define FS_DCD_TRACE3(a,b,c,d)			FS_TRACE3( "[DCD]" a "\r\n", b, c, d)
#else
#define FS_DCD_TRACE0(a)
#define FS_DCD_TRACE1(a,b)
#define FS_DCD_TRACE2(a,b,c)
#define FS_DCD_TRACE3(a,b,c,d)
#endif

#define FS_DCD_STATE_IDLE			0	/* idle state */
#define FS_DCD_STATE_SYNC			1	/* sending sync request */
#define FS_DCD_STATE_DIMG			2	/* downlonad image etc. */

#define FS_DCD_IDLE_STATE_NORMAL	0
#define FS_DCD_IDLE_STATE_PAUSE		1

#define FS_DCD_SYNC_REQ_ID_CNT		5
#define FS_DCD_SYNC_REQ_ID_LEN		(FS_DCD_SYNC_REQ_ID_CNT * 32)
#define FS_DCD_NET_HEADER_LEN		2048
#define FS_DCD_SYNC_ANCHOR_LEN		1024
#define FS_DCD_FIRST_UPDATE_DELAY	120000	/* 120s */
#define FS_DCD_FEED_ENTRY_CNT		10
#define FS_DCD_PKG_FILE_HEAD_LEN	1024
#define FS_DCD_CHANNEL_BUF_LEN		256
#define FS_DCD_CHANNEL_FEED_BUF_LEN	20480

#define FS_DCD_PKG_FILE				"DcdPkg.bin"
#define FS_DCD_CFG_FILE				"DcdConfig.bin"
#define FS_DCD_SNAP_FILE			"DcdSnapshot.bin"

#define FS_DCD_NET_GATWAY		"10.0.0.172"
#define FS_DCD_DEFAULT_URL		"http://dcd.monternet.com/content/refresh/dp"

typedef struct FS_DcdSession_Tag
{
	FS_DcdSyncPkg *		pkg;			/* current pkg data in use */
	FS_UINT4			ttl_timer;
	FS_UINT4			client_time;	/* last update client time. use for handle power off ttl */
	
	FS_DcdChannelFeed *	idle_feed;
	FS_DcdEntry *		idle_entry;
	FS_UINT4			idle_timer;
	FS_SINT4			idle_state;

	FS_List 			snapshot_list;
	
	FS_SINT4			state;
	FS_DCD_REQ_TYPE		sync_type;
	FS_HttpHandle		hHttp;
	FS_CHAR *			host;
	FS_CHAR *			next_url;

	FS_CHAR				file[FS_FILE_NAME_LEN];
	FS_SINT4			content_len;
	FS_SINT4			offset;
	FS_CHAR *			boundary;
	
	FS_DcdSyncPkg *		new_pkg;		/* new pkg data in update */
}FS_DcdSession;

static FS_DcdSession *GFS_DcdSession;
static FS_DcdConfig GFS_DcdConfig;

static void FS_DcdDownloadContent( FS_DcdSession *dcdSes );
static void FS_DcdSaveContent( FS_DcdSession *dcdSes, FS_List *rspEntryList );
static FS_CHAR *FS_DcdGetRequestIds( FS_DcdSession *dcdSes );
static void FS_DcdIdleInit( FS_DcdSession *dcdSes );
static void FS_DcdIdleTimerCallBack( FS_DcdSession *dcdSes );
static void FS_DcdConfigInit( void );
static void FS_DcdFreeSnapshotList( FS_DcdSession *dcdSes, FS_BOOL del_file );
static void FS_DcdReadSnapshotList( FS_DcdSession *dcdSes );

void FS_DcdUICtxUpdate( FS_BOOL ret_to_list );
void FS_DcdUICtxUpdateFinish( void );

static FS_UINT4 FS_DcdGetIdleTime( void )
{
	if( GFS_DcdConfig.idle_speed == FS_DCD_IDLE_SLW ){
		return 10000;
	}else if( GFS_DcdConfig.idle_speed == FS_DCD_IDLE_MID ){
		return 5000;
	}else{
		return 3000;
	}
}

static void FS_DcdTtlTimerCallback( FS_DcdSession *dcdSes )
{
	FS_ASSERT( dcdSes == GFS_DcdSession );
	dcdSes->ttl_timer = 0;
	if( GFS_DcdConfig.net_mode == FS_DCD_NET_ALWAYS_OFF ) return;
	if( dcdSes->state != FS_DCD_STATE_IDLE ){
		FS_DCD_TRACE1( "FS_DcdTtlTimerCallback state error. ttl timeout. state = %d", dcdSes->state );
		return;
	}
	
	FS_DcdUpdate( FS_DCD_REQ_TTL );
}

static FS_BOOL FS_DcdIsUpdateFinish( FS_DcdSession *dcdSes )
{
	/* if there was no any empty image to download. we say it update finished. */
	FS_CHAR *request_id;
	
	request_id = FS_DcdGetRequestIds( dcdSes );
	if( request_id ){
		IFS_Free( request_id );
		return FS_FALSE;
	}else{
		return FS_TRUE;
	}
}

static FS_DcdChannelFeed *FS_DcdFindChannelFeedByCid( FS_DcdSyncPkg *pkg, FS_CHAR *cid )
{
	FS_DcdChannelFeed *feed;
	FS_List *node;

	node = pkg->feed_list.next;
	while( node != &pkg->feed_list ){
		feed = FS_ListEntry( node, FS_DcdChannelFeed, list );
		node = node->next;
		if( FS_STR_I_EQUAL(feed->id, cid) ){
			return feed;
		}
	}
	return FS_NULL;
}

/* return the empty entry, and the feed channel this entry belong to */
static FS_DcdEntry *FS_DcdFindEmptyFeedEntry( FS_DcdSyncPkg *pkg, FS_DcdChannelFeed **feed_belong_to )
{
	FS_List *feed_node, *entry_node;
	FS_DcdChannelFeed *feed;
	FS_DcdEntry *entry;
		
	feed_node = pkg->feed_list.next;
	while( feed_node != &pkg->feed_list ){
		feed = FS_ListEntry( feed_node, FS_DcdChannelFeed, list );
		feed_node = feed_node->next;

		entry_node = feed->entry_list.next;
		while( entry_node != &feed->entry_list ){
			entry = FS_ListEntry( entry_node, FS_DcdEntry, list );
			entry_node = entry_node->next;

			if( entry->id && entry->title == FS_NULL && entry->summary == FS_NULL 
				&& FS_ListIsEmpty(&entry->content_list) && FS_ListIsEmpty(&entry->link_list) ){
				if( feed_belong_to ){
					*feed_belong_to = feed;
				}
				return entry;
			}
		}
	}
	return FS_NULL;
}

/* return the match entry, and the feed channel this entry belong to */
static FS_DcdEntry *FS_DcdFindFeedEntryByCid( FS_DcdSyncPkg *pkg, FS_CHAR *cid, FS_DcdChannelFeed **feed_belong_to )
{
	FS_List *feed_node, *entry_node;
	FS_DcdChannelFeed *feed;
	FS_DcdEntry *entry;
		
	feed_node = pkg->feed_list.next;
	while( feed_node != &pkg->feed_list ){
		feed = FS_ListEntry( feed_node, FS_DcdChannelFeed, list );
		feed_node = feed_node->next;

		entry_node = feed->entry_list.next;
		while( entry_node != &feed->entry_list ){
			entry = FS_ListEntry( entry_node, FS_DcdEntry, list );
			entry_node = entry_node->next;

			if( FS_STR_I_EQUAL(entry->id, cid)){
				if( feed_belong_to ){
					*feed_belong_to = feed;
				}
				return entry;
			}
		}
	}
	return FS_NULL;
}

static FS_SINT4 FS_DcdLinkListFromBuf( FS_List *head, FS_BYTE *buf )
{
	FS_SINT4 offset = 0, i, cnt;
	FS_DcdLink *link;
	
	cnt = FS_LE_BYTE_TO_UINT4( buf );
	offset += 4;
	for( i = 0; i < cnt; i ++ ){
		link = IFS_Malloc( sizeof(FS_DcdLink) );
		FS_ASSERT( link != FS_NULL );
		IFS_Memset( link, 0, sizeof(FS_DcdLink) );

		if(buf[offset]){
			link->title = IFS_Strdup( buf + offset );
			offset += IFS_Strlen( link->title );
		}
		offset += 1;
		if(buf[offset]){
			link->href = IFS_Strdup( buf + offset );
			offset += IFS_Strlen( link->href ) ;
		}
		offset += 1;
		
		FS_ListAddTail( head, &link->list );
	}
	return offset;
}

static FS_SINT4 FS_DcdLinkListToBuf( FS_List *head, FS_BYTE *buf )
{
	FS_List *node;
	FS_DcdLink *link;
	FS_SINT4 offset = 0, cnt;

	cnt = FS_ListCount( head );
	FS_UINT4_TO_LE_BYTE( cnt, buf );
	offset += 4;
	FS_ListForEach( node, head ){
		link = FS_ListEntry( node, FS_DcdLink, list );
		
		IFS_Strcpy( buf + offset, link->title ? link->title : "" );
		offset += IFS_Strlen( link->title ? link->title : "" ) + 1;
		IFS_Strcpy( buf + offset, link->href ? link->href : "" );
		offset += IFS_Strlen( link->href ? link->href : "" ) + 1;
	}
	return offset;
}

static FS_SINT4 FS_DcdContentListFromBuf( FS_List *head, FS_BYTE *buf )
{
	FS_SINT4 offset = 0, i, cnt;
	FS_DcdContent *content;
	
	cnt = FS_LE_BYTE_TO_UINT4( buf );
	offset += 4;
	for( i = 0; i < cnt; i ++ ){
		content = IFS_Malloc( sizeof(FS_DcdContent) );
		FS_ASSERT( content != FS_NULL );
		IFS_Memset( content, 0, sizeof(FS_DcdContent) );

		if(buf[offset]){
			content->id = IFS_Strdup( buf + offset );
			offset += IFS_Strlen( content->id );
		}
		offset += 1;
		if(buf[offset]){
			content->type = IFS_Strdup( buf + offset );
			offset += IFS_Strlen( content->type );
		}
		offset += 1;
		if(buf[offset]){
			content->href = IFS_Strdup( buf + offset );
			offset += IFS_Strlen( content->href );
		}
		offset += 1;
		if(buf[offset]){
			content->file = IFS_Strdup( buf + offset );
			offset += IFS_Strlen( content->file );
		}
		offset += 1;
		FS_ListAddTail( head, &content->list );
	}
	return offset;
}

static FS_SINT4 FS_DcdContentListToBuf( FS_List *head, FS_BYTE *buf )
{
	FS_List *node;
	FS_DcdContent *content;
	FS_SINT4 offset = 0, cnt;

	cnt = FS_ListCount( head );
	FS_UINT4_TO_LE_BYTE( cnt, buf );
	offset += 4;
	FS_ListForEach( node, head ){
		content = FS_ListEntry( node, FS_DcdContent, list );

		IFS_Strcpy( buf + offset, content->id ? content->id : "" );
		offset += IFS_Strlen( content->id ? content->id : "" ) + 1;
		IFS_Strcpy( buf + offset, content->type ? content->type : "" );
		offset += IFS_Strlen( content->type ? content->type : "" ) + 1;
		IFS_Strcpy( buf + offset, content->href ? content->href : "" );
		offset += IFS_Strlen( content->href ? content->href : "" ) + 1;
		IFS_Strcpy( buf + offset, content->file ? content->file : "" );
		offset += IFS_Strlen( content->file ? content->file : "" ) + 1;		
	}
	return offset;
}

static FS_SINT4 FS_DcdEntryListFromBuf( FS_List *head, FS_BYTE *buf )
{
	FS_SINT4 offset = 0, i, cnt;
	FS_DcdEntry *entry;
	
	cnt = FS_LE_BYTE_TO_UINT4( buf );
	offset += 4;
	for( i = 0; i < cnt; i ++ ){
		entry = IFS_Malloc( sizeof(FS_DcdEntry) );
		FS_ASSERT( entry != FS_NULL );
		IFS_Memset( entry, 0, sizeof(FS_DcdEntry) );
		FS_ListInit( &entry->link_list );
		FS_ListInit( &entry->content_list );

		if(buf[offset]){
			entry->id = IFS_Strdup( buf + offset );
			offset += IFS_Strlen( entry->id );
		}
		offset += 1;
		if(buf[offset]){
			entry->title = IFS_Strdup( buf + offset );
			offset += IFS_Strlen( entry->title );
		}
		offset += 1;
		if(buf[offset]){
			entry->summary = IFS_Strdup( buf + offset );
			offset += IFS_Strlen( entry->summary );
		}
		offset += 1;
		entry->issue = FS_LE_BYTE_TO_UINT4( buf + offset );
		offset += 4;
		if(buf[offset]){
			entry->author = IFS_Strdup( buf + offset );
			offset += IFS_Strlen( entry->author );
		}
		offset += 1;
		entry->flag = FS_LE_BYTE_TO_UINT4( buf + offset );
		offset += 4;
		offset += FS_DcdLinkListFromBuf( &entry->link_list, buf + offset );
		offset += FS_DcdContentListFromBuf( &entry->content_list, buf + offset );
		
		FS_ListAddTail( head, &entry->list );
	}
	return offset;
}

static FS_SINT4 FS_DcdEntryListToBuf( FS_List *head, FS_BYTE *buf )
{
	FS_List *node;
	FS_DcdEntry *entry;
	FS_SINT4 offset = 0, cnt;

	cnt = FS_ListCount( head );
	FS_UINT4_TO_LE_BYTE( cnt, buf );
	offset += 4;
	FS_ListForEach( node, head ){
		entry = FS_ListEntry( node, FS_DcdEntry, list );

		IFS_Strcpy( buf + offset, entry->id ? entry->id : "" );
		offset += IFS_Strlen( entry->id ? entry->id : "" ) + 1;
		IFS_Strcpy( buf + offset, entry->title ? entry->title : "" );
		offset += IFS_Strlen( entry->title ? entry->title : "" ) + 1;
		IFS_Strcpy( buf + offset, entry->summary ? entry->summary : "" );
		offset += IFS_Strlen( entry->summary ? entry->summary : "" ) + 1;
		FS_UINT4_TO_LE_BYTE( entry->issue, buf + offset );
		offset += 4;
		IFS_Strcpy( buf + offset, entry->author ? entry->author : "" );
		offset += IFS_Strlen( entry->author ? entry->author : "" ) + 1;
		FS_UINT4_TO_LE_BYTE( entry->flag, buf + offset );
		offset += 4;
		offset += FS_DcdLinkListToBuf( &entry->link_list, buf + offset );
		offset += FS_DcdContentListToBuf( &entry->content_list, buf + offset );
	}
	return offset;
}

static void FS_DcdSaveSyncPkg( FS_DcdSession *dcdSes )
{
	FS_List *node;
	FS_DcdChannel *channel;
	FS_DcdChannelFeed *feed;
	FS_DcdSyncPkg *pkg = dcdSes->pkg;
	FS_BYTE *buf;
	FS_SINT4 offset, len, cnt, f_off = 0;
	
	if( pkg == FS_NULL ) return;

	buf = IFS_Malloc( FS_DCD_PKG_FILE_HEAD_LEN );
	FS_ASSERT( buf != FS_NULL );
	IFS_Memset( buf, 0, FS_DCD_PKG_FILE_HEAD_LEN );
	offset = 0;
	/* save dcd file head. global_anchor, ttl, server_time */
	IFS_Memcpy( buf, "DCDF", 4 );
	offset += 4;
	IFS_Strcpy( buf + offset, pkg->global_anchor ? pkg->global_anchor : "" );
	offset += IFS_Strlen( pkg->global_anchor ? pkg->global_anchor : "" )+ 1;
	FS_UINT4_TO_LE_BYTE( pkg->ttl, buf + offset );
	offset += 4;
	FS_UINT4_TO_LE_BYTE( pkg->server_time, buf + offset );
	offset += 4;
	/* client time, host, next_url */
	FS_UINT4_TO_LE_BYTE( dcdSes->client_time, buf + offset );
	offset += 4;
	IFS_Strcpy( buf + offset, dcdSes->host ? dcdSes->host : "" );
	offset += IFS_Strlen( dcdSes->host ? dcdSes->host : "" )+ 1;
	IFS_Strcpy( buf + offset, dcdSes->next_url? dcdSes->next_url : "" );
	offset += IFS_Strlen( dcdSes->next_url ? dcdSes->next_url : "" )+ 1;
	f_off += FS_FileWrite( FS_DIR_DCD, FS_DCD_PKG_FILE, f_off, buf, offset );
	IFS_Free( buf );
	
	/* save dcd channel list */
	node = pkg->channel_list.next;
	cnt = FS_ListCount( &pkg->channel_list );
	if( cnt == 0 ) return;
	len = cnt * FS_DCD_CHANNEL_BUF_LEN;
	buf = IFS_Malloc( len );
	FS_ASSERT( buf != FS_NULL );
	IFS_Memset( buf, 0, len );
	offset = 0;
	FS_UINT4_TO_LE_BYTE( cnt, buf );	/* channel count */
	offset += 4;
	while( node != &pkg->channel_list ){
		channel = FS_ListEntry( node, FS_DcdChannel, list );
		node = node->next;

		IFS_Strcpy( buf + offset, channel->id ? channel->id : "" );
		offset += IFS_Strlen( channel->id ? channel->id : "" ) + 1;
		IFS_Strcpy( buf + offset, channel->anchor ? channel->anchor : "" );
		offset += IFS_Strlen( channel->anchor ? channel->anchor : "" ) + 1;
		FS_UINT4_TO_LE_BYTE( channel->charge, buf + offset );
		offset += 4;
	}
	FS_ASSERT( offset < len );
	f_off += FS_FileWrite( FS_DIR_DCD, FS_DCD_PKG_FILE, f_off, buf, offset );
	IFS_Free( buf );
	/* save dcd channel feed list */
	node = pkg->feed_list.next;
	len = FS_DCD_CHANNEL_FEED_BUF_LEN;
	buf = IFS_Malloc( len );
	FS_ASSERT( buf != FS_NULL );
	IFS_Memset( buf, 0, len );
	offset = 0;
	while( node != &pkg->feed_list ){
		feed = FS_ListEntry( node, FS_DcdChannelFeed, list );
		node = node->next;
		
		IFS_Strcpy( buf + offset, feed->id ? feed->id : "" );
		offset += IFS_Strlen( feed->id ? feed->id : "" ) + 1;
		IFS_Strcpy( buf + offset, feed->title ? feed->title : "" );
		offset += IFS_Strlen( feed->title ? feed->title : "" ) + 1;
		IFS_Strcpy( buf + offset, feed->summary ? feed->summary : "" );
		offset += IFS_Strlen( feed->summary ? feed->summary : "" ) + 1;
		
		offset += FS_DcdLinkListToBuf( &feed->link_list, buf + offset );
		offset += FS_DcdContentListToBuf( &feed->content_list, buf + offset );
		offset += FS_DcdEntryListToBuf( &feed->entry_list, buf + offset );
	}
	FS_ASSERT( offset < len );
	f_off += FS_FileWrite( FS_DIR_DCD, FS_DCD_PKG_FILE, f_off, buf, offset );
	IFS_Free( buf );
}

static void FS_DcdReadSyncPkg( FS_DcdSession *dcdSes )
{
	FS_SINT4 len, i, cnt, offset = 0;
	FS_BYTE *buf;
	FS_DcdSyncPkg *pkg;
	FS_DcdChannel *channel;
	FS_DcdChannelFeed *feed;
	
	len = FS_FileGetSize( FS_DIR_DCD, FS_DCD_PKG_FILE );
	if( len <= 0 ) return;

	buf = IFS_Malloc( len + 1 );
	FS_ASSERT( buf );
	if( buf == FS_NULL ) return;
	FS_FileRead( FS_DIR_DCD, FS_DCD_PKG_FILE, 0, buf, len );
	buf[len] = 0;

	if( dcdSes->pkg ){
		FS_DcdFreeSyncPkg( dcdSes->pkg, FS_FALSE );
		dcdSes->pkg = FS_NULL;
	}
	pkg = IFS_Malloc( sizeof(FS_DcdSyncPkg) );
	FS_ASSERT( pkg );
	IFS_Memset( pkg, 0, sizeof(FS_DcdSyncPkg) );
	FS_ListInit( &pkg->channel_list );
	FS_ListInit( &pkg->feed_list );	
	/* read dcd pkg file head, global_anchor, ttl, server_time */
	if( IFS_Memcmp( buf, "DCDF", 4 ) != 0 ){
		goto READ_DONE;
	}
	offset += 4;
	if( buf[offset] ){
		pkg->global_anchor = IFS_Strdup( buf + offset );
		offset += IFS_Strlen( pkg->global_anchor );
	}
	offset += 1;
	pkg->ttl = FS_LE_BYTE_TO_UINT4( buf + offset );
	offset += 4;
	pkg->server_time = FS_LE_BYTE_TO_UINT4( buf + offset );
	offset += 4;
	/* client_time, host, next_url */
	dcdSes->client_time = FS_LE_BYTE_TO_UINT4( buf + offset );
	offset += 4;
	if( buf[offset] ){
		dcdSes->host = IFS_Strdup( buf + offset );
		offset += IFS_Strlen( dcdSes->host );
	}
	offset += 1;
	if( buf[offset] ){
		dcdSes->next_url = IFS_Strdup( buf + offset );
		offset += IFS_Strlen( dcdSes->next_url );
	}
	offset += 1;
	
	if( offset >= len ){
		goto READ_DONE;
	}
	/* read dcd channel list */
	cnt = FS_LE_BYTE_TO_UINT4( buf + offset );
	offset += 4;
	for( i = 0; i < cnt; i ++ ){
		channel = IFS_Malloc( sizeof(FS_DcdChannel) );
		FS_ASSERT( channel != FS_NULL );
		IFS_Memset( channel, 0, sizeof(FS_DcdChannel) );
		if(buf[offset]){
			channel->id = IFS_Strdup( buf + offset);
			offset += IFS_Strlen( channel->id );
		}
		offset += 1;
		if(buf[offset]){
			channel->anchor = IFS_Strdup( buf + offset);
			offset += IFS_Strlen( channel->anchor );
		}
		offset += 1;
		channel->charge = FS_LE_BYTE_TO_UINT4( buf + offset );
		offset += 4;

		FS_ListAddTail( &pkg->channel_list, &channel->list );
	}
	/* read dcd channel feed list */
	for( i = 0; i < cnt; i ++ ){
		feed = IFS_Malloc( sizeof(FS_DcdChannelFeed) );
		FS_ASSERT( feed != FS_NULL );
		IFS_Memset( feed, 0, sizeof(FS_DcdChannelFeed) );
		FS_ListInit( &feed->link_list );
		FS_ListInit( &feed->content_list );
		FS_ListInit( &feed->entry_list );

		if(buf[offset]){
			feed->id = IFS_Strdup( buf + offset );
			offset += IFS_Strlen( feed->id );
		}
		offset += 1;
		if(buf[offset]){
			feed->title = IFS_Strdup( buf + offset );
			offset += IFS_Strlen( feed->title );
		}
		offset += 1;
		if(buf[offset]){
			feed->summary = IFS_Strdup( buf + offset );
			offset += IFS_Strlen( feed->summary );
		}
		offset += 1;
		offset += FS_DcdLinkListFromBuf( &feed->link_list, buf + offset );
		offset += FS_DcdContentListFromBuf( &feed->content_list, buf + offset );
		offset += FS_DcdEntryListFromBuf( &feed->entry_list, buf + offset );

		FS_ListAddTail( &pkg->feed_list, &feed->list );
	}
	FS_ASSERT( offset == len );
	if( offset != len ){
		FS_DcdFreeSyncPkg( pkg, FS_FALSE );
		dcdSes->pkg = FS_NULL;
	}else{
		dcdSes->pkg = pkg;
	}
READ_DONE:
	IFS_Free( buf );
	return;
}

/*
 * reference: CMCC《动态内容分发接口规范》
 * 对于未更新的频道项及频道的同步响应处理： 
 * 对于没有更新的频道项，可以用<entry ctxt_id='频道项ID'/>来表示。
 * 对于没有更新的频道，由于在<feed-meta>中已经表明了该用户已
 * 经订阅了该频道，所以没有必要在响应信息包中增加<feed>标签。
 * 但是如果频道中有任何信息的更新，都得在响应信息包中填写
 * 完整的<feed>标签（频道的title信息以及频道项信息）
*/
static void FS_DcdMergeSyncPkg( FS_DcdSession *dcdSes )
{
	FS_DcdSyncPkg *pkg, *new_pkg;
	FS_List *node;
	FS_DcdChannel *channel;
	FS_DcdChannelFeed *feed;
	FS_DcdEntry *entry, *old_entry;
	FS_DcdChannelFeed *old_feed;
/*
	FS_SINT4 i, cnt;
	FS_List *entry_node;
*/	
	pkg = dcdSes->pkg;
	new_pkg = dcdSes->new_pkg;
	FS_ASSERT( new_pkg != FS_NULL );

	if( pkg == FS_NULL ){
		/* update first time. do not need to merge */
		goto MERGE_DONE;
	}
	
	/* merge feed channel which is not update */
	node = new_pkg->channel_list.next;
	while( node != &new_pkg->channel_list ){
		channel = FS_ListEntry( node, FS_DcdChannel, list );
		node = node->next;
		feed = FS_DcdFindChannelFeedByCid( new_pkg, channel->id );
		if( feed == FS_NULL ){
			/* we find a channel feed, which is not update yet. merge old pkg to new pkg */
			feed = FS_DcdFindChannelFeedByCid( pkg, channel->id );
			if( feed ){
				FS_ListDel( &feed->list );
				FS_ListAddTail( &new_pkg->feed_list, &feed->list );
			}
		}
	}

	/* merge feed entry which is not update */
	do{
		entry = FS_DcdFindEmptyFeedEntry( new_pkg, &feed );
		if( entry ){
			/* we find a entry, which is not update yet. merge old pkg to new pkg */
			old_entry = FS_DcdFindFeedEntryByCid( pkg, entry->id, &old_feed );
			if( old_entry ){
				FS_ListDel( &entry->list );
				FS_ListDel( &old_entry->list );
				FS_ListAddTail( &feed->entry_list, &old_entry->list );
				FS_ListAddTail( &old_feed->entry_list, &entry->list );
			}
		}
	}while( entry != FS_NULL );

#if 0
	/* add old feed entry to new pkg when the entry count is permision. */
	node = new_pkg->feed_list.next;
	while( node != &new_pkg->feed_list ){
		feed = FS_ListEntry( node, FS_DcdChannelFeed, list );
		node = node->next;

		cnt = FS_ListCount( &feed->entry_list );
		old_feed = FS_DcdFindChannelFeedByCid( pkg, feed->id );
		if( cnt < FS_DCD_FEED_ENTRY_CNT && old_feed ){
			cnt = FS_DCD_FEED_ENTRY_CNT - cnt;
			entry_node = old_feed->entry_list.next;
			for( i = 0; i < cnt && entry_node != &old_feed->entry_list; i ++ ){
				old_entry = FS_ListEntry( entry_node, FS_DcdEntry, list );
				entry_node = entry_node->next;
				
				FS_ListDel( &old_entry->list );
				FS_ListAddTail( &feed->entry_list, &old_entry->list );
			}
		}
	}
#endif

MERGE_DONE:
	/* switch to new pkg */
	dcdSes->pkg = new_pkg;
	dcdSes->new_pkg = pkg;
	dcdSes->client_time = FS_GetSeconds( 0 );
}

static void FS_DcdUpdateResult( FS_DcdSession *dcdSes, FS_SINT4 err_code )
{	
	FS_BYTE *data;
	FS_List *rspEntryList;
	FS_UINT4 delay;
	
	FS_DCD_TRACE1("FS_DcdUpdateResult err_code = %d", err_code);
	if( dcdSes->state == FS_DCD_STATE_SYNC ){
		if( err_code == FS_DCD_ERR_OK ){
			/* sync pkg download ok. now, we parse it, and start to download image */
			dcdSes->new_pkg = FS_DcdParseSyncPkg( dcdSes->file );
			if( dcdSes->new_pkg == FS_NULL ){
				goto UPDATE_DONE;
			}
			#ifndef FS_DCD_TMP_FILE_DEBUG_
			FS_FileDelete( FS_DIR_TMP, dcdSes->file );
			#endif
			IFS_Memset( dcdSes->file, 0, sizeof(dcdSes->file) );
			dcdSes->state = FS_DCD_STATE_DIMG;
			goto DOWNLOAD_IMG;
		}
	}else if( dcdSes->state == FS_DCD_STATE_DIMG ){
		if( err_code == FS_DCD_ERR_OK ){
			/* download image ok. */
			data = IFS_Malloc( dcdSes->offset + 1 );
			if( data == FS_NULL ){
				goto UPDATE_DONE;
			}
			IFS_Memset( data, 0, dcdSes->offset + 1 );
			FS_FileRead( FS_DIR_TMP, dcdSes->file, 0, data, dcdSes->offset );
			rspEntryList = FS_DcdParseEntryList( dcdSes->boundary, data, dcdSes->offset );
			IFS_Free( data );
			FS_SAFE_FREE( dcdSes->boundary );
			#ifndef FS_DCD_TMP_FILE_DEBUG_
			FS_FileDelete( FS_DIR_TMP, dcdSes->file );
			#endif
			IFS_Memset( dcdSes->file, 0, sizeof(dcdSes->file) );
			if( rspEntryList == FS_NULL ){
				goto UPDATE_DONE;
			}
			FS_DcdSaveContent( dcdSes, rspEntryList );
			
DOWNLOAD_IMG:
			if( FS_DcdIsUpdateFinish( dcdSes ) ){
				FS_DcdMergeSyncPkg( dcdSes );
				FS_DcdSaveSyncPkg( dcdSes );
				FS_DcdIdleInit( dcdSes );
				/* inform dcd ui to update new data */
				FS_DcdUICtxUpdate( FS_TRUE );
				goto UPDATE_DONE;
			}else{
				/* there are still some content did not download yet. we download it again. */
				FS_DcdDownloadContent( dcdSes );
				return;
			}
		}
	}
	
UPDATE_DONE:
	/* finish this sync. do some clean up. and start ttl shedule */
	dcdSes->state = FS_DCD_STATE_IDLE;
	if( dcdSes->hHttp )
	{
		FS_HttpRequestCancel( dcdSes->hHttp, FS_FALSE );
		FS_HttpDestroyHandle( dcdSes->hHttp );
		dcdSes->hHttp = FS_NULL;
	}
	if( dcdSes->file[0] ){
		FS_FileDelete( FS_DIR_TMP, dcdSes->file );
		IFS_Memset( dcdSes->file, 0, sizeof(dcdSes->file) );
	}
	FS_SAFE_FREE( dcdSes->boundary );
	if( dcdSes->new_pkg ){
		FS_DcdFreeSyncPkg( dcdSes->new_pkg, FS_TRUE );
		dcdSes->new_pkg = FS_NULL;
	}
	/* start ttl */
	if( dcdSes->pkg ){
		delay = FS_MAX( dcdSes->pkg->ttl * 1000, FS_DCD_FIRST_UPDATE_DELAY );
	}else{
		delay = FS_DCD_FIRST_UPDATE_DELAY;
	}
#ifdef FS_PLT_WIN
	delay = FS_MIN( delay, 10000 );
#endif
	dcdSes->ttl_timer = IFS_StartTimer( FS_TIMER_ID_DCD_TTL, delay, FS_DcdTtlTimerCallback, dcdSes );
	FS_NetDisconnect( FS_APP_DCD );
	FS_DcdUICtxUpdateFinish( );
}

static FS_CHAR *FS_DcdGetReqType( FS_DCD_REQ_TYPE type )
{
	static FS_CHAR *s_req_type[] = { "ttl", "man", "svr", "mc", "mr", "sim", "data", "data2", "data3" };
	
	if( type < 0 || type >= FS_DCD_REQ_MAX ){
		return "man";
	}else{
		return s_req_type[type];
	}
}

static FS_CHAR *FS_DcdGetSyncAnchor( FS_DcdSession *dcdSes )
{
	FS_List *node;
	FS_CHAR *sync_anchor;
	FS_SINT4 offset = 0;
	FS_DcdChannel *channel;
	
	if( dcdSes->pkg == FS_NULL ){
		return FS_NULL;
	}
	if( FS_ListIsEmpty(&dcdSes->pkg->channel_list) ){
		return FS_NULL;
	}
	
	sync_anchor = IFS_Malloc( FS_DCD_SYNC_ANCHOR_LEN );
	if( sync_anchor == FS_NULL ){
		return FS_NULL;
	}
	IFS_Memset( sync_anchor, 0, FS_DCD_SYNC_ANCHOR_LEN );

	node = dcdSes->pkg->channel_list.next;
	while( node != &dcdSes->pkg->channel_list )
	{
		channel = FS_ListEntry( node, FS_DcdChannel, list );
		node = node->next;
		
		offset += IFS_Sprintf( sync_anchor + offset, "%s=%s,", channel->id, channel->anchor );
	}
	FS_ASSERT( (offset < FS_DCD_SYNC_ANCHOR_LEN) && (offset > 0) );
	sync_anchor[offset - 1] = 0;		/* delete tail ',' */
	return sync_anchor;
}

static FS_SINT4 FS_DcdFillRequestIds( FS_List *content_head, FS_CHAR *request_id, FS_SINT4 max_cnt )
{
	FS_List *content_node;
	FS_DcdContent *content;
	FS_SINT4 id_count = 0;
	
	content_node = content_head->next;
	while( content_node != content_head ){
		content = FS_ListEntry( content_node, FS_DcdContent, list );
		content_node = content_node->next;

		if( content->file == FS_NULL && IFS_Strstr( request_id, content->id ) == FS_NULL ){
			/* not in request id list and not download. add it. */
			IFS_Strcat( request_id, content->id );
			IFS_Strcat( request_id, "," );
			id_count ++;
			if( id_count >= max_cnt ){
				break;
			}
		}
	}
	return id_count;
}

static FS_CHAR *FS_DcdGetRequestIds( FS_DcdSession *dcdSes )
{
	FS_CHAR *request_id;
	FS_List *feed_node, *entry_node;
	FS_DcdChannelFeed *feed;
	FS_DcdEntry *entry;
	FS_SINT4 id_count = 0;
	
	if( dcdSes->new_pkg == FS_NULL ){
		return FS_NULL;
	}

	request_id  = IFS_Malloc( FS_DCD_SYNC_REQ_ID_LEN );
	if( request_id == FS_NULL ){
		return FS_NULL;
	}
	IFS_Memset( request_id, 0, FS_DCD_SYNC_REQ_ID_LEN );
	
	/* search feed list */
	feed_node = dcdSes->new_pkg->feed_list.next;
	while( feed_node != &dcdSes->new_pkg->feed_list ){
		feed = FS_ListEntry( feed_node, FS_DcdChannelFeed, list );
		feed_node = feed_node->next;

		/* fill feed list content */
		id_count += FS_DcdFillRequestIds( &feed->content_list, request_id, FS_DCD_SYNC_REQ_ID_CNT - id_count );
		if( id_count >= FS_DCD_SYNC_REQ_ID_CNT ){
			goto RET_REQIDS;
		}
		/* search entry list content */
		entry_node = feed->entry_list.next;
		while( entry_node != &feed->entry_list ){
			entry = FS_ListEntry( entry_node, FS_DcdEntry, list );
			entry_node = entry_node->next;

			/* fill entry list content */
			id_count += FS_DcdFillRequestIds( &entry->content_list, request_id, FS_DCD_SYNC_REQ_ID_CNT - id_count );
			if( id_count >= FS_DCD_SYNC_REQ_ID_CNT ){
				goto RET_REQIDS;
			}
		}
	}
	
RET_REQIDS:	
	if( id_count == 0 ){
		IFS_Free( request_id );
		request_id = FS_NULL;
	}else{
		id_count = IFS_Strlen( request_id );
		FS_ASSERT( id_count < FS_DCD_SYNC_REQ_ID_LEN );
		request_id[id_count - 1] = 0;		/* delete tail ',' */
	}
	return request_id;
}

static void FS_DcdHttpResponseStart( FS_DcdSession *dcdSes, FS_HttpHandle hHttp, FS_SINT4 status, FS_HttpHeader *headers )
{	
	FS_CHAR *boundary;
	
	if( dcdSes->state == FS_DCD_STATE_IDLE )
	{
		FS_DCD_TRACE1( "FS_DcdHttpResponseStart state not ready. state = %d", dcdSes->state );
		return;
	}
	if( status != FS_HTTP_OK ){
		FS_DCD_TRACE1( "FS_DcdHttpResponseStart status = %d", status );
		FS_DcdUpdateResult( dcdSes, FS_DCD_ERR_HTTP );
		return;
	}
	FS_DCD_TRACE3( "FS_DcdHttpResponseStart content_type = %s, content_len = %d, next_url = %s", headers->content_type, headers->content_length, headers->x_next_url );
	if( headers->x_next_url ){
		FS_COPY_TEXT( dcdSes->next_url, headers->x_next_url);
	}

	FS_GetGuid( dcdSes->file );
	dcdSes->content_len = headers->content_length;
	dcdSes->offset = 0;
	if( FS_STR_NI_EQUAL(headers->content_type, "application/dcd.xml", 19)  )
	{
		IFS_Strcat( dcdSes->file, ".xml" );
	}
	else if( FS_STR_NI_EQUAL( headers->content_type, "multipart/mixed", 15)  )
	{
		boundary = FS_GetParam( &headers->params, "boundary" );
		FS_ASSERT( boundary != FS_NULL );
		FS_COPY_TEXT( dcdSes->boundary, boundary );
		IFS_Strcat( dcdSes->file, ".mix" );
	}
	else
	{
		/* unexpect content-type */
		FS_DcdUpdateResult( dcdSes, FS_DCD_ERR_UNKNOW );
	}
}

static void FS_DcdHttpResponseData( FS_DcdSession *dcdSes, FS_HttpHandle hHttp, FS_BYTE *rdata, FS_SINT4 rdata_len )
{
	if( dcdSes->state == FS_DCD_STATE_IDLE )
	{
		FS_DCD_TRACE1( "FS_DcdHttpResponseData state not ready. state = %d", dcdSes->state );
		return;
	}
	
	FS_DCD_TRACE3( "FS_DcdHttpResponseData len = %d, offset = %d, rlen = %d", dcdSes->content_len,  dcdSes->offset, rdata_len );
	if( rdata_len == FS_FileWrite( FS_DIR_TMP, dcdSes->file, dcdSes->offset, rdata, rdata_len ) )
	{
		dcdSes->offset += rdata_len;
	}
	else
	{
		FS_DCD_TRACE0( "FS_DcdHttpResponseData FS_FileWrite error." );
		FS_DcdUpdateResult( dcdSes, FS_DCD_ERR_FILE );
	}
}

static void FS_DcdHttpResponseEnd( FS_DcdSession *dcdSes, FS_HttpHandle hHttp, FS_SINT4 error_code )
{
	FS_DCD_TRACE0( "FS_DcdHttpResponseEnd" );
	if( error_code == FS_HTTP_ERR_OK ){
		FS_DcdUpdateResult( dcdSes, FS_DCD_ERR_OK );
	} else {
		FS_DcdUpdateResult( dcdSes, FS_DCD_ERR_HTTP );
	}
}

static FS_CHAR *FS_DcdFormatHttpRequest( FS_DcdSession *dcdSes )
{
	FS_CHAR *header, *sync_anchor, *request_id;
	FS_SINT4 offset = 0;
	
	/* format header */
	header = IFS_Malloc( FS_DCD_NET_HEADER_LEN );
	if( header == FS_NULL ){
		return FS_NULL;
	}
	IFS_Memset( header, 0, FS_DCD_NET_HEADER_LEN );
	if( dcdSes->state == FS_DCD_STATE_SYNC )
	{
		offset += IFS_Sprintf( header, "X-DP-RequestType: %s\r\n", FS_DcdGetReqType(dcdSes->sync_type) );
		if( dcdSes->pkg && dcdSes->pkg->global_anchor ){
			offset += IFS_Sprintf( header + offset, "X-DP-Global-Sync-Anchor: %s\r\n", dcdSes->pkg->global_anchor );
		}
		sync_anchor = FS_DcdGetSyncAnchor( dcdSes );
		if( sync_anchor ){
			offset += IFS_Sprintf( header + offset, "X-DP-Sync-Anchor: %s\r\n", sync_anchor );
			IFS_Free( sync_anchor );
		}
	}
	else if( dcdSes->state == FS_DCD_STATE_DIMG )
	{
		offset += IFS_Sprintf( header, "X-DP-RequestType: %s\r\n", FS_DcdGetReqType(FS_DCD_REQ_DATA2) );
		request_id = FS_DcdGetRequestIds( dcdSes );
		FS_ASSERT( request_id != FS_NULL );
		if( request_id ){
			offset += IFS_Sprintf( header + offset, "X-DP-Request-Id: %s\r\n", request_id );
			IFS_Free( request_id );
		}
	}
	else
	{
		FS_DCD_TRACE1("FS_DcdFormatHttpRequest error. state = %d", dcdSes->state);		
		IFS_Free( header );
		return FS_NULL;
	}
	return header;	
}

static FS_DcdContent *FS_DcdFindEmptyContentInContentList( FS_List *content_head, FS_CHAR *cid )
{
	FS_List *content_node;
	FS_DcdContent *content;
	
	content_node = content_head->next;
	while( content_node != content_head ){
		content = FS_ListEntry( content_node, FS_DcdContent, list );
		content_node = content_node->next;

		if( content->file == FS_NULL && FS_STR_I_EQUAL(cid, content->id) ){
			/* we find a empty content */
			return content;
		}
	}
	return FS_NULL;
}

static FS_DcdContent *FS_DcdFindEmtryContentByCid( FS_DcdSession *dcdSes, FS_CHAR *cid )
{
	FS_List *feed_node, *entry_node;
	FS_DcdChannelFeed *feed;
	FS_DcdEntry *entry;
	FS_DcdContent *content;	
	
	/* search feed list */
	feed_node = dcdSes->new_pkg->feed_list.next;
	while( feed_node != &dcdSes->new_pkg->feed_list ){
		feed = FS_ListEntry( feed_node, FS_DcdChannelFeed, list );
		feed_node = feed_node->next;

		/* fill feed list content */
		content = FS_DcdFindEmptyContentInContentList( &feed->content_list, cid );
		if( content ){
			return content;
		}
		/* search entry list content */
		entry_node = feed->entry_list.next;
		while( entry_node != &feed->entry_list ){
			entry = FS_ListEntry( entry_node, FS_DcdEntry, list );
			entry_node = entry_node->next;

			/* fill entry list content */
			content = FS_DcdFindEmptyContentInContentList( &entry->content_list, cid );
			if( content ){
				return content;
			}
		}
	}
	return FS_NULL;
}

static void FS_DcdSaveContent( FS_DcdSession *dcdSes, FS_List *rspEntryList )
{
	FS_List *node;
	FS_DcdRspEntry *rspEntry;
	FS_DcdContent *content;
	
	node = rspEntryList->next;
	while( node != rspEntryList ){
		rspEntry = FS_ListEntry( node, FS_DcdRspEntry, list );
		node = node->next;
		content = FS_DcdFindEmtryContentByCid( dcdSes, rspEntry->content_id );
		if( content == FS_NULL ){
			/* delete this no match file to avoid generate some dummy file in file system */
			FS_FileDelete( FS_DIR_DCD, rspEntry->file );
			continue;
		}
		do{
			content->file = IFS_Strdup( rspEntry->file );
			content = FS_DcdFindEmtryContentByCid( dcdSes, rspEntry->content_id );
		} while( content != FS_NULL );
	}

	FS_DcdFreeRspEntryList( rspEntryList );
}

static void FS_DcdDownloadContent( FS_DcdSession *dcdSes )
{
	FS_CHAR *header, *url;
	FS_SockAddr addr;

	/* format header */
	header = FS_DcdFormatHttpRequest( dcdSes );
	if( header == FS_NULL ){
		FS_DcdUpdateResult( dcdSes, FS_DCD_ERR_UNKNOW );
	}
	/* send request */
	addr.host = FS_DCD_NET_GATWAY;
	addr.port = 80;
	url = FS_StrConCat( dcdSes->host, dcdSes->next_url, FS_NULL, FS_NULL );
	FS_HttpRequest( dcdSes->hHttp, &addr, "GET", url, FS_NULL, header );
	FS_DCD_TRACE2( "FS_DcdDownloadContent url = %s\r\nheader = %s", url, header );
	IFS_Free( url );
	IFS_Free( header );	
}

static void FS_DcdNetConnCallback( FS_DcdSession *dcdSes, FS_BOOL bOK )
{
	FS_CHAR *header, *url;
	FS_SockAddr addr;

	FS_DCD_TRACE1( "FS_DcdNetConnCallback ok = %d", bOK );
	if( !bOK ){
		FS_DcdUpdateResult( dcdSes, FS_DCD_ERR_CONN );
		return;
	}
	dcdSes->hHttp = FS_HttpCreateHandle( dcdSes, FS_DcdHttpResponseStart, FS_DcdHttpResponseData, FS_DcdHttpResponseEnd );
	if( dcdSes->hHttp == FS_NULL ){
		FS_DcdUpdateResult( dcdSes, FS_DCD_ERR_HTTP_BUSY );
		return;
	}

	/* format header */
	header = FS_DcdFormatHttpRequest( dcdSes );
	if( header == FS_NULL ){
		FS_DcdUpdateResult( dcdSes, FS_DCD_ERR_UNKNOW );
	}
	/* send request */
	addr.host = FS_DCD_NET_GATWAY;
	addr.port = 80;
	url = FS_StrConCat( dcdSes->host, dcdSes->next_url, FS_NULL, FS_NULL );
	FS_HttpRequest( dcdSes->hHttp, &addr, "GET", url, FS_NULL, header );
	FS_DCD_TRACE2( "FS_DcdNetConnCallback url = %s\r\nheader = %s", url, header );
	IFS_Free( url );
	IFS_Free( header );
}

FS_SINT4 FS_DcdUpdate( FS_DCD_REQ_TYPE type)
{
	FS_DcdSession *dcdSes = GFS_DcdSession;
	
	if( dcdSes && dcdSes->state != FS_DCD_STATE_IDLE ){
		FS_DCD_TRACE1( "FS_DcdDoUpdate not ready. state = %d", dcdSes->state );
		return FS_DCD_ERR_NET_READY;
	}
	if( dcdSes->ttl_timer ){
		IFS_StopTimer( dcdSes->ttl_timer );
		dcdSes->ttl_timer = 0;
	}
	dcdSes->state = FS_DCD_STATE_SYNC;
	dcdSes->sync_type = type;
	FS_NetConnect( "CMWAP", "", "", FS_DcdNetConnCallback, FS_APP_DCD, FS_FALSE, dcdSes );
	return FS_DCD_ERR_OK;
}

static void FS_DcdSwitchToNextIdleEntry( FS_DcdSession *dcdSes )
{
	FS_List *node;
	FS_BOOL find_feed = FS_TRUE;
	
	if( dcdSes->pkg == FS_NULL || dcdSes->idle_feed == FS_NULL || dcdSes->idle_entry == FS_NULL){
		return;
	}

	node = dcdSes->idle_entry->list.next;
	if( node == &dcdSes->idle_feed->entry_list ){
		find_feed = FS_FALSE;
		/* switch to next non empty channel */
		node = dcdSes->idle_feed->list.next;
		while( node != &dcdSes->pkg->feed_list ){
			dcdSes->idle_feed = FS_ListEntry( node, FS_DcdChannelFeed, list );
			node = node->next;
			if( ! FS_ListIsEmpty(&dcdSes->idle_feed->entry_list) ){
				find_feed = FS_TRUE;
				goto FIND_FEED;
			}
		}
	}else{
		dcdSes->idle_entry = FS_ListEntry( node, FS_DcdEntry, list);
		goto FIND_ENTRY;
	}

	if( ! find_feed ){
		/* reach to last channel. reinit to first channel */
		FS_DcdIdleInit( dcdSes );
		FS_DcdDrawIdle( );
		return;
	}
	
FIND_FEED:
	node = dcdSes->idle_feed->entry_list.next;
	dcdSes->idle_entry = FS_ListEntry( node, FS_DcdEntry, list);
FIND_ENTRY:
	FS_DcdDrawIdle( );
	dcdSes->idle_timer = IFS_StartTimer( FS_TIMER_ID_DCD_IDLE, FS_DcdGetIdleTime(), FS_DcdIdleTimerCallBack, dcdSes );
}

static void FS_DcdIdleTimerCallBack( FS_DcdSession *dcdSes )
{
	FS_ASSERT( dcdSes == GFS_DcdSession );
	FS_ASSERT( dcdSes->pkg != FS_NULL );
	FS_ASSERT( dcdSes->idle_feed != FS_NULL && dcdSes->idle_entry != FS_NULL );

	dcdSes->idle_timer = 0;
	FS_DcdSwitchToNextIdleEntry( dcdSes );
}

static void FS_DcdIdleInit( FS_DcdSession *dcdSes )
{
	FS_List *node, *entry_node;
	FS_BOOL find_entry = FS_FALSE;
	
	dcdSes->idle_feed = FS_NULL;
	dcdSes->idle_entry = FS_NULL;
	if( dcdSes->idle_timer ){
		IFS_StopTimer( dcdSes->idle_timer );
		dcdSes->idle_timer = 0;
	}
	
	if( dcdSes->pkg ){
		if( ! FS_ListIsEmpty(&dcdSes->pkg->feed_list) ){
			FS_ListForEach( node, &dcdSes->pkg->feed_list ){
				dcdSes->idle_feed = FS_ListEntry( node, FS_DcdChannelFeed, list );
				if( ! FS_ListIsEmpty( &dcdSes->idle_feed->entry_list ) ){
					entry_node = dcdSes->idle_feed->entry_list.next;
					dcdSes->idle_entry = FS_ListEntry( entry_node, FS_DcdEntry, list );
					find_entry = FS_TRUE;
					goto IDLE_INIT_DONE;
				}
			}
		}
	}
	
IDLE_INIT_DONE:
	if( find_entry ){
		if( dcdSes->idle_state == FS_DCD_IDLE_STATE_NORMAL ){
			dcdSes->idle_timer = IFS_StartTimer( FS_TIMER_ID_DCD_IDLE, FS_DcdGetIdleTime(), FS_DcdIdleTimerCallBack, dcdSes );
		}
	}else{
		dcdSes->idle_entry = FS_NULL;
		dcdSes->idle_feed = FS_NULL;
	}
}

FS_SINT4 FS_DcdInit( void )
{
	FS_CHAR *str;
	FS_DcdSession *dcdSes;
	FS_UINT4 delay, power_off;
	
	if( GFS_DcdSession != FS_NULL ){
		return FS_DCD_OK;
	}
	FS_DcdConfigInit( );
	dcdSes = IFS_Malloc( sizeof(FS_DcdSession) );
	if( dcdSes == FS_NULL ){
		return FS_DCD_ERR_MEMORY;
	}
	IFS_Memset( dcdSes, 0, sizeof(FS_DcdSession) );
	FS_ListInit( &dcdSes->snapshot_list );
	
	/* read sync pkg */
	FS_DcdReadSyncPkg( dcdSes );
	/* read dcd snapshot */
	FS_DcdReadSnapshotList( dcdSes );
	if( dcdSes->host == FS_NULL ){
		dcdSes->host = FS_UrlGetHost( FS_DCD_DEFAULT_URL );
		str = IFS_Strstr( FS_DCD_DEFAULT_URL, dcdSes->host );
		str += IFS_Strlen( dcdSes->host );
		dcdSes->next_url = IFS_Strdup( str );		
	}
	/* init idle display entry */
	FS_DcdIdleInit( dcdSes );
	dcdSes->state = FS_DCD_STATE_IDLE;
	/* calculate ttl. we must consider of power off ttl. */
	if( dcdSes->pkg ){
		power_off = FS_GetSeconds( 0 ) - dcdSes->client_time;
		if( dcdSes->pkg->ttl > power_off ){
			dcdSes->pkg->ttl -= power_off;
		}else{
			dcdSes->pkg->ttl = 0;
		}
		dcdSes->pkg->server_time += power_off;
		delay = FS_MAX( FS_DCD_FIRST_UPDATE_DELAY, dcdSes->pkg->ttl * 1000 );
	}else{
		delay = FS_DCD_FIRST_UPDATE_DELAY;
	}
	
	/* start ttl */
#ifdef FS_PLT_WIN
	delay = FS_MIN( delay, 10000 );
#endif
	dcdSes->ttl_timer = IFS_StartTimer( FS_TIMER_ID_DCD_TTL, delay, FS_DcdTtlTimerCallback, dcdSes );
	GFS_DcdSession = dcdSes;
	return FS_DCD_OK;
}

void FS_DcdDeinit( void )
{
	FS_DcdSession *dcdSes;
	
	if( GFS_DcdSession == FS_NULL ){
		return;
	}
	dcdSes = GFS_DcdSession;
	GFS_DcdSession = FS_NULL;
	FS_SAFE_FREE( dcdSes->host );
	FS_SAFE_FREE( dcdSes->next_url );
	FS_SAFE_FREE( dcdSes->boundary );
	if( dcdSes->ttl_timer ){
		IFS_StopTimer( dcdSes->ttl_timer );
		dcdSes->ttl_timer = 0;
	}
	if( dcdSes->idle_timer ){
		IFS_StopTimer( dcdSes->idle_timer );
		dcdSes->idle_timer = 0;
	}
	if( dcdSes->hHttp ){
		FS_HttpRequestCancel( dcdSes->hHttp, FS_FALSE );
		FS_HttpDestroyHandle( dcdSes->hHttp );
		dcdSes->hHttp = FS_NULL;
	}
	if( dcdSes->file[0] ){
		FS_FileDelete( FS_DIR_TMP, dcdSes->file );
	}
	if( dcdSes->new_pkg ){
		FS_DcdFreeSyncPkg( dcdSes->new_pkg, FS_TRUE );
		dcdSes->new_pkg = FS_NULL;
	}
	if( dcdSes->pkg ){
		FS_DcdFreeSyncPkg( dcdSes->pkg, FS_FALSE );
		dcdSes->pkg = FS_NULL;
	}
	FS_DcdFreeSnapshotList( dcdSes, FS_FALSE );
	IFS_Free( dcdSes );
	FS_NetDisconnect( FS_APP_DCD );
}

FS_List *FS_DcdGetChannelList( void )
{
	FS_DcdSession *dcdSes = GFS_DcdSession;
	FS_ASSERT( dcdSes != FS_NULL );
	if( GFS_DcdConfig.on && dcdSes->pkg ){
		return &dcdSes->pkg->feed_list;
	}else{
		return FS_NULL;
	}
}

FS_DcdEntry *FS_DcdGetIdleEntry( void )
{
	FS_DcdSession *dcdSes = GFS_DcdSession;
	FS_ASSERT( dcdSes != FS_NULL );
	if( GFS_DcdConfig.on && GFS_DcdConfig.idle_display ){
		return dcdSes->idle_entry;
	}else{
		return FS_NULL;
	}
}

FS_DcdChannelFeed *FS_DcdGetIdleChannel( void )
{
	FS_DcdSession *dcdSes = GFS_DcdSession;
	FS_ASSERT( dcdSes != FS_NULL );
	if( GFS_DcdConfig.on && GFS_DcdConfig.idle_display ){
		return dcdSes->idle_feed;
	}else{
		return FS_NULL;
	}
}

FS_SINT4 FS_DcdGetChannelCount( void )
{
	FS_DcdSession *dcdSes = GFS_DcdSession;
	FS_ASSERT( dcdSes != FS_NULL );
	if( GFS_DcdConfig.on && dcdSes->pkg ){
		return FS_ListCount( &dcdSes->pkg->feed_list );
	}else{
		return 0;
	}
}

FS_SINT4 FS_DcdGetIdleChannelIndex( void )
{
	FS_List *node;
	FS_DcdChannelFeed *feed;
	FS_SINT4 index = 0;
	FS_DcdSession *dcdSes = GFS_DcdSession;
	
	FS_ASSERT( dcdSes != FS_NULL );
	if( ! GFS_DcdConfig.on ){
		return -1;
	}
	if( dcdSes->idle_feed ){
		FS_ListForEach( node, &dcdSes->pkg->feed_list ){
			feed = FS_ListEntry( node, FS_DcdChannelFeed, list );
			if( feed == dcdSes->idle_feed ){
				return index;
			}
			index ++;
		}
	}
	return -1;
}

void FS_DcdPauseIdleTimer( void )
{
	FS_DcdSession *dcdSes = GFS_DcdSession;
	
	FS_ASSERT( dcdSes != FS_NULL );
	if( ! GFS_DcdConfig.on || ! GFS_DcdConfig.idle_display ){
		return;
	}
	if( dcdSes->idle_timer ){
		IFS_StopTimer( dcdSes->idle_timer );
		dcdSes->idle_timer = 0;
	}
	dcdSes->idle_state = FS_DCD_IDLE_STATE_PAUSE;
}

void FS_DcdResumeIdleTimer( void )
{
	FS_DcdSession *dcdSes = GFS_DcdSession;
	
	FS_ASSERT( dcdSes != FS_NULL );
	if( ! GFS_DcdConfig.on || ! GFS_DcdConfig.idle_display ){
		return;
	}
	if( dcdSes->idle_timer ){
		return;
	}
	dcdSes->idle_state = FS_DCD_IDLE_STATE_NORMAL;
	FS_DcdSwitchToNextIdleEntry( dcdSes );
}

void FS_DcdSaveChannelList( void )
{
	FS_DcdSession *dcdSes = GFS_DcdSession;
	
	FS_ASSERT( dcdSes != FS_NULL );
	if( dcdSes ){
		FS_DcdSaveSyncPkg( dcdSes );
	}
}

static FS_DcdEntry *FS_DcdEntryDup( FS_DcdEntry *entry )
{
	FS_List *node;
	FS_DcdEntry *dentry;
	FS_DcdLink *link, *dlink;
	FS_DcdContent *content, *dcontent;
	FS_CHAR *ext;
	
	dentry = IFS_Malloc( sizeof(FS_DcdEntry) );
	FS_ASSERT( dentry != FS_NULL );
	IFS_Memset( dentry, 0, sizeof(FS_DcdEntry) );
	FS_ListInit( &dentry->link_list );
	FS_ListInit( &dentry->content_list );

	FS_COPY_TEXT( dentry->id, entry->id );
	FS_COPY_TEXT( dentry->title, entry->title );
	FS_COPY_TEXT( dentry->summary, entry->summary );
	FS_COPY_TEXT( dentry->author, entry->author );
	dentry->issue = entry->issue;
	dentry->flag = entry->flag;
	
	FS_ListForEach( node, &entry->link_list ){
		link = FS_ListEntry( node, FS_DcdLink, list );
		dlink = IFS_Malloc( sizeof(FS_DcdLink) );
		IFS_Memset( dlink, 0, sizeof(FS_DcdLink) );
		FS_COPY_TEXT( dlink->title, link->title );
		FS_COPY_TEXT( dlink->href, link->href );
		FS_ListAddTail( &dentry->link_list, &dlink->list );
	}

	FS_ListForEach( node, &entry->content_list ){
		content = FS_ListEntry( node, FS_DcdContent, list );
		dcontent = IFS_Malloc( sizeof(FS_DcdContent) );
		IFS_Memset( dcontent, 0, sizeof(FS_DcdContent) );
		FS_COPY_TEXT( dcontent->id, content->id );
		FS_COPY_TEXT( dcontent->type, content->type );
		FS_COPY_TEXT( dcontent->href, content->href );
		/* generate new file name */
		dcontent->file = IFS_Malloc( FS_FILE_NAME_LEN );
		FS_GetGuid( dcontent->file );
		ext = FS_GetExtFromMime( dcontent->type );
		if( ext ){
			IFS_Strcat( dcontent->file, "." );
			IFS_Strcat( dcontent->file, ext );
		}
		/* copy file */
		FS_FileCopy( FS_DIR_DCD, content->file, FS_DIR_DCD, dcontent->file );
		dcontent->autoplay = content->autoplay;
		
		FS_ListAddTail( &dentry->content_list, &dcontent->list );
	}
	return dentry;
}

static void FS_DcdSaveSnapshotList( FS_DcdSession *dcdSes )
{
	FS_BYTE *buf;
	FS_SINT4 offset = 0;
	
	buf = IFS_Malloc( FS_DCD_CHANNEL_FEED_BUF_LEN );
	FS_ASSERT( buf != FS_NULL );
	if( buf == FS_NULL ) return;
	IFS_Memset( buf, 0, FS_DCD_CHANNEL_FEED_BUF_LEN );
	offset = FS_DcdEntryListToBuf( &dcdSes->snapshot_list, buf );
	FS_FileWrite( FS_DIR_DCD, FS_DCD_SNAP_FILE, 0, buf, offset );
	IFS_Free( buf );
}

static void FS_DcdFreeSnapshotList( FS_DcdSession *dcdSes, FS_BOOL del_file )
{
	FS_List *node;
	FS_DcdEntry *entry;

	node = dcdSes->snapshot_list.next;
	while( node != &dcdSes->snapshot_list ){
		entry = FS_ListEntry( node, FS_DcdEntry, list );
		node = node->next;
		FS_ListDel( &entry->list );
		FS_DcdFreeEntry( entry, del_file );
	}
}

static void FS_DcdReadSnapshotList( FS_DcdSession *dcdSes )
{
	FS_SINT4 len, offset;
	FS_BYTE *buf;
	
	len = FS_FileGetSize( FS_DIR_DCD, FS_DCD_SNAP_FILE );
	if( len <= 0 ){
		return;
	}
	buf = IFS_Malloc( len );
	FS_ASSERT( buf != FS_NULL );
	if( buf == FS_NULL ){
		return;
	}
	IFS_Memset( buf, 0, len );
	FS_FileRead( FS_DIR_DCD, FS_DCD_SNAP_FILE, 0, buf, len );

	offset = FS_DcdEntryListFromBuf( &dcdSes->snapshot_list, buf );
	IFS_Free( buf );
	FS_ASSERT( offset == len );
	if( offset != len ){
		FS_DcdFreeSnapshotList( dcdSes, FS_TRUE );
	}
}

FS_SINT4 FS_DcdSnapshotAddEntry( FS_DcdEntry *entry )
{
	FS_DcdSession *dcdSes = GFS_DcdSession;
	FS_DcdEntry *sentry;
	
	FS_ASSERT( dcdSes != FS_NULL );
	if( dcdSes == FS_NULL || entry == FS_NULL )
		return FS_DCD_ERR_UNKNOW;
	if( FS_ListCount(&dcdSes->snapshot_list) >= FS_DCD_FEED_ENTRY_CNT )
		return FS_DCD_ERR_FULL;
	sentry = FS_DcdEntryDup( entry );
	FS_ListAddTail( &dcdSes->snapshot_list, &sentry->list );
	FS_DcdSaveSnapshotList( dcdSes );
	return FS_DCD_OK;
}

FS_SINT4 FS_DcdSnapshotDelEntry( FS_DcdEntry *entry )
{
	FS_DcdSession *dcdSes = GFS_DcdSession;
	
	FS_ASSERT( dcdSes != FS_NULL );
	if( dcdSes == FS_NULL || entry == FS_NULL )
		return FS_DCD_ERR_UNKNOW;
	FS_ListDel( &entry->list );
	FS_DcdFreeEntry( entry, FS_TRUE );	
	FS_DcdSaveSnapshotList( dcdSes );
	return FS_DCD_OK;
}

FS_SINT4 FS_DcdSnaphotDelAll( void )
{
	FS_DcdSession *dcdSes = GFS_DcdSession;
	
	FS_ASSERT( dcdSes != FS_NULL );
	if( dcdSes == FS_NULL )
		return FS_DCD_ERR_UNKNOW;
	FS_DcdFreeSnapshotList( dcdSes, FS_TRUE );
	FS_FileDelete( FS_DIR_DCD, FS_DCD_SNAP_FILE );
	return FS_DCD_OK;
}

FS_List *FS_DcdGetSnapshotList( void )
{
	FS_DcdSession *dcdSes = GFS_DcdSession;
	FS_ASSERT( dcdSes != FS_NULL );
	return &dcdSes->snapshot_list;
}

static void FS_DcdConfigInit( void )
{
	if( sizeof(FS_DcdConfig) != FS_FileRead(FS_DIR_DCD, FS_DCD_CFG_FILE, 0, &GFS_DcdConfig, sizeof(FS_DcdConfig)) ){
		GFS_DcdConfig.idle_display = FS_TRUE;
		GFS_DcdConfig.idle_speed = (FS_BYTE)FS_DCD_IDLE_MID;
		GFS_DcdConfig.on = FS_TRUE;
		GFS_DcdConfig.net_mode = (FS_BYTE)FS_DCD_NET_ALWAYS_ON;
	}
}

FS_DcdConfig FS_DcdGetConfig( void )
{
	return GFS_DcdConfig;
}

void FS_DcdSaveConfig( FS_DcdConfig cfg )
{
	FS_DcdSession *dcdSes = GFS_DcdSession;
	FS_DcdConfig orgCfg = GFS_DcdConfig;

	FS_ASSERT( dcdSes );
	
	GFS_DcdConfig = cfg;
	FS_FileWrite( FS_DIR_DCD, FS_DCD_CFG_FILE, 0, &GFS_DcdConfig, sizeof(FS_DcdConfig) );
	/* if turn on dcd. we may start idle display and do a update */
	if( ! orgCfg.on && cfg.on ){
		if( cfg.idle_display ){
			FS_DcdIdleInit( dcdSes );
		}else{
			if( dcdSes->idle_timer ){
				IFS_StopTimer( dcdSes->idle_timer );
				dcdSes->idle_timer = FS_NULL;
			}
			dcdSes->idle_feed = FS_NULL;
			dcdSes->idle_entry = FS_NULL;
		}
		if( cfg.net_mode == FS_DCD_NET_ALWAYS_ON || (cfg.net_mode == FS_DCD_NET_ROAMING_OFF && ! IFS_IsInternationalRoaming()) ){
			FS_DcdUpdate( FS_DCD_REQ_MAN );
		}
		FS_DcdUICtxUpdate( FS_FALSE );
		return;
	}
	/* if turn off dcd. we may stop idle display and stop ttl */
	if( orgCfg.on && ! cfg.on ){
		if( dcdSes->ttl_timer ){
			IFS_StopTimer( dcdSes->ttl_timer );
			dcdSes->ttl_timer = FS_NULL;
		}
		if( dcdSes->idle_timer ){
			IFS_StopTimer( dcdSes->idle_timer );
			dcdSes->idle_timer = FS_NULL;
		}
		dcdSes->idle_feed = FS_NULL;
		dcdSes->idle_entry = FS_NULL;
		FS_DcdUICtxUpdate( FS_FALSE );
		return;
	}
	/* if net mode change. we make an update */
	if( cfg.on && orgCfg.net_mode == FS_DCD_NET_ALWAYS_OFF 
		&& ( cfg.net_mode == FS_DCD_NET_ALWAYS_ON || (cfg.net_mode == FS_DCD_NET_ROAMING_OFF && ! IFS_IsInternationalRoaming()) )){
		FS_DcdUpdate( FS_DCD_REQ_MAN );
		return;
	}
	/* if turn on idle display */
	if( cfg.on && ! orgCfg.idle_display && cfg.idle_display ){
		FS_DcdIdleInit( GFS_DcdSession );
		return;
	}
	/* if turn off idle display */
	if( cfg.on && orgCfg.idle_display && ! cfg.idle_display ){
		if( dcdSes->idle_timer ){
			IFS_StopTimer( dcdSes->idle_timer );
			dcdSes->idle_timer = FS_NULL;
		}
		dcdSes->idle_feed = FS_NULL;
		dcdSes->idle_entry = FS_NULL;
		return;
	}
}

#ifdef FS_DEBUG_
void FS_DcdReadWriteSyncPkgTest( void )
{
	FS_DcdSession dcdSes;
	FS_DcdChannel *channel;
	FS_DcdChannelFeed *feed;
	FS_DcdEntry *entry;
	FS_DcdLink *link;
	FS_DcdContent *content;
	
	IFS_Memset( &dcdSes, 0, sizeof(FS_DcdSession) );
	dcdSes.pkg = IFS_Malloc( sizeof(FS_DcdSyncPkg) );
	IFS_Memset( dcdSes.pkg, 0, sizeof(FS_DcdSyncPkg) );
	FS_ListInit( &dcdSes.pkg->channel_list );
	FS_ListInit( &dcdSes.pkg->feed_list );
	dcdSes.pkg->global_anchor = IFS_Strdup( "globalanchor" );
	dcdSes.pkg->ttl = 120;
	dcdSes.pkg->server_time = 231231;

	channel = IFS_Malloc( sizeof(FS_DcdChannel) );
	IFS_Memset( channel, 0, sizeof(FS_DcdChannel) );
	FS_ListAddTail( &dcdSes.pkg->channel_list, &channel->list );
	channel->id = IFS_Strdup( "id1234567" );
	channel->anchor = IFS_Strdup( "anchor1234567" );

	feed = IFS_Malloc(sizeof(FS_DcdChannelFeed) );
	IFS_Memset( feed, 0, sizeof(FS_DcdChannelFeed) );
	FS_ListAddTail( &dcdSes.pkg->feed_list, &feed->list );
	FS_ListInit( &feed->link_list );
	FS_ListInit( &feed->content_list );
	FS_ListInit( &feed->entry_list );

	feed->id = IFS_Strdup( "id1234567" );
	feed->title = IFS_Strdup( "title1234567" );
	feed->summary = IFS_Strdup( "summary1234567" );
	link = IFS_Malloc( sizeof(FS_DcdLink) );
	IFS_Memset( link, 0, sizeof(FS_DcdLink) );
	FS_ListAddTail( &feed->link_list, &link->list );
	link->href = IFS_Strdup("linkhref1");
	
	content = IFS_Malloc( sizeof(FS_DcdContent) );
	IFS_Memset( content, 0, sizeof(FS_DcdContent) );
	FS_ListAddTail( &feed->content_list, &content->list );
	content->id = IFS_Strdup("cid908908018");
	content->type = IFS_Strdup("image/gif");
	content->autoplay = 1;

	entry = IFS_Malloc( sizeof(FS_DcdEntry) );
	IFS_Memset( entry, 0, sizeof(FS_DcdEntry) );
	FS_ListAddTail( &feed->entry_list, &entry->list );
	FS_ListInit( &entry->link_list );
	FS_ListInit( &entry->content_list );
	entry->id = IFS_Strdup("entry1234343");
	entry->title = IFS_Strdup( "entrytitle34234" );
	entry->summary = IFS_Strdup( "entrysumarry09-0989" );
	content = IFS_Malloc( sizeof(FS_DcdContent) );
	IFS_Memset( content, 0, sizeof(FS_DcdContent) );
	FS_ListAddTail( &entry->content_list, &content->list );
	content->id = IFS_Strdup("cid908908034");
	content->type = IFS_Strdup("image/jpeg");
	content->file = IFS_Strdup("32423423.jpg");

	FS_DcdSaveSyncPkg( &dcdSes );
	FS_DcdReadSyncPkg( &dcdSes );
	FS_DcdFreeSyncPkg( dcdSes.pkg, FS_FALSE );
}
#endif

#endif // FS_MODULE_DCD
