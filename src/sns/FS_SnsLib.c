#include "inc/FS_Config.h"

#ifdef FS_MODULE_SNS
#include "inc/inte/FS_Inte.h" 
#include "inc/util/FS_File.h"
#include "inc/util/FS_Util.h"
#include "inc/web/FS_Http.h"
#include "inc/util/FS_NetConn.h"
#include "inc/util/FS_Sax.h"
#include "inc/res/FS_Res.h"
#include "inc/sns/FS_SnsLib.h"
#include "inc/util/FS_MemDebug.h"

/*
 * Define this feature: do not download image for rss category detail list
 */
#define FEATURE_DISABLE_RSS_CATEGORY_DETAIL_IMAGE

#define _SNSLIB_DEBUG

#ifdef _SNSLIB_DEBUG
#define FS_SNS_TRACE0(a)				FS_TRACE0( "[SNS]" a "\r\n")
#define FS_SNS_TRACE1(a,b)				FS_TRACE1( "[SNS]" a "\r\n", b)
#define FS_SNS_TRACE2(a,b,c)			FS_TRACE2( "[SNS]" a "\r\n", b, c)
#define FS_SNS_TRACE3(a,b,c,d)			FS_TRACE3( "[SNS]" a "\r\n", b, c, d)
#else
#define FS_SNS_TRACE0(a)
#define FS_SNS_TRACE1(a,b)
#define FS_SNS_TRACE2(a,b,c)
#define FS_SNS_TRACE3(a,b,c,d)
#endif

#define FS_SNS_BOUNDARY			"SFOX_ISYNC_NOW_87634720_74583340"
#define FS_SNS_FORM_DATA_FILE	"FormData.dat"
#define FS_SNS_CFG_FILE			"SnsConfig.bin"
#define FS_SNS_CACHE_FILE		"SnsCache.bin"
#define FS_SNS_SERVER_URL		"http://apiasiad.avalaa.com/weibonewtest/"
#define FS_SNS_SERVER_API_URL	FS_SNS_SERVER_URL"api.php"
#define FS_SNS_LOGIN_URL		FS_SNS_SERVER_URL"login.php?username=%s&password=%s&device=embed&version=1.0.2"
#define FS_SNS_REGISTER_URL		FS_SNS_SERVER_URL"reg.php?username=%s&password=%s&email=%s"
#define FS_SNS_GET_SUMMARY_URL	FS_SNS_SERVER_API_URL"?type=isync&opera=getsummary"
#define FS_SNS_GET_TWEET_URL	FS_SNS_SERVER_API_URL"?type=%s&opera=gettweet&rtype=xml"
#define FS_SNS_GET_UPDATE_URL	FS_SNS_SERVER_API_URL"?type=%s&opera=%s&page=%d&count=%d&rtype=xml"
#define FS_SNS_GET_UPDATE_CONTENT_URL	FS_SNS_SERVER_API_URL"?type=isync&opera=get_all_updates_content&id=%s&types=%s&rtype=xml"
#define FS_SNS_GET_REPLY_URL	FS_SNS_SERVER_API_URL"?type=%s&opera=%s&page=%d&count=%d&rtype=xml"
#define FS_SNS_SET_TWEET_URL	FS_SNS_SERVER_API_URL"?type=%s&opera=settweet&msg="
#define FS_SNS_RETWEET_URL		FS_SNS_SERVER_API_URL"?type=%s&opera=setretweet&reply_id=%s"
#define FS_SNS_SETLIKE_URL		FS_SNS_SERVER_API_URL"?type=%s&opera=setlike&reply_id=%s&v=y"
#define FS_SNS_REPLY_URL		FS_SNS_SERVER_API_URL"?type=%s&opera=setreply&reply_id=%s&msg="
#define FS_SNS_UPLOAD_PHOTO_URL	FS_SNS_SERVER_API_URL"?type=%s&opera=setimage&msg="
#define FS_SNS_GET_FRIENDS_URL	FS_SNS_SERVER_API_URL"?type=%s&opera=getfriend&page=%d&count=%d&rtype=xml"
#define FS_SNS_FIND_FRIENDS_URL	FS_SNS_SERVER_API_URL"?type=%s&opera=findfriend&key=%s&page=%d&count=%d&rtype=xml"
#define FS_SNS_ADD_FRIEND_URL	FS_SNS_SERVER_API_URL"?type=%s&opera=addfriend&key=%s"
#define FS_SNS_DEL_FRIEND_URL	FS_SNS_SERVER_API_URL"?type=%s&opera=delfriend&key=%s"
#define FS_SNS_RSS_ALL_ARTICLE_URL				FS_SNS_SERVER_API_URL"?type=rss&opera=get_all_article&page=%d&count=%d"
#define FS_SNS_RSS_ALL_ARTICLE_CONTENT_URL		FS_SNS_SERVER_API_URL"?type=rss&opera=get_all_article_content&id=%s"
#define FS_SNS_RSS_ALL_CHANNEL_URL				FS_SNS_SERVER_API_URL"?type=rss&opera=get_all_channel"
#define FS_SNS_RSS_CHANNEL_DETAIL_URL			FS_SNS_SERVER_API_URL"?type=rss&opera=get_channel_detail&id=%s&page=%d&count=%d"
#define FS_SNS_RSS_CHANNEL_DETAIL_CONTENT_URL	FS_SNS_SERVER_API_URL"?type=rss&opera=get_channel_detail_content&id=%s"
#define FS_SNS_RSS_FIRST_CATEGORY_URL	FS_SNS_SERVER_API_URL"?type=rss&opera=get_first_category"
#define FS_SNS_RSS_SECOND_CATEGORY_URL	FS_SNS_SERVER_API_URL"?type=rss&opera=get_second_category&id=%s"
#define FS_SNS_RSS_CATEGORY_DETAIL_URL	FS_SNS_SERVER_API_URL"?type=rss&opera=get_category_detail&id=%s"
#define FS_SNS_RSS_SET_MY_CHANNEL_URL	FS_SNS_SERVER_API_URL"?type=rss&opera=set_my_channel&data=%s"
#define FS_SNS_EML_SET_ACCOUNT_URL		FS_SNS_SERVER_API_URL"?type=email&opera=setmail"
#define FS_SNS_EML_GET_ACCOUNT_URL		FS_SNS_SERVER_API_URL"?type=email&opera=getaccount"
#define FS_SNS_EML_SEND_URL				FS_SNS_SERVER_API_URL"?type=email&opera=sendmail"
#define FS_SNS_EML_RECV_URL				FS_SNS_SERVER_API_URL"?type=email&opera=getmail&page=%d&count=%d&rtype=xml"
#define FS_SNS_EML_RECV_CONTENT_URL		FS_SNS_SERVER_API_URL"?type=email&opera=getmailcontent&id=%s"
#define FS_SNS_EML_GET_CONTACT_URL		FS_SNS_SERVER_API_URL"?type=email&opera=getcontact&page=%d&count=%d"
#define FS_SNS_EML_ADD_CONTACT_URL		FS_SNS_SERVER_API_URL"?type=email&opera=addcontact"
#define FS_SNS_EML_DEL_CONTACT_URL		FS_SNS_SERVER_API_URL"?type=email&opera=delcontact"
#define FS_SNS_EML_MOD_CONTACT_URL		FS_SNS_SERVER_API_URL"?type=email&opera=modcontact"

#define FS_SNS_IMAGE_PROXY_URL	FS_SNS_SERVER_API_URL"?type=extension&opera=getpicture&url="

#define FS_SNS_DOWNLOAD_IMAGE_TIME		2000
#define FS_SNS_MAX_REQ_LEN				(1024 * 8)
#define FS_SNS_MAX_CACHE_ITEM			(2 * FS_SNS_MAX_MSGS_COUNT)

typedef enum FS_SnsLibState_Tag
{
	FS_SNS_ST_IDLE,
	FS_SNS_ST_LOGIN,
	FS_SNS_ST
}FS_SnsLibState;

typedef struct FS_SnsLib_Tag
{
	FS_HttpHandle			http;
	FS_SnsLibCallback		callback;
	FS_SnsRequest			req;
	FS_SnsResponse			rsp;
	FS_SnsReqStatus			status;
	FS_SnsConfig			*config;

	FS_SINT4				content_len;
	FS_SINT4				offset;

	FS_CHAR					*sessid;
	FS_CHAR					file[FS_FILE_NAME_LEN];

	FS_UINT4				timer_id;
	FS_SINT4				account_idx;	/* internal used for isync summary parse */
}FS_SnsSession;

typedef struct FS_SnsCacheImage_Tag
{
	FS_CHAR					url[FS_URL_LEN];
	FS_CHAR					file[FS_FILE_NAME_LEN];
}FS_SnsCacheImage;

typedef struct FS_SnsCache_Tag
{
	FS_SINT4				pos;
	FS_SnsCacheImage		cache[FS_SNS_MAX_CACHE_ITEM];
}FS_SnsCache;

static FS_SnsCache GFS_SnsCache;
static FS_SnsConfig GFS_SnsConfig;
static FS_SnsAccount GFS_SnsAccounts[] = 
{
	{FS_I_ISYNC,	FS_T_ISYNC,			FS_T_UPDATE,	FS_T_SNS_REPLY,		"isync",		FS_TRUE },	/* isync default bound */
	{FS_I_SINA,		FS_T_SINA,			FS_T_UPDATE,	FS_T_SNS_REPLY,		"sina",			FS_FALSE },
	{FS_I_FACEBOOK,	FS_T_FACEBOOK,		FS_T_NEWS_FEED,	FS_T_MESSAGES,		"facebook",		FS_FALSE },
	{FS_I_TWITTER,	FS_T_TWITTER,		FS_T_TIMELINE,	FS_T_MENTIONS,		"twitter",		FS_FALSE },
	{0,				0,					0,				0,					"email",		FS_FALSE },
};

static void FS_SnsRequestResult( FS_SnsSession *sns );
static void FS_SnsSendHttpRequest( FS_SnsSession *sns, FS_CHAR *url, FS_CHAR *headers );
static void FS_SnsRequestStatusReport( FS_SnsSession *sns );

/************************************************************************/
/* SNS Image Cache                                                      */
/************************************************************************/

static FS_CHAR * FS_SnsCacheGetImage( FS_CHAR *url ) 
{
	FS_SINT4 i;

	if ( url == FS_NULL ) {
		return FS_NULL;
	}
	for ( i = 0; i < FS_SNS_MAX_CACHE_ITEM; i ++ ) {
		if ( GFS_SnsCache.cache[i].url && IFS_Strcmp(url, GFS_SnsCache.cache[i].url) == 0 ) {
			return GFS_SnsCache.cache[i].file;
		}
	}
	return FS_NULL;
}

static void FS_SnsCachePutImage( FS_CHAR *url, FS_CHAR *file )
{
	if ( url == FS_NULL || file == FS_NULL ) {
		return;
	}
	if ( GFS_SnsCache.cache[GFS_SnsCache.pos].file[0] ) {
		FS_FileDelete( FS_DIR_TMP, GFS_SnsCache.cache[GFS_SnsCache.pos].file );
	}
	IFS_Strncpy( GFS_SnsCache.cache[GFS_SnsCache.pos].url, url, FS_URL_LEN - 1 );
	IFS_Strncpy( GFS_SnsCache.cache[GFS_SnsCache.pos].file, file, FS_FILE_NAME_LEN - 1 );
	GFS_SnsCache.pos ++;
	if ( GFS_SnsCache.pos >= FS_SNS_MAX_CACHE_ITEM ) {
		GFS_SnsCache.pos = 0;	/* ring buffer */
	}

	FS_FileWrite( FS_DIR_TMP, FS_SNS_CACHE_FILE, 0, &GFS_SnsCache, sizeof(GFS_SnsCache) );
}

/* will return the icon url which did not hit cache, return NULL means that all image hit cache */
static FS_CHAR * FS_SnsMsgsCheckCacheImage( FS_SnsSession *sns, FS_SINT4 *pIndex )
{
	FS_CHAR *file, *proxy_url;
	FS_SINT4 i, len;
	FS_SINT4 non_cache_idx = -1;
	FS_SnsMsg *msgs = sns->rsp.data.msglist.msgs;
	FS_SINT4 count = sns->rsp.data.msglist.count;

	for ( i = 0; i < count; i ++ ) {
		file = FS_SnsCacheGetImage( msgs[i].icon_url );
		if ( file ) {
			FS_COPY_TEXT( msgs[i].icon_file, file );
		} else {
			if ( non_cache_idx == -1 ) {
				non_cache_idx = i;
			}
		}
	}

	if ( non_cache_idx != -1 ) {
		/*
		 * cannot download picture from facebook/twitter for same reason everyone knows.
		 * Here, we use proxy to try it.
		 */
		if ( FS_STR_I_EQUAL(msgs[non_cache_idx].type, "facebook")
			|| FS_STR_I_EQUAL(msgs[non_cache_idx].type, "twitter") 
			|| FS_STR_I_EQUAL(sns->req.data.get_msgs.type, "facebook")
			|| FS_STR_I_EQUAL(sns->req.data.get_msgs.type, "twitter") ) {
			if ( msgs[non_cache_idx].icon_url 
				&& msgs[non_cache_idx].icon_url_proxy == FS_NULL ) {
				len = IFS_Strlen(FS_SNS_IMAGE_PROXY_URL) + 3 * IFS_Strlen(msgs[non_cache_idx].icon_url) + 1;
				proxy_url = IFS_Malloc( len );
				if ( proxy_url ) {
					IFS_Memset( proxy_url, 0, len );
					IFS_Strcpy( proxy_url, FS_SNS_IMAGE_PROXY_URL );
					FS_UrlEncode( proxy_url + IFS_Strlen(proxy_url), len - IFS_Strlen(proxy_url),
						msgs[non_cache_idx].icon_url, -1 );
					msgs[non_cache_idx].icon_url_proxy = proxy_url;
				}
			}
		}

		if ( pIndex ) {
			*pIndex = non_cache_idx;
		}
		if ( msgs[non_cache_idx].icon_url_proxy ) {
			return msgs[non_cache_idx].icon_url_proxy;
		} else {
			return msgs[non_cache_idx].icon_url;
		}
	} else {
		if ( pIndex ) {
			*pIndex = 0;
		}
		return FS_NULL;
	}
}

/* will return the icon url which did not hit cache, return NULL means that all image hit cache */
static FS_CHAR * FS_SnsMsgsSetImageFile( FS_SnsSession *sns, FS_CHAR *icon_file )
{
	FS_CHAR *file;
	FS_SINT4 i;
	FS_SnsMsg *msgs = sns->rsp.data.msglist.msgs;
	FS_SINT4 count = sns->rsp.data.msglist.count;
	
	for ( i = 0; i < count; i ++ ) {
		file = FS_SnsCacheGetImage( msgs[i].icon_url );
		if ( file == FS_NULL ) {
			FS_COPY_TEXT( msgs[i].icon_file, icon_file );
			FS_SnsCachePutImage( msgs[i].icon_url, msgs[i].icon_file );
			return FS_SnsMsgsCheckCacheImage( sns, FS_NULL );
		}
	}
	return FS_NULL;
}

/* will return the icon url which did not hit cache, return NULL means that all image hit cache */
static FS_CHAR * FS_SnsFriendsCheckCacheImage( FS_SnsSession *sns, FS_SINT4 *pIndex )
{
	FS_CHAR *file;
	FS_SINT4 i;
	FS_SINT4 non_cache_idx = -1;
	FS_SnsFriends *friends = sns->rsp.data.friends.friends;
	FS_SINT4 count = sns->rsp.data.friends.count;

	for ( i = 0; i < count; i ++ ) {
		file = FS_SnsCacheGetImage( friends[i].icon_url );
		if ( file ) {
			FS_COPY_TEXT( friends[i].icon_file, file );
		} else {
			if ( non_cache_idx == -1 ) {
				non_cache_idx = i;
			}
		}
	}

	if ( non_cache_idx != -1 ) {
		if ( pIndex ) {
			*pIndex = non_cache_idx;
		}
		return friends[non_cache_idx].icon_url;
	} else {
		if ( pIndex ) {
			*pIndex = 0;
		}
		return FS_NULL;
	}
}

/* will return the icon url which did not hit cache, return NULL means that all image hit cache */
static FS_CHAR * FS_SnsFriendsSetImageFile( FS_SnsSession *sns, FS_CHAR *icon_file )
{
	FS_CHAR *file;
	FS_SINT4 i;
	FS_SnsFriends *friends = sns->rsp.data.friends.friends;
	FS_SINT4 count = sns->rsp.data.friends.count;
	
	for ( i = 0; i < count; i ++ ) {
		file = FS_SnsCacheGetImage( friends[i].icon_url );
		if ( file == FS_NULL ) {
			FS_COPY_TEXT( friends[i].icon_file, icon_file );
			FS_SnsCachePutImage( friends[i].icon_url, friends[i].icon_file );
			return FS_SnsFriendsCheckCacheImage( sns, FS_NULL );
		}
	}
	return FS_NULL;
}

/* will return the icon url which did not hit cache, return NULL means that all image hit cache */
static FS_CHAR * FS_SnsRssChannelCheckCacheImage( FS_SnsSession *sns, FS_SINT4 *pIndex )
{
	FS_CHAR *file;
	FS_SINT4 i;
	FS_SINT4 non_cache_idx = -1;
	FS_SnsRssChannel *channels = sns->rsp.data.rss_channel.channels;
	FS_SINT4 count = sns->rsp.data.rss_channel.count;
	
	for ( i = 0; i < count; i ++ ) {
		file = FS_SnsCacheGetImage( channels[i].icon_url );
		if ( file ) {
			FS_COPY_TEXT( channels[i].icon_file, file );
		} else {
			if ( non_cache_idx == -1 ) {
				non_cache_idx = i;
			}
		}
	}
	
	if ( non_cache_idx != -1 ) {
		if ( pIndex ) {
			*pIndex = non_cache_idx;
		}
		return channels[non_cache_idx].icon_url;
	} else {
		if ( pIndex ) {
			*pIndex = 0;
		}
		return FS_NULL;
	}
}

/* will return the icon url which did not hit cache, return NULL means that all image hit cache */
static FS_CHAR * FS_SnsRssChannelSetImageFile( FS_SnsSession *sns, FS_CHAR *icon_file )
{
	FS_CHAR *file;
	FS_SINT4 i;
	FS_SnsRssChannel *channels = sns->rsp.data.rss_channel.channels;
	FS_SINT4 count = sns->rsp.data.rss_channel.count;
	
	for ( i = 0; i < count; i ++ ) {
		file = FS_SnsCacheGetImage( channels[i].icon_url );
		if ( file == FS_NULL ) {
			FS_COPY_TEXT( channels[i].icon_file, icon_file );
			FS_SnsCachePutImage( channels[i].icon_url, channels[i].icon_file );
			return FS_SnsRssChannelCheckCacheImage( sns, FS_NULL );
		}
	}
	return FS_NULL;
}

/* will return the icon url which did not hit cache, return NULL means that all image hit cache */
static FS_CHAR * FS_SnsRssCategoryCheckCacheImage( FS_SnsSession *sns, FS_SINT4 *pIndex )
{
	FS_CHAR *file;
	FS_SINT4 i;
	FS_SINT4 non_cache_idx = -1;
	FS_SnsRssCategory *channels = sns->rsp.data.rss_category.items;
	FS_SINT4 count = sns->rsp.data.rss_category.count;
	
	for ( i = 0; i < count; i ++ ) {
		file = FS_SnsCacheGetImage( channels[i].icon_url );
		if ( file ) {
			FS_COPY_TEXT( channels[i].icon_file, file );
		} else {
			if ( non_cache_idx == -1 ) {
				non_cache_idx = i;
			}
		}
	}
	
	if ( non_cache_idx != -1 ) {
		if ( pIndex ) {
			*pIndex = non_cache_idx;
		}
		return channels[non_cache_idx].icon_url;
	} else {
		if ( pIndex ) {
			*pIndex = 0;
		}
		return FS_NULL;
	}
}

/* will return the icon url which did not hit cache, return NULL means that all image hit cache */
static FS_CHAR * FS_SnsRssCategorySetImageFile( FS_SnsSession *sns, FS_CHAR *icon_file )
{
	FS_CHAR *file;
	FS_SINT4 i;
	FS_SnsRssCategory *channels = sns->rsp.data.rss_category.items;
	FS_SINT4 count = sns->rsp.data.rss_category.count;
	
	for ( i = 0; i < count; i ++ ) {
		file = FS_SnsCacheGetImage( channels[i].icon_url );
		if ( file == FS_NULL ) {
			FS_COPY_TEXT( channels[i].icon_file, icon_file );
			FS_SnsCachePutImage( channels[i].icon_url, channels[i].icon_file );
			return FS_SnsRssCategoryCheckCacheImage( sns, FS_NULL );
		}
	}
	return FS_NULL;
}

/* will return the icon url which did not hit cache, return NULL means that all image hit cache */
static FS_CHAR * FS_SnsRssCategoryDetailCheckCacheImage( FS_SnsSession *sns, FS_SINT4 *pIndex )
{
	FS_CHAR *file;
	FS_SINT4 i;
	FS_SINT4 non_cache_idx = -1;
	FS_SnsRssCategoryItem *channels = sns->rsp.data.rss_category_detail.items;
	FS_SINT4 count = sns->rsp.data.rss_category_detail.count;
	
	for ( i = 0; i < count; i ++ ) {
		file = FS_SnsCacheGetImage( channels[i].icon_url );
		if ( file ) {
			FS_COPY_TEXT( channels[i].icon_file, file );
		} else {
			if ( non_cache_idx == -1 ) {
				non_cache_idx = i;
			}
		}
	}
	
	if ( non_cache_idx != -1 ) {
		if ( pIndex ) {
			*pIndex = non_cache_idx;
		}
		return channels[non_cache_idx].icon_url;
	} else {
		if ( pIndex ) {
			*pIndex = 0;
		}
		return FS_NULL;
	}
}

/* will return the icon url which did not hit cache, return NULL means that all image hit cache */
static FS_CHAR * FS_SnsRssCategoryDetailSetImageFile( FS_SnsSession *sns, FS_CHAR *icon_file )
{
	FS_CHAR *file;
	FS_SINT4 i;
	FS_SnsRssCategoryItem *channels = sns->rsp.data.rss_category_detail.items;
	FS_SINT4 count = sns->rsp.data.rss_category_detail.count;
	
	for ( i = 0; i < count; i ++ ) {
		file = FS_SnsCacheGetImage( channels[i].icon_url );
		if ( file == FS_NULL ) {
			FS_COPY_TEXT( channels[i].icon_file, icon_file );
			FS_SnsCachePutImage( channels[i].icon_url, channels[i].icon_file );
			return FS_SnsRssCategoryDetailCheckCacheImage( sns, FS_NULL );
		}
	}
	return FS_NULL;
}

static void FS_SnsFreeMsg( FS_SnsMsg * msg )
{
	FS_SAFE_FREE( msg->username );
	FS_SAFE_FREE( msg->author );
	FS_SAFE_FREE( msg->date );
	FS_SAFE_FREE( msg->icon_file );
	FS_SAFE_FREE( msg->icon_url );
	FS_SAFE_FREE( msg->icon_url_proxy );
	FS_SAFE_FREE( msg->id );
	FS_SAFE_FREE( msg->msg );
	FS_SAFE_FREE( msg->type );
}

static void FS_SnsFreeFriends( FS_SnsFriends * friends )
{
	FS_SAFE_FREE( friends->id );
	FS_SAFE_FREE( friends->username );
	FS_SAFE_FREE( friends->author );
	FS_SAFE_FREE( friends->icon_url );
	FS_SAFE_FREE( friends->icon_file );
	FS_SAFE_FREE( friends->icon_url_proxy );
	FS_SAFE_FREE( friends->location );
	FS_SAFE_FREE( friends->sex );
	FS_SAFE_FREE( friends->relation );
}

static void FS_SnsFreeRssArticle( FS_SnsRssArticle * article )
{
	FS_SAFE_FREE( article->id );
	FS_SAFE_FREE( article->channel_name );
	FS_SAFE_FREE( article->title );
	FS_SAFE_FREE( article->date );
	FS_SAFE_FREE( article->msg );
}

static void FS_SnsFreeRssChannel( FS_SnsRssChannel * channel )
{
	FS_SAFE_FREE( channel->id );
	FS_SAFE_FREE( channel->icon_url );
	FS_SAFE_FREE( channel->icon_file );
	FS_SAFE_FREE( channel->channel_name );
}

static void FS_SnsFreeEmail( FS_SnsEmail * email )
{
	FS_SAFE_FREE( email->id );
	FS_SAFE_FREE( email->sender );
	FS_SAFE_FREE( email->date );
	FS_SAFE_FREE( email->subject );
	FS_SAFE_FREE( email->content );
}

static void FS_SnsFreeRssCategory( FS_SnsRssCategory * item )
{
	FS_SAFE_FREE( item->id );
	FS_SAFE_FREE( item->icon_url );
	FS_SAFE_FREE( item->icon_file );
	FS_SAFE_FREE( item->name );
}

static void FS_SnsFreeRssCategoryDetail( FS_SnsRssCategoryItem * item )
{
	FS_SAFE_FREE( item->id );
	FS_SAFE_FREE( item->icon_url );
	FS_SAFE_FREE( item->icon_file );
	FS_SAFE_FREE( item->name );
	FS_SAFE_FREE( item->language );
}

static void FS_SnsFreeEmlContact( FS_SnsEmlContact * item )
{
	FS_SAFE_FREE( item->id );
	FS_SAFE_FREE( item->name );
	FS_SAFE_FREE( item->email );
}

void FS_SnsFreeResponse( FS_SnsResponse *rsp, FS_SnsReqCode reqCode )
{
	FS_SINT4 i;

	switch ( reqCode ) {
	case FS_SNS_REQ_LOGIN:
	case FS_SNS_REQ_REGISTER:
	case FS_SNS_REQ_SET_TWEET:
	case FS_SNS_REQ_RETWEET:
	case FS_SNS_REQ_SET_LIKE:
	case FS_SNS_REQ_REPLY:
	case FS_SNS_REQ_GET_SUMMARY:
	case FS_SNS_REQ_UPLOAD_PHOTO:
	case FS_SNS_REQ_EML_SET_ACOUNT:
	case FS_SNS_REQ_EML_SEND:
	case FS_SNS_REQ_RSS_SET_MY_CHANNEL:
	case FS_SNS_REQ_ADD_FRIEND:
	case FS_SNS_REQ_EML_ADD_CONTACT:
	case FS_SNS_REQ_EML_DEL_CONTACT:
	case FS_SNS_REQ_EML_MOD_CONTACT:
		break;
	case FS_SNS_REQ_GET_TWEET:
		FS_SnsFreeMsg( &rsp->data.get_tweet.msg );
		break;
	case FS_SNS_REQ_GET_UPDATE:
	case FS_SNS_REQ_GET_REPLY:
	case FS_SNS_REQ_GET_UPDATE_CONTENT:
		for ( i = 0; i < FS_SNS_MAX_MSGS_COUNT; i ++ ) {
			FS_SnsFreeMsg( &rsp->data.msglist.msgs[i] );
		}
		break;
	case FS_SNS_REQ_GET_FRIENDS:
	case FS_SNS_REQ_FIND_FRIENDS:
		for ( i = 0; i < FS_SNS_MAX_MSGS_COUNT; i ++ ) {
			FS_SnsFreeFriends( &rsp->data.friends.friends[i] );
		}
		break;
	case FS_SNS_REQ_RSS_GET_ALL_ARTICLE:
	case FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL:
	case FS_SNS_REQ_RSS_GET_ALL_ARTICLE_CONTENT:
	case FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL_CONTENT:
		for ( i = 0; i < FS_SNS_MAX_MSGS_COUNT; i ++ ) {
			FS_SnsFreeRssArticle( &rsp->data.rss_article.articles[i] );
		}
		break;
	case FS_SNS_REQ_RSS_GET_ALL_CHANNEL:
		for ( i = 0; i < FS_SNS_MAX_MSGS_COUNT; i ++ ) {
			FS_SnsFreeRssChannel( &rsp->data.rss_channel.channels[i] );
		}
		break;
	case FS_SNS_REQ_EML_RECV:
	case FS_SNS_REQ_EML_RECV_CONTENT:
		for ( i = 0; i < FS_SNS_MAX_MSGS_COUNT; i ++ ) {
			FS_SnsFreeEmail( &rsp->data.eml_list.emails[i] );
		}
		break;
	case FS_SNS_REQ_EML_GET_ACOUNT:
		FS_SAFE_FREE( rsp->data.eml_account.email );
		FS_SAFE_FREE( rsp->data.eml_account.smtp );
		FS_SAFE_FREE( rsp->data.eml_account.pop3 );
		FS_SAFE_FREE( rsp->data.eml_account.password );
		break;
	case FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY:
		for ( i = 0; i < FS_SNS_MAX_MSGS_COUNT; i ++ ) {
			FS_SnsFreeRssCategory( &rsp->data.rss_category.items[i] );
		}
		break;
	case FS_SNS_REQ_RSS_GET_CATEGORY_DETAIL:
		for ( i = 0; i < FS_SNS_MAX_MSGS_COUNT; i ++ ) {
			FS_SnsFreeRssCategoryDetail( &rsp->data.rss_category_detail.items[i] );
		}
		break;
	case FS_SNS_REQ_EML_GET_CONTACT:
		for ( i = 0; i < FS_SNS_MAX_MSGS_COUNT; i ++ ) {
			FS_SnsFreeEmlContact( &rsp->data.eml_contact.contacts[i] );
		}
		break;
	default:
		FS_TRACE1( "FS_SnsFreeResponse ERROR unknow reqCode(%d)", reqCode );
		break;
	}
	FS_SAFE_FREE( rsp->err_info );
	IFS_Memset( rsp, 0, sizeof(FS_SnsResponse) );
}

static void FS_SnsCopyRequestData( FS_SnsSession *sns, FS_SnsRequest *pReq )
{
	sns->req.req = pReq->req;
	switch ( pReq->req )
	{
	case FS_SNS_REQ_LOGIN:
		FS_COPY_TEXT( sns->req.data.login.user, pReq->data.login.user );
		FS_COPY_TEXT( sns->req.data.login.pass, pReq->data.login.pass );
		break;
	case FS_SNS_REQ_REGISTER:
		FS_COPY_TEXT( sns->req.data.reg.user, pReq->data.reg.user );
		FS_COPY_TEXT( sns->req.data.reg.pass, pReq->data.reg.pass );
		FS_COPY_TEXT( sns->req.data.reg.email, pReq->data.reg.email );
		break;
	case FS_SNS_REQ_GET_SUMMARY:
		break;
	case FS_SNS_REQ_GET_TWEET:
		FS_COPY_TEXT( sns->req.data.get_tweet.type, pReq->data.get_tweet.type );
		break;
	case FS_SNS_REQ_GET_UPDATE:
	case FS_SNS_REQ_GET_REPLY:
	case FS_SNS_REQ_GET_UPDATE_CONTENT:
		FS_COPY_TEXT( sns->req.data.get_msgs.id, pReq->data.get_msgs.id );
		FS_COPY_TEXT( sns->req.data.get_msgs.type, pReq->data.get_msgs.type );
		FS_COPY_TEXT( sns->req.data.get_msgs.name, pReq->data.get_msgs.name );
		sns->req.data.get_msgs.count = pReq->data.get_msgs.count;
		sns->req.data.get_msgs.page = pReq->data.get_msgs.page;
		break;
	case FS_SNS_REQ_SET_TWEET:
		FS_COPY_TEXT( sns->req.data.set_tweet.type, pReq->data.set_tweet.type );
		FS_COPY_TEXT( sns->req.data.set_tweet.sync_to, pReq->data.set_tweet.sync_to );
		FS_COPY_TEXT( sns->req.data.set_tweet.msg, pReq->data.set_tweet.msg );
		break;
	case FS_SNS_REQ_RETWEET:
	case FS_SNS_REQ_SET_LIKE:
	case FS_SNS_REQ_REPLY:
		FS_COPY_TEXT( sns->req.data.reply.type, pReq->data.reply.type );
		FS_COPY_TEXT( sns->req.data.reply.id, pReq->data.reply.id );
		FS_COPY_TEXT( sns->req.data.reply.msg, pReq->data.reply.msg );
		break;
	case FS_SNS_REQ_UPLOAD_PHOTO:
	{
		/* need encode as multipart/form-data to upload */
		FS_CHAR *boundary = FS_SNS_BOUNDARY;
		FS_CHAR *fname = pReq->data.photo.fname;
		FS_CHAR szTxt[512] = {0};
		FS_SINT4 len, offset = 0, srcoffset = 0;
		FS_CHAR *srcbuf = IFS_Malloc( FS_FILE_BLOCK );
		FS_CHAR *name = FS_GetFileNameFromPath( fname );
		FS_CHAR *mime = FS_GetMimeFromExt( name );

		if ( srcbuf == FS_NULL ) {
			FS_SNS_TRACE0("FS_SnsCopyRequestData ERROR memory." );
			return;
		}

		/* form_data header */
		IFS_Snprintf( szTxt, sizeof(szTxt) - 1, 
			"--%s\r\nContent-Disposition: form-data; name=\"img\"; filename=\"%s\"\r\nContent-Type: %s\r\n\r\n",
			boundary, name, mime );

		len = IFS_Strlen( szTxt );
		if ( len >= sizeof(szTxt) - 1 ) {
			FS_SNS_TRACE0("FS_SnsCopyRequestData ERROR Buffer overflow" );
		}
		FS_FileWrite( FS_DIR_TMP, FS_SNS_FORM_DATA_FILE, offset, szTxt, len );
		offset += len;
		/* form_data body */
		do {
			len = FS_FileRead( -1, fname, srcoffset, srcbuf, FS_FILE_BLOCK );
			if ( len >= 0 ) {
				FS_FileWrite( FS_DIR_TMP, FS_SNS_FORM_DATA_FILE, offset, srcbuf, len );
				offset += len;
				srcoffset += len;
			}
		} while( len == FS_FILE_BLOCK );
		/* form_data end boundary */
		IFS_Snprintf( szTxt, sizeof(szTxt) - 1, "\r\n--%s--\r\n", boundary );
		len = IFS_Strlen( szTxt );
		FS_FileWrite( FS_DIR_TMP, FS_SNS_FORM_DATA_FILE, offset, szTxt, len );
		offset += len;
		IFS_Free( srcbuf );
		/* save request parameter */
		FS_GetAbsFileName( FS_DIR_TMP, FS_SNS_FORM_DATA_FILE, szTxt );
		FS_COPY_TEXT( sns->req.data.photo.fname, szTxt );
		sns->req.data.photo.fsize = offset;
		FS_COPY_TEXT( sns->req.data.photo.type, pReq->data.photo.type );
		FS_COPY_TEXT( sns->req.data.photo.sync_to, pReq->data.photo.sync_to );
		FS_COPY_TEXT( sns->req.data.photo.msg, pReq->data.photo.msg );
		break;
	}
	case FS_SNS_REQ_GET_FRIENDS:
	case FS_SNS_REQ_FIND_FRIENDS:
	case FS_SNS_REQ_ADD_FRIEND:
	case FS_SNS_REQ_DEL_FRIEND:
		FS_COPY_TEXT( sns->req.data.friends.type, pReq->data.friends.type );
		FS_COPY_TEXT( sns->req.data.friends.key, pReq->data.friends.key );
		sns->req.data.friends.page = pReq->data.friends.page;
		sns->req.data.friends.count = pReq->data.friends.count;
		break;
	case FS_SNS_REQ_RSS_GET_ALL_ARTICLE:
	case FS_SNS_REQ_RSS_GET_ALL_CHANNEL:
	case FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL:
	case FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY:
	case FS_SNS_REQ_RSS_GET_CATEGORY_DETAIL:
	case FS_SNS_REQ_RSS_SET_MY_CHANNEL:
	case FS_SNS_REQ_RSS_GET_ALL_ARTICLE_CONTENT:
	case FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL_CONTENT:
		FS_COPY_TEXT( sns->req.data.rss.id, pReq->data.rss.id );
		sns->req.data.rss.page = pReq->data.rss.page;
		sns->req.data.rss.count = pReq->data.rss.count;
		break;
	case FS_SNS_REQ_EML_SET_ACOUNT:
		FS_COPY_TEXT( sns->req.data.eml_account.email, pReq->data.eml_account.email );
		FS_COPY_TEXT( sns->req.data.eml_account.smtp, pReq->data.eml_account.smtp );
		FS_COPY_TEXT( sns->req.data.eml_account.pop3, pReq->data.eml_account.pop3 );
		FS_COPY_TEXT( sns->req.data.eml_account.password, pReq->data.eml_account.password );
		sns->req.data.eml_account.bind_home = pReq->data.eml_account.bind_home;
		break;
	case FS_SNS_REQ_EML_SEND:
		FS_COPY_TEXT( sns->req.data.eml_send.receiver, pReq->data.eml_send.receiver );
		FS_COPY_TEXT( sns->req.data.eml_send.subject, pReq->data.eml_send.subject );
		FS_COPY_TEXT( sns->req.data.eml_send.content, pReq->data.eml_send.content );
		break;
	case FS_SNS_REQ_EML_RECV:
	case FS_SNS_REQ_EML_RECV_CONTENT:
		FS_COPY_TEXT( sns->req.data.eml_recv.id, pReq->data.eml_recv.id );
		sns->req.data.eml_recv.page = pReq->data.eml_recv.page;
		sns->req.data.eml_recv.count = pReq->data.eml_recv.count;
		break;
	case FS_SNS_REQ_EML_GET_ACOUNT:
		break;
	case FS_SNS_REQ_EML_GET_CONTACT:
	case FS_SNS_REQ_EML_ADD_CONTACT:
	case FS_SNS_REQ_EML_DEL_CONTACT:
	case FS_SNS_REQ_EML_MOD_CONTACT:
		FS_COPY_TEXT( sns->req.data.eml_contact.id, pReq->data.eml_contact.id );
		FS_COPY_TEXT( sns->req.data.eml_contact.name, pReq->data.eml_contact.name );
		FS_COPY_TEXT( sns->req.data.eml_contact.email, pReq->data.eml_contact.email );
		sns->req.data.eml_contact.page = pReq->data.eml_contact.page;
		sns->req.data.eml_contact.count = pReq->data.eml_contact.count;
		break;
	default:
		FS_SNS_TRACE1( "FS_SnsCopyRequestData ERROR. unknow req(%d)", pReq->req );
		break;
	}
}

static void FS_SnsFreeRequest( FS_SnsSession *sns )
{
	switch ( sns->req.req )
	{
	case FS_SNS_REQ_LOGIN:
		FS_SAFE_FREE( sns->req.data.login.user );
		FS_SAFE_FREE( sns->req.data.login.pass );
		break;
	case FS_SNS_REQ_REGISTER:
		FS_SAFE_FREE( sns->req.data.reg.user );
		FS_SAFE_FREE( sns->req.data.reg.pass );
		FS_SAFE_FREE( sns->req.data.reg.email );
		break;
	case FS_SNS_REQ_GET_SUMMARY:
		break;
	case FS_SNS_REQ_GET_TWEET:
		FS_SAFE_FREE( sns->req.data.get_tweet.type );
		break;
	case FS_SNS_REQ_GET_UPDATE:
	case FS_SNS_REQ_GET_REPLY:
	case FS_SNS_REQ_GET_UPDATE_CONTENT:
		FS_SAFE_FREE( sns->req.data.get_msgs.id );
		FS_SAFE_FREE( sns->req.data.get_msgs.type );
		FS_SAFE_FREE( sns->req.data.get_msgs.name );
		break;
	case FS_SNS_REQ_SET_TWEET:
		FS_SAFE_FREE( sns->req.data.set_tweet.type );
		FS_SAFE_FREE( sns->req.data.set_tweet.sync_to );
		FS_SAFE_FREE( sns->req.data.set_tweet.msg );
		break;
	case FS_SNS_REQ_RETWEET:
	case FS_SNS_REQ_SET_LIKE:
	case FS_SNS_REQ_REPLY:
		FS_SAFE_FREE( sns->req.data.reply.type );
		FS_SAFE_FREE( sns->req.data.reply.id );
		FS_SAFE_FREE( sns->req.data.reply.msg );
		break;
	case FS_SNS_REQ_UPLOAD_PHOTO:
		FS_FileDelete( -1, sns->req.data.photo.fname );
		FS_SAFE_FREE( sns->req.data.photo.fname );
		FS_SAFE_FREE( sns->req.data.photo.type );
		FS_SAFE_FREE( sns->req.data.photo.sync_to );
		FS_SAFE_FREE( sns->req.data.photo.msg );
		break;
	case FS_SNS_REQ_GET_FRIENDS:
	case FS_SNS_REQ_FIND_FRIENDS:
	case FS_SNS_REQ_ADD_FRIEND:
	case FS_SNS_REQ_DEL_FRIEND:
		FS_SAFE_FREE( sns->req.data.friends.type );
		FS_SAFE_FREE( sns->req.data.friends.key );
		break;
	case FS_SNS_REQ_RSS_GET_ALL_ARTICLE:
	case FS_SNS_REQ_RSS_GET_ALL_CHANNEL:
	case FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL:
	case FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY:
	case FS_SNS_REQ_RSS_GET_CATEGORY_DETAIL:
	case FS_SNS_REQ_RSS_SET_MY_CHANNEL:
	case FS_SNS_REQ_RSS_GET_ALL_ARTICLE_CONTENT:
	case FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL_CONTENT:
		FS_SAFE_FREE( sns->req.data.rss.id );
		break;
	case FS_SNS_REQ_EML_SET_ACOUNT:
		FS_SAFE_FREE( sns->req.data.eml_account.email );
		FS_SAFE_FREE( sns->req.data.eml_account.smtp );
		FS_SAFE_FREE( sns->req.data.eml_account.pop3 );
		FS_SAFE_FREE( sns->req.data.eml_account.password );
		break;
	case FS_SNS_REQ_EML_SEND:
		FS_SAFE_FREE( sns->req.data.eml_send.receiver );
		FS_SAFE_FREE( sns->req.data.eml_send.subject );
		FS_SAFE_FREE( sns->req.data.eml_send.content );
		break;
	case FS_SNS_REQ_EML_RECV:
	case FS_SNS_REQ_EML_RECV_CONTENT:
		FS_SAFE_FREE( sns->req.data.eml_recv.id );
		break;
	case FS_SNS_REQ_EML_GET_ACOUNT:
		break;
	case FS_SNS_REQ_EML_GET_CONTACT:
	case FS_SNS_REQ_EML_ADD_CONTACT:
	case FS_SNS_REQ_EML_DEL_CONTACT:
	case FS_SNS_REQ_EML_MOD_CONTACT:
		FS_SAFE_FREE( sns->req.data.eml_contact.id );
		FS_SAFE_FREE( sns->req.data.eml_contact.name );
		FS_SAFE_FREE( sns->req.data.eml_contact.email );
		break;
	default:
		FS_SNS_TRACE1( "FS_SnsFreeRequest ERROR. unknow req(%d)", sns->req.req );
		break;
	}
	IFS_Memset( &sns->req, 0, sizeof(FS_SnsRequest) );
}

/************************************************************************/
/* SNS XML handler                                                      */
/************************************************************************/

static void FS_SnsXmlParseSummary( FS_SnsSession *sns, FS_CHAR *name, FS_CHAR *value )
{
	FS_SINT4 i;

	if (FS_STR_I_EQUAL(name, "type")) {
		for ( i = 0; i < sns->config->account_num; i ++ ) {
			if ( FS_STR_I_EQUAL(value, sns->config->accout[i].name) ) {
				sns->account_idx = i;
				return;
			}
		}
		sns->account_idx = -1;	/* did not found any match account */
		FS_SNS_TRACE1( "FS_SnsXmlParseSummary ERROR. %s unknown sns", value );
	} else if ( sns->account_idx >= 0 &&  FS_STR_I_EQUAL(name, "updates") ) {
		sns->config->accout[sns->account_idx].update_cnt = IFS_Atoi(value);
	} else if ( sns->account_idx >= 0 &&  FS_STR_I_EQUAL(name, "reply") ) {
		sns->config->accout[sns->account_idx].reply_cnt = IFS_Atoi(value);
	}
}

static void FS_SnsXmlParseMsg( FS_SnsMsg *msg, FS_CHAR *name, FS_CHAR *value )
{
	FS_CHAR *str;

	if (FS_STR_I_EQUAL(name, "id")) {
		FS_COPY_TEXT( msg->id, value );
	} else if ( FS_STR_I_EQUAL(name, "author") ) {
		FS_COPY_TEXT( msg->author, value );
	} else if ( FS_STR_I_EQUAL(name, "username") ) {
		FS_COPY_TEXT( msg->username, value );
	} else if ( FS_STR_I_EQUAL(name, "date") ) {
		str = IFS_Strstr( value, "+0" );
		if ( str ) {
			*str = 0;
		}
		FS_COPY_TEXT( msg->date, value );
	} else if ( FS_STR_I_EQUAL(name, "icons") ) {
		FS_COPY_TEXT( msg->icon_url, value );
	} else if ( FS_STR_I_EQUAL(name, "msg") ) {
		FS_ProcessEsc( value, -1 );
		FS_COPY_TEXT( msg->msg, value );
	} else if ( FS_STR_I_EQUAL(name, "type") ) {
		FS_COPY_TEXT( msg->type, value );
	}
}

static void FS_SnsXmlParseFriends( FS_SnsFriends *friends, FS_CHAR *name, FS_CHAR *value )
{	
	if (FS_STR_I_EQUAL(name, "id")) {
		FS_COPY_TEXT( friends->id, value );
	} else if ( FS_STR_I_EQUAL(name, "username") ) {
		FS_COPY_TEXT( friends->username, value );
	} else if ( FS_STR_I_EQUAL(name, "author") ) {
		FS_COPY_TEXT( friends->author, value );
	} else if ( FS_STR_I_EQUAL(name, "icons") ) {
		FS_COPY_TEXT( friends->icon_url, value );
	} else if ( FS_STR_I_EQUAL(name, "location") ) {
		FS_COPY_TEXT( friends->location, value );
	} else if ( FS_STR_I_EQUAL(name, "sex") ) {
		FS_COPY_TEXT( friends->sex, value );
	} else if ( FS_STR_I_EQUAL(name, "relation") ) {
		FS_COPY_TEXT( friends->relation, value );
	} else if ( FS_STR_I_EQUAL(name, "isfriend") ) {
		if ( FS_STR_NI_EQUAL(value, "yes", 3) ) {
			friends->is_friend = FS_TRUE;
		}
	}
}

static void FS_SnsXmlParseRssArticle( FS_SnsRssArticle *article, FS_CHAR *name, FS_CHAR *value )
{
	if (FS_STR_I_EQUAL(name, "id")) {
		FS_COPY_TEXT( article->id, value );
	} else if (FS_STR_I_EQUAL(name, "channels")) {
		FS_COPY_TEXT( article->channel_name, value );
	} else if ( FS_STR_I_EQUAL(name, "title") ) {
		FS_ProcessEsc( value, -1 );
		FS_COPY_TEXT( article->title, value );
	} else if ( FS_STR_I_EQUAL(name, "date") ) {
		FS_COPY_TEXT( article->date, value );
	} else if ( FS_STR_I_EQUAL(name, "msg") ) {
		FS_ProcessEsc( value, -1 );
		FS_COPY_TEXT( article->msg, value );
	}
}

static void FS_SnsXmlParseRssChannel( FS_SnsRssChannel *channel, FS_CHAR *name, FS_CHAR *value )
{		
	if (FS_STR_I_EQUAL(name, "id")) {
		FS_COPY_TEXT( channel->id, value );
	} else if ( FS_STR_I_EQUAL(name, "icon") ) {
		FS_COPY_TEXT( channel->icon_url, value );
	} else if ( FS_STR_I_EQUAL(name, "channels") ) {
		FS_COPY_TEXT( channel->channel_name, value );
	} else if ( FS_STR_I_EQUAL(name, "count") ) {
		channel->count = IFS_Atoi(value);
	}
}

static void FS_SnsXmlParseEmail( FS_SnsEmail *email, FS_CHAR *name, FS_CHAR *value )
{		
	if (FS_STR_I_EQUAL(name, "id")) {
		FS_COPY_TEXT( email->id, value );
	} else if (FS_STR_I_EQUAL(name, "sender")) {
		FS_COPY_TEXT( email->sender, value );
	} else if ( FS_STR_I_EQUAL(name, "date") ) {
		FS_COPY_TEXT( email->date, value );
	} else if ( FS_STR_I_EQUAL(name, "subject") ) {
		FS_COPY_TEXT( email->subject, value );
	} else if ( FS_STR_I_EQUAL(name, "content") ) {
		FS_ProcessEsc( value, -1 );
		FS_COPY_TEXT( email->content, value );
	}
}

static void FS_SnsXmlElementAttr( FS_SnsSession *sns, FS_CHAR *ename, FS_CHAR *name, FS_CHAR *value )
{
	FS_CHAR * str = FS_NULL;

	switch (sns->req.req) {
	case FS_SNS_REQ_GET_TWEET:
		if ( FS_STR_I_EQUAL(ename, "data") ) {
			FS_SnsXmlParseMsg( &sns->rsp.data.get_tweet.msg, name, value );
		}
		break;
	case FS_SNS_REQ_GET_UPDATE:
	case FS_SNS_REQ_GET_REPLY:
	case FS_SNS_REQ_GET_UPDATE_CONTENT:
		if ( sns->rsp.data.msglist.count >= FS_SNS_MAX_MSGS_COUNT ) {
			FS_SNS_TRACE0( "FS_SnsXmlElementAttr ERROR data.msglist.count exceed limit" );
			return;
		}
		if ( FS_STR_I_EQUAL(ename, "data") ) {
			FS_SnsXmlParseMsg( &sns->rsp.data.msglist.msgs[sns->rsp.data.msglist.count], name, value );
		}
		break;
	case FS_SNS_REQ_GET_SUMMARY:
		if ( FS_STR_I_EQUAL(ename, "data") ) {
			FS_SnsXmlParseSummary( sns, name, value );
		}
		break;
	case FS_SNS_REQ_GET_FRIENDS:
	case FS_SNS_REQ_FIND_FRIENDS:
		if ( sns->rsp.data.friends.count >= FS_SNS_MAX_MSGS_COUNT ) {
			FS_SNS_TRACE0( "FS_SnsXmlElementAttr ERROR data.friends.count exceed limit" );
			return;
		}
		if ( FS_STR_I_EQUAL(ename, "data") ) {
			FS_SnsXmlParseFriends( &sns->rsp.data.friends.friends[sns->rsp.data.friends.count], name, value );
		}
		break;
	case FS_SNS_REQ_RSS_GET_ALL_ARTICLE:
	case FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL:
	case FS_SNS_REQ_RSS_GET_ALL_ARTICLE_CONTENT:
	case FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL_CONTENT:
		if ( sns->rsp.data.rss_article.count >= FS_SNS_MAX_MSGS_COUNT ) {
			FS_SNS_TRACE0( "FS_SnsXmlElementAttr ERROR data.rss_article.count exceed limit" );
			return;
		}
		if ( FS_STR_I_EQUAL(ename, "data") ) {
			FS_SnsXmlParseRssArticle( &sns->rsp.data.rss_article.articles[sns->rsp.data.rss_article.count], name, value );
		}
		break;
	case FS_SNS_REQ_RSS_GET_ALL_CHANNEL:
		if ( sns->rsp.data.rss_channel.count >= FS_SNS_MAX_MSGS_COUNT ) {
			FS_SNS_TRACE0( "FS_SnsXmlElementAttr ERROR data.rss_channel.count exceed limit" );
			return;
		}
		if ( FS_STR_I_EQUAL(ename, "data") ) {
			FS_SnsXmlParseRssChannel( &sns->rsp.data.rss_channel.channels[sns->rsp.data.rss_channel.count], name, value );
		}
		break;
	case FS_SNS_REQ_EML_RECV:
	case FS_SNS_REQ_EML_RECV_CONTENT:
		if ( sns->rsp.data.eml_list.count >= FS_SNS_MAX_MSGS_COUNT ) {
			FS_SNS_TRACE0( "FS_SnsXmlElementAttr ERROR data.eml_list.count exceed limit" );
			return;
		}
		if ( FS_STR_I_EQUAL(ename, "data") ) {
			FS_SnsXmlParseEmail( &sns->rsp.data.eml_list.emails[sns->rsp.data.eml_list.count], name, value );
		}
		break;
	default:
		break;
	}
}

static void FS_SnsXmlEndElement( FS_SnsSession *sns, FS_CHAR *ename )
{
	if ( sns->req.req == FS_SNS_REQ_GET_UPDATE 
		|| sns->req.req == FS_SNS_REQ_GET_REPLY 
		|| sns->req.req == FS_SNS_REQ_GET_UPDATE_CONTENT) {
		if( FS_STR_I_EQUAL(ename, "data") ) {
			sns->rsp.data.msglist.count ++;
			if ( sns->rsp.data.msglist.count > FS_SNS_MAX_MSGS_COUNT ) {
				FS_SNS_TRACE0( "FS_SnsXmlEndElement ERROR data.msglist.count exceed limit" );
				sns->rsp.data.msglist.count = FS_SNS_MAX_MSGS_COUNT;
			}
		}
	} else if ( sns->req.req == FS_SNS_REQ_GET_FRIENDS || sns->req.req == FS_SNS_REQ_FIND_FRIENDS ) {
		if( FS_STR_I_EQUAL(ename, "data") ) {
			sns->rsp.data.friends.count ++;
			if ( sns->rsp.data.friends.count > FS_SNS_MAX_MSGS_COUNT ) {
				FS_SNS_TRACE0( "FS_SnsXmlEndElement ERROR data.friends.count exceed limit" );
				sns->rsp.data.friends.count = FS_SNS_MAX_MSGS_COUNT;
			}
		}
	} else if ( sns->req.req == FS_SNS_REQ_RSS_GET_ALL_ARTICLE 
		|| sns->req.req == FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL
		|| sns->req.req == FS_SNS_REQ_RSS_GET_ALL_ARTICLE_CONTENT 
		|| sns->req.req == FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL_CONTENT ) {
		if( FS_STR_I_EQUAL(ename, "data") ) {
			sns->rsp.data.rss_article.count ++;
			if ( sns->rsp.data.rss_article.count > FS_SNS_MAX_MSGS_COUNT ) {
				FS_SNS_TRACE0( "FS_SnsXmlEndElement ERROR data.rss_article.count exceed limit" );
				sns->rsp.data.rss_article.count = FS_SNS_MAX_MSGS_COUNT;
			}
		}
	} else if ( sns->req.req == FS_SNS_REQ_RSS_GET_ALL_CHANNEL ) {
		if( FS_STR_I_EQUAL(ename, "data") ) {
			sns->rsp.data.rss_channel.count ++;
			if ( sns->rsp.data.rss_channel.count > FS_SNS_MAX_MSGS_COUNT ) {
				FS_SNS_TRACE0( "FS_SnsXmlEndElement ERROR data.rss_channel.count exceed limit" );
				sns->rsp.data.rss_channel.count = FS_SNS_MAX_MSGS_COUNT;
			}
		}
	} else if ( sns->req.req == FS_SNS_REQ_EML_RECV || sns->req.req == FS_SNS_REQ_EML_RECV_CONTENT ) {
		if( FS_STR_I_EQUAL(ename, "data") ) {
			sns->rsp.data.eml_list.count ++;
			if ( sns->rsp.data.eml_list.count > FS_SNS_MAX_MSGS_COUNT ) {
				FS_SNS_TRACE0( "FS_SnsXmlEndElement ERROR data.eml_list.count exceed limit" );
				sns->rsp.data.eml_list.count = FS_SNS_MAX_MSGS_COUNT;
			}
		}
	} else if ( sns->req.req == FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY ) {
		if( FS_STR_I_EQUAL(ename, "category") ) {
			sns->rsp.data.rss_category.count ++;
			if ( sns->rsp.data.rss_category.count > FS_SNS_MAX_MSGS_COUNT ) {
				FS_SNS_TRACE0( "FS_SnsXmlEndElement ERROR data.rss_category.count exceed limit" );
				sns->rsp.data.rss_category.count = FS_SNS_MAX_MSGS_COUNT;
			}
		}
	} else if ( sns->req.req == FS_SNS_REQ_RSS_GET_CATEGORY_DETAIL ) {
		if( FS_STR_I_EQUAL(ename, "channel") ) {
			sns->rsp.data.rss_category_detail.count ++;
			if ( sns->rsp.data.rss_category_detail.count > FS_SNS_MAX_MSGS_COUNT ) {
				FS_SNS_TRACE0( "FS_SnsXmlEndElement ERROR data.rss_category_detail.count exceed limit" );
				sns->rsp.data.rss_category_detail.count = FS_SNS_MAX_MSGS_COUNT;
			}
		}
	} else if ( sns->req.req == FS_SNS_REQ_EML_GET_CONTACT ) {
		if( FS_STR_I_EQUAL(ename, "mailitem") ) {
			sns->rsp.data.eml_contact.count ++;
			if ( sns->rsp.data.eml_contact.count > FS_SNS_MAX_MSGS_COUNT ) {
				FS_SNS_TRACE0( "FS_SnsXmlEndElement ERROR data.eml_contact.count exceed limit" );
				sns->rsp.data.eml_contact.count = FS_SNS_MAX_MSGS_COUNT;
			}
		}
	}
}

static void FS_SnsXmlElementText( FS_SnsSession *sns, FS_CHAR *ename, FS_CHAR *str, FS_SINT4 slen )
{
	if ( str == FS_NULL || slen <= 0 ) return;
	
	if ( sns->req.req == FS_SNS_REQ_EML_GET_ACOUNT ) {
		if ( FS_STR_I_EQUAL(ename, "email") && sns->rsp.data.eml_account.email == FS_NULL ) {
			sns->rsp.data.eml_account.email = FS_Strndup( str, slen );
		} else if ( FS_STR_I_EQUAL(ename, "smtp") && sns->rsp.data.eml_account.smtp == FS_NULL ) {
			sns->rsp.data.eml_account.smtp = FS_Strndup( str, slen );
		} else if ( FS_STR_I_EQUAL(ename, "pop3") && sns->rsp.data.eml_account.pop3 == FS_NULL ) {
			sns->rsp.data.eml_account.pop3 = FS_Strndup( str, slen );
		} else if ( FS_STR_I_EQUAL(ename, "password") && sns->rsp.data.eml_account.password == FS_NULL ) {
			sns->rsp.data.eml_account.password = FS_Strndup( str, slen );
		} else if ( FS_STR_I_EQUAL(ename, "bindhome") && FS_STR_NI_EQUAL(str, "yes", 3) ) {
			sns->rsp.data.eml_account.bind_home = FS_TRUE;
		}
	} else if ( sns->req.req == FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY ) {
		if ( sns->rsp.data.rss_category.count >= FS_SNS_MAX_MSGS_COUNT ) {
			return;
		}
		if ( FS_STR_I_EQUAL(ename, "id") && sns->rsp.data.rss_category.items[sns->rsp.data.rss_category.count].id == FS_NULL ) {
			sns->rsp.data.rss_category.items[sns->rsp.data.rss_category.count].id = FS_Strndup( str, slen );
		} else if ( FS_STR_I_EQUAL(ename, "icon") && sns->rsp.data.rss_category.items[sns->rsp.data.rss_category.count].icon_url == FS_NULL ) {
			sns->rsp.data.rss_category.items[sns->rsp.data.rss_category.count].icon_url = FS_Strndup( str, slen );
		} else if ( FS_STR_I_EQUAL(ename, "name") && sns->rsp.data.rss_category.items[sns->rsp.data.rss_category.count].name == FS_NULL ) {
			sns->rsp.data.rss_category.items[sns->rsp.data.rss_category.count].name = FS_Strndup( str, slen );
		} else if ( FS_STR_I_EQUAL(ename, "count") && sns->rsp.data.rss_category.items[sns->rsp.data.rss_category.count].count == 0 ) {
			sns->rsp.data.rss_category.items[sns->rsp.data.rss_category.count].count = IFS_Atoi(str);
		}
	} else if ( sns->req.req == FS_SNS_REQ_RSS_GET_CATEGORY_DETAIL ) {
		if ( sns->rsp.data.rss_category_detail.count >= FS_SNS_MAX_MSGS_COUNT ) {
			return;
		}
		if ( FS_STR_I_EQUAL(ename, "id") && sns->rsp.data.rss_category_detail.items[sns->rsp.data.rss_category_detail.count].id == FS_NULL ) {
			sns->rsp.data.rss_category_detail.items[sns->rsp.data.rss_category_detail.count].id = FS_Strndup( str, slen );
		} else if ( FS_STR_I_EQUAL(ename, "icon") && sns->rsp.data.rss_category_detail.items[sns->rsp.data.rss_category_detail.count].icon_url == FS_NULL ) {
			sns->rsp.data.rss_category_detail.items[sns->rsp.data.rss_category_detail.count].icon_url = FS_Strndup( str, slen );
		} else if ( FS_STR_I_EQUAL(ename, "name") && sns->rsp.data.rss_category_detail.items[sns->rsp.data.rss_category_detail.count].name == FS_NULL ) {
			sns->rsp.data.rss_category_detail.items[sns->rsp.data.rss_category_detail.count].name = FS_Strndup( str, slen );
		} else if ( FS_STR_I_EQUAL(ename, "lang") && sns->rsp.data.rss_category_detail.items[sns->rsp.data.rss_category_detail.count].language == FS_NULL ) {
			sns->rsp.data.rss_category_detail.items[sns->rsp.data.rss_category_detail.count].language = FS_Strndup( str, slen );
		} else if( FS_STR_I_EQUAL(ename, "selected") && FS_STR_NI_EQUAL(str, "yes", 3) ) {
			sns->rsp.data.rss_category_detail.items[sns->rsp.data.rss_category_detail.count].selected = FS_TRUE;
		}
	} else if( sns->req.req == FS_SNS_REQ_RSS_SET_MY_CHANNEL ) {
		if( FS_STR_I_EQUAL(ename, "code") && FS_STR_NI_EQUAL(str, "ok", 2) ) {
			sns->rsp.result = FS_SNS_RSP_SUCCESS;
		} else if ( FS_STR_I_EQUAL(ename, "reason") && str && slen > 0 ) {
			sns->rsp.err_info = FS_Strndup( str, slen );
		} 
	} else if( sns->req.req == FS_SNS_REQ_EML_GET_CONTACT ) {
		if ( sns->rsp.data.eml_contact.count >= FS_SNS_MAX_MSGS_COUNT ) {
			return;
		}
		if ( FS_STR_I_EQUAL(ename, "id") && sns->rsp.data.eml_contact.contacts[sns->rsp.data.eml_contact.count].id == FS_NULL ) {
			sns->rsp.data.eml_contact.contacts[sns->rsp.data.eml_contact.count].id = FS_Strndup( str, slen );
		} else if ( FS_STR_I_EQUAL(ename, "name") && sns->rsp.data.eml_contact.contacts[sns->rsp.data.eml_contact.count].name == FS_NULL ) {
			sns->rsp.data.eml_contact.contacts[sns->rsp.data.eml_contact.count].name = FS_Strndup( str, slen );
		} else if ( FS_STR_I_EQUAL(ename, "email") && sns->rsp.data.eml_contact.contacts[sns->rsp.data.eml_contact.count].email == FS_NULL ) {
			sns->rsp.data.eml_contact.contacts[sns->rsp.data.eml_contact.count].email = FS_Strndup( str, slen );
		}
	}
}

static void FS_SnsXmlFileRead( FS_SnsSession *sns, FS_SaxHandle hsax )
{
	FS_SINT4 rlen;
	FS_BOOL bDone = FS_FALSE;
	FS_BYTE *buf = IFS_Malloc( FS_FILE_BLOCK );
	if( buf )
	{
		rlen = FS_FileRead( FS_DIR_TMP, sns->file, sns->offset, buf, FS_FILE_BLOCK );
		if( rlen < FS_FILE_BLOCK )
			bDone = FS_TRUE;
		sns->offset += rlen;
		FS_SaxDataFeed( hsax, buf, rlen, bDone );
		IFS_Free( buf );
	}
}

static void FS_SnsXmlParseFile( FS_SnsSession *sns )
{
	FS_SaxHandle hsax = FS_NULL;
	
	sns->offset = 0;
	hsax = FS_CreateSaxHandler( sns );
	FS_SaxSetAttributeHandler( hsax, FS_SnsXmlElementAttr );
	FS_SaxSetElementTextHandler( hsax, FS_SnsXmlElementText );
	FS_SaxSetEndElementHandler( hsax, FS_SnsXmlEndElement );
	FS_SaxSetDataRequest( hsax, FS_SnsXmlFileRead );
	
	FS_SaxProcXmlDoc( hsax );
	FS_FreeSaxHandler( hsax );
}

static void FS_SnsLoginHandler( FS_SnsSession *sns, FS_BYTE *data, FS_SINT4 dlen )
{
	FS_CHAR *pRsp = (FS_CHAR *)data;
	FS_SnsRspCode rspCode = FS_SNS_RSP_ERR_UNKNOW;
	FS_SINT4 i;

	FS_SNS_TRACE2( "FS_SnsLoginHandler len=%d, data=%s", dlen, data );
	while ( *pRsp == '\r' || *pRsp == '\n' || *pRsp == '\t' || *pRsp == ' ' )
		pRsp ++;

	if ( IFS_Strnicmp(pRsp, "ok", 2) == 0 )
	{
		rspCode = FS_SNS_RSP_SUCCESS;
		for ( i = 0; i < sns->config->account_num; i ++ )
		{
			if (IFS_Strstr(pRsp, sns->config->accout[i].name))
			{
				sns->config->accout[i].bound = FS_TRUE;
			}
		}
	}
	else
	{
		FS_COPY_TEXT(sns->rsp.err_info, pRsp);
	}
	sns->rsp.result = rspCode;
	FS_SnsRequestResult( sns );
}

static void FS_SnsUploadPhotoHandler( FS_SnsSession *sns, FS_BYTE *data, FS_SINT4 dlen )
{
	FS_CHAR *pRsp = (FS_CHAR *)data;
	FS_SnsRspCode rspCode = FS_SNS_RSP_ERR_UNKNOW;
	
	FS_SNS_TRACE2( "FS_SnsUploadPhotoHandler len=%d, data=%s", dlen, data );
	while ( *pRsp == '\r' || *pRsp == '\n' || *pRsp == '\t' || *pRsp == ' ' )
		pRsp ++;
	
	if ( IFS_Strnicmp(pRsp, "ok", 2) == 0 )
	{
		rspCode = FS_SNS_RSP_SUCCESS;
	}
	else
	{
		FS_COPY_TEXT(sns->rsp.err_info, pRsp);
	}
	sns->rsp.result = rspCode;
	FS_SnsRequestResult( sns );
}


static void FS_SnsDefaultHandler( FS_SnsSession *sns, FS_BYTE *data, FS_SINT4 dlen )
{
	FS_CHAR *pRsp = (FS_CHAR *)data;
	FS_SnsRspCode rspCode = FS_SNS_RSP_ERR_UNKNOW;
	
	FS_SNS_TRACE2( "FS_SnsDefaultHandler len=%d, data=%s", dlen, data );

	while ( *pRsp == '\r' || *pRsp == '\n' || *pRsp == '\t' || *pRsp == ' ' )
		pRsp ++;

	if ( IFS_Strnicmp(pRsp, "ok", 2) == 0 )
	{
		rspCode = FS_SNS_RSP_SUCCESS;
	}
	else
	{
		FS_COPY_TEXT(sns->rsp.err_info, pRsp);
	}
	sns->rsp.result = rspCode;
	FS_SnsRequestResult( sns );
}

static void FS_SnsWriteNetData( FS_SnsSession *sns, FS_BYTE *data, FS_SINT4 dlen )
{
	FS_SINT4 wlen;
	FS_SNS_TRACE3( "FS_SnsWriteData file=%s, len=%d, data=%s", sns->file, dlen, data );
	wlen = FS_FileWrite( FS_DIR_TMP, sns->file, sns->offset, data, dlen );
	FS_ASSERT( wlen == dlen );
	sns->offset += dlen;
}

static void FS_SnsHttpSendProgress( FS_SnsSession *sns, FS_HttpHandle hHttp, FS_SINT4 offset )
{
	if( offset == 0 )
	{
		if ( sns->status.status == FS_SNS_REQ_STS_SEND_REQUEST ) {
			sns->status.status = FS_SNS_REQ_STS_WAIT_RESPONSE;
		} else if ( sns->status.status == FS_SNS_REQ_STS_DOWNLOAD_IMAGE ){
			sns->status.image_status = FS_SNS_REQ_STS_WAIT_RESPONSE;
		}
	}
	else
	{
		FS_SNS_TRACE2( "FS_SnsHttpSendProgress offset=%d, total=%d", sns->status.index, sns->status.total );
		sns->status.status = FS_SNS_REQ_STS_UPLOAD_PHOTO;
		sns->status.index = offset;
	}
}

static void FS_SnsHttpRspStartCB( FS_SnsSession *sns, FS_HttpHandle hHttp, FS_SINT4 status, FS_HttpHeader *headers )
{
	FS_CHAR *ext = FS_NULL;

	if( sns->req.req == FS_SNS_REQ_NONE || sns->http != hHttp ) {
		FS_SNS_TRACE0( "FS_SnsHttpRspStartCB ERROR state" );
		return;
	}
	if( status != FS_HTTP_OK ) {
		FS_SNS_TRACE1( "FS_SnsHttpRspStartCB ERROR status = %d", status );
		FS_SnsRequestResult( sns );
		return;
	}
	sns->content_len = headers->content_length;
	sns->offset = 0;
	FS_SNS_TRACE3( "FS_SnsHttpRspStartCB content_type = %s, content_len = %d, cookies=%s",
		headers->content_type, headers->content_length, headers->cookies ? headers->cookies : "" );
	
	if ( sns->req.req == FS_SNS_REQ_LOGIN ) {
		/* save cookie PHPSESSION for further use */
		if ( headers->cookies ) {
			FS_CHAR *tmp;
			FS_SAFE_FREE( sns->sessid );
			sns->sessid = FS_StrConCat( "Cookie: ", headers->cookies, "\r\n", FS_NULL );
			tmp = IFS_Strchr( sns->sessid, ';' );
			if (tmp) {
				IFS_Strcpy( tmp, "\r\n" );
			}
		}
	} else if ( sns->req.req == FS_SNS_REQ_GET_TWEET ) {
		if ( sns->rsp.data.get_tweet.msg.id == FS_NULL ) { /* get_gweet xml */
			IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "gettweet_%s.xml", sns->req.data.get_tweet.type );
			FS_FileDelete( FS_DIR_TMP, sns->file );
		} else {
			FS_GetGuid( sns->file );
			ext = FS_GetExtFromMime( headers->content_type );
			if (ext) {
				IFS_Strcat( sns->file, "." );
				IFS_Strcat( sns->file, ext );
			}
		}
	} else if ( sns->req.req == FS_SNS_REQ_GET_UPDATE ) {
		if ( sns->rsp.data.msglist.count == 0 ) { 
			/* sns data.msglist xml */
			if ( sns->req.data.get_msgs.name == FS_NULL ) {
				IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "updates_%s_%d.xml", sns->req.data.get_msgs.type, sns->req.data.get_msgs.page );
			} else {
				IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "updates_%s_%s_%d.xml", 
					sns->req.data.get_msgs.type, sns->req.data.get_msgs.name, sns->req.data.get_msgs.page );
			}
			FS_FileDelete( FS_DIR_TMP, sns->file );
		} else {
			/* sns data.msglist image */
			FS_GetGuid( sns->file );
			ext = FS_GetExtFromMime( headers->content_type );
			if (ext) {
				/* sns server has a bug that will return text/html whatever the image format is */
				if ( FS_STR_I_EQUAL(ext, "htm") )
					ext = "jpg";
				IFS_Strcat( sns->file, "." );
				IFS_Strcat( sns->file, ext );
			}
		}
	} else if ( sns->req.req == FS_SNS_REQ_GET_REPLY ) {
		if ( sns->rsp.data.msglist.count == 0 ) { 
			/* sns data.msglist xml */
			if ( sns->req.data.get_msgs.name == FS_NULL ) {
				IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "reply_%s_%d.xml", sns->req.data.get_msgs.type, sns->req.data.get_msgs.page );
			} else {
				IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "reply_%s_%s_%d.xml", 
					sns->req.data.get_msgs.type, sns->req.data.get_msgs.name, sns->req.data.get_msgs.page );
			}
			FS_FileDelete( FS_DIR_TMP, sns->file );
		} else {
			/* sns data.msglist image */
			FS_GetGuid( sns->file );
			ext = FS_GetExtFromMime( headers->content_type );
			if (ext) {
				/* sns server has a bug that will return text/html whatever the image format is */
				if ( FS_STR_I_EQUAL(ext, "htm") )
					ext = "jpg";
				IFS_Strcat( sns->file, "." );
				IFS_Strcat( sns->file, ext );
			}
		}
	} else if ( sns->req.req == FS_SNS_REQ_GET_UPDATE_CONTENT ) {
		IFS_Strncpy( sns->file, "isync_content.xml", sizeof(sns->file) - 1 );
		FS_FileDelete( FS_DIR_TMP, sns->file );
	} else if ( sns->req.req == FS_SNS_REQ_GET_SUMMARY ) {
		IFS_Strncpy( sns->file, "isync_summary.xml", sizeof(sns->file) - 1 );
		FS_FileDelete( FS_DIR_TMP, sns->file );
	} else if ( sns->req.req == FS_SNS_REQ_GET_FRIENDS || sns->req.req == FS_SNS_REQ_FIND_FRIENDS ) {
		if ( sns->rsp.data.friends.count == 0 ) {
			if ( sns->req.req == FS_SNS_REQ_GET_FRIENDS ) {
				IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "%s_friends_%d.xml", 
					sns->req.data.friends.type, sns->req.data.friends.page );
			} else {
				IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "%s_find_friends_%d.xml", 
					sns->req.data.friends.type, sns->req.data.friends.page );
			}
			FS_FileDelete( FS_DIR_TMP, sns->file );
		} else {
			FS_GetGuid( sns->file );
			ext = FS_GetExtFromMime( headers->content_type );
			if (ext) {
				/* sns server has a bug that will return text/html whatever the image format is */
				if ( FS_STR_I_EQUAL(ext, "htm") )
					ext = "jpg";
				IFS_Strcat( sns->file, "." );
				IFS_Strcat( sns->file, ext );
			}
		}
	} else if ( sns->req.req == FS_SNS_REQ_RSS_GET_ALL_ARTICLE ) {
		IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "rss_articles_%d.xml", sns->req.data.rss.page );
		FS_FileDelete( FS_DIR_TMP, sns->file );
	} else if ( sns->req.req == FS_SNS_REQ_RSS_GET_ALL_ARTICLE_CONTENT ) {
		IFS_Strncpy( sns->file, "rss_articles_content.xml", sizeof(sns->file) - 1 );
		FS_FileDelete( FS_DIR_TMP, sns->file );
	} else if ( sns->req.req == FS_SNS_REQ_RSS_GET_ALL_CHANNEL ) {
		if ( sns->rsp.data.rss_channel.count == 0 ) {
			IFS_Strncpy( sns->file, "rss_channels.xml", sizeof(sns->file) - 1 );
			FS_FileDelete( FS_DIR_TMP, sns->file );
		} else {
			FS_GetGuid( sns->file );
			ext = FS_GetExtFromMime( headers->content_type );
			if (ext) {
				/* sns server has a bug that will return text/html whatever the image format is */
				if ( FS_STR_I_EQUAL(ext, "htm") )
					ext = "jpg";
				IFS_Strcat( sns->file, "." );
				IFS_Strcat( sns->file, ext );
			}
		}
	} else if ( sns->req.req == FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL ) {
		IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "channel_articles_%d.xml", sns->req.data.rss.page );
		FS_FileDelete( FS_DIR_TMP, sns->file );
	} else if ( sns->req.req == FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL_CONTENT ) {
		IFS_Strncpy( sns->file, "rss_articles_content.xml", sizeof(sns->file) - 1 );
		FS_FileDelete( FS_DIR_TMP, sns->file );
	} else if ( sns->req.req == FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY ) {
		if ( sns->rsp.data.rss_category.count == 0 ) {
			if ( sns->req.data.rss.id == FS_NULL ) {
				IFS_Strncpy( sns->file, "channel_category.xml", sizeof(sns->file) - 1 );
			} else {
				IFS_Strncpy( sns->file, "rss_second_category.xml", sizeof(sns->file) - 1 );
			}
			FS_FileDelete( FS_DIR_TMP, sns->file );
		} else {
			FS_GetGuid( sns->file );
			ext = FS_GetExtFromMime( headers->content_type );
			if (ext) {
				/* sns server has a bug that will return text/html whatever the image format is */
				if ( FS_STR_I_EQUAL(ext, "htm") )
					ext = "jpg";
				IFS_Strcat( sns->file, "." );
				IFS_Strcat( sns->file, ext );
			}
		}
	} else if ( sns->req.req == FS_SNS_REQ_RSS_GET_CATEGORY_DETAIL ) {
		if ( sns->rsp.data.rss_category_detail.count == 0 ) {
			IFS_Strncpy( sns->file, "category_detail.xml", sizeof(sns->file) - 1 );
			FS_FileDelete( FS_DIR_TMP, sns->file );
		} else {
			FS_GetGuid( sns->file );
			ext = FS_GetExtFromMime( headers->content_type );
			if (ext) {
				/* sns server has a bug that will return text/html whatever the image format is */
				if ( FS_STR_I_EQUAL(ext, "htm") )
					ext = "jpg";
				IFS_Strcat( sns->file, "." );
				IFS_Strcat( sns->file, ext );
			}
		}
	} else if ( sns->req.req == FS_SNS_REQ_RSS_SET_MY_CHANNEL ) {
		IFS_Strncpy( sns->file, "set_my_channel.xml", sizeof(sns->file) - 1 );
		FS_FileDelete( FS_DIR_TMP, sns->file );
	} else if ( sns->req.req == FS_SNS_REQ_EML_RECV ) {
		IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "emails_%d.xml", sns->req.data.eml_recv.page );
		FS_FileDelete( FS_DIR_TMP, sns->file );
	} else if ( sns->req.req == FS_SNS_REQ_EML_RECV_CONTENT ) {
		IFS_Strncpy( sns->file, "email_content.xml", sizeof(sns->file) - 1 );
		FS_FileDelete( FS_DIR_TMP, sns->file );
	} else if ( sns->req.req == FS_SNS_REQ_EML_GET_ACOUNT ) {
		IFS_Strncpy( sns->file, "email_account.xml", sizeof(sns->file) - 1 );
		FS_FileDelete( FS_DIR_TMP, sns->file );
	} else if ( sns->req.req == FS_SNS_REQ_EML_GET_CONTACT ) {
		IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "eml_contact_%d.xml", sns->req.data.eml_contact.page );
		FS_FileDelete( FS_DIR_TMP, sns->file );
	} 
}

static void FS_SnsHttpRspDataCB( FS_SnsSession *sns, FS_HttpHandle hHttp, FS_BYTE *data, FS_SINT4 data_len )
{
	if( sns->req.req == FS_SNS_REQ_NONE || sns->http != hHttp ) {
		FS_SNS_TRACE0( "FS_SnsHttpRspDataCB ERROR state" );
		return;
	}
	FS_SNS_TRACE3( "FS_SnsHttpRspDataCB content_len = %d, offset = %d, rlen = %d", sns->content_len,  sns->offset, data_len );
	switch (sns->req.req) {
	case FS_SNS_REQ_LOGIN:
		FS_SnsLoginHandler( sns, data, data_len );
		break;
	case FS_SNS_REQ_REGISTER:
	case FS_SNS_REQ_SET_TWEET:
	case FS_SNS_REQ_RETWEET:
	case FS_SNS_REQ_SET_LIKE:
	case FS_SNS_REQ_REPLY:
	case FS_SNS_REQ_ADD_FRIEND:
	case FS_SNS_REQ_DEL_FRIEND:
	case FS_SNS_REQ_EML_SET_ACOUNT:
	case FS_SNS_REQ_EML_SEND:
	case FS_SNS_REQ_EML_ADD_CONTACT:
	case FS_SNS_REQ_EML_DEL_CONTACT:
	case FS_SNS_REQ_EML_MOD_CONTACT:
		FS_SnsDefaultHandler( sns, data, data_len );
		break;
	case FS_SNS_REQ_GET_TWEET:
	case FS_SNS_REQ_GET_UPDATE:
	case FS_SNS_REQ_GET_REPLY:
	case FS_SNS_REQ_GET_SUMMARY:
	case FS_SNS_REQ_GET_FRIENDS:
	case FS_SNS_REQ_FIND_FRIENDS:
	case FS_SNS_REQ_RSS_GET_ALL_ARTICLE:
	case FS_SNS_REQ_RSS_GET_ALL_CHANNEL:
	case FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL:
	case FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY:
	case FS_SNS_REQ_RSS_GET_CATEGORY_DETAIL:
	case FS_SNS_REQ_RSS_SET_MY_CHANNEL:
	case FS_SNS_REQ_EML_RECV:
	case FS_SNS_REQ_EML_RECV_CONTENT:
	case FS_SNS_REQ_EML_GET_ACOUNT:
	case FS_SNS_REQ_EML_GET_CONTACT:
	case FS_SNS_REQ_RSS_GET_ALL_ARTICLE_CONTENT:
	case FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL_CONTENT:
	case FS_SNS_REQ_GET_UPDATE_CONTENT:
		FS_SnsWriteNetData( sns, data, data_len );
		break;
	case FS_SNS_REQ_UPLOAD_PHOTO:
		FS_SnsUploadPhotoHandler( sns, data, data_len );
		break;
	default:
		break;
	}
}

static void FS_SnsTimerCallback( FS_SnsSession *sns )
{
	FS_CHAR *url;
	FS_SINT4 idx = 0;

	FS_SNS_TRACE1( "FS_SnsTimerCallback req = %d", sns->req.req );
	if( sns->req.req == FS_SNS_REQ_NONE ) {
		FS_SNS_TRACE0("FS_SnsTimerCallback ERROR. Wrong state.");
		return;
	}
	sns->timer_id = 0;
	
	switch ( sns->req.req ) {
	case FS_SNS_REQ_GET_UPDATE:
	case FS_SNS_REQ_GET_REPLY:
		/* try to download image if did not hit cache */
		url = FS_SnsMsgsCheckCacheImage( sns, &idx );
		if ( url ) {
			/* did not hit cache, need to download new image */
			FS_SNS_TRACE0( "FS_SnsTimerCallback data.msglist try to download image" );

			sns->status.image_status = FS_SNS_REQ_STS_SEND_REQUEST;
			sns->status.index = idx;
			FS_SnsSendHttpRequest( sns, url, sns->sessid );
		} else {
			/* all images hit cache, finish data.msglist */
			sns->rsp.result = FS_SNS_RSP_SUCCESS;
			FS_SnsRequestResult( sns );
		}
		break;
	case FS_SNS_REQ_GET_FRIENDS:
	case FS_SNS_REQ_FIND_FRIENDS:
		/* try to download image if did not hit cache */
		url = FS_SnsFriendsCheckCacheImage( sns, &idx );
		if ( url ) {
			/* did not hit cache, need to download new image */
			FS_SNS_TRACE0( "FS_SnsTimerCallback data.friends try to download image" );
			
			sns->status.image_status = FS_SNS_REQ_STS_SEND_REQUEST;
			sns->status.index = idx;
			FS_SnsSendHttpRequest( sns, url, sns->sessid );
		} else {
			/* all images hit cache, finish */
			sns->rsp.result = FS_SNS_RSP_SUCCESS;
			FS_SnsRequestResult( sns );
		}
		break;
	case FS_SNS_REQ_RSS_GET_ALL_CHANNEL:
		/* try to download image if did not hit cache */
		url = FS_SnsRssChannelCheckCacheImage( sns, &idx );
		if ( url ) {
			/* did not hit cache, need to download new image */
			FS_SNS_TRACE0( "FS_SnsTimerCallback data.rss_channel try to download image" );
			
			sns->status.image_status = FS_SNS_REQ_STS_SEND_REQUEST;
			sns->status.index = idx;
			FS_SnsSendHttpRequest( sns, url, sns->sessid );
		} else {
			/* all images hit cache, finish */
			sns->rsp.result = FS_SNS_RSP_SUCCESS;
			FS_SnsRequestResult( sns );
		}
		break;
	case FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY:
		/* try to download image if did not hit cache */
		url = FS_SnsRssCategoryCheckCacheImage( sns, &idx );
		if ( url ) {
			/* did not hit cache, need to download new image */
			FS_SNS_TRACE0( "FS_SnsTimerCallback data.rss_category try to download image" );
			
			sns->status.image_status = FS_SNS_REQ_STS_SEND_REQUEST;
			sns->status.index = idx;
			FS_SnsSendHttpRequest( sns, url, sns->sessid );
		} else {
			/* all images hit cache, finish */
			sns->rsp.result = FS_SNS_RSP_SUCCESS;
			FS_SnsRequestResult( sns );
		}
		break;
	case FS_SNS_REQ_RSS_GET_CATEGORY_DETAIL:
		/* try to download image if did not hit cache */
		url = FS_SnsRssCategoryDetailCheckCacheImage( sns, &idx );
		if ( url ) {
			/* did not hit cache, need to download new image */
			FS_SNS_TRACE0( "FS_SnsTimerCallback data.rss_category_detail try to download image" );
			
			sns->status.image_status = FS_SNS_REQ_STS_SEND_REQUEST;
			sns->status.index = idx;
			FS_SnsSendHttpRequest( sns, url, sns->sessid );
		} else {
			/* all images hit cache, finish */
			sns->rsp.result = FS_SNS_RSP_SUCCESS;
			FS_SnsRequestResult( sns );
		}
		break;
	default:
		FS_SNS_TRACE0( "FS_SnsTimerCallback ERROR. unknow request in timer callback" );
		break;
	}
}

static void FS_SnsHttpRspEndCB( FS_SnsSession *sns, FS_HttpHandle hHttp, FS_SINT4 error_code )
{
	FS_CHAR *file;
	FS_CHAR *url;

	if( sns->req.req == FS_SNS_REQ_NONE || sns->http != hHttp ) {
		return;
	}
	FS_SNS_TRACE2( "FS_SnsHttpRspEndCB code=%d, req=%d", error_code, sns->req.req );

	if( error_code != FS_HTTP_ERR_OK ) {
		FS_SnsRequestResult( sns );
		return;
	}

	if ( sns->req.req == FS_SNS_REQ_GET_TWEET ) {
		if ( sns->rsp.data.get_tweet.msg.id == FS_NULL ) { 
			/* data.get_tweet.msg xml ready */
			FS_SnsXmlParseFile( sns );
			if ( sns->rsp.data.get_tweet.msg.icon_url ) {
				/* try to find a cache image */
				file = FS_SnsCacheGetImage( sns->rsp.data.get_tweet.msg.icon_url );
				if ( file ) {
					FS_SNS_TRACE0( "FS_SnsHttpRspEndCB data.get_tweet.msg hit cache image" );
					FS_COPY_TEXT( sns->rsp.data.get_tweet.msg.icon_file, file );
					sns->rsp.result = FS_SNS_RSP_SUCCESS;
					FS_SnsRequestResult( sns );
				} else {
					/* did not hit cache, need to download new image */
					if ( sns->config->display_image ) {
						FS_SNS_TRACE0( "FS_SnsHttpRspEndCB data.get_tweet.msg try to download image" );
						FS_SnsSendHttpRequest( sns, sns->rsp.data.get_tweet.msg.icon_url, sns->sessid );
					} else {
						sns->rsp.result = FS_SNS_RSP_SUCCESS;
						FS_SnsRequestResult( sns );
					}
				}
			} else {
				/* data.get_tweet.msg is empty */
				FS_SNS_TRACE0( "FS_SnsHttpRspEndCB data.get_tweet.msg empty" );
				FS_SAFE_FREE( sns->rsp.data.get_tweet.msg.icon_file );
				sns->rsp.result = FS_SNS_RSP_SUCCESS;
				FS_SnsRequestResult( sns );
			}
		} else { 
			/* data.get_tweet.msg image ready */
			FS_SNS_TRACE1( "FS_SnsHttpRspEndCB data.get_tweet.msg download image success. file=%s", sns->file );
			FS_COPY_TEXT( sns->rsp.data.get_tweet.msg.icon_file, sns->file );
			sns->rsp.result = FS_SNS_RSP_SUCCESS;
			FS_SnsCachePutImage( sns->rsp.data.get_tweet.msg.icon_url, sns->rsp.data.get_tweet.msg.icon_file );
			FS_SnsRequestResult( sns );
		}
		
		return;

	} else if ( sns->req.req == FS_SNS_REQ_GET_UPDATE || sns->req.req == FS_SNS_REQ_GET_REPLY ) {
		if ( sns->rsp.data.msglist.msgs[0].id == FS_NULL ) {	
			/* sns data.msglist xml ready */
			FS_SnsXmlParseFile( sns );
			FS_SNS_TRACE1( "FS_SnsHttpRspEndCB data.msglist xml. count=%d", sns->rsp.data.msglist.count );
			if ( sns->rsp.data.msglist.count == 0 ) {
				/* sns data.msglist empty */
				sns->rsp.result = FS_SNS_RSP_SUCCESS;
				FS_FileDelete( FS_DIR_TMP, sns->file );
				FS_SnsRequestResult( sns );
			} else {
				/* try to download image if did not hit cache */
				if ( sns->config->display_image ) {
					sns->status.status = FS_SNS_REQ_STS_DOWNLOAD_IMAGE;
					sns->status.total = sns->rsp.data.msglist.count;
					url = FS_SnsMsgsCheckCacheImage( sns, FS_NULL );
					if ( url ) {
						/* did not hit cache, need to download new image */
						FS_SNS_TRACE0( "FS_SnsHttpRspEndCB data.msglist try to download image" );
						sns->timer_id = IFS_StartTimer( FS_TIMER_ID_SNSLIB, FS_SNS_DOWNLOAD_IMAGE_TIME, FS_SnsTimerCallback, sns );
					} else {
						/* all images hit cache, finish data.msglist */
						sns->rsp.result = FS_SNS_RSP_SUCCESS;
						FS_SnsRequestResult( sns );
					}
				} else {
					/* config did not display image */
					sns->rsp.result = FS_SNS_RSP_SUCCESS;
					FS_SnsRequestResult( sns );
				}
			}
		} else {
			/* sns updateds image ready */
			FS_SNS_TRACE0( "FS_SnsHttpRspEndCB data.msglist download one image success" );
			url = FS_SnsMsgsSetImageFile( sns, sns->file );
			if ( url ) {
				/* still got some image did not hit cache, need to download again */
				FS_SNS_TRACE0( "FS_SnsHttpRspEndCB data.msglist try to download another image later" );
				/* we need to start timer to download later */
				sns->timer_id = IFS_StartTimer( FS_TIMER_ID_SNSLIB, FS_SNS_DOWNLOAD_IMAGE_TIME, FS_SnsTimerCallback, sns );
			} else {
				/* all images hit cache or download complete, finish data.msglist */
				sns->rsp.result = FS_SNS_RSP_SUCCESS;
				FS_SnsRequestResult( sns );
			}
		}

		return;

	} else if ( sns->req.req == FS_SNS_REQ_GET_UPDATE_CONTENT ) {
		/* isync content xml ready */
		FS_SnsXmlParseFile( sns );
		FS_SNS_TRACE1( "FS_SnsHttpRspEndCB data.msglist xml. count=%d", sns->rsp.data.msglist.count );
		if ( sns->rsp.data.msglist.count == 0 ) {
			FS_FileDelete( FS_DIR_TMP, sns->file );
		}
		sns->rsp.result = FS_SNS_RSP_SUCCESS;
		FS_SnsRequestResult( sns );
	} else if ( sns->req.req == FS_SNS_REQ_GET_SUMMARY ) {
		/* isync summary xml ready */
		FS_SnsXmlParseFile( sns );
		sns->rsp.result = FS_SNS_RSP_SUCCESS;
		FS_SnsRequestResult( sns );
	} else if ( sns->req.req == FS_SNS_REQ_GET_FRIENDS || sns->req.req == FS_SNS_REQ_FIND_FRIENDS ) {
		/* isync get/find friends xml ready */
		if ( sns->rsp.data.friends.count == 0 ) {	
			/* sns data.friends xml ready */
			FS_SnsXmlParseFile( sns );
			FS_SNS_TRACE1( "FS_SnsHttpRspEndCB data.friends xml. count=%d", sns->rsp.data.friends.count );
			if ( sns->rsp.data.friends.count == 0 ) {
				/* sns data.friends empty */
				sns->rsp.result = FS_SNS_RSP_SUCCESS;
				FS_FileDelete( FS_DIR_TMP, sns->file );
				FS_SnsRequestResult( sns );
			} else {
				/* try to download image if did not hit cache */
				if ( sns->config->display_image ) {
					sns->status.status = FS_SNS_REQ_STS_DOWNLOAD_IMAGE;
					sns->status.total = sns->rsp.data.friends.count;
					url = FS_SnsFriendsCheckCacheImage( sns, FS_NULL );
					if ( url ) {
						/* did not hit cache, need to download new image */
						FS_SNS_TRACE0( "FS_SnsHttpRspEndCB data.friends try to download image" );
						sns->timer_id = IFS_StartTimer( FS_TIMER_ID_SNSLIB, FS_SNS_DOWNLOAD_IMAGE_TIME, FS_SnsTimerCallback, sns );
					} else {
						/* all images hit cache, finish data.friends */
						sns->rsp.result = FS_SNS_RSP_SUCCESS;
						FS_SnsRequestResult( sns );
					}
				} else {
					/* config did not display image */
					sns->rsp.result = FS_SNS_RSP_SUCCESS;
					FS_SnsRequestResult( sns );
				}
			}
		} else {
			/* sns friends image ready */
			FS_SNS_TRACE0( "FS_SnsHttpRspEndCB data.friends download one image success" );
			url = FS_SnsFriendsSetImageFile( sns, sns->file );
			if ( url ) {
				/* still got some image did not hit cache, need to download again */
				FS_SNS_TRACE0( "FS_SnsHttpRspEndCB data.friends try to download another image later" );
				/* we need to start timer to download later */
				sns->timer_id = IFS_StartTimer( FS_TIMER_ID_SNSLIB, FS_SNS_DOWNLOAD_IMAGE_TIME, FS_SnsTimerCallback, sns );
			} else {
				/* all images hit cache or download complete, finish data.friends */
				sns->rsp.result = FS_SNS_RSP_SUCCESS;
				FS_SnsRequestResult( sns );
			}
		}
	} else if ( sns->req.req == FS_SNS_REQ_RSS_GET_ALL_ARTICLE 
		|| sns->req.req == FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL 
		|| sns->req.req == FS_SNS_REQ_RSS_GET_ALL_ARTICLE_CONTENT 
		|| sns->req.req == FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL_CONTENT ) {
		/* sns data.rss_article xml ready */
		FS_SnsXmlParseFile( sns );
		FS_SNS_TRACE1( "FS_SnsHttpRspEndCB data.rss_article xml. count=%d", sns->rsp.data.rss_article.count );
		if ( sns->rsp.data.rss_article.count == 0 ) {
			FS_FileDelete( FS_DIR_TMP, sns->file );
		}
		sns->rsp.result = FS_SNS_RSP_SUCCESS;
		FS_SnsRequestResult( sns );
	} else if ( sns->req.req == FS_SNS_REQ_RSS_GET_ALL_CHANNEL ) {
		/* rss all channel xml ready */
		if ( sns->rsp.data.rss_channel.count == 0 ) {	
			/* sns data.rss_channel xml ready */
			FS_SnsXmlParseFile( sns );
			FS_SNS_TRACE1( "FS_SnsHttpRspEndCB data.rss_channel xml. count=%d", sns->rsp.data.rss_channel.count );
			if ( sns->rsp.data.rss_channel.count == 0 ) {
				/* sns data.rss_channel empty */
				sns->rsp.result = FS_SNS_RSP_SUCCESS;
				FS_SnsRequestResult( sns );
			} else {
				/* try to download image if did not hit cache */
				if ( sns->config->display_image ) {
					sns->status.status = FS_SNS_REQ_STS_DOWNLOAD_IMAGE;
					sns->status.total = sns->rsp.data.rss_channel.count;
					url = FS_SnsRssChannelCheckCacheImage( sns, FS_NULL );
					if ( url ) {
						/* did not hit cache, need to download new image */
						FS_SNS_TRACE0( "FS_SnsHttpRspEndCB data.rss_channel try to download image" );
						sns->timer_id = IFS_StartTimer( FS_TIMER_ID_SNSLIB, FS_SNS_DOWNLOAD_IMAGE_TIME, FS_SnsTimerCallback, sns );
					} else {
						/* all images hit cache, finish data.friends */
						sns->rsp.result = FS_SNS_RSP_SUCCESS;
						FS_SnsRequestResult( sns );
					}
				} else {
					/* config did not display image */
					sns->rsp.result = FS_SNS_RSP_SUCCESS;
					FS_SnsRequestResult( sns );
				}
			}
		} else {
			/* sns channel image ready */
			FS_SNS_TRACE0( "FS_SnsHttpRspEndCB data.rss_channel download one image success" );
			url = FS_SnsRssChannelSetImageFile( sns, sns->file );
			if ( url ) {
				/* still got some image did not hit cache, need to download again */
				FS_SNS_TRACE0( "FS_SnsHttpRspEndCB data.rss_channel try to download another image later" );
				/* we need to start timer to download later */
				sns->timer_id = IFS_StartTimer( FS_TIMER_ID_SNSLIB, FS_SNS_DOWNLOAD_IMAGE_TIME, FS_SnsTimerCallback, sns );
			} else {
				/* all images hit cache or download complete, finish data.friends */
				sns->rsp.result = FS_SNS_RSP_SUCCESS;
				FS_SnsRequestResult( sns );
			}
		}
	} else if ( sns->req.req == FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY ) {
		/* rss channel category xml ready */
		if ( sns->rsp.data.rss_category.count == 0 ) {	
			/* sns data.rss_category xml ready */
			FS_SnsXmlParseFile( sns );
			FS_SNS_TRACE1( "FS_SnsHttpRspEndCB data.rss_category xml. count=%d", sns->rsp.data.rss_category.count );
			if ( sns->rsp.data.rss_category.count == 0 ) {
				/* sns data.rss_category empty */
				sns->rsp.result = FS_SNS_RSP_SUCCESS;
				FS_SnsRequestResult( sns );
			} else {
				/* try to download image if did not hit cache */
				if ( sns->config->display_image ) {
					sns->status.status = FS_SNS_REQ_STS_DOWNLOAD_IMAGE;
					sns->status.total = sns->rsp.data.rss_category.count;
					url = FS_SnsRssCategoryCheckCacheImage( sns, FS_NULL );
					if ( url ) {
						/* did not hit cache, need to download new image */
						FS_SNS_TRACE0( "FS_SnsHttpRspEndCB data.rss_category try to download image" );
						sns->timer_id = IFS_StartTimer( FS_TIMER_ID_SNSLIB, FS_SNS_DOWNLOAD_IMAGE_TIME, FS_SnsTimerCallback, sns );
					} else {
						/* all images hit cache, finish data.friends */
						sns->rsp.result = FS_SNS_RSP_SUCCESS;
						FS_SnsRequestResult( sns );
					}
				} else {
					/* config did not display image */
					sns->rsp.result = FS_SNS_RSP_SUCCESS;
					FS_SnsRequestResult( sns );
				}
			}
		} else {
			/* sns channel image ready */
			FS_SNS_TRACE0( "FS_SnsHttpRspEndCB data.rss_category download one image success" );
			url = FS_SnsRssCategorySetImageFile( sns, sns->file );
			if ( url ) {
				/* still got some image did not hit cache, need to download again */
				FS_SNS_TRACE0( "FS_SnsHttpRspEndCB data.rss_category try to download another image later" );
				/* we need to start timer to download later */
				sns->timer_id = IFS_StartTimer( FS_TIMER_ID_SNSLIB, FS_SNS_DOWNLOAD_IMAGE_TIME, FS_SnsTimerCallback, sns );
			} else {
				/* all images hit cache or download complete, finish data.friends */
				sns->rsp.result = FS_SNS_RSP_SUCCESS;
				FS_SnsRequestResult( sns );
			}
		}
	} else if( sns->req.req == FS_SNS_REQ_RSS_SET_MY_CHANNEL ) {
		/* sns data.rss_article xml ready */
		FS_SnsXmlParseFile( sns );
		/* result is contain in response xml, parse the xml to get result */
		FS_SNS_TRACE1( "FS_SnsHttpRspEndCB data.set_my_channel xml. result=%d", sns->rsp.result );
		FS_SnsRequestResult( sns );
	} else if ( sns->req.req == FS_SNS_REQ_RSS_GET_CATEGORY_DETAIL ) {
		/* rss category detail xml ready */
#ifdef FEATURE_DISABLE_RSS_CATEGORY_DETAIL_IMAGE
		/* sns data.rss_category_detail xml ready */
		FS_SnsXmlParseFile( sns );
		FS_SNS_TRACE1( "FS_SnsHttpRspEndCB data.rss_category_detail xml. count=%d", sns->rsp.data.rss_category_detail.count );
		sns->rsp.result = FS_SNS_RSP_SUCCESS;
		FS_SnsRequestResult( sns );
#else
		if ( sns->rsp.data.rss_category_detail.count == 0 ) {	
			/* sns data.rss_category_detail xml ready */
			FS_SnsXmlParseFile( sns );
			FS_SNS_TRACE1( "FS_SnsHttpRspEndCB data.rss_category_detail xml. count=%d", sns->rsp.data.rss_category_detail.count );

			if ( sns->rsp.data.rss_category_detail.count == 0 ) {
				/* sns data.rss_category_detail empty */
				sns->rsp.result = FS_SNS_RSP_SUCCESS;
				FS_SnsRequestResult( sns );
			} else {
				/* try to download image if did not hit cache */
				if ( sns->config->display_image ) {
					sns->status.status = FS_SNS_REQ_STS_DOWNLOAD_IMAGE;
					sns->status.total = sns->rsp.data.rss_category_detail.count;
					url = FS_SnsRssCategoryDetailCheckCacheImage( sns, FS_NULL );
					if ( url ) {
						/* did not hit cache, need to download new image */
						FS_SNS_TRACE0( "FS_SnsHttpRspEndCB data.rss_category_detail try to download image" );
						sns->timer_id = IFS_StartTimer( FS_TIMER_ID_SNSLIB, FS_SNS_DOWNLOAD_IMAGE_TIME, FS_SnsTimerCallback, sns );
					} else {
						/* all images hit cache, finish data.friends */
						sns->rsp.result = FS_SNS_RSP_SUCCESS;
						FS_SnsRequestResult( sns );
					}
				} else {
					/* config did not display image */
					sns->rsp.result = FS_SNS_RSP_SUCCESS;
					FS_SnsRequestResult( sns );
				}
			}
		} else {
			/* sns channel image ready */
			FS_SNS_TRACE0( "FS_SnsHttpRspEndCB data.rss_category_detail download one image success" );
			url = FS_SnsRssCategoryDetailSetImageFile( sns, sns->file );
			if ( url ) {
				/* still got some image did not hit cache, need to download again */
				FS_SNS_TRACE0( "FS_SnsHttpRspEndCB data.rss_category_detail try to download another image later" );
				/* we need to start timer to download later */
				sns->timer_id = IFS_StartTimer( FS_TIMER_ID_SNSLIB, FS_SNS_DOWNLOAD_IMAGE_TIME, FS_SnsTimerCallback, sns );
			} else {
				/* all images hit cache or download complete, finish data.friends */
				sns->rsp.result = FS_SNS_RSP_SUCCESS;
				FS_SnsRequestResult( sns );
			}
		}
#endif	/* FEATURE_DISABLE_RSS_CATEGORY_DETAIL_IMAGE */
	} else if ( sns->req.req == FS_SNS_REQ_EML_RECV || sns->req.req == FS_SNS_REQ_EML_RECV_CONTENT ) {
		/* email list xml ready */
		FS_SnsXmlParseFile( sns );
		FS_SNS_TRACE1( "FS_SnsHttpRspEndCB data.eml_list count=%d", sns->rsp.data.eml_list.count );
		if ( sns->rsp.data.eml_list.count == 0 ) {
			FS_FileDelete( FS_DIR_TMP, sns->file );
		}
		sns->rsp.result = FS_SNS_RSP_SUCCESS;
		FS_SnsRequestResult( sns );
	} else if ( sns->req.req == FS_SNS_REQ_EML_GET_ACOUNT ) {
		/* email account xml ready */
		FS_SnsXmlParseFile( sns );
		sns->rsp.result = FS_SNS_RSP_SUCCESS;
		FS_SnsRequestResult( sns );
	} else if ( sns->req.req == FS_SNS_REQ_EML_GET_CONTACT ) {
		/* email contact xml ready */
		FS_SnsXmlParseFile( sns );
		sns->rsp.result = FS_SNS_RSP_SUCCESS;
		FS_SnsRequestResult( sns );
	} else {
		FS_SnsRequestResult( sns );
	}
}

static FS_CHAR *FS_SnsFormatHttpRequest( FS_SnsSession *sns )
{
	FS_CHAR *szReq = IFS_Malloc( FS_SNS_MAX_REQ_LEN );
	FS_SINT4 len, msglen;
	FS_CHAR szText[256] = {0};

	FS_SNS_TRACE1( "FS_SnsFormatHttpRequest req=%d", sns->req.req );
	if ( szReq ) {
		switch (sns->req.req) {
		case FS_SNS_REQ_LOGIN:
			IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_LOGIN_URL, sns->req.data.login.user,
				sns->req.data.login.pass );
			break;
		case FS_SNS_REQ_REGISTER:
			IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_REGISTER_URL, sns->req.data.reg.user,
				sns->req.data.reg.pass, sns->req.data.reg.email );
			break;
		case FS_SNS_REQ_GET_SUMMARY:
			IFS_Strncpy( szReq, FS_SNS_GET_SUMMARY_URL, FS_SNS_MAX_REQ_LEN - 1 );
			break;
		case FS_SNS_REQ_GET_TWEET:
			IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_GET_TWEET_URL, sns->req.data.get_tweet.type );
			break;
		case FS_SNS_REQ_GET_UPDATE:
			if ( sns->req.data.get_msgs.count == 0 ) {
				sns->req.data.get_msgs.count = FS_SNS_DEF_MSGS_COUNT;
			}
			if ( sns->req.data.get_msgs.page <= 0 ) {
				sns->req.data.get_msgs.page = 1;
			}
			if ( IFS_Stricmp(sns->req.data.get_msgs.type, "isync") == 0 ) {
				if ( sns->req.data.get_msgs.name == FS_NULL ) {
					IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_GET_UPDATE_URL, 
						sns->req.data.get_msgs.type, 
						"get_all_updates", 
						sns->req.data.get_msgs.page,
						sns->req.data.get_msgs.count);
					if ( IFS_Strlen(szReq) < FS_SNS_MAX_REQ_LEN - 1 - 12 ) {
						if ( sns->req.data.get_msgs.page == 1 ) {
							IFS_Strcat( szReq, "&refresh=yes" );
						} else {
							IFS_Strcat( szReq, "&refresh=no" );
						}
					}
				} else {
					IFS_Snprintf( szText, sizeof(szText) - 1, "get_%s_updates", sns->req.data.get_msgs.name );
					IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_GET_UPDATE_URL, 
						sns->req.data.get_msgs.type, 
						szText, 
						sns->req.data.get_msgs.page,
						sns->req.data.get_msgs.count);
				}
			} else {
				IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_GET_UPDATE_URL, 
					sns->req.data.get_msgs.type, 
					"updates", 
					sns->req.data.get_msgs.page,
					sns->req.data.get_msgs.count);
			}
			break;
		case FS_SNS_REQ_GET_REPLY:
			if ( sns->req.data.get_msgs.count == 0 ) {
				sns->req.data.get_msgs.count = FS_SNS_DEF_MSGS_COUNT;
			}
			if ( sns->req.data.get_msgs.page <= 0 ) {
				sns->req.data.get_msgs.page = 1;
			}
			if ( IFS_Stricmp(sns->req.data.get_msgs.type, "isync") == 0 ) {
				if ( sns->req.data.get_msgs.name == FS_NULL ) {
					IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_GET_REPLY_URL, 
						sns->req.data.get_msgs.type, 
						"get_all_reply",
						sns->req.data.get_msgs.page, 
						sns->req.data.get_msgs.count);
					if ( IFS_Strlen(szReq) < FS_SNS_MAX_REQ_LEN - 1 - 12 ) {
						if ( sns->req.data.get_msgs.page == 1 ) {
							IFS_Strcat( szReq, "&refresh=yes" );
						} else {
							IFS_Strcat( szReq, "&refresh=no" );
						}
					}
				} else {
					IFS_Snprintf( szText, sizeof(szText) - 1, "get_%s_reply", sns->req.data.get_msgs.name );
					IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_GET_REPLY_URL, 
						sns->req.data.get_msgs.type, 
						szText,
						sns->req.data.get_msgs.page, 
						sns->req.data.get_msgs.count);
				}
			} else {
				IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_GET_REPLY_URL, 
					sns->req.data.get_msgs.type, 
					"getreply",
					sns->req.data.get_msgs.page, 
					sns->req.data.get_msgs.count);
			}
			break;
		case FS_SNS_REQ_GET_UPDATE_CONTENT:
			IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_GET_UPDATE_CONTENT_URL, 
				sns->req.data.get_msgs.id, sns->req.data.get_msgs.type );
			break;
		case FS_SNS_REQ_SET_TWEET:
			IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_SET_TWEET_URL, sns->req.data.set_tweet.type );
			len = IFS_Strlen( szReq );
			msglen = IFS_Strlen(sns->req.data.set_tweet.msg);
			FS_UrlEncode( szReq + len, FS_SNS_MAX_REQ_LEN - len, sns->req.data.set_tweet.msg, msglen );

			if ( sns->req.data.set_tweet.sync_to ) {
				len = IFS_Strlen(szReq);
				if ( len + IFS_Strlen(sns->req.data.set_tweet.sync_to) + 6 < FS_SNS_MAX_REQ_LEN - 1 ) {
					IFS_Strcat( szReq + len, "&sync=" );
					IFS_Strcat( szReq + len + 6, sns->req.data.set_tweet.sync_to );
				} else {
					FS_SNS_TRACE1( "FS_SnsFormatHttpRequest ERROR. req len exceed to limit, ignore syncto. len=%d", len );
				}
			}
			break;
		case FS_SNS_REQ_RETWEET:
			FS_UrlEncode( szText, sizeof(szText), sns->req.data.reply.id, -1 );
			IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_RETWEET_URL, sns->req.data.reply.type, szText );
			break;
		case FS_SNS_REQ_SET_LIKE:
			FS_UrlEncode( szText, sizeof(szText), sns->req.data.reply.id, -1 );
			IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_SETLIKE_URL, sns->req.data.reply.type, szText );
			break;
		case FS_SNS_REQ_REPLY:
			FS_UrlEncode( szText, sizeof(szText), sns->req.data.reply.id, -1 );
			IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_REPLY_URL, sns->req.data.reply.type, szText );
			
			len = IFS_Strlen( szReq );
			msglen = IFS_Strlen(sns->req.data.reply.msg);
			if ( FS_SNS_MAX_REQ_LEN - 1 - len > 3 * msglen ) {
				/* make sure we got enough space and will not overflow the buffer */
				FS_UrlEncode( szReq + len, FS_SNS_MAX_REQ_LEN - len, sns->req.data.reply.msg, msglen );
			} else {
				FS_SNS_TRACE1( "FS_SnsFormatHttpRequest ERROR. msg may too long. req=%d", sns->req.req );
			}
			break;
		case FS_SNS_REQ_UPLOAD_PHOTO:
			IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_UPLOAD_PHOTO_URL, sns->req.data.photo.type );
			len = IFS_Strlen( szReq );
			if ( sns->req.data.photo.msg ) {
				msglen = IFS_Strlen(sns->req.data.photo.msg);
				FS_UrlEncode( szReq + len, FS_SNS_MAX_REQ_LEN - len, sns->req.data.photo.msg, msglen );
			}
			if ( sns->req.data.photo.sync_to ) {
				len = IFS_Strlen(szReq);
				if ( len + IFS_Strlen(sns->req.data.photo.sync_to) + 6 < FS_SNS_MAX_REQ_LEN - 1 ) {
					IFS_Strcat( szReq + len, "&sync=" );
					IFS_Strcat( szReq + len + 6, sns->req.data.photo.sync_to );
				} else {
					FS_SNS_TRACE1( "FS_SnsFormatHttpRequest ERROR. req len exceed to limit, ignore syncto. len=%d", len );
				}
			}
			break;
		case FS_SNS_REQ_GET_FRIENDS:
			if ( sns->req.data.friends.count == 0 ) {
				sns->req.data.friends.count = FS_SNS_DEF_MSGS_COUNT;
			}
			if ( sns->req.data.friends.page <= 0 ) {
				sns->req.data.friends.page = 1;
			}
			IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_GET_FRIENDS_URL, 
				sns->req.data.friends.type, sns->req.data.friends.page, sns->req.data.friends.count );
			break;
		case FS_SNS_REQ_FIND_FRIENDS:
			if ( sns->req.data.friends.count == 0 ) {
				sns->req.data.friends.count = FS_SNS_DEF_MSGS_COUNT;
			}
			if ( sns->req.data.friends.page <= 0 ) {
				sns->req.data.friends.page = 1;
			}
			FS_UrlEncode( szText, sizeof(szText), sns->req.data.friends.key, -1 );
			IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_FIND_FRIENDS_URL, 
				sns->req.data.friends.type, szText, sns->req.data.friends.page, sns->req.data.friends.count );
			break;
		case FS_SNS_REQ_ADD_FRIEND:
			FS_UrlEncode( szText, sizeof(szText), sns->req.data.friends.key, -1 );
			IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_ADD_FRIEND_URL, 
				sns->req.data.friends.type, szText );
			break;
		case FS_SNS_REQ_DEL_FRIEND:
			FS_UrlEncode( szText, sizeof(szText), sns->req.data.friends.key, -1 );
			IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_DEL_FRIEND_URL, 
				sns->req.data.friends.type, szText );
			break;
		case FS_SNS_REQ_RSS_GET_ALL_ARTICLE:
			if ( sns->req.data.rss.count == 0 ) {
				sns->req.data.rss.count = FS_SNS_DEF_MSGS_COUNT;
			}
			if ( sns->req.data.rss.page <= 0 ) {
				sns->req.data.rss.page = 1;
			}
			IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_RSS_ALL_ARTICLE_URL, 
				sns->req.data.rss.page, sns->req.data.rss.count );
			if ( IFS_Strlen(szReq) < FS_SNS_MAX_REQ_LEN - 1 - 12 ) {
				if ( sns->req.data.rss.page == 1 ) {
					IFS_Strcat( szReq, "&refresh=yes");
				}
			}
			break;
		case FS_SNS_REQ_RSS_GET_ALL_ARTICLE_CONTENT:
			IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_RSS_ALL_ARTICLE_CONTENT_URL, sns->req.data.rss.id );
			break;
		case FS_SNS_REQ_RSS_GET_ALL_CHANNEL:
			IFS_Strncpy( szReq, FS_SNS_RSS_ALL_CHANNEL_URL, FS_SNS_MAX_REQ_LEN - 1 );
			break;
		case FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL:
			IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_RSS_CHANNEL_DETAIL_URL, 
				sns->req.data.rss.id, sns->req.data.rss.page, sns->req.data.rss.count );
			if ( IFS_Strlen(szReq) < FS_SNS_MAX_REQ_LEN - 1 - 12 ) {
				if ( sns->req.data.rss.page == 1 ) {
					IFS_Strcat( szReq, "&refresh=yes");
				}
			}
			break;
		case FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL_CONTENT:
			IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_RSS_CHANNEL_DETAIL_CONTENT_URL, sns->req.data.rss.id );
			break;
		case FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY:
			if ( sns->req.data.rss.id == FS_NULL ) {
				IFS_Strncpy( szReq, FS_SNS_RSS_FIRST_CATEGORY_URL, FS_SNS_MAX_REQ_LEN - 1 );
			} else {
				IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_RSS_SECOND_CATEGORY_URL, sns->req.data.rss.id );
			}
			break;
		case FS_SNS_REQ_RSS_GET_CATEGORY_DETAIL:
			IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_RSS_CATEGORY_DETAIL_URL, sns->req.data.rss.id );
			break;
		case FS_SNS_REQ_RSS_SET_MY_CHANNEL:
			IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_RSS_SET_MY_CHANNEL_URL, sns->req.data.rss.id );
			break;
		case FS_SNS_REQ_EML_SET_ACOUNT:
			IFS_Strncpy( szReq, FS_SNS_EML_SET_ACCOUNT_URL, FS_SNS_MAX_REQ_LEN - 1 );
			len = IFS_Strlen( szReq );
			/* email */
			if ( FS_SNS_MAX_REQ_LEN - len > 3 * IFS_Strlen(sns->req.data.eml_account.email) + 7 ) {
				IFS_Strcat( szReq + len, "&email=" );
				len += 7;
				len += FS_UrlEncode( szReq + len, FS_SNS_MAX_REQ_LEN - len, sns->req.data.eml_account.email, -1 );
			} else {
				FS_SNS_TRACE1( "FS_SnsFormatHttpRequest ERROR. req len exceed to limit. req=%d", sns->req.req );
			}
			/* smtp */
			if ( FS_SNS_MAX_REQ_LEN - len > 3 * IFS_Strlen(sns->req.data.eml_account.smtp) + 6 ) {
				IFS_Strcat( szReq + len, "&smtp=" );
				len += 6;
				len += FS_UrlEncode( szReq + len, FS_SNS_MAX_REQ_LEN - len, sns->req.data.eml_account.smtp, -1 );
			} else {
				FS_SNS_TRACE1( "FS_SnsFormatHttpRequest ERROR. req len exceed to limit. req=%d", sns->req.req );
			}
			/* pop3 */
			if ( FS_SNS_MAX_REQ_LEN - len > 3 * IFS_Strlen(sns->req.data.eml_account.pop3) + 6 ) {
				IFS_Strcat( szReq + len, "&pop3=" );
				len += 6;
				len += FS_UrlEncode( szReq + len, FS_SNS_MAX_REQ_LEN - len, sns->req.data.eml_account.pop3, -1 );
			} else {
				FS_SNS_TRACE1( "FS_SnsFormatHttpRequest ERROR. req len exceed to limit. req=%d", sns->req.req );
			}
			/* password */
			if ( FS_SNS_MAX_REQ_LEN - len > 3 * IFS_Strlen(sns->req.data.eml_account.password) + 10 ) {
				IFS_Strcat( szReq + len, "&password=" );
				len += 10;
				len += FS_UrlEncode( szReq + len, FS_SNS_MAX_REQ_LEN - len, sns->req.data.eml_account.password, -1 );
			} else {
				FS_SNS_TRACE1( "FS_SnsFormatHttpRequest ERROR. req len exceed to limit. req=%d", sns->req.req );
			}
			/* bind home */
			if ( FS_SNS_MAX_REQ_LEN - len > 3 * IFS_Strlen(sns->req.data.eml_account.password) + 13 ) {
				if ( sns->req.data.eml_account.bind_home ) {
					IFS_Strcat( szReq + len, "&bindhome=yes" );
					len += 13;
				} else {
					IFS_Strcat( szReq + len, "&bindhome=no" );
					len += 12;
				}
			} else {
				FS_SNS_TRACE1( "FS_SnsFormatHttpRequest ERROR. req len exceed to limit. req=%d", sns->req.req );
			}
			break;
		case FS_SNS_REQ_EML_SEND:
			IFS_Strncpy( szReq, FS_SNS_EML_SEND_URL, FS_SNS_MAX_REQ_LEN - 1 );
			len = IFS_Strlen( szReq );
			/* receiver */
			if ( FS_SNS_MAX_REQ_LEN - len > 3 * IFS_Strlen(sns->req.data.eml_send.receiver) + 10 ) {
				IFS_Strcat( szReq + len, "&receiver=" );
				len += 10;
				len += FS_UrlEncode( szReq + len, FS_SNS_MAX_REQ_LEN - len, sns->req.data.eml_send.receiver, -1 );
			} else {
				FS_SNS_TRACE1( "FS_SnsFormatHttpRequest ERROR. req len exceed to limit. req=%d", sns->req.req );
			}
			/* subject */
			if ( FS_SNS_MAX_REQ_LEN - len > 3 * IFS_Strlen(sns->req.data.eml_send.subject) + 9 ) {
				IFS_Strcat( szReq + len, "&subject=" );
				len += 9;
				len += FS_UrlEncode( szReq + len, FS_SNS_MAX_REQ_LEN - len, sns->req.data.eml_send.subject, -1 );
			} else {
				FS_SNS_TRACE1( "FS_SnsFormatHttpRequest ERROR. req len exceed to limit. req=%d", sns->req.req );
			}
			/* content */
			if ( FS_SNS_MAX_REQ_LEN - len <= 3 * IFS_Strlen(sns->req.data.eml_send.content) + 9 ) {
				FS_SNS_TRACE1( "FS_SnsFormatHttpRequest ERROR. req len exceed to limit. req=%d", sns->req.req );
			}
			IFS_Strcat( szReq + len, "&content=" );
			len += 9;
			len += FS_UrlEncode( szReq + len, FS_SNS_MAX_REQ_LEN - len, sns->req.data.eml_send.content, -1 );
			break;
		case FS_SNS_REQ_EML_RECV:
			if ( sns->req.data.eml_recv.count == 0 ) {
				sns->req.data.eml_recv.count = FS_SNS_DEF_MSGS_COUNT;
			}
			if ( sns->req.data.eml_recv.page <= 0 ) {
				sns->req.data.eml_recv.page = 1;
			}
			IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_EML_RECV_URL, 
				sns->req.data.eml_recv.page, sns->req.data.eml_recv.count );
			if ( IFS_Strlen(szReq) < FS_SNS_MAX_REQ_LEN - 1 - 12 ) {
				if ( sns->req.data.eml_recv.page == 1 ) {
					IFS_Strcat( szReq, "&refresh=yes");
				} else {
					IFS_Strcat( szReq, "&refresh=no");
				}
			}
			break;
		case FS_SNS_REQ_EML_RECV_CONTENT:
			IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_EML_RECV_CONTENT_URL, sns->req.data.eml_recv.id );
			break;
		case FS_SNS_REQ_EML_GET_ACOUNT:
			IFS_Strncpy( szReq, FS_SNS_EML_GET_ACCOUNT_URL, FS_SNS_MAX_REQ_LEN - 1 );
			break;
		case FS_SNS_REQ_EML_GET_CONTACT:
			if ( sns->req.data.eml_contact.count == 0 ) {
				sns->req.data.eml_contact.count = FS_SNS_DEF_MSGS_COUNT;
			}
			if ( sns->req.data.eml_contact.page <= 0 ) {
				sns->req.data.eml_contact.page = 1;
			}
			IFS_Snprintf( szReq, FS_SNS_MAX_REQ_LEN - 1, FS_SNS_EML_GET_CONTACT_URL, 
				sns->req.data.eml_contact.page, sns->req.data.eml_contact.count );
			break;
		case FS_SNS_REQ_EML_ADD_CONTACT:
			IFS_Strncpy( szReq, FS_SNS_EML_ADD_CONTACT_URL, FS_SNS_MAX_REQ_LEN - 1 );
			len = IFS_Strlen( szReq );
			/* name */
			if ( FS_SNS_MAX_REQ_LEN - len > 3 * IFS_Strlen(sns->req.data.eml_contact.name) + 6 ) {
				IFS_Strcat( szReq + len, "&name=" );
				len += 6;
				len += FS_UrlEncode( szReq + len, FS_SNS_MAX_REQ_LEN - len, sns->req.data.eml_contact.name, -1 );
			} else {
				FS_SNS_TRACE1( "FS_SnsFormatHttpRequest ERROR. req len exceed to limit. req=%d", sns->req.req );
			}
			/* email */
			if ( FS_SNS_MAX_REQ_LEN - len > 3 * IFS_Strlen(sns->req.data.eml_contact.email) + 7 ) {
				IFS_Strcat( szReq + len, "&email=" );
				len += 7;
				len += FS_UrlEncode( szReq + len, FS_SNS_MAX_REQ_LEN - len, sns->req.data.eml_contact.email, -1 );
			} else {
				FS_SNS_TRACE1( "FS_SnsFormatHttpRequest ERROR. req len exceed to limit. req=%d", sns->req.req );
			}
			break;
		case FS_SNS_REQ_EML_DEL_CONTACT:
			IFS_Strncpy( szReq, FS_SNS_EML_DEL_CONTACT_URL, FS_SNS_MAX_REQ_LEN - 1 );
			len = IFS_Strlen( szReq );
			/* id */
			if ( FS_SNS_MAX_REQ_LEN - len > 3 * IFS_Strlen(sns->req.data.eml_contact.id) + 4 ) {
				IFS_Strcat( szReq + len, "&id=" );
				len += 4;
				len += FS_UrlEncode( szReq + len, FS_SNS_MAX_REQ_LEN - len, sns->req.data.eml_contact.id, -1 );
			} else {
				FS_SNS_TRACE1( "FS_SnsFormatHttpRequest ERROR. req len exceed to limit. req=%d", sns->req.req );
			}
			break;
		case FS_SNS_REQ_EML_MOD_CONTACT:
			IFS_Strncpy( szReq, FS_SNS_EML_MOD_CONTACT_URL, FS_SNS_MAX_REQ_LEN - 1 );
			len = IFS_Strlen( szReq );
			/* id */
			if ( FS_SNS_MAX_REQ_LEN - len > 3 * IFS_Strlen(sns->req.data.eml_contact.id) + 4 ) {
				IFS_Strcat( szReq + len, "&id=" );
				len += 4;
				len += FS_UrlEncode( szReq + len, FS_SNS_MAX_REQ_LEN - len, sns->req.data.eml_contact.id, -1 );
			} else {
				FS_SNS_TRACE1( "FS_SnsFormatHttpRequest ERROR. req len exceed to limit. req=%d", sns->req.req );
			}
			/* name */
			if ( FS_SNS_MAX_REQ_LEN - len > 3 * IFS_Strlen(sns->req.data.eml_contact.name) + 6 ) {
				IFS_Strcat( szReq + len, "&name=" );
				len += 6;
				len += FS_UrlEncode( szReq + len, FS_SNS_MAX_REQ_LEN - len, sns->req.data.eml_contact.name, -1 );
			} else {
				FS_SNS_TRACE1( "FS_SnsFormatHttpRequest ERROR. req len exceed to limit. req=%d", sns->req.req );
			}
			/* email */
			if ( FS_SNS_MAX_REQ_LEN - len > 3 * IFS_Strlen(sns->req.data.eml_contact.email) + 7 ) {
				IFS_Strcat( szReq + len, "&email=" );
				len += 7;
				len += FS_UrlEncode( szReq + len, FS_SNS_MAX_REQ_LEN - len, sns->req.data.eml_contact.email, -1 );
			} else {
				FS_SNS_TRACE1( "FS_SnsFormatHttpRequest ERROR. req len exceed to limit. req=%d", sns->req.req );
			}
			break;
		default:
			IFS_Free( szReq );
			szReq = FS_NULL;
			break;
		}
	} 

	return szReq;
}

static void FS_SnsRequestResult( FS_SnsSession *sns )
{
	FS_SnsReqCode reqCode = sns->req.req;

	FS_SNS_TRACE1( "FS_SnsRequestResult reqCode=%d", reqCode );
	if (reqCode != FS_SNS_REQ_NONE)
	{
		sns->offset = 0;
		sns->content_len = 0;
		FS_SnsFreeRequest( sns );
		sns->callback( sns, reqCode, &sns->rsp );
		FS_SnsFreeResponse( &sns->rsp, reqCode );
		FS_HttpRequestCancel( sns->http, FS_FALSE );
	}	
}

static void FS_SnsSendHttpRequest( FS_SnsSession *sns, FS_CHAR *url, FS_CHAR *headers )
{
	FS_SockAddr addr = {0};

	sns->offset = 0;
	IFS_Memset( sns->file, 0, sizeof(sns->file) );

	if( sns->config->use_proxy ){
		addr.host = sns->config->proxy_addr;
		addr.port = sns->config->proxy_port;
		FS_HttpRequest( sns->http, &addr, "GET", url, FS_NULL, headers );
	}else{
		FS_HttpRequest( sns->http, FS_NULL, "GET", url, FS_NULL, headers );
	}
}

static void FS_SnsPostHttpRequest( FS_SnsSession *sns, FS_CHAR *url, FS_CHAR *headers )
{
	FS_SockAddr addr = {0};
	FS_HttpPostDataStruct hPData = {0};

	sns->offset = 0;
	IFS_Memset( sns->file, 0, sizeof(sns->file) );

	hPData.content_type = "multipart/form-data; boundary="FS_SNS_BOUNDARY;
	hPData.data_is_file = FS_TRUE;
	hPData.data = sns->req.data.photo.fname;
	hPData.data_len = sns->req.data.photo.fsize;
	/* photo size */
	sns->status.total = sns->req.data.photo.fsize;

	if( sns->config->use_proxy ){
		addr.host = sns->config->proxy_addr;
		addr.port = sns->config->proxy_port;
		FS_HttpRequest( sns->http, &addr, "POST", url, &hPData, headers );
	}else{
		FS_HttpRequest( sns->http, FS_NULL, "POST", url, &hPData, headers );
	}
}

static void FS_SnsNetConnCallback( FS_SnsSession *sns, FS_BOOL bOK )
{
	FS_CHAR *url = FS_NULL;
	FS_BOOL ret = FS_FALSE;
	FS_CHAR *cookie = FS_NULL;

	FS_SNS_TRACE1( "FS_SnsNetConnCallback ok = %d", bOK );
	if( !bOK ){
		sns->rsp.result = FS_SNS_RSP_ERR_NET_CONN;
		FS_SnsRequestResult( sns );
		return;
	}
	
	/* format header */
	url = FS_SnsFormatHttpRequest(sns);
	if ( url == FS_NULL ) {
		FS_SNS_TRACE0( "FS_SnsNetConnCallback ERROR url = FS_NULL" );
		FS_SnsRequestResult( sns );
		return;
	}
	/* send request */
	if ( sns->req.req == FS_SNS_REQ_UPLOAD_PHOTO ) {
		FS_SnsPostHttpRequest( sns, url, sns->sessid );
	} else {
		FS_SnsSendHttpRequest( sns, url, sns->sessid );
	}
	IFS_Free( url );
}

FS_SnsLib *FS_SnsLibCreate( FS_SnsLibCallback cb )
{
	FS_SnsSession *sns = FS_NEW(FS_SnsSession);
	if (sns)
	{
		IFS_Memset( sns, 0, sizeof(FS_SnsSession) );
		sns->callback = cb;
		sns->http = FS_HttpCreateHandle( sns, FS_SnsHttpRspStartCB, FS_SnsHttpRspDataCB, FS_SnsHttpRspEndCB );
		if (sns->http == FS_NULL) {
			IFS_Free( sns );
			sns = FS_NULL;
		}
		FS_HttpSetRequestProgFunc( sns->http, FS_SnsHttpSendProgress );
		sns->config = &GFS_SnsConfig;
	}
	FS_SNS_TRACE1( "FS_SnsLibCreate sns = 0x%x", sns );
	return (FS_SnsLib *)sns;
}

/* return FALSE when cache is empty, reutrn TRUE if cache is available or partly available */
FS_BOOL FS_SnsLibReadCache( FS_SnsLib pSns, FS_SnsRequest *pReq, FS_SnsResponse *pRsp )
{
	FS_BOOL ret = FS_TRUE;
	FS_SnsSession *sns = (FS_SnsSession *)pSns;
	FS_CHAR *file;

	FS_SNS_TRACE3( "FS_SnsLibReadCache sns=0x%x, req=%d, sns->req=%d", pSns, pReq->req, sns->req.req );
	if (sns->req.req != FS_SNS_REQ_NONE) {
		return FS_FALSE;
	}

	IFS_Memcpy( &sns->req, pReq, sizeof(FS_SnsRequest) );

	switch ( pReq->req ) {
	case FS_SNS_REQ_GET_TWEET:
		IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "gettweet_%s.xml", pReq->data.get_tweet.type );
		if ( FS_FileGetSize( FS_DIR_TMP, sns->file ) <= 0 ) {
			ret = FS_FALSE;
			goto ERR_CATCH;
		}
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		FS_SnsXmlParseFile( sns );
		file = FS_SnsCacheGetImage( sns->rsp.data.get_tweet.msg.icon_url );
		if ( file ) {
			FS_COPY_TEXT( sns->rsp.data.get_tweet.msg.icon_file, file );
		}
		IFS_Memcpy( pRsp, &sns->rsp, sizeof(FS_SnsResponse) );
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		break;
	case FS_SNS_REQ_GET_UPDATE:
		IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "updates_%s_%d.xml", 
			pReq->data.get_msgs.type, pReq->data.get_msgs.page );
		if ( FS_FileGetSize( FS_DIR_TMP, sns->file ) <= 0 ) {
			ret = FS_FALSE;
			goto ERR_CATCH;
		}
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		FS_SnsXmlParseFile( sns );
		FS_SnsMsgsCheckCacheImage( sns, FS_NULL );
		IFS_Memcpy( pRsp, &sns->rsp, sizeof(FS_SnsResponse) );
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		break;
	case FS_SNS_REQ_GET_REPLY:
		IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "reply_%s_%d.xml", 
			pReq->data.get_msgs.type, pReq->data.get_msgs.page );
		if ( FS_FileGetSize( FS_DIR_TMP, sns->file ) <= 0 ) {
			ret = FS_FALSE;
			goto ERR_CATCH;
		}
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		FS_SnsXmlParseFile( sns );
		FS_SnsMsgsCheckCacheImage( sns, FS_NULL );
		IFS_Memcpy( pRsp, &sns->rsp, sizeof(FS_SnsResponse) );
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		break;
	case FS_SNS_REQ_RSS_GET_ALL_ARTICLE:
		IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "rss_articles_%d.xml", sns->req.data.rss.page );
		if ( FS_FileGetSize( FS_DIR_TMP, sns->file ) <= 0 ) {
			ret = FS_FALSE;
			goto ERR_CATCH;
		}
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		FS_SnsXmlParseFile( sns );
		IFS_Memcpy( pRsp, &sns->rsp, sizeof(FS_SnsResponse) );
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		break;
	case FS_SNS_REQ_RSS_GET_ALL_CHANNEL:
		IFS_Strncpy( sns->file, "rss_channels.xml", sizeof(sns->file) - 1 );
		if ( FS_FileGetSize( FS_DIR_TMP, sns->file ) <= 0 ) {
			ret = FS_FALSE;
			goto ERR_CATCH;
		}
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		FS_SnsXmlParseFile( sns );
		FS_SnsRssChannelCheckCacheImage( sns, FS_NULL );
		IFS_Memcpy( pRsp, &sns->rsp, sizeof(FS_SnsResponse) );
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		break;
	case FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY:
		IFS_Strncpy( sns->file, "channel_category.xml", sizeof(sns->file) - 1 );
		if ( FS_FileGetSize( FS_DIR_TMP, sns->file ) <= 0 ) {
			ret = FS_FALSE;
			goto ERR_CATCH;
		}
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		FS_SnsXmlParseFile( sns );
		FS_SnsRssCategoryCheckCacheImage( sns, FS_NULL );
		IFS_Memcpy( pRsp, &sns->rsp, sizeof(FS_SnsResponse) );
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		break;
	case FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL:
		IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "channel_articles_%d.xml", sns->req.data.rss.page );
		if ( FS_FileGetSize( FS_DIR_TMP, sns->file ) <= 0 ) {
			ret = FS_FALSE;
			goto ERR_CATCH;
		}
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		FS_SnsXmlParseFile( sns );
		IFS_Memcpy( pRsp, &sns->rsp, sizeof(FS_SnsResponse) );
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		break;
	case FS_SNS_REQ_EML_RECV:
		IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "emails_%d.xml", sns->req.data.eml_recv.page );
		if ( FS_FileGetSize( FS_DIR_TMP, sns->file ) <= 0 ) {
			ret = FS_FALSE;
			goto ERR_CATCH;
		}
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		FS_SnsXmlParseFile( sns );
		IFS_Memcpy( pRsp, &sns->rsp, sizeof(FS_SnsResponse) );
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		break;
	case FS_SNS_REQ_EML_GET_ACOUNT:
		IFS_Strncpy( sns->file, "email_account.xml", sizeof(sns->file) - 1 );
		if ( FS_FileGetSize( FS_DIR_TMP, sns->file ) <= 0 ) {
			ret = FS_FALSE;
			goto ERR_CATCH;
		}
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		FS_SnsXmlParseFile( sns );
		IFS_Memcpy( pRsp, &sns->rsp, sizeof(FS_SnsResponse) );
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		break;
	case FS_SNS_REQ_GET_FRIENDS:
		IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "%s_friends_%d.xml", 
			sns->req.data.friends.type, sns->req.data.friends.page );	
		if ( FS_FileGetSize( FS_DIR_TMP, sns->file ) <= 0 ) {
			ret = FS_FALSE;
			goto ERR_CATCH;
		}
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		FS_SnsXmlParseFile( sns );
		FS_SnsFriendsCheckCacheImage( sns, FS_NULL );
		IFS_Memcpy( pRsp, &sns->rsp, sizeof(FS_SnsResponse) );
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		break;
	case FS_SNS_REQ_FIND_FRIENDS:
		IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "%s_find_friends_%d.xml", 
			sns->req.data.friends.type, sns->req.data.friends.page );	
		if ( FS_FileGetSize( FS_DIR_TMP, sns->file ) <= 0 ) {
			ret = FS_FALSE;
			goto ERR_CATCH;
		}
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		FS_SnsXmlParseFile( sns );
		FS_SnsFriendsCheckCacheImage( sns, FS_NULL );
		IFS_Memcpy( pRsp, &sns->rsp, sizeof(FS_SnsResponse) );
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		break;
	case FS_SNS_REQ_EML_GET_CONTACT:
		IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "eml_contact_%d.xml", sns->req.data.eml_contact.page );
		if ( FS_FileGetSize( FS_DIR_TMP, sns->file ) <= 0 ) {
			ret = FS_FALSE;
			goto ERR_CATCH;
		}
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		FS_SnsXmlParseFile( sns );
		IFS_Memcpy( pRsp, &sns->rsp, sizeof(FS_SnsResponse) );
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		break;
	default:
		FS_SNS_TRACE1( "FS_SnsLibReadCache ERROR. unknow req(%d)", pReq->req );
		ret = FS_FALSE;
		break;
	}

ERR_CATCH:
	IFS_Memset( sns->file, 0, sizeof(sns->file) );
	IFS_Memset( &sns->req, 0, sizeof(sns->req) );
	return ret;
}

void FS_SnsLibClearCache( FS_SnsLib pSns, FS_SnsRequest *pReq )
{
	FS_SnsSession *sns = (FS_SnsSession *)pSns;
	FS_CHAR szFileName[FS_FILE_NAME_LEN] = {0};
	FS_SINT4 i;

	FS_SNS_TRACE3( "FS_SnsLibClearCache sns=0x%x, req=%d, sns->req=%d", pSns, pReq->req, sns->req.req );
	if (sns->req.req != FS_SNS_REQ_NONE) {
		/* only alown to clear cache when sns lib is in idle state */
		return;
	}

	switch ( pReq->req ) {
	case FS_SNS_REQ_GET_UPDATE:
		IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "updates_%s", pReq->data.get_msgs.type );
		break;
	case FS_SNS_REQ_GET_REPLY:
		IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "reply_%s", pReq->data.get_msgs.type );
		break;
	case FS_SNS_REQ_RSS_GET_ALL_ARTICLE:
		IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "rss_articles" );
		break;
	case FS_SNS_REQ_EML_RECV:
		IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "emails" );
		break;
	case FS_SNS_REQ_GET_FRIENDS:
		IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "%s_friends", pReq->data.friends.type );	
		break;
	case FS_SNS_REQ_FIND_FRIENDS:
		IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "%s_find_friends", pReq->data.friends.type );	
		break;
	case FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL:
		IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "channel_articles" );
		break;
	case FS_SNS_REQ_EML_GET_CONTACT:
		IFS_Snprintf( sns->file, sizeof(sns->file) - 1, "eml_contact" );
		break;
	default:
		FS_SNS_TRACE1( "FS_SnsLibClearCache ERROR. unknow req(%d)", pReq->req );
		return;
	}

	for ( i = 1; i < 100; i ++ ) {
		IFS_Snprintf( szFileName, sizeof(szFileName) - 1, "%s_%d.xml", sns->file, i );
		if ( FS_FALSE == FS_FileDelete( FS_DIR_TMP, szFileName ) ) {
			break;
		}
	}
	IFS_Memset( sns->file, 0, sizeof(sns->file) );

	FS_SNS_TRACE1( "FS_SnsLibClearCache clear %d cache pages", i );
}

FS_BOOL FS_SnsLibRequest( FS_SnsLib pSns, FS_SnsRequest *pReq )
{
	FS_SnsSession *sns = (FS_SnsSession *)pSns;
	FS_SNS_TRACE3( "FS_SnsLibRequest sns=0x%x, req=%d, sns->req=%d", pSns, pReq->req, sns->req.req );
	
	if (sns->req.req == FS_SNS_REQ_NONE) {
		FS_SnsCopyRequestData( sns, pReq );
		IFS_Memset( &sns->rsp, 0, sizeof(FS_SnsResponse) );
		IFS_Memset( &sns->status, 0, sizeof(FS_SnsReqStatus) );
		FS_NetConnect( sns->config->apn, sns->config->user, sns->config->pass, 
			FS_SnsNetConnCallback, FS_APP_SNS, FS_FALSE, sns );

		sns->status.status = FS_SNS_REQ_STS_SEND_REQUEST;
		return FS_TRUE;
	}
	return FS_FALSE;
}

FS_SnsReqStatus * FS_SnsLibGetReqStatus( FS_SnsLib pSns )
{
	FS_SnsSession *sns = (FS_SnsSession *)pSns;
	return &sns->status;
}

void FS_SnsLibCancelRequest( FS_SnsLib pSns )
{
	FS_SnsSession *sns = (FS_SnsSession *)pSns;
	if (sns->req.req != FS_SNS_REQ_NONE) {
		if (sns->http) {
			FS_HttpRequestCancel( sns->http, FS_FALSE );
		}
		sns->req.req = FS_SNS_REQ_NONE;
		if (sns->timer_id) {
			IFS_StopTimer( sns->timer_id );
			sns->timer_id = 0;
		}
	}
}

void FS_SnsLibDestroy( FS_SnsLib pSns )
{
	FS_SnsSession *sns = (FS_SnsSession *)pSns;
	FS_SNS_TRACE1( "FS_SnsLibDestroy sns = 0x%x", sns );
	if (pSns)
	{
		if (sns->req.req != FS_SNS_REQ_NONE)
			FS_HttpRequestCancel( sns->http, FS_FALSE );
		FS_HttpDestroyHandle( sns->http );
		FS_NetDisconnect( FS_APP_SNS );
		FS_SAFE_FREE( sns->sessid );
		if ( sns->timer_id != 0 ) {
			IFS_StopTimer( sns->timer_id );
			sns->timer_id = 0;
		}
		IFS_Free( sns );
	}
}

void FS_SnsInitConfig( void )
{
	if( FS_FileRead( FS_DIR_TMP, FS_SNS_CFG_FILE, 0, &GFS_SnsConfig, sizeof(FS_SnsConfig) ) != sizeof(FS_SnsConfig) )
	{
		/* set to default */
		GFS_SnsConfig.save_passwd = FS_TRUE;
		GFS_SnsConfig.auto_login = FS_TRUE;
		GFS_SnsConfig.display_image = FS_FALSE;
		GFS_SnsConfig.image_cache = FS_TRUE;
		GFS_SnsConfig.time_zone = 8;
		GFS_SnsConfig.msg_cnt_per_page = FS_SNS_DEF_MSGS_COUNT;

		IFS_Strcpy( GFS_SnsConfig.apn, "CMNET" );
		IFS_Strcpy( GFS_SnsConfig.proxy_addr, "10.0.0.172" );
		GFS_SnsConfig.proxy_port = 80;
		GFS_SnsConfig.use_proxy = FS_FALSE;
	}

	GFS_SnsConfig.accout = GFS_SnsAccounts;
	GFS_SnsConfig.account_num = sizeof(GFS_SnsAccounts) / sizeof(GFS_SnsAccounts[0]);

	IFS_Memset( &GFS_SnsCache, 0, sizeof(GFS_SnsCache) );
	/* init sns cache */
	FS_FileRead( FS_DIR_TMP, FS_SNS_CACHE_FILE, 0, &GFS_SnsCache, sizeof(GFS_SnsCache) );
}

FS_SnsConfig *FS_SnsGetConfig( void )
{
	return &GFS_SnsConfig;
}

void FS_SnsSetConfig( FS_SnsConfig *config )
{
	if (config != &GFS_SnsConfig)
		IFS_Memcpy( &GFS_SnsConfig, config, sizeof(FS_SnsConfig) );
	FS_FileWrite( FS_DIR_TMP, FS_SNS_CFG_FILE, 0, &GFS_SnsConfig, sizeof(FS_SnsConfig) );
}


#endif //FS_MODULE_SNS
