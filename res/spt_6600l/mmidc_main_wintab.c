/*****************************************************************************
** File Name:      mmidc_main_wintab.c                                       *
** Author:                                                                   *
** Date:           2006-5	                                                 *
** Copyright:      2003 Spreadtrum, Incorporated. All Rights Reserved.       *
** Description:    This file is used to describe dc preview window table     *
*****************************************************************************
**                         Important Edit History                            *
** --------------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                               *
** 05/2006        gang.tong        Create									 *
*****************************************************************************/

/**--------------------------------------------------------------------------*
**                         Include Files                                    *
**--------------------------------------------------------------------------*/
#include "std_header.h"
#ifdef  CAMERA_SUPPORT
#include "mmidisplay_data.h"
#include "window_parse.h"
#include "mmi_common.h"
#include "mmi_subwintab.h"
#include "mmiphone.h"
#include "mmi_default.h"
#include "mmiaudio_ctrl.h" 
#include "mmk_timer.h"
#include "mmi_appmsg.h"
#include "guicommon.h"
#include "guilcd.h"
#include "guiblock.h"
#include "mmidc_camera_id.h"
#include "mmipub.h"
#include "block_mem.h"
#include "mmidc_gui.h"
#include "mmidc_flow.h"
#include "mmidc_camera_text.h"
#include "mmi_common.h"
#include "guilistbox.h"
#include "mmi_module.h"
#include "mmi_filetask.h"
#include "guilabel.h"
#include "guitextbox.h"
#include "mmiudisk_export.h"
#include "mmidc_display.h"
#include "mmidc_option.h"
#include "mmidc.h"
#include "mmisd_export.h"
#include "mmieng_uitestwin.h"
#include "guirichtext.h"
#include "mmidc_save.h"
#include "mmidc_setting.h"
#include "mmieng.h"
#include "guires.h"
#include "guiref_scale.h"

#ifdef QQ_SUPPORT_TENCENT
#include "qq2009.h"
#endif

#if ATECH_SOFT//wuty for Spt00032942
#include "mmipicview_id.h"
#endif

/*---------------------------------------------------------------------------*/
/*                          MACRO DEFINITION                                 */
/*---------------------------------------------------------------------------*/
#define WIN_ID_DEF(win_id, win_id_name)          win_id_name,

const uint8 mmidc_id_name_arr[][MMI_WIN_ID_NAME_MAX_LENGTH] =
{
#include "mmidc_camera_id.def"    
};
#undef WIN_ID_DEF

#define MMIDC_FULL_PATH_LENGTH				256


typedef enum
{
    DC_FOLDER,
    DV_FOLDER_3GP,
    DV_FOLDER_MP4,
    DV_FOLDER_AVI,
    CAMERA_MAX
}MMICAMERA_TYPE_E;

typedef void (*DCKEYFUNCTION)(void);

typedef enum
{
    DC_LEFT_KEY = 0,
    DC_RIGHT_KEY,
    DC_UP_KEY,
    DC_DOWN_KEY,
    DC_LEFT_SOFT_KEY,
    DC_RIGHT_SOFT_KEY,
    DC_OK_KEY,
    DC_KEY_MAX
}DC_KEY_E;

/*---------------------------------------------------------------------------*/
/*                         STATIC DEFINITION                                 */
/*---------------------------------------------------------------------------*/
LOCAL uint8				s_timer_delay = 0; // used for delay shoot
LOCAL uint8				s_3seconds_timer = 0;    //used for close OSD in 3 seconds if user doesn't do any operations
LOCAL uint8				s_timer_count_down = 0;
LOCAL uint8				s_updown_tip_timer = 0;
LOCAL uint8				s_record_timer = 0;
LOCAL uint8				s_text_scroll_timer = 0;
LOCAL uint32			s_hit_timer = 0;
LOCAL uint8				s_switch_osd_key_timer_id = 0;             //key timer
LOCAL DIRECTION_KEY		s_last_key_down = CANCEL_KEY;   
LOCAL uint32			s_record_total_time = 0;
const char		        s_type_name[CAMERA_MAX][12] =
{
    "*.jpg",
    "*.3gp",
    "*.mp4",
    "*.avi"
};
LOCAL uint32		s_udisk_photo_total = 0;
LOCAL uint32		s_udisk_video_total = 0;
LOCAL uint32		s_sd_photo_total = 0;
LOCAL uint32		s_sd_video_total = 0;

LOCAL BOOLEAN       s_is_calcuate_space = FALSE;
LOCAL uint8         s_is_chip_test_timer = 0;
LOCAL BOOLEAN       s_is_chip_test = FALSE;
LOCAL BOOLEAN       s_is_pressed_save = FALSE;

#ifdef ATECH_ENG_SUPPORT
#define ENG_AUTO_TEST_ROUND_NUM  1000

extern BOOLEAN g_is_eng_auto_test;
extern uint8   g_auto_test_time_id;
uint32 g_eng_test_round_time = 0;
#endif

extern PUBLIC void MMIDC_RegSettingNv(void);

/*---------------------------------------------------------------------------*/
/*                          LOCAL FUNCTION DECLARE                           */
/*---------------------------------------------------------------------------*/

/*****************************************************************************/
// 	Description : handle the message of dc window
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL MMI_RESULT_E HandleCameraWinMsg(MMI_WIN_ID_T win_id, MMI_MESSAGE_ID_E	msg_id, DPARAM param);

/*****************************************************************************/
// 	Description : handle photo and video send window msg
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL MMI_RESULT_E HandleSendWinMsg(MMI_WIN_ID_T win_id, MMI_MESSAGE_ID_E msg_id, DPARAM param);

/*****************************************************************************/
// 	Description : handle the help window msg
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL MMI_RESULT_E HandleHelpWinMsg(MMI_WIN_ID_T win_id, MMI_MESSAGE_ID_E msg_id, DPARAM param);

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void KeyFunction(DC_KEY_E key);

/*****************************************************************************/
// 	Description : start hit timer
//	Global resource dependence : none
//  Author: robert.wang
//	Note: camera exit while do nothing in 1 minute 
/*****************************************************************************/
LOCAL void MMIDC_ResetHitTimer(void);

/*****************************************************************************/
// 	Description : close hit timer
//	Global resource dependence : none
//  Author: robert.wang
//	Note:
/*****************************************************************************/
LOCAL void MMIDC_CloseHitTimer(void);

//extern PUBLIC void LCD_SetLCDBufferWidth(uint32 width);
extern PUBLIC void GUI_LCDSetForOSD(BOOLEAN valid);

#ifdef ATECH_SOFT
LOCAL MMI_RESULT_E HandleDvSendWinMsg(MMI_WIN_ID_T win_id, MMI_MESSAGE_ID_E msg_id, DPARAM param);
#endif

/*---------------------------------------------------------------------------*/
/*                          CONSTANT VARIABLES                               */
/*---------------------------------------------------------------------------*/
WINDOW_TABLE(MMIDC_DC_TAB) = 
{
    //CLEAR_LCD,
        //WIN_PRIO( WIN_ONE_LEVEL ),
        WIN_FUNC((uint32)HandleCameraWinMsg),   
        WIN_ID(MMIDC_MAIN_WIN_ID),
        END_WIN
};

WINDOW_TABLE(MMIDC_SEND_TAB) = 
{
    //CLEAR_LCD,
        //WIN_PRIO( WIN_ONE_LEVEL ),
        WIN_FUNC((uint32)HandleSendWinMsg),
        WIN_ID(MMIDC_SEND_WIN_ID),
        WIN_TITLE(TXT_DC_SEND),
        CREATE_MENU_CTRL(0, MMI_CLIENT_RECT_TOP, MMI_MAINSCREEN_RIGHT_MAX_PIXEL, MMI_CLIENT_RECT_BOTTOM, 
        MMIDC_SEND_OPT, MMIDC_SEND_MENU_CTRL_ID),
        WIN_SOFTKEY(STXT_OK, TXT_NULL, STXT_RETURN),
        END_WIN
};
#ifdef ATECH_SOFT
WINDOW_TABLE(MMIDV_SEND_TAB) = 
{
    //CLEAR_LCD,
        //WIN_PRIO( WIN_ONE_LEVEL ),
        WIN_FUNC((uint32)HandleDvSendWinMsg),
        WIN_ID(MMIDV_SEND_WIN_ID),
        WIN_TITLE(TXT_DC_SEND),
        CREATE_MENU_CTRL(0, MMI_CLIENT_RECT_TOP, MMI_MAINSCREEN_RIGHT_MAX_PIXEL, MMI_CLIENT_RECT_BOTTOM, 
        MMIDV_SEND_OPT, MMIDV_SEND_MENU_CTRL_ID),
        WIN_SOFTKEY(STXT_OK, TXT_NULL, STXT_RETURN),
        END_WIN
};
#endif
WINDOW_TABLE(MMIDC_HELP_TAB) = 
{
    //CLEAR_LCD,
        //WIN_PRIO( WIN_ONE_LEVEL ),
        WIN_FUNC((uint32)HandleHelpWinMsg),    
        WIN_ID(MMIDC_HELP_WIN_ID),
        WIN_SOFTKEY(STXT_OK, TXT_NULL, STXT_RETURN),
        WIN_TITLE(TXT_DC_HELP),
	    CREATE_TEXTBOX_CTRL(0, MMI_CLIENT_RECT_TOP, MMI_MAINSCREEN_RIGHT_MAX_PIXEL, MMI_CLIENT_RECT_BOTTOM, 
		    MAINLCD_ID, MMIDC_HELP_TEXT_CTRL_ID),
        END_WIN
};

/*---------------------------------------------------------------------------*/
/*                          FUNCTION DEFINITION                              */
/*---------------------------------------------------------------------------*/
/*****************************************************************************/
//  Description : HandleAlertWin
//  Global resource dependence :
//  Author: ryan.xu
//  Note:
/*****************************************************************************/
LOCAL MMI_RESULT_E HandleAlertWin(MMI_WIN_ID_T          win_id,
                                  MMI_MESSAGE_ID_E      msg_id,
                                  DPARAM                param
                                  )
{
    MMI_RESULT_E result = MMI_RESULT_TRUE;
    
    switch (msg_id)
    {        
    case MSG_APP_CANCEL:
        MMK_CloseWin(win_id);
        break;
        
    default:
        result = MMIPUB_HandleAlertWinMsg(win_id,msg_id,param);
        break;
    }
    
    return (result);
}

/*****************************************************************************/
// 	Description : open alert window
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC void MMIDC_OpenAlertWin(									 
                               MMIPUB_SOFTKEY_STYLE_E  softkey_type,   //softkey type
                               MMI_TEXT_ID_T           text_id,        //text id
                               MMI_IMAGE_ID_T          image_id,       //image ptr
                               uint32                  time_out)       //time out
{
    MMIPUB_OpenAlertWinByTextId(&time_out,text_id,TXT_NULL,image_id,PNULL,PNULL,softkey_type,HandleAlertWin);/*lint !e64*/
}

/*****************************************************************************/
// 	Description : handle the help window msg
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL MMI_RESULT_E HandleHelpWinMsg(MMI_WIN_ID_T win_id, MMI_MESSAGE_ID_E	msg_id, DPARAM param)
{
    MMI_RESULT_E        recode		= MMI_RESULT_TRUE;
    GUI_LCD_DEV_INFO	lcd_dev_info = {0};
    MMI_STRING_T        text_str = {0};
    GUI_POINT_T         dis_point = {0};

    lcd_dev_info.lcd_id = GUI_MAIN_LCD_ID;
    lcd_dev_info.block_id = GUI_BLOCK_MAIN;
    
    SCI_TRACE_LOW("[MMI DC]: HandleHelpWinMsg, msg_id = %x", msg_id);
    switch(msg_id)
    {
    case MSG_OPEN_WINDOW:
        MMK_SetAtvCtrl(win_id, MMIDC_HELP_TEXT_CTRL_ID);
        MMI_GetLabelTextByLang(TXT_DC_HELP_CONTENT, &text_str);
        GUITEXTBOX_SetText(MMIDC_HELP_TEXT_CTRL_ID, &text_str, FALSE);
        GUIWIN_SetSoftkeyTextId(win_id,  (MMI_TEXT_ID_T)TXT_NULL, (MMI_TEXT_ID_T)TXT_NULL, (MMI_TEXT_ID_T)STXT_RETURN, FALSE);
        break;
        
    case MSG_FULL_PAINT:
        dis_point.x = 0;
        dis_point.y = MMI_TITLE_HEIGHT;
        GUIRES_DisplayImg(&dis_point,
            PNULL,
            PNULL,
            win_id,
            IMAGE_COMMON_BG,
            &lcd_dev_info);
        break;
        
    case MSG_APP_CANCEL:
        MMK_CloseWin(win_id);
        break;
        
    default:
        recode = MMI_RESULT_FALSE;
        break;
    }
    
    return recode; 
}

/*****************************************************************************/
// 	Description : set lcd infor for rotate dc
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC void MMIDC_OpenHelpWin(void)
{
    MMK_CreateWin((uint32*)MMIDC_HELP_TAB, (ADD_DATA)PNULL);/*lint !e64*/
}

/*****************************************************************************/
// 	Description : set lcd infor for rotate dc
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC void MMIDC_SetLcdForRotate(BOOLEAN set)
{
    MMIDC_DISPLAY_MODE_E  display_mode = MMIDC_GetDisplayMode();

    SCI_TRACE_LOW("[MMIDC]: MMIDC_SetLcdForRotate set = %d", set);
    
    if(set)
    {
        if(MMIDC_GetScreenMode() == SCREEN_MODE_HORIIZONTAL)
        {
            GUILCD_SetLcdRotMode(GUI_MAIN_LCD_ID, GUI_LCD_ROTATE_90);/*lint !e64*/
            GUIBLOCK_SetNeedRotateOSD(TRUE);
        }
        else
        {
            GUI_LCDSetForOSD(FALSE);
        }
    }
    else
    {
        if(MMIDC_GetScreenMode() == SCREEN_MODE_HORIIZONTAL)
        {
            if (MMIDC_DISPLAY_HOR_HOR == display_mode)
            {
               GUILCD_SetLcdRotMode(GUI_MAIN_LCD_ID, GUI_LCD_ROTATE_90);
            }
            else
            {
                GUILCD_SetLcdRotMode(GUI_MAIN_LCD_ID, GUI_LCD_ROTATE_0);/*lint !e64*/
            }
            
            GUIBLOCK_SetNeedRotateOSD(FALSE);
        }
        else
        {
            if (MMIDC_DISPLAY_HOR_VER == display_mode)
            {
                GUILCD_SetLcdRotMode(GUI_MAIN_LCD_ID, GUI_LCD_ROTATE_0);          
                GUIBLOCK_SetNeedRotateOSD(FALSE);
            }
        }
    }
}

/*****************************************************************************/
// 	Description : handle the message of dc window
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL BOOLEAN HandleOSDKey(DIRECTION_KEY key)
{

    SCI_TRACE_LOW("[MMIDC]: HandleOSDKey key = %d", key);   
    if(MMIDC_GetSettingItemHandle()->SettingItemIsOpen())
    {
        MMIDC_GetSettingItemHandle()->SettingItemHandleKeyDown(key);
        MMIDC_GetSettingItemHandle()->SettingItemDisplay();
        return TRUE;
    }
    else if(MMIDC_GetIconHandle()->IconIsOpen())
    {
        if(MMIDC_HandleAdjustKey(key))
        {
            return TRUE;
        }
        else if(MMIDC_GetIconHandle()->IconHandleKeyDown(key))
        {
            MMIDC_GetIconHandle()->IconDisplay();
            return TRUE;
        }
        else if(MMIDC_GetMenuHandle()->MenuIsOpen())
        {
            if(MMIDC_GetMenuHandle()->MenuHandleKeyDown(key))
            {
                MMIDC_GetMenuHandle()->MenuDisplay();
                return TRUE;
            }
        }
    }
    else if(MMIDC_GetMenuHandle()->MenuIsOpen())
    {
        if(MMIDC_GetMenuHandle()->MenuHandleKeyDown(key))
        {
            MMIDC_GetMenuHandle()->MenuDisplay();
            return TRUE;
        }
    }
    return FALSE;
}

/*****************************************************************************/
// 	Description : start delay shoot timer
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void StartShootDelayTimer(uint32 time)
{
    if(0 == s_timer_delay)
    {
        s_timer_delay = MMK_CreateTimer(time, TRUE);			
    }
    if(0 == s_timer_count_down)
    {
        s_timer_count_down = MMIDC_GetSelfShootDelaySeconds();
    }
}

/*****************************************************************************/
// 	Description : close delay shoot timer
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void CloseShootDelayTimer(void)
{
    if(0 != s_timer_delay)
    {
        MMK_StopTimer(s_timer_delay);
        s_timer_delay = 0;
    }
}

/*****************************************************************************/
// 	Description : start osd menu display timer
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC void MMIDC_StartTipDisplayTimer(void)
{
    if(0 == s_3seconds_timer)
    {
        s_3seconds_timer = MMK_CreateTimer(MMI_3SECONDS, TRUE);			
    }
    else
    {
        MMK_StopTimer(s_3seconds_timer);
        s_3seconds_timer = MMK_CreateTimer(MMI_3SECONDS, TRUE);			
    }
}

/*****************************************************************************/
// 	Description : close osd menu display timer
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void CloseTipDisplayTimer(void)
{
    if(0 != s_3seconds_timer)
    {
        MMK_StopTimer(s_3seconds_timer);
        s_3seconds_timer = 0;
    }
}

/*****************************************************************************/
// 	Description : close up down tip timer
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void CloseUpDownTipTimer(void)
{
    if(0 != s_updown_tip_timer)
    {
        MMK_StopTimer(s_updown_tip_timer);
        s_updown_tip_timer = 0;
    }
}

/*****************************************************************************/
// 	Description : start record timer
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void StartRecordTimer(void)
{
    if(0 == s_record_timer)
    {
        s_record_timer = MMK_CreateTimer(MMI_1SECONDS, TRUE);
    }
    else
    {
        MMK_StopTimer(s_record_timer);
        s_record_timer = MMK_CreateTimer(MMI_1SECONDS, TRUE);
    }
}

/*****************************************************************************/
// 	Description : close record timer
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void CloseRecordTimer(void)
{
    if(0 != s_record_timer)
    {
        MMK_StopTimer(s_record_timer);
        s_record_timer = 0;
    }
}

/*****************************************************************************/
//  Description : start switch osd key timer
//  Global resource dependence : 
//  Author: ryan.xu
//  Note:
/*****************************************************************************/
LOCAL void StartSwitchOSDMenuItemsKeyTimer(DIRECTION_KEY key)
{
    if (0 == s_switch_osd_key_timer_id)
    {
        s_switch_osd_key_timer_id = MMK_CreateTimer(500, FALSE);
        
    }
    else
    {
        SCI_TRACE_LOW("StartSwitchOSDMenuItemsKeyTimer: the key timer has started!");
    }
    s_last_key_down = key;
}

/*****************************************************************************/
//  Description : stop switch osd key timer
//  Global resource dependence : 
//  Author: ryan.xu
//  Note:
/*****************************************************************************/
LOCAL void StopSwitchOSDMenuItemsKeyTimer(void)
{
    if (0 < s_switch_osd_key_timer_id)
    {
        MMK_StopTimer(s_switch_osd_key_timer_id);
        s_switch_osd_key_timer_id = 0;
    }
    else
    {
        SCI_TRACE_LOW("StopSwitchOSDMenuItemsKeyTimer: the key timer has stop!");
    }
}

/*****************************************************************************/
//  Description : start text scroll timer
//  Global resource dependence : 
//  Author: ryan.xu
//  Note:
/*****************************************************************************/
PUBLIC void MMIDC_StartTextScrollTimer(void)
{
    if (0 == s_text_scroll_timer)
    {
        s_text_scroll_timer = MMK_CreateTimer(500, FALSE);
    }
    else
    {
        SCI_TRACE_LOW("MMIDC_StartTextScrollTimer: the key timer has started!");
    }
}

/*****************************************************************************/
//  Description : stop text scroll timer
//  Global resource dependence : 
//  Author: ryan.xu
//  Note:
/*****************************************************************************/
PUBLIC void MMIDC_StopTextScrollTimer(void)
{
    if (0 < s_text_scroll_timer)
    {
        MMK_StopTimer(s_text_scroll_timer);
        s_text_scroll_timer = 0;
    }
    else
    {
        SCI_TRACE_LOW("MMIDC_StopTextScrollTimer: the key timer has stop!");
    }
}

/*****************************************************************************/
// 	Description : open preview window
//	Global resource dependence : none
//  Author: 
//	Note:
/*****************************************************************************/
PUBLIC BOOLEAN MMIAPIDC_OpenPhotoWin(void)
{	
    if (MMIAPIUDISK_UdiskIsRun()) //U盘使用中
    {
        MMIPUB_OpenAlertWinByTextId(PNULL,TXT_COMMON_UDISK_USING,TXT_NULL,IMAGE_PUBWIN_WARNING,PNULL,PNULL,MMIPUB_SOFTKEY_ONE,PNULL);
        return FALSE;
    }
    else if (MMIAPIENG_GetIQModeStatus())
    {
        MMIPUB_OpenAlertWinByTextId(PNULL,TXT_IQ_DATA_PROCESSING,TXT_NULL,IMAGE_PUBWIN_WARNING,PNULL,PNULL,MMIPUB_SOFTKEY_ONE,PNULL);
        return FALSE;
    }
    if(s_is_calcuate_space)
    {
        MMIDC_OpenAlertWin(MMIPUB_SOFTKEY_ONE, TXT_ERROR, IMAGE_PUBWIN_SUCCESS, MMI_3SECONDS);
        return FALSE;
    }

    //destroy task,free buffer,idle enter wap,need free jpg decode buffer
    GUIANIM_DestroyDecTask();
    
    MMIDC_NewSaveData();
    MMIDC_AllocSettingMemory();
    MMIDC_InitOSDMenuIcon();

    MMIDC_SetCameraMode(CAMERA_MODE_DC);	
    MMIDC_Setting_InitDefaultValue();
    MMI_Enable3DMMI(FALSE);
    return MMK_CreateWin((uint32*)MMIDC_DC_TAB, (ADD_DATA)PNULL);/*lint !e64*/
}

/*****************************************************************************/
// 	Description : open preview window
//	Global resource dependence : none
//  Author: 
//	Note:
/*****************************************************************************/
PUBLIC BOOLEAN MMIAPIDC_OpenForChipTest(void)
{
    MMIAPIDC_OpenPhotoWin();
    MMIDC_SetAutoSave(AUTO_SAVE_ON);
    s_is_chip_test = TRUE;	
    return TRUE;
}

/*****************************************************************************/
// 	Description : open preview window
//	Global resource dependence : none
//  Author: 
//	Note:
/*****************************************************************************/
PUBLIC BOOLEAN MMIAPIDC_OpenVideoWin(void)
{	
    if (MMIAPIUDISK_UdiskIsRun()) //U盘使用中
    {
        MMIPUB_OpenAlertWinByTextId(PNULL,TXT_COMMON_UDISK_USING,TXT_NULL,IMAGE_PUBWIN_WARNING,PNULL,PNULL,MMIPUB_SOFTKEY_ONE,PNULL);
        return FALSE;
    }
    else if (MMIAPIENG_GetIQModeStatus())
    {
        MMIPUB_OpenAlertWinByTextId(PNULL,TXT_IQ_DATA_PROCESSING,TXT_NULL,IMAGE_PUBWIN_WARNING,PNULL,PNULL,MMIPUB_SOFTKEY_ONE,PNULL);
        return FALSE;
    }
    if(s_is_calcuate_space)
    {
        MMIDC_OpenAlertWin(MMIPUB_SOFTKEY_ONE, TXT_ERROR, IMAGE_PUBWIN_SUCCESS, MMI_3SECONDS);
        return FALSE;
    }
    MMIDC_NewSaveData();
    MMIDC_AllocSettingMemory();
    MMIDC_InitOSDMenuIcon();

    MMIDC_SetCameraMode(CAMERA_MODE_DV);
    MMIDC_Setting_InitDefaultValue();
    MMI_Enable3DMMI(FALSE);
    return MMK_CreateWin((uint32*)MMIDC_DC_TAB, (ADD_DATA)PNULL);/*lint !e64*/
}

/*****************************************************************************/
// 	Description : record video
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void Record(void)
{
    MMIDC_RemoveAllOSD();
    if(MMIDC_FlowFunction(DC_FORWARD))
    {
        s_record_total_time = 0;
        StartRecordTimer();
        MMIDC_ClearOSDBlock();
       // MMIDC_CreateDigitalBuffer();  //removed, @robert.wang, 09-9-2, cr150390
        MMIDC_DisplayVideoRecordTip(DV_RECORD_MODE);
        MMIDC_DisplaySoftKey(DV_RECORD_MODE);
    }
}

/*****************************************************************************/
// 	Description : capture photo
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void Capture(void)
{
    MMIDC_RemoveAllOSD();
    MMIDC_FlowFunction(DC_FORWARD);
    s_is_pressed_save = FALSE;
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void LeftDCPreview(void)
{
    if(!HandleOSDKey(LEFT_KEY))
    { 
        MMIDC_OpenOSDIcons();      
    }
    MMIDC_DisplaySettingTip();
    MMIDC_DisplaySoftKey(DC_PREVIEW_MODE);
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void LeftSave(void)
{
    if(MMIDC_GetMultiShootEnum() != MULTI_SHOOT_DISABLE)
    {
        MMIDC_MoveCurReviewPhotoID(MOVE_LEFT);
        MMIDC_RemoveAllOSD();
        MMIDC_DisplayCurrentFileName();
        MMIDC_DisplaySoftKey(MMIDC_GetCurrentMode());
        SCI_Sleep(200);
        MMIDC_ReviewPhotos();
    }
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void LeftDVPreview(void)
{
    if(!HandleOSDKey(LEFT_KEY))
    {
        MMIDC_OpenOSDIcons();
        MMIDC_DisplaySoftKey(DV_PREVIEW_MODE);
    }   
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void RightDCPreview(void)
{
    if(!HandleOSDKey(RIGHT_KEY))
    {   
        MMIDC_OpenOSDIcons();       
    }
    
    MMIDC_DisplaySettingTip();
    MMIDC_DisplaySoftKey(DC_PREVIEW_MODE);
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void RightSave(void)
{
    if(MMIDC_GetMultiShootEnum() != MULTI_SHOOT_DISABLE) 
    {
        MMIDC_MoveCurReviewPhotoID(MOVE_RIGHT);
        MMIDC_RemoveAllOSD();
        MMIDC_DisplayCurrentFileName();
        MMIDC_DisplaySoftKey(MMIDC_GetCurrentMode());
        SCI_Sleep(200);
        MMIDC_ReviewPhotos();
    }
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void RightDVPreview(void)
{
    if(!HandleOSDKey(RIGHT_KEY))
    {
        MMIDC_OpenOSDIcons();
        MMIDC_DisplaySoftKey(DV_PREVIEW_MODE);
    }   
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void UpDCPreview(void)
{
    if(!HandleOSDKey(UP_KEY))
    {  
        if(MMIDC_IncreaseZoomValue())
        {
            MMIDC_DisplayZoomTip(MMIDC_GetPhotoZoomValue() + 1);
        }       
    }
    MMIDC_DisplaySettingTip();
    MMIDC_DisplaySoftKey(DC_PREVIEW_MODE);
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void UpSave(void)
{
   return ;
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void UpDVPreivew(void)
{
    HandleOSDKey(UP_KEY);
    MMIDC_DisplaySoftKey(DV_PREVIEW_MODE);
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void UpDVReview(void)
{
    HandleOSDKey(UP_KEY);
    MMIDC_DisplaySoftKey(MMIDC_GetCurrentMode());
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void DownDCPreview(void)
{
    if(!HandleOSDKey(DOWN_KEY))
    {
        if(MMIDC_DecreaseZoomValue())
        {
            MMIDC_DisplayZoomTip(MMIDC_GetPhotoZoomValue() + 1);
        }      
    }
    MMIDC_DisplaySettingTip();
    MMIDC_DisplaySoftKey(DC_PREVIEW_MODE);
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void DownSave(void)
{
    return ;
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void DownDVPreview(void)
{
    HandleOSDKey(DOWN_KEY);
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void DownDVReview(void)
{
    HandleOSDKey(DOWN_KEY);
    MMIDC_DisplaySoftKey(MMIDC_GetCurrentMode());
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void OKDCPreview(void)
{
    if(!HandleOSDKey(OK_KEY))
    {
        if(MMIDC_GetSelfShootDelayTime() == SELF_SHOOT_DISABLE)
        {
            Capture();
        }
        else
        {
            MMIDC_PlayCountVoice(1);
            StartShootDelayTimer(MMI_1SECONDS);
            MMIDC_RemoveAllOSD();
            MMIDC_SetCurrentMode(DC_CAPTURE_CONT_DOWN_MODE);
            MMIDC_DisplayDelaySecondsTip(s_timer_count_down);
        }
    }
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void LSDCPreview(void)
{
    if(!MMIDC_GetIconHandle()->IconIsOpen() && !MMIDC_GetSettingItemHandle()->SettingItemIsOpen()
        && !MMIDC_GetMenuHandle()->MenuIsOpen())
    {
        MMIDC_OpenPhotoOption();
        MMIDC_DisplaySoftKey(DC_PREVIEW_MODE);
    }
    else
    {
        OKDCPreview();
    }
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void LSSave(void)
{
    if(!HandleOSDKey(OK_KEY))
    {
        MMIDC_OpenPhotoReviewOption();
        MMIDC_DisplaySoftKey(DC_SAVE_MODE);
    }
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void LSDVPreview(void)
{
    if(!MMIDC_GetIconHandle()->IconIsOpen() && !MMIDC_GetSettingItemHandle()->SettingItemIsOpen()
        && !MMIDC_GetMenuHandle()->MenuIsOpen())
    {
        MMIDC_OpenVideoOption();
        MMIDC_DisplaySoftKey(DV_PREVIEW_MODE);
    }
    else if(!HandleOSDKey(OK_KEY))
    {
        Record();
    }
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void StopRecord(void)
{
    MMIDC_RemoveAllOSD();
    CloseRecordTimer();
    MMIDC_FlowFunction(DC_FORWARD);
    MMIDC_SetTransparentColor(MMI_BLACK_COLOR);

    if((GUI_LCD_ROTATE_0 == MMIAPICOM_GetCommonRotateMode())
      && (MMIDC_GetDefaultScreenMode() == SCREEN_MODE_HORIIZONTAL))
    {
        MMIDC_SetLcdForRotate(FALSE);
        MMIDC_SetScreenMode(SCREEN_MODE_VERTICAL);
    }
    MMIDC_DisplayVideoReview();
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void LSDVReview(void)
{
    if(!HandleOSDKey(OK_KEY))
    {
        MMIDC_OpenVideoReviewOption();
        MMIDC_DisplaySoftKey(DV_REVIEW_MODE);
    }
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void RSDCPreview(void)
{
    BOOLEAN res = FALSE;
    
    res = MMIDC_GetMenuHandle()->MenuHandleKeyDown(CANCEL_KEY);
    res = MMIDC_GetIconHandle()->IconHandleKeyDown(CANCEL_KEY) || res;
    res = MMIDC_GetSettingItemHandle()->SettingItemHandleKeyDown(CANCEL_KEY) || res;
    res = MMIDC_HandleAdjustKey(CANCEL_KEY) || res;
    MMIDC_DisplaySettingTip();
    MMIDC_DisplaySoftKey(DC_PREVIEW_MODE);
    if(!res)
    {
        MMIDC_PostStateMsg();
    }
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void RSCountDown(void)
{
    MMIAPISET_StopAllRing(FALSE);
    CloseShootDelayTimer();
    s_timer_count_down = 0;
    MMIDC_SetCurrentMode(DC_PREVIEW_MODE);
    MMIDC_ClearOSDBlock();
    MMIDC_RemoveAllOSD();
    if(MMIDC_GetFrameIndex() != FRAME_DISABLE)
    {
        MMIDC_DisplayFrame();
    }
    MMIDC_DisplaySettingTip();
    MMIDC_DisplaySoftKey(MMIDC_GetCurrentMode());
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void RSDCReview(void)
{
    MMIDC_RemoveAllOSD();
    MMIDC_ClearOSDBlock();
    MMIDC_FlowFunction(DC_BACKWARD);
    MMIDC_RemoveAllOSD();
    if(MMIDC_GetFrameIndex() != FRAME_DISABLE)
    {
        MMIDC_DisplayFrame();
    }
    MMIDC_DisplaySettingTip();
    MMIDC_DisplaySoftKey(MMIDC_GetCurrentMode());
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void RSSave(void)
{
    if(HandleOSDKey(CANCEL_KEY))
    {
        if(MMIDC_GetMultiShootEnum() != MULTI_SHOOT_DISABLE && !MMIDC_IsReviewFullScreen())
        {
            MMIDC_DisplayCurrentFileName();
        }
        MMIDC_DisplaySoftKey(DC_SAVE_MODE);
    }
    else
    {
        if(MMIDC_IsReviewFullScreen())
        {
            MMIDC_FlowFunction(DC_BACKWARD);
            MMIDC_DisplaySoftKey(DC_SAVE_MODE);
        }
        else
        {
            MMIDC_RemoveAllOSD();
            MMIDC_ClearOSDBlock();
            MMIDC_FlowFunction(DC_BACKWARD);
            if(MMIDC_GetFrameIndex() != FRAME_DISABLE)
            {
                MMIDC_DisplayFrame();
            }
            MMIDC_DisplaySettingTip();
            MMIDC_DisplaySoftKey(MMIDC_GetCurrentMode());
        }
    }
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void RSDVPreview(void)
{
    BOOLEAN res = FALSE;
    
    res = MMIDC_GetMenuHandle()->MenuHandleKeyDown(CANCEL_KEY);
    res = MMIDC_GetIconHandle()->IconHandleKeyDown(CANCEL_KEY) || res;
    res = MMIDC_GetSettingItemHandle()->SettingItemHandleKeyDown(CANCEL_KEY) || res;
    res = MMIDC_HandleAdjustKey(CANCEL_KEY) || res;
    MMIDC_DisplaySoftKey(DV_PREVIEW_MODE);
    if(!res)
    {
        MMIDC_PostStateMsg();
    }
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void CancelRecord(void)
{
    CloseRecordTimer();
    MMIDC_FlowFunction(DC_BACKWARD);
    MMIDC_RemoveAllOSD();
    MMIDC_DisplaySettingTip();
    MMIDC_DisplaySoftKey(MMIDC_GetCurrentMode());
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void RSDVReview(void)
{
    if(HandleOSDKey(CANCEL_KEY))
    {
        //MMIDC_DisplaySoftKey(DV_REVIEW_MODE);
        MMIDC_DisplayVideoReview();
    }
    else
    {
        MMIDC_SetTransparentColor(MMIDC_TRANSPARENT_COLOR);
        MMIDC_ClearMainBlock(MMI_WINDOW_BACKGROUND_COLOR);
        if(MMIDC_GetDefaultScreenMode() == SCREEN_MODE_HORIIZONTAL)
        {
            MMIDC_SetScreenMode(SCREEN_MODE_HORIIZONTAL);
            MMIDC_SetLcdForRotate(TRUE);
        }
        MMIDC_FlowFunction(DC_BACKWARD);
        MMIDC_RemoveAllOSD();
        MMIDC_DisplaySettingTip();
        MMIDC_DisplaySoftKey(MMIDC_GetCurrentMode());
    }
    
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void OKDVPreview(void)
{
    if(!HandleOSDKey(OK_KEY))
    {
        Record();
    }
}

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void OKDVReview(void)
{
    if(!HandleOSDKey(OK_KEY))
    {
        MMIDC_ReviewVideo();
    }
}

/*****************************************************************************/
// 	Description : save the captured photo
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void Save(void)
{
    GUI_LCD_DEV_INFO dev_info_ptr= {0};
    GUI_POINT_T         dis_point = {0};
    MMIDC_DISPLAY_MODE_E  display_mode = MMIDC_GetDisplayMode();

    
    SCI_TRACE_LOW("[MMIDC]: Save photo");
    
#ifdef PLATFORM_SC6600L
    if(MMIDC_CanPressSave())
    {
        SCI_Sleep(200);
        
        dev_info_ptr.lcd_id = MMIDC_GetLcdID();
        if(MMIDC_GetScreenMode() ==SCREEN_MODE_HORIIZONTAL)
        {
            dis_point.x = (MMIDC_GetPreviewHeight() - 28) / 2;
            dis_point.y = (MMIDC_GetPreviewWidth() - 28) / 2;
        }
        else
        {
            dis_point.x = (MMIDC_GetPreviewWidth() - 28) / 2;
            dis_point.y = (MMIDC_GetPreviewHeight() - 28) / 2;
        }
        
        GUIRES_DisplayImg(&dis_point, PNULL, PNULL, MMIDC_MAIN_WIN_ID, IMG_DC_WAIT, &dev_info_ptr);
        
		//jesse yu add for sub-lcd start
        GUILCD_InvalidateLCDRect((GUI_LCD_ID_E)MMIDC_GetLcdID(), dis_point.x, dis_point.y, dis_point.x + 27, dis_point.y + 27, GUIREF_GetUpdateBlockSet(dev_info_ptr.block_id));/*lint !e64*/
		//jesse yu add for sub-lcd end
		
        MMITHEME_InitMainLcdStortUpdateRect();
        s_is_pressed_save = TRUE;
    }
#endif
    if(!MMIDC_IsCapturing())
    {
        MMIDC_FlowFunction(DC_FORWARD);        
    }
}

const DCKEYFUNCTION s_key_function[DC_KEY_MAX][DC_FLOW_MODE_MAX] = 
{
    /*DC_PREVIEW     CONT_DOWN     DC_CAPTURE  DC_REVIEW    DC_SAVE                 DV_PREVIEW          DV_RECORD        DV_PAUSE         DV_REVIEW */
    {LeftDCPreview,  PNULL,        PNULL,      PNULL,       LeftSave,               LeftDVPreview,      PNULL,           PNULL,           PNULL},
    {RightDCPreview, PNULL,        PNULL,      PNULL,       RightSave,              RightDVPreview,     PNULL,           PNULL,           PNULL},
    {UpDCPreview,    PNULL,        PNULL,      PNULL,       UpSave,                 UpDVPreivew,        PNULL,           PNULL,           UpDVReview},
    {DownDCPreview,  PNULL,        PNULL,      PNULL,       DownSave,               DownDVPreview,      PNULL,           PNULL,           DownDVReview},
    {LSDCPreview,    PNULL,        PNULL,      Save,        LSSave,                 LSDVPreview,        StopRecord,      StopRecord,      LSDVReview},
    {RSDCPreview,    RSCountDown,  PNULL,      RSDCReview,  RSSave,                 RSDVPreview,        CancelRecord,    CancelRecord,    RSDVReview},
    {OKDCPreview,    PNULL,        PNULL,      Save,        MMIDC_OpenSendPhotoWin, OKDVPreview,        StopRecord,           PNULL,           OKDVReview},
};

/*****************************************************************************/
// 	Description : handle key function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void KeyFunction(DC_KEY_E key)
{    
    SCI_ASSERT(key < DC_KEY_MAX);/*lint !e718*/
    if(PNULL != s_key_function[key][MMIDC_GetCurrentMode()])
    {
        if(key <= DC_DOWN_KEY)
        {
            SCI_Sleep(200);
        }
        s_key_function[key][MMIDC_GetCurrentMode()]();
        MMIDC_CompleteDisplayOSD();
    }
}

/*****************************************************************************/
// 	Description : delete window handle function
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC MMI_RESULT_E MMIDC_HandleDeletePubWinBG(MMI_WIN_ID_T win_id, MMI_MESSAGE_ID_E msg_id, DPARAM param)
{
    GUI_POINT_T         dis_point = {0};

    SCI_TRACE_LOW("[MMIDC]: MMIDC_HandleDeletePubWinBG");
    switch (msg_id)
    {
    case MSG_FULL_PAINT:
        GUIRES_DisplayImg(&dis_point,
            PNULL,
            PNULL,
            win_id,
            IMAGE_COMMON_BG,
            MMITHEME_GetDefaultLcdDev());
        break;
    }
    return MMIPUB_HandleQueryWinMsg(win_id, msg_id, param);
}

/*****************************************************************************/
// 	Description : handle the message of dc window
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void HandleDCWinTpPressDown(GUI_POINT_T	point)
{
    BOOLEAN res = FALSE;
    GUI_RECT_T box_right = {0};	
    GUI_RECT_T box_left = {0};
    GUI_RECT_T box_middle = {0};
    MMIDC_DISPLAY_MODE_E  display_mode = MMIDC_GetDisplayMode();
    
    //rocky 3201 ,it is limited.
#ifdef PLATFORM_SC6600R    
    GUI_RECT_T file_name_left_arrow = {SWITCH_PHOTO_LEFT_ARROW_LEFT, SWITCH_PHOTO_LEFT_ARROW_TOP, 
        SWITCH_PHOTO_LEFT_ARROW_RIGHT, SWITCH_PHOTO_LEFT_ARROW_BOTTOM};
    GUI_RECT_T file_name_right_arrow = {SWITCH_PHOTO_RIGHT_ARROW_LEFT, SWITCH_PHOTO_RIGHT_ARROW_TOP, 
        SWITCH_PHOTO_RIGHT_ARROW_RIGHT, SWITCH_PHOTO_RIGHT_ARROW_BOTTOM};
#else
    GUI_RECT_T file_name_left_arrow = {0};
    GUI_RECT_T file_name_right_arrow ={0};
    GUI_RECT_T multi_photo_rect ={0};
#endif
    
    SCI_TRACE_LOW("[MMIDC]: HandleDCWinTpPressDown %d, %d", point.x, point.y);
    
    //except rocky 3201 ,it is limited.
#ifndef PLATFORM_SC6600R
    multi_photo_rect = MMIDC_GetMultiPhotoRect();
    file_name_left_arrow.left = multi_photo_rect.left;
    file_name_left_arrow.right = file_name_left_arrow.left + MMIDC_ITEM_TEXT_HEIGHT - 1;
    file_name_left_arrow.top = multi_photo_rect.top;
    file_name_left_arrow.bottom = file_name_left_arrow.top + MMIDC_ITEM_TEXT_HEIGHT - 1;
    
    file_name_right_arrow.right= multi_photo_rect.right;
    file_name_right_arrow.left = file_name_right_arrow.right - MMIDC_ITEM_TEXT_HEIGHT + 1;
    file_name_right_arrow.top = multi_photo_rect.top;
    file_name_right_arrow.bottom = file_name_right_arrow.top + MMIDC_ITEM_TEXT_HEIGHT - 1;
#endif
    
    box_left = MMMIDC_GetLeftSoftkey();
    box_middle = MMMIDC_GetMiddleSoftkey();
    box_right = MMMIDC_GetRightSoftkey();
    SCI_TRACE_LOW("[MMIDC]: HandleDCWinTpPressDown box_left left= %d, right=%d, top=%d, bottom=%d", box_left.left,box_left.right,box_left.top,box_left.bottom);
    
    if(GUI_PointIsInRect(point, box_right))
    {
        SCI_TRACE_LOW("[MMIDC]: GUI_PointIsInRect rs_rect");
        KeyFunction(DC_RIGHT_SOFT_KEY);
    }
    else if(GUI_PointIsInRect(point, box_middle) && !MMIDC_IsMiddleSoftKeyNULL())
    {
        SCI_TRACE_LOW("[MMIDC]: GUI_PointIsInRect mid_rect");
        KeyFunction(DC_OK_KEY);
    }
    else if(GUI_PointIsInRect(point, box_left))
    {
        SCI_TRACE_LOW("[MMIDC]: GUI_PointIsInRect ls_rect");
        KeyFunction(DC_LEFT_SOFT_KEY);
    }
    else if(GUI_PointIsInRect(point, file_name_left_arrow))
    {
        if(MMIDC_GetCapturedPhotosNumber() > 1 && MMIDC_GetCurrentMode() == DC_SAVE_MODE)
        {
            KeyFunction(DC_LEFT_KEY);
        }
    }
    else if(GUI_PointIsInRect(point, file_name_right_arrow))
    {
        if(MMIDC_GetCapturedPhotosNumber() > 1 && MMIDC_GetCurrentMode() == DC_SAVE_MODE)
        {
            KeyFunction(DC_RIGHT_KEY);
        }
    }
    else
    {
        res = MMIDC_GetIconHandle()->IconHandleTpDown(point.x, point.y);
        res = MMIDC_GetMenuHandle()->MenuHandleTpDown(point.x, point.y) || res;
        res = MMIDC_GetSettingItemHandle()->SettingItemHandleTpDown(point.x, point.y) || res;
        res = MMIDC_HandleAdjustTP(point.x, point.y) || res;
        MMIDC_GetIconHandle()->IconDisplay();
        MMIDC_GetMenuHandle()->MenuDisplay();
        MMIDC_GetSettingItemHandle()->SettingItemDisplay();
        MMIDC_DisplaySoftKey(MMIDC_GetCurrentMode());
        MMIDC_CompleteDisplayOSD();
        if(!res)
        {
            if(MMIDC_GetCurrentMode() == DC_PREVIEW_MODE || MMIDC_GetCurrentMode() == DV_PREVIEW_MODE)
            {
                if(!MMIDC_GetIconHandle()->IconIsOpen() &&
                    !MMIDC_GetSettingItemHandle()->SettingItemIsOpen() && 
                    !MMIDC_GetMenuHandle()->MenuIsOpen())
                {
                    KeyFunction(DC_LEFT_KEY);
                }
                else if(MMIDC_GetIconHandle()->IconIsOpen())
                {
                    KeyFunction(DC_RIGHT_SOFT_KEY);
                }
            }
        }
    }
}
/*****************************************************************************/
// 	Description : handle the message of dc window
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void CloseDCAllTimers(void)
{
    s_timer_count_down = 0;

    CloseShootDelayTimer();
    CloseTipDisplayTimer();
    CloseUpDownTipTimer();
    StopSwitchOSDMenuItemsKeyTimer();
    CloseRecordTimer();
    MMIDC_StopTextScrollTimer();
    MMIDC_CloseHitTimer();
}
/*****************************************************************************/
// 	Description : handle the message of dc window
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC void MMIDC_HandleLoseFocus(void)
{
    MMIDC_RemoveAllOSD();
    MMIDC_GetMenuHandle()->MenuClose();
    MMIDC_GetIconHandle()->IconClose();
    MMIDC_GetSettingItemHandle()->SettingItemClose(TRUE);
    CleanAdjustRect();
    
    MMIDC_ResetScrollNumber();

    CloseDCAllTimers();
    
    MMIDC_FlowFunction(DC_INTERRUPT);
    if((GUI_LCD_ROTATE_0 == MMIAPICOM_GetCommonRotateMode())
      && (MMIDC_GetDefaultScreenMode() == SCREEN_MODE_HORIIZONTAL))
    {
        MMIDC_SetLcdForRotate(FALSE);
        MMIDC_SetScreenMode(SCREEN_MODE_VERTICAL);
    }
    
    MMIDC_ClearMainBlock(MMI_WINDOW_BACKGROUND_COLOR);
    MMITHEME_InitMainLcdStortUpdateRect();
    if(MMIDC_GetCameraMode() == CAMERA_MODE_DC)
    {
        MMIDC_SetCurrentMode(DC_PREVIEW_MODE);
    }
    else if(MMIDC_GetCameraMode() == CAMERA_MODE_DV && MMIDC_GetCurrentMode() != DV_REVIEW_MODE)
    {
        MMIDC_SetCurrentMode(DV_PREVIEW_MODE);
    }
}

/*****************************************************************************/
// 	Description : handle the message of dc window
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC void MMIDC_HandleGetFocus(void)
{
    MMIDC_SetTransparentColor(MMIDC_TRANSPARENT_COLOR);
    MMIDC_ClearMainBlock(MMI_WINDOW_BACKGROUND_COLOR);
    
    MMIDC_ClearOSDBlock();
    
    SCI_TRACE_LOW("[MMIDC]: HandleCameraWinMsg MMIDC_FlowFunction DC_CONTINUE");
    MMIDC_RemoveAllOSD();
    if(MMIDC_GetCurrentMode() == DC_PREVIEW_MODE)
    {
        if(MMIDC_GetDefaultScreenMode() == SCREEN_MODE_HORIIZONTAL &&
            MMIDC_GetScreenMode() == SCREEN_MODE_VERTICAL && 
            MMIDC_GetPhotoSize() != MMIDC_GePhotoSizetVerLcd())
        {
            MMIDC_SetScreenMode(SCREEN_MODE_HORIIZONTAL);
            MMIDC_SetLcdForRotate(TRUE);
        }
        if(! MMIDC_FlowFunction(DC_CONTINUE))
        {
            SCI_TRACE_LOW("[MMIDC]: MMIDC_HandleGetFocus MMIDC_FlowFunction fail!");
            return;
        }
        if(MMIDC_GetFrameIndex() != FRAME_DISABLE)
        {
            MMIDC_DisplayFrame();
        }
        MMIDC_DisplaySettingTip();
        MMIDC_DisplaySoftKey(DC_PREVIEW_MODE);
    }
    else if(MMIDC_GetCurrentMode() == DV_PREVIEW_MODE)
    {
        if(MMIDC_GetDefaultScreenMode() == SCREEN_MODE_HORIIZONTAL && MMIDC_GetScreenMode() == SCREEN_MODE_VERTICAL)
        {
            MMIDC_SetScreenMode(SCREEN_MODE_HORIIZONTAL);
            MMIDC_SetLcdForRotate(TRUE);
        }
        MMIDC_FlowFunction(DC_CONTINUE);
        MMIDC_DisplaySettingTip();
        MMIDC_DisplaySoftKey(DV_PREVIEW_MODE);
    }
    else if(MMIDC_GetCurrentMode() == DV_REVIEW_MODE)
    {
        SCI_TRACE_LOW("[MMIDC] HandleGetFocus flow_mode=%d",MMIDC_GetCurrentMode());
        //若是视频文件小于等于5个字节时，认为创建文件失败，
        //直接回预览界面
        if (MMIDC_GetVideoFileSize() <= 5)
        {
            MMIDC_FlowFunction(DC_BACKWARD);
            MMIDC_DisplaySettingTip();
            MMIDC_DisplaySoftKey(DV_PREVIEW_MODE);
        }else{
            MMIDC_FlowFunction(DC_CONTINUE);
            MMIDC_SetTransparentColor(MMI_BLACK_COLOR);
            MMIDC_DisplayVideoReview();
        }
    }
    else
    {
        MMIDC_FlowFunction(DC_CONTINUE);
        MMIDC_DisplaySoftKey(MMIDC_GetCurrentMode());
    }
    MMIDC_CompleteDisplayOSD();
}

/*****************************************************************************/
// 	Description : handle the message of dc window
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void HandleWinTimer(uint8 timer_id)
{
    if(s_3seconds_timer == timer_id)
    {
        SCI_TRACE_LOW("[MMIDC]: MSG_TIMER s_3seconds_timer");
        if(MMIDC_GetCurrentMode() == DC_PREVIEW_MODE && !MMIDC_GetSettingItemHandle()->SettingItemIsOpen())
        {
            MMIDC_DisplayZoomTip(INVALID_DATA);
            CloseTipDisplayTimer();
            MMIDC_CompleteDisplayOSD();
        }
    }
    else if(s_timer_delay == timer_id)
    {
        static uint8 short_voice = 0;

        SCI_TRACE_LOW("[MMIDC]: MSG_TIMER s_timer_delay");
        if(1 == s_timer_count_down && 4 == short_voice)
        {
            MMIAPISET_StopAllRing(FALSE);
            MMIDC_SetCurrentMode(DC_PREVIEW_MODE);
            Capture();
            CloseShootDelayTimer();
            s_timer_count_down = 0;
#ifndef ATECH_SOFT	// limn delete for Spt00033615 2次拍照，3s设置失效	 @2010.12.07
            MMIDC_SetSelfShootDelayTime(SELF_SHOOT_DISABLE);
#endif
            MMIDC_CompleteDisplayOSD();
        }
        else
        {
            if(s_timer_count_down > 2)
            {
                short_voice = 0;
                s_timer_count_down --;
                MMIDC_RemoveAllOSD();
                MMIDC_DisplayDelaySecondsTip(s_timer_count_down);
                CloseShootDelayTimer();
                StartShootDelayTimer(MMI_1SECONDS);
                if(MMIDC_GetCurrentMode() == DC_CAPTURE_CONT_DOWN_MODE)
                {
                    MMIDC_PlayCountVoice(1);
                }
            }
            else
            {
                short_voice ++;
                if(2 == short_voice)
                {
                    s_timer_count_down = 1;
                }
                MMIDC_RemoveAllOSD();
                MMIDC_DisplayDelaySecondsTip(s_timer_count_down);
                CloseShootDelayTimer();
                StartShootDelayTimer(500);
                if(MMIDC_GetCurrentMode() == DC_CAPTURE_CONT_DOWN_MODE)
                {
                    MMIDC_PlayCountVoice(1);
                }
            }
        }
    }
    else if(s_updown_tip_timer == timer_id)
    {
        SCI_TRACE_LOW("[MMIDC]: MSG_TIMER s_updown_tip_timer");
        if(!MMIDC_GetIconHandle()->IconIsOpen())
        {
            if((MMIDC_GetCurrentMode() == DC_PREVIEW_MODE || MMIDC_GetCurrentMode() == DV_PREVIEW_MODE))
            {
                MMIDC_RemoveAllOSD();
                MMIDC_DisplaySettingTip();
            }
        }
        CloseUpDownTipTimer();
    }
    else if (s_switch_osd_key_timer_id == timer_id)
    {
        SCI_TRACE_LOW("[MMIDC]: MSG_TIMER s_switch_osd_key_timer_id");
        StopSwitchOSDMenuItemsKeyTimer();
        StartSwitchOSDMenuItemsKeyTimer(s_last_key_down);
        if(LEFT_KEY == s_last_key_down)
        {
            if(!MMIDC_IsReviewing() && !MMIDC_IsCapturing() && !MMIDC_IsSettingParamCtrl())
            {
                KeyFunction(DC_LEFT_KEY);
            }
        }
        else if(RIGHT_KEY == s_last_key_down)
        {
            if(!MMIDC_IsReviewing() && !MMIDC_IsCapturing() && !MMIDC_IsSettingParamCtrl())
            {
                KeyFunction(DC_RIGHT_KEY);
            }
        }
        else if(UP_KEY == s_last_key_down)
        {
            if(!MMIDC_IsReviewing() && !MMIDC_IsCapturing() && !MMIDC_IsSettingParamCtrl())
            {
                KeyFunction(DC_UP_KEY);
            }
        }
        else if(DOWN_KEY == s_last_key_down)
        {
            if(!MMIDC_IsReviewing() && !MMIDC_IsCapturing() && !MMIDC_IsSettingParamCtrl())
            {
                KeyFunction(DC_DOWN_KEY);
            }
        }
    }
    else if(s_record_timer == timer_id)
    {
        SCI_TRACE_LOW("[MMIDC]: MSG_TIMER s_record_timer");
        s_record_total_time ++;
        MMIDC_DisplayVideoRecordTip(DV_RECORD_MODE);
    }
    else if(s_text_scroll_timer == timer_id)
    {
        GUI_RECT_T temp = {0};
        
        SCI_TRACE_LOW("[MMIDC]: MSG_TIMER s_text_scroll_timer");
        MMIDC_StopTextScrollTimer();
        MMIDC_DisplayString(temp, PNULL, 0,FALSE);/*lint !e64*/
        MMIDC_CompleteDisplayOSD();
    }
    else if(MMIDC_GetCallbackTimer() == timer_id)
    {
        MMIDC_PostStateMsg();
        MMIDC_ErrorTipForExit(TXT_SYSTEM_FAILURE);/*lint !e64*/
    }
    else if(s_is_chip_test_timer == timer_id)
    {
        KeyFunction(DC_OK_KEY);
    }
    else if(s_hit_timer == timer_id)
    {
#ifdef ATECH_SOFT	//zhangy_sh
		if ((DC_PREVIEW_MODE == MMIDC_GetCurrentMode()) || (DV_PREVIEW_MODE == MMIDC_GetCurrentMode()))
#else
        if (DC_PREVIEW_MODE == MMIDC_GetCurrentMode())
#endif
        {
            SCI_TRACE_LOW("[MMIDC] s_hit_timer exec");
            MMIDC_PostStateMsg();
        }
    }
#ifdef ATECH_ENG_SUPPORT
	else if(g_auto_test_time_id == timer_id)
	{
       	KeyFunction(DC_OK_KEY);
		if(g_auto_test_time_id != 0)
		{
			MMK_StopTimer(g_auto_test_time_id);
			g_auto_test_time_id = 0;
		}
		
		SCI_Sleep(3000);
		MMK_PostMsg(MMIDC_MAIN_WIN_ID, MSG_APP_CANCEL, PNULL, NULL);

		SCI_Sleep(3000);
		Save();
		SCI_Sleep(3000);
		
		g_eng_test_round_time++;
		if(g_eng_test_round_time < ENG_AUTO_TEST_ROUND_NUM)
		{
			MMK_CloseWin(MMIDC_MAIN_WIN_ID);
			MMIENG_EnterAutoTestMenu();
		}
		else
		{
			g_is_eng_auto_test = FALSE;
			g_eng_test_round_time = 0;
			MMK_CloseWin(MMIDC_MAIN_WIN_ID);
		}
		//MMIAPIBT_OpenMainMenuWin();
	}
#endif
}

/*****************************************************************************/
// 	Description : handle the message of dc window
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL MMI_RESULT_E HandleCameraWinMsg(
                                      MMI_WIN_ID_T		win_id, 
                                      MMI_MESSAGE_ID_E	msg_id, 
                                      DPARAM				param	
                                      )
{
    MMI_RESULT_E recode	= MMI_RESULT_TRUE;
    MMISET_VALIDATE_PRIVACY_RETURN_T *result_info_ptr = (MMISET_VALIDATE_PRIVACY_RETURN_T *)param; //added
    MMIDC_DISPLAY_MODE_E  display_mode = MMIDC_GetDisplayMode();
    BOOLEAN  flip_status = FALSE;

    int32 screen_width = 0;
    int32 screen_height = 0;

    //camera exit when do nothing o dc preview state and in one minute.
#ifdef ATECH_SOFT	//zhangy_sh
	if ((DC_PREVIEW_MODE == MMIDC_GetCurrentMode()) || (DV_PREVIEW_MODE == MMIDC_GetCurrentMode()))
#else
    if (DC_PREVIEW_MODE == MMIDC_GetCurrentMode())
#endif
    {
        if (((msg_id >= MSG_KEYDOWN_UP) && (msg_id <= MSG_KEYDOWN_EXCLAMATION))
            || ((msg_id >=MSG_TP_PRESS_DOWN) && (msg_id <= MSG_TP_PRESS_DRAG))
           )
        {
            MMIDC_ResetHitTimer();
        }
    }
    else
    {
        MMIDC_CloseHitTimer();
    }

    SCI_TRACE_LOW("[MMIDC]: HandleCameraWinMsg, msg_id = 0x%.2x", msg_id);
    switch(msg_id)
    {
    case MSG_OPEN_WINDOW:
        SCI_TRACE_LOW("[MMIDC]: MSG_OPEN_WINDOW");

#ifdef COOLBAR_ENABLE
        MMIMAIN_SuspendCoolbarInit(TRUE);
#endif

//QQ运行后，内存不足，进入DC要提示QQ退?
#if 0	// add by hyg 20101014, for gongjj advice
#ifdef QQ_SUPPORT_TENCENT
        if (QQ_IsRunning())
        {
            MMIDC_ErrorTipForExit(TXT_EXIT_QQ);
            MMIDC_PostStateMsg();
            break;
        }
#endif
#endif
#if 0
#ifdef TENCENT_APP_QQ // add by hyg 20101022, for quit Camera while run QQ	
		if(QIsQQRuning())
		{
		       MMIDC_ErrorTipForExit(TXT_EXIT_QQ);
			MMIDC_PostStateMsg();
			break;
		}
#endif // end of hyg
#endif

#ifdef ATECH_SUBLCD_SUPPORT  // atech limn add for 进入照相机时只显示logo，不刷新
		MMISUB_SetSubLcdDisplay(FALSE, TRUE,SUB_CONTENT_CAMERA_LOGO, NULL);
		MMISUB_IsPermitUpdate(FALSE);
#endif

        GUIBLOCK_SetPermitChangeBlock(FALSE);

        flip_status = MMIDEFAULT_GetFlipStatus();
        SCI_TRACE_LOW("[MMIDC] HandleCameraWinMsg flip_status = %d",flip_status);
        if (flip_status)
        {
            MMIDC_SetLcdID(MAIN_LCD_ID);
        }
        else
        {
           MMIDC_SetLcdID(SUB_LCD_ID);
        }

        MMIDC_SetTransparentColor(MMIDC_TRANSPARENT_COLOR);
        MMIDC_ClearMainBlock(MMI_WINDOW_BACKGROUND_COLOR);
        MMIAUDIO_PauseBgPlay(MMIBGPLAY_TYPE_ALL, MMIBGPLAY_MODULE_DC);
        MMIDEFAULT_EnableKeyRing(FALSE);
        MMIDEFAULT_EnableTpRing(FALSE);
        MMIAPISET_StopAllRing(FALSE);
        MMIDEFAULT_TurnOnBackLight(); 
        MMIDEFAULT_AllowTurnOffBackLight(FALSE);	
        MMIDC_InitSettingParamCtrl();

        s_timer_count_down=0;//added, @robert.wang, 09-8-21, cr149228
#ifdef ATECH_SOFT	//zhangy_sh
		if ((DC_PREVIEW_MODE == MMIDC_GetCurrentMode()) || (DV_PREVIEW_MODE == MMIDC_GetCurrentMode()))
#else
        if (DC_PREVIEW_MODE == MMIDC_GetCurrentMode())
#endif
        {
            MMIDC_ResetHitTimer();
        }

#ifdef ATECH_ENG_SUPPORT
		if(g_is_eng_auto_test)
		{
			if(0 != g_auto_test_time_id)
			{
				MMK_StopTimer(g_auto_test_time_id);
				g_auto_test_time_id = 0;
			}
			
			g_auto_test_time_id = MMK_CreateWinTimer(win_id,8000,FALSE);
		}
#endif
		
        if(MMIDC_FlowStart())
        {
            MMIDC_RemoveAllOSD();
            MMIDC_DisplaySettingTip();
            MMIDC_DisplaySoftKey(MMIDC_GetCurrentMode());
            MMIDC_CompleteDisplayOSD();
        }
        if(s_is_chip_test)
        {
            if(s_is_chip_test_timer > 0)
            {
                MMK_StopTimer(s_is_chip_test_timer);
            }
            s_is_chip_test_timer = MMK_CreateTimer(5000, TRUE);		
        }
        s_is_pressed_save = FALSE;
        break;
        
    case MSG_LOSE_FOCUS:
        SCI_TRACE_LOW("[MMIDC]: HandleCameraWinMsg, MSG_LOSE_FOCUS");	

        //DC无论在小屏时, 还是大屏失去焦点时，都允许刷小屏 CR175065
    	MMISUB_IsPermitUpdate(TRUE);
    	//end CR175065

        MMIDC_HandleLoseFocus();
        MMIDEFAULT_EnableKeyRing(TRUE);
        MMIDEFAULT_EnableTpRing(TRUE);
        MMIAPISET_StopAllRing(TRUE);

        GUIBLOCK_SetPermitChangeBlock(TRUE);

        MMIDC_CloseCallbackTimer();
		
#ifdef ATECH_SUBLCD_SUPPORT  // wuty add for Spt31814 Spt00032942
		MMISUB_SetSubLcdDisplay(FALSE, FALSE,SUB_CONTENT_CAMERA_LOGO, NULL);
		MMISUB_IsPermitUpdate(TRUE);
#endif 
        break;
        
    case MSG_GET_FOCUS:
        SCI_TRACE_LOW("[MMIDC]: HandleCameraWinMsg, MSG_GET_FOCUS");

#ifdef COOLBAR_ENABLE
        MMIMAIN_SuspendCoolbarInit(TRUE);
#endif
        if(MMK_IsOpenWin(win_id))
        {
            //DC在小屏时，不允许刷新小屏， CR175065
            flip_status = MMIDEFAULT_GetFlipStatus();
            if (!flip_status)
            {
    	        MMISUB_IsPermitUpdate(FALSE);
            }
            else
            {
                //在DV REVIEW 模式时，不再刷新屏幕,否则回放结束时，会花屏或分块,CR176249
                if (DV_REVIEW_MODE != MMIDC_GetCurrentMode())
                {
                    //刷新最后一帧数据，插入USB时，显示选中后的白色焦点, CR165851
                    if(GUI_LCD_ROTATE_90 == MMIAPICOM_GetCommonRotateMode())
                    {   
                        screen_width = MMIDC_GetPreviewHeight();
                        screen_height = MMIDC_GetPreviewWidth();
                    }
                    else
                    {
                        screen_width = MMIDC_GetPreviewWidth();
                        screen_height = MMIDC_GetPreviewHeight();
                    }
                    GUILCD_InvalidateLCDRect(GUI_MAIN_LCD_ID, 0, 0, screen_width - 1, screen_height - 1,GUIREF_GetUpdateBlockSet(GUI_BLOCK_MAIN));
                    //end cr165851
                }
            }
            //end cr175065
        
            MMI_Enable3DMMI(FALSE);
            GUIBLOCK_SetPermitChangeBlock(FALSE);
            MMIAUDIO_PauseBgPlay(MMIBGPLAY_TYPE_ALL, MMIBGPLAY_MODULE_DC);
            MMIDEFAULT_EnableKeyRing(FALSE);
            MMIDEFAULT_EnableTpRing(FALSE);
            MMIAPISET_StopAllRing(FALSE);
            MMIDEFAULT_TurnOnBackLight();
            MMIDEFAULT_AllowTurnOffBackLight(FALSE);	
            MMIDC_InitSettingParamCtrl();

#ifdef ATECH_SOFT	//zhangy_sh
			if ((DC_PREVIEW_MODE == MMIDC_GetCurrentMode()) || (DV_PREVIEW_MODE == MMIDC_GetCurrentMode()))
#else
            if (DC_PREVIEW_MODE == MMIDC_GetCurrentMode())
#endif				
            {
                MMIDC_ResetHitTimer();
            }

            
            if(MMIDC_GetSettingItemHandle()->SettingItemIsOpen())
            {
                MMITHEME_InitMainLcdStortUpdateRect();
                MMIDC_OpenSetting();
                MMIDC_CompleteDisplayOSD();
            }
            else
            {
                MMIDC_HandleGetFocus();
            }			
#ifdef ATECH_SUBLCD_SUPPORT  // wuty add for Spt31814 Spt00032942
			MMISUB_SetSubLcdDisplay(FALSE, TRUE,SUB_CONTENT_CAMERA_LOGO, NULL);
			MMISUB_IsPermitUpdate(FALSE);
#endif 
        }
        break;
        
    case MSG_DC_CAPTURE_SUCCESS_MSG:	/** capture success **/
        SCI_TRACE_LOW("[MMIDC]: MSG_DC_CAPTURE_SUCCESS_MSG");
        if(MMK_IsFocusWin(win_id))
        {
#ifdef PLATFORM_SC6600L
            if(SUB_LCD_ID == MMIDC_GetLcdID())
            {
                MMIDC_SetCurrentMode(DC_REVIEW_MODE);
                Save();
                MMIDC_ClearSUBLCD();
                RSSave();//返回预览状态
                MMIDC_CompleteDisplayOSD();
                s_is_pressed_save = FALSE;
            }
            else if(MMIDC_GetIsAutoSave() == AUTO_SAVE_ON || s_is_pressed_save)
            {
                MMIDC_SetCurrentMode(DC_REVIEW_MODE);
                Save();
                MMIDC_CompleteDisplayOSD();
                s_is_pressed_save = FALSE;
            }
            else
            {
                MMIDC_FlowFunction(DC_FORWARD);
            }
#elif PLATFORM_SC6600R
            if(MMIDC_GetIsAutoSave() == AUTO_SAVE_ON)
            {
                MMIDC_SetCurrentMode(DC_REVIEW_MODE);
            }
            MMIDC_FlowFunction(DC_FORWARD);
            MMIDC_CompleteDisplayOSD();
#endif
        }
        break;

    case MSG_DC_PLAY_SHUTTER_VOICE:
        MMIAPISET_PlayRing(0, FALSE, 0, 1, MMISET_RING_TYPE_DC, MMIDC_ShutterViceCallback);/*lint !e64*/
        break;

    case MSG_DC_REVIEW_SUCCESS_MSG:
        SCI_TRACE_LOW("[MMIDC]: MSG_DC_REVIEW_SUCCESS_MSG");
        if(s_is_chip_test)
        {
            SCI_Sleep(3000);
            MMIDC_PostStateMsg();
        }
        break;
        
    case MSG_DC_CAPTURE_FAILE_MSG:
        SCI_TRACE_LOW("[MMIDC]: MSG_DC_CAPTURE_FAILE_MSG");
        MMIDC_OpenAlertWin(MMIPUB_SOFTKEY_ONE, TXT_DC_CAPTURE_FAILED, IMAGE_PUBWIN_FAIL, MMI_2SECONDS);
        break;
        
    case MSG_RECORD_END:
        SCI_TRACE_LOW("[MMIDC]: MSG_RECORD_END = %d", *((DV_END_TYPE_E*)param));
        switch(*((DV_END_TYPE_E*)param))
        {
        case DV_END_ERROR:
            MMK_CloseWin(win_id);
            break;
        case DV_END_ROOM_INSUF_ERROR:
            MMIDC_ErrorTip(TXT_NO_SPACE);
            break;
        default:
            break;
        }
        break;
        
    case MSG_APP_WEB:
    case MSG_CTL_MIDSK:
        SCI_TRACE_LOW("[MMIDC]: MSG_APP_WEB");
#ifdef PLATFORM_SC6600L
        if(MMIDC_IsCapturing())
        {
            Save();
        }
        else
#endif
        if(!MMIDC_IsReviewing() && !MMIDC_IsCapturing() && !MMIDC_IsSettingParamCtrl())
        {
            KeyFunction(DC_OK_KEY);
        }
        break;
            
    case MSG_TIMER:
#ifdef ATECH_ENG_SUPPORT
        if (g_is_eng_auto_test)
        {
            HandleWinTimer(*((uint8*)param));
        }
        else
        {
#endif        
            if(MMK_IsFocusWin(win_id))
            {
                HandleWinTimer(*((uint8*)param));
            }
#ifdef ATECH_ENG_SUPPORT			
        }
#endif
        break;
        
    case MSG_APP_CAMERA:
        if(!MMIDC_IsReviewing() && !MMIDC_IsCapturing() && !MMIDC_IsSettingParamCtrl())
        {
            if(MMIDC_GetCurrentMode() == DC_PREVIEW_MODE || MMIDC_GetCurrentMode() == DV_PREVIEW_MODE)
            {
                MMIDC_GetIconHandle()->IconClose();
                MMIDC_GetMenuHandle()->MenuClose();
                CleanAdjustRect();
                while(MMIDC_IsSettingParamCtrl())
                {
                    SCI_Sleep(10);
                }
                KeyFunction(DC_OK_KEY);
            }
        }
        break;
        
    case MSG_FULL_PAINT:
        if(!MMK_IsFocusWin(win_id))
        {
            MMIDC_ClearMainBlock(MMI_WINDOW_BACKGROUND_COLOR);
        }
        break;
        
    case MSG_APP_OK:
        SCI_TRACE_LOW("[MMIDC]: MSG_APP_OK");
#ifdef PLATFORM_SC6600L
        if(MMIDC_IsCapturing())
        {
            Save();
        }
        else
#endif
        if(!MMIDC_IsReviewing() && !MMIDC_IsCapturing() && !MMIDC_IsSettingParamCtrl())
        {
            KeyFunction(DC_LEFT_SOFT_KEY);
        }
        break;		
        	
    case MSG_APP_CANCEL:
        SCI_TRACE_LOW("[MMIDC]: MSG_APP_CANCEL");
#ifdef ATECH_ENG_SUPPORT		
		if(g_is_eng_auto_test)
		{
			g_is_eng_auto_test = FALSE;
		}
#endif
        if(!MMIDC_IsReviewing() && !MMIDC_IsCapturing() && !MMIDC_IsSettingParamCtrl())
        {
            KeyFunction(DC_RIGHT_SOFT_KEY);
        }
        break;
        
    case MSG_APP_LEFT:
        SCI_TRACE_LOW("[MMIDC]: MSG_APP_LEFT");
        if(!MMIDC_IsReviewing() && !MMIDC_IsCapturing() && !MMIDC_IsSettingParamCtrl())
        {
            KeyFunction(DC_LEFT_KEY);
            StartSwitchOSDMenuItemsKeyTimer(LEFT_KEY);
        }
        break;
        
    case MSG_APP_RIGHT:
        SCI_TRACE_LOW("[MMIDC]: MSG_APP_RIGHT");
        if(!MMIDC_IsReviewing() && !MMIDC_IsCapturing() && !MMIDC_IsSettingParamCtrl())
        {
            KeyFunction(DC_RIGHT_KEY);
            StartSwitchOSDMenuItemsKeyTimer(RIGHT_KEY);
        }
        break;
        
    case MSG_KEYUP_LEFT:
    case MSG_KEYUP_RIGHT:	
    case MSG_KEYPRESSUP_LEFT:
    case MSG_KEYPRESSUP_RIGHT:
    case MSG_KEYUP_UP:
    case MSG_KEYUP_DOWN:	
    case MSG_KEYPRESSUP_UP:
    case MSG_KEYPRESSUP_DOWN:
        SCI_TRACE_LOW("[MMIDC]: MSG_KEYUP");
        StopSwitchOSDMenuItemsKeyTimer();
        break;
        
    case MSG_APP_UP:
        SCI_TRACE_LOW("[MMIDC]: MSG_APP_UP");
        if(!MMIDC_IsReviewing() && !MMIDC_IsCapturing() && !MMIDC_IsSettingParamCtrl())
        {
            KeyFunction(DC_UP_KEY);
            StartSwitchOSDMenuItemsKeyTimer(UP_KEY);
        }
        break;
        
    case MSG_APP_DOWN:
        SCI_TRACE_LOW("[MMIDC]: MSG_APP_DOWN");
        if(!MMIDC_IsReviewing() && !MMIDC_IsCapturing() && !MMIDC_IsSettingParamCtrl())
        {
            KeyFunction(DC_DOWN_KEY); 
            StartSwitchOSDMenuItemsKeyTimer(DOWN_KEY);
        }
        break;
        
    case MSG_PROMPTWIN_OK:
        if(MMIDC_GetCameraMode() == CAMERA_MODE_DC)
        {
            MMIDC_DeletePhoto();
            MMIDC_OpenAlertWin(MMIPUB_SOFTKEY_ONE, TXT_SUCCESS, IMAGE_PUBWIN_SUCCESS, MMI_2SECONDS);
            MMIPUB_CloseQuerytWin(PNULL);
        }
        else
        {
            MMIDC_DeleteNoSavedFile();
            MMIDC_OpenAlertWin(MMIPUB_SOFTKEY_ONE, TXT_SUCCESS, IMAGE_PUBWIN_SUCCESS, MMI_2SECONDS);
            MMIPUB_CloseQuerytWin(PNULL);
            MMIDC_SetCurrentMode(DV_PREVIEW_MODE);
        }
        break;
        
    case MSG_PROMPTWIN_CANCEL: 
        MMIPUB_CloseQuerytWin(PNULL);
        break;
        
    case MSG_TP_PRESS_UP:
        if(!MMIDC_IsReviewing() && !MMIDC_IsCapturing() && !MMIDC_IsSettingParamCtrl())
        {
            GUI_POINT_T	point = {0};
            point.x = MMK_GET_TP_X(param);
            point.y = MMK_GET_TP_Y(param);
            HandleDCWinTpPressDown(point);
        }
        break;	
    case MSG_DC_MMI_STATE:
        MMIAPIDC_Exit();
        break;
    case MSG_CLOSE_WINDOW:
        SCI_TRACE_LOW("[MMIDC]: MSG_CLOSE_WINDOW");        
        MMIDC_RemoveAllOSD();
		
#ifdef COOLBAR_ENABLE
        MMIMAIN_SuspendCoolbarInit(FALSE);
#endif	

        //close all timer  
        CloseDCAllTimers();

        MMIDC_SaveSettings();
        MMIDC_ClearGUIValue();
        MMIDC_FlowFunction(DC_EXIT);
        MMIDC_SetLcdForRotate(FALSE);
        
        if (GUI_LCD_ROTATE_90 == MMIAPICOM_GetCommonRotateMode())
        {
    		GUILCD_SetLcdRotMode(GUI_MAIN_LCD_ID, GUI_LCD_ROTATE_90);
        }
        else
        {
            GUILCD_SetLcdRotMode(GUI_MAIN_LCD_ID, GUI_LCD_ROTATE_0);/*lint !e64*/
        }
        
        CleanAdjustRect();
        GUI_LCDSetForOSD(FALSE);
        MMIDC_ClearMainBlock(MMI_WINDOW_BACKGROUND_COLOR);
        MMITHEME_InitMainLcdStortUpdateRect();
        MMIDC_ResetScrollNumber();
        MMIAUDIO_ResumeBgPlay(MMIBGPLAY_TYPE_ALL, MMIBGPLAY_MODULE_DC);
        MMIDEFAULT_EnableKeyRing(TRUE);
        MMIDEFAULT_EnableTpRing(TRUE);
        MMIAPISET_StopAllRing(TRUE);
        MMIDEFAULT_AllowTurnOffBackLight(TRUE);

        GUIBLOCK_SetPermitChangeBlock(TRUE);

        MMI_Enable3DMMI(TRUE);
        MMIDC_CloseCallbackTimer();
        MMIDC_FreeGUI();
        MMIDC_FreeSettingMemory();
        MMIDC_DeleteSaveData();

        MMIAPIDC_SetChangeFolderState(FALSE); //added, @robert.wang, 09-9-8, cr151682

        MMIDC_FreeLastFrame();

        if(s_is_chip_test)
        {
            MMK_StopTimer(s_is_chip_test_timer);
            s_is_chip_test_timer = 0;
            s_is_chip_test = FALSE;
        }
		
#ifdef ATECH_SUBLCD_SUPPORT  // atech limn add for 进入照相机时只显示logo，不刷新
		MMISUB_SetSubLcdDisplay(FALSE, FALSE,SUB_CONTENT_CAMERA_LOGO, NULL);
		MMISUB_IsPermitUpdate(TRUE);
#endif 

        break;
    case MSG_SET_VALIDATE_PRIVACY_PWD_RETURN:
            SCI_ASSERT( PNULL != result_info_ptr );
            
            if ( result_info_ptr->is_validate_sucess )
            {
                    switch ( result_info_ptr->protect_type )
                    {
                     case MMISET_PROTECT_MYDOC_TYPE:
                        MMIDC_OpenMovieWin();
                        MMIDC_SetCurrentMode(DV_PREVIEW_MODE);					
                        break;
                     default:
                        break;
                     }
              }
       break;        

#ifdef ATECH_PRODUCT_TYPE_SLIDE//zhaoqf modify for Spt 32568       
	//jesse yu add for sub-lcd start
    case MSG_KEYDOWN_FLIP:
		{
	        SCI_TRACE_LOW("[MMIDC] HandleCameraWinMsg: MSG_KEYDOWN_FLIP!");
			if(MMK_IsFocusWin(MMIDC_MAIN_WIN_ID) && DC_PREVIEW_MODE == MMIDC_GetCurrentMode()
				&& MMIAPIDC_IsPreviewing())
			{
				MMIDC_HandleLoseFocus();
				MMIDC_SetLcdID(SUB_LCD_ID);
				MMIDC_HandleGetFocus();
				MMISUB_IsPermitUpdate(FALSE);
			}
			else
			{
				recode = MMI_RESULT_FALSE;
			}
		}
		break;

    case MSG_KEYUP_FLIP:
		{
	        SCI_TRACE_LOW("[MMIDC] HandleCameraWinMsg: MSG_KEYUP_FLIP! %d %d",(MMK_IsFocusWin(MMIDC_MAIN_WIN_ID)),(MMIDC_GetCurrentMode()));
			MMIDC_SetLcdID(MAIN_LCD_ID);
			if(MMK_IsFocusWin(MMIDC_MAIN_WIN_ID) && DC_PREVIEW_MODE == MMIDC_GetCurrentMode()
				&& MMIAPIDC_IsPreviewing())
			{
				SCI_TRACE_LOW("[MMIDC] Go on preview!");
				MMIDC_HandleLoseFocus();
				MMIDC_HandleGetFocus();
	            MMIDC_RemoveAllOSD();
	            MMIDC_DisplaySettingTip();
	            MMIDC_DisplaySoftKey(MMIDC_GetCurrentMode());
	            MMIDC_CompleteDisplayOSD();
			}
			else if(MMK_IsFocusWin(MMIDC_MAIN_WIN_ID) && DC_REVIEW_MODE == MMIDC_GetCurrentMode()
				||DC_SAVE_MODE == MMIDC_GetCurrentMode())
			{
				SCI_TRACE_LOW("[MMIDC] Display picture!");
				MMIDC_UnConfigBlock();
				MMIDC_ConfigBlock(MMIDC_GetCameraMode());
				MMIDC_RemoveAllOSD();
	            MMIDC_ClearOSDBlock();
				if(DC_SAVE_MODE == MMIDC_GetCurrentMode())
				{
				    MMIDC_DisplayCurrentFileName();
				}
	            MMIDC_DisplaySoftKey(MMIDC_GetCurrentMode());
	            MMIDC_CompleteDisplayOSD();
			    MMIDC_ReviewPhotos();
			}
			MMISUB_IsPermitUpdate(TRUE);
			recode = MMI_RESULT_FALSE;
	    }
        break;
	//jesse yu add for sub-lcd end
#endif
	
    default:
        recode = MMI_RESULT_FALSE;
        break;
    }
    return recode;
}

/*****************************************************************************/
// 	Description : get camera is open
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC BOOLEAN MMIAPIDC_IsOpened(void)
{
    return (MMK_IsOpenWin(MMIDC_MAIN_WIN_ID) || MMIENG_IsDCEngMode());
}

/*=====================unused function delete=====================*/
/*****************************************************************************/
// 	Description : get camera is focus
//	Global resource dependence : none
//  Author:merlin.yang
//	Note:
/*****************************************************************************/
/*PUBLIC BOOLEAN MMIAPIDC_IsFocusWin(void)
{
    SCI_TRACE_LOW("merlin [MMIDC]: MMIAPIDCWin_IsFocused %d",MMK_IsFocusWin(MMIDC_MAIN_WIN_ID));
    return (MMK_IsFocusWin(MMIDC_MAIN_WIN_ID) );
}*/

/*****************************************************************************/
// 	Description : close camera
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC void MMIAPIDC_Exit(void)
{
    SCI_TRACE_LOW("[MMIDC]: MMIAPIDC_Exit");
	
    if(MMK_IsOpenWin(MMIDC_MAIN_WIN_ID))
    {
        MMK_CloseWin(MMIDC_MAIN_WIN_ID);
    }
    if(MMK_IsOpenWin(MMIDC_HELP_WIN_ID))
    {
        MMK_CloseWin(MMIDC_HELP_WIN_ID);
    }
    if(MMK_IsOpenWin(MMIDC_SEND_WIN_ID))
    {
        MMK_CloseWin(MMIDC_SEND_WIN_ID);
    }
    if(MMK_IsOpenWin(MMIDC_ON_OFF_SET_WIN_ID))
    {
        MMK_CloseWin(MMIDC_ON_OFF_SET_WIN_ID);
    }
    if(MMK_IsOpenWin(MMIDC_QUALITY_WIN_ID))
    {
        MMK_CloseWin(MMIDC_QUALITY_WIN_ID);
    }
    if(MMK_IsOpenWin(MMIDC_PHOTO_OPTION_WIN_ID))
    {
        MMK_CloseWin(MMIDC_PHOTO_OPTION_WIN_ID);
    }
    if(MMK_IsOpenWin(MMIDC_VIDEO_OPTION_WIN_ID))
    {
        MMK_CloseWin(MMIDC_VIDEO_OPTION_WIN_ID);
    }
    if(MMK_IsOpenWin(MMIDC_PHOTO_REVIEW_OPTION_WIN_ID))
    {
        MMK_CloseWin(MMIDC_PHOTO_REVIEW_OPTION_WIN_ID);
    }
    if(MMK_IsOpenWin(MMIDC_VIDEO_REVIEW_OPTION_WIN_ID))
    {
        MMK_CloseWin(MMIDC_VIDEO_REVIEW_OPTION_WIN_ID);
    }
    if(MMK_IsOpenWin(MMIDC_SIZE_WIN_ID))
    {
        MMK_CloseWin(MMIDC_SIZE_WIN_ID);
    }
    if(MMK_IsOpenWin(MMIDC_ENVIRONMENT_WIN_ID))
    {
        MMK_CloseWin(MMIDC_ENVIRONMENT_WIN_ID);
    }
    if(MMK_IsOpenWin(MMIDC_BRIGHTNESS_WIN_ID))
    {
        MMK_CloseWin(MMIDC_BRIGHTNESS_WIN_ID);
    }
    if(MMK_IsOpenWin(MMIDC_CONTRAST_WIN_ID))
    {
        MMK_CloseWin(MMIDC_CONTRAST_WIN_ID);
    }
    if(MMK_IsOpenWin(MMIDC_SELF_SHOOT_WIN_ID))
    {
        MMK_CloseWin(MMIDC_SELF_SHOOT_WIN_ID);
    }
    if(MMK_IsOpenWin(MMIDC_EFFECT_WIN_ID))
    {
        MMK_CloseWin(MMIDC_EFFECT_WIN_ID);
    }
    if(MMK_IsOpenWin(MMIDC_PHOTO_OPTION_SETTING_WIN_ID))
    {
        MMK_CloseWin(MMIDC_PHOTO_OPTION_SETTING_WIN_ID);
    }
    if(MMK_IsOpenWin(MMIDC_VIDEO_OPTION_SETTING_WIN_ID))
    {
        MMK_CloseWin(MMIDC_VIDEO_OPTION_SETTING_WIN_ID);
    }
    if(MMK_IsOpenWin(MMICAMERA_STORAGE_SPACE_WIN_ID))
    {
        MMK_CloseWin(MMICAMERA_STORAGE_SPACE_WIN_ID);
    }
	MMIPUB_CloseQuerytWin(PNULL);
    MMIPUB_CloseWaitWin(MMIDC_MAIN_WIN_ID);
    MMIPUB_CloseAlertWin();
}

/*****************************************************************************/
// 	Description : interrupt camera
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC void MMIAPIDC_Interrupt(void)
{
    SCI_TRACE_LOW("[MMIDC]: MMIAPIDC_Interrupt");
    if(MMK_IsFocusWin(MMIDC_MAIN_WIN_ID))
    {
        MMK_SendMsg(MMIDC_MAIN_WIN_ID, MSG_LOSE_FOCUS, 0);
    }
}

/*****************************************************************************/
// 	Description : handle the message of dc window
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC uint32 MMIDC_GetRecordSecond(void)
{
    return s_record_total_time;
}

/*****************************************************************************/
// 	Description : init camera module
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC void MMIAPIDC_InitModule(void)
{
    MMIDC_RegMenuWin();
#ifndef MMI_AUTOTEST_LOW_MEMORY
    MMI_RegWinIdNameArr(MMI_MODULE_CAMERA, mmidc_id_name_arr);/*lint !e64*/
#endif
    MMIDC_RegSettingNv();
}


/*****************************************************************************/
// 	Description : handle photo and video send window msg
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL MMI_RESULT_E HandleSendWinMsg(MMI_WIN_ID_T win_id, MMI_MESSAGE_ID_E msg_id, DPARAM param)
{
    MMI_RESULT_E            result      = MMI_RESULT_TRUE;
    MMI_MENU_ID_T	        menu_id     = 0;
    MMI_MENU_GROUP_ID_T     group_id    = 0;
    
    SCI_TRACE_LOW("mmidc_wintab.c,  HandleSendWinMsg(), msg_id = %x", msg_id);
    switch (msg_id)
    {
    case MSG_OPEN_WINDOW:
        MMK_SetAtvCtrl(MMIDC_SEND_WIN_ID, MMIDC_SEND_MENU_CTRL_ID);
        GUIWIN_SetSoftkeyTextId(win_id,  STXT_OK, NULL, STXT_RETURN, FALSE);
        break;
        
    case MSG_CTL_PENOK:
    case MSG_CTL_MIDSK:
    case MSG_CTL_OK:
    case MSG_APP_WEB:
        GUIMENU_GetId(MMIDC_SEND_MENU_CTRL_ID, (MMI_MENU_GROUP_ID_T *)(&group_id),(MMI_MENU_ID_T *) (&menu_id));
        switch ( menu_id )
        {
        case ID_CAMERA_SEND_BY_MMS:
#ifndef PROJECT_6802	//atech lanliyuan 20100308	Spt00029690					
            MMIDC_SendByMMS();
#else
			if (MMIAPISET_IsOpenPrivacyProtect(MMISET_PROTECT_SMS))
			{
				MMIAPISET_ValidatePrivacyPwd(win_id, MMISET_PROTECT_SMS_TYPE);
			}
			else
			{
				MMIDC_SendByMMS();
			}	
#endif
            break;
            
        case ID_CAMERA_SEND_BY_BT:
            MMIDC_SendByBt();
            break;
        default:
            break;
        }
        break;

#ifdef ATECH_SOFT
	//add atech lanliyuan 20100308	Spt00029690	
	case MSG_SET_VALIDATE_PRIVACY_PWD_RETURN:
		{
			MMISET_VALIDATE_PRIVACY_RETURN_T    result_info = {MMISET_PROTECT_SMS, FALSE};
			result_info = *((MMISET_VALIDATE_PRIVACY_RETURN_T *)param);
			if ((result_info.protect_type == MMISET_PROTECT_SMS_TYPE) && (result_info.is_validate_sucess))
			{
				MMIDC_SendByMMS();
				MMK_CloseWin(win_id);
			}
		}
		break;
	//add end	atech lanliyuan 20100308	Spt00029690	
#endif
        
        case MSG_CTL_CANCEL:
            MMK_CloseWin(win_id);
            break;
            
        default:
            result = MMI_RESULT_FALSE;
            break;
    }
    
    return (result);
}
#ifdef ATECH_SOFT
/*****************************************************************************/
// 	Description : handle photo and video send window msg
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL MMI_RESULT_E HandleDvSendWinMsg(MMI_WIN_ID_T win_id, MMI_MESSAGE_ID_E msg_id, DPARAM param)
{
    MMI_RESULT_E            result      = MMI_RESULT_TRUE;
    MMI_MENU_ID_T	        menu_id     = 0;
    MMI_MENU_GROUP_ID_T     group_id    = 0;
    
    SCI_TRACE_LOW("mmidc_wintab.c,  HandleDvSendWinMsg(), msg_id = %x", msg_id);
    switch (msg_id)
    {
    case MSG_OPEN_WINDOW:
        MMK_SetAtvCtrl(MMIDV_SEND_WIN_ID, MMIDV_SEND_MENU_CTRL_ID);
        GUIWIN_SetSoftkeyTextId(win_id,  STXT_OK, NULL, STXT_RETURN, FALSE);
        break;
        
    case MSG_CTL_PENOK:
    case MSG_CTL_MIDSK:
    case MSG_CTL_OK:
    case MSG_APP_WEB:
        GUIMENU_GetId(MMIDV_SEND_MENU_CTRL_ID, (MMI_MENU_GROUP_ID_T *)(&group_id),(MMI_MENU_ID_T *) (&menu_id));
        switch ( menu_id )
        {
        case ID_CAMERA_SEND_BY_BT:
            MMIDC_SendByBt();
            break;
        default:
            break;
        }
        break;

        case MSG_CTL_CANCEL:
            MMK_CloseWin(win_id);
            break;
            
        default:
            result = MMI_RESULT_FALSE;
            break;
    }
    
    return (result);
}
#endif
/*****************************************************************************/
// 	Description : open send photo window
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
//SFOX_SUPPORT
extern BOOLEAN SFOX_IsSFOXInUsing( void );
extern void MMIDC_SendBySFOX(void);

PUBLIC void MMIDC_OpenSendPhotoWin(void)
{
	//SFOX_SUPPORT
	if ( SFOX_IsSFOXInUsing() )
	{
		MMIDC_SendBySFOX();
		MMIAPIDC_Exit();
		return;
	}
	
    #ifndef BLUETOOTH_SUPPORT
        MMIDC_HandleLoseFocus();
        MMIDC_SendByMMS();
    #else
        MMK_CreateWin((uint32*)MMIDC_SEND_TAB, (ADD_DATA)PNULL);/*lint !e64*/
    #endif    
}
#ifdef ATECH_SOFT
/*****************************************************************************/
// 	Description : open send photo window
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC void MMIDC_OpenSendVideoWin(void)
{
    #ifndef BLUETOOTH_SUPPORT
        MMIDC_HandleLoseFocus();
        MMIDC_SendByMMS();
    #else
        MMK_CreateWin((uint32*)MMIDV_SEND_TAB, (ADD_DATA)PNULL);/*lint !e64*/
    #endif    
}
#endif
/*****************************************************************************/
// 	Description : dc engineer mode
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC void MMIDC_CloseStoreWin(void)
{
	if(MMK_IsOpenWin(MMICAMERA_STORAGE_SPACE_WIN_ID))
	{
		MMK_CloseWin(MMICAMERA_STORAGE_SPACE_WIN_ID);
	}
}

/*****************************************************************************/
// 	Description : post dc state msg  for error
//	Global resource dependence : none
//  Author: robert.wang
//	Note:
/*****************************************************************************/
PUBLIC void MMIDC_PostStateMsg(void)
{
     MMK_PostMsg(MMIDC_MAIN_WIN_ID, MSG_DC_MMI_STATE, (DPARAM)PNULL, 0);
}

/*****************************************************************************/
// 	Description : start hit timer
//	Global resource dependence : none
//  Author: robert.wang
//	Note: camera exit while do nothing in 1 minute 
/*****************************************************************************/
LOCAL void MMIDC_ResetHitTimer(void)
{
    if (0 == s_hit_timer)
    {
        s_hit_timer = MMK_CreateTimer(60*MMI_1SECONDS, TRUE);
    }
    else
    {
        MMK_StopTimer(s_hit_timer);
        s_hit_timer = MMK_CreateTimer(60*MMI_1SECONDS, TRUE);
    }
}

/*****************************************************************************/
// 	Description : close hit timer
//	Global resource dependence : none
//  Author: robert.wang
//	Note:
/*****************************************************************************/
LOCAL void MMIDC_CloseHitTimer(void)
{
    if (0 != s_hit_timer)
    {
        MMK_StopTimer(s_hit_timer);
        s_hit_timer = 0;
    }
}

#endif  //#ifdef  CAMERA_SUPPORT
