#ifndef _FS_SNS_LIB_H_
#define _FS_SNS_LIB_H_

#include "inc/FS_Config.h"
#include "inc/util/FS_File.h"

#define FS_SNS_DEF_MSGS_COUNT		6
#define FS_SNS_MAX_MSGS_COUNT		20

typedef void * FS_SnsLib;

typedef struct FS_SnsAccount_Tag
{
	FS_SINT4		icon_id;
	FS_SINT4		title_tid;
	FS_SINT4		update_tid;
	FS_SINT4		reply_tid;
	FS_CHAR			*name;
	FS_BOOL			bound;
	FS_SINT4		update_cnt;
	FS_SINT4		reply_cnt;
}FS_SnsAccount;

typedef struct FS_SnsConfig_Tag
{
	/* account */
	FS_CHAR			user_name[FS_URL_LEN];
	FS_CHAR			password[FS_URL_LEN];
	FS_BOOL			save_passwd;
	FS_BOOL			auto_login;
	/* sns display setting */
	FS_BOOL			display_image;
	FS_BOOL			image_cache;
	FS_SINT4		msg_cnt_per_page;
	FS_SINT4		time_zone;
	/* network param */
	FS_CHAR 		apn[FS_URL_LEN];
	FS_CHAR			user[FS_URL_LEN];
	FS_CHAR			pass[FS_URL_LEN];
	FS_BOOL			use_proxy;
	FS_CHAR			proxy_addr[FS_URL_LEN];
	FS_SINT4		proxy_port;
	/* sns accout */
	FS_SnsAccount	*accout;
	FS_SINT4		account_num;
}FS_SnsConfig;

typedef enum FS_SnsReqCode_Tag
{
	FS_SNS_REQ_NONE = 0,
	FS_SNS_REQ_LOGIN,
	FS_SNS_REQ_REGISTER,
	FS_SNS_REQ_GET_SUMMARY,
	FS_SNS_REQ_GET_TWEET,
	FS_SNS_REQ_GET_UPDATE,	/* 5 */
	FS_SNS_REQ_GET_REPLY,
	FS_SNS_REQ_SET_TWEET,
	FS_SNS_REQ_RETWEET,
	FS_SNS_REQ_SET_LIKE,
	FS_SNS_REQ_REPLY,		/* 10 */
	FS_SNS_REQ_UPLOAD_PHOTO,
	FS_SNS_REQ_GET_FRIENDS,
	FS_SNS_REQ_FIND_FRIENDS,
	FS_SNS_REQ_ADD_FRIEND,
	FS_SNS_REQ_DEL_FRIEND,	/* 15 */
	FS_SNS_REQ_RSS_GET_ALL_ARTICLE,
	FS_SNS_REQ_RSS_GET_ALL_ARTICLE_CONTENT,
	FS_SNS_REQ_RSS_GET_ALL_CHANNEL,
	FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL,
	FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL_CONTENT,	/* 20 */
	FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY,
	FS_SNS_REQ_RSS_GET_CATEGORY_DETAIL,	
	FS_SNS_REQ_RSS_SET_MY_CHANNEL,
	FS_SNS_REQ_EML_SET_ACOUNT,
	FS_SNS_REQ_EML_GET_ACOUNT,	/* 25 */
	FS_SNS_REQ_EML_SEND,
	FS_SNS_REQ_EML_RECV,
	FS_SNS_REQ_EML_RECV_CONTENT,
	FS_SNS_REQ_EML_GET_CONTACT,
	FS_SNS_REQ_EML_ADD_CONTACT,	/* 30 */
	FS_SNS_REQ_EML_DEL_CONTACT,	
	FS_SNS_REQ_EML_MOD_CONTACT,
	FS_SNS_REQ_GET_UPDATE_CONTENT,

	FS_SNS_MAX
}FS_SnsReqCode;

typedef struct FS_SnsRequest_Tag
{
	FS_SnsReqCode req;
	union
	{
		struct 
		{
			FS_CHAR *user;
			FS_CHAR *pass;
		}login;

		struct 
		{
			FS_CHAR *user;
			FS_CHAR *pass;
			FS_CHAR *email;
		}reg;

		struct  
		{
			FS_CHAR *type;
		}get_tweet;

		struct 
		{
			FS_CHAR *id;
			FS_CHAR *type;
			FS_CHAR *name;
			FS_SINT4 page;
			FS_SINT4 count;
		}get_msgs;	/* FS_SNS_REQ_GET_UPDATE && FS_SNS_REQ_GET_REPLY */

		struct  
		{
			FS_CHAR *type;
			FS_CHAR *sync_to;
			FS_CHAR *msg;
		}set_tweet;

		struct  
		{
			FS_CHAR *type;
			FS_CHAR *msg;
			FS_CHAR *id;
		}reply;

		struct 
		{
			FS_CHAR *type;
			FS_CHAR *sync_to;
			FS_CHAR *msg;
			FS_CHAR *fname;
			FS_SINT4 fsize;
		}photo;

		struct
		{
			FS_CHAR *type;
			FS_CHAR *key;
			FS_SINT4 page;
			FS_SINT4 count;
		}friends;

		struct  
		{
			FS_SINT4 page;
			FS_SINT4 count;
			FS_CHAR *id;
		}rss;

		struct 
		{
			FS_CHAR *email;
			FS_CHAR *smtp;
			FS_CHAR *pop3;
			FS_CHAR *password;
			FS_BOOL bind_home;
		}eml_account;
		
		struct  
		{
			FS_CHAR *receiver;
			FS_CHAR *subject;
			FS_CHAR *content;
		}eml_send;

		struct  
		{
			FS_CHAR *id;
			FS_SINT4 page;
			FS_SINT4 count;
		}eml_recv;

		struct
		{
			FS_SINT4 page;
			FS_SINT4 count;
			FS_CHAR *id;
			FS_CHAR *name;
			FS_CHAR *email;
		}eml_contact;

	}data;
}FS_SnsRequest;

typedef enum FS_SnsRspCode_Tag
{
	FS_SNS_RSP_ERR_UNKNOW = 0,
	FS_SNS_RSP_ERR_NET_CONN,
	FS_SNS_RSP_SUCCESS,
	FS_SNS_RSP_MAX
}FS_SnsRspCode;

typedef struct FS_SnsMsg_Tag
{
	FS_CHAR *id;
	FS_CHAR *username;
	FS_CHAR *author;
	FS_CHAR *date;
	FS_CHAR *icon_url;
	FS_CHAR *icon_url_proxy;	/* used proxy to download icon */
	FS_CHAR *icon_file;
	FS_CHAR *msg;
	FS_CHAR *type;
}FS_SnsMsg;

typedef struct FS_SnsFriends_Tag
{
	FS_CHAR *id;
	FS_CHAR *username;
	FS_CHAR *author;
	FS_CHAR *icon_url;
	FS_CHAR *icon_url_proxy;
	FS_CHAR *icon_file;
	FS_CHAR *location;
	FS_CHAR *sex;
	FS_CHAR *relation;
	FS_BOOL is_friend;
}FS_SnsFriends;

typedef struct FS_SnsRssArticle_Tag
{
	FS_CHAR *id;
	FS_CHAR *channel_name;
	FS_CHAR *title;
	FS_CHAR *date;
	FS_CHAR *msg;
}FS_SnsRssArticle;

typedef struct FS_SnsRssChannel_Tag
{
	FS_CHAR *id;
	FS_CHAR *icon_url;
	FS_CHAR *icon_file;
	FS_CHAR *channel_name;
	FS_SINT4 count;
}FS_SnsRssChannel;

typedef struct FS_SnsRssCategory_Tag
{
	FS_CHAR *id;
	FS_CHAR *icon_url;
	FS_CHAR *icon_file;
	FS_CHAR *name;
	FS_SINT4 count;
}FS_SnsRssCategory;

typedef struct FS_SnsRssCategoryItem_Tag
{
	FS_CHAR *id;
	FS_CHAR *icon_url;
	FS_CHAR *icon_file;
	FS_CHAR *name;
	FS_CHAR *language;
	FS_BOOL selected;
}FS_SnsRssCategoryItem;

typedef struct FS_SnsEmail_Tag
{
	FS_CHAR *id;
	FS_CHAR *sender;
	FS_CHAR *date;
	FS_CHAR *subject;
	FS_CHAR *content;
}FS_SnsEmail;

typedef struct FS_SnsEmlContact_Tag
{
	FS_CHAR *id;
	FS_CHAR *name;
	FS_CHAR *email;
}FS_SnsEmlContact;

typedef struct FS_SnsResponse_Tag
{
	FS_SnsRspCode result;
	FS_CHAR *err_info;
	union
	{
		struct  
		{
			FS_SnsMsg msg;
		}get_tweet;

		struct  
		{
			FS_SnsMsg msgs[FS_SNS_MAX_MSGS_COUNT];
			FS_SINT4 count;
		}msglist;	/* FS_SNS_REQ_GET_UPDATE && FS_SNS_REQ_GET_REPLY */

		struct
		{
			FS_SnsFriends friends[FS_SNS_MAX_MSGS_COUNT];
			FS_SINT4 count;
		}friends;

		struct  
		{
			FS_SnsRssArticle articles[FS_SNS_MAX_MSGS_COUNT];
			FS_SINT4 count;
		}rss_article;

		struct 
		{
			FS_SnsRssChannel channels[FS_SNS_MAX_MSGS_COUNT];
			FS_SINT4 count;
		}rss_channel;

		struct 
		{
			FS_SnsRssCategory items[FS_SNS_MAX_MSGS_COUNT];
			FS_SINT4 count;
		}rss_category;

		struct 
		{
			FS_SnsRssCategoryItem items[FS_SNS_MAX_MSGS_COUNT];
			FS_SINT4 count;
		}rss_category_detail;

		struct
		{
			FS_SnsEmail emails[FS_SNS_MAX_MSGS_COUNT];
			FS_SINT4 count;
		}eml_list;

		struct 
		{
			FS_CHAR *email;
			FS_CHAR *smtp;
			FS_CHAR *pop3;
			FS_CHAR *password;
			FS_BOOL bind_home;
		}eml_account;

		struct
		{
			FS_SnsEmlContact contacts[FS_SNS_MAX_MSGS_COUNT];
			FS_SINT4 count;
		}eml_contact;
	}data;
}FS_SnsResponse;

/* request status report */
typedef enum FS_SnsReqStatusCode_Tag
{
	FS_SNS_REQ_STS_NONE = 0,
		
	FS_SNS_REQ_STS_SEND_REQUEST,
	FS_SNS_REQ_STS_WAIT_RESPONSE,
	FS_SNS_REQ_STS_DOWNLOAD_IMAGE,
	FS_SNS_REQ_STS_UPLOAD_PHOTO,

	FS_SNS_REQ_STS_MAX
}FS_SnsReqStatusCode;

typedef struct FS_SnsReqStatus_Tag
{
	FS_SnsReqStatusCode	status;
	FS_SnsReqStatusCode image_status;
	FS_SINT4			index;
	FS_SINT4			total;
}FS_SnsReqStatus;

typedef void ( *FS_SnsLibCallback ) ( FS_SnsLib pSns, FS_SnsReqCode req, FS_SnsResponse *rsp );

FS_SnsLib *FS_SnsLibCreate( FS_SnsLibCallback cb );

FS_BOOL FS_SnsLibReadCache( FS_SnsLib pSns, FS_SnsRequest *pReq, FS_SnsResponse *pRsp );

void FS_SnsLibClearCache( FS_SnsLib pSns, FS_SnsRequest *pReq );

FS_BOOL FS_SnsLibRequest( FS_SnsLib pSns, FS_SnsRequest *pReq );

void FS_SnsLibCancelRequest( FS_SnsLib pSns );

FS_SnsReqStatus * FS_SnsLibGetReqStatus( FS_SnsLib pSns );

void FS_SnsLibDestroy( FS_SnsLib pSns );

void FS_SnsInitConfig( void );

FS_SnsConfig *FS_SnsGetConfig( void );

void FS_SnsSetConfig( FS_SnsConfig *config );

/* utility functions. Only return by FS_SnsLibReadCache need to free */
void FS_SnsFreeResponse( FS_SnsResponse *rsp, FS_SnsReqCode reqCode );

#endif //_FS_SNS_LIB_H_
