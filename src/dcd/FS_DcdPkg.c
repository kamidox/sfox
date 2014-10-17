#include "inc/FS_Config.h"

#ifdef FS_MODULE_DCD

#include "inc/dcd/FS_DcdPkg.h"
#include "inc/util/FS_Util.h"
#include "inc/inte/FS_Inte.h"
#include "inc/util/FS_Sax.h"
#include "inc/util/FS_Mime.h"
#include "inc/util/FS_MemDebug.h"

typedef struct FS_DcdPkgData_Tag
{
	FS_DcdSyncPkg *		pkg;
	FS_CHAR *			charset;
	FS_DcdChannel *		channel;
	FS_DcdChannelFeed *	feed;
	FS_DcdEntry *		entry;
	FS_DcdLink *			link;
	FS_DcdContent *		content;

	FS_CHAR *			file;
	FS_SINT4			offset;
}FS_DcdPkgData;

static void FS_DcdPkgStartElement( FS_DcdPkgData *pkg, FS_CHAR *ename )
{
	if( FS_STR_I_EQUAL(ename, "feed-meta") ){
		pkg->channel = IFS_Malloc( sizeof(FS_DcdChannel) );
		IFS_Memset( pkg->channel, 0, sizeof(FS_DcdChannel) );
	}else if( FS_STR_I_EQUAL(ename, "feed") ){
		pkg->feed = IFS_Malloc( sizeof(FS_DcdChannelFeed) );
		IFS_Memset( pkg->feed, 0, sizeof(FS_DcdChannelFeed) );
		FS_ListInit( &pkg->feed->link_list );
		FS_ListInit( &pkg->feed->content_list );
		FS_ListInit( &pkg->feed->entry_list );
	}else if( FS_STR_I_EQUAL(ename, "link") ){
		pkg->link = IFS_Malloc( sizeof(FS_DcdLink) );
		IFS_Memset( pkg->link, 0, sizeof(FS_DcdLink) );
	}else if( FS_STR_I_EQUAL(ename, "content") ){
		pkg->content = IFS_Malloc( sizeof(FS_DcdContent) );
		IFS_Memset( pkg->content, 0, sizeof(FS_DcdContent) );
	}else if( FS_STR_I_EQUAL(ename, "entry") ){
		pkg->entry = IFS_Malloc( sizeof(FS_DcdEntry) );
		IFS_Memset( pkg->entry, 0, sizeof(FS_DcdEntry) );
		FS_ListInit( &pkg->entry->link_list );
		FS_ListInit( &pkg->entry->content_list );
	}
}

static void FS_DcdPkgStartElementEnd( FS_DcdPkgData *pkg, FS_CHAR *ename )
{
	if( FS_STR_I_EQUAL(ename, "feed-meta") && pkg->channel ){
		FS_ListAddTail( &pkg->pkg->channel_list, &pkg->channel->list );
		pkg->channel = FS_NULL;
	}else if( FS_STR_I_EQUAL(ename, "link") && pkg->link ){
		if( pkg->entry ){
			FS_ListAddTail( &pkg->entry->link_list, &pkg->link->list );
			pkg->link = FS_NULL;
		}else if( pkg->feed ){
			FS_ListAddTail( &pkg->feed->link_list, &pkg->link->list );
			pkg->link = FS_NULL;
		}
	}else if( FS_STR_I_EQUAL(ename, "content") && pkg->content ){
		if( pkg->entry ){
			FS_ListAddTail( &pkg->entry->content_list, &pkg->content->list );
			pkg->content= FS_NULL;
		}else if( pkg->feed ){
			FS_ListAddTail( &pkg->feed->content_list, &pkg->content->list );
			pkg->content= FS_NULL;
		}
	}
}

static void FS_DcdPkglEndElement( FS_DcdPkgData *pkg, FS_CHAR *ename )
{
	if( FS_STR_I_EQUAL(ename, "entry") && pkg->feed && pkg->entry ){
		FS_ListAddTail( &pkg->feed->entry_list, &pkg->entry->list );
		pkg->entry = FS_NULL;
	} else if( FS_STR_I_EQUAL(ename, "feed") && pkg->pkg && pkg->feed ){
		FS_ListAddTail( &pkg->pkg->feed_list, &pkg->feed->list );
		pkg->feed = FS_NULL;		
	}
}

static void FS_DcdPkgElementText( FS_DcdPkgData *pkg, FS_CHAR *ename, FS_CHAR *str, FS_SINT4 slen )
{
	if( str == FS_NULL || slen <= 0 ) return;
	
	if( FS_STR_I_EQUAL(ename, "title") ){
		if( pkg->entry ){
			pkg->entry->title = FS_ProcessCharset( str, slen, pkg->charset, FS_NULL );
			if( pkg->entry->title == FS_NULL )
				pkg->entry->title = FS_Strndup( str, slen );
		}else if( pkg->feed ){
			pkg->feed->title = FS_ProcessCharset( str, slen, pkg->charset, FS_NULL );
			if( pkg->feed->title == FS_NULL )
				pkg->feed->title = FS_Strndup( str, slen );
		}
	}else if( FS_STR_I_EQUAL(ename, "summary") ){
		if( pkg->entry ){
			pkg->entry->summary = FS_ProcessCharset( str, slen, pkg->charset, FS_NULL );
			if( pkg->entry->summary == FS_NULL )
				pkg->entry->summary = FS_Strndup( str, slen );
		}else if( pkg->feed ){
			pkg->feed->summary = FS_ProcessCharset( str, slen, pkg->charset, FS_NULL );
			if( pkg->feed->summary == FS_NULL )
				pkg->feed->summary = FS_Strndup( str, slen );
		}
	}else if( FS_STR_I_EQUAL(ename, "issued") && pkg->entry ){
		pkg->entry->issue = IFS_Atoi(str);
	}else if( FS_STR_I_EQUAL(ename, "name") && pkg->entry ){
		pkg->entry->author = FS_ProcessCharset( str, slen, pkg->charset, FS_NULL );
		if( pkg->entry->author == FS_NULL )
			pkg->entry->author = FS_Strndup( str, slen );
	}
}

static void FS_DcdPkgElementAttr( FS_DcdPkgData *pkg, FS_CHAR *ename, FS_CHAR *name, FS_CHAR *value )
{
	FS_SINT4 len;
	if( FS_STR_I_EQUAL(ename, "global-meta") ){
		/* global meta */
		if( FS_STR_I_EQUAL(name, "anchor") ){
			FS_COPY_TEXT( pkg->pkg->global_anchor, value );
		}else if( FS_STR_I_EQUAL(name, "ttl") ){
			pkg->pkg->ttl = (IFS_Atoi( value ) / 1000) + 1;
		}else if( FS_STR_I_EQUAL(name, "server-time") ){
			/* server-time 指定服务器侧的系统时间，取值为从GMT 1970年1月1日00:00:00 起的毫秒值 */
			len = IFS_Strlen( value );
			if( len > 3 ) value[len - 3] = 0;
			pkg->pkg->server_time = IFS_Atoi( value );
		}
	}else if( FS_STR_I_EQUAL(ename, "feed-meta") && pkg->channel ){
		/* feed_meta */
		if( FS_STR_I_EQUAL(name, "ctxt-id") ){
			FS_COPY_TEXT( pkg->channel->id, value );
		}else if( FS_STR_I_EQUAL(name, "anchor") ){
			FS_COPY_TEXT( pkg->channel->anchor, value );
		}else if( FS_STR_I_EQUAL(name, "charge") ){
			if( value && (value[0] == 'Y' || value[0] == 'y') ){
				pkg->channel->charge = FS_DCD_FEED_CHARGE;
			}else if( value && (value[0] == 'N' || value[0] == 'n')  ){
				pkg->channel->charge = FS_DCD_FEED_FREE;
			}else if( value && (value[0] == 'P' || value[0] == 'p')  ){
				pkg->channel->charge = FS_DCD_FEED_PACKAGE;
			}
		}
	}else if( FS_STR_I_EQUAL(ename, "link") && pkg->link ){
		/* link */
		if( FS_STR_I_EQUAL(name, "href") ){
			FS_ProcessEsc( value, -1 );
			FS_COPY_TEXT( pkg->link->href, value );
		}else if( FS_STR_I_EQUAL(name, "title") ){
			pkg->link->title = FS_ProcessCharset( value, -1, pkg->charset, FS_NULL );
			if( pkg->link->title == FS_NULL )
				pkg->link->title = IFS_Strdup( value );
		}
	}else if( FS_STR_I_EQUAL(ename, "content") && pkg->content ){
		/* content */
		if( FS_STR_I_EQUAL(name, "type") ){
			FS_COPY_TEXT( pkg->content->type, value );
		}else if( FS_STR_I_EQUAL(name, "ctxt-id") ){
			FS_COPY_TEXT( pkg->content->id, value );
		}else if( FS_STR_I_EQUAL(name, "href") ){
			FS_ProcessEsc( value, -1 );
			FS_COPY_TEXT( pkg->content->href, value );
		}else if( FS_STR_I_EQUAL(name, "autoplay") ){
			if( value && (value[0] == 'Y' || value[0] == 'y') ){
				pkg->content->autoplay = FS_TRUE;
			}
		}
	}else if( FS_STR_I_EQUAL(ename, "entry") && pkg->feed ){
		/* entry */
		if( FS_STR_I_EQUAL(name, "ctxt-id") ){
			FS_COPY_TEXT( pkg->entry->id, value );
		}
	}else if( FS_STR_I_EQUAL(ename, "feed") && pkg->feed ){
		/* feed */
		if( FS_STR_I_EQUAL(name, "ctxt-id") ){
			FS_COPY_TEXT( pkg->feed->id, value );
		}
	}
}

static void FS_DcdPkgNote( FS_DcdPkgData * pkg, FS_CHAR *version, FS_CHAR *encoding )
{
	if( encoding )
		FS_COPY_TEXT( pkg->charset, encoding );
}

static void FS_DcdPkgFileRead( FS_DcdPkgData *pkg, FS_SaxHandle hsax )
{
	FS_SINT4 rlen;
	FS_BOOL bDone = FS_FALSE;
	FS_BYTE *buf = IFS_Malloc( FS_FILE_BLOCK );
	if( buf )
	{
		rlen = FS_FileRead( FS_DIR_TMP, pkg->file, pkg->offset, buf, FS_FILE_BLOCK );
		if( rlen < FS_FILE_BLOCK )
			bDone = FS_TRUE;
		pkg->offset += rlen;
		FS_SaxDataFeed( hsax, buf, rlen, bDone );
		IFS_Free( buf );
	}
}

FS_DcdSyncPkg *FS_DcdParseSyncPkg(FS_CHAR *xml_file )
{
	FS_SaxHandle hsax = FS_NULL;
	FS_DcdSyncPkg *pkg;
	FS_DcdPkgData *pkgData;

	pkgData = IFS_Malloc( sizeof(FS_DcdPkgData) );
	if( pkgData == FS_NULL ){
		return FS_NULL;
	}
	IFS_Memset( pkgData, 0, sizeof( FS_DcdPkgData ) );
	pkg = IFS_Malloc( sizeof(FS_DcdSyncPkg) );
	if( pkg == FS_NULL ){
		IFS_Free( pkgData );
		return FS_NULL;
	}
	IFS_Memset( pkg, 0, sizeof(FS_DcdSyncPkg) );
	FS_ListInit( &pkg->channel_list );
	FS_ListInit( &pkg->feed_list );
	pkgData->pkg = pkg;
	pkgData->charset = IFS_Strdup( "UTF-8" );	/* default charset set to UTF-8 */
	pkgData->file = IFS_Strdup( xml_file );
	pkgData->offset = 0;
	
	hsax = FS_CreateSaxHandler( pkgData );
	FS_SaxSetStartElementHandler( hsax, FS_DcdPkgStartElement );
	FS_SaxSetStartElementEndHandler( hsax, FS_DcdPkgStartElementEnd );
	FS_SaxSetEndElementHandler( hsax, FS_DcdPkglEndElement );
	FS_SaxSetElementTextHandler( hsax, FS_DcdPkgElementText );
	FS_SaxSetAttributeHandler( hsax, FS_DcdPkgElementAttr );
	FS_SaxSetXmlNoteHandler( hsax, FS_DcdPkgNote );	
	FS_SaxSetDataRequest( hsax, FS_DcdPkgFileRead );

	FS_SaxProcXmlDoc( hsax );
	
	IFS_Free( pkgData->charset );
	IFS_Free( pkgData->file );
	IFS_Free( pkgData );
	FS_FreeSaxHandler( hsax );
	return pkg;
}

static void FS_DcdFreeChannelFeed( FS_DcdChannelFeed *feed, FS_BOOL del_file )
{
	FS_List *node;
	FS_DcdLink *link;
	FS_DcdEntry *entry;
	FS_DcdContent *content;
	
	FS_SAFE_FREE( feed->id );
	FS_SAFE_FREE( feed->title );
	FS_SAFE_FREE( feed->summary );

	node = feed->link_list.next;
	while( node != &feed->link_list ){
		link = FS_ListEntry( node, FS_DcdLink, list );
		node =  node->next;
		FS_ListDel( &link->list );
		FS_SAFE_FREE( link->title );
		FS_SAFE_FREE( link->href );
		IFS_Free( link );
	}

	node = feed->content_list.next;
	while( node != &feed->content_list ){
		content = FS_ListEntry( node, FS_DcdContent, list );
		node =  node->next;
		FS_ListDel( &content->list );
		FS_SAFE_FREE( content->id );
		FS_SAFE_FREE( content->type );
		FS_SAFE_FREE( content->href );
		if( del_file && content->file ){
			FS_FileDelete( FS_DIR_DCD, content->file );
		}
		FS_SAFE_FREE( content->file );
		IFS_Free( content );
	}

	node = feed->entry_list.next;
	while( node != &feed->entry_list ){
		entry = FS_ListEntry( node, FS_DcdEntry, list );
		node =  node->next;
		FS_ListDel( &entry->list );
		FS_DcdFreeEntry( entry, del_file );
	}

	IFS_Free( feed );
}

void FS_DcdFreeEntry( FS_DcdEntry *entry, FS_BOOL del_file )
{
	FS_List *node2;
	FS_DcdLink *link;
	FS_DcdContent *content;
	
	FS_SAFE_FREE( entry->id );
	FS_SAFE_FREE( entry->title );
	FS_SAFE_FREE( entry->summary );
	FS_SAFE_FREE( entry->author );
	
	node2 = entry->link_list.next;
	while( node2 != &entry->link_list ){
		link = FS_ListEntry( node2, FS_DcdLink, list );
		node2 =  node2->next;
		FS_ListDel( &link->list );
		FS_SAFE_FREE( link->title );
		FS_SAFE_FREE( link->href );
		IFS_Free( link );
	}	
	
	node2 = entry->content_list.next;
	while( node2 != &entry->content_list ){
		content = FS_ListEntry( node2, FS_DcdContent, list );
		node2 =  node2->next;
		FS_ListDel( &content->list );
		FS_SAFE_FREE( content->id );
		FS_SAFE_FREE( content->type );
		FS_SAFE_FREE( content->href );
		if( del_file && content->file ){
			FS_FileDelete( FS_DIR_DCD, content->file );
		}
		FS_SAFE_FREE( content->file );
		IFS_Free( content );
	}
	
	IFS_Free( entry );
}

void FS_DcdFreeSyncPkg( FS_DcdSyncPkg *pkg, FS_BOOL del_file )
{
	FS_List *node;
	FS_DcdChannel *channel;
	FS_DcdChannelFeed *feed;
	
	FS_SAFE_FREE( pkg->global_anchor );
	node = pkg->channel_list.next;
	while( node != &pkg->channel_list ){
		channel = FS_ListEntry( node, FS_DcdChannel, list );
		node = node->next;
		FS_ListDel( &channel->list );
		FS_SAFE_FREE( channel->id );
		FS_SAFE_FREE( channel->anchor );
		IFS_Free( channel );
	}

	node = pkg->feed_list.next;
	while( node != &pkg->feed_list ){
		feed = FS_ListEntry( node, FS_DcdChannelFeed, list );
		node = node->next;
		FS_ListDel( &feed->list );
		FS_DcdFreeChannelFeed( feed, del_file );
	}

	IFS_Free( pkg );
}

FS_List *FS_DcdParseEntryList(FS_CHAR *boundary, FS_BYTE *data, FS_SINT4 len)
{
	typedef enum FS_DcdEntryHeadField_Tag
	{
		FS_H_CONTENT_LENGTH = 0,
		FS_H_CONTENT_TYPE,
		FS_H_CONTENT_ID
	}FS_HttpHeadField;
	
	FS_HeadField hentry[] = 
	{
		{ "Content-Length:",			FS_NULL },
		{ "Content-Type:",				FS_NULL },
		{ "Content-ID:",				FS_NULL },
		// last item
		{ FS_NULL,						FS_NULL }
	};
	FS_SINT4 hnum, bndrylen;
	FS_CHAR *ptr, *buf = FS_NULL, *hp, *ext;
	FS_DcdRspEntry *entry;
	FS_List *entry_list = FS_NULL;
	

	entry_list = IFS_Malloc( sizeof(FS_List) );
	if( entry_list == FS_NULL ){
		return FS_NULL;
	}
	FS_ListInit( entry_list );
	buf = IFS_Malloc( FS_MIME_HEAD_FIELD_MAX_LEN );
	if( buf == FS_NULL ){
		goto ERR_RET;
	}

	bndrylen = IFS_Strlen(boundary);
	ptr = data;
	while( ptr + len >= data ) {
		/* parse entry header */
		if( !FS_IS_BOUNDARY( data, boundary, bndrylen) ){
			goto ERR_RET;
		}
		if( data[bndrylen + 2] == '-' && data[bndrylen + 3] =='-' ){
			break;	/* parse complete */
		}
		entry = IFS_Malloc( sizeof(FS_DcdRspEntry) );
		if( entry == FS_NULL ){
			goto ERR_RET;
		}
		IFS_Memset( entry, 0, sizeof(FS_DcdRspEntry) );
		FS_ListAddTail( entry_list, &entry->list );
		data += bndrylen + 4;	/* prefix -- and trail CRLF */
		while(( hnum = FS_GetOneField( buf, FS_MIME_HEAD_FIELD_MAX_LEN, &data, hentry )) != -1 )
		{
			hp = buf + IFS_Strlen( hentry[hnum].name );
			while (*hp == ' ' || *hp == '\t')
				hp++;

			switch (hnum)
			{
			case FS_H_CONTENT_LENGTH:
				entry->len = IFS_Atoi( hp );
				break;
			case FS_H_CONTENT_TYPE:
				if( ! entry->content_type )
					entry->content_type = IFS_Strdup( hp );
				break;
			case FS_H_CONTENT_ID:
				if( ! entry->content_id )
					entry->content_id = IFS_Strdup( hp );
				break;
			default:
				break;
			}
		}
		if( entry->len == 0 ){
			goto ERR_RET;
		}
		FS_GetGuid( entry->file );
		ext = FS_GetExtFromMime( entry->content_type );
		if( ext != FS_NULL ){
			IFS_Strcat( entry->file, "." );
			IFS_Strcat( entry->file, ext );
		}
		FS_FileWrite( FS_DIR_DCD, entry->file, 0, data, entry->len );
		data += entry->len + 2;	/* skip trail CRLF */
	}
	
	IFS_Free( buf );
	return entry_list;
	
ERR_RET:
	IFS_Free( buf );
	FS_DcdFreeRspEntryList( entry_list );
	return FS_NULL;
}

void FS_DcdFreeRspEntryList( FS_List *entry_list ){
	FS_DcdRspEntry *entry;
	FS_List *node;
	
	if( entry_list == FS_NULL ) return;
	node = entry_list->next;
	while( node != entry_list ){
		entry = FS_ListEntry( node, FS_DcdRspEntry, list );
		node = node->next;
		FS_ListDel( &entry->list );
		FS_SAFE_FREE( entry->content_type );
		FS_SAFE_FREE( entry->content_id );
		IFS_Free( entry );
	}
	IFS_Free( entry_list );
}

#ifdef FS_DEBUG_
#include "inc/util/FS_File.h"

void FS_DcdPkgTest( void )
{
	FS_SINT4 len = FS_FileGetSize(FS_DIR_TMP, "dcd.xml");
	FS_DcdSyncPkg*pkg;
	FS_List *entry_list;
	FS_BYTE *data;
	
	if( len > 0 ){
		pkg = FS_DcdParseSyncPkg( "dcd.xml" );
		FS_DcdFreeSyncPkg( pkg, FS_TRUE );
	}

	len = FS_FileGetSize(FS_DIR_TMP, "multipart.dat");
	if( len > 0 ){
		data = IFS_Malloc( len + 1 );
		FS_FileRead( FS_DIR_TMP, "multipart.dat", 0, data, len );
		data[len] = 0;
		entry_list = FS_DcdParseEntryList("boundary1", data, len);
		FS_DcdFreeRspEntryList( entry_list );
		IFS_Free( data );
	}
}
#endif

#endif // FS_MODULE_DCD

