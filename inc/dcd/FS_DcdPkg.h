#ifndef _FS_DCD_PKG_H_
#define _FS_DCD_PKG_H_
#include "inc/FS_Config.h"
#include "inc/util/FS_File.h"
#include "inc/util/FS_List.h"

#define FS_DCD_FEED_FREE		0	/* 免费频道 */
#define FS_DCD_FEED_CHARGE	1	/* 收费频道 */
#define FS_DCD_FEED_PACKAGE	2	/* 打包收费频道 */

/* DCD频道 */
typedef struct FS_DcdChannel_Tag{
	FS_List			list;
	
	FS_CHAR * 		id;			/* 频道唯一ID标识 */
	FS_CHAR * 		anchor;
	FS_SINT4		charge;
}FS_DcdChannel;

/* DCD链接 */
typedef struct FS_DcdLink_Tag{
	FS_List			list;
	
	FS_CHAR *		title;
	FS_CHAR * 		href;
}FS_DcdLink;

/* DCD内容 */
typedef struct FS_DcdContent_Tag{
	FS_List			list;
	
	FS_CHAR * 		id;			/* 内容的ID，下载内容时要用到这个ID */
	FS_CHAR * 		type;		/* 内容的MIME类型，如text/plain, image/gif */
	FS_CHAR * 		href;		/* 内容的下载地址 */
	FS_CHAR *		file;			/* 内容在本地所对应的文件名 */
	FS_BOOL 		autoplay;	/* 自动播放，对于音频内容有效 */
}FS_DcdContent;

#define FS_DCD_ENTRY_READ		0x01

#define FS_DCD_ENTRY_IS_READ(entry)		((entry)->flag & FS_DCD_ENTRY_READ)
#define FS_DCD_ENTRY_SET_READ(entry)	((entry)->flag |= FS_DCD_ENTRY_READ)

/* 频道项 */
typedef struct FS_DcdEntry_Tag{
	FS_List			list;
	
	FS_CHAR * 		id;	
	FS_CHAR * 		title;
	FS_CHAR * 		summary;
	FS_List  		link_list;
	FS_UINT4 		issue;					/* 频道发布时间 */
	FS_CHAR * 		author;					/* 作者名称 */
	FS_List 		content_list;
	FS_UINT4		flag;					/* 标识: 未读，己删除 etc */
}FS_DcdEntry;

/* DCD频道更新列表 */
typedef struct FS_DcdChannelFeed_Tag{
	FS_List			list;
	
	FS_CHAR * 		id;			/* 频道唯一ID标识 */
	FS_CHAR * 		title;		/* 频道标题 */
	FS_CHAR * 		summary;	/* 频道描述信息 */
	
	FS_List 			link_list;	/* 链接列表，可以有多个 */
	FS_List  			content_list;
	
	FS_List 			entry_list;	/* 频道项列表 */
}FS_DcdChannelFeed;

/* DCD同步消息结构 */
typedef struct FS_DcdSyncPkg_Tag{
	FS_CHAR *		global_anchor;		/* 全局同步定位符 */
	FS_UINT4 		ttl;				/* seconds */
	FS_UINT4 		server_time;		/* seconds */
	
	FS_List 			channel_list;	/* 频道列表 */
	FS_List 			feed_list;		/* 更新的频道内容 */
}FS_DcdSyncPkg;

typedef struct FS_DcdRspEntry_Tag{
	FS_List				list;
	
	FS_CHAR *			content_type;
	FS_CHAR *			content_id;
	FS_SINT4 			len;
	FS_CHAR 			file[FS_FILE_NAME_LEN];
}FS_DcdRspEntry;

/*
 * 解析DCD同步包
 * xml_doc [in] 输入，同步包的数据
 * len [in] 输入，同步包的数据长度，如果len <= 0，则xml_doc必须是以0结尾的字符串
 *
 * return: 
 * 解析出来的dcd同步包结构体。函数内分配内存。
 * 调用者使用后需要调用dcd_sync_pkg_free来释放内存。
 * 解析失败返回NULL
*/ 
FS_DcdSyncPkg *FS_DcdParseSyncPkg( FS_CHAR *xml_file );

/* 释放DCD同步包内存 */
void FS_DcdFreeSyncPkg(FS_DcdSyncPkg *pkg, FS_BOOL del_file);

void FS_DcdFreeEntry( FS_DcdEntry *entry, FS_BOOL del_file );

/* return FS_DcdRspEntry list if success */
FS_List *FS_DcdParseEntryList(FS_CHAR *boundary, FS_BYTE *data, FS_SINT4 len);

void FS_DcdFreeRspEntryList(FS_List* entrys);

#endif

