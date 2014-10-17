#ifndef _FS_HISTORY_H_
#define _FS_HISTORY_H_

#include "inc\FS_Config.h"
#include "inc\util\FS_File.h"
#include "inc\util\FS_List.h"

#define FS_BM_TITLE_LEN		32
#define FS_CHARSET_LEN		16
#define FS_HS_TITLE_LEN		32

typedef struct FS_HistoryEntry_Tag
{
	FS_List			list;
	FS_CHAR	*		url;
	FS_DateTime		date;
	FS_CHAR			title[FS_HS_TITLE_LEN];
	FS_CHAR			file[FS_FILE_NAME_LEN];	
	FS_CHAR			charset[FS_CHARSET_LEN];
	FS_SINT4		view_port;
	FS_SINT4		size;
}FS_HistoryEntry;

typedef struct FS_Bookmark_Tag
{
	FS_List			list;
	FS_UINT4		id;
	FS_CHAR			title[FS_BM_TITLE_LEN];
	FS_CHAR			url[FS_URL_LEN];
}FS_Bookmark;

typedef struct FS_RecommandUrl_Tag
{
	FS_List			list;
	FS_CHAR *		title;
	FS_CHAR *		url;
}FS_RecommandUrl;

typedef struct FS_RecmmandUrlDir_Tag
{
	FS_List			list;
	FS_CHAR	*		title;
	FS_List			url_list;
}FS_RecommandUrlDir;

#define FS_BMK_IS_DIR(bmk)					(!((bmk)->url[0]))
#define FS_BMK_OWN_TO_DIR(bmk, dir)		((bmk)->id == (dir)->id)

/*------------------------------------------- History API -------------------------------------------*/
FS_CHAR *FS_HistoryCurUrlDir( void );

FS_CHAR *FS_HistoryCurHost( void );

FS_CHAR *FS_HistoryCurFile( void );

FS_CHAR *FS_HistoryCurUrl( void );

FS_CHAR *FS_HistoryBack( void );

FS_CHAR *FS_HistoryForward( void );

void FS_HistorySetCurrent( FS_CHAR *url );
	
void FS_HistoryPushEntry( FS_CHAR *url, FS_CHAR *file );

FS_HistoryEntry *FS_HistoryFindEntry( FS_CHAR *url );

void FS_HistorySetViewport( FS_CHAR *url, FS_SINT4 view_port );

FS_SINT4 FS_HistoryFindEntryViewportByFile( FS_CHAR * file );

/*------------------------------------------- Cache API -------------------------------------------*/
FS_CHAR *FS_CacheFindFile( FS_CHAR *url );

FS_HistoryEntry *FS_CacheFindEntryByFile( FS_CHAR *file );

FS_List *FS_CacheGetList( void );

FS_SINT4 FS_CacheGetSize( void );

void FS_CacheDeleteAll( void );

void FS_CacheDeinit( void );

void FS_CacheInit( void );

void FS_HistoryDeinit( void );

FS_HistoryEntry *FS_CacheFindEntry( FS_CHAR *url );

void FS_CacheAddEntry( FS_CHAR *url, FS_CHAR *file, FS_CHAR *charset );

void FS_CacheEntrySetTitle( FS_CHAR *url, FS_CHAR *title );

/*------------------------------------------- Bookmark API -------------------------------------------*/
FS_List *FS_BookmarkGetList( void );

void FS_BookmarkDeinit( void );

FS_Bookmark * FS_BookmarkAddItem( FS_CHAR *title, FS_CHAR *url, FS_UINT4 id );

void FS_BookmarkDelItem( FS_Bookmark *bmk );

void FS_BookmarkSave( void );

FS_SINT4 FS_BookmarkGetCount( void );

FS_SINT4 FS_BookmarkGetDirCount( void );

FS_List *FS_RecmdUrlGetList( void );

#endif
