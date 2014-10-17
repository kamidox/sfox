#ifndef _FS_DCD_PKG_H_
#define _FS_DCD_PKG_H_
#include "inc/FS_Config.h"
#include "inc/util/FS_File.h"
#include "inc/util/FS_List.h"

#define FS_DCD_FEED_FREE		0	/* ���Ƶ�� */
#define FS_DCD_FEED_CHARGE	1	/* �շ�Ƶ�� */
#define FS_DCD_FEED_PACKAGE	2	/* ����շ�Ƶ�� */

/* DCDƵ�� */
typedef struct FS_DcdChannel_Tag{
	FS_List			list;
	
	FS_CHAR * 		id;			/* Ƶ��ΨһID��ʶ */
	FS_CHAR * 		anchor;
	FS_SINT4		charge;
}FS_DcdChannel;

/* DCD���� */
typedef struct FS_DcdLink_Tag{
	FS_List			list;
	
	FS_CHAR *		title;
	FS_CHAR * 		href;
}FS_DcdLink;

/* DCD���� */
typedef struct FS_DcdContent_Tag{
	FS_List			list;
	
	FS_CHAR * 		id;			/* ���ݵ�ID����������ʱҪ�õ����ID */
	FS_CHAR * 		type;		/* ���ݵ�MIME���ͣ���text/plain, image/gif */
	FS_CHAR * 		href;		/* ���ݵ����ص�ַ */
	FS_CHAR *		file;			/* �����ڱ�������Ӧ���ļ��� */
	FS_BOOL 		autoplay;	/* �Զ����ţ�������Ƶ������Ч */
}FS_DcdContent;

#define FS_DCD_ENTRY_READ		0x01

#define FS_DCD_ENTRY_IS_READ(entry)		((entry)->flag & FS_DCD_ENTRY_READ)
#define FS_DCD_ENTRY_SET_READ(entry)	((entry)->flag |= FS_DCD_ENTRY_READ)

/* Ƶ���� */
typedef struct FS_DcdEntry_Tag{
	FS_List			list;
	
	FS_CHAR * 		id;	
	FS_CHAR * 		title;
	FS_CHAR * 		summary;
	FS_List  		link_list;
	FS_UINT4 		issue;					/* Ƶ������ʱ�� */
	FS_CHAR * 		author;					/* �������� */
	FS_List 		content_list;
	FS_UINT4		flag;					/* ��ʶ: δ������ɾ�� etc */
}FS_DcdEntry;

/* DCDƵ�������б� */
typedef struct FS_DcdChannelFeed_Tag{
	FS_List			list;
	
	FS_CHAR * 		id;			/* Ƶ��ΨһID��ʶ */
	FS_CHAR * 		title;		/* Ƶ������ */
	FS_CHAR * 		summary;	/* Ƶ��������Ϣ */
	
	FS_List 			link_list;	/* �����б������ж�� */
	FS_List  			content_list;
	
	FS_List 			entry_list;	/* Ƶ�����б� */
}FS_DcdChannelFeed;

/* DCDͬ����Ϣ�ṹ */
typedef struct FS_DcdSyncPkg_Tag{
	FS_CHAR *		global_anchor;		/* ȫ��ͬ����λ�� */
	FS_UINT4 		ttl;				/* seconds */
	FS_UINT4 		server_time;		/* seconds */
	
	FS_List 			channel_list;	/* Ƶ���б� */
	FS_List 			feed_list;		/* ���µ�Ƶ������ */
}FS_DcdSyncPkg;

typedef struct FS_DcdRspEntry_Tag{
	FS_List				list;
	
	FS_CHAR *			content_type;
	FS_CHAR *			content_id;
	FS_SINT4 			len;
	FS_CHAR 			file[FS_FILE_NAME_LEN];
}FS_DcdRspEntry;

/*
 * ����DCDͬ����
 * xml_doc [in] ���룬ͬ����������
 * len [in] ���룬ͬ���������ݳ��ȣ����len <= 0����xml_doc��������0��β���ַ���
 *
 * return: 
 * ����������dcdͬ�����ṹ�塣�����ڷ����ڴ档
 * ������ʹ�ú���Ҫ����dcd_sync_pkg_free���ͷ��ڴ档
 * ����ʧ�ܷ���NULL
*/ 
FS_DcdSyncPkg *FS_DcdParseSyncPkg( FS_CHAR *xml_file );

/* �ͷ�DCDͬ�����ڴ� */
void FS_DcdFreeSyncPkg(FS_DcdSyncPkg *pkg, FS_BOOL del_file);

void FS_DcdFreeEntry( FS_DcdEntry *entry, FS_BOOL del_file );

/* return FS_DcdRspEntry list if success */
FS_List *FS_DcdParseEntryList(FS_CHAR *boundary, FS_BYTE *data, FS_SINT4 len);

void FS_DcdFreeRspEntryList(FS_List* entrys);

#endif

