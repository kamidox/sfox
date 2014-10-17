#include "inc/FS_Config.h"

#ifdef FS_MODULE_WEB

#include "inc/web/FS_History.h"
#include "inc/inte/FS_Inte.h" 
#include "inc/util/FS_Util.h"
#include "inc/util/FS_Sax.h"
#include "inc/util/FS_MemDebug.h"

#define FS_CACHE_FILE		"Cache.bin"
#define FS_BOOKMARK_FILE	"Bookmark.bin"
#define FS_RECMDURL_FILE	"recmdurl.xml"

static FS_List GFS_CacheList = {&GFS_CacheList, &GFS_CacheList};
static FS_List GFS_HistoryStack = { &GFS_HistoryStack, &GFS_HistoryStack };
static FS_HistoryEntry *GFS_HstStkPos = FS_NULL;

static FS_List GFS_Bookmarks = {&GFS_Bookmarks, &GFS_Bookmarks};
static FS_List GFS_RecmdUrlDir = {&GFS_RecmdUrlDir, &GFS_RecmdUrlDir};

#if 0
static FS_RecommandUrl GFS_RecmdUrlList[] = 
{
	{ 1,	"WAP搜索",				FS_NULL },
	{ 1,	"hao123网址导航",		"http://wap.hao123.com" },
	{ 1,	"百度",					"http://wap.baidu.com" },
	{ 1,	"GOOGLE",				"http://www.google.cn/wml" },
	{ 1,	"易查",					"http://wap.yicha.cn" },
	{ 1,	"搜狗WAP",				"http://wap.sogou.com/" },

	{ 2,	"WAP门户",		FS_NULL },
	{ 2,	"搜狐手机",		"http://wap.sohu.com" },
	{ 2,	"新浪手机",		"http://3g.sina.com.cn" },
	{ 2,	"3G门户",		"http://3g.net.cn" },
	{ 2,	"网易",			"http://wap.163.com" },
	{ 2,	"手机之家",		"http://17wap.com/" },
	{ 2,	"空中网",		"http://kong.net/" },
	{ 2,	"WAP天下",		"http://waptx.com/" },
	{ 2,	"捉鱼",			"http://wap.joyes.com/" },
	{ 2,	"腾讯QQ",		"http://3g.qq.com" },

	{ 3,	"WAP书城",		FS_NULL },
	{ 3,	"起点移动书库",	"http://wap.cmfu.com/" },
	{ 3,	"乐讯",			"http://wap.lxyes.com/" },
	{ 3,	"星空玄幻小说",	"http://wap.3gair.com/" },
	{ 3,	"17K书城",		"http://wap.17k.com/" },
	{ 3,	"掌上书城",		"http://5ibk.com/catalog.jsp?id=0" },
	{ 3,	"小说天下",		"http://wapfj.dwap.cn" },
	{ 3,	"新梦网典",		"http://kj.dwap.cn/" },
	{ 3,	"逍遥掌",		"http://wap.goxoyo.cn/" },
	{ 3,	"鱼儿书屋",		"http://wap.yuerw.com/book/" },
	
	{ 4,	"其它推荐",		FS_NULL },
	{ 4,	"邦邦网",		"http://bang.cn/" },
	{ 4,	"小虎在线",		"http://wap.hucn.net" },
	{ 4,	"乐讯",			"http://wap.lxyes.com/" },
	{ 4,	"摩网",			"http://wap.moabc.com" },
	{ 4,	"当乐网",		"http://wap.downjoy.com/" },
	{ 4,	"手机加油站",	"http://wap.sjjyz.com" },
	{ 4,	"万蝶移动博客",	"http://wap.pdx.cn/pdx/" },
	{ 4,	"动感网",		"http://wap.mozone.cn/" },
	{ 4,	"DTOP娱乐",		"http://dtop.cn/" },
	{ 4,	"稀饭网",		"http://wap.xifan.cn/" },
	{ 4,	"掌景无限",		"http://wap.funvio.com/" },
	{ 4,	"风风网",		"http://wap.7234.com/" },
	{ 4,	"蘑菇无线",		"http://wap.mogoo.cn" },
	{ 4,	"思酷门户",		"http://wap.skooo.com/" },
	{ 4,	"3G无限",		"http://wap.3gmax.cn" },
	{ 4,	"彩蛙网",		"http://wap.haha.com.cn/" },
	{ 4,	"黑客WAP站",	"http://18kp.com" },
	{ 4,	"比分在线",		"http://wap.wapzq.com/" },
	{ 4,	"wap世纪",		"http://waps.cn/" },
	{ 4,	"拇哈网",		"http://moha.cn" },
	{ 4,	"搜蛙社区",		"http://sowap.cn" },
	{ 4,	"掌握中国",		"http://wap.zhangwo.cn" },
	{ 4,	"凤凰网",		"http://wap.phoenixtv.com/" },
	{ 4,	"3G泡泡",		"http://3g.pp.cn/" },
		
	{ 5,	"社区论坛",		FS_NULL },
	{ 5,	"手机天涯",		"http://wap.tianya.cn/" },
	{ 5,	"百度贴吧",		"http://wapp.baidu.com/" },
		
	{ 0, FS_NULL, FS_NULL }
};
#endif

typedef struct FS_RecmdUrlDoc_Tag
{
	FS_RecommandUrlDir *dir;
	FS_RecommandUrl *url;
	FS_CHAR *charset;
	FS_SINT4 offset;
	FS_CHAR *file;
}FS_RecmdUrlDoc;

void FS_RecmdUrlFileRead( FS_RecmdUrlDoc *doc, FS_SaxHandle hsax )
{
	FS_SINT4 rlen;
	FS_BOOL bDone = FS_FALSE;
	FS_BYTE *buf = IFS_Malloc( FS_FILE_BLOCK );
	if( buf )
	{
		rlen = FS_FileRead( FS_DIR_WEB, doc->file, doc->offset, buf, FS_FILE_BLOCK );
		if( rlen < FS_FILE_BLOCK )
			bDone = FS_TRUE;
		doc->offset += rlen;
		FS_SaxDataFeed( hsax, buf, rlen, bDone );
		IFS_Free( buf );
	}
}

static void FS_RecmdUrlStartElement( FS_RecmdUrlDoc * doc, FS_CHAR *ename )
{
	if( FS_STR_I_EQUAL(ename, "dir") ){
		doc->dir = IFS_Malloc( sizeof(FS_RecommandUrlDir) );
		IFS_Memset( doc->dir, 0, sizeof(FS_RecommandUrlDir) );
		FS_ListInit( &doc->dir->url_list );
	}else if( FS_STR_I_EQUAL(ename, "entry") ){
		doc->url = IFS_Malloc( sizeof(FS_RecommandUrl) );
		IFS_Memset( doc->url, 0, sizeof(FS_RecommandUrl) );
	}
}

static void FS_RecmdUrlEndElement( FS_RecmdUrlDoc * doc, FS_CHAR *ename )
{
	if( doc->dir == FS_NULL ) return;
	
	if( FS_STR_I_EQUAL(ename, "dir") ){
		FS_ListAddTail( &GFS_RecmdUrlDir, &doc->dir->list );
		doc->dir = FS_NULL;
	}else if( FS_STR_I_EQUAL(ename, "entry") ){
		FS_ListAddTail( &doc->dir->url_list, &doc->url->list );
		doc->url = FS_NULL;
	}
}

static void FS_RecmdUrlElementAttr( FS_RecmdUrlDoc * doc, FS_CHAR *ename, FS_CHAR *name, FS_CHAR *value )
{
	if( doc->dir == FS_NULL ) return;
	if( FS_STR_I_EQUAL(ename, "dir") && FS_STR_I_EQUAL(name, "name")){
		doc->dir->title = FS_ProcessCharset(value, -1, doc->charset, FS_NULL );
		if( doc->dir->title == FS_NULL )
			doc->dir->title = IFS_Strdup( value );
	}else if( FS_STR_I_EQUAL(ename, "entry") && doc->url ){
		if( FS_STR_I_EQUAL(name, "name") ){
			doc->url->title = FS_ProcessCharset(value, -1, doc->charset, FS_NULL );
			if( doc->url->title == FS_NULL )
				doc->url->title = IFS_Strdup( value );
		}else if( FS_STR_I_EQUAL(name, "url") ){
			doc->url->url = IFS_Strdup( value );
		}
	}
}

static void FS_RecmdUrlDocNote( FS_RecmdUrlDoc * doc, FS_CHAR *version, FS_CHAR *encoding )
{
	if( encoding )
		FS_COPY_TEXT( doc->charset, encoding );
}

static void FS_RecommandUrlInit( void )
{
	FS_SaxHandle hsax;
	FS_RecmdUrlDoc *doc;

	doc = IFS_Malloc( sizeof(FS_RecmdUrlDoc) );
	IFS_Memset( doc, 0, sizeof(FS_RecmdUrlDoc) );
	
	doc->file = FS_RECMDURL_FILE;
	doc->charset = IFS_Strdup( "UTF-8" );
	hsax = FS_CreateSaxHandler( doc );
	FS_SaxSetDataRequest( hsax, FS_RecmdUrlFileRead );
	FS_SaxSetStartElementHandler( hsax, FS_RecmdUrlStartElement );
	FS_SaxSetEndElementHandler( hsax, FS_RecmdUrlEndElement );
	FS_SaxSetAttributeHandler( hsax, FS_RecmdUrlElementAttr );
	FS_SaxSetXmlNoteHandler( hsax, FS_RecmdUrlDocNote );
	FS_SaxProcXmlDoc( hsax );

	IFS_Free( doc->charset );
	IFS_Free( doc );
	FS_FreeSaxHandler( hsax );
}

static void FS_RecommandUrlDeinit( void )
{
	FS_List *dnode, *unode;
	FS_RecommandUrlDir *dir;
	FS_RecommandUrl *url;

	dnode = GFS_RecmdUrlDir.next;
	while( dnode != &GFS_RecmdUrlDir ){
		dir = FS_ListEntry( dnode, FS_RecommandUrlDir, list );
		dnode = dnode->next;

		unode = dir->url_list.next;
		while( unode != &dir->url_list ){
			url = FS_ListEntry( unode, FS_RecommandUrl, list );
			unode = unode->next;

			FS_ListDel( &url->list );
			FS_SAFE_FREE( url->title );
			FS_SAFE_FREE( url->url );
			IFS_Free( url );
		}
		FS_ListDel( &dir->list );
		FS_SAFE_FREE( dir->title );
		IFS_Free( dir );
	}
}

static void FS_CacheCheckLimit( FS_SINT4 nSize, FS_SINT4 nItem )
{
	FS_List *node;
	FS_HistoryEntry *he;
	FS_SINT4 nMaxItems = IFS_GetMaxCacheItem( );
	FS_SINT4 nMaxSizes = IFS_GetMaxCacheSize( );

	if( nItem >= nMaxItems )
	{
		/* cache items exceed. delete some old items */ 
		node = GFS_CacheList.next;
		while( node != &GFS_CacheList )
		{
			he = FS_ListEntry( node, FS_HistoryEntry, list );
			node = node->next;
			
			FS_ListDel( &he->list );
			if( he->size <= 0 )
				he->size = FS_FileGetSize( FS_DIR_WEB, he->file );
			nSize -= he->size;
			nItem --;
			FS_FileDelete( FS_DIR_WEB, he->file );
			FS_SAFE_FREE( he->url );
			IFS_Free( he );

			if( nItem < nMaxItems )
				break;
		}
	}

	if( nSize >= nMaxSizes )
	{
		/* cache size exceed. delete some old items */ 
		node = GFS_CacheList.next;
		while( node != &GFS_CacheList )
		{
			he = FS_ListEntry( node, FS_HistoryEntry, list );
			node = node->next;
			
			FS_ListDel( &he->list );
			if( he->size <= 0 )
				he->size = FS_FileGetSize( FS_DIR_WEB, he->file );
			nSize -= he->size;
			FS_FileDelete( FS_DIR_WEB, he->file );
			FS_SAFE_FREE( he->url );
			IFS_Free( he );
		
			if( nSize < nMaxSizes )
				break;
		}
	}
}

static FS_SINT4 FS_CacheEntrySize( FS_HistoryEntry *he )
{
	FS_SINT4 len = 1;
	if( he->url )
		len += IFS_Strlen( he->url );
	return len + sizeof(FS_HistoryEntry);
}

static FS_SINT4 FS_CacheEntryToBuffer( FS_BYTE *buf, FS_HistoryEntry *he )
{
	FS_SINT4 len = 1, size = sizeof(FS_HistoryEntry);
	IFS_Memcpy( buf, he, size );
	if( he->url )
	{
		len += IFS_Strlen( he->url );
		IFS_Memcpy( buf + size, he->url, len );
	}
	else
	{
		buf[size] = '\0';
	}
	return size + len;
}

static FS_SINT4 FS_CacheEntryFromBuffer( FS_BYTE *buf, FS_HistoryEntry *he )
{
	FS_SINT4 len = 1, size = sizeof(FS_HistoryEntry);
	
	IFS_Memcpy( he, buf, size );
	if( buf[size] != '\0' )
	{
		he->url = IFS_Strdup( buf + size );
		len += IFS_Strlen( buf + size );
	}
	else
	{
		he->url = FS_NULL;
	}
	return size + len;
}

static void FS_CacheListWriteToFile( void )
{
	FS_Handle hFile;
	FS_List *node;
	FS_HistoryEntry * he;
	FS_BYTE *buf, *pos;
	FS_SINT4 size = 0;
	
	node = GFS_CacheList.next;
	
	if( FS_FileCreate( &hFile, FS_DIR_WEB, FS_CACHE_FILE, FS_OPEN_WRITE ) )
	{
		buf = IFS_Malloc( FS_FILE_BLOCK );
		if( buf )
		{
			pos = buf;
			while( node != &GFS_CacheList )
			{
				he = FS_ListEntry( node, FS_HistoryEntry, list );
				size = (FS_SINT4)(pos - buf);
				if( size + FS_CacheEntrySize(he) >= FS_FILE_BLOCK )
				{
					IFS_FileWrite( hFile, buf, size );
					pos = buf;
				}
				pos += FS_CacheEntryToBuffer( pos, he );
				node = node->next;
			}
			size = (FS_SINT4)(pos - buf);
			FS_ASSERT( size < FS_FILE_BLOCK );
			if( size > 0 )
				IFS_FileWrite( hFile, buf, size );
			IFS_Free( buf );
		}
		IFS_FileClose( hFile );
	}
}

static void FS_CacheListReadFromFile( void )
{
	FS_Handle hFile;
	FS_BYTE *buf, *pos;
	FS_UINT4 i;
	FS_HistoryEntry * he;

	FS_CacheDeinit( );
	
	if( FS_FileOpen( &hFile, FS_DIR_WEB, FS_CACHE_FILE, FS_OPEN_READ ) )
	{
		FS_SINT4 size = IFS_FileGetSize( hFile );
		if( size > 0 )
		{
			buf = IFS_Malloc( size );
			if( buf )
			{
				if( IFS_FileRead( hFile, buf, size ) == size )
				{
					pos = buf;
					for( i = 0; (pos - buf) < size; i ++ ) 
					{
						he = IFS_Malloc( sizeof(FS_HistoryEntry) );
						if( he )
						{
							pos += FS_CacheEntryFromBuffer( pos, he );
							FS_ListAddTail( &GFS_CacheList, &he->list );
						}
					}
				}
				IFS_Free( buf );
			}
		}
		IFS_FileClose( hFile );
	}
}

static void FS_BookmarkListWriteToFile( void )
{
	FS_Handle hFile;
	FS_List *node;
	FS_Bookmark * bmk;
	FS_BYTE *buf, *pos;
	FS_SINT4 size = 0;
	
	node = GFS_Bookmarks.next;
	
	if( FS_FileCreate( &hFile, FS_DIR_WEB, FS_BOOKMARK_FILE, FS_OPEN_WRITE ) )
	{
		buf = IFS_Malloc( FS_FILE_BLOCK );
		if( buf )
		{
			pos = buf;
			while( node != &GFS_Bookmarks )
			{
				bmk = FS_ListEntry( node, FS_Bookmark, list );
				size = (FS_SINT4)(pos - buf);
				if( size + sizeof(FS_Bookmark) >= FS_FILE_BLOCK )
				{
					IFS_FileWrite( hFile, buf, size );
					pos = buf;
				}
				IFS_Memcpy( pos, bmk, sizeof(FS_Bookmark) );
				pos += sizeof(FS_Bookmark);
				node = node->next;
			}
			size = (FS_SINT4)(pos - buf);
			if( size > 0 )
				IFS_FileWrite( hFile, buf, size );
			IFS_Free( buf );
		}
		IFS_FileClose( hFile );
	}
}

static void FS_BookmarkListReadFromFile( void )
{
	FS_Handle hFile;
	FS_BYTE *buf, *pos;
	FS_UINT4 i;
	FS_Bookmark * bmk;
	
	FS_BookmarkDeinit( );
	
	if( FS_FileOpen( &hFile, FS_DIR_WEB, FS_BOOKMARK_FILE, FS_OPEN_READ ) )
	{
		FS_SINT4 size = IFS_FileGetSize( hFile );
		if( size > 0 && (size % sizeof(FS_Bookmark) == 0) )
		{
			buf = IFS_Malloc( size );
			if( buf )
			{
				if( IFS_FileRead( hFile, buf, size ) == size )
				{
					pos = buf;
					for( i = 0; (pos - buf) < size; i ++ ) 
					{
						bmk = IFS_Malloc( sizeof(FS_Bookmark) );
						if( bmk )
						{
							IFS_Memcpy( bmk, pos, sizeof(FS_Bookmark) );
							FS_ListAddTail( &GFS_Bookmarks, &bmk->list );
						}
						pos += sizeof(FS_Bookmark);
					}
				}
				IFS_Free( buf );
			}
		}
		IFS_FileClose( hFile );
	}
}

/* here we will do some rabish clean here. delete some file and limit cache size */
FS_HistoryEntry *FS_CacheFindEntry( FS_CHAR *url )
{
	FS_SINT4 nSize = 0, nItems = 0;
	FS_HistoryEntry *he;
	FS_List *node = GFS_CacheList.next;
	while( node != &GFS_CacheList )
	{
		he = FS_ListEntry( node, FS_HistoryEntry, list );
		node = node->next;
		if( FS_STR_I_EQUAL(url, he->url) )
		{
			if( he->size <= 0 )
				he->size = FS_FileGetSize( FS_DIR_WEB, he->file );
			
			if( he->size > 0 && FS_FileGetSize(FS_DIR_WEB, he->file) == he->size )
			{
				return he;
			}
			else
			{
				/* an empty file. delete it */
				FS_FileDelete( FS_DIR_WEB, he->file );
				FS_ListDel( &he->list );
				FS_SAFE_FREE( he->url );
				IFS_Free( he );
				return FS_NULL;
			}
		}
		nSize += he->size;
		nItems ++;
	}

	/* didnot find a entry. we may do some clean here */
	FS_CacheCheckLimit( nSize, nItems );
	return FS_NULL;
}

void FS_CacheEntrySetTitle( FS_CHAR *url, FS_CHAR *title )
{
	FS_HistoryEntry *he;
	if( ! url || ! title || title[0] == 0 ) return;

	he = FS_CacheFindEntry( url );
	if( he )
	{
		IFS_Strncpy( he->title, title, sizeof(he->title) - 2 );
		FS_CacheListWriteToFile( );
	}
}

FS_CHAR *FS_CacheFindFile( FS_CHAR *url )
{
	FS_CHAR *ret = FS_NULL;
	FS_HistoryEntry *he = FS_CacheFindEntry( url );
	if( he )
	{
		ret = he->file;
	}

	return ret;
}

FS_HistoryEntry *FS_CacheFindEntryByFile( FS_CHAR *file )
{
	FS_HistoryEntry *he;
	FS_List *node = GFS_CacheList.next;

	if( file == FS_NULL ) return FS_NULL;
	
	while( node != &GFS_CacheList )
	{
		he = FS_ListEntry( node, FS_HistoryEntry, list );
		node = node->next;
		if( IFS_Stricmp(file, he->file) == 0 )
		{
			if( he->size > 0 && FS_FileGetSize(FS_DIR_WEB, he->file) == he->size )
			{
				return he;
			}
			else
			{
				/* an empty file. delete it */
				FS_FileDelete( FS_DIR_WEB, he->file );
				FS_ListDel( &he->list );
				FS_SAFE_FREE( he->url );
				IFS_Free( he );
				return FS_NULL;
			}
		}
	}
	return FS_NULL;
}

void FS_CacheAddEntry( FS_CHAR *url, FS_CHAR *file, FS_CHAR *charset )
{
	FS_HistoryEntry *he;
	
	he = FS_CacheFindEntry( url );
	/* already exist in cache, update it */
	if( he )
	{
		FS_FileDelete( FS_DIR_WEB, he->file );
		IFS_Strcpy( he->file, file );
		IFS_GetDateTime( &he->date );
		he->size = 0;
		FS_ListDel( &he->list );
		FS_ListAddTail( &GFS_CacheList, &he->list );
	}
	else
	{
		/* add new entry to history stack */
		he = IFS_Malloc( sizeof(FS_HistoryEntry) );
		if( he )
		{
			IFS_Memset( he, 0, sizeof(FS_HistoryEntry) );
			he->url = IFS_Strdup( url );
			IFS_Strcpy( he->file, file );
			if( charset ) IFS_Strncpy( he->charset, charset, FS_CHARSET_LEN - 2 );
			IFS_GetDateTime( &he->date );
			he->size = 0;
			FS_ListAddTail( &GFS_CacheList, &he->list );
		}
	}
	
	FS_CacheListWriteToFile( );
}

FS_CHAR *FS_HistoryCurUrlDir( void )
{
	FS_CHAR *dir = FS_NULL;

	if( GFS_HstStkPos )
	{
		if( GFS_HstStkPos->url )
		{
			dir = FS_UrlGetDir( GFS_HstStkPos->url );
		}
	}
	return dir;
}

FS_CHAR *FS_HistoryCurUrl( void )
{
	if( GFS_HstStkPos )
		return GFS_HstStkPos->url;
	else
		return FS_NULL;
}

FS_CHAR *FS_HistoryCurHost( void )
{
	FS_CHAR *host = FS_NULL;
	
	if( GFS_HstStkPos )
	{
		if( GFS_HstStkPos->url )
		{
			host = FS_UrlGetHost( GFS_HstStkPos->url );
		}
	}
	return host;
}

FS_CHAR *FS_HistoryCurFile( void )
{
	if( GFS_HstStkPos )
		return GFS_HstStkPos->file;
	else
		return FS_NULL;
}

FS_CHAR *FS_HistoryBack( void )
{
	FS_CHAR *ret = FS_NULL;
	FS_HistoryEntry *he;
	FS_List *node;
	if( GFS_HstStkPos )
	{
		node = GFS_HstStkPos->list.next;
		if( node != &GFS_HistoryStack )
		{
			he = FS_ListEntry( node, FS_HistoryEntry, list );
			if( FS_FileGetSize(FS_DIR_WEB, he->file) > 0 )
			{
				ret = he->file;
				GFS_HstStkPos = he;
			}
			else
			{
				ret = FS_NULL;
				FS_ListDel( &he->list );
				FS_FileDelete( FS_DIR_WEB, he->file );
				FS_SAFE_FREE( he->url );
				IFS_Free( he );
			}
		}
	}
	return ret;
}

FS_CHAR *FS_HistoryForward( void )
{
	FS_CHAR *ret = FS_NULL;
	FS_HistoryEntry *he;
	FS_List *node;

	if( GFS_HstStkPos )
	{
		node = GFS_HstStkPos->list.prev;
		if( node != &GFS_HistoryStack )
		{
			he = FS_ListEntry( node, FS_HistoryEntry, list );
			ret = he->file;
			GFS_HstStkPos = he;
		}
	}
	return ret;
}

void FS_HistoryPushEntry( FS_CHAR *url, FS_CHAR *file )
{
	FS_HistoryEntry *he, *te;
	FS_List *node;
	/* add new entry to history stack */
	he = IFS_Malloc( sizeof(FS_HistoryEntry) );
	if( he )
	{
		IFS_Memset( he, 0, sizeof(FS_HistoryEntry) );
		he->url = IFS_Strdup( url );
		IFS_Strcpy( he->file, file );
		IFS_GetDateTime( &he->date );
		if( GFS_HstStkPos )
		{
			/* remove all forward entry */
			node = GFS_HstStkPos->list.prev;
			while( node != &GFS_HistoryStack )
			{
				te = FS_ListEntry( node, FS_HistoryEntry, list );
				node = node->prev;
				FS_ListDel( &te->list );
				IFS_Free( te->url );
				IFS_Free( te );
			}
			
			FS_ListAddTail( &GFS_HstStkPos->list, &he->list );
		}
		else
		{
			FS_ListAddTail( &GFS_HistoryStack, &he->list );
		}
		
		GFS_HstStkPos = he;
	}
}

FS_HistoryEntry *FS_HistoryFindEntry( FS_CHAR *url )
{
	FS_HistoryEntry *he;
	FS_List *node = GFS_HistoryStack.next;
	while( url && node != &GFS_HistoryStack )
	{
		he = FS_ListEntry( node, FS_HistoryEntry, list );
		node = node->next;
		if( FS_STR_I_EQUAL(url, he->url) )
			return he;
	}
	return FS_NULL;
}

void FS_HistorySetViewport( FS_CHAR *url, FS_SINT4 view_port )
{
	FS_HistoryEntry *he = FS_HistoryFindEntry( url );
	if( he )
	{
		he->view_port = view_port;
	}
}

FS_SINT4 FS_HistoryFindEntryViewportByFile( FS_CHAR * file )
{
	FS_HistoryEntry *he;
	FS_List *node = GFS_HistoryStack.next;
	while( node != &GFS_HistoryStack )
	{
		he = FS_ListEntry( node, FS_HistoryEntry, list );
		node = node->next;
		if( IFS_Stricmp(file, he->file) == 0 )
			return he->view_port;
	}
	return 0;
}

void FS_HistorySetCurrent( FS_CHAR *url )
{
	FS_HistoryEntry *he = FS_HistoryFindEntry( url );
	if( he )
	{
		GFS_HstStkPos = he;
	}
}

FS_SINT4 FS_CacheGetSize( void )
{
	FS_SINT4 size = 0;
	FS_HistoryEntry *he;
	FS_List *node = GFS_CacheList.next;
	while( node != &GFS_CacheList )
	{
		he = FS_ListEntry( node, FS_HistoryEntry, list );
		node = node->next;
		if( he->size <= 0 )
			he->size = FS_FileGetSize( FS_DIR_WEB, he->file );
		size += he->size;
	}

	return size;
}

FS_List *FS_CacheGetList( void )
{
	return &GFS_CacheList;
}

void FS_CacheDeleteAll( void )
{
	FS_HistoryEntry *he;
	FS_List *node = GFS_CacheList.next;
	while( node != &GFS_CacheList )
	{
		he = FS_ListEntry( node, FS_HistoryEntry, list );
		node = node->next;
		FS_ListDel( &he->list );

		FS_FileDelete( FS_DIR_WEB, he->file );
		FS_SAFE_FREE( he->url );
		IFS_Free( he );
	}

	FS_FileDelete( FS_DIR_WEB, FS_CACHE_FILE );
}

void FS_HistoryDeinit( void )
{
	FS_HistoryEntry *he;
	FS_List *node;
	GFS_HstStkPos = FS_NULL;
	node = GFS_HistoryStack.next;
	while( node != &GFS_HistoryStack )
	{
		he = FS_ListEntry( node, FS_HistoryEntry, list );
		node = node->next;
		FS_ListDel( &he->list );
		FS_SAFE_FREE( he->url );
		IFS_Free( he );
	}
}

void FS_CacheDeinit( void )
{
	FS_HistoryEntry *he;
	FS_List *node = GFS_CacheList.next;
	while( node != &GFS_CacheList )
	{
		he = FS_ListEntry( node, FS_HistoryEntry, list );
		node = node->next;
		FS_ListDel( &he->list );	
		FS_SAFE_FREE( he->url );
		IFS_Free( he );
	}

	FS_HistoryDeinit( );
	FS_RecommandUrlDeinit( );
}

void FS_CacheInit( void )
{
	FS_CacheListReadFromFile( );
}

void FS_BookmarkDeinit( void )
{
	FS_List *node;
	FS_Bookmark *bmk;
	
	node = GFS_Bookmarks.next;
	while( node != &GFS_Bookmarks )
	{
		bmk = FS_ListEntry( node, FS_Bookmark, list );
		node = node->next;
		FS_ListDel( &bmk->list );
		IFS_Free( bmk );
	}
}

FS_Bookmark * FS_BookmarkAddItem( FS_CHAR *title, FS_CHAR *url, FS_UINT4 id )
{
	FS_Bookmark *bmk;
	FS_List *bmkList;
	FS_CHAR str[16];

	bmkList = FS_BookmarkGetList( );	/* this will make lazy init */
	
	bmk = IFS_Malloc( sizeof(FS_Bookmark) );
	if( bmk )
	{
		IFS_Memset( bmk, 0, sizeof(FS_Bookmark) );
		bmk->id = id;
		if( title ) IFS_Strncpy( bmk->title, title, sizeof(bmk->title) - 2 );
		if( url ) IFS_Strncpy( bmk->url, url, sizeof(bmk->url) - 2 );
		if( FS_BMK_IS_DIR(bmk) )
		{
			FS_GetGuid( str );
			bmk->id = IFS_Atoi( str );
		}
		FS_ListAddTail( bmkList, &bmk->list );
		FS_BookmarkListWriteToFile( );
	}

	return bmk;
}

void FS_BookmarkDelItem( FS_Bookmark *bmk )
{
	if( bmk && ! FS_BMK_IS_DIR(bmk) )
	{
		FS_ListDel( &bmk->list );
		IFS_Free( bmk );
		FS_BookmarkListWriteToFile( );
	}
	else if( bmk && FS_BMK_IS_DIR(bmk) )
	{
		FS_UINT4 id;
		FS_List *head, *node;

		id = bmk->id;
		head = FS_BookmarkGetList( );
		node = head->next;
		while( node != head )
		{
			bmk = FS_ListEntry( node, FS_Bookmark, list );
			node = node->next;

			if( bmk->id == id )
			{
				FS_ListDel( &bmk->list );
				IFS_Free( bmk );
			}
		}
		FS_BookmarkListWriteToFile( );
	}
}

void FS_BookmarkSave( void )
{	
	FS_BookmarkListWriteToFile( );
}

FS_List *FS_BookmarkGetList( void )
{
	if( FS_ListIsEmpty(&GFS_Bookmarks) )
	{
		/* lazy read from bookmark file */
		FS_BookmarkListReadFromFile( );
	}
	
	return &GFS_Bookmarks;
}

FS_SINT4 FS_BookmarkGetCount( void )
{
	FS_List *bmkList = FS_BookmarkGetList( );
	return FS_ListCount( bmkList );
}

FS_SINT4 FS_BookmarkGetDirCount( void )
{
	FS_List *node;
	FS_Bookmark *bmk;
	FS_SINT4 ret = 0;
	FS_List *bmkList = FS_BookmarkGetList( );
	node = bmkList->next;
	while( node != bmkList )
	{
		bmk = FS_ListEntry( node, FS_Bookmark, list );
		node = node->next;
		if( FS_BMK_IS_DIR(bmk) )
			ret ++;
	}
	return ret;
}

FS_List *FS_RecmdUrlGetList( void )
{
	if( FS_ListIsEmpty( &GFS_RecmdUrlDir ) ){
		FS_RecommandUrlInit( );
	}
	return &GFS_RecmdUrlDir;
}

#endif	//FS_MODULE_WEB


