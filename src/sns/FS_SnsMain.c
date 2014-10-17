#include "inc/FS_Config.h"

#ifdef FS_MODULE_SNS
#include "inc\FS_Config.h"
#include "inc\inte\FS_Inte.h"
#include "inc\res\FS_Res.h"
#include "inc\gui\FS_Gui.h"
#include "inc\util\FS_File.h"
#include "inc\util\FS_Util.h"
#include "inc\sns\FS_SnsLib.h"
#include "inc\util\FS_NetConn.h"
#include "inc\util\FS_MemDebug.h"

/************************************************************************/
/*                           local feature                              */
/************************************************************************/
/*
 * disable message list function in sns list form, 
 * also disable to display summary for sns list form
 */
#undef FS_SNS_ENABLE_MSG_LIST
/*
 * define to enable about menu item in login menu
 */
#define FS_SNS_ENABLE_SFOX_COPYRIGHT

/*
 * define to enable photo upload feature
 */
#undef FS_SNS_ENABLE_PHOTO_UPLOAD

/************************************************************************/
/*                          local feature end                           */
/************************************************************************/

typedef struct FS_SnsApp_Tag
{
	FS_SnsLib		*snslib;
	FS_UINT4		timer_id;
	FS_UINT4		counter;

	FS_UINT4		update_page;
	FS_UINT4		reply_page;
	FS_UINT4		msg_page;
	FS_UINT4		friends_page;
	FS_UINT4		friends_search_page;
	FS_UINT4		rss_article_page;
	FS_UINT4		rss_channel_article_page;
	FS_UINT4		eml_page;
	FS_UINT4		eml_contact_page;
	FS_SnsResponse	rsp_update;
	FS_SnsResponse	rsp_reply;
	FS_SnsResponse	rsp_msglist;
	FS_SnsResponse	rsp_friends;
	FS_SnsResponse	rsp_rss_articles;
	FS_SnsResponse	rsp_rss_channels;
	FS_SnsResponse	rsp_rss_channel_articles;
	FS_SnsResponse	rsp_rss_channel_category;
	FS_SnsResponse	rsp_rss_second_category;
	FS_SnsResponse	rsp_rss_category_detail;
	FS_SnsResponse	rsp_eml_inbox;
	FS_SnsResponse	rsp_eml_contact;
	FS_Window		*cur_req_win;	/* current request win */
	FS_UINT4		backup_page_index;
	FS_CHAR			*fname;			/* filename user selected to upload */
	FS_SINT4		fsize;
	FS_BOOL			is_friends_search_list;
	/*
	 * current msg view in msg detail form
	 * we save this for user to data.reply in case of the msg list is updated in background
	*/
	FS_SnsMsg		cur_msg;		
}FS_SnsApp;

extern FS_UINT1 GFS_SkinIndex;

static FS_SnsApp GFS_SnsApp;

static void FS_SnsList_UI( FS_Window *win );
static FS_BOOL FS_SnsRequestWinProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam );
static void FS_SnsUIRequestStop( void );
static void FS_SnsEntryShowMsgList( FS_SnsResponse *rsp, FS_SnsReqCode reqCode, FS_BOOL bUpdateScreen );
static void FS_SnsEntryGetMsgs( FS_Window *win, FS_SnsReqCode reqCode, FS_BOOL bCacheFirst );
static void FS_SnsMsgDetailMenu_UI( FS_Window *win );
static void FS_SnsMsgShowDetail_UI( FS_BOOL bReply );
static void FS_SnsEntryFormSetTitle( FS_Window *win, FS_SnsReqCode reqCode, FS_BOOL bUpdateScreen );
static FS_BOOL FS_SnsPhotoUploadCnf_CB( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam );
static void FS_SnsDelPhoto_CB( FS_Window *win );
static void FS_MainMenu_UI( FS_Window *win );
static void FS_SnsEntryShowFriendsList( FS_SnsResponse *rsp, FS_SnsReqCode reqCode, FS_BOOL bUpdateScreen );
static void FS_SnsRssShowArticleList( FS_SnsResponse *rsp, FS_SnsReqCode req, FS_BOOL bUpdateScreen );
static void FS_SnsRssShowChannelList( FS_SnsResponse *rsp, FS_SnsReqCode req, FS_BOOL bUpdateScreen );
static void FS_SnsRssShowChannelArticleList( FS_SnsResponse *rsp, FS_SnsReqCode req, FS_BOOL bUpdateScreen );
static void FS_SnsRssArticleUpdate_CB( FS_Window *win );
static void FS_SnsRss_UI( FS_Window *win );
static void FS_SnsEmail_UI( FS_Window *win );
static void FS_SnsEmlShowInboxList( FS_SnsResponse *rsp, FS_SnsReqCode req, FS_BOOL bUpdateScreen );
static void FS_SnsRssFormSetTitle( FS_Window *win, FS_SnsReqCode reqCode, FS_BOOL bUpdateScreen );
static void FS_SnsEmailFormSetTitle( FS_Window *win, FS_SnsReqCode reqCode, FS_BOOL bUpdateScreen );
static void FS_SnsEmlShowAccount( FS_SnsResponse *rsp );
static void FS_SnsEntryShowFindFriendsWidget( FS_Window *win, FS_CHAR *txt );
static void FS_SnsRssChannelArticleListSetTitle( FS_Window *win, FS_BOOL bUpdateScreen );
static void FS_SnsEmlDetail_UI( FS_Window *lwin );
static void FS_SnsRssShowChannelCategoryList( FS_SnsResponse *rsp, FS_SnsReqCode req, FS_BOOL bUpdateScreen );
static void FS_SnsRssCategoryDetailSetTitle( FS_Window *win, FS_BOOL bUpdateScreen );
static void FS_SnsRssShowCategoryDetail( FS_SnsResponse *rsp, FS_SnsReqCode req, FS_BOOL bUpdateScreen );
static void FS_SnsEmlShowContactList( FS_SnsResponse *rsp, FS_SnsReqCode req, FS_BOOL bUpdateScreen );
static void FS_SnsEmlAddContact_CB( FS_Window *win );
static void FS_SnsRssArticleDetailMore_CB( FS_Window *win );
static void FS_SnsRssChannelArticleList_UI( FS_Window *win );
static void FS_SnsEmlContactPostUpdate_CB( void );
static void FS_SnsEmlReceiverMenu_UI( FS_Window *win );
static void FS_SnsEmlGetAccountPost_CB( void );
static void FS_SnsRssShowSecondCategoryList( FS_SnsResponse *rsp, FS_SnsReqCode req, FS_BOOL bUpdateScreen );
static void FS_SnsRssSecondCategoryFormSetTitle( FS_Window *win, FS_BOOL bUpdateScreen );

static FS_CHAR *FS_SnsUtilSecondsToDateTime( FS_CHAR *seconds )
{
	static FS_CHAR s_text[64] = {0};
	FS_DateTime dt = {0};
	FS_UINT4 secs;
	FS_DateTime now = {0};
	FS_SnsConfig *config = FS_SnsGetConfig( );

	if ( seconds == FS_NULL ) seconds = "";
	secs = IFS_Atoi( seconds );
	if ( secs != 0 ) {
		FS_SecondsToDateTime( &dt, secs, config->time_zone );
	} else {
		IFS_GetDateTime( &dt );
	}

	IFS_GetDateTime( &now );
	if ( dt.year != now.year ) {
		IFS_Snprintf( s_text, sizeof(s_text) - 1, "%d%s", now.year - dt.year, FS_Text(FS_T_N_YEAR_AGO) );
	} else if ( dt.month != now.month ) {
		IFS_Snprintf( s_text, sizeof(s_text) - 1, "%d%s", now.month - dt.month, FS_Text(FS_T_N_MONTH_AGO) );
	} else if ( dt.day != now.day ) {
		IFS_Snprintf( s_text, sizeof(s_text) - 1, "%d%s", now.day - dt.day, FS_Text(FS_T_N_DAY_AGO) );
	} else if ( dt.hour != now.hour ) {
		IFS_Snprintf( s_text, sizeof(s_text) - 1, "%d%s", now.hour - dt.hour, FS_Text(FS_T_N_HOUR_AGO) );
	} else if ( dt.min != now.min ) {
		IFS_Snprintf( s_text, sizeof(s_text) - 1, "%d%s", now.min - dt.min, FS_Text(FS_T_N_MINUTE_AGO) );
	} else {
		IFS_Snprintf( s_text, sizeof(s_text) - 1, "%d%s", now.sec - dt.sec, FS_Text(FS_T_N_SECOND_AGO) );
	}
	return s_text;
}

static void FS_SnsLibEventCB( FS_SnsLib *pSns, FS_SnsReqCode reqCode, FS_SnsResponse *rsp )
{
	FS_CHAR szError[512] = {0};

	switch (reqCode)
	{
	case FS_SNS_REQ_LOGIN:
		if (rsp->result == FS_SNS_RSP_SUCCESS)
		{
			FS_MainMenu_UI( FS_NULL );	//FS_SnsList_UI( FS_NULL );
			FS_DestroyWindowByID( FS_W_SnsLoginFrm );
		}
		else
		{
			if ( rsp->err_info ) {
				IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
				FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
			} else {
				FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
			}
		}
		FS_SnsUIRequestStop( );
		break;

	case FS_SNS_REQ_REGISTER:
		if (rsp->result == FS_SNS_RSP_SUCCESS)
		{
			FS_Window *rwin, *lwin;
			FS_Widget *rwgt, *lwgt;

			FS_MessageBox( FS_MS_OK, FS_Text(FS_T_REGISTER_SUCCESS), FS_NULL, FS_FALSE);

			// register success, put the new user name into data.login win
			rwin = FS_WindowFindId( FS_W_SnsRegisterFrm );
			if (rwin)
			{
				rwgt = FS_WindowGetWidget( rwin, FS_W_SnsUserName );	
				lwin = FS_WindowFindId( FS_W_SnsLoginFrm );
				if (lwin)
				{
					lwgt = FS_WindowGetWidget( lwin, FS_W_SnsUserName );
					FS_WidgetSetText( lwgt, rwgt->text );
					lwgt = FS_WindowGetWidget( lwin, FS_W_SnsPassword );
					FS_WidgetSetText( lwgt, FS_NULL );
				}
				FS_DestroyWindow( rwin );
			}
			
		}
		else
		{
			if ( rsp->err_info ) {
				IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
				FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
			} else {
				FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
			}
		}
		FS_SnsUIRequestStop( );
		break;
	case FS_SNS_REQ_GET_TWEET:
	{
		FS_Window *win;
		FS_Widget *wgt;
		FS_CHAR absFile[FS_MAX_PATH_LEN] = {0};

		win = FS_WindowFindId( FS_W_SnsEntryFrm );
		if ( win && rsp->data.get_tweet.msg.msg ) {
			wgt = FS_WindowGetWidget( win, FS_W_SnsComposeTabAuthor );
			if ( wgt ) {
				if ( rsp->data.get_tweet.msg.icon_file ) {
					FS_GetAbsFileName( FS_DIR_TMP, rsp->data.get_tweet.msg.icon_file, absFile );
					if ( wgt->file == FS_NULL || IFS_Strcmp(wgt->file, absFile) != 0 ) {
						FS_WidgetSetIconFile( wgt, absFile );
					}
				}
				FS_WidgetSetText( wgt, rsp->data.get_tweet.msg.author );
				FS_WidgetSetSubText( wgt, FS_SnsUtilSecondsToDateTime(rsp->data.get_tweet.msg.date) );
			}
			wgt = FS_WindowGetWidget( win, FS_W_SnsComposeTabTweet );
			if ( wgt ) {
				FS_WidgetSetText( wgt, rsp->data.get_tweet.msg.msg );
			}
			FS_InvalidateRect( win, &win->client_rect );
		} else {
			if ( rsp->result != FS_SNS_RSP_SUCCESS ) {
				if ( rsp->err_info ) {
					IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
					FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
				} else {
					FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
				}
			}
		}
		FS_SnsUIRequestStop( );
		break;
	}
	case FS_SNS_REQ_GET_UPDATE:
	case FS_SNS_REQ_GET_REPLY:
		if ( rsp->data.msglist.count > 0 ) {
			FS_SnsEntryShowMsgList( rsp, reqCode, FS_TRUE );
		} else {
			FS_Window *win = FS_WindowFindId( FS_W_SnsEntryFrm );			
			if ( win == FS_NULL ) {
				win = FS_WindowFindId( FS_W_SnsMsgListFrm );
			}
			if ( win ) {
				FS_SnsEntryFormSetTitle( win, reqCode, FS_TRUE );
			}

			if ( rsp->result != FS_SNS_RSP_SUCCESS ) {
				if ( rsp->err_info ) {
					IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
					FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
				} else {
					FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
				}
			} else {
				FS_MessageBox( FS_MS_OK, FS_Text(FS_T_NO_NEW_DATA), FS_NULL, FS_FALSE );
			}
		}
		FS_SnsUIRequestStop( );
		break;
	case FS_SNS_REQ_GET_UPDATE_CONTENT:
		if ( rsp->data.msglist.count > 0 ) {
			FS_Window *win;
			FS_Widget *wgtContent = FS_NULL;
			FS_CHAR *content = FS_NULL;
			
			/* update rss detail if any */
			win = FS_WindowFindId( FS_W_SnsRssDetailFrm );
			if ( win ) {
				wgtContent = FS_WindowGetWidget( win, FS_W_SnsRssDetailMsg );
			}
			/* update email detail if any */
			if ( wgtContent == FS_NULL ) {
				win = FS_WindowFindId( FS_W_SnsEmlDetailFrm );
				if ( win ) {
					wgtContent = FS_WindowGetWidget( win, FS_W_SnsEmlDetailContent );
				}
			}
			if ( wgtContent ) {
				content = IFS_Strstr( rsp->data.msglist.msgs[0].msg, "=--=" );
				if ( content == FS_NULL ) {
					content = rsp->data.msglist.msgs[0].msg;
				} else {
					content += 4;
				}
				FS_WidgetSetText( wgtContent, FS_ProcessParagraph(content, -1) );
			}
		} else {
			if ( rsp->result != FS_SNS_RSP_SUCCESS ) {
				if ( rsp->err_info ) {
					IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
					FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
				} else {
					FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
				}
			} else {
				FS_MessageBox( FS_MS_OK, FS_Text(FS_T_NO_NEW_DATA), FS_NULL, FS_FALSE );
			}
		}
		FS_SnsUIRequestStop( );
		break;
	case FS_SNS_REQ_SET_TWEET:
		if ( rsp->result == FS_SNS_RSP_SUCCESS ) {
			FS_Window *win = FS_WindowFindId( FS_W_SnsEntryFrm );
			FS_Widget *wgt, *wEdit;
			if ( win ) {
				wEdit = FS_WindowGetWidget( win, FS_W_SnsComposeTabEdit );
				wgt = FS_WindowGetWidget( win, FS_W_SnsComposeTabTweet );

				FS_WidgetSetText( wgt, wEdit->text );
				FS_WidgetSetText( wEdit, FS_NULL );
			}
			FS_DestroyWindowByID( FS_W_SnsRssForwardFrm );
			FS_MessageBox( FS_MS_OK, FS_Text(FS_T_SUCCESS), FS_NULL, FS_FALSE );
		} else {
			if ( rsp->err_info ) {
				IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
				FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
			} else {
				FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
			}
		}
		FS_SnsUIRequestStop( );
		break;
	case FS_SNS_REQ_RETWEET:
	case FS_SNS_REQ_SET_LIKE:
	case FS_SNS_REQ_REPLY:
		if ( rsp->result == FS_SNS_RSP_SUCCESS ) {
			if ( reqCode == FS_SNS_REQ_REPLY ) {
				FS_SnsMsgShowDetail_UI( FS_FALSE );
			}
			FS_MessageBox( FS_MS_OK, FS_Text(FS_T_SUCCESS), FS_NULL, FS_FALSE );
		} else {
			if ( rsp->err_info ) {
				IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
				FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
			} else {
				FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
			}
		}
		FS_SnsUIRequestStop( );
		break;
	case FS_SNS_REQ_GET_SUMMARY:
		if ( rsp->result == FS_SNS_RSP_SUCCESS ) {
			FS_Widget *wgt;
			FS_Window *win;
			FS_SnsConfig *config = FS_SnsGetConfig();
			FS_SINT4 i;
			FS_CHAR szTxt[64] = {0};

			win = FS_WindowFindId( FS_W_SnsListFrm );
			if ( win ) {
				for ( i = 0; i < config->account_num; i ++ ) {
					wgt = FS_WindowGetWidget( win, FS_W_SnsItemUpdates + i );
					if ( wgt ) {
						IFS_Snprintf( szTxt, sizeof(szTxt) - 1, "%03d >>", config->accout[i].update_cnt );
						FS_WidgetSetExtraText( wgt, szTxt );
					}
					wgt = FS_WindowGetWidget( win, FS_W_SnsItemReply + i );
					if ( wgt ) {
						IFS_Snprintf( szTxt, sizeof(szTxt) - 1, "%03d >>", config->accout[i].reply_cnt );
						FS_WidgetSetExtraText( wgt, szTxt );
					}
				}
				FS_WindowSetTitle( win, FS_Text(FS_T_EXTENSION) );
				FS_InvalidateRect( win, FS_NULL );
			}
		} else {
			if ( rsp->err_info ) {
				IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
				FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
			} else {
				FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
			}
		}
		FS_SnsUIRequestStop( );
		break;
	case FS_SNS_REQ_UPLOAD_PHOTO:
		if ( rsp->result == FS_SNS_RSP_SUCCESS ) {
			FS_SnsDelPhoto_CB( FS_NULL );
			FS_MessageBox( FS_MS_OK, FS_Text(FS_T_SUCCESS), FS_NULL, FS_FALSE );
		} else {
			if ( rsp->err_info ) {
				IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
				FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
			} else {
				FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
			}
		}
		FS_SnsUIRequestStop( );
		break;
	case FS_SNS_REQ_GET_FRIENDS:
	case FS_SNS_REQ_FIND_FRIENDS:
		if ( rsp->data.friends.count > 0 ) {
			FS_SnsEntryShowFriendsList( rsp, reqCode, FS_TRUE );
		} else {
			FS_Window *win = FS_WindowFindId( FS_W_SnsEntryFrm );			
			if ( reqCode == FS_SNS_REQ_GET_FRIENDS ) {
				GFS_SnsApp.friends_page = GFS_SnsApp.backup_page_index;
			}
			if ( win ) {
				FS_Widget *wItem = FS_NULL;
				FS_CHAR *txt = FS_NULL;
				FS_SnsEntryFormSetTitle( win, reqCode, FS_FALSE );
				wItem = FS_WindowGetWidget( win, FS_W_SnsFriendsTabEdit );
				if ( wItem && wItem->text ) {
					txt = IFS_Strdup( wItem->text ); 
				}
				
				if ( reqCode == FS_SNS_REQ_FIND_FRIENDS ) {
					FS_WindowDelTabSheetWidgetList( win, FS_W_SnsTabFriends );
					FS_SnsEntryShowFindFriendsWidget( win, txt );
				}
				FS_SAFE_FREE( txt );
				FS_InvalidateRect( win, FS_NULL );
			}
			
			if ( rsp->result != FS_SNS_RSP_SUCCESS ) {
				if ( rsp->err_info ) {
					IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
					FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
				} else {
					FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
				}
			} else {
				FS_MessageBox( FS_MS_OK, FS_Text(FS_T_NO_NEW_DATA), FS_NULL, FS_FALSE );
			}
		}
		FS_SnsUIRequestStop( );
		break;
	case FS_SNS_REQ_ADD_FRIEND:
	case FS_SNS_REQ_DEL_FRIEND:
		if ( rsp->result == FS_SNS_RSP_SUCCESS ) {
			FS_MessageBox( FS_MS_OK, FS_Text(FS_T_SUCCESS), FS_NULL, FS_FALSE );
		} else {
			if ( rsp->err_info ) {
				IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
				FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
			} else {
				FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
			}
		}
		FS_SnsUIRequestStop( );
		break;
	case FS_SNS_REQ_RSS_GET_ALL_ARTICLE:
		if ( rsp->data.rss_article.count > 0 ) {
			FS_SnsRssShowArticleList( rsp, reqCode, FS_TRUE );
		} else {
			FS_Window *win = FS_WindowFindId( FS_W_SnsRssFrm );			
			GFS_SnsApp.rss_article_page = GFS_SnsApp.backup_page_index;
			FS_SnsRssFormSetTitle( win, FS_SNS_REQ_RSS_GET_ALL_ARTICLE, FS_TRUE );
			if ( rsp->result != FS_SNS_RSP_SUCCESS ) {
				if ( rsp->err_info ) {
					IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
					FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
				} else {
					FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
				}
			} else {
				FS_MessageBox( FS_MS_OK, FS_Text(FS_T_NO_NEW_DATA), FS_NULL, FS_FALSE );
			}
		}
		FS_SnsUIRequestStop( );
		break;
	case FS_SNS_REQ_RSS_GET_ALL_CHANNEL:
		if ( rsp->data.rss_channel.count > 0 ) {
			FS_SnsRssShowChannelList( rsp, reqCode, FS_TRUE );
		} else {
			FS_Window *win = FS_WindowFindId( FS_W_SnsRssFrm );			
			FS_SnsRssFormSetTitle( win, FS_SNS_REQ_RSS_GET_ALL_CHANNEL, FS_TRUE );
			if ( rsp->result != FS_SNS_RSP_SUCCESS ) {
				if ( rsp->err_info ) {
					IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
					FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
				} else {
					FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
				}
			} else {
				FS_MessageBox( FS_MS_OK, FS_Text(FS_T_NO_NEW_DATA), FS_NULL, FS_FALSE );
			}
		}
		FS_SnsUIRequestStop( );
		break;
	case FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL:
		if ( rsp->data.rss_article.count > 0 ) {
			FS_SnsRssShowChannelArticleList( rsp, reqCode, FS_TRUE );
		} else {
			FS_Window *win = FS_WindowFindId( FS_W_SnsRssChannelArticleListFrm );			
			GFS_SnsApp.rss_channel_article_page = GFS_SnsApp.backup_page_index;
			FS_SnsRssChannelArticleListSetTitle( win, FS_FALSE );
			if ( rsp->result != FS_SNS_RSP_SUCCESS ) {
				if ( rsp->err_info ) {
					IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
					FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
				} else {
					FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
				}
			} else {
				FS_MessageBox( FS_MS_OK, FS_Text(FS_T_NO_NEW_DATA), FS_NULL, FS_FALSE );
			}
		}
		FS_SnsUIRequestStop( );
		break;
	case FS_SNS_REQ_RSS_GET_ALL_ARTICLE_CONTENT:
	case FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL_CONTENT:
		if ( rsp->data.rss_article.count > 0 ) {
			FS_Window *win = FS_WindowFindId( FS_W_SnsRssDetailFrm );
			FS_Widget *wgt = FS_NULL;
			if ( win ) wgt = FS_WindowGetWidget( win, FS_W_SnsRssDetailMsg );
			if ( wgt ) FS_WidgetSetText( wgt, FS_ProcessParagraph(rsp->data.rss_article.articles[0].msg, -1) );
		} else {
			if ( rsp->result != FS_SNS_RSP_SUCCESS ) {
				if ( rsp->err_info ) {
					IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
					FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
				} else {
					FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
				}
			} else {
				FS_MessageBox( FS_MS_OK, FS_Text(FS_T_NO_NEW_DATA), FS_NULL, FS_FALSE );
			}
		}
		FS_SnsUIRequestStop( );
		break;
	case FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY:
		if ( rsp->data.rss_category.count > 0 ) {
			FS_Window *win = FS_WindowFindId( FS_W_SnsRssSecondCategoryFrm );
			if ( win ) {
				FS_SnsRssShowSecondCategoryList( rsp, reqCode, FS_TRUE );
			} else {
				FS_SnsRssShowChannelCategoryList( rsp, reqCode, FS_TRUE );
			}
		} else {
			FS_Window *win = FS_WindowFindId( FS_W_SnsRssSecondCategoryFrm );
			if ( win ) {
				FS_SnsRssSecondCategoryFormSetTitle( win, FS_FALSE );
			} else {
				FS_Window *win = FS_WindowFindId( FS_W_SnsRssFrm );
				if ( win ) FS_SnsRssFormSetTitle( win, reqCode, FS_FALSE );
			}
			if ( rsp->result != FS_SNS_RSP_SUCCESS ) {
				if ( rsp->err_info ) {
					IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
					FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
				} else {
					FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
				}
			} else {
				FS_MessageBox( FS_MS_OK, FS_Text(FS_T_NO_NEW_DATA), FS_NULL, FS_FALSE );
			}
		}
		FS_SnsUIRequestStop( );
		break;
	case FS_SNS_REQ_RSS_GET_CATEGORY_DETAIL:
		if ( rsp->data.rss_category_detail.count > 0 ) {
			FS_SnsRssShowCategoryDetail( rsp, reqCode, FS_TRUE );
		} else {
			FS_DestroyWindowByID( FS_W_SnsRssCategoryDetailFrm );
			if ( rsp->result != FS_SNS_RSP_SUCCESS ) {
				if ( rsp->err_info ) {
					IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
					FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
				} else {
					FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
				}
			} else {
				FS_MessageBox( FS_MS_OK, FS_Text(FS_T_NO_NEW_DATA), FS_NULL, FS_FALSE );
			}
		}
		FS_SnsUIRequestStop( );
		break;
	case FS_SNS_REQ_RSS_SET_MY_CHANNEL:
		if (rsp->result == FS_SNS_RSP_SUCCESS)
		{
			FS_MessageBox( FS_MS_OK, FS_Text(FS_T_SUCCESS), FS_NULL, FS_FALSE );
			FS_DestroyWindowByID( FS_W_SnsRssCategoryDetailFrm );
		}
		else
		{
			if ( rsp->err_info ) {
				IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
				FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
			} else {
				FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
			}
		}
		FS_SnsUIRequestStop( );
		break;
	case FS_SNS_REQ_EML_SET_ACOUNT:
		if (rsp->result == FS_SNS_RSP_SUCCESS)
		{
			FS_SnsConfig *config = FS_SnsGetConfig( );
			FS_SINT4 i;

			FS_MessageBox( FS_MS_OK, FS_Text(FS_T_SUCCESS), FS_NULL, FS_FALSE );
			
			for (i = 0; i < config->account_num; i ++)
			{
				if (IFS_Stricmp("email", config->accout[i].name) == 0) {
					config->accout[i].bound = FS_TRUE;
					break;
				}
			}
			IFS_PostMessage( FS_MSG_UTIL_CALL, (FS_UINT4)FS_SnsEmlGetAccountPost_CB );
		}
		else
		{
			if ( rsp->err_info ) {
				IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
				FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
			} else {
				FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
			}
		}
		FS_SnsUIRequestStop( );
		break;
	case FS_SNS_REQ_EML_RECV:
		if ( rsp->data.eml_list.count > 0 ) {
			FS_SnsEmlShowInboxList( rsp, reqCode, FS_TRUE );
		} else {
			FS_Window *win = FS_WindowFindId( FS_W_SnsEmailFrm );			
			GFS_SnsApp.eml_page = GFS_SnsApp.backup_page_index;
			FS_SnsEmailFormSetTitle( win, FS_SNS_REQ_EML_RECV, FS_TRUE );
			if ( rsp->result != FS_SNS_RSP_SUCCESS ) {
				if ( rsp->err_info ) {
					IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
					FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
				} else {
					FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
				}
			} else {
				FS_MessageBox( FS_MS_OK, FS_Text(FS_T_NO_NEW_DATA), FS_NULL, FS_FALSE );
			}
		}
		FS_SnsUIRequestStop( );
		break;
	case FS_SNS_REQ_EML_RECV_CONTENT:
		if ( rsp->data.eml_list.count > 0 ) {
			FS_Window *win = FS_WindowFindId( FS_W_SnsEmlDetailFrm );
			FS_Widget *wgt = FS_NULL;
			FS_CHAR *str;
			if ( win ) wgt = FS_WindowGetWidget( win, FS_W_SnsEmlDetailContent );
			if ( wgt ) {
				str = IFS_Strstr( rsp->data.eml_list.emails[0].content, "=--=" );
				if ( str == FS_NULL ) {
					str = rsp->data.eml_list.emails[0].content;
				} else {
					str += 4;
				}
				FS_WidgetSetText( wgt, FS_ProcessParagraph(str, -1) );
			}
		} else {
			if ( rsp->result != FS_SNS_RSP_SUCCESS ) {
				if ( rsp->err_info ) {
					IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
					FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
				} else {
					FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
				}
			} else {
				FS_MessageBox( FS_MS_OK, FS_Text(FS_T_NO_NEW_DATA), FS_NULL, FS_FALSE );
			}
		}
		FS_SnsUIRequestStop( );
		break;
	case FS_SNS_REQ_EML_SEND:
		if (rsp->result == FS_SNS_RSP_SUCCESS)
		{
			FS_Window *win;
			FS_Widget *wgt;
			FS_MessageBox( FS_MS_OK, FS_Text(FS_T_SUCCESS), FS_NULL, FS_FALSE );
			win = FS_WindowFindId( FS_W_SnsEmlReplyFrm );
			if ( win ) {
				FS_DestroyWindow( win );
			} else {			
				win = FS_WindowFindId( FS_W_SnsEmailFrm );
				wgt = FS_WindowGetWidget( win, FS_W_SnsEmlSendTo );
				FS_WidgetSetText( wgt, FS_NULL );
				wgt = FS_WindowGetWidget( win, FS_W_SnsEmlSendSubject );
				FS_WidgetSetText( wgt, FS_NULL );
				wgt = FS_WindowGetWidget( win, FS_W_SnsEmlSendContent );
				FS_WidgetSetText( wgt, FS_NULL );
			}
		}
		else
		{
			if ( rsp->err_info ) {
				IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
				FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
			} else {
				FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
			}
		}
		FS_SnsUIRequestStop( );
		break;
	case FS_SNS_REQ_EML_GET_ACOUNT:
		if ( rsp->data.eml_account.email != FS_NULL ) {
			FS_SnsEmlShowAccount( rsp );
		} else {
			FS_Window *win = FS_WindowFindId( FS_W_SnsEmailFrm );			
			FS_SnsEmailFormSetTitle( win, FS_SNS_REQ_EML_SET_ACOUNT, FS_FALSE );
			if ( rsp->result != FS_SNS_RSP_SUCCESS ) {
				if ( rsp->err_info ) {
					IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
					FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
				} else {
					FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
				}
			} else {
				win->private_data = 1;	/* do not get accout again */
			}
		}
		FS_SnsUIRequestStop( );
		break;
	case FS_SNS_REQ_EML_GET_CONTACT:
		if ( rsp->data.eml_contact.count > 0 ) {
			FS_SnsEmlShowContactList( rsp, reqCode, FS_TRUE );
		} else {
			FS_Window *win;
			FS_Widget *wLabel;
			FS_TabSheet *tabContact;
			GFS_SnsApp.eml_contact_page = GFS_SnsApp.backup_page_index;
			
			win = FS_WindowFindId( FS_W_SnsEmlContactSelectFrm );
			if ( win == FS_NULL ) {
				win = FS_WindowFindId( FS_W_SnsEmailFrm );
			}
			if ( win ) {
				FS_SnsEmailFormSetTitle( win, FS_SNS_REQ_EML_GET_CONTACT, FS_FALSE );
				if ( GFS_SnsApp.eml_contact_page == 1 && win->id == FS_W_SnsEmailFrm ) {
					FS_WindowDelTabSheetWidgetList( win, FS_W_SnsTabEmlContact );
					wLabel =  FS_CreateListItem( FS_W_SnsEmlAddContact, FS_Text(FS_T_ADD_CONTACT), FS_NULL, 0, 1 );
					tabContact = FS_WindowGetTabSheet( win, FS_W_SnsTabEmlContact );
					FS_WidgetSetHandler( wLabel, FS_SnsEmlAddContact_CB );
					FS_TabSheetAddWidget( tabContact, wLabel );
				}
			}
			if ( rsp->result != FS_SNS_RSP_SUCCESS ) {
				if ( rsp->err_info ) {
					IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
					FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
				} else {
					FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
				}
			} else {
				FS_MessageBox( FS_MS_OK, FS_Text(FS_T_NO_NEW_DATA), FS_NULL, FS_FALSE );
			}
		}
		FS_SnsUIRequestStop( );
		break;
	case FS_SNS_REQ_EML_ADD_CONTACT:
	case FS_SNS_REQ_EML_MOD_CONTACT:
	case FS_SNS_REQ_EML_DEL_CONTACT:
		if (rsp->result == FS_SNS_RSP_SUCCESS)
		{
			FS_MessageBox( FS_MS_OK, FS_Text(FS_T_SUCCESS), FS_NULL, FS_FALSE );
			FS_DestroyWindowByID( FS_W_SnsEmlContactEditFrm );

			IFS_PostMessage( FS_MSG_UTIL_CALL, (FS_UINT4)FS_SnsEmlContactPostUpdate_CB );
		}
		else
		{
			if ( rsp->err_info ) {
				IFS_Snprintf( szError, sizeof(szError) - 1, "%s\r\n\r\n%s", FS_Text(FS_T_SERVER_ERR), rsp->err_info );
				FS_StdShowDetail( FS_Text(FS_T_SERVER_ERR), szError );
			} else {
				FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_ERR), FS_NULL, FS_FALSE );
			}
		}
		FS_SnsUIRequestStop( );
		break;
	default:
		break;
	}

	if ( rsp->result == FS_SNS_RSP_ERR_NET_CONN ) {
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_DIAL_FAILED), FS_NULL, FS_FALSE );
	}
}

static void FS_SnsUIRequestTimerCB( void *dummy )
{
	FS_Window *win;
	FS_CHAR szSts[128] = {0};
	FS_SnsReqStatus *sts = FS_SnsLibGetReqStatus(GFS_SnsApp.snslib);
	FS_SINT4 tid;

	GFS_SnsApp.timer_id = 0;
	GFS_SnsApp.counter ++;

	if ( sts->status == FS_SNS_REQ_STS_SEND_REQUEST ) {
		IFS_Snprintf( szSts, sizeof(szSts) - 1, "%d-%s", GFS_SnsApp.counter, FS_Text(FS_T_WEB_REQUEST) );
	} else if ( sts->status == FS_SNS_REQ_STS_WAIT_RESPONSE ) {
		IFS_Snprintf( szSts, sizeof(szSts) - 1, "%d-%s", GFS_SnsApp.counter, FS_Text(FS_T_WAIT_RESPONSE) );
	} else if ( sts->status == FS_SNS_REQ_STS_DOWNLOAD_IMAGE ) {
		IFS_Snprintf( szSts, sizeof(szSts) - 1, "%d-%s(%d/%d)", 
			GFS_SnsApp.counter, FS_Text(FS_T_DOWNLOAD_IMG), sts->index, sts->total );
	} else if ( sts->status == FS_SNS_REQ_STS_UPLOAD_PHOTO ) {
		if ( sts->index >= sts->total ) {
			tid = FS_T_WAIT_RESPONSE;
		} else {
			tid = FS_T_UPLOADING;
		}
		IFS_Snprintf( szSts, sizeof(szSts) - 1, "%d-%s %d%%\r\n\r\n%d/%d KB", 
			GFS_SnsApp.counter, FS_Text(tid), sts->index * 100 /sts->total, 
			FS_KB(sts->index), FS_KB(sts->total) );
	} else {
		IFS_Snprintf( szSts, sizeof(szSts) - 1, "%d-%s", GFS_SnsApp.counter, FS_Text(FS_T_PLS_WAITING) );
	}

	win = FS_WindowFindId( FS_W_SnsRequestingFrm );
	if ( win ) {
		FS_MsgBoxSetText( win, szSts );
		GFS_SnsApp.timer_id = IFS_StartTimer( FS_TIMER_ID_SNSUI, 1000, FS_SnsUIRequestTimerCB, FS_NULL );
	} else {
		win = FS_WindowFindPtr( GFS_SnsApp.cur_req_win );
		if ( win ) {
			FS_WindowSetTitle( win, szSts );
			if ( FS_WindowIsTopMost( win->id ) ) {
				FS_RedrawWinTitle( win );
			}

			GFS_SnsApp.timer_id = IFS_StartTimer( FS_TIMER_ID_SNSUI, 1000, FS_SnsUIRequestTimerCB, FS_NULL );
		}
	}
}

static void FS_SnsUIRequestStart( FS_Window *win, FS_SnsRequest *req, FS_BOOL bCacheOK )
{
	FS_Window *msgBox;
	FS_CHAR text[64] = {0};

	/* cancel request if any */
	FS_SnsLibCancelRequest( GFS_SnsApp.snslib );
	FS_SnsUIRequestStop( );

	GFS_SnsApp.cur_req_win = win;
	if ( FS_SnsLibRequest( GFS_SnsApp.snslib, req ) ) {
		if ( ! bCacheOK ) {
			IFS_Snprintf( text, sizeof(text) - 1, "0-%s", FS_Text(FS_T_PLS_WAITING) );
			msgBox = FS_MessageBox( FS_MS_INFO_CANCEL, text, FS_SnsRequestWinProc, FS_FALSE );
			msgBox->id = FS_W_SnsRequestingFrm;
		} else {
			IFS_Snprintf( text, sizeof(text) - 1, "0-%s", FS_Text(FS_T_PLS_WAITING) );
			FS_WindowSetTitle( win, text );
			FS_RedrawWinTitle( win );
		}
		GFS_SnsApp.counter = 0;
		GFS_SnsApp.timer_id = IFS_StartTimer( FS_TIMER_ID_SNSUI, 1000, FS_SnsUIRequestTimerCB, FS_NULL );
	} else {
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_NET_BUSY), FS_NULL, FS_TRUE );
	}
}

static void FS_SnsUIRequestStop( void )
{
	GFS_SnsApp.cur_req_win = FS_NULL;
	FS_DestroyWindowByID( FS_W_SnsRequestingFrm );
	if ( GFS_SnsApp.timer_id ) {
		IFS_StopTimer( GFS_SnsApp.timer_id );
		GFS_SnsApp.timer_id = 0;
	}
}

static FS_BOOL FS_SnsSysInit( void )
{
	FS_SystemInit( );
	FS_SnsInitConfig( );
	
	GFS_SnsApp.snslib = FS_SnsLibCreate( FS_SnsLibEventCB );
	if (GFS_SnsApp.snslib == FS_NULL)
		return FS_FALSE;
	GFS_SnsApp.update_page = 1;	/* page index count from 1 */
	GFS_SnsApp.reply_page = 1;
	GFS_SnsApp.friends_page = 1;
	GFS_SnsApp.friends_search_page = 1;
	GFS_SnsApp.rss_article_page = 1;
	GFS_SnsApp.eml_page = 1;
	GFS_SnsApp.eml_contact_page = 1;
	GFS_SnsApp.rss_channel_article_page = 1;
	return FS_TRUE;
}

static void FS_SnsExit_CB( FS_Window *win )
{
	if ( GFS_SnsApp.snslib ) {
		FS_SnsLibDestroy( GFS_SnsApp.snslib );

		/* free current msg if any */
		FS_SAFE_FREE( GFS_SnsApp.cur_msg.id );
		FS_SAFE_FREE( GFS_SnsApp.cur_msg.author );
		FS_SAFE_FREE( GFS_SnsApp.cur_msg.date );
		FS_SAFE_FREE( GFS_SnsApp.cur_msg.icon_file );
		FS_SAFE_FREE( GFS_SnsApp.cur_msg.msg );
		FS_SAFE_FREE( GFS_SnsApp.cur_msg.type );

		/* free rsp msg list if any */
		FS_SnsFreeResponse( &GFS_SnsApp.rsp_update, FS_SNS_REQ_GET_UPDATE );
		FS_SnsFreeResponse( &GFS_SnsApp.rsp_msglist, FS_SNS_REQ_GET_UPDATE );
		FS_SnsFreeResponse( &GFS_SnsApp.rsp_reply, FS_SNS_REQ_GET_UPDATE );
		FS_SnsFreeResponse( &GFS_SnsApp.rsp_friends, FS_SNS_REQ_GET_FRIENDS );
		FS_SnsFreeResponse( &GFS_SnsApp.rsp_rss_articles, FS_SNS_REQ_RSS_GET_ALL_ARTICLE );
		FS_SnsFreeResponse( &GFS_SnsApp.rsp_rss_channels, FS_SNS_REQ_RSS_GET_ALL_CHANNEL );
		FS_SnsFreeResponse( &GFS_SnsApp.rsp_rss_channel_articles, FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL );
		FS_SnsFreeResponse( &GFS_SnsApp.rsp_rss_channel_category, FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY );
		FS_SnsFreeResponse( &GFS_SnsApp.rsp_rss_second_category, FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY );
		FS_SnsFreeResponse( &GFS_SnsApp.rsp_rss_category_detail, FS_SNS_REQ_RSS_GET_CATEGORY_DETAIL );
		FS_SnsFreeResponse( &GFS_SnsApp.rsp_eml_inbox, FS_SNS_REQ_EML_RECV );
		FS_SnsFreeResponse( &GFS_SnsApp.rsp_eml_contact, FS_SNS_REQ_EML_RECV );
		FS_SAFE_FREE( GFS_SnsApp.fname );
		IFS_Memset( &GFS_SnsApp, 0, sizeof(GFS_SnsApp) );
		FS_DeactiveApplication( FS_APP_SNS );
		if( ! FS_HaveActiveApplication() )
		{
			FS_GuiExit( );
			IFS_SystemExit( );
		}
	}
}

static void FS_SnsPutTweet_CB( FS_Window *win )
{
	FS_Widget *wEdit;
	FS_SnsRequest req = {0};
	FS_Window *swin;
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_SINT4 i, len = 0;
	FS_CHAR szText[128] = {0};
	FS_Widget *wgt;

	swin = FS_WindowFindId( FS_W_SnsRssForwardFrm );
	if ( swin == FS_NULL ) {
		swin = FS_WindowFindId( FS_W_SnsEntryFrm );
	}
	wEdit = FS_WindowGetWidget( swin, FS_W_SnsComposeTabEdit );
	if ( wEdit == FS_NULL || wEdit->text == FS_NULL || wEdit->text[0] == 0 ) {
		FS_MessageBox( FS_MS_OK, FS_Text(FS_T_MSG_CANNOT_EMPTY), FS_NULL, FS_FALSE );
		return;
	}

	if ( GFS_SnsApp.fname && GFS_SnsApp.fsize > 0 ) {
		IFS_Snprintf( szText, sizeof(szText) - 1, FS_Text(FS_T_UPLOAD_CONFIRM), FS_KB(GFS_SnsApp.fsize) );
		FS_MessageBox( FS_MS_YES_NO, szText, FS_SnsPhotoUploadCnf_CB, FS_FALSE );
		return;
	}
	
	req.req = FS_SNS_REQ_SET_TWEET;
	req.data.set_tweet.type = (FS_CHAR *)swin->private_data;
	req.data.set_tweet.msg = wEdit->text;
	for ( i = 0; i < config->account_num; i ++ ) {
		wgt = FS_WindowGetWidget( swin, FS_W_SnsComposeTabCheck + i );
		if ( wgt && FS_WGT_GET_CHECK(wgt) ) {
			len = IFS_Strlen( szText );
			if ( len != 0 ) {
				IFS_Strcat( szText, "," );
			}
			len = IFS_Strlen( szText );
			IFS_Strncpy( szText + len, (FS_CHAR *)wgt->private_data, sizeof(szText) - len - 1 );
		}
	}
	if ( szText[0] ) {
		req.data.set_tweet.sync_to = szText;
	}

	FS_SnsUIRequestStart( win, &req, FS_FALSE );
}

static FS_BOOL FS_SnsIsEntryBound( FS_CHAR *name )
{
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_SINT4 i;

	for (i = 0; i < config->account_num; i ++)
	{
		if (IFS_Stricmp(name, config->accout[i].name) == 0)
			return config->accout[i].bound;
	}
	return FS_FALSE;
}

static void FS_SnsMsgReply_CB( FS_Window *win )
{
	FS_SnsRequest req = {0};
	FS_Window *swin, *dwin;
	FS_Widget *wEdit;

	swin = FS_WindowFindId( FS_W_SnsEntryFrm );
	if ( swin == FS_NULL ) {
		swin = FS_WindowFindId( FS_W_SnsMsgListFrm );
	}
	if ( swin == FS_NULL ) {
		return;
	}
	
	dwin = FS_WindowFindId( FS_W_SnsMsgDetailFrm );
	wEdit = FS_WindowGetWidget( dwin, FS_W_SnsComposeTabEdit );
	if ( wEdit == FS_NULL ) {
		return;
	}

	if ( wEdit->text == FS_NULL || wEdit->text[0] == 0 ) {
		FS_MessageBox( FS_MS_OK, FS_Text(FS_T_MSG_CANNOT_EMPTY), FS_NULL, FS_FALSE );
		return;
	}
	
	req.req = FS_SNS_REQ_REPLY;
	if ( GFS_SnsApp.cur_msg.type ) {
		req.data.reply.type = GFS_SnsApp.cur_msg.type;
	}
	else {
		req.data.reply.type = (FS_CHAR *)swin->private_data;
	}
	req.data.reply.msg = wEdit->text;
	req.data.reply.id = GFS_SnsApp.cur_msg.id;
	
	FS_SnsUIRequestStart( win, &req, FS_FALSE );	
}

static void FS_SnsMsgShowDetail_UI( FS_BOOL bReply )
{
	FS_Widget *wAuthor, *wTweet, *wEdit = FS_NULL;
	FS_Window *dwin;
	FS_CHAR szStr[FS_MAX_PATH_LEN] = {0};
	FS_EditParam eParam = { FS_IM_CHI, FS_IM_ALL, 140 };
	FS_SnsConfig *config = FS_SnsGetConfig( );

	eParam.limit_character = FS_TRUE;

	dwin = FS_WindowFindId( FS_W_SnsMsgDetailFrm );
	if ( dwin == FS_NULL ) {
		dwin = FS_CreateWindow( FS_W_SnsMsgDetailFrm, FS_Text(FS_T_DETAIL), FS_NULL );
	} else {
		FS_WindowDelWidgetList( dwin );
	}

	wAuthor = FS_CreateListItem( FS_W_SnsComposeTabAuthor, GFS_SnsApp.cur_msg.author, 
		FS_SnsUtilSecondsToDateTime(GFS_SnsApp.cur_msg.date), 0, 2 ); 
	if ( config->display_image && GFS_SnsApp.cur_msg.icon_file ) {
		FS_GetAbsFileName( FS_DIR_TMP, GFS_SnsApp.cur_msg.icon_file, szStr );
		FS_WidgetSetIconFile( wAuthor, szStr );
	}
	wTweet = FS_CreateScroller( FS_W_SnsComposeTabTweet, GFS_SnsApp.cur_msg.msg );
	if ( bReply ) {
		wEdit = FS_CreateEditBox( FS_W_SnsComposeTabEdit, FS_NULL, 0, 2, &eParam );
		FS_WGT_SET_FORCE_MULTI_LINE( wEdit );
		FS_WGT_SET_FORCE_NO_STATUS_BAR( wEdit );
		FS_WindowAddWidget( dwin, wEdit );

		IFS_Memset( szStr, 0, sizeof(szStr) );
		szStr[0] = '@';
		IFS_Strncpy( szStr + 1, GFS_SnsApp.cur_msg.author, sizeof(szStr) - 4 );
		IFS_Strcat( szStr, ": " );
		FS_WidgetSetText( wEdit, szStr );
	}

	FS_WindowAddWidget( dwin, wAuthor );
	FS_WindowAddWidget( dwin, wTweet );
	
	if ( bReply ) {
		FS_WindowSetSoftkey( dwin, 1, FS_Text(FS_T_SEND), FS_SnsMsgReply_CB );
		FS_WindowSetSoftkey( dwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	} else {
		FS_WindowSetSoftkey( dwin, 1, FS_Text(FS_T_MENU), FS_SnsMsgDetailMenu_UI );
		FS_WindowSetSoftkey( dwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	}
	FS_ShowWindow( dwin );
}

static void FS_SnsMsgListViewDetail_UI( FS_Window *win )
{
	FS_Widget *wItem;
	FS_Window *lwin;
	FS_UINT4 idx;
	FS_CHAR absFile[FS_MAX_PATH_LEN] = {0};
	FS_SnsResponse *rsp;

	lwin = FS_WindowFindId( FS_W_SnsEntryFrm );
	if ( lwin == FS_NULL ) {
		lwin = FS_WindowFindId( FS_W_SnsMsgListFrm );
	}
	if ( lwin == FS_NULL ) {
		goto ERR_CATCH;
	}

	wItem = FS_WindowGetFocusItem( lwin );
	if ( wItem == FS_NULL ) {
		goto ERR_CATCH;
	}
	if ( lwin->focus_sheet && lwin->focus_sheet->id == FS_W_SnsTabUpdate ) {
		rsp = &GFS_SnsApp.rsp_update;
	} else if ( lwin->focus_sheet && lwin->focus_sheet->id == FS_W_SnsTabReply ) {
		rsp = &GFS_SnsApp.rsp_reply;
	} else {
		rsp = &GFS_SnsApp.rsp_msglist;
	}
	idx = wItem->private_data;
	if (idx >= FS_SNS_MAX_MSGS_COUNT) {
		goto ERR_CATCH;
	}

	/* save to current msg for further use */
	FS_COPY_TEXT( GFS_SnsApp.cur_msg.id, rsp->data.msglist.msgs[idx].id );
	FS_COPY_TEXT( GFS_SnsApp.cur_msg.author, rsp->data.msglist.msgs[idx].author );
	FS_COPY_TEXT( GFS_SnsApp.cur_msg.date, rsp->data.msglist.msgs[idx].date );
	FS_COPY_TEXT( GFS_SnsApp.cur_msg.icon_file, rsp->data.msglist.msgs[idx].icon_file );
	FS_COPY_TEXT( GFS_SnsApp.cur_msg.msg, rsp->data.msglist.msgs[idx].msg );
	FS_COPY_TEXT( GFS_SnsApp.cur_msg.type, rsp->data.msglist.msgs[idx].type );

	FS_SnsMsgShowDetail_UI( FS_FALSE );

ERR_CATCH:
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_SnsMsgListNextPage_CB( FS_Window *win )
{
	FS_SnsReqCode reqCode = FS_SNS_REQ_GET_UPDATE;
	FS_Window *lwin = FS_WindowFindId( FS_W_SnsEntryFrm );
	
	if ( lwin && lwin->focus_sheet && lwin->focus_sheet->id == FS_W_SnsTabReply ) {
		reqCode = FS_SNS_REQ_GET_REPLY;
		GFS_SnsApp.reply_page ++;
	} else if ( lwin && lwin->focus_sheet && lwin->focus_sheet->id == FS_W_SnsTabUpdate ) {
		reqCode = FS_SNS_REQ_GET_UPDATE;
		GFS_SnsApp.update_page ++;
	} else {
		lwin = FS_WindowFindId( FS_W_SnsMsgListFrm );
		GFS_SnsApp.msg_page ++;
	}
	
	if ( lwin == FS_NULL ) return;

	FS_SnsEntryGetMsgs( lwin, reqCode, FS_TRUE );
	
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU ) )
		FS_DestroyWindow( win );
}

static void FS_SnsMsgListPrevPage_CB( FS_Window *win )
{
	FS_SnsReqCode reqCode = FS_SNS_REQ_GET_UPDATE;
	FS_Window *lwin = FS_WindowFindId( FS_W_SnsEntryFrm );
	FS_BOOL bUpdate = FS_FALSE;

	if ( lwin && lwin->focus_sheet && lwin->focus_sheet->id == FS_W_SnsTabReply ) {
		reqCode = FS_SNS_REQ_GET_REPLY;
		if ( GFS_SnsApp.reply_page > 1 ) {
			GFS_SnsApp.reply_page --;
			bUpdate = FS_TRUE;
		}
	} else if ( lwin && lwin->focus_sheet && lwin->focus_sheet->id == FS_W_SnsTabUpdate ) {
		reqCode = FS_SNS_REQ_GET_UPDATE;
		if ( GFS_SnsApp.update_page > 1 ) {
			GFS_SnsApp.update_page --;
			bUpdate = FS_TRUE;
		}
	} else {
		lwin = FS_WindowFindId( FS_W_SnsMsgListFrm );
		if ( lwin && GFS_SnsApp.msg_page > 1 ) {
			GFS_SnsApp.msg_page --;
			bUpdate = FS_TRUE;
		}
	}

	if ( bUpdate ) {
		FS_SnsEntryGetMsgs( lwin, reqCode, FS_TRUE );
	}

	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_SnsMsgRetweet_CB( FS_Window *win )
{
	FS_SnsRequest req = {0};
	FS_Window *swin;
	
	swin = FS_WindowFindId( FS_W_SnsEntryFrm );
	if ( swin == FS_NULL ) {
		swin = FS_WindowFindId( FS_W_SnsMsgListFrm );
	}
	if ( swin == FS_NULL ) {
		goto ERR_CATCH;
	}
		
	req.req = FS_SNS_REQ_RETWEET;
	if ( GFS_SnsApp.cur_msg.type ) {
		req.data.reply.type = GFS_SnsApp.cur_msg.type;
	} else {
		req.data.reply.type = (FS_CHAR *)swin->private_data;
	}
	req.data.reply.id = GFS_SnsApp.cur_msg.id;
	
	FS_SnsUIRequestStart( win, &req, FS_FALSE );

ERR_CATCH:
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_SnsMsgSetLike_CB( FS_Window *win )
{
	FS_SnsRequest req = {0};
	FS_Window *swin;
	
	swin = FS_WindowFindId( FS_W_SnsEntryFrm );
	if ( swin == FS_NULL ) {
		swin = FS_WindowFindId( FS_W_SnsMsgListFrm );
	}
	if ( swin == FS_NULL ) {
		goto ERR_CATCH;
	}
	
	req.req = FS_SNS_REQ_SET_LIKE;
	if ( GFS_SnsApp.cur_msg.type ) {
		req.data.reply.type = GFS_SnsApp.cur_msg.type;
	} else {
		req.data.reply.type = (FS_CHAR *)swin->private_data;
	}
	req.data.reply.id = GFS_SnsApp.cur_msg.id;
	
	FS_SnsUIRequestStart( win, &req, FS_FALSE );
	
ERR_CATCH:
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_SnsMsgReply_UI( FS_Window *win )
{
	FS_SnsMsgShowDetail_UI( FS_TRUE );

	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_SnsMsgDetailMenu_UI( FS_Window *win )
{
	FS_Widget *iLike = FS_NULL, *iFavorite = FS_NULL, *iForward = FS_NULL, *iRetweet = FS_NULL, *iReply = FS_NULL;
	FS_Window *pMenu;
	FS_CHAR *type;
	FS_Window *mwin = FS_WindowFindId( FS_W_SnsEntryFrm );
	FS_SINT4 count;

	if ( mwin == FS_NULL ) mwin = FS_WindowFindId( FS_W_SnsMsgListFrm );
	if ( mwin == FS_NULL ) return;

	type = GFS_SnsApp.cur_msg.type;
	if ( type == FS_NULL ) type = (FS_CHAR *)mwin->private_data;

	if ( FS_STR_I_EQUAL(type, "twitter") ) {
		count = 3;
		iFavorite = FS_CreateMenuItem( 0, FS_Text(FS_T_FAVORITE) );
		iRetweet = FS_CreateMenuItem( 0, FS_Text(FS_T_RETWEET) );
		iReply = FS_CreateMenuItem( 0, FS_Text(FS_T_REPLY) );
	} else if ( FS_STR_I_EQUAL(type, "facebook") ) {
		count = 2;
		iLike = FS_CreateMenuItem( 0, FS_Text(FS_T_LIKE) );
		iReply = FS_CreateMenuItem( 0, FS_Text(FS_T_REPLY) );
	} else {	/* sina and isync and default */
		count = 3;
		iFavorite = FS_CreateMenuItem( 0, FS_Text(FS_T_FAVORITE) );
		iForward = FS_CreateMenuItem( 0, FS_Text(FS_T_FORWARD) );
		iReply = FS_CreateMenuItem( 0, FS_Text(FS_T_REPLY) );
	}
	
	pMenu = FS_CreateMenu( 0, count );
	if( iFavorite ) {
		FS_MenuAddItem( pMenu, iFavorite );
		FS_WidgetSetHandler( iFavorite, FS_SnsMsgSetLike_CB );
	}
	if( iForward ) {
		FS_MenuAddItem( pMenu, iForward );
		FS_WidgetSetHandler( iForward, FS_SnsMsgRetweet_CB );
	}
	if( iRetweet ) {
		FS_MenuAddItem( pMenu, iRetweet );
		FS_WidgetSetHandler( iRetweet, FS_SnsMsgRetweet_CB );
	}
	if( iLike ) {
		FS_MenuAddItem( pMenu, iLike );
		FS_WidgetSetHandler( iLike, FS_SnsMsgSetLike_CB );
	}
	if( iReply ) {
		FS_MenuAddItem( pMenu, iReply );
		FS_WidgetSetHandler( iReply, FS_SnsMsgReply_UI );
	}
	
	FS_MenuSetSoftkey( pMenu );
	
	FS_ShowWindow( pMenu );
}

static void FS_SnsPhotoUpload_CB( void )
{
	FS_Window *swin = FS_WindowFindId( FS_W_SnsEntryFrm );
	FS_Widget *wgt = FS_WindowGetWidget( swin, FS_W_SnsComposeTabEdit );
	FS_SnsRequest req = {0};
	FS_CHAR szText[128] = {0};
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_SINT4 i, len;

	if ( swin == FS_NULL || wgt == FS_NULL ) return;

	req.req = FS_SNS_REQ_UPLOAD_PHOTO;

	req.data.photo.type = (FS_CHAR *)swin->private_data;
	req.data.photo.msg = wgt->text;
	req.data.photo.fname = GFS_SnsApp.fname;
	req.data.photo.fsize = GFS_SnsApp.fsize;
	for ( i = 0; i < config->account_num; i ++ ) {
		wgt = FS_WindowGetWidget( swin, FS_W_SnsComposeTabCheck + i );
		if ( wgt && FS_WGT_GET_CHECK(wgt) ) {
			len = IFS_Strlen( szText );
			if ( len != 0 ) {
				IFS_Strcat( szText, "," );
			}
			len = IFS_Strlen( szText );
			IFS_Strncpy( szText + len, (FS_CHAR *)wgt->private_data, sizeof(szText) - len - 1 );
		}
	}
	if ( szText[0] ) {
		req.data.photo.sync_to = szText;
	}

	FS_SnsUIRequestStart( swin, &req, FS_FALSE );
}

static FS_BOOL FS_SnsPhotoUploadCnf_CB( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	if( wparam == FS_EV_YES )	// exit without save account data
	{
		IFS_PostMessage( FS_MSG_UTIL_CALL, (FS_UINT4)FS_SnsPhotoUpload_CB );
		ret = FS_TRUE;
	}
	return ret;
}

static void FS_SnsPhotoComment_CB( FS_CHAR * text, FS_SINT4 len, void *param )
{
	FS_Window *win = FS_WindowFindId( FS_W_SnsEntryFrm );
	FS_Widget *wgt;

	if ( win == FS_NULL ) return;
	wgt = FS_WindowGetWidget( win, FS_W_SnsComposeTabEdit );
	if ( wgt == FS_NULL ) return;

	FS_WidgetSetText( wgt, text );
}

static void FS_SnsPhotoSelected_CB( FS_CHAR *fname, void *dummy )
{
	FS_Window *win = FS_WindowFindId( FS_W_SnsEntryFrm );
	FS_Widget *wgt;
	FS_SINT4 size = 0;
	FS_CHAR szText[32] = {0};
	FS_EditParam eParam = { FS_IM_CHI, FS_IM_ALL, 140 };

	if ( win == FS_NULL ) return;
	wgt = FS_WindowGetWidget( win, FS_W_SnsComposeTabImage );
	if ( wgt == FS_NULL ) return;

	if ( fname && fname[0] ) {
		size = FS_FileGetSize( -1, fname );
	}
	if ( size >= 0 )
	{
		FS_COPY_TEXT( GFS_SnsApp.fname, fname );
		GFS_SnsApp.fsize = size;
		FS_WidgetSetText( wgt, FS_GetFileNameFromPath(fname) );
		IFS_Snprintf( szText, sizeof(szText) - 1, "%dKB", FS_KB(size) );
		FS_WidgetSetExtraText( wgt, szText );

		wgt = FS_WindowGetWidget( win, FS_W_SnsComposeTabEdit );
		if ( wgt->text == FS_NULL || wgt->text[0] == 0 ){
			IFS_InputDialog( FS_Text(FS_T_INPUT_PHOTO_COMMENT), &eParam, FS_SnsPhotoComment_CB, FS_NULL );
		} else {
			IFS_InputDialog( wgt->text, &eParam, FS_SnsPhotoComment_CB, FS_NULL );
		}
	}
}

static void FS_SnsPhotoNew_CB( FS_Window *win )
{
	IFS_CameraNewPhoto( FS_SnsPhotoSelected_CB, FS_NULL );
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_SnsPhotoSelect_CB( FS_Window *win )
{
	IFS_FileDialogOpen( FS_FDO_IMAGE, FS_SnsPhotoSelected_CB, FS_NULL );
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_SnsDelPhoto_CB( FS_Window *win )
{
	FS_Window *ewin = FS_WindowFindId( FS_W_SnsEntryFrm );
	FS_Widget *wgt;

	FS_SAFE_FREE( GFS_SnsApp.fname );
	GFS_SnsApp.fsize = 0;

	if ( ewin == FS_NULL ) return;
	wgt = FS_WindowGetWidget( ewin, FS_W_SnsComposeTabImage );
	if ( wgt == FS_NULL ) return;
	FS_WidgetSetText( wgt, FS_Text(FS_T_PHOTO) );
	FS_WidgetSetExtraText( wgt, FS_NULL );

	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_SnsPhotoMenu_UI( FS_Window *win )
{
	FS_Widget *iNewPhoto, *iSelectPhoto, *iDelPhoto = FS_NULL;
	FS_Window *pMenu;
	FS_Widget *wgt;
	FS_Rect rect;
	
	wgt = FS_WindowGetWidget( win, FS_W_SnsComposeTabImage );
	if ( wgt == FS_NULL ) return;
	rect = FS_GetWidgetDrawRect( wgt );

	if ( GFS_SnsApp.fname && GFS_SnsApp.fsize > 0 ) {
		pMenu = FS_CreatePopUpMenu( 0, &rect, 3 );
		iDelPhoto = FS_CreateMenuItem( 0, FS_Text(FS_T_DEL_IMAGE) );
	} else {
		pMenu = FS_CreatePopUpMenu( 0, &rect, 2 );
	}
	
	iNewPhoto = FS_CreateMenuItem( 0, FS_Text(FS_T_NEW_PHOTO) );
	iSelectPhoto = FS_CreateMenuItem( 0, FS_Text(FS_T_SELECT_PHOTO) );
	
	FS_MenuAddItem( pMenu, iNewPhoto );
	FS_MenuAddItem( pMenu, iSelectPhoto );
	if ( iDelPhoto ) 
	{
		FS_MenuAddItem( pMenu, iDelPhoto );
		FS_WidgetSetHandler( iDelPhoto, FS_SnsDelPhoto_CB );
	}

	FS_WidgetSetHandler( iNewPhoto, FS_SnsPhotoNew_CB );
	FS_WidgetSetHandler( iSelectPhoto, FS_SnsPhotoSelect_CB );
	
	FS_MenuSetSoftkey( pMenu );
	
	FS_ShowWindow( pMenu );
}

static void FS_SnsMsgListUpdate_CB( FS_Window *win )
{
	FS_SnsReqCode reqCode = FS_SNS_REQ_GET_UPDATE;
	FS_Window *lwin = FS_WindowFindId( FS_W_SnsEntryFrm );
	
	if ( lwin && lwin->focus_sheet && lwin->focus_sheet->id == FS_W_SnsTabReply ) {
		reqCode = FS_SNS_REQ_GET_REPLY;
		GFS_SnsApp.reply_page = 1;
	} else if ( lwin && lwin->focus_sheet && lwin->focus_sheet->id == FS_W_SnsTabUpdate ){
		reqCode = FS_SNS_REQ_GET_UPDATE;	
		GFS_SnsApp.update_page = 1;
	} else {
		lwin = FS_WindowFindId( FS_W_SnsMsgListFrm );
		if ( lwin ) {
			GFS_SnsApp.msg_page = 1;
		} else {
			return;
		}
	}
	
	FS_SnsEntryGetMsgs( lwin, reqCode, FS_FALSE );

	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_SnsFriendsListUpdate( FS_BOOL bCacheFirst )
{
	FS_SnsRequest req = {0};
	FS_SnsResponse rsp = {0};
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_Window *mwin = FS_WindowFindId( FS_W_SnsEntryFrm );
	FS_Widget *wItem;
	FS_BOOL bCacheOK = FS_FALSE;

	if ( mwin == FS_NULL ) return;
	wItem = FS_WindowGetWidget( mwin, FS_W_SnsFriendsTabEdit );
	FS_WidgetSetText( wItem, FS_NULL );
	
	req.req = FS_SNS_REQ_GET_FRIENDS;
	req.data.friends.type = (FS_CHAR *)mwin->private_data;
	req.data.friends.page = GFS_SnsApp.friends_page;
	req.data.friends.count = config->msg_cnt_per_page;
	
	if ( bCacheFirst ) {
		bCacheOK = FS_SnsLibReadCache( GFS_SnsApp.snslib, &req, &rsp );
		if ( bCacheOK ) {
			FS_SnsEntryShowFriendsList( &rsp, req.req, FS_TRUE );
			return;
		}
	} else {
		FS_SnsLibClearCache( GFS_SnsApp.snslib, &req );
	}
	FS_SnsUIRequestStart( mwin, &req, FS_TRUE );
}

static void FS_SnsFriendsListUpdate_CB( FS_Window *win )
{
	GFS_SnsApp.friends_page = 1;
	GFS_SnsApp.backup_page_index = GFS_SnsApp.friends_page;
	FS_SnsFriendsListUpdate( FS_FALSE );

	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_SnsFriendsListNextPage_CB( FS_Window *win )
{
	GFS_SnsApp.backup_page_index = GFS_SnsApp.friends_page;
	GFS_SnsApp.friends_page ++;
	FS_SnsFriendsListUpdate( FS_TRUE );
	
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_SnsFriendsListPrevPage_CB( FS_Window *win )
{
	if ( GFS_SnsApp.friends_page > 1 )
	{
		GFS_SnsApp.backup_page_index = GFS_SnsApp.friends_page;
		GFS_SnsApp.friends_page --;
		FS_SnsFriendsListUpdate( FS_TRUE );
	}
	
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_SnsEntryFormSetTitle( FS_Window *win, FS_SnsReqCode reqCode, FS_BOOL bUpdateScreen )
{
	FS_Window *lwin;
	FS_Widget *wgt;
	FS_CHAR *szTitle;
	FS_CHAR szPage[16] = {0};

	lwin = FS_WindowFindId( FS_W_SnsListFrm );
	wgt = FS_WindowGetFocusItem( lwin );
	if ( wgt == FS_NULL ) {
		lwin = FS_WindowFindId( FS_W_SnsMainMenuFrm );
		wgt = FS_WindowGetFocusItem( lwin );
	}
	if ( wgt == FS_NULL ) return;

	lwin = FS_WindowFindId( FS_W_SnsMsgListFrm );

	if ( reqCode == FS_SNS_REQ_GET_UPDATE ) {
		if ( lwin == FS_NULL ) {
			IFS_Snprintf( szPage, sizeof(szPage) - 1, "(%d)", GFS_SnsApp.update_page );
			if ( FS_STR_I_EQUAL((FS_CHAR *)win->private_data, "facebook") ) {
				szTitle = FS_StrConCat( wgt->text, "-", FS_Text(FS_T_NEWS_FEED), szPage );
			} else if ( FS_STR_I_EQUAL((FS_CHAR *)win->private_data, "twitter") ) {
				szTitle = FS_StrConCat( wgt->text, "-", FS_Text(FS_T_TIMELINE), szPage );
			} else {
				szTitle = FS_StrConCat( wgt->text, "-", FS_Text(FS_T_UPDATE), szPage );
			}
		} else {
			IFS_Snprintf( szPage, sizeof(szPage) - 1, "(%d)", GFS_SnsApp.msg_page );
			szTitle = FS_StrConCat( FS_Text(FS_T_ISYNC), "-", FS_Text(FS_T_UPDATE), szPage );
		}
	} else if ( reqCode == FS_SNS_REQ_GET_REPLY ) {
		if ( lwin == FS_NULL ) {
			IFS_Snprintf( szPage, sizeof(szPage) - 1, "(%d)", GFS_SnsApp.reply_page );
			if ( FS_STR_I_EQUAL((FS_CHAR *)win->private_data, "sina") ) {
				szTitle = FS_StrConCat( wgt->text, "-", FS_Text(FS_T_ME), szPage );
			} else if ( FS_STR_I_EQUAL((FS_CHAR *)win->private_data, "facebook") ) {
				szTitle = FS_StrConCat( wgt->text, "-", FS_Text(FS_T_WALL), szPage );
			} else if ( FS_STR_I_EQUAL((FS_CHAR *)win->private_data, "twitter") ) {
				szTitle = FS_StrConCat( wgt->text, "-", FS_Text(FS_T_MENTIONS), szPage );
			} else {
				szTitle = FS_StrConCat( wgt->text, "-", FS_Text(FS_T_SNS_REPLY), szPage );
			}
		} else {
			IFS_Snprintf( szPage, sizeof(szPage) - 1, "(%d)", GFS_SnsApp.msg_page );
			szTitle = FS_StrConCat( FS_Text(FS_T_ISYNC), "-", FS_Text(FS_T_SNS_REPLY), szPage );
		}
	} else if ( reqCode == FS_SNS_REQ_GET_TWEET ) {
		szTitle = FS_StrConCat( wgt->text, "-", FS_Text(FS_T_COMPOSE), FS_NULL );
	} else if ( reqCode == FS_SNS_REQ_GET_FRIENDS ) {
		IFS_Snprintf( szPage, sizeof(szPage) - 1, "(%d)", GFS_SnsApp.friends_page );
		szTitle = FS_StrConCat( wgt->text, "-", FS_Text(FS_T_FRIENDS), szPage );
	} else if ( reqCode == FS_SNS_REQ_FIND_FRIENDS ) {
		IFS_Snprintf( szPage, sizeof(szPage) - 1, "(%d)", GFS_SnsApp.friends_search_page );
		szTitle = FS_StrConCat( wgt->text, "-", FS_Text(FS_T_FIND_FRIEND), szPage );
	} else {
		return;
	}
	
	FS_WindowSetTitle( win, szTitle );
	FS_SAFE_FREE( szTitle );
	if ( bUpdateScreen ) {
		FS_RedrawWinTitle( win );
	}
}

static void FS_SnsEntryShowMsgList( FS_SnsResponse *rsp, FS_SnsReqCode reqCode, FS_BOOL bUpdateScreen )
{
	FS_CHAR absFile[FS_MAX_PATH_LEN] = {0};
	FS_TabSheet *sheet = FS_NULL;
	FS_SINT4 i;
	FS_Widget *wItem, *wNextPage = FS_NULL, *wPrevPage = FS_NULL;
	FS_Window *win = FS_WindowFindId( FS_W_SnsEntryFrm );
	FS_UINT4 tab_id = 0, pageIdx;
	FS_CHAR szText[128] = {0};
	FS_BOOL bUpdateSnsEntryFrm = FS_TRUE;
	FS_SnsConfig *config = FS_SnsGetConfig( );

	if ( win == FS_NULL ) {
		win = FS_WindowFindId( FS_W_SnsMsgListFrm );
		bUpdateSnsEntryFrm = FS_FALSE;
	}
	if ( win == FS_NULL ) return;

	if ( bUpdateSnsEntryFrm ) {
		if ( reqCode == FS_SNS_REQ_GET_UPDATE ) {
			tab_id = FS_W_SnsTabUpdate;
			pageIdx = GFS_SnsApp.update_page;
			FS_SnsFreeResponse( &GFS_SnsApp.rsp_update, FS_SNS_REQ_GET_UPDATE );
			IFS_Memcpy( &GFS_SnsApp.rsp_update, rsp, sizeof(FS_SnsResponse) );
		} else if ( reqCode == FS_SNS_REQ_GET_REPLY ) {
			tab_id = FS_W_SnsTabReply;
			pageIdx = GFS_SnsApp.reply_page;
			FS_SnsFreeResponse( &GFS_SnsApp.rsp_reply, FS_SNS_REQ_GET_REPLY );
			IFS_Memcpy( &GFS_SnsApp.rsp_reply, rsp, sizeof(FS_SnsResponse) );
		} else {
			return;
		}
	} else {
		pageIdx = GFS_SnsApp.msg_page;
		FS_SnsFreeResponse( &GFS_SnsApp.rsp_msglist, FS_SNS_REQ_GET_REPLY );
		IFS_Memcpy( &GFS_SnsApp.rsp_msglist, rsp, sizeof(FS_SnsResponse) );
	}
	
	FS_SnsEntryFormSetTitle( win, reqCode, bUpdateScreen );

	if ( bUpdateSnsEntryFrm ) {
		FS_WindowDelTabSheetWidgetList( win, tab_id );
		sheet = FS_WindowGetTabSheet( win, tab_id );
	} else {
		FS_WindowDelWidgetList( win );
	}

	for ( i = 0; i < rsp->data.msglist.count; i ++ ) {
		if ( rsp->data.msglist.msgs[i].type ) {
			IFS_Snprintf( szText, sizeof(szText) - 1, "[%s] %s", 
				rsp->data.msglist.msgs[i].type, rsp->data.msglist.msgs[i].author );
		} else {
			IFS_Strncpy( szText, rsp->data.msglist.msgs[i].author, sizeof(szText) - 1 );
		}
		if ( rsp->data.msglist.msgs[i].msg && rsp->data.msglist.msgs[i].msg[0] != 0 ) {
			wItem = FS_CreateListItem( i, szText, rsp->data.msglist.msgs[i].msg, 0, 2 );
		} else {
			wItem = FS_CreateListItem( i, szText, FS_SnsUtilSecondsToDateTime(rsp->data.msglist.msgs[i].date), 0, 2 );
		}
		
		wItem->private_data = (FS_UINT4)i;
		if ( FS_STR_I_EQUAL(rsp->data.msglist.msgs[i].type, "rss") ) {
			FS_WidgetSetHandler( wItem, FS_SnsRssArticleDetailMore_CB );
		} else if ( FS_STR_I_EQUAL(rsp->data.msglist.msgs[i].type, "email") ) {
			FS_WidgetSetHandler( wItem, FS_SnsEmlDetail_UI );
		} else {
			FS_WidgetSetHandler( wItem, FS_SnsMsgListViewDetail_UI );
		}
		if ( config->display_image && rsp->data.msglist.msgs[i].icon_file ) {
			FS_GetAbsFileName( FS_DIR_TMP, rsp->data.msglist.msgs[i].icon_file, absFile );
			FS_WidgetSetIconFile( wItem, absFile );
		}
		if ( bUpdateSnsEntryFrm && sheet ) {
			FS_TabSheetAddWidget( sheet, wItem );
		} else {
			FS_WindowAddWidget( win, wItem );
		}
	}
	
	/* add page up/down widget */
	if ( rsp->data.msglist.count >= config->msg_cnt_per_page ) {
		wNextPage = FS_CreateListItem( 0, FS_Text(FS_T_NEXT_PAGE), FS_NULL, 0, 1 );
		FS_WidgetSetHandler( wNextPage, FS_SnsMsgListNextPage_CB );
		if ( bUpdateSnsEntryFrm && sheet ) {
			FS_TabSheetAddWidget( sheet, wNextPage );
		} else {
			FS_WindowAddWidget( win, wNextPage );
		}
	}
	if ( pageIdx > 1 ) {
		wPrevPage = FS_CreateListItem( 0, FS_Text(FS_T_PREV_PAGE), FS_NULL, 0, 1 );
		if ( wNextPage ) FS_WGT_SET_SHARE_HEIGHT( wPrevPage );
		FS_WidgetSetHandler( wPrevPage, FS_SnsMsgListPrevPage_CB );
		if ( bUpdateSnsEntryFrm && sheet ) {
			FS_TabSheetAddWidget( sheet, wPrevPage );
		} else {
			FS_WindowAddWidget( win, wPrevPage );
		}
	}

	if ( bUpdateScreen ) {
		FS_InvalidateRect( win, &win->client_rect );
	}

	/* avoid been free by SnsLib */
	IFS_Memset( rsp, 0, sizeof(FS_SnsResponse) );
}

static void FS_SnsFriendsFind( FS_BOOL bCacheFirst )
{
	FS_SnsRequest req = {0};
	FS_SnsResponse rsp = {0};
	FS_Window *mwin = FS_WindowFindId( FS_W_SnsEntryFrm );
	FS_Widget *wgt;
	FS_BOOL bCacheOK = FS_FALSE;
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_CHAR *szKey;

	wgt = FS_WindowGetWidget( mwin, FS_W_SnsFriendsTabEdit );
	if ( mwin == FS_NULL || wgt == FS_NULL ) return;
	szKey = wgt->text;
	if ( szKey ) FS_TrimRight( szKey, -1 );
	req.req = FS_SNS_REQ_FIND_FRIENDS;
	req.data.friends.page = GFS_SnsApp.friends_search_page;
	req.data.friends.count = config->msg_cnt_per_page;
	req.data.friends.type = (FS_CHAR *)mwin->private_data;
	req.data.friends.key = szKey;

	if ( bCacheFirst ) {
		bCacheOK = FS_SnsLibReadCache( GFS_SnsApp.snslib, &req, &rsp );
		if ( bCacheOK ) {
			FS_SnsEntryShowFriendsList( &rsp, req.req, FS_TRUE );
			return;
		}
	} else {
		FS_SnsLibClearCache( GFS_SnsApp.snslib, &req );
	}
	
	FS_SnsUIRequestStart( mwin, &req, FS_FALSE );
}

static void FS_SnsFriendsFind_CB( FS_Window *win )
{
	GFS_SnsApp.friends_search_page = 1;
	GFS_SnsApp.backup_page_index = GFS_SnsApp.friends_search_page;
	FS_SnsFriendsFind( FS_FALSE );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_SnsFriendsFindPrevPage_CB( FS_Window *win )
{
	if ( GFS_SnsApp.friends_search_page > 1 )
	{
		GFS_SnsApp.backup_page_index = GFS_SnsApp.friends_search_page;
		GFS_SnsApp.friends_search_page --;
		FS_SnsFriendsFind( FS_TRUE );
	}
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_SnsFriendsFindNextPage_CB( FS_Window *win )
{
	GFS_SnsApp.backup_page_index = GFS_SnsApp.friends_search_page;
	GFS_SnsApp.friends_search_page ++;
	FS_SnsFriendsFind( FS_TRUE );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_SnsFriendsAdd_CB( FS_Window *win )
{
	FS_SnsRequest req = {0};
	FS_Window *swin;
	FS_Widget *wgt;
	FS_SnsFriends *friends;
	
	swin = FS_WindowFindId( FS_W_SnsEntryFrm );
	wgt = FS_WindowGetFocusItem( swin );
	if ( wgt == FS_NULL ) return;
	if ( (FS_SINT4)wgt->private_data >= GFS_SnsApp.rsp_friends.data.friends.count ) return;
	friends = &GFS_SnsApp.rsp_friends.data.friends.friends[wgt->private_data];
	
	req.req = FS_SNS_REQ_ADD_FRIEND;
	req.data.friends.type = (FS_CHAR *)swin->private_data;
	req.data.friends.key = friends->id;
	
	FS_SnsUIRequestStart( win, &req, FS_FALSE );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_SnsFriendsDel_CB( FS_Window *win )
{
	FS_SnsRequest req = {0};
	FS_Window *swin;
	FS_Widget *wgt;
	FS_SnsFriends *friends;

	swin = FS_WindowFindId( FS_W_SnsEntryFrm );
	wgt = FS_WindowGetFocusItem( swin );
	if ( wgt == FS_NULL ) return;
	if ( (FS_SINT4)wgt->private_data >= GFS_SnsApp.rsp_friends.data.friends.count ) return;
	friends = &GFS_SnsApp.rsp_friends.data.friends.friends[wgt->private_data];
	
	req.req = FS_SNS_REQ_DEL_FRIEND;
	req.data.friends.type = (FS_CHAR *)swin->private_data;
	req.data.friends.key = friends->username;
	
	FS_SnsUIRequestStart( win, &req, FS_FALSE );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_SnsFriendsMenu_UI( FS_Window *win )
{
	FS_Widget *iFriend = FS_NULL;
	FS_Window *pMenu;
	FS_Widget *wgt;
	FS_Rect rect;
	FS_SnsFriends *friends;

	wgt = FS_WindowGetFocusItem( win );
	if ( wgt == FS_NULL ) return;
	if ( (FS_SINT4)wgt->private_data >= GFS_SnsApp.rsp_friends.data.friends.count ) return;
	friends = &GFS_SnsApp.rsp_friends.data.friends.friends[wgt->private_data];
	rect = FS_GetWidgetDrawRect( wgt );
	
	pMenu = FS_CreatePopUpMenu( 0, &rect, 1 );

	if ( GFS_SnsApp.is_friends_search_list ) {
		if ( friends->is_friend ) {
			if ( FS_STR_I_EQUAL((FS_CHAR *)win->private_data, "facebook" )) {
				iFriend = FS_CreateMenuItem( 0, FS_Text(FS_T_UNFRIEND) );
			} else {
				iFriend = FS_CreateMenuItem( 0, FS_Text(FS_T_UNFOLLOW) );
			}
			FS_WidgetSetHandler( iFriend, FS_SnsFriendsDel_CB );
		} else {
			if ( FS_STR_I_EQUAL((FS_CHAR *)win->private_data, "facebook" )) {
				iFriend = FS_CreateMenuItem( 0, FS_Text(FS_T_ADD_FRIEND) );
			} else {
				iFriend = FS_CreateMenuItem( 0, FS_Text(FS_T_FOLLOW) );
			}
			FS_WidgetSetHandler( iFriend, FS_SnsFriendsAdd_CB );
		}
	} else {
		if ( FS_STR_I_EQUAL((FS_CHAR *)win->private_data, "facebook" )) {
			iFriend = FS_CreateMenuItem( 0, FS_Text(FS_T_UNFRIEND) );
		} else {
			iFriend = FS_CreateMenuItem( 0, FS_Text(FS_T_UNFOLLOW) );
		}
		FS_WidgetSetHandler( iFriend, FS_SnsFriendsDel_CB );
	}
	
	FS_MenuAddItem( pMenu, iFriend );
	
	FS_MenuSetSoftkey( pMenu );
	
	FS_ShowWindow( pMenu );
}

static void FS_SnsEntryShowFindFriendsWidget( FS_Window *win, FS_CHAR *txt )
{
	FS_Widget *wItem, *wEdit;
	FS_TabSheet *sheet = FS_WindowGetTabSheet( win, FS_W_SnsTabFriends );

	wEdit = FS_CreateEditBox( FS_W_SnsFriendsTabEdit, txt, 0, 1, FS_NULL );
	FS_WGT_SET_FORCE_NO_STATUS_BAR( wEdit );
	FS_TabSheetAddWidget( sheet, wEdit );
	wItem = FS_CreateListItem( 0, FS_Text(FS_T_FIND_FRIEND), FS_NULL, 0, 1 );
	FS_WGT_SET_SHARE_HEIGHT( wItem );
	FS_WGT_SET_ALIGN_CENTER( wItem );
	FS_WGT_SET_DRAW_BORDER( wItem );
	FS_WidgetSetHandler( wItem, FS_SnsFriendsFind_CB );
	FS_TabSheetAddWidget( sheet, wItem );
}

static void FS_SnsEntryShowFriendsList( FS_SnsResponse *rsp, FS_SnsReqCode reqCode, FS_BOOL bUpdateScreen )
{
	FS_CHAR absFile[FS_MAX_PATH_LEN] = {0};
	FS_TabSheet *sheet = FS_NULL;
	FS_SINT4 i;
	FS_Widget *wItem, *wNextPage = FS_NULL, *wPrevPage = FS_NULL;
	FS_Window *win = FS_WindowFindId( FS_W_SnsEntryFrm );
	FS_BOOL bUpdateSnsEntryFrm = FS_TRUE;
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_CHAR *txt = FS_NULL;

	if ( win == FS_NULL ) return;
	
	FS_SnsEntryFormSetTitle( win, reqCode, bUpdateScreen );
	
	/* save global data, just reference, do not copy */
	FS_SnsFreeResponse( &GFS_SnsApp.rsp_friends, FS_SNS_REQ_GET_FRIENDS );
	IFS_Memcpy( &GFS_SnsApp.rsp_friends, rsp, sizeof(FS_SnsResponse) );

	if ( reqCode == FS_SNS_REQ_FIND_FRIENDS ) {
		GFS_SnsApp.is_friends_search_list = FS_TRUE;
	} else {
		GFS_SnsApp.is_friends_search_list = FS_FALSE;
	}
	
	wItem = FS_WindowGetWidget( win, FS_W_SnsFriendsTabEdit );
	if ( wItem && wItem->text ) {
		txt = IFS_Strdup( wItem->text ); 
	}
	FS_WindowDelTabSheetWidgetList( win, FS_W_SnsTabFriends );
	sheet = FS_WindowGetTabSheet( win, FS_W_SnsTabFriends );
	FS_SnsEntryShowFindFriendsWidget( win, txt );
	FS_SAFE_FREE( txt );

	for ( i = 0; i < rsp->data.friends.count; i ++ ) {
		if ( FS_STR_I_EQUAL(rsp->data.friends.friends[i].sex, "m" )) {
			IFS_Snprintf( absFile, sizeof(absFile) - 1, "%s (%s)", 
				rsp->data.friends.friends[i].username, FS_Text(FS_T_MALE));
		} else if ( FS_STR_I_EQUAL(rsp->data.friends.friends[i].sex, "f" )) {
			IFS_Snprintf( absFile, sizeof(absFile) - 1, "%s (%s)", 
				rsp->data.friends.friends[i].username, FS_Text(FS_T_FEMALE));
		} else {
			IFS_Snprintf( absFile, sizeof(absFile) - 1, "%s (%s)", 
				rsp->data.friends.friends[i].username, FS_Text(FS_T_UNKNOW));
		}
		wItem = FS_CreateListItem( i, absFile, rsp->data.friends.friends[i].location, 0, 2 );
		wItem->private_data = i;
		if ( config->display_image && rsp->data.friends.friends[i].icon_file ) {
			FS_GetAbsFileName( FS_DIR_TMP, rsp->data.friends.friends[i].icon_file, absFile );
			FS_WidgetSetIconFile( wItem, absFile );
		}
		FS_WidgetSetHandler( wItem, FS_SnsFriendsMenu_UI );
		FS_TabSheetAddWidget( sheet, wItem );
	}
	
	if ( GFS_SnsApp.is_friends_search_list == FS_FALSE ) {
		/* add page up/down widget */
		if ( rsp->data.friends.count >= config->msg_cnt_per_page ) {
			wNextPage = FS_CreateListItem( 0, FS_Text(FS_T_NEXT_PAGE), FS_NULL, 0, 1 );
			FS_WidgetSetHandler( wNextPage, FS_SnsFriendsListNextPage_CB );
			FS_TabSheetAddWidget( sheet, wNextPage );
		}
		if ( GFS_SnsApp.friends_page > 1 ) {
			wPrevPage = FS_CreateListItem( 0, FS_Text(FS_T_PREV_PAGE), FS_NULL, 0, 1 );
			if ( wNextPage ) FS_WGT_SET_SHARE_HEIGHT( wPrevPage );
			FS_WidgetSetHandler( wPrevPage, FS_SnsFriendsListPrevPage_CB );
			FS_TabSheetAddWidget( sheet, wPrevPage );
		}
	} else {
		/* add page up/down widget */
		if ( rsp->data.friends.count >= config->msg_cnt_per_page ) {
			wNextPage = FS_CreateListItem( 0, FS_Text(FS_T_NEXT_PAGE), FS_NULL, 0, 1 );
			FS_WidgetSetHandler( wNextPage, FS_SnsFriendsFindNextPage_CB );
			FS_TabSheetAddWidget( sheet, wNextPage );
		}
		if ( GFS_SnsApp.friends_search_page > 1 ) {
			wPrevPage = FS_CreateListItem( 0, FS_Text(FS_T_PREV_PAGE), FS_NULL, 0, 1 );
			if ( wNextPage ) FS_WGT_SET_SHARE_HEIGHT( wPrevPage );
			FS_WidgetSetHandler( wPrevPage, FS_SnsFriendsFindPrevPage_CB );
			FS_TabSheetAddWidget( sheet, wPrevPage );
		}
	}

	if ( bUpdateScreen ) {
		FS_InvalidateRect( win, &win->client_rect );
	}
	
	/* avoid been free by SnsLib */
	IFS_Memset( rsp, 0, sizeof(FS_SnsResponse) );
}

static void FS_SnsEntryGetMsgs( FS_Window *win, FS_SnsReqCode reqCode, FS_BOOL bCacheFirst )
{
	FS_SnsRequest req = {0};
	FS_SnsResponse rsp = {0};
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_BOOL bCacheOK = FS_FALSE;

	req.req = reqCode;
	if ( win->id == FS_W_SnsMsgListFrm ) {
		req.data.get_msgs.type = "isync";
		req.data.get_msgs.page = GFS_SnsApp.msg_page;
		req.data.get_msgs.name = (FS_CHAR *)win->private_data;
	} else {
		req.data.get_msgs.type = (FS_CHAR *)win->private_data;
		if ( reqCode == FS_SNS_REQ_GET_UPDATE ) {
			req.data.get_msgs.page = GFS_SnsApp.update_page;
		} else {
			req.data.get_msgs.page = GFS_SnsApp.reply_page;
		}
	}
	req.data.get_msgs.count = config->msg_cnt_per_page;

	if ( bCacheFirst ) {
		bCacheOK = FS_SnsLibReadCache( GFS_SnsApp.snslib, &req, &rsp );
		if ( bCacheOK ) {
			FS_SnsEntryShowMsgList( &rsp, req.req, FS_TRUE );
			return;
		}
	} else {
		FS_SnsLibClearCache( GFS_SnsApp.snslib, &req );
	}

	FS_SnsUIRequestStart( win, &req, FS_TRUE );
}

static FS_BOOL FS_SnsEntryWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	if ( cmd == FS_WM_COMMAND && wparam == FS_EV_TAB_FOCUS_CHANGE ) {
		FS_TabSheet *tab = (FS_TabSheet *)lparam;
		FS_SnsResponse rsp = {0};

		/* cancel request if any */
		FS_SnsLibCancelRequest( GFS_SnsApp.snslib );
		FS_SnsUIRequestStop( );

		if ( tab->id == FS_W_SnsTabUpdate ) {
			FS_SnsEntryFormSetTitle( win, FS_SNS_REQ_GET_UPDATE, FS_TRUE );
			FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_REFRESH), FS_SnsMsgListUpdate_CB );
			FS_WindowSetSoftkey( win, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
			FS_RedrawSoftkeys( win );
		} else if ( tab->id == FS_W_SnsTabCompose ) {
			FS_SnsEntryFormSetTitle( win, FS_SNS_REQ_GET_TWEET, FS_TRUE );
			FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_SEND), FS_SnsPutTweet_CB );
			FS_WindowSetSoftkey( win, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
			FS_RedrawSoftkeys( win );
		} else if ( tab->id == FS_W_SnsTabReply ) {
			FS_SnsRequest req = {0};
			FS_SnsResponse rsp = {0};
			FS_BOOL bCacheOK = FS_FALSE;

			if ( win->private_data2 == 0 ) {
				win->private_data2 = 1;
				req.req = FS_SNS_REQ_GET_REPLY;
				req.data.get_msgs.type = (FS_CHAR *)win->private_data;
				req.data.get_msgs.page = GFS_SnsApp.reply_page;
				bCacheOK = FS_SnsLibReadCache( GFS_SnsApp.snslib, &req, &rsp );
				if ( bCacheOK ) {
					FS_SnsEntryShowMsgList( &rsp, req.req, FS_FALSE );
					FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_REFRESH), FS_SnsMsgListUpdate_CB );
					FS_WindowSetSoftkey( win, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
					FS_InvalidateRect( win, FS_NULL );
				}
			}

			if ( ! bCacheOK )
			{
				FS_SnsEntryFormSetTitle( win, FS_SNS_REQ_GET_REPLY, FS_TRUE );
				FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_REFRESH), FS_SnsMsgListUpdate_CB );
				FS_WindowSetSoftkey( win, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
				FS_RedrawSoftkeys( win );
			}
		} else if ( tab->id == FS_W_SnsTabFriends ) {
			FS_SnsEntryFormSetTitle( win, FS_SNS_REQ_GET_FRIENDS, FS_TRUE );
			FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_REFRESH), FS_SnsFriendsListUpdate_CB );
			FS_WindowSetSoftkey( win, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
			FS_RedrawSoftkeys( win );
		}
	} else if ( cmd == FS_WM_DESTROY ) {
		/* cancel request if any when this window destroy */
		FS_SnsLibCancelRequest( GFS_SnsApp.snslib );
		FS_SnsFreeResponse( &GFS_SnsApp.rsp_update, FS_SNS_REQ_GET_UPDATE );
		FS_SnsFreeResponse( &GFS_SnsApp.rsp_reply, FS_SNS_REQ_GET_UPDATE );
		FS_SnsFreeResponse( &GFS_SnsApp.rsp_friends, FS_SNS_REQ_GET_FRIENDS );
		FS_SAFE_FREE( GFS_SnsApp.fname );
		GFS_SnsApp.fsize = 0;
	}
	return FS_FALSE;
}

static void FS_SnsEntry_UI( FS_Window *win )
{
	FS_Window *mWin;
	FS_Widget *wgt;
	FS_TabSheet *tabUpdate, *tabReply, *tabCompose, *tabFriends;
	FS_Widget *wEdit, *wLabel, *wCheck;
#ifdef FS_SNS_ENABLE_PHOTO_UPLOAD
	FS_Widget *wImage;
#endif
	FS_SnsRequest req = {0};
	FS_SnsResponse rsp = {0};
	FS_BOOL bCacheOK = FS_FALSE;
	FS_CHAR szTxt[64] = {0};
	FS_EditParam eParam = { FS_IM_CHI, FS_IM_ALL, 140 };
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_SINT4 i;

	eParam.limit_character = FS_TRUE;

	wgt = FS_WindowGetFocusItem( win );
	if ( ! FS_SnsIsEntryBound( (FS_CHAR *)wgt->private_data ) )
	{
		FS_MessageBox( FS_MS_OK, FS_Text(FS_T_SNS_UNBOUND), FS_NULL, FS_TRUE );
		return;
	}

	/* cancel request if any */
	FS_SnsLibCancelRequest( GFS_SnsApp.snslib );
	FS_SnsUIRequestStop( );

	mWin = FS_CreateWindow( FS_W_SnsEntryFrm, FS_Text(FS_T_UPDATE), FS_SnsEntryWndProc );
	mWin->private_data = wgt->private_data;	/* save sns type */

	tabCompose = FS_CreateTabSheet( FS_W_SnsTabCompose, FS_Text(FS_T_COMPOSE), FS_I_COMPOSE, FS_FALSE );
	tabUpdate = FS_CreateTabSheet( FS_W_SnsTabUpdate, FS_Text(FS_T_UPDATE), FS_I_UPDATES, FS_FALSE );
	tabReply = FS_CreateTabSheet( FS_W_SnsTabReply, FS_Text(FS_T_SNS_REPLY), FS_I_REPLY, FS_FALSE );
	tabFriends = FS_CreateTabSheet( FS_W_SnsTabFriends, FS_Text(FS_T_FRIENDS), FS_I_FRIENDS, FS_FALSE );

	/* compose tab */
	wLabel = FS_CreateLabel( 0, FS_Text(FS_T_SNS_SAY_SOMETHING), 0, 1 );
	wEdit = FS_CreateEditBox( FS_W_SnsComposeTabEdit, FS_NULL, 0, 2, &eParam );
	FS_WGT_SET_FORCE_MULTI_LINE( wEdit );
	FS_WGT_SET_FORCE_NO_STATUS_BAR( wEdit );
#ifdef FS_SNS_ENABLE_PHOTO_UPLOAD
	wImage = FS_CreateListItem( FS_W_SnsComposeTabImage, FS_Text(FS_T_PHOTO), FS_NULL, FS_I_IMAGE, 1 );
	FS_WidgetSetHandler( wImage, FS_SnsPhotoMenu_UI );
#endif
	FS_TabSheetAddWidget( tabCompose, wLabel );
	FS_TabSheetAddWidget( tabCompose, wEdit );
#ifdef FS_SNS_ENABLE_PHOTO_UPLOAD
	FS_TabSheetAddWidget( tabCompose, wImage );
#endif
	for ( i = 0; i < config->account_num; i ++ )
	{
		if ( config->accout[i].bound && 0 != IFS_Stricmp((FS_CHAR *)mWin->private_data, config->accout[i].name)
			&& 0 != IFS_Stricmp(config->accout[i].name, "isync") )
		{
			IFS_Snprintf( szTxt, sizeof(szTxt) - 1, "%s%s", FS_Text(FS_T_SYNC_TO), config->accout[i].name );
			wCheck = FS_CreateCheckBox( FS_W_SnsComposeTabCheck + i, szTxt );
			wCheck->private_data = (FS_UINT4)config->accout[i].name;
			FS_WidgetSetCheck( wCheck, FS_TRUE );
			FS_TabSheetAddWidget( tabCompose, wCheck );
		}
	}
	
	/* insert all tab sheet */
	FS_WindowSetSheetCountPerPage( mWin, 4 );
	FS_WindowAddTabSheet( mWin, tabUpdate );
	FS_WindowAddTabSheet( mWin, tabCompose );
	FS_WindowAddTabSheet( mWin, tabReply );
	FS_WindowAddTabSheet( mWin, tabFriends );

	/* update tab */
	req.req = FS_SNS_REQ_GET_UPDATE;
	req.data.get_msgs.type = (FS_CHAR *)mWin->private_data;
	req.data.get_msgs.page = GFS_SnsApp.update_page;
	bCacheOK = FS_SnsLibReadCache( GFS_SnsApp.snslib, &req, &rsp );
	if ( bCacheOK ) {
		FS_SnsEntryShowMsgList( &rsp, req.req, FS_FALSE );
	} else {
		wLabel =  FS_CreateLabel( 0, FS_Text(FS_T_PRESS_REFRESH), 0, 1 );
		FS_TabSheetAddWidget( tabUpdate, wLabel );
	}

	/* reply tab */
	wLabel =  FS_CreateLabel( 0, FS_Text(FS_T_PRESS_REFRESH), 0, 1 );
	FS_TabSheetAddWidget( tabReply, wLabel );

	/* friends tab */
	FS_SnsEntryShowFindFriendsWidget( mWin, FS_NULL );

	FS_SnsEntryFormSetTitle( mWin, FS_SNS_REQ_GET_UPDATE, FS_FALSE );
	FS_WindowSetSoftkey( mWin, 1, FS_Text(FS_T_REFRESH), FS_SnsMsgListUpdate_CB );
	FS_WindowSetSoftkey( mWin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );

	FS_ShowWindow( mWin );
}

static void FS_SnsFeedback_UI( FS_Window *win )
{
	FS_Widget *wLabel;
	FS_Window *twin = FS_CreateWindow( 0, FS_Text(FS_T_ABOUT), FS_NULL );

	/* use id as skin's index. must follow up GFS_Skins define */
	wLabel = FS_CreateLabel( 0, FS_Text(FS_T_CONTACT_US), 0, 1 );
	FS_WindowAddWidget( twin, wLabel );
	wLabel = FS_CreateLabel( 0, FS_Text(FS_T_AVALAA), 0, 4 );
	FS_WindowAddWidget( twin, wLabel );

	FS_WindowSetSoftkey( twin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	FS_ShowWindow( twin );
}

static void FS_SnsMsgList_UI( FS_Window *win )
{
	FS_Window *swin;
	FS_Window *lwin;
	FS_Widget *wgt;
	FS_SnsRequest req = {0};
	FS_SINT4 msgCount = 0;
	FS_SnsConfig *config = FS_SnsGetConfig( );

	swin = FS_WindowFindId( FS_W_SnsListFrm );
	wgt = FS_WindowGetFocusItem( swin );
	if ( wgt == FS_NULL ) return;
	
	if ( wgt->extra_text )
	{
		msgCount = IFS_Atoi( wgt->extra_text );
	}

	if ( msgCount == 0 )
	{
		FS_MessageBox( FS_MS_OK, FS_Text(FS_T_LIST_EMPTY), FS_NULL, FS_TRUE );
		return;
	}

	if ( wgt->id >= FS_W_SnsItemUpdates && wgt->id <= FS_W_SnsItemUpdatesLast ) {
		req.req = FS_SNS_REQ_GET_UPDATE;
	} else if ( wgt->id >= FS_W_SnsItemReply && wgt->id <= FS_W_SnsItemReplyLast ) {
		req.req = FS_SNS_REQ_GET_REPLY;
	} else {
		return;
	}
	req.data.get_msgs.type = "isync";
	req.data.get_msgs.name = (FS_CHAR *)wgt->private_data;
	req.data.get_msgs.count = config->msg_cnt_per_page;
	req.data.get_msgs.page = 1;
	GFS_SnsApp.msg_page = 1;

	lwin = FS_CreateWindow( FS_W_SnsMsgListFrm, FS_Text(FS_T_ISYNC), FS_NULL );
	lwin->private_data = wgt->private_data;
	FS_WindowSetSoftkey( lwin, 1, FS_Text(FS_T_REFRESH), FS_SnsMsgListUpdate_CB );
	FS_WindowSetSoftkey( lwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );

	FS_ShowWindow( lwin );

	FS_SnsUIRequestStart( lwin, &req, FS_FALSE );
}

static void FS_SnsSettingSave_CB( FS_Window *win )
{
	FS_Widget *wgt;
	FS_SnsConfig *config = FS_SnsGetConfig( );

	wgt = FS_WindowGetWidget( win, FS_W_SnsUserName );
	if ( wgt->text && wgt->text[0] ) {
		IFS_Strncpy( config->user, wgt->text, sizeof(config->user) - 1 );
	} else {
		config->user[0] = 0;
	}
	wgt = FS_WindowGetWidget( win, FS_W_SnsPassword );
	if ( wgt->text && wgt->text[0] ) {
		IFS_Strncpy( config->pass, wgt->text, sizeof(config->pass) - 1 );
	} else {
		config->pass[0] = 0;
	}
	wgt = FS_WindowGetWidget( win, FS_W_SnsApn );
	/* when apn changed, we need reset net connection */
	if ( wgt->text && wgt->text[0] ) {
		if ( IFS_Stricmp(wgt->text, config->apn) != 0 ) {
			IFS_Strncpy( config->apn, wgt->text, sizeof(config->apn) - 1 );
			FS_NetDisconnect( FS_APP_SNS );
		}
	} else {
		if ( config->apn[0] ) {
			config->apn[0] = 0;
			FS_NetDisconnect( FS_APP_SNS );
		}
	}
	wgt = FS_WindowGetWidget( win, FS_W_SnsProxyAddr );
	if ( wgt->text && wgt->text[0] ) {
		IFS_Strncpy( config->proxy_addr, wgt->text, sizeof(config->proxy_addr) - 1 );
	} else {
		config->proxy_addr[0] = 0;
	}
	wgt = FS_WindowGetWidget( win, FS_W_SnsProxyPort );
	if ( wgt->text && wgt->text[0] ) {
		config->proxy_port = IFS_Atoi(wgt->text);
	} else {
		config->proxy_port = 0;
	}
	wgt = FS_WindowGetWidget( win, FS_W_SnsUseProxy );
	config->use_proxy = FS_WGT_GET_CHECK( wgt ) ? FS_TRUE : FS_FALSE;

	wgt = FS_WindowGetWidget( win, FS_W_SnsMsgCount );
	if ( wgt->text && wgt->text[0] ) {
		config->msg_cnt_per_page = IFS_Atoi(wgt->text);
	} else {
		config->msg_cnt_per_page = FS_SNS_DEF_MSGS_COUNT;
	}
	if ( config->msg_cnt_per_page <= 0 || config->msg_cnt_per_page >= FS_SNS_MAX_MSGS_COUNT ) {
		config->msg_cnt_per_page = FS_SNS_DEF_MSGS_COUNT;
	}

	wgt = FS_WindowGetWidget( win, FS_W_SnsSettingTimeZone );
	if ( wgt->text && wgt->text[0] ) {
		config->time_zone = IFS_Atoi(wgt->text);
	} else {
		config->time_zone = 8;
	}
	if ( config->time_zone < -12 || config->time_zone > 12 ) {
		config->time_zone = 8;
	}

	wgt = FS_WindowGetWidget( win, FS_W_SnsDisplayImage );
	config->display_image = FS_WGT_GET_CHECK( wgt ) ? FS_TRUE : FS_FALSE;

	wgt = FS_WindowGetWidget( win, FS_W_SnsImageCache );
	config->image_cache = FS_WGT_GET_CHECK( wgt ) ? FS_TRUE : FS_FALSE;

	FS_SnsSetConfig( config );
	FS_DestroyWindow( win );
}
static void FS_SnsSettingDiscard_CB( FS_Window *win )
{
	GFS_SkinIndex = (FS_UINT1)win->private_data;	// restore theme
	FS_DestroyWindow( win );
}

static void FS_SnsSettingSelectTheme_CB( FS_Window *win )
{
	FS_Widget *wgt;
	wgt = FS_WindowGetFocusItem( win );
	GFS_SkinIndex = wgt->id;
	FS_DestroyWindow( win );
}

static void FS_SnsSettingSelectTheme_UI( FS_Window *win )
{
	FS_Window *menu;
	FS_Widget *wgt;
	FS_Widget *focusWgt = FS_WindowGetFocusItem( win );
	FS_Rect rect = FS_GetWidgetDrawRect( focusWgt );
	FS_SINT4 i, total = 5;
	FS_SINT4 ThemeNameIds[] = 
	{FS_T_BLUE_SKY, FS_T_RED_RECALL, FS_T_GREEN_GRASS, FS_T_GRAY_CLASSICAL, FS_T_YELLOW_STORM, };

	menu = FS_CreatePopUpMenu( FS_W_MmsEditFrameList, &rect, total );
	for( i = 0; i < total; i ++ )
	{
		wgt = FS_CreateMenuItem( i, FS_Text(ThemeNameIds[i]) );
		wgt->private_data = i;
		FS_WidgetSetHandler( wgt, FS_SnsSettingSelectTheme_CB );
		FS_MenuAddItem( menu, wgt );
	}	
	FS_MenuSetSoftkey( menu );
	FS_ShowWindow( menu );
}

static void FS_SnsSetting_UI( FS_Window *pwin )
{
	FS_Widget *wMsgCntLabel, *wMsgCnt, *wDispImg, *wUserLabel, *wUserName, 
		*wPwdLabel, *wPasswd, *wApnLabel, *wApn, 
		*wProxyAddrLabel, *wProxyAddr, *wProxyPortLabel, *wProxyPort, *wEnableProxy,
		*wVerLabel, *wVersion, *wThemeLabel, *wTheme, *wImageCache, *wTimeZoneLabel, *wTimeZone;
	FS_Window *win;
	FS_EditParam eParam = { FS_IM_ABC, FS_IM_123 | FS_IM_ABC, FS_URL_LEN };
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_CHAR szStr[32] = {0};
	FS_SINT4 ThemeNameIds[] = 
		{FS_T_BLUE_SKY, FS_T_RED_RECALL, FS_T_GREEN_GRASS, FS_T_GRAY_CLASSICAL, FS_T_YELLOW_STORM, };

	win = FS_CreateWindow( FS_W_SnsSettingFrm, FS_Text(FS_T_SETTING), FS_NULL );
	
	FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_SAVE), FS_SnsSettingSave_CB );
	FS_WindowSetSoftkey( win, 3, FS_Text(FS_T_BACK), FS_SnsSettingDiscard_CB );
	
	wVerLabel = FS_CreateLabel( 0, FS_Text(FS_T_VERSION), 0, 1 );
	wVersion = FS_CreateEditBox( 0, "1.0.2", 0, 1, &eParam );
	FS_WGT_CLR_CAN_WRITE( wVersion );
	FS_WGT_CLR_MULTI_LINE( wVersion );
	wThemeLabel = FS_CreateLabel( 0, FS_Text(FS_T_THEME), 0, 1 );
	wTheme = FS_CreateComboBox( FS_W_SnsSettingTheme, FS_Text(ThemeNameIds[GFS_SkinIndex%5]), 0 );
	wTimeZoneLabel = FS_CreateLabel( 0, FS_Text(FS_T_TIMEZONE), 0, 1 );
	IFS_Snprintf( szStr, sizeof(szStr) - 1, "%d", config->time_zone );
	wTimeZone = FS_CreateEditBox( FS_W_SnsSettingTimeZone, szStr, 0, 1, &eParam );
	FS_WGT_CLR_MULTI_LINE( wTimeZone );
	wUserLabel = FS_CreateLabel( 0, FS_Text(FS_T_USER_NAME), 0, 1 );
	wUserName = FS_CreateEditBox( FS_W_SnsUserName, config->user, 0, 1, &eParam );
	FS_WGT_CLR_MULTI_LINE( wUserName );
	wPwdLabel = FS_CreateLabel( 0, FS_Text(FS_T_PASSWORD), 0, 1 );
	wPasswd = FS_CreateEditBox( FS_W_SnsPassword, config->pass, 0, 1, &eParam );
	FS_WGT_SET_MARK_CHAR( wPasswd );
	FS_WGT_CLR_MULTI_LINE( wPasswd );
	wApnLabel = FS_CreateLabel( 0, FS_Text(FS_T_APN), 0, 1 );
	wApn = FS_CreateEditBox( FS_W_SnsApn, config->apn, 0, 1, &eParam );
	FS_WGT_CLR_MULTI_LINE( wApn );
	eParam.preferred_method = FS_IM_123;
	wProxyAddrLabel = FS_CreateLabel( 0, FS_Text(FS_T_PROXY_ADDR), 0, 1 );
	wProxyAddr = FS_CreateEditBox( FS_W_SnsProxyAddr, config->proxy_addr, 0, 1, &eParam );
	FS_WGT_CLR_MULTI_LINE( wProxyAddr );
	wProxyPortLabel = FS_CreateLabel( 0, FS_Text(FS_T_PROXY_PORT), 0, 1 );
	IFS_Itoa( config->proxy_port, szStr, 10 );
	wProxyPort = FS_CreateEditBox( FS_W_SnsProxyPort, szStr, 0, 1, &eParam );
	FS_WGT_CLR_MULTI_LINE( wProxyPort );
	wEnableProxy = FS_CreateCheckBox( FS_W_SnsUseProxy, FS_Text(FS_T_USE_PROXY) );
	FS_WidgetSetCheck( wEnableProxy, config->use_proxy );

	wMsgCntLabel = FS_CreateLabel( 0, FS_Text(FS_T_MSG_CNT_PER_PAGE), 0, 1 );
	IFS_Itoa( config->msg_cnt_per_page, szStr, 10 );
	eParam.preferred_method = FS_IM_123;
	wMsgCnt = FS_CreateEditBox( FS_W_SnsMsgCount, szStr, 0, 1, &eParam );
	FS_WGT_CLR_MULTI_LINE( wMsgCnt );
	wDispImg = FS_CreateCheckBox( FS_W_SnsDisplayImage, FS_Text(FS_T_WEB_DOWN_IMAGE) );
	FS_WidgetSetCheck( wDispImg, config->display_image );
	wImageCache = FS_CreateCheckBox( FS_W_SnsImageCache, FS_Text(FS_T_IMAGE_CACHE) );
	FS_WidgetSetCheck( wImageCache, config->image_cache );

	FS_WGT_SET_FORCE_NO_STATUS_BAR( wUserName );
	FS_WGT_SET_FORCE_NO_STATUS_BAR( wPasswd );
	FS_WGT_SET_FORCE_NO_STATUS_BAR( wApn );
	FS_WGT_SET_FORCE_NO_STATUS_BAR( wProxyAddr );
	FS_WGT_SET_FORCE_NO_STATUS_BAR( wProxyPort );
	FS_WGT_SET_FORCE_NO_STATUS_BAR( wVersion );
	FS_WGT_SET_FORCE_NO_STATUS_BAR( wTimeZone );
	FS_WGT_SET_FORCE_NO_STATUS_BAR( wMsgCnt );

	FS_WidgetSetHandler( wTheme, FS_SnsSettingSelectTheme_UI );

	FS_WindowAddWidget( win, wVerLabel );
	FS_WGT_SET_SHARE_HEIGHT( wVersion );
	FS_WindowAddWidget( win, wVersion );
	FS_WindowAddWidget( win, wThemeLabel );
	FS_WGT_SET_SHARE_HEIGHT( wTheme );
	FS_WindowAddWidget( win, wTheme );
	FS_WindowAddWidget( win, wTimeZoneLabel );
	FS_WGT_SET_SHARE_HEIGHT( wTimeZone );
	FS_WindowAddWidget( win, wTimeZone );
	FS_WindowAddWidget( win, wMsgCntLabel );
	FS_WGT_SET_SHARE_HEIGHT( wMsgCnt );
	FS_WindowAddWidget( win, wMsgCnt );
	FS_WindowAddWidget( win, wDispImg );
	FS_WindowAddWidget( win, wImageCache );

	FS_WindowAddWidget( win, wApnLabel );
	FS_WindowAddWidget( win, wApn );
	FS_WindowAddWidget( win, wUserLabel );
	FS_WindowAddWidget( win, wUserName );
	FS_WindowAddWidget( win, wPwdLabel );
	FS_WindowAddWidget( win, wPasswd );
	FS_WindowAddWidget( win, wEnableProxy );
	FS_WindowAddWidget( win, wProxyAddrLabel );
	FS_WindowAddWidget( win, wProxyAddr );
	FS_WindowAddWidget( win, wProxyPortLabel );
	FS_WindowAddWidget( win, wProxyPort );
	
	win->private_data = GFS_SkinIndex;
	FS_ShowWindow( win );	
	
	if( pwin->type == FS_WT_MENU || pwin->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( pwin );
}

static void FS_SnsListUpdateSummary( void )
{
	FS_Window *win = FS_WindowFindId( FS_W_SnsListFrm );
	FS_SnsRequest req = {0};
	
	req.req = FS_SNS_REQ_GET_SUMMARY;
	if ( win ) {
		FS_SnsUIRequestStart( win, &req, FS_TRUE );
	}
}

static FS_BOOL FS_SnsListWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
#ifdef FS_SNS_ENABLE_MSG_LIST
	if ( cmd == FS_WM_SETFOCUS ) {
		IFS_PostMessage( FS_MSG_UTIL_CALL, (FS_UINT4)FS_SnsListUpdateSummary );
	}
#endif
	return FS_FALSE;
}

static void FS_SnsList_UI( FS_Window *win )
{	
	FS_Widget *liSnsItem;
	FS_Window *lstWin;
	FS_SINT4 i;
	FS_SnsConfig *config = FS_SnsGetConfig();
	FS_CHAR szTxt[64] = {0};
		
	lstWin = FS_CreateWindow( FS_W_SnsListFrm, FS_Text(FS_T_EXTENSION), FS_SnsListWndProc );
	
	FS_WindowSetSoftkey( lstWin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	
	for (i = 0; i < config->account_num; i ++)
	{
		if ( IFS_Stricmp(config->accout[i].name, "isync") != 0 
			&& IFS_Stricmp(config->accout[i].name, "email") != 0 )
		{
			liSnsItem = FS_CreateListItem( FS_W_SnsItemMain + i, FS_Text(config->accout[i].title_tid),
				FS_NULL, (FS_SINT2)(config->accout[i].icon_id), 1 );
			liSnsItem->private_data = (FS_UINT4)config->accout[i].name;
			FS_WidgetSetHandler( liSnsItem, FS_SnsEntry_UI );
			IFS_Snprintf( szTxt, sizeof(szTxt) - 1, "%s >>", FS_Text(FS_T_GO) );
			FS_WidgetSetExtraText( liSnsItem, szTxt );
			FS_WindowAddWidget( lstWin, liSnsItem );
#ifdef FS_SNS_ENABLE_MSG_LIST
			liSnsItem = FS_CreateListItem( FS_W_SnsItemUpdates + i, FS_Text(config->accout[i].update_tid), FS_NULL, 0, 1 );
			liSnsItem->private_data = (FS_UINT4)config->accout[i].name;
			FS_WidgetSetHandler( liSnsItem, FS_SnsMsgList_UI );
			IFS_Snprintf( szTxt, sizeof(szTxt) - 1, "%03d >>", config->accout[i].update_cnt );
			FS_WidgetSetExtraText( liSnsItem, szTxt );
			FS_WindowAddWidget( lstWin, liSnsItem );

			liSnsItem = FS_CreateListItem( FS_W_SnsItemReply + i, FS_Text(config->accout[i].reply_tid), FS_NULL, 0, 1 );
			liSnsItem->private_data = (FS_UINT4)config->accout[i].name;
			FS_WidgetSetHandler( liSnsItem, FS_SnsMsgList_UI );
			IFS_Snprintf( szTxt, sizeof(szTxt) - 1, "%03d >>", config->accout[i].reply_cnt );
			FS_WidgetSetExtraText( liSnsItem, szTxt );
			FS_WindowAddWidget( lstWin, liSnsItem );
#endif
		}
	}

	/* rss */
	liSnsItem = FS_CreateListItem( FS_W_SnsRss, FS_Text(FS_T_RSS), FS_NULL, FS_I_RSS, 1 );
	FS_WidgetSetHandler( liSnsItem, FS_SnsRss_UI );
	IFS_Snprintf( szTxt, sizeof(szTxt) - 1, "%s >>", FS_Text(FS_T_GO) );
	FS_WidgetSetExtraText( liSnsItem, szTxt );
	FS_WindowAddWidget( lstWin, liSnsItem );
	/* email */
	liSnsItem = FS_CreateListItem( FS_W_SnsEmail, FS_Text(FS_T_EMAIL), FS_NULL, FS_I_EML, 1 );
	FS_WidgetSetHandler( liSnsItem, FS_SnsEmail_UI );
	IFS_Snprintf( szTxt, sizeof(szTxt) - 1, "%s >>", FS_Text(FS_T_GO) );
	FS_WidgetSetExtraText( liSnsItem, szTxt );
	FS_WindowAddWidget( lstWin, liSnsItem );

	FS_ShowWindow( lstWin );
}

static void FS_SnsRssFormSetTitle( FS_Window *win, FS_SnsReqCode reqCode, FS_BOOL bUpdateScreen )
{
	FS_CHAR *szTitle;
	FS_CHAR szPage[16] = {0};
	
	if ( reqCode == FS_SNS_REQ_RSS_GET_ALL_ARTICLE ) {
		IFS_Snprintf( szPage, sizeof(szPage) - 1, "(%d)", GFS_SnsApp.rss_article_page );
		szTitle = FS_StrConCat( FS_Text(FS_T_RSS), "-", FS_Text(FS_T_ARTICLE), szPage );
	} else if ( reqCode == FS_SNS_REQ_RSS_GET_ALL_CHANNEL ) {
		szTitle = FS_StrConCat( FS_Text(FS_T_RSS), "-", FS_Text(FS_T_CHANNEL), FS_NULL );
	} else if ( reqCode == FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY ) {
		szTitle = FS_StrConCat( FS_Text(FS_T_RSS), "-", FS_Text(FS_T_BINDING), FS_NULL );
	} else {
		return;
	}
	
	FS_WindowSetTitle( win, szTitle );
	FS_SAFE_FREE( szTitle );
	if ( bUpdateScreen ) {
		FS_RedrawWinTitle( win );
	}
}

static void FS_SnsRssForward_CB( FS_Window *win )
{
	FS_Window *ewin;
	FS_Widget *wEdit, *wCheck, *wgt;
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_SINT4 i;
	FS_CHAR szTxt[64] = {0};

	wgt = FS_WindowGetWidget( win, FS_W_SnsRssDetailMsg );
	if ( wgt == FS_NULL ) return;

	ewin = FS_CreateWindow( FS_W_SnsRssForwardFrm, FS_Text(FS_T_FORWARD), FS_NULL );
	ewin->private_data = (FS_UINT4)"isync";
	wEdit = FS_CreateEditBox( FS_W_SnsComposeTabEdit, wgt->text, 0, 2, FS_NULL );
	FS_WGT_SET_FORCE_MULTI_LINE( wEdit );
	FS_WGT_SET_FORCE_NO_STATUS_BAR( wEdit );
	FS_WindowAddWidget( ewin, wEdit );
	for ( i = 0; i < config->account_num; i ++ )
	{
		if ( config->accout[i].bound && 0 != IFS_Stricmp(config->accout[i].name, "isync") )
		{
			IFS_Snprintf( szTxt, sizeof(szTxt) - 1, "%s%s", FS_Text(FS_T_SYNC_TO), config->accout[i].name );
			wCheck = FS_CreateCheckBox( FS_W_SnsComposeTabCheck + i, szTxt );
			wCheck->private_data = (FS_UINT4)config->accout[i].name;
			FS_WidgetSetCheck( wCheck, FS_TRUE );
			FS_WindowAddWidget( ewin, wCheck );
		}
	}

	FS_WindowSetSoftkey( ewin, 1, FS_Text(FS_T_SEND), FS_SnsPutTweet_CB );
	FS_WindowSetSoftkey( ewin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );

	FS_ShowWindow( ewin );
}

static void FS_SnsRssArticleDetail_UI( FS_Window *win, FS_BOOL bMore )
{
	FS_Widget *wChannel, *wTitle, *wMsg;
	FS_Window *dwin;
	FS_Widget *focusWgt = FS_WindowGetFocusItem( win );
	FS_SnsRssArticle *article = FS_NULL;
	FS_CHAR *cn = FS_NULL, *date = FS_NULL, *title = FS_NULL, *msg = FS_NULL, *id = FS_NULL, *type = FS_NULL;
	FS_CHAR *str, *pMsg = FS_NULL;
	
	if (focusWgt == FS_NULL) return;
	if ( win->id == FS_W_SnsRssFrm )
	{
		if ((FS_SINT4)focusWgt->private_data >= GFS_SnsApp.rsp_rss_articles.data.rss_article.count) return;
		article = &GFS_SnsApp.rsp_rss_articles.data.rss_article.articles[focusWgt->private_data];
		cn = article->channel_name;
		date = article->date;
		title = article->title;
		msg = article->msg;
		id = article->id;
	}
	else if ( win->id == FS_W_SnsRssChannelArticleListFrm )
	{
		if ((FS_SINT4)focusWgt->private_data >= GFS_SnsApp.rsp_rss_channel_articles.data.rss_article.count) return;
		article = &GFS_SnsApp.rsp_rss_channel_articles.data.rss_article.articles[focusWgt->private_data];
		cn = win->title;
		date = article->date;
		title = article->title;
		msg = article->msg;
		id = article->id;
	}
	else if ( win->id == FS_W_SnsEntryFrm ) 
	{
		if ( (FS_SINT4)focusWgt->private_data >= GFS_SnsApp.rsp_update.data.msglist.count ) return;
		cn = GFS_SnsApp.rsp_update.data.msglist.msgs[focusWgt->private_data].author;
		date = GFS_SnsApp.rsp_update.data.msglist.msgs[focusWgt->private_data].date;
		id = GFS_SnsApp.rsp_update.data.msglist.msgs[focusWgt->private_data].id;
		type = GFS_SnsApp.rsp_update.data.msglist.msgs[focusWgt->private_data].type;
		if ( GFS_SnsApp.rsp_update.data.msglist.msgs[focusWgt->private_data].msg ) {
			str = IFS_Strdup(GFS_SnsApp.rsp_update.data.msglist.msgs[focusWgt->private_data].msg);
			pMsg = str;
			title = str;
			str = IFS_Strstr( str, "=--=" );
			if ( str )
			{
				*str = 0;
				str += 4;
			}
			msg = str;
		}
	}
	else
	{
		return;
	}

	FS_DestroyWindowByID( FS_W_SnsRssDetailFrm );
	
	dwin = FS_CreateWindow( FS_W_SnsRssDetailFrm, FS_Text(FS_T_DETAIL), FS_NULL );
	wChannel = FS_CreateListItem( 0, cn, FS_SnsUtilSecondsToDateTime(date), 0, 2 ); 
	wTitle = FS_CreateLabel( FS_W_SnsRssDetailTitle, title, 0, 1 );
	wMsg = FS_CreateScroller( FS_W_SnsRssDetailMsg, FS_ProcessParagraph(msg, -1) );
	
	FS_SAFE_FREE( pMsg );

	FS_WindowAddWidget( dwin, wChannel );
	FS_WindowAddWidget( dwin, wTitle );
	FS_WindowAddWidget( dwin, wMsg );

	if ( bMore ) {
		FS_WindowSetSoftkey( dwin, 1, FS_Text(FS_T_MORE), FS_SnsRssChannelArticleList_UI );
	} else {
		FS_WindowSetSoftkey( dwin, 1, FS_Text(FS_T_FORWARD), FS_SnsRssForward_CB );
	}
	FS_WindowSetSoftkey( dwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	FS_ShowWindow( dwin );

	/* get rss article content */
	if ( id ) {
		FS_SnsRequest req = {0};

		if ( win->id == FS_W_SnsEntryFrm ) {
			req.req = FS_SNS_REQ_GET_UPDATE_CONTENT;
			req.data.get_msgs.id = id;
			req.data.get_msgs.type = type;
		} else {
			if ( win->id == FS_W_SnsRssChannelArticleListFrm ) {
				req.req = FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL_CONTENT;
			} else {
				req.req = FS_SNS_REQ_RSS_GET_ALL_ARTICLE_CONTENT;
			}
			req.data.rss.id = id;
		}
		FS_SnsUIRequestStart( win, &req, FS_FALSE );
	}
}

static void FS_SnsRssArticleDetail_CB( FS_Window *win )
{
	FS_SnsRssArticleDetail_UI( win, FS_FALSE );
}

static void FS_SnsRssArticleDetailMore_CB( FS_Window *win )
{
	FS_SnsRssArticleDetail_UI( win, FS_TRUE );
}

static void FS_SnsRssChannelArticleListUpdate( FS_BOOL bCacheFirst )
{
	FS_SnsRequest req = {0};
	FS_SnsResponse rsp = {0};
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_Window *win = FS_WindowFindId( FS_W_SnsRssChannelArticleListFrm );
	FS_BOOL bCacheOK = FS_FALSE;

	if ( win == FS_NULL ) return;

	req.req = FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL;
	req.data.rss.id = (FS_CHAR *)win->private_data;
	req.data.rss.page = GFS_SnsApp.rss_channel_article_page;
	req.data.rss.count = config->msg_cnt_per_page;

	if ( bCacheFirst ) {
		bCacheOK = FS_SnsLibReadCache( GFS_SnsApp.snslib, &req, &rsp );
		if ( bCacheOK ) {
			FS_SnsRssShowChannelArticleList( &rsp, req.req, FS_TRUE );
			return;
		}
	} else {
		FS_SnsLibClearCache( GFS_SnsApp.snslib, &req );
	}
	FS_SnsUIRequestStart( win, &req, FS_TRUE );
}

static void FS_SnsRssChannelArticleListUpdate_CB( FS_Window *win )
{
	GFS_SnsApp.rss_channel_article_page = 1;
	GFS_SnsApp.backup_page_index = GFS_SnsApp.rss_channel_article_page;
	FS_SnsRssChannelArticleListUpdate( FS_FALSE );
	
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_SnsRssChannelArticleListNextPage_CB( FS_Window *win )
{
	GFS_SnsApp.backup_page_index = GFS_SnsApp.rss_channel_article_page;
	GFS_SnsApp.rss_channel_article_page ++;
	FS_SnsRssChannelArticleListUpdate( FS_TRUE );
	
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_SnsRssChannelArticleListPrevPage_CB( FS_Window *win )
{
	if ( GFS_SnsApp.rss_channel_article_page > 1 )
	{
		GFS_SnsApp.backup_page_index = GFS_SnsApp.rss_channel_article_page;
		GFS_SnsApp.rss_channel_article_page --;
		FS_SnsRssChannelArticleListUpdate( FS_TRUE );
	}
	
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_SnsRssChannelArticleListSetTitle( FS_Window *win, FS_BOOL bUpdateScreen )
{
	FS_Widget *wgt;
	FS_CHAR *channel_name = FS_NULL;
	FS_Window *swin = FS_WindowFindId( FS_W_SnsRssFrm );
	FS_CHAR szText[128] = {0};

	wgt = FS_WindowGetFocusItem( swin );
	if ( wgt ) {
		if ( (FS_SINT4)wgt->private_data >= GFS_SnsApp.rsp_rss_channels.data.rss_channel.count ) return;
		channel_name = GFS_SnsApp.rsp_rss_channels.data.rss_channel.channels[wgt->private_data].channel_name;
	} else {
		swin = FS_WindowFindId( FS_W_SnsEntryFrm );
		wgt = FS_WindowGetFocusItem( swin );
		if ( wgt == FS_NULL ) return;
		if ( (FS_SINT4)wgt->private_data >= GFS_SnsApp.rsp_update.data.msglist.count ) return;
		channel_name = GFS_SnsApp.rsp_update.data.msglist.msgs[wgt->private_data].author;
	}

	IFS_Snprintf( szText, sizeof(szText) - 1, "%s(%d)", 
		channel_name ? channel_name : FS_Text(FS_T_RSS), GFS_SnsApp.rss_channel_article_page );
	FS_WindowSetTitle( win, szText );
	if ( bUpdateScreen ) {
		FS_RedrawWinTitle( win );
	}
}

static void FS_SnsRssShowChannelArticleList( FS_SnsResponse *rsp, FS_SnsReqCode req, FS_BOOL bUpdateScreen )
{
	FS_SINT4 i;
	FS_Widget *wItem, *wNextPage = FS_NULL, *wPrevPage = FS_NULL;
	FS_Window *win = FS_WindowFindId( FS_W_SnsRssChannelArticleListFrm );
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_UINT4 pageIdx = GFS_SnsApp.rss_channel_article_page;

	if ( win == FS_NULL ) return;
	
	/* save global data, just reference, do not copy */
	FS_SnsFreeResponse( &GFS_SnsApp.rsp_rss_channel_articles, FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL );
	IFS_Memcpy( &GFS_SnsApp.rsp_rss_channel_articles, rsp, sizeof(FS_SnsResponse) );

	FS_SnsRssChannelArticleListSetTitle( win, FS_FALSE );
	FS_WindowDelWidgetList( win );
	for ( i = 0; i < rsp->data.rss_article.count; i ++ ) {
		wItem = FS_CreateListItem( i, rsp->data.rss_article.articles[i].title, 
			FS_SnsUtilSecondsToDateTime(rsp->data.rss_article.articles[i].date), FS_I_RSS, 2 );
		wItem->private_data = (FS_UINT4)i;
		FS_WidgetSetHandler( wItem, FS_SnsRssArticleDetail_CB );
		FS_WindowAddWidget( win, wItem );
	}

	/* add page up/down widget */
	if ( rsp->data.rss_article.count >= config->msg_cnt_per_page ) {
		wNextPage = FS_CreateListItem( 0, FS_Text(FS_T_NEXT_PAGE), FS_NULL, 0, 1 );
		FS_WidgetSetHandler( wNextPage, FS_SnsRssChannelArticleListNextPage_CB );
		FS_WindowAddWidget( win, wNextPage );
	}
	if ( pageIdx > 1 ) {
		wPrevPage = FS_CreateListItem( 0, FS_Text(FS_T_PREV_PAGE), FS_NULL, 0, 1 );
		if ( wNextPage ) FS_WGT_SET_SHARE_HEIGHT( wPrevPage );
		FS_WidgetSetHandler( wPrevPage, FS_SnsRssChannelArticleListPrevPage_CB );
		FS_WindowAddWidget( win, wPrevPage );
	}
	
	if ( bUpdateScreen ) {
		FS_InvalidateRect( win, FS_NULL );
	}
	
	/* avoid been free by SnsLib */
	IFS_Memset( rsp, 0, sizeof(FS_SnsResponse) );
}

static void FS_SnsRssChannelArticleList_UI( FS_Window *win )
{
	FS_Window *swin;
	FS_Window *lwin;
	FS_Widget *wgt;
	FS_SnsRequest req = {0};
	FS_SINT4 msgCount = 0;
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_CHAR *channel_id, *channel_name;

	swin = FS_WindowFindId( FS_W_SnsRssFrm );
	if ( swin ) {
		wgt = FS_WindowGetFocusItem( swin );
		if ( wgt == FS_NULL ) return;
		if ( (FS_SINT4)wgt->private_data >= GFS_SnsApp.rsp_rss_channels.data.rss_channel.count ) return;
		channel_id = GFS_SnsApp.rsp_rss_channels.data.rss_channel.channels[wgt->private_data].id;
		channel_name = GFS_SnsApp.rsp_rss_channels.data.rss_channel.channels[wgt->private_data].channel_name;
	} else {
		swin = FS_WindowFindId( FS_W_SnsEntryFrm );
		wgt = FS_WindowGetFocusItem( swin );
		if ( wgt == FS_NULL ) return;
		if ( (FS_SINT4)wgt->private_data >= GFS_SnsApp.rsp_update.data.msglist.count ) return;
		channel_id = GFS_SnsApp.rsp_update.data.msglist.msgs[wgt->private_data].username;
		channel_name = GFS_SnsApp.rsp_update.data.msglist.msgs[wgt->private_data].author;
	}
	GFS_SnsApp.rss_channel_article_page = 1;
	req.req = FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL;
	req.data.rss.id = channel_id;
	req.data.rss.page = GFS_SnsApp.rss_channel_article_page;
	req.data.rss.count = config->msg_cnt_per_page;
	lwin = FS_CreateWindow( FS_W_SnsRssChannelArticleListFrm, channel_name ? channel_name : FS_Text(FS_T_RSS), FS_NULL );
	lwin->private_data = (FS_UINT4)channel_id;
	FS_WindowSetSoftkey( lwin, 1, FS_Text(FS_T_REFRESH), FS_SnsRssChannelArticleListUpdate_CB );
	FS_WindowSetSoftkey( lwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	
	FS_ShowWindow( lwin );
	
	FS_SnsLibClearCache( GFS_SnsApp.snslib, &req );
	FS_SnsUIRequestStart( lwin, &req, FS_FALSE );
}

static void FS_SnsRssArticleUpdate( FS_BOOL bCacheFirst )
{
	FS_SnsRequest req = {0};
	FS_SnsResponse rsp = {0};
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_Window *lwin = FS_WindowFindId( FS_W_SnsRssFrm );
	FS_BOOL bCacheOK = FS_FALSE;

	if ( lwin == FS_NULL ) return;

	req.req = FS_SNS_REQ_RSS_GET_ALL_ARTICLE;
	req.data.rss.count = config->msg_cnt_per_page;
	req.data.rss.page = GFS_SnsApp.rss_article_page;

	if ( bCacheFirst ) {
		bCacheOK = FS_SnsLibReadCache( GFS_SnsApp.snslib, &req, &rsp );
		if ( bCacheOK ) {
			FS_SnsRssShowArticleList( &rsp, req.req, FS_TRUE );
			return;
		}
	} else {
		FS_SnsLibClearCache( GFS_SnsApp.snslib, &req );
	}

	FS_SnsUIRequestStart( lwin, &req, FS_TRUE );
}

static void FS_SnsRssArticleNextPage_CB( FS_Window *win )
{
	GFS_SnsApp.backup_page_index = GFS_SnsApp.rss_article_page;
	GFS_SnsApp.rss_article_page ++;
	FS_SnsRssArticleUpdate( FS_TRUE );
	
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_SnsRssArticlePrevPage_CB( FS_Window *win )
{
	if ( GFS_SnsApp.rss_article_page > 1 )
	{
		GFS_SnsApp.backup_page_index = GFS_SnsApp.rss_article_page;
		GFS_SnsApp.rss_article_page --;
		FS_SnsRssArticleUpdate( FS_TRUE );
	}
	
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_SnsRssShowArticleList( FS_SnsResponse *rsp, FS_SnsReqCode req, FS_BOOL bUpdateScreen )
{
	FS_TabSheet *sheet = FS_NULL;
	FS_SINT4 i;
	FS_Widget *wItem, *wNextPage = FS_NULL, *wPrevPage = FS_NULL;
	FS_Window *win = FS_WindowFindId( FS_W_SnsRssFrm );
	FS_UINT4 pageIdx;
	FS_SnsConfig *config = FS_SnsGetConfig( );
	
	if ( win == FS_NULL ) return;
	
	pageIdx = GFS_SnsApp.rss_article_page;	
	/* save global data, just reference, do not copy */
	FS_SnsFreeResponse( &GFS_SnsApp.rsp_rss_articles, FS_SNS_REQ_RSS_GET_ALL_ARTICLE );
	IFS_Memcpy( &GFS_SnsApp.rsp_rss_articles, rsp, sizeof(FS_SnsResponse) );

	FS_SnsRssFormSetTitle( win, req, bUpdateScreen );
	FS_WindowDelTabSheetWidgetList( win, FS_W_SnsTabRssArticle );
	sheet = FS_WindowGetTabSheet( win, FS_W_SnsTabRssArticle );
	for ( i = 0; i < rsp->data.rss_article.count; i ++ ) {
		wItem = FS_CreateListItem( i, rsp->data.rss_article.articles[i].title, 
			FS_SnsUtilSecondsToDateTime(rsp->data.rss_article.articles[i].date), FS_I_RSS, 2 );
		wItem->private_data = (FS_UINT4)i;
		FS_WidgetSetHandler( wItem, FS_SnsRssArticleDetail_CB );
		FS_TabSheetAddWidget( sheet, wItem );
	}
	
	/* add page up/down widget */
	if ( rsp->data.rss_article.count >= config->msg_cnt_per_page ) {
		wNextPage = FS_CreateListItem( 0, FS_Text(FS_T_NEXT_PAGE), FS_NULL, 0, 1 );
		FS_WidgetSetHandler( wNextPage, FS_SnsRssArticleNextPage_CB );
		FS_TabSheetAddWidget( sheet, wNextPage );
	}
	if ( pageIdx > 1 ) {
		wPrevPage = FS_CreateListItem( 0, FS_Text(FS_T_PREV_PAGE), FS_NULL, 0, 1 );
		if ( wNextPage ) FS_WGT_SET_SHARE_HEIGHT( wPrevPage );
		FS_WidgetSetHandler( wPrevPage, FS_SnsRssArticlePrevPage_CB );
		FS_TabSheetAddWidget( sheet, wPrevPage );
	}
	
	if ( bUpdateScreen ) {
		FS_InvalidateRect( win, FS_NULL );
	}
	
	/* avoid been free by SnsLib */
	IFS_Memset( rsp, 0, sizeof(FS_SnsResponse) );
}

static void FS_SnsRssShowChannelList( FS_SnsResponse *rsp, FS_SnsReqCode req, FS_BOOL bUpdateScreen )
{
	FS_TabSheet *sheet = FS_NULL;
	FS_SINT4 i;
	FS_Widget *wItem, *wNextPage = FS_NULL, *wPrevPage = FS_NULL;
	FS_Window *win = FS_WindowFindId( FS_W_SnsRssFrm );
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_CHAR szTxt[16] = {0};
	FS_CHAR absFile[FS_MAX_PATH_LEN] = {0};

	if ( win == FS_NULL ) return;
	
	/* save global data, just reference, do not copy */
	FS_SnsFreeResponse( &GFS_SnsApp.rsp_rss_channels, FS_SNS_REQ_RSS_GET_ALL_CHANNEL );
	IFS_Memcpy( &GFS_SnsApp.rsp_rss_channels, rsp, sizeof(FS_SnsResponse) );
	
	FS_SnsRssFormSetTitle( win, req, bUpdateScreen );
	FS_WindowDelTabSheetWidgetList( win, FS_W_SnsTabRssChannel );
	sheet = FS_WindowGetTabSheet( win, FS_W_SnsTabRssChannel );
	for ( i = 0; i < rsp->data.rss_channel.count; i ++ ) {
		wItem = FS_CreateListItem( i, rsp->data.rss_channel.channels[i].channel_name, FS_NULL, 0, 1 );
		wItem->private_data = (FS_UINT4)i;
		if ( config->display_image ) {
			if ( rsp->data.rss_channel.channels[i].icon_file ) {
				FS_GetAbsFileName( FS_DIR_TMP, rsp->data.rss_channel.channels[i].icon_file, absFile );
				FS_WidgetSetIconFile( wItem, absFile );
			} else {
				FS_WidgetSetIcon( wItem, FS_I_RSS );
			}
		}
		FS_WidgetSetHandler( wItem, FS_SnsRssChannelArticleList_UI );
		FS_TabSheetAddWidget( sheet, wItem );
	}

	if ( bUpdateScreen ) {
		FS_InvalidateRect( win, FS_NULL );
	}
	
	/* avoid been free by SnsLib */
	IFS_Memset( rsp, 0, sizeof(FS_SnsResponse) );
}

static void FS_SnsRssCategorySetMyChannel_CB( FS_Window *win )
{
	FS_Widget *wgt;
	FS_List *node, *head = FS_WindowGetListItems( win );
	FS_CHAR szSelect[128] = {0}, szCancel[128] = {0};
	FS_SINT4 slen = 0, clen = 0;
	FS_CHAR *data = FS_NULL;
	FS_SnsRequest req = {0};

	node = head->next;
	while( node != head ) {
		wgt = FS_ListEntry( node, FS_Widget, list );
		node = node->next;
		if (wgt->type == FS_WGT_CHECK_BOX && wgt->private_data) {
			if ( FS_WGT_GET_CHECK(wgt) ) {
				if ( slen == 0 ) {
					slen += IFS_Snprintf( szSelect + slen, sizeof(szSelect) - 1 - slen, 
						"%s", (FS_CHAR *)wgt->private_data);
				} else {
					slen += IFS_Snprintf( szSelect + slen, sizeof(szSelect) - 1 - slen, 
						"_%s", (FS_CHAR *)wgt->private_data);
				}
			} else {
				if ( clen == 0 ) {
					clen += IFS_Snprintf( szCancel + clen, sizeof(szCancel) - 1 - clen, 
						"%s", (FS_CHAR *)wgt->private_data);
				} else {
					clen += IFS_Snprintf( szCancel + clen, sizeof(szCancel) - 1 - clen, 
						"_%s", (FS_CHAR *)wgt->private_data);
				}
			}
		}
	}

	data = FS_StrConCat( szSelect, ",", szCancel, FS_NULL );
	req.req = FS_SNS_REQ_RSS_SET_MY_CHANNEL;
	req.data.rss.id = data;
	FS_SnsUIRequestStart( win, &req, FS_FALSE );

	FS_SAFE_FREE( data );
}

static void FS_SnsRssSecondCategoryUpdate_CB( FS_Window *win )
{
	FS_SnsRequest req = {0};
	FS_Window *lwin = FS_WindowFindId( FS_W_SnsRssFrm );
	FS_Widget *wgt;
	FS_SnsRssCategory *channel;

	wgt = FS_WindowGetFocusItem( lwin );
	if ( wgt == FS_NULL ) return;	
	if ( (FS_SINT4)wgt->private_data >= GFS_SnsApp.rsp_rss_channel_category.data.rss_category.count ) return;
	channel = &GFS_SnsApp.rsp_rss_channel_category.data.rss_category.items[wgt->private_data];
	
	req.req = FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY;
	req.data.rss.id = channel->id;
	FS_SnsUIRequestStart( win, &req, FS_FALSE );
}

static void FS_SnsRssCategoryDetailSetTitle( FS_Window *win, FS_BOOL bUpdateScreen )
{
	FS_Widget *wgt;
	FS_SnsRssCategory *channel;
	FS_Window *swin = FS_WindowFindId( FS_W_SnsRssSecondCategoryFrm );
	FS_CHAR szText[128] = {0};
	
	wgt = FS_WindowGetFocusItem( swin );
	if ( wgt == FS_NULL ) return;	
	if ( (FS_SINT4)wgt->private_data >= GFS_SnsApp.rsp_rss_second_category.data.rss_category.count ) return;
	channel = &GFS_SnsApp.rsp_rss_second_category.data.rss_category.items[wgt->private_data];
	IFS_Snprintf( szText, sizeof(szText) - 1, "%s-%s", FS_Text(FS_T_BINDING), channel->name );
	FS_WindowSetTitle( win, szText );
	if ( bUpdateScreen ) {
		FS_RedrawWinTitle( win );
	}
}

static void FS_SnsRssSecondCategoryFormSetTitle( FS_Window *win, FS_BOOL bUpdateScreen )
{
	FS_Widget *wgt;
	FS_SnsRssCategory *channel;
	FS_Window *swin = FS_WindowFindId( FS_W_SnsRssFrm );
	FS_CHAR szText[128] = {0};
	
	wgt = FS_WindowGetFocusItem( swin );
	if ( wgt == FS_NULL ) return;	
	if ( (FS_SINT4)wgt->private_data >= GFS_SnsApp.rsp_rss_channel_category.data.rss_category.count ) return;
	channel = &GFS_SnsApp.rsp_rss_channel_category.data.rss_category.items[wgt->private_data];
	IFS_Snprintf( szText, sizeof(szText) - 1, "%s-%s", FS_Text(FS_T_BINDING), channel->name );
	FS_WindowSetTitle( win, szText );
	if ( bUpdateScreen ) {
		FS_RedrawWinTitle( win );
	}
}

static void FS_SnsRssShowCategoryDetail( FS_SnsResponse *rsp, FS_SnsReqCode req, FS_BOOL bUpdateScreen )
{
	FS_SINT4 i;
	FS_Widget *wItem;
	FS_Window *win = FS_WindowFindId( FS_W_SnsRssCategoryDetailFrm );
	FS_CHAR szText[128] = {0};

	if ( win == FS_NULL ) return;
	
	/* save global data, just reference, do not copy */
	FS_SnsFreeResponse( &GFS_SnsApp.rsp_rss_category_detail, FS_SNS_REQ_RSS_GET_CATEGORY_DETAIL );
	IFS_Memcpy( &GFS_SnsApp.rsp_rss_category_detail, rsp, sizeof(FS_SnsResponse) );
	
	FS_SnsRssCategoryDetailSetTitle( win, FS_FALSE );
	FS_WindowDelWidgetList( win );
	for ( i = 0; i < rsp->data.rss_category_detail.count; i ++ ) {
		IFS_Snprintf( szText, sizeof(szText) - 1, "%s(%s)", 
			rsp->data.rss_category_detail.items[i].name, rsp->data.rss_category_detail.items[i].language );
		wItem = FS_CreateCheckBox( i, szText );
		wItem->private_data = (FS_UINT4)rsp->data.rss_category_detail.items[i].id;
		FS_WidgetSetCheck( wItem, rsp->data.rss_category_detail.items[i].selected );
		FS_WindowAddWidget( win, wItem );
	}

	if ( bUpdateScreen ) {
		FS_InvalidateRect( win, FS_NULL );
	}
	
	/* avoid been free by SnsLib */
	IFS_Memset( rsp, 0, sizeof(FS_SnsResponse) );
}

static void FS_SnsRssCategoryDetail_UI( FS_Window *win )
{
	FS_Window *swin;
	FS_Window *lwin;
	FS_Widget *wgt;
	FS_SnsRequest req = {0};
	FS_SINT4 msgCount = 0;
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_SnsRssCategory *channel;
	
	swin = FS_WindowFindId( FS_W_SnsRssSecondCategoryFrm );
	wgt = FS_WindowGetFocusItem( swin );
	if ( wgt == FS_NULL ) return;	
	if ( (FS_SINT4)wgt->private_data >= GFS_SnsApp.rsp_rss_second_category.data.rss_category.count ) return;
	channel = &GFS_SnsApp.rsp_rss_second_category.data.rss_category.items[wgt->private_data];
	
	if ( channel->count == 0 )
	{
		FS_MessageBox( FS_MS_OK, FS_Text(FS_T_LIST_EMPTY), FS_NULL, FS_TRUE );
		return;
	}
	req.req = FS_SNS_REQ_RSS_GET_CATEGORY_DETAIL;
	req.data.rss.id = channel->id;
	lwin = FS_CreateWindow( FS_W_SnsRssCategoryDetailFrm, channel->name, FS_NULL );
	lwin->private_data = (FS_UINT4)channel->id;
	FS_WindowSetSoftkey( lwin, 1, FS_Text(FS_T_SAVE), FS_SnsRssCategorySetMyChannel_CB );
	FS_WindowSetSoftkey( lwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	FS_SnsRssCategoryDetailSetTitle( lwin, FS_FALSE );
	FS_ShowWindow( lwin );
	
	FS_SnsUIRequestStart( lwin, &req, FS_FALSE );
}

static void FS_SnsRssSecondCategory_UI( FS_Window *win )
{
	FS_Window *swin;
	FS_Window *lwin;
	FS_Widget *wgt;
	FS_SnsRequest req = {0};
	FS_SINT4 msgCount = 0;
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_SnsRssCategory *channel;
	
	swin = FS_WindowFindId( FS_W_SnsRssFrm );
	wgt = FS_WindowGetFocusItem( swin );
	if ( wgt == FS_NULL ) return;	
	if ( (FS_SINT4)wgt->private_data >= GFS_SnsApp.rsp_rss_channel_category.data.rss_category.count ) return;
	channel = &GFS_SnsApp.rsp_rss_channel_category.data.rss_category.items[wgt->private_data];
	
	if ( channel->count == 0 )
	{
		FS_MessageBox( FS_MS_OK, FS_Text(FS_T_LIST_EMPTY), FS_NULL, FS_TRUE );
		return;
	}
	req.req = FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY;
	req.data.rss.id = channel->id;
	lwin = FS_CreateWindow( FS_W_SnsRssSecondCategoryFrm, channel->name, FS_NULL );
	lwin->private_data = (FS_UINT4)channel->id;
	FS_WindowSetSoftkey( lwin, 1, FS_Text(FS_T_REFRESH), FS_SnsRssSecondCategoryUpdate_CB );
	FS_WindowSetSoftkey( lwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	FS_SnsRssSecondCategoryFormSetTitle( lwin, FS_FALSE );
	FS_ShowWindow( lwin );
	
	FS_SnsUIRequestStart( lwin, &req, FS_FALSE );
}

static void FS_SnsRssShowSecondCategoryList( FS_SnsResponse *rsp, FS_SnsReqCode req, FS_BOOL bUpdateScreen )
{
	FS_SINT4 i;
	FS_Widget *wItem;
	FS_Window *win = FS_WindowFindId( FS_W_SnsRssSecondCategoryFrm );
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_CHAR szTxt[16] = {0};
	FS_CHAR absFile[FS_MAX_PATH_LEN] = {0};
	
	if ( win == FS_NULL ) return;
	
	/* save global data, just reference, do not copy */
	FS_SnsFreeResponse( &GFS_SnsApp.rsp_rss_second_category, FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY );
	IFS_Memcpy( &GFS_SnsApp.rsp_rss_second_category, rsp, sizeof(FS_SnsResponse) );
	
	FS_SnsRssSecondCategoryFormSetTitle( win, bUpdateScreen );
	FS_WindowDelWidgetList( win );
	for ( i = 0; i < rsp->data.rss_category.count; i ++ ) {
		wItem = FS_CreateListItem( i, rsp->data.rss_category.items[i].name, FS_NULL, 0, 1 );
		wItem->private_data = (FS_UINT4)i;
		IFS_Snprintf( szTxt, sizeof(szTxt) - 1, "%d", rsp->data.rss_category.items[i].count );
		FS_WidgetSetExtraText( wItem, szTxt );
		if ( config->display_image ) {
			if ( rsp->data.rss_category.items[i].icon_file ) {
				FS_GetAbsFileName( FS_DIR_TMP, rsp->data.rss_category.items[i].icon_file, absFile );
				FS_WidgetSetIconFile( wItem, absFile );
			} else {
				FS_WidgetSetIcon( wItem, FS_I_RSS );
			}
		}
		FS_WidgetSetHandler( wItem, FS_SnsRssCategoryDetail_UI );
		FS_WindowAddWidget( win, wItem );
	}
	
	if ( bUpdateScreen ) {
		FS_InvalidateRect( win, FS_NULL );
	}
	
	/* avoid been free by SnsLib */
	IFS_Memset( rsp, 0, sizeof(FS_SnsResponse) );
}

static void FS_SnsRssShowChannelCategoryList( FS_SnsResponse *rsp, FS_SnsReqCode req, FS_BOOL bUpdateScreen )
{
	FS_TabSheet *sheet = FS_NULL;
	FS_SINT4 i;
	FS_Widget *wItem;
	FS_Window *win = FS_WindowFindId( FS_W_SnsRssFrm );
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_CHAR szTxt[16] = {0};
	FS_CHAR absFile[FS_MAX_PATH_LEN] = {0};
	
	if ( win == FS_NULL ) return;
	
	/* save global data, just reference, do not copy */
	FS_SnsFreeResponse( &GFS_SnsApp.rsp_rss_channel_category, FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY );
	IFS_Memcpy( &GFS_SnsApp.rsp_rss_channel_category, rsp, sizeof(FS_SnsResponse) );
	
	FS_SnsRssFormSetTitle( win, req, bUpdateScreen );
	FS_WindowDelTabSheetWidgetList( win, FS_W_SnsTabRssBind );
	sheet = FS_WindowGetTabSheet( win, FS_W_SnsTabRssBind );
	for ( i = 0; i < rsp->data.rss_category.count; i ++ ) {
		wItem = FS_CreateListItem( i, rsp->data.rss_category.items[i].name, FS_NULL, 0, 1 );
		wItem->private_data = (FS_UINT4)i;
		IFS_Snprintf( szTxt, sizeof(szTxt) - 1, "%d", rsp->data.rss_category.items[i].count );
		FS_WidgetSetExtraText( wItem, szTxt );
		if ( config->display_image ) {
			if ( rsp->data.rss_category.items[i].icon_file ) {
				FS_GetAbsFileName( FS_DIR_TMP, rsp->data.rss_category.items[i].icon_file, absFile );
				FS_WidgetSetIconFile( wItem, absFile );
			} else {
				FS_WidgetSetIcon( wItem, FS_I_RSS );
			}
		}
		FS_WidgetSetHandler( wItem, FS_SnsRssSecondCategory_UI );
		FS_TabSheetAddWidget( sheet, wItem );
	}
	
	if ( bUpdateScreen ) {
		FS_InvalidateRect( win, FS_NULL );
	}
	
	/* avoid been free by SnsLib */
	IFS_Memset( rsp, 0, sizeof(FS_SnsResponse) );
}

static void FS_SnsRssChannelCategoryUpdate_CB( FS_Window *win )
{
	FS_SnsRequest req = {0};
	FS_Window *lwin = FS_WindowFindId( FS_W_SnsRssFrm );
	
	req.req = FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY;
	if ( lwin ) {
		FS_SnsUIRequestStart( lwin, &req, FS_TRUE );
	}
}

static void FS_SnsRssChannelUpdate_CB( FS_Window *win )
{
	FS_SnsRequest req = {0};
	FS_Window *lwin = FS_WindowFindId( FS_W_SnsRssFrm );
	
	req.req = FS_SNS_REQ_RSS_GET_ALL_CHANNEL;
	if ( lwin ) {
		FS_SnsUIRequestStart( lwin, &req, FS_TRUE );
	}
}

static void FS_SnsRssArticleUpdate_CB( FS_Window *win )
{
	GFS_SnsApp.rss_article_page = 1;
	GFS_SnsApp.backup_page_index = GFS_SnsApp.rss_article_page;
	FS_SnsRssArticleUpdate( FS_FALSE );
	
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static FS_BOOL FS_SnsRssWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	if ( cmd == FS_WM_COMMAND && wparam == FS_EV_TAB_FOCUS_CHANGE ) {
		FS_TabSheet *tab = (FS_TabSheet *)lparam;
		
		/* cancel request if any */
		FS_SnsLibCancelRequest( GFS_SnsApp.snslib );
		FS_SnsUIRequestStop( );
		
		if ( tab->id == FS_W_SnsTabRssArticle ) {
			/* switch current msglist */
			FS_SnsRssFormSetTitle( win, FS_SNS_REQ_RSS_GET_ALL_ARTICLE, FS_TRUE );
			FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_REFRESH), FS_SnsRssArticleUpdate_CB );
			FS_RedrawSoftkeys( win );
		} else if ( tab->id == FS_W_SnsTabRssChannel ) {
			FS_SnsRequest req = {0};
			FS_SnsResponse rsp = {0};
			FS_BOOL bCacheOK = FS_FALSE;
			
			FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_REFRESH), FS_SnsRssChannelUpdate_CB );
			if ( win->private_data2 == 0 ) {
				win->private_data2 = 1;
				req.req = FS_SNS_REQ_RSS_GET_ALL_CHANNEL;
				bCacheOK = FS_SnsLibReadCache( GFS_SnsApp.snslib, &req, &rsp );
				if ( bCacheOK ) {
					FS_SnsRssShowChannelList( &rsp, req.req, FS_TRUE );
				}
			} else {
				FS_SnsRssFormSetTitle( win, FS_SNS_REQ_RSS_GET_ALL_CHANNEL, FS_TRUE );
				FS_RedrawSoftkeys( win );
			}
		} else if ( tab->id == FS_W_SnsTabRssBind ) {
			FS_SnsRssFormSetTitle( win, FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY, FS_TRUE );
			FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_REFRESH), FS_SnsRssChannelCategoryUpdate_CB );
			FS_RedrawSoftkeys( win );
		}
		
	} else if ( cmd == FS_WM_DESTROY ) {
		/* cancel request if any when this window destroy */
		FS_SnsFreeResponse( &GFS_SnsApp.rsp_rss_articles, FS_SNS_REQ_RSS_GET_ALL_ARTICLE );
		FS_SnsFreeResponse( &GFS_SnsApp.rsp_rss_channels, FS_SNS_REQ_RSS_GET_ALL_CHANNEL );
		FS_SnsFreeResponse( &GFS_SnsApp.rsp_rss_channel_articles, FS_SNS_REQ_RSS_GET_CHANNEL_DETAIL );
		FS_SnsFreeResponse( &GFS_SnsApp.rsp_rss_channel_category, FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY );
		FS_SnsFreeResponse( &GFS_SnsApp.rsp_rss_second_category, FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY );
		FS_SnsFreeResponse( &GFS_SnsApp.rsp_rss_category_detail, FS_SNS_REQ_RSS_GET_CATEGORY_DETAIL );
		FS_SnsLibCancelRequest( GFS_SnsApp.snslib );
	}
	return FS_FALSE;
}

static void FS_SnsRss_UI( FS_Window *win )
{
	FS_Window *mWin;
	FS_Widget *wLabel;
	FS_TabSheet *tabArticle, *tabChannel, *tabBind;
	FS_SnsRequest req = {0};
	FS_SnsResponse rsp = {0};
	FS_BOOL bCacheOK = FS_FALSE;
	FS_SnsConfig *config = FS_SnsGetConfig( );
		
	/* cancel request if any */
	FS_SnsLibCancelRequest( GFS_SnsApp.snslib );
	FS_SnsUIRequestStop( );
	
	mWin = FS_CreateWindow( FS_W_SnsRssFrm, FS_Text(FS_T_RSS), FS_SnsRssWndProc );
	
	tabArticle = FS_CreateTabSheet( FS_W_SnsTabRssArticle, FS_Text(FS_T_ARTICLE), FS_I_ARTICLE, FS_FALSE );
	tabChannel = FS_CreateTabSheet( FS_W_SnsTabRssChannel, FS_Text(FS_T_CHANNEL), FS_I_CHANNEL, FS_FALSE );
	tabBind = FS_CreateTabSheet( FS_W_SnsTabRssBind, FS_Text(FS_T_BINDING), FS_I_BIND, FS_FALSE );
	/* insert all tab sheet */
	FS_WindowSetSheetCountPerPage( mWin, 3 );
	FS_WindowAddTabSheet( mWin, tabArticle );
	FS_WindowAddTabSheet( mWin, tabChannel );
	FS_WindowAddTabSheet( mWin, tabBind );

	/* channel tab */
	wLabel =  FS_CreateLabel( 0, FS_Text(FS_T_PRESS_REFRESH), 0, 1 );
	FS_TabSheetAddWidget( tabChannel, wLabel );

	/* bind tab */
	IFS_Memset( &req, 0, sizeof(FS_SnsRequest) );
	IFS_Memset( &rsp, 0, sizeof(FS_SnsResponse) );
	req.req = FS_SNS_REQ_RSS_GET_CHANNEL_CATEGORY;
	bCacheOK = FS_SnsLibReadCache( GFS_SnsApp.snslib, &req, &rsp );
	if ( bCacheOK ) {
		FS_SnsRssShowChannelCategoryList( &rsp, req.req, FS_FALSE );
	} else {
		wLabel =  FS_CreateLabel( 0, FS_Text(FS_T_PRESS_REFRESH), 0, 1 );
		FS_TabSheetAddWidget( tabBind, wLabel );
	}

	/* article tab */
	req.req = FS_SNS_REQ_RSS_GET_ALL_ARTICLE;
	req.data.rss.page = GFS_SnsApp.rss_article_page;
	bCacheOK = FS_SnsLibReadCache( GFS_SnsApp.snslib, &req, &rsp );
	if ( bCacheOK ) {
		FS_SnsRssShowArticleList( &rsp, req.req, FS_FALSE );
	} else {
		FS_SnsRssFormSetTitle( mWin, FS_SNS_REQ_RSS_GET_ALL_ARTICLE, FS_FALSE );
		wLabel =  FS_CreateLabel( 0, FS_Text(FS_T_PRESS_REFRESH), 0, 1 );
		FS_TabSheetAddWidget( tabArticle, wLabel );
	}

	FS_WindowSetSoftkey( mWin, 1, FS_Text(FS_T_REFRESH), FS_SnsRssArticleUpdate_CB );
	FS_WindowSetSoftkey( mWin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	
	FS_ShowWindow( mWin );
}

static void FS_SnsEmlSend_CB( FS_Window *win )
{
	FS_Widget *wgt;
	FS_CHAR *szReceiver, *szSubject, *szContent;
	FS_SnsRequest req = {0};
	FS_SnsConfig *config = FS_SnsGetConfig( );
	
	wgt = FS_WindowGetWidget( win, FS_W_SnsEmlSendTo );
	szReceiver = wgt->text;
	if ( szReceiver == FS_NULL || szReceiver[0] == 0 )
	{
		FS_MessageBox(FS_MS_ALERT, FS_Text(FS_T_INPUT_ALL_DATA_PLS), FS_NULL, FS_TRUE);
		FS_WidgetSetFocus( win, wgt);
		return;
	}
	
	wgt = FS_WindowGetWidget( win, FS_W_SnsEmlSendSubject );
	szSubject = wgt->text;
	if ( szSubject == FS_NULL || szSubject[0] == 0 )
	{
		FS_MessageBox(FS_MS_ALERT, FS_Text(FS_T_INPUT_ALL_DATA_PLS), FS_NULL, FS_TRUE);
		FS_WidgetSetFocus( win, wgt);
		return;
	}
	
	wgt = FS_WindowGetWidget( win, FS_W_SnsEmlSendContent );
	szContent = wgt->text;	
	if (szContent == FS_NULL || szContent[0] == 0 )
	{
		FS_MessageBox(FS_MS_ALERT, FS_Text(FS_T_INPUT_ALL_DATA_PLS), FS_NULL, FS_TRUE);
		FS_WidgetSetFocus( win, wgt);
		return;
	}
	
	req.req = FS_SNS_REQ_EML_SEND;
	req.data.eml_send.receiver = szReceiver;
	req.data.eml_send.subject = szSubject;
	req.data.eml_send.content = szContent;
	FS_SnsUIRequestStart( win, &req, FS_FALSE );
}

static void FS_SnsEmlEdit_UI( FS_CHAR *receiver, FS_CHAR *subject, FS_CHAR *content )
{
	FS_Window *ewin;
	FS_Widget *wLabel, *wEdit;

	ewin = FS_CreateWindow( FS_W_SnsEmlReplyFrm, FS_Text(FS_T_SEND), FS_NULL );
	wLabel = FS_CreateLabel( 0, FS_Text(FS_T_RECEIVER), 0, 1 );
	FS_WindowAddWidget( ewin, wLabel );
	wEdit = FS_CreateEditBox( FS_W_SnsEmlSendTo, receiver, 0, 1, FS_NULL );
	FS_WidgetSetHandler( wEdit, FS_SnsEmlReceiverMenu_UI );
	FS_WGT_SET_FORCE_NO_STATUS_BAR( wEdit );
	FS_WindowAddWidget( ewin, wEdit );
	if( receiver == FS_NULL ) FS_WidgetSetFocus( ewin, wEdit );
	wLabel = FS_CreateLabel( 0, FS_Text(FS_T_SUBJECT), 0, 1 );
	FS_WindowAddWidget( ewin, wLabel );
	wEdit = FS_CreateEditBox( FS_W_SnsEmlSendSubject, subject, 0, 1, FS_NULL );
	FS_WGT_SET_FORCE_NO_STATUS_BAR( wEdit );
	FS_WindowAddWidget( ewin, wEdit );
	wLabel = FS_CreateLabel( 0, FS_Text(FS_T_CONTENT), 0, 1 );
	FS_WindowAddWidget( ewin, wLabel );
	wEdit = FS_CreateEditBox( FS_W_SnsEmlSendContent, content, 0, 2, FS_NULL );
	FS_WGT_SET_FORCE_MULTI_LINE( wEdit );
	FS_WGT_SET_FORCE_NO_STATUS_BAR( wEdit );
	FS_WGT_SET_FORCE_NO_STATUS_BAR( wEdit );
	FS_WindowAddWidget( ewin, wEdit );
	if( content == FS_NULL ) FS_WidgetSetFocus( ewin, wEdit );

	FS_WindowSetSoftkey( ewin, 1, FS_Text(FS_T_SEND), FS_SnsEmlSend_CB );
	FS_WindowSetSoftkey( ewin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );

	FS_ShowWindow( ewin );
}

static void FS_SnsEmlReply_UI( FS_Window *win )
{
	FS_CHAR szText[128] = {0};
	FS_CHAR *subject, *sender;
	FS_Widget *wgt;
	FS_Window *dwin = FS_WindowFindId( FS_W_SnsEmlDetailFrm );
	
	if ( dwin == FS_NULL ) return;
	wgt = FS_WindowGetWidget( dwin, FS_W_SnsEmlDetailSubject );
	if ( wgt == FS_NULL ) return;
	if ( FS_STR_NI_EQUAL(wgt->text, "re:", 3) ) {
		subject = wgt->text;
	} else {
		if ( wgt->text == FS_NULL ) {
			IFS_Strcpy( szText, "Re: " );
		} else {
			IFS_Snprintf( szText, sizeof(szText) - 1, "Re: %s", wgt->text );
		}
		subject = szText;
	}
	wgt = FS_WindowGetWidget( dwin, FS_W_SnsEmlDetailSender );
	if ( wgt == FS_NULL ) return;
	sender = wgt->text;
	
	FS_SnsEmlEdit_UI( sender, subject, FS_NULL );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_SnsEmlForward_UI( FS_Window *win )
{
	FS_CHAR szText[128] = {0};
	FS_CHAR *subject, *content;
	FS_Widget *wgt;
	FS_Window *dwin = FS_WindowFindId( FS_W_SnsEmlDetailFrm );

	if ( dwin == FS_NULL ) return;
	wgt = FS_WindowGetWidget( dwin, FS_W_SnsEmlDetailSubject );
	if ( wgt == FS_NULL ) return;
	if ( FS_STR_NI_EQUAL(wgt->text, "fw:", 3) ) {
		subject = wgt->text;
	} else {
		if ( wgt->text == FS_NULL ) {
			IFS_Strcpy( szText, "Fw: " );
		} else {
			IFS_Snprintf( szText, sizeof(szText) - 1, "Fw: %s", wgt->text );
		}
		subject = szText;
	}
	wgt = FS_WindowGetWidget( dwin, FS_W_SnsEmlDetailContent );
	if ( wgt == FS_NULL ) return;
	content = wgt->text;

	FS_SnsEmlEdit_UI( FS_NULL, subject, content );

	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_SnsEmlDetailMenu_UI( FS_Window *win )
{
	FS_Widget *iForward = FS_NULL, *iReply = FS_NULL;
	FS_Window *pMenu;
	FS_Window *mwin = FS_WindowFindId( FS_W_SnsEmailFrm );
	
	if ( mwin == FS_NULL ) mwin = FS_WindowFindId( FS_W_SnsEntryFrm );
	if ( mwin == FS_NULL ) return;
	
	iForward = FS_CreateMenuItem( 0, FS_Text(FS_T_FORWARD) );
	iReply = FS_CreateMenuItem( 0, FS_Text(FS_T_REPLY) );
	
	pMenu = FS_CreateMenu( 0, 2 );
	FS_MenuAddItem( pMenu, iForward );
	FS_WidgetSetHandler( iForward, FS_SnsEmlForward_UI );
	FS_MenuAddItem( pMenu, iReply );
	FS_WidgetSetHandler( iReply, FS_SnsEmlReply_UI );
	
	FS_MenuSetSoftkey( pMenu );
	
	FS_ShowWindow( pMenu );
}

static void FS_SnsEmlDetail_UI( FS_Window *lwin )
{
	FS_Widget *wSender,*wSubject, *wContent;
	FS_CHAR *sender = FS_NULL, *subject = FS_NULL, *content = FS_NULL, *date = FS_NULL;
	FS_Window *dwin;
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_Widget *wgt = FS_WindowGetFocusItem( lwin );
	FS_SnsEmail *email = FS_NULL;
	FS_CHAR *pMsg = FS_NULL, *str = FS_NULL, *id = FS_NULL, *type = FS_NULL;

	if ( wgt == FS_NULL ) return;

	if ( lwin->id == FS_W_SnsEmailFrm )
	{
		if ( (FS_SINT4)wgt->private_data >= GFS_SnsApp.rsp_eml_inbox.data.eml_list.count ) return;
		email = &GFS_SnsApp.rsp_eml_inbox.data.eml_list.emails[wgt->private_data];
		sender = email->sender;
		date = email->date;
		subject = email->subject;
		content = email->content;
		id = email->id;
	}
	else if ( lwin->id == FS_W_SnsEntryFrm ) 
	{
		if ( (FS_SINT4)wgt->private_data >= GFS_SnsApp.rsp_update.data.msglist.count ) return;
		sender = GFS_SnsApp.rsp_update.data.msglist.msgs[wgt->private_data].author;
		date = GFS_SnsApp.rsp_update.data.msglist.msgs[wgt->private_data].date;
		id = GFS_SnsApp.rsp_update.data.msglist.msgs[wgt->private_data].id;
		type = GFS_SnsApp.rsp_update.data.msglist.msgs[wgt->private_data].type;
		if ( GFS_SnsApp.rsp_update.data.msglist.msgs[wgt->private_data].msg ) {
			str = IFS_Strdup(GFS_SnsApp.rsp_update.data.msglist.msgs[wgt->private_data].msg);
			pMsg = str;
			subject = str;
			str = IFS_Strstr( str, "=--=" );
			if ( str )
			{
				*str = 0;
				str += 4;
			}
			content = str;
		}
	}

	dwin = FS_CreateWindow( FS_W_SnsEmlDetailFrm, FS_Text(FS_T_DETAIL), FS_NULL );
	wSender = FS_CreateListItem( FS_W_SnsEmlDetailSender, sender ? sender : "no sender", 
		FS_SnsUtilSecondsToDateTime(date), 0, 2 ); 
	wSubject = FS_CreateLabel( FS_W_SnsEmlDetailSubject, subject, 0, 1 ); 
	wContent = FS_CreateScroller( FS_W_SnsEmlDetailContent, FS_ProcessParagraph(content, -1) );
	
	FS_SAFE_FREE( pMsg );

	FS_WindowAddWidget( dwin, wSender );
	FS_WindowAddWidget( dwin, wSubject );
	FS_WindowAddWidget( dwin, wContent );

	FS_WindowSetSoftkey( dwin, 1, FS_Text(FS_T_MENU), FS_SnsEmlDetailMenu_UI );
	FS_WindowSetSoftkey( dwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	FS_ShowWindow( dwin );

	/* get email content */
	if ( id ) {
		FS_SnsRequest req = {0};
		
		if ( lwin->id == FS_W_SnsEmailFrm ) {
			req.req = FS_SNS_REQ_EML_RECV_CONTENT;
			req.data.eml_recv.id = id;
		} else {
			req.req = FS_SNS_REQ_GET_UPDATE_CONTENT;
			req.data.get_msgs.id = id;
			req.data.get_msgs.type = type;
		}
		FS_SnsUIRequestStart( dwin, &req, FS_FALSE );
	}
}

static void FS_SnsEmlInboxUpdate( FS_BOOL bCacheFirst )
{
	FS_SnsRequest req = {0};
	FS_SnsResponse rsp = {0};
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_Window *win = FS_WindowFindId( FS_W_SnsEmailFrm );
	FS_BOOL bCacheOK = FS_FALSE;

	req.req = FS_SNS_REQ_EML_RECV;
	req.data.eml_recv.page = GFS_SnsApp.eml_page;
	req.data.eml_recv.count = config->msg_cnt_per_page;
	if ( bCacheFirst ) {
		bCacheOK = FS_SnsLibReadCache( GFS_SnsApp.snslib, &req, &rsp );
		if ( bCacheOK ) {
			FS_SnsEmlShowInboxList( &rsp, req.req, FS_TRUE );
			return;
		}
	} else {
		FS_SnsLibClearCache( GFS_SnsApp.snslib, &req );
	}
	FS_SnsUIRequestStart( win, &req, FS_TRUE );	
}

static void FS_SnsEmlInboxUpdate_CB( FS_Window *win )
{	
	GFS_SnsApp.eml_page = 1;
	GFS_SnsApp.backup_page_index = GFS_SnsApp.eml_page;
	FS_SnsEmlInboxUpdate( FS_FALSE );

	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_SnsEmlInboxNextPage_CB( FS_Window *win )
{
	GFS_SnsApp.backup_page_index = GFS_SnsApp.eml_page;
	GFS_SnsApp.eml_page ++;
	FS_SnsEmlInboxUpdate( FS_TRUE );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_SnsEmlInboxPrevPage_CB( FS_Window *win )
{
	if ( GFS_SnsApp.eml_page > 1 ) 
	{
		GFS_SnsApp.backup_page_index = GFS_SnsApp.eml_page;
		GFS_SnsApp.eml_page --;
		FS_SnsEmlInboxUpdate( FS_TRUE );
	}
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_SnsEmlContactUpdate( FS_BOOL bCacheFirst )
{
	FS_SnsRequest req = {0};
	FS_SnsResponse rsp = {0};
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_Window *win;
	FS_BOOL bCacheOK = FS_FALSE;
	
	win = FS_WindowFindId( FS_W_SnsEmlContactSelectFrm );
	if ( win == FS_NULL ) {
		win = FS_WindowFindId( FS_W_SnsEmailFrm );
	}
	if ( win == FS_NULL ) return;

	req.req = FS_SNS_REQ_EML_GET_CONTACT;
	req.data.eml_contact.page = GFS_SnsApp.eml_contact_page;
	req.data.eml_contact.count = config->msg_cnt_per_page;
	if ( bCacheFirst ) {
		bCacheOK = FS_SnsLibReadCache( GFS_SnsApp.snslib, &req, &rsp );
		if ( bCacheOK ) {
			FS_SnsEmlShowContactList( &rsp, req.req, FS_TRUE );
			return;
		}
	} else {
		FS_SnsLibClearCache( GFS_SnsApp.snslib, &req );
	}
	FS_SnsUIRequestStart( win, &req, FS_TRUE );	
}

static void FS_SnsEmlContactUpdate_CB( FS_Window *win )
{	
	GFS_SnsApp.eml_contact_page = 1;
	GFS_SnsApp.backup_page_index = GFS_SnsApp.eml_contact_page;
	FS_SnsEmlContactUpdate( FS_FALSE );
	
	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU) )
		FS_DestroyWindow( win );
}

static void FS_SnsEmlContactPostUpdate_CB( void )
{
	FS_SnsEmlContactUpdate_CB( FS_NULL );
}

static void FS_SnsEmlContactNextPage_CB( FS_Window *win )
{
	GFS_SnsApp.backup_page_index = GFS_SnsApp.eml_contact_page;
	GFS_SnsApp.eml_contact_page ++;
	FS_SnsEmlContactUpdate( FS_TRUE );
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_SnsEmlContactPrevPage_CB( FS_Window *win )
{
	if ( GFS_SnsApp.eml_contact_page > 1 ) 
	{
		GFS_SnsApp.backup_page_index = GFS_SnsApp.eml_contact_page;
		GFS_SnsApp.eml_contact_page --;
		FS_SnsEmlContactUpdate( FS_TRUE );
	}
	
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_SnsEmlContactSave_CB( FS_Window *win )
{
	FS_SnsRequest req = {0};
	FS_Window *ewin = FS_WindowFindId( FS_W_SnsEmlContactEditFrm );
	FS_CHAR *name, *email, *id;

	if ( ewin == FS_NULL ) return;
	id = (FS_CHAR *)ewin->private_data;
	name = FS_WindowGetWidgetText( ewin, FS_W_SnsEmlContactName );
	email = FS_WindowGetWidgetText( ewin, FS_W_SnsEmlContactEmail );
	if ( name == FS_NULL || email == FS_NULL ) {
		FS_MessageBox(FS_MS_ALERT, FS_Text(FS_T_INPUT_ALL_DATA_PLS), FS_NULL, FS_TRUE);
		return;
	}

	if ( id == FS_NULL ) {
		req.req = FS_SNS_REQ_EML_ADD_CONTACT;
	} else {
		req.req = FS_SNS_REQ_EML_MOD_CONTACT;
	}
	req.data.eml_contact.id = id;
	req.data.eml_contact.name = name;
	req.data.eml_contact.email = email;

	FS_SnsUIRequestStart( ewin, &req, FS_FALSE );
}

static void FS_SnsEmlAddContact_UI( FS_CHAR *id, FS_CHAR *name, FS_CHAR *email )
{
	FS_Window *ewin;
	FS_Widget *wLabel, *wEdit;

	ewin = FS_CreateWindow( FS_W_SnsEmlContactEditFrm, FS_Text(FS_T_CONTACT), FS_NULL );
	ewin->private_data = (FS_UINT4)id;
	wLabel = FS_CreateLabel( 0, FS_Text(FS_T_USER_NAME), 0, 1 );
	FS_WindowAddWidget( ewin, wLabel );
	wEdit = FS_CreateEditBox( FS_W_SnsEmlContactName, name, 0, 1, FS_NULL );
	FS_WindowAddWidget( ewin, wEdit );
	wLabel = FS_CreateLabel( 0, FS_Text(FS_T_EMAIL), 0, 1 );
	FS_WindowAddWidget( ewin, wLabel );
	wEdit = FS_CreateEditBox( FS_W_SnsEmlContactEmail, email, 0, 1, FS_NULL );
	FS_WindowAddWidget( ewin, wEdit );
	
	FS_WindowSetSoftkey( ewin, 1, FS_Text(FS_T_SAVE), FS_SnsEmlContactSave_CB );
	FS_WindowSetSoftkey( ewin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );

	FS_ShowWindow( ewin );
}

static void FS_SnsEmlAddContact_CB( FS_Window *win )
{
	FS_SnsEmlAddContact_UI( FS_NULL, FS_NULL, FS_NULL );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_SnsEmlModContact_CB( FS_Window *win )
{
	FS_Window *lwin = FS_WindowFindId( FS_W_SnsEmailFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( lwin );
	FS_SnsEmlContact *contact;

	if ( wgt == FS_NULL ) return;
	if ( (FS_SINT4)wgt->private_data >= GFS_SnsApp.rsp_eml_contact.data.eml_contact.count ) return;
	contact = &GFS_SnsApp.rsp_eml_contact.data.eml_contact.contacts[wgt->private_data];

	FS_SnsEmlAddContact_UI( contact->id, contact->name, contact->email );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_SnsEmlDelContact_CB( FS_Window *win )
{
	FS_Window *lwin = FS_WindowFindId( FS_W_SnsEmailFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( lwin );
	FS_SnsEmlContact *contact;
	FS_SnsRequest req = {0};
	
	if ( wgt == FS_NULL ) return;
	if ( (FS_SINT4)wgt->private_data >= GFS_SnsApp.rsp_eml_contact.data.eml_contact.count ) return;
	contact = &GFS_SnsApp.rsp_eml_contact.data.eml_contact.contacts[wgt->private_data];
	
	req.req = FS_SNS_REQ_EML_DEL_CONTACT;
	req.data.eml_contact.id = contact->id;

	FS_SnsUIRequestStart( lwin, &req, FS_FALSE );

	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_SnsEmlContactSend_CB( FS_Window *win )
{
	FS_Window *lwin = FS_WindowFindId( FS_W_SnsEmailFrm );
	FS_Widget *wgt = FS_WindowGetFocusItem( lwin );
	FS_SnsEmlContact *contact;
	
	if ( wgt == FS_NULL ) return;
	if ( (FS_SINT4)wgt->private_data >= GFS_SnsApp.rsp_eml_contact.data.eml_contact.count ) return;
	contact = &GFS_SnsApp.rsp_eml_contact.data.eml_contact.contacts[wgt->private_data];

	wgt = FS_WindowGetWidget( lwin, FS_W_SnsEmlSendTo );
	if ( wgt == FS_NULL ) return;
	FS_WidgetSetText( wgt, contact->email );

	FS_WindowSetFocusTab( lwin, FS_W_SnsTabEmlSend );
	if( win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( win );
}

static void FS_SnsEmlContactSelectItem_CB( FS_Window *win )
{
	FS_Window *ewin;
	FS_SnsEmlContact *contact;
	FS_List *node, *head = FS_WindowGetListItems( win );
	FS_Widget *wgt;
	FS_CHAR szText[512] = {0};
	FS_SINT4 nlen = 0;
	FS_CHAR *pStr = FS_NULL;

	node = head->next;
	while( node != head ) {
		wgt = FS_ListEntry( node, FS_Widget, list );
		node = node->next;

		if ( wgt->type == FS_WGT_CHECK_BOX && FS_WGT_GET_CHECK(wgt) ) {
			if ( (FS_SINT4)wgt->private_data >= GFS_SnsApp.rsp_eml_contact.data.eml_contact.count ) return;
			contact = &GFS_SnsApp.rsp_eml_contact.data.eml_contact.contacts[wgt->private_data];
			if ( nlen == 0 ) {
				nlen += IFS_Snprintf( szText + nlen, sizeof(szText) - 1 - nlen, "%s", contact->email );
			} else {
				nlen += IFS_Snprintf( szText + nlen, sizeof(szText) - 1 - nlen, ",%s", contact->email );
			}
		}
	}

	ewin = FS_WindowFindId( FS_W_SnsEmlReplyFrm );
	if ( ewin == FS_NULL ) {
		ewin = FS_WindowFindId( FS_W_SnsEmailFrm );
	}
	if ( ewin == FS_NULL ) return;
	wgt = FS_WindowGetWidget( ewin, FS_W_SnsEmlSendTo );
	if ( wgt == FS_NULL ) return;
	if ( wgt->text != FS_NULL ) {
		pStr = FS_StrConCat( wgt->text, ",", szText, FS_NULL );
		FS_WidgetSetText( wgt, pStr );
		FS_SAFE_FREE( pStr );
	} else {
		FS_WidgetSetText( wgt, szText );
	}
	FS_DestroyWindow( win );
}

static void FS_SnsEmlContactItemMenu_UI( FS_Window *win )
{
	FS_Widget *iAdd, *iMod, *iDel, *iSend;
	FS_Window *pMenu;
	FS_Widget *wgt;
	FS_Rect rect;
	
	wgt = FS_WindowGetFocusItem( win );
	if ( wgt == FS_NULL ) return;
	rect = FS_GetWidgetDrawRect( wgt );
	
	pMenu = FS_CreatePopUpMenu( 0, &rect, 4 );
	
	iAdd = FS_CreateMenuItem( 0, FS_Text(FS_T_ADD_CONTACT) );
	iMod = FS_CreateMenuItem( 0, FS_Text(FS_T_MOD_CONTACT) );
	iDel = FS_CreateMenuItem( 0, FS_Text(FS_T_DEL_CONTACT) );
	iSend = FS_CreateMenuItem( 0, FS_Text(FS_T_SEND_EML) );

	FS_WidgetSetHandler( iAdd, FS_SnsEmlAddContact_CB );
	FS_WidgetSetHandler( iMod, FS_SnsEmlModContact_CB );
	FS_WidgetSetHandler( iDel, FS_SnsEmlDelContact_CB );
	FS_WidgetSetHandler( iSend, FS_SnsEmlContactSend_CB );

	FS_MenuAddItem( pMenu, iAdd );
	FS_MenuAddItem( pMenu, iMod );
	FS_MenuAddItem( pMenu, iDel );
	FS_MenuAddItem( pMenu, iSend );
	
	FS_MenuSetSoftkey( pMenu );
	
	FS_ShowWindow( pMenu );
}

static void FS_SnsEmailFormSetTitle( FS_Window *win, FS_SnsReqCode reqCode, FS_BOOL bUpdateScreen )
{
	FS_CHAR *szTitle;
	FS_CHAR szPage[16] = {0};
	
	if ( win->id == FS_W_SnsEmailFrm ) {
		if ( reqCode == FS_SNS_REQ_EML_RECV ) {
			IFS_Snprintf( szPage, sizeof(szPage) - 1, "(%d)", GFS_SnsApp.eml_page );
			szTitle = FS_StrConCat( FS_Text(FS_T_EMAIL), "-", FS_Text(FS_T_INBOX), szPage );
		} else if ( reqCode == FS_SNS_REQ_EML_SEND ) {
			szTitle = FS_StrConCat( FS_Text(FS_T_EMAIL), "-", FS_Text(FS_T_COMPOSE), FS_NULL );
		} else if ( reqCode == FS_SNS_REQ_EML_SET_ACOUNT || reqCode == FS_SNS_REQ_EML_GET_ACOUNT ) {
			szTitle = FS_StrConCat( FS_Text(FS_T_EMAIL), "-", FS_Text(FS_T_EML_ACT), FS_NULL );
		} else if ( reqCode == FS_SNS_REQ_EML_GET_CONTACT ) {
			IFS_Snprintf( szPage, sizeof(szPage) - 1, "(%d)", GFS_SnsApp.eml_contact_page );
			szTitle = FS_StrConCat( FS_Text(FS_T_EMAIL), "-", FS_Text(FS_T_CONTACT), szPage );
		} else {
			return;
		}
	} else if ( win->id == FS_W_SnsEmlContactSelectFrm ) {
		IFS_Snprintf( szPage, sizeof(szPage) - 1, "(%d)", GFS_SnsApp.eml_contact_page );
		szTitle = FS_StrConCat( FS_Text(FS_T_CONTACT), szPage, FS_NULL, FS_NULL );
	} else {
		return;
	}
	FS_WindowSetTitle( win, szTitle );
	FS_SAFE_FREE( szTitle );
	if ( bUpdateScreen ) {
		FS_RedrawWinTitle( win );
	}
}

static void FS_SnsEmlShowInboxList( FS_SnsResponse *rsp, FS_SnsReqCode req, FS_BOOL bUpdateScreen )
{
	FS_SINT4 i;
	FS_Widget *wItem, *wNextPage = FS_NULL, *wPrevPage = FS_NULL;
	FS_Window *win = FS_WindowFindId( FS_W_SnsEmailFrm );
	FS_TabSheet *sheet;
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_UINT4 pageIdx = GFS_SnsApp.eml_page;

	if ( win == FS_NULL ) return;	
	/* save global data, just reference, do not copy */
	FS_SnsFreeResponse( &GFS_SnsApp.rsp_eml_inbox, FS_SNS_REQ_EML_RECV );
	IFS_Memcpy( &GFS_SnsApp.rsp_eml_inbox, rsp, sizeof(FS_SnsResponse) );
	
	FS_SnsEmailFormSetTitle( win, req, bUpdateScreen );
	FS_WindowDelTabSheetWidgetList( win, FS_W_SnsTabEmlInbox );
	sheet = FS_WindowGetTabSheet( win, FS_W_SnsTabEmlInbox );
	for ( i = 0; i < rsp->data.eml_list.count; i ++ ) {
		wItem = FS_CreateListItem( i, rsp->data.eml_list.emails[i].sender ? 
			rsp->data.eml_list.emails[i].sender : "no sender", 
			FS_SnsUtilSecondsToDateTime(rsp->data.eml_list.emails[i].date), FS_I_EML, 2 );
		wItem->private_data = (FS_UINT4)i;
		FS_WidgetSetHandler( wItem, FS_SnsEmlDetail_UI );
		FS_TabSheetAddWidget( sheet, wItem );
	}

	/* add page up/down widget */
	if ( rsp->data.eml_list.count >= config->msg_cnt_per_page ) {
		wNextPage = FS_CreateListItem( 0, FS_Text(FS_T_NEXT_PAGE), FS_NULL, 0, 1 );
		FS_WidgetSetHandler( wNextPage, FS_SnsEmlInboxNextPage_CB );
		FS_TabSheetAddWidget( sheet, wNextPage );
	}
	if ( pageIdx > 1 ) {
		wPrevPage = FS_CreateListItem( 0, FS_Text(FS_T_PREV_PAGE), FS_NULL, 0, 1 );
		if ( wNextPage ) FS_WGT_SET_SHARE_HEIGHT( wPrevPage );
		FS_WidgetSetHandler( wPrevPage, FS_SnsEmlInboxPrevPage_CB );
		FS_TabSheetAddWidget( sheet, wPrevPage );
	}
	
	if ( bUpdateScreen ) {
		FS_InvalidateRect( win, FS_NULL );
	}
	
	/* avoid been free by SnsLib */
	IFS_Memset( rsp, 0, sizeof(FS_SnsResponse) );
}

static void FS_SnsEmlShowContactList( FS_SnsResponse *rsp, FS_SnsReqCode req, FS_BOOL bUpdateScreen )
{
	FS_SINT4 i;
	FS_Widget *wItem, *wNextPage = FS_NULL, *wPrevPage = FS_NULL;
	FS_Window *win;
	FS_TabSheet *sheet;
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_UINT4 pageIdx = GFS_SnsApp.eml_contact_page;

	win = FS_WindowFindId( FS_W_SnsEmlContactSelectFrm );
	if ( win == FS_NULL ) {
		win = FS_WindowFindId( FS_W_SnsEmailFrm );
	}
	if ( win == FS_NULL ) return;

	/* save global data, just reference, do not copy */
	FS_SnsFreeResponse( &GFS_SnsApp.rsp_eml_contact, FS_SNS_REQ_EML_GET_CONTACT );
	IFS_Memcpy( &GFS_SnsApp.rsp_eml_contact, rsp, sizeof(FS_SnsResponse) );
	
	FS_SnsEmailFormSetTitle( win, req, bUpdateScreen );
	if ( win->id == FS_W_SnsEmailFrm ) {
		FS_WindowDelTabSheetWidgetList( win, FS_W_SnsTabEmlContact );
		sheet = FS_WindowGetTabSheet( win, FS_W_SnsTabEmlContact );
		for ( i = 0; i < rsp->data.eml_contact.count; i ++ ) {
			wItem = FS_CreateListItem( i, rsp->data.eml_contact.contacts[i].name ? rsp->data.eml_contact.contacts[i].name : FS_Text(FS_T_NONE), 
				rsp->data.eml_contact.contacts[i].email, 0, 2 );
			wItem->private_data = i;
			FS_WidgetSetHandler( wItem, FS_SnsEmlContactItemMenu_UI );
			FS_TabSheetAddWidget( sheet, wItem );
		}
	} else if ( win->id == FS_W_SnsEmlContactSelectFrm ) {
		FS_WindowDelWidgetList( win );
		for ( i = 0; i < rsp->data.eml_contact.count; i ++ ) {
			wItem = FS_CreateCheckBox( i, rsp->data.eml_contact.contacts[i].email );
			wItem->private_data = i;
			FS_WindowAddWidget( win, wItem );
		}
	}
	/* add page up/down widget */
	if ( rsp->data.eml_contact.count >= config->msg_cnt_per_page ) {
		wNextPage = FS_CreateListItem( 0, FS_Text(FS_T_NEXT_PAGE), FS_NULL, 0, 1 );
		FS_WidgetSetHandler( wNextPage, FS_SnsEmlContactNextPage_CB );
		if ( win->id == FS_W_SnsEmailFrm ) {
			FS_TabSheetAddWidget( sheet, wNextPage );
		} else {
			FS_WindowAddWidget( win, wNextPage );
		}
	}
	if ( pageIdx > 1 ) {
		wPrevPage = FS_CreateListItem( 0, FS_Text(FS_T_PREV_PAGE), FS_NULL, 0, 1 );
		if ( wNextPage ) FS_WGT_SET_SHARE_HEIGHT( wPrevPage );
		FS_WidgetSetHandler( wPrevPage, FS_SnsEmlContactPrevPage_CB );
		if ( win->id == FS_W_SnsEmailFrm ) {
			FS_TabSheetAddWidget( sheet, wPrevPage );
		} else {
			FS_WindowAddWidget( win, wPrevPage );
		}
	}
	
	if ( bUpdateScreen ) {
		FS_InvalidateRect( win, FS_NULL );
	}
	
	/* avoid been free by SnsLib */
	IFS_Memset( rsp, 0, sizeof(FS_SnsResponse) );
}

static void FS_SnsEmlSaveAccount_CB( FS_Window *win )
{
	FS_Widget *wgt;
	FS_CHAR *szPass, *szEmail, *szSmtp, *szPop3;
	FS_SnsRequest req = {0};
	FS_BOOL bBindHome = FS_FALSE;

	wgt = FS_WindowGetWidget( win, FS_W_SnsEmlActEmail );
	szEmail = wgt->text;
	if ( szEmail == FS_NULL || szEmail[0] == 0 )
	{
		FS_MessageBox(FS_MS_ALERT, FS_Text(FS_T_PLS_INPUT_EMAIL), FS_NULL, FS_TRUE);
		FS_WidgetSetFocus( win, wgt);
		return;
	}
	
	wgt = FS_WindowGetWidget( win, FS_W_SnsEmlActPassword );
	szPass = wgt->text;	
	if (szPass == FS_NULL || szPass[0] == 0 )
	{
		FS_MessageBox(FS_MS_ALERT, FS_Text(FS_T_PLS_INPUT_PWD), FS_NULL, FS_TRUE);
		FS_WidgetSetFocus( win, wgt);
		return;
	}
	
	wgt = FS_WindowGetWidget( win, FS_W_SnsEmlActSmtp );
	szSmtp = wgt->text;
	if ( szSmtp == FS_NULL || szSmtp[0] == 0 )
	{
		FS_MessageBox(FS_MS_ALERT, FS_Text(FS_T_INPUT_ALL_DATA_PLS), FS_NULL, FS_TRUE);
		FS_WidgetSetFocus( win, wgt);
		return;
	}
	
	wgt = FS_WindowGetWidget( win, FS_W_SnsEmlActPop3 );
	szPop3 = wgt->text;
	if ( szPop3 == FS_NULL || szPop3[0] == 0 )
	{
		FS_MessageBox(FS_MS_ALERT, FS_Text(FS_T_INPUT_ALL_DATA_PLS), FS_NULL, FS_TRUE);
		FS_WidgetSetFocus( win, wgt);
		return;
	}
	wgt = FS_WindowGetWidget( win, FS_W_SnsEmlActBindHome );
	bBindHome = (FS_BOOL)FS_WGT_GET_CHECK( wgt );

	req.req = FS_SNS_REQ_EML_SET_ACOUNT;
	req.data.eml_account.email = szEmail;
	req.data.eml_account.password = szPass;
	req.data.eml_account.smtp = szSmtp;
	req.data.eml_account.pop3 = szPop3;
	req.data.eml_account.bind_home = bBindHome;
	FS_SnsUIRequestStart( win, &req, FS_FALSE );
}

static void FS_SnsEmlShowAccount( FS_SnsResponse *rsp )
{
	FS_Widget *wgt;
	FS_Window *win = FS_WindowFindId( FS_W_SnsEmailFrm );

	if ( win == FS_NULL ) return;
	win->private_data = 1;
	FS_SnsEmailFormSetTitle( win, FS_SNS_REQ_EML_SET_ACOUNT, FS_FALSE );
	wgt = FS_WindowGetWidget( win, FS_W_SnsEmlActEmail );
	FS_WidgetSetText( wgt, rsp->data.eml_account.email );
	wgt = FS_WindowGetWidget( win, FS_W_SnsEmlActSmtp );
	FS_WidgetSetText( wgt, rsp->data.eml_account.smtp );
	wgt = FS_WindowGetWidget( win, FS_W_SnsEmlActPop3 );
	FS_WidgetSetText( wgt, rsp->data.eml_account.pop3 );
	wgt = FS_WindowGetWidget( win, FS_W_SnsEmlActPassword );
	FS_WidgetSetText( wgt, rsp->data.eml_account.password );
	wgt = FS_WindowGetWidget( win, FS_W_SnsEmlActBindHome );
	FS_WidgetSetCheck( wgt, rsp->data.eml_account.bind_home );

	FS_InvalidateRect( win, FS_NULL );
}

static void FS_SnsEmlGetAccount_CB( FS_Window *win )
{
	FS_SnsRequest req = {0};

	req.req = FS_SNS_REQ_EML_GET_ACOUNT;
	FS_SnsUIRequestStart( win, &req, FS_TRUE );
}

static void FS_SnsEmlGetAccountPost_CB( void )
{
	FS_Window *win = FS_WindowFindId( FS_W_SnsEmailFrm );
	
	if ( win == FS_NULL ) return;
	FS_SnsEmlGetAccount_CB( win );
}

static void FS_SnsEmlReceiverEdit_CB( FS_Window * win )
{
	FS_Window *ewin;

	ewin = FS_WindowFindId( FS_W_SnsEmlReplyFrm );
	if ( ewin == FS_NULL ) {
		ewin = FS_WindowFindId( FS_W_SnsEmailFrm );
	}
	if ( ewin == FS_NULL ) return;
	FS_StdEditBoxHandler( ewin );

	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU ) )
		FS_DestroyWindow( win );
}

static void FS_SnsEmlReceiverDel_CB( FS_Window * win )
{
	FS_Window *ewin;
	FS_Widget *wgt;

	ewin = FS_WindowFindId( FS_W_SnsEmlReplyFrm );
	if ( ewin == FS_NULL ) {
		ewin = FS_WindowFindId( FS_W_SnsEmailFrm );
	}
	if ( ewin == FS_NULL ) return;
	wgt = FS_WindowGetFocusItem( ewin );
	FS_WidgetSetText( wgt, FS_NULL );

	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU ) )
		FS_DestroyWindow( win );
}

static FS_BOOL FS_SnsEmlSelectContactWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	if ( cmd == FS_WM_COMMAND && wparam == FS_EV_ITEM_VALUE_CHANGE ) {
		FS_List *node, *head = FS_WindowGetListItems( win );
		FS_Widget *wgt;
		FS_BOOL bItemSelected = FS_FALSE;

		node = head->next;
		while( node != head ) {
			wgt = FS_ListEntry( node, FS_Widget, list );
			node = node->next;
			if ( wgt->type == FS_WGT_CHECK_BOX && FS_WGT_GET_CHECK(wgt) ) {
				bItemSelected = FS_TRUE;
				break;
			}
		}

		if ( bItemSelected ) {
			FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_OK), FS_SnsEmlContactSelectItem_CB );
		} else {
			FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_REFRESH), FS_SnsEmlContactUpdate_CB );
		}
		FS_RedrawSoftkeys( win );
	}

	return FS_FALSE;
}

static void FS_SnsEmlSelectContact_UI( FS_Window * win )
{
	FS_Window *lwin;
	FS_SnsRequest req = {0};
	FS_SnsResponse rsp = {0};
	FS_BOOL bCacheOK = FS_FALSE;
	FS_Widget *wLabel;

	lwin = FS_CreateWindow( FS_W_SnsEmlContactSelectFrm, FS_Text(FS_T_CONTACT), FS_SnsEmlSelectContactWndProc );

	/* contact tab */
	req.req = FS_SNS_REQ_EML_GET_CONTACT;
	req.data.eml_contact.page = GFS_SnsApp.eml_contact_page;
	bCacheOK = FS_SnsLibReadCache( GFS_SnsApp.snslib, &req, &rsp );
	if ( bCacheOK ) {
		FS_SnsEmlShowContactList( &rsp, req.req, FS_FALSE );
	} else {
		wLabel =  FS_CreateLabel( 0, FS_Text(FS_T_PRESS_REFRESH), 0, 1 );
		FS_WindowAddWidget( lwin, wLabel );
	}
	
	FS_WindowSetSoftkey( lwin, 1, FS_Text(FS_T_REFRESH), FS_SnsEmlContactUpdate_CB );
	FS_WindowSetSoftkey( lwin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	
	FS_ShowWindow( lwin );

	if( win && (win->type == FS_WT_MENU || win->type == FS_WT_POPUP_MENU ) )
		FS_DestroyWindow( win );
}

static void FS_SnsEmlReceiverMenu_UI( FS_Window *win )
{
	FS_Widget *iEdit, *iSelect, *iClear;
	FS_Window *pMenu;
	FS_Widget *wgt;
	FS_Rect rect;
	
	wgt = FS_WindowGetFocusItem( win );
	if ( wgt == FS_NULL ) return;
	rect = FS_GetWidgetDrawRect( wgt );
	
	pMenu = FS_CreatePopUpMenu( 0, &rect, 3 );
	
	iEdit = FS_CreateMenuItem( 0, FS_Text(FS_T_EDIT) );
	iSelect = FS_CreateMenuItem( 0, FS_Text(FS_T_SELECT) );
	iClear = FS_CreateMenuItem( 0, FS_Text(FS_T_DEL_ALL) );

	FS_WidgetSetHandler( iEdit, FS_SnsEmlReceiverEdit_CB );
	FS_WidgetSetHandler( iSelect, FS_SnsEmlSelectContact_UI );
	FS_WidgetSetHandler( iClear, FS_SnsEmlReceiverDel_CB );

	FS_MenuAddItem( pMenu, iEdit );
	FS_MenuAddItem( pMenu, iSelect );
	FS_MenuAddItem( pMenu, iClear );
	
	FS_MenuSetSoftkey( pMenu );
	
	FS_ShowWindow( pMenu );
}

static FS_BOOL FS_SnsEmailWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	if ( cmd == FS_WM_COMMAND && wparam == FS_EV_TAB_FOCUS_CHANGE ) {
		FS_TabSheet *tab = (FS_TabSheet *)lparam;
		
		/* cancel request if any */
		FS_SnsLibCancelRequest( GFS_SnsApp.snslib );
		FS_SnsUIRequestStop( );
		
		if ( tab->id == FS_W_SnsTabEmlInbox ) {
			FS_SnsEmailFormSetTitle( win, FS_SNS_REQ_EML_RECV, FS_TRUE );
			FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_REFRESH), FS_SnsEmlInboxUpdate_CB );
			FS_RedrawSoftkeys( win );
		} else if ( tab->id == FS_W_SnsTabEmlSend ) {
			FS_SnsEmailFormSetTitle( win, FS_SNS_REQ_EML_SEND, FS_TRUE );
			FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_SEND), FS_SnsEmlSend_CB );
			FS_RedrawSoftkeys( win );
		} else if ( tab->id == FS_W_SnsTabEmlContact ) {
			FS_SnsRequest req = {0};
			FS_SnsResponse rsp = {0};
			FS_BOOL bCacheOK = FS_FALSE;
			FS_Widget *wLabel;

			req.req = FS_SNS_REQ_EML_GET_CONTACT;
			req.data.eml_contact.page = GFS_SnsApp.eml_contact_page;
			bCacheOK = FS_SnsLibReadCache( GFS_SnsApp.snslib, &req, &rsp );
			if ( bCacheOK ) {
				FS_SnsEmlShowContactList( &rsp, req.req, FS_FALSE );
			} else {
				FS_SnsEmailFormSetTitle( win, FS_SNS_REQ_EML_GET_CONTACT, FS_FALSE );
				if ( FS_NULL == FS_WindowGetWidget( win, FS_W_SnsEmlAddContact ) ) {
					wLabel =  FS_CreateListItem( FS_W_SnsEmlAddContact, FS_Text(FS_T_ADD_CONTACT), FS_NULL, 0, 1 );
					FS_WidgetSetHandler( wLabel, FS_SnsEmlAddContact_CB );
					FS_TabSheetAddWidget( tab, wLabel );
				}
			}
			FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_REFRESH), FS_SnsEmlContactUpdate_CB );
			FS_InvalidateRect( win, FS_NULL );
		} else if ( tab->id == FS_W_SnsTabEmlAccount ) {
			FS_SnsEmailFormSetTitle( win, FS_SNS_REQ_EML_SET_ACOUNT, FS_TRUE );
			FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_SAVE), FS_SnsEmlSaveAccount_CB );
			if ( win->private_data ) {
				/* already have email account */
				FS_RedrawSoftkeys( win );
			} else {
				/* retrieve account from server */
				FS_SnsEmlGetAccount_CB( win );
			}
		}
	} else if ( cmd == FS_WM_DESTROY ) {
		/* cancel request if any when this window destroy */
		FS_SnsFreeResponse( &GFS_SnsApp.rsp_eml_inbox, FS_SNS_REQ_EML_RECV );
		FS_SnsFreeResponse( &GFS_SnsApp.rsp_eml_contact, FS_SNS_REQ_EML_GET_CONTACT );
		FS_SnsLibCancelRequest( GFS_SnsApp.snslib );
		FS_SnsUIRequestStop( );
	}
	return FS_FALSE;
}

static void FS_SnsEmail_UI( FS_Window *win )
{
	FS_Window *mWin;
	FS_Widget *wLabel, *wEdit;
	FS_TabSheet *tabInbox, *tabSend, *tabAccount, *tabContact;
	FS_SnsRequest req = {0};
	FS_SnsResponse rsp = {0};
	FS_BOOL bCacheOK = FS_FALSE;

	/* cancel request if any */
	FS_SnsLibCancelRequest( GFS_SnsApp.snslib );
	FS_SnsUIRequestStop( );
	
	mWin = FS_CreateWindow( FS_W_SnsEmailFrm, FS_Text(FS_T_EMAIL), FS_SnsEmailWndProc );
	
	tabInbox = FS_CreateTabSheet( FS_W_SnsTabEmlInbox, FS_Text(FS_T_INBOX), FS_I_UPDATES, FS_FALSE );
	tabSend = FS_CreateTabSheet( FS_W_SnsTabEmlSend, FS_Text(FS_T_COMPOSE), FS_I_COMPOSE, FS_FALSE );
	tabContact = FS_CreateTabSheet( FS_W_SnsTabEmlContact, FS_Text(FS_T_CONTACT), FS_I_CONTACT, FS_FALSE );
	tabAccount = FS_CreateTabSheet( FS_W_SnsTabEmlAccount, FS_Text(FS_T_EML_ACT), FS_I_EML_ACT, FS_FALSE );

	/* insert all tab sheet */
	FS_WindowSetSheetCountPerPage( mWin, 4 );
	FS_WindowAddTabSheet( mWin, tabInbox );
	FS_WindowAddTabSheet( mWin, tabSend );
	FS_WindowAddTabSheet( mWin, tabContact );
	FS_WindowAddTabSheet( mWin, tabAccount );

	/* inbox tab */
	req.req = FS_SNS_REQ_EML_RECV;
	req.data.eml_recv.page = GFS_SnsApp.eml_page;
	bCacheOK = FS_SnsLibReadCache( GFS_SnsApp.snslib, &req, &rsp );
	if ( bCacheOK ) {
		FS_SnsEmlShowInboxList( &rsp, req.req, FS_FALSE );
	} else {
		wLabel =  FS_CreateLabel( 0, FS_Text(FS_T_PRESS_REFRESH), 0, 1 );
		FS_TabSheetAddWidget( tabInbox, wLabel );
	}

	/* send tab */
	wLabel = FS_CreateLabel( 0, FS_Text(FS_T_RECEIVER), 0, 1 );
	FS_TabSheetAddWidget( tabSend, wLabel );
	wEdit = FS_CreateEditBox( FS_W_SnsEmlSendTo, FS_NULL, 0, 1, FS_NULL );
	FS_WidgetSetHandler( wEdit, FS_SnsEmlReceiverMenu_UI );
	FS_TabSheetAddWidget( tabSend, wEdit );
	wLabel = FS_CreateLabel( 0, FS_Text(FS_T_SUBJECT), 0, 1 );
	FS_TabSheetAddWidget( tabSend, wLabel );
	wEdit = FS_CreateEditBox( FS_W_SnsEmlSendSubject, FS_NULL, 0, 1, FS_NULL );
	FS_TabSheetAddWidget( tabSend, wEdit );
	wLabel = FS_CreateLabel( 0, FS_Text(FS_T_CONTENT), 0, 1 );
	FS_TabSheetAddWidget( tabSend, wLabel );
	wEdit = FS_CreateEditBox( FS_W_SnsEmlSendContent, FS_NULL, 0, 2, FS_NULL );
	FS_WGT_SET_FORCE_MULTI_LINE( wEdit );
	FS_WGT_SET_FORCE_NO_STATUS_BAR( wEdit );
	FS_TabSheetAddWidget( tabSend, wEdit );

	/* contact tab need to load when tab get focus */
	
	/* accout tab */
	req.req = FS_SNS_REQ_EML_GET_ACOUNT;
	FS_SnsLibReadCache( GFS_SnsApp.snslib, &req, &rsp );

	wLabel = FS_CreateLabel( 0, FS_Text(FS_T_EMAIL), 0, 1 );
	FS_TabSheetAddWidget( tabAccount, wLabel );
	wEdit = FS_CreateEditBox( FS_W_SnsEmlActEmail, rsp.data.eml_account.email, 0, 1, FS_NULL );
	FS_TabSheetAddWidget( tabAccount, wEdit );
	wLabel = FS_CreateLabel( 0, FS_Text(FS_T_EML_TIP_SMTP_ADDR), 0, 1 );
	FS_TabSheetAddWidget( tabAccount, wLabel );
	wEdit = FS_CreateEditBox( FS_W_SnsEmlActSmtp, rsp.data.eml_account.smtp, 0, 1, FS_NULL );
	FS_TabSheetAddWidget( tabAccount, wEdit );
	wLabel = FS_CreateLabel( 0, FS_Text(FS_T_EML_TIP_POP3_ADDR), 0, 1 );
	FS_TabSheetAddWidget( tabAccount, wLabel );
	wEdit = FS_CreateEditBox( FS_W_SnsEmlActPop3, rsp.data.eml_account.pop3, 0, 1, FS_NULL );
	FS_TabSheetAddWidget( tabAccount, wEdit );
	wLabel = FS_CreateLabel( 0, FS_Text(FS_T_PASSWORD), 0, 1 );
	FS_TabSheetAddWidget( tabAccount, wLabel );
	wEdit = FS_CreateEditBox( FS_W_SnsEmlActPassword, rsp.data.eml_account.password, 0, 1, FS_NULL );
	FS_WGT_SET_MARK_CHAR( wEdit );
	FS_WGT_CLR_MULTI_LINE( wEdit );	
	FS_TabSheetAddWidget( tabAccount, wEdit );
	wLabel = FS_CreateCheckBox( FS_W_SnsEmlActBindHome, FS_Text(FS_T_BIND_HOME) );
	FS_WidgetSetCheck( wLabel, rsp.data.eml_account.bind_home );
	FS_TabSheetAddWidget( tabAccount, wLabel );

	if ( rsp.data.eml_account.email ) {
		mWin->private_data = 1;	/* means that we already register a email account */
	}
	FS_SnsFreeResponse( &rsp, FS_SNS_REQ_EML_GET_ACOUNT );

	FS_SnsEmailFormSetTitle( mWin, FS_SNS_REQ_EML_RECV, FS_FALSE );
	FS_WindowSetSoftkey( mWin, 1, FS_Text(FS_T_REFRESH), FS_SnsEmlInboxUpdate_CB );
	FS_WindowSetSoftkey( mWin, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );

	FS_ShowWindow( mWin );
}

static void FS_SnsBind_CB( FS_Window *win )
{
	FS_CHAR url[256] = {0};
	FS_SnsConfig *config = FS_SnsGetConfig( );
	FS_UINT1 lan = FS_GetLanguage( );

	IFS_Snprintf( url, sizeof(url) - 1, 
		"http://apiasiad.avalaa.com/weibonewtest/bind.php?display=wap2.0&username=%s&flag=j2me&lang=%s",
		config->user_name, lan == 0 ? "en_US" : "zh_CN" );
	IFS_BrowserOpenURL( url );
}

static void FS_MainMenu_UI( FS_Window *win )
{
	FS_Window *mwin;
	FS_Widget *wISync, *wBind, *wExt, *wSet;

	mwin = FS_CreateWindow( FS_W_SnsMainMenuFrm, FS_Text(FS_T_ISYNC), FS_NULL );
	
	wISync = FS_CreateButton( 0, FS_Text(FS_T_ISYNC), FS_I_ISYNC_L, 3 );
	wBind = FS_CreateButton( 0, FS_Text(FS_T_BIND), FS_I_BIND_L, 3 );
	FS_WGT_SET_SHARE_HEIGHT( wBind );

	wExt = FS_CreateButton( 0, FS_Text(FS_T_EXTENSION), FS_I_EXT_L, 3 );
	wSet = FS_CreateButton( 0, FS_Text(FS_T_SETTING), FS_I_SET_L, 3 );
	FS_WGT_SET_SHARE_HEIGHT( wSet );

	wISync->private_data = (FS_UINT4)"isync";
	FS_WidgetSetHandler( wISync, FS_SnsEntry_UI );
	FS_WidgetSetHandler( wSet, FS_SnsSetting_UI );
	FS_WidgetSetHandler( wExt, FS_SnsList_UI );
	FS_WidgetSetHandler( wBind, FS_SnsBind_CB );
	FS_WindowAddWidget( mwin, wISync );
	FS_WindowAddWidget( mwin, wBind );
	FS_WindowAddWidget( mwin, wExt );
	FS_WindowAddWidget( mwin, wSet );

	FS_WindowSetSoftkey( mwin, 1, FS_Text(FS_T_FEEDBACK), FS_SnsFeedback_UI );
	FS_WindowSetSoftkey( mwin, 3, FS_Text(FS_T_EXIT), FS_SnsExit_CB );

	FS_ShowWindow( mwin );
}

static FS_BOOL FS_SnsRequestWinProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	
	if( cmd == FS_WM_COMMAND && wparam == FS_EV_NO )
	{
		if ( GFS_SnsApp.timer_id ) {
			IFS_StopTimer( GFS_SnsApp.timer_id );
			GFS_SnsApp.timer_id = 0;
		}
		GFS_SnsApp.cur_req_win = FS_NULL;
		FS_SnsLibCancelRequest( GFS_SnsApp.snslib );
		FS_DestroyWindowByID( FS_W_SnsRssCategoryDetailFrm );
		ret = FS_TRUE;
	}
	return ret;
}

static void FS_SnsLogin_CB( FS_Window *win )
{
	FS_Widget *wgt;
	FS_CHAR *szUser, *szPass;
	FS_SnsConfig *pConfig;
	FS_SnsRequest req = {0};

	wgt = FS_WindowGetWidget( win, FS_W_SnsUserName );
	szUser = wgt->text;
	
	if ( szUser == FS_NULL || szUser[0] == 0 )
	{
		FS_MessageBox(FS_MS_ALERT, FS_Text(FS_T_PLS_INPUT_USER), FS_NULL, FS_TRUE);
		FS_WidgetSetFocus( win, wgt);
		return;
	}

	wgt = FS_WindowGetWidget( win, FS_W_SnsPassword );
	szPass = wgt->text;

	if (szPass == FS_NULL || szPass[0] == 0 )
	{
		FS_MessageBox(FS_MS_ALERT, FS_Text(FS_T_PLS_INPUT_PWD), FS_NULL, FS_TRUE);
		FS_WidgetSetFocus( win, wgt);
		return;
	}

	pConfig = FS_SnsGetConfig( );
	IFS_Strncpy( pConfig->user_name, szUser, sizeof(pConfig->user_name) - 1 );
	wgt = FS_WindowGetWidget( win, FS_W_SnsSavePwd );
	if ( FS_WGT_GET_CHECK(wgt) )
	{
		IFS_Strncpy( pConfig->password, szPass, sizeof(pConfig->password) - 1 );

		FS_SnsSetConfig( pConfig );
	}

	FS_TrimRight( szUser, -1 );
	FS_TrimRight( szPass, -1 );
	req.req = FS_SNS_REQ_LOGIN;
	req.data.login.user = szUser;
	req.data.login.pass = szPass;
	FS_SnsUIRequestStart( win, &req, FS_FALSE );
}

static FS_BOOL FS_SnsLoginWndProc( FS_Window *win, FS_SINT4 cmd, FS_SINT4 wparam, FS_UINT4 lparam )
{
	FS_BOOL ret = FS_FALSE;
	if( cmd == FS_WM_COMMAND && wparam == FS_EV_ITEM_VALUE_CHANGE )
	{
		FS_Widget *wgt = (FS_Widget *)lparam;
		FS_SnsConfig *pConfig = FS_SnsGetConfig( );
		if( wgt->id == FS_W_SnsSavePwd )
		{
			pConfig->save_passwd = FS_WGT_GET_CHECK( wgt );
			FS_SnsSetConfig( pConfig );
		}
		else if( wgt->id == FS_W_SnsAutoLogin )
		{
			pConfig->auto_login = FS_WGT_GET_CHECK( wgt );
			FS_SnsSetConfig( pConfig );
		}
		ret = FS_TRUE;
	}
	return ret;
}

static void FS_SnsRegister_CB( FS_Window *win )
{
	FS_Widget *wgt;
	FS_CHAR *szUser, *szPass, *szPassCheck, *szEmail;
	FS_SnsRequest req = {0};
	
	wgt = FS_WindowGetWidget( win, FS_W_SnsUserName );
	szUser = wgt->text;
	if ( szUser == FS_NULL || szUser[0] == 0 )
	{
		FS_MessageBox(FS_MS_ALERT, FS_Text(FS_T_PLS_INPUT_USER), FS_NULL, FS_TRUE);
		FS_WidgetSetFocus( win, wgt);
		return;
	}
	
	wgt = FS_WindowGetWidget( win, FS_W_SnsPassword );
	szPass = wgt->text;	
	if (szPass == FS_NULL || szPass[0] == 0 )
	{
		FS_MessageBox(FS_MS_ALERT, FS_Text(FS_T_PLS_INPUT_PWD), FS_NULL, FS_TRUE);
		FS_WidgetSetFocus( win, wgt);
		return;
	}

	wgt = FS_WindowGetWidget( win, FS_W_SnsPasswordCheck );
	szPassCheck = wgt->text;
	if ( szPassCheck == FS_NULL || szPassCheck[0] == 0 )
	{
		FS_MessageBox(FS_MS_ALERT, FS_Text(FS_T_PLS_INPUT_PWD), FS_NULL, FS_TRUE);
		FS_WidgetSetFocus( win, wgt);
		return;
	}

	wgt = FS_WindowGetWidget( win, FS_W_SnsEmail );
	szEmail = wgt->text;
	if ( szEmail == FS_NULL || szEmail[0] == 0 )
	{
		FS_MessageBox(FS_MS_ALERT, FS_Text(FS_T_PLS_INPUT_EMAIL), FS_NULL, FS_TRUE);
		FS_WidgetSetFocus( win, wgt);
		return;
	}
	
	if (IFS_Strcmp(szPass, szPassCheck) != 0)
	{
		FS_MessageBox( FS_MS_ALERT, FS_Text(FS_T_PASSWORD_MISMATCH), FS_NULL, FS_FALSE );
		return;
	}

	FS_TrimRight( szUser, -1 );
	FS_TrimRight( szPass, -1 );
	FS_TrimRight( szEmail, -1 );
	req.req = FS_SNS_REQ_REGISTER;
	req.data.reg.user = szUser;
	req.data.reg.pass = szPass;
	req.data.reg.email = szEmail;
	FS_SnsUIRequestStart( win, &req, FS_FALSE );
}

static void FS_SnsRegister_UI( FS_Window *pwin )
{
	FS_Widget *wUserLabel, *wUserName, *wPwdLabel, *wPasswd, *wPwdLabelAgain, *wPwdAgain, *wEmailLabel, *wEmail;
	FS_Window *win;
	FS_EditParam eParam = { FS_IM_ABC, FS_IM_123 | FS_IM_ABC, FS_URL_LEN };
			
	win = FS_CreateWindow( FS_W_SnsRegisterFrm, FS_Text(FS_T_REGISTER), FS_NULL );
	
	FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_REGISTER), FS_SnsRegister_CB );
	FS_WindowSetSoftkey( win, 3, FS_Text(FS_T_BACK), FS_StandardKey3Handler );
	
	wUserLabel = FS_CreateLabel( 0, FS_Text(FS_T_USER_NAME), 0, 1 );
	wUserName = FS_CreateEditBox( FS_W_SnsUserName, FS_NULL, 0, 1, &eParam );
	FS_WGT_CLR_MULTI_LINE( wUserName );
	wPwdLabel = FS_CreateLabel( 0, FS_Text(FS_T_PASSWORD), 0, 1 );
	wPasswd = FS_CreateEditBox( FS_W_SnsPassword, FS_NULL, 0, 1, &eParam );
	FS_WGT_SET_MARK_CHAR( wPasswd );
	FS_WGT_CLR_MULTI_LINE( wPasswd );
	wPwdLabelAgain = FS_CreateLabel( 0, FS_Text(FS_T_PASSWORD_CHECK), 0, 1 );
	wPwdAgain = FS_CreateEditBox( FS_W_SnsPasswordCheck, FS_NULL, 0, 1, &eParam );
	FS_WGT_SET_MARK_CHAR( wPwdAgain );
	FS_WGT_CLR_MULTI_LINE( wPwdAgain );
	wEmailLabel = FS_CreateLabel( 0, FS_Text(FS_T_EMAIL), 0, 1 );
	wEmail = FS_CreateEditBox( FS_W_SnsEmail, FS_NULL, 0, 1, &eParam );
	FS_WGT_CLR_MULTI_LINE( wEmail );
	
	FS_WGT_SET_FORCE_NO_STATUS_BAR( wUserName );
	FS_WGT_SET_FORCE_NO_STATUS_BAR( wPasswd );
	FS_WGT_SET_FORCE_NO_STATUS_BAR( wPwdAgain );
	FS_WGT_SET_FORCE_NO_STATUS_BAR( wEmail );
	FS_WindowAddWidget( win, wUserLabel );
	FS_WindowAddWidget( win, wUserName );
	FS_WindowAddWidget( win, wPwdLabel );
	FS_WindowAddWidget( win, wPasswd );
	FS_WindowAddWidget( win, wPwdLabelAgain );
	FS_WindowAddWidget( win, wPwdAgain );
	FS_WindowAddWidget( win, wEmailLabel );
	FS_WindowAddWidget( win, wEmail );

	FS_ShowWindow( win );	

	if( pwin->type == FS_WT_MENU || pwin->type == FS_WT_POPUP_MENU )
		FS_DestroyWindow( pwin );
}

static void FS_SnsLoginFrmMenu_UI( FS_Window *win )
{
	FS_Widget *mRegister, *mExit, *mSetting, *mAbout;
	FS_Window *pMenu;

	mRegister = FS_CreateMenuItem( 0,	FS_Text(FS_T_REGISTER) );
	mExit = FS_CreateMenuItem( 0,  FS_Text(FS_T_EXIT) );
	mSetting = FS_CreateMenuItem( 0,  FS_Text(FS_T_SETTING) );
#ifdef FS_SNS_ENABLE_SFOX_COPYRIGHT
	pMenu = FS_CreateMenu( FS_W_SnsLoginFrmMenu, 4 );
#else
	pMenu = FS_CreateMenu( FS_W_SnsLoginFrmMenu, 3 );
#endif


	FS_MenuAddItem( pMenu, mRegister );
	FS_MenuAddItem( pMenu, mSetting );
#ifdef FS_SNS_ENABLE_SFOX_COPYRIGHT
	mAbout = FS_CreateMenuItem( 0,  FS_Text(FS_T_ABOUT) );
	FS_MenuAddItem( pMenu, mAbout );
	FS_WidgetSetHandler( mAbout, FS_ThemeSetting_UI );
#endif
	FS_MenuAddItem( pMenu, mExit );
	FS_WidgetSetHandler( mRegister, FS_SnsRegister_UI );
	FS_WidgetSetHandler( mSetting, FS_SnsSetting_UI );
	FS_WidgetSetHandler( mExit, FS_SnsExit_CB );
	
	FS_MenuSetSoftkey( pMenu );
	
	FS_ShowWindow( pMenu );
}

//-----------------------------------------------------------------------------------
// open the sns main window - data.login win
void FS_SnsMain( void )
{	
	FS_Widget *wUserLabel, *wUserName, *wPwdLabel, *wPasswd, *wSavePwd, *wAutoLogin;
	FS_Window *win;
	FS_EditParam eParam = { FS_IM_ABC, FS_IM_123 | FS_IM_ABC, FS_URL_LEN };
	FS_SnsConfig *pConfig;
	
	// sys init place here
	if ( ! FS_SnsSysInit( ) )
		return;

	if ( FS_ApplicationIsActive( FS_APP_SNS) )
		return;

	FS_ActiveApplication( FS_APP_SNS );
	// end init
	
	pConfig = FS_SnsGetConfig( );
	
	win = FS_CreateWindow( FS_W_SnsLoginFrm, FS_Text(FS_T_ISYNC), FS_SnsLoginWndProc );
	
	FS_WindowSetSoftkey( win, 1, FS_Text(FS_T_MENU), FS_SnsLoginFrmMenu_UI );
	FS_WindowSetSoftkey( win, 3, FS_Text(FS_T_LOGIN), FS_SnsLogin_CB );
	
	wUserLabel = FS_CreateLabel( 0, FS_Text(FS_T_USER_NAME), 0, 1 );
	wUserName = FS_CreateEditBox( FS_W_SnsUserName, pConfig->user_name, 0, 1, &eParam );
	FS_WGT_CLR_MULTI_LINE( wUserName );
	wPwdLabel = FS_CreateLabel( 0, FS_Text(FS_T_PASSWORD), 0, 1 );
	if (pConfig->save_passwd)
		wPasswd = FS_CreateEditBox( FS_W_SnsPassword, pConfig->password, 0, 1, &eParam );
	else
		wPasswd = FS_CreateEditBox( FS_W_SnsPassword, FS_NULL, 0, 1, &eParam );
	FS_WGT_SET_MARK_CHAR( wPasswd );
	FS_WGT_CLR_MULTI_LINE( wPasswd );
	wSavePwd = FS_CreateCheckBox( FS_W_SnsSavePwd, FS_Text(FS_T_SAVE_PWD) );
	wAutoLogin = FS_CreateCheckBox( FS_W_SnsAutoLogin, FS_Text(FS_T_AUTO_LOGIN) );

	FS_WindowAddWidget( win, wUserLabel );
	FS_WindowAddWidget( win, wUserName );
	FS_WindowAddWidget( win, wPwdLabel );
	FS_WindowAddWidget( win, wPasswd );
	FS_WindowAddWidget( win, wSavePwd );
	FS_WindowAddWidget( win, wAutoLogin );
	
	FS_WidgetSetCheck( wSavePwd, pConfig->save_passwd );
	FS_WidgetSetCheck( wAutoLogin, pConfig->auto_login );

	FS_ShowWindow( win );

	// handle auto data.login
	if (pConfig->auto_login)
	{
		if (pConfig->user_name[0] && pConfig->password[0] && pConfig->save_passwd)
			FS_SnsLogin_CB( win );
	}
}

void FS_SnsExit( void )
{
	FS_SnsExit_CB( FS_NULL );
}

#endif //FS_MODULE_SNS
