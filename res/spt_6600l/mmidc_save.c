#include "std_header.h"

#ifdef  CAMERA_SUPPORT
#include "mmidc_setting.h"
#include "mmidc_save.h"
#include "mmi_filemgr.h"
#include "mmidc_flow.h"
#include "dal_jpeg.h"
#include "mmidc_camera_text.h"
#include "mmidc_display.h"
#include "dal_dcamera.h"
#include "dal_time.h"
#include "mmipub.h"
#include "mmi_text.h"
#include "mmi_image.h"
#include "mmi_common.h"
#include "mmipub.h"
#include "mmimultim_text.h"
#include "mmimms.h"
#include "mmipic.h"
#include "mmisd_export.h"
#include "mmidc_option.h"
#include "mmidc_setting.h"
#include "mmifmm.h"
#include "mmifmm_interface.h"
#include "mmibt_export.h"
#include "mmipicview.h"

#define PIC_NAME_MAX_SUFFIX_LIMIT		            100
#define DC_MULTI_SHOT_240X320_MAX_SIZE	            (50 * 1024)

#define DC_PANORAMA_JPEG_MAX_SIZE                   (200 * 1024)
#define DC_EXIF_THUMBNAIL_WIDTH			            320
#define DC_EXIF_THUMBNAIL_HEIGHT		            240
#define DC_EXIF_THUMBNAIL_YUV_BUFFER_SIZE	        (150 * 1024)
#define DC_EXIF_THUMB_OFFSET			            (100 * 1024)
#define DC_EXIF_HEAD_OFFSET				            (16 * 1024)
#define MMIDC_NAME_MAX_LEN		                    52

#define DV_VIDEOEXT_3GP					            "3gp"
#define DV_VIDEOEXT_MP4					            "mp4"
#define DV_VIDEOEXT_AVI					            "avi"

typedef struct 
{
    wchar           ucs2_name[MMIDC_NAME_MAX_LEN];
    char            ascii_name[MMIDC_NAME_MAX_LEN];	
    FILE_DEV_E_T    dev_file;
}PHOTO_STOREG_INFO_T;

typedef struct 
{
    SFS_HANDLE				sfs_handle;
    wchar					record_file_name_arr[MMIDC_NAME_MAX_LEN];
    wchar		            video_full_path_arr[MMIDC_FULL_PATH_LENGTH];
    PHOTO_STOREG_INFO_T		photo_info[DC_BURST_9_PHOTO];
}MMIDC_SAVE_DATA;
 
static MMIDC_SAVE_DATA*          s_save_data_ptr = PNULL;
#define MMIDC_SAVE_DATA_PTR      (PNULL != s_save_data_ptr ? s_save_data_ptr : MMIDC_AssertMacro())


/*****************************************************************************/
// 	Description : get record video storage device
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL FILE_DEV_E_T GetVideoStoreDev(void);

/*****************************************************************************/
// 	Description : get photos storeage device
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL FILE_DEV_E_T GetPhotoStoreDev(void);

/*****************************************************************************/
// 	Description : 
//	Global resource dependence : none
//  Author: 
//	Note:
/*****************************************************************************/
PUBLIC void MMIDC_NewSaveData(void)
{
    SCI_ASSERT(PNULL == s_save_data_ptr);
    s_save_data_ptr = (MMIDC_SAVE_DATA*)SCI_ALLOC_APP(sizeof(MMIDC_SAVE_DATA));
    SCI_MEMSET(s_save_data_ptr, 0, sizeof(MMIDC_SAVE_DATA));
}

/*****************************************************************************/
// 	Description : fill rect in mtv
//	Global resource dependence : 
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC void MMIDC_DeleteSaveData(void)
{
    SCI_TRACE_LOW("[MMIDC]: MMIDC_DeleteSaveData");
    
    if(PNULL != s_save_data_ptr)
    {
        SCI_Free(s_save_data_ptr);
        s_save_data_ptr = PNULL;
    }
}

//SFOX_SUPPORT
extern void SFOX_CameraNewPhotoDone( wchar *fname, uint16 fnlen, uint32 fsize );
PUBLIC void MMIDC_SendBySFOX(void)
{
    if(MMIDC_GetCameraMode() == CAMERA_MODE_DC)
    {
        wchar		full_path_arr[MMIDC_FULL_PATH_LENGTH] = {0};
        uint16		full_path_len = 0;
        uint32		send_pic_size = 0;

        SCI_ASSERT(PNULL != MMIDC_GetDCInfo());
        send_pic_size = MMIDC_GetDCInfo()->image_len[MMIDC_GetCurrentPhotoID()];

        SCI_ASSERT(PNULL != MMIDC_GetPhotoSavePath());
        full_path_len = MMIDC_CombinePathName(full_path_arr, MMIDC_FULL_PATH_LENGTH, MMIDC_GetPhotoSavePath(),
            MMIDC_SAVE_DATA_PTR->photo_info[MMIDC_GetCurrentPhotoID()].ucs2_name);
		SFOX_CameraNewPhotoDone( full_path_arr, full_path_len, send_pic_size );
    }
}

/*****************************************************************************/
// 	Description : send by mms
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC void MMIDC_SendByMMS(void)
{
    if(MMIDC_GetCameraMode() == CAMERA_MODE_DC)
    {
        uint8		*send_data_ptr = PNULL;
        wchar		full_path_arr[MMIDC_FULL_PATH_LENGTH] = {0};
        uint16		full_path_len = 0;
        uint32		send_pic_size = 0;
        uint32      data_buf_offset = 0;

        SCI_ASSERT(PNULL != MMIDC_GetDCInfo());
        send_pic_size = MMIDC_GetDCInfo()->image_len[MMIDC_GetCurrentPhotoID()];

        SCI_ASSERT(PNULL != MMIDC_GetPhotoSavePath());
        full_path_len = MMIDC_CombinePathName(full_path_arr, MMIDC_FULL_PATH_LENGTH, MMIDC_GetPhotoSavePath(),
            MMIDC_SAVE_DATA_PTR->photo_info[MMIDC_GetCurrentPhotoID()].ucs2_name);

        if (MMIAPIPIC_GetCurIconDataBuffer(&send_data_ptr,full_path_arr,full_path_len,&send_pic_size,&data_buf_offset,MMIMMS_GetMMSMaxSize(),GUIANIM_TYPE_IMAGE_JPG))
        {
            MMIMMS_SendImage(GUIANIM_TYPE_IMAGE_JPG,send_pic_size,(void *)(send_data_ptr+data_buf_offset));
        }
        else if(0 < send_pic_size)
        {
            MMIPUB_OpenAlertWinByTextId(PNULL,TXT_FILESIZE_TOOBIG,TXT_NULL,IMAGE_PUBWIN_WARNING,PNULL,PNULL,MMIPUB_SOFTKEY_ONE,PNULL);
        }
        else
        {
            MMIPUB_OpenAlertWinByTextId(PNULL,TXT_EMPTY_FILE,TXT_NULL,IMAGE_PUBWIN_WARNING,PNULL,PNULL,MMIPUB_SOFTKEY_ONE,PNULL);
        }
        
        //free buffer
        MMIMULTIM_FreeFileDataBuf(send_data_ptr);
    }
    else
    {
        uint16 full_path_len = MMIAPICOM_Wstrlen(MMIDC_SAVE_DATA_PTR->video_full_path_arr);
        uint32 create_time = 0;
        uint32 file_size = 0;
        uint8 *send_data_ptr = PNULL;
        MMICOM_TYPE_E movie_type = MMICOM_TYPE_3GP;

        MMIFILE_GetFileInfo(MMIDC_SAVE_DATA_PTR->video_full_path_arr, full_path_len, &file_size, NULL, NULL);
        if (file_size > MMIMMS_GetMMSMaxSize())
        {
            MMIPUB_OpenAlertWinByTextId(PNULL,TXT_FILESIZE_TOOBIG,TXT_NULL,IMAGE_PUBWIN_WARNING,PNULL,PNULL,MMIPUB_SOFTKEY_ONE,PNULL);
            return;
        } 
        
        movie_type = MMIAPICOM_GetMovieType(MMIDC_SAVE_DATA_PTR->video_full_path_arr, full_path_len);
        SCI_TRACE_LOW("movie_type=%d",movie_type);      
        send_data_ptr = MMIMULTIM_AllocFileDataBuf(file_size, PNULL);
        MMIDC_Assert(NULL != send_data_ptr, __FILE__, __LINE__);
        
        if(MMIFILE_ReadFilesDataSyn(MMIDC_SAVE_DATA_PTR->video_full_path_arr, full_path_len, send_data_ptr, file_size))
        {
            MMIMMS_SendMovie((MMIMPEG4_TYPE_E)movie_type, file_size,(void *)send_data_ptr);
        }
        else
        {
            MMIPUB_OpenAlertWinByTextId(PNULL,TXT_MULTIM_FILESYSTEM_ERROR,TXT_NULL,IMAGE_PUBWIN_WARNING,PNULL,PNULL,MMIPUB_SOFTKEY_ONE,PNULL);
        }	
        MMIMULTIM_FreeFileDataBuf(send_data_ptr);
        send_data_ptr = PNULL;
    }
}

/*****************************************************************************/
// 	Description : send by blue tooth
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC void MMIDC_SendByBt(void)
{
    uint16 full_path_len = 0;
    uint32 file_size = 0;
    wchar full_path_arr_photo[MMIDC_FULL_PATH_LENGTH] = {0};
    wchar* full_path_arr = full_path_arr_photo;
    SFS_DATE_T	date = {0};
    SFS_TIME_T	time = {0};


    if(MMIDC_GetCameraMode() == CAMERA_MODE_DC)
    {        
        SCI_ASSERT(PNULL != MMIDC_GetPhotoSavePath());
        full_path_len = MMIDC_CombinePathName(full_path_arr, MMIDC_FULL_PATH_LENGTH, MMIDC_GetPhotoSavePath(),
            MMIDC_SAVE_DATA_PTR->photo_info[MMIDC_GetCurrentPhotoID()].ucs2_name);
    }
    else
    {
        full_path_arr = MMIDC_SAVE_DATA_PTR->video_full_path_arr;
        full_path_len = MMIAPICOM_Wstrlen(MMIDC_SAVE_DATA_PTR->video_full_path_arr);
    }
    MMIFILE_GetFileInfo(full_path_arr, full_path_len, &file_size, &date, &time);   
#ifdef BLUETOOTH_SUPPORT	
    MMIAPIBT_SendFile(full_path_arr, full_path_len, file_size);
#endif
}


/*****************************************************************************/
// 	Description : set current photo as wallpaper
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC void MMIDC_SetCurrentPhotoAsWallpaper(void)
{
    wchar full_path_arr[MMIDC_FULL_PATH_LENGTH] = {0};
    uint16 full_path_len = 0;

	SCI_ASSERT(PNULL != MMIDC_GetPhotoSavePath());			
    full_path_len = MMIDC_CombinePathName(full_path_arr, MMIDC_FULL_PATH_LENGTH, MMIDC_GetPhotoSavePath(),
        MMIDC_SAVE_DATA_PTR->photo_info[MMIDC_GetCurrentPhotoID()].ucs2_name);
    
    MMIAPIMULTIM_SetPictureToWallpaper(FALSE, full_path_arr, full_path_len);
}

/*****************************************************************************/
// 	Description : get photos storeage device
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL FILE_DEV_E_T GetPhotoStoreDev(void)
{
    FILE_DEV_E_T res = FS_INVALID;

    switch(MMIDC_GetPhotoStorageDevice())
    {
    case DC_DEV_UDISK:
        res = FS_UDISK;
        break;
    case DC_DEV_SD:
        res = FS_MMC;
        break;
    default:
        if (MMIAPISD_GetStatus())
        {
            res = FS_MMC;
        }
        else
        {
            res = FS_UDISK;
        }
        SCI_TRACE_LOW("[MMIDC] GetPhotoStoreDev DEFAULT !!!");
        break;
    }
    return res;
}

/*****************************************************************************/
// 	Description : check room is enough
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL BOOLEAN CheckDevRoomSufficient(FILE_DEV_E_T file_dev, uint32 need_size)
{
    uint32 free_high_word = 0;
    uint32 free_low_word = 0;
    
    SCI_TRACE_LOW("[MMIDC] CheckDevRoomSufficient %d, %d", file_dev, need_size);
    switch(file_dev)
    {
    case FS_UDISK:
        MMIFILE_GetDeviceFreeSpace(MMIFILE_DEVICE_UDISK, MMIFILE_DEVICE_UDISK_LEN, &free_high_word, &free_low_word);    
        break;

    case FS_MMC:
        if(!MMIAPISD_GetStatus())
        {
            SCI_TRACE_LOW("[MMIDC]: CheckDevRoomSufficient No sd card");
            return FALSE;
        }
        MMIFILE_GetDeviceFreeSpace(MMIFILE_DEVICE_SDCARD, MMIFILE_DEVICE_SDCARD_LEN, &free_high_word, &free_low_word);    
        break;

    default:
        SCI_TRACE_LOW("[MMIDC] CheckDevRoomSufficient DEFAULT !!!");
        break;
    }		
    SCI_TRACE_LOW("[MMIDC]: file_dev = 0x%.2x, need_size = %ld, free_high_word = %ld, free_low_word = %ld", file_dev, need_size, free_high_word, free_low_word);
    if((free_low_word <= need_size + 2048) && 0 == free_high_word)
    {
        SCI_TRACE_LOW("[MMIDC] CheckDevRoomSufficient - free_low_word <= need_size + 2048");
        return FALSE;
    }
    return TRUE;
}

/*****************************************************************************/
// 	Description : create capture photo name
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC uint16 MMIDC_GetFileIDString(char* file_name, const char* prefix, uint32 id, uint32 id_max, const char* file_type)
{
    uint16 id_len = 0;
    uint16 id_max_len = 0;
    char string_max[20] = {0};
    uint32 i = 0;
 
    if(id >= id_max)
    {
        id = id_max - 100;
    }

    
    id_len = sprintf(file_name, "%d", id);
    id_max_len = sprintf(string_max, "%d", id_max);
    for(i = 0; i < id_len; i++)
    {
        string_max[id_max_len - id_len + i] = file_name[i];
    }
    string_max[0] = '_';
    string_max[id_max_len] = '.';
    id_len = sprintf(file_name, "%s%s%s", prefix, string_max, file_type);
    return id_len;
}

/*****************************************************************************/
// 	Description : create capture photo name
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL BOOLEAN RealCreateOnePicName(FILE_DEV_E_T file_dev, PHOTO_STOREG_INFO_T* name_info_ptr)
{
    wchar full_path_arr[MMIDC_FULL_PATH_LENGTH] = {0};
    char temp_photo_name[MMIDC_NAME_MAX_LEN] = {0};
    uint16 full_path_len = 0;
    uint16 name_len = 0;
    register uint32 step = 1;
    uint32 circle = 1;
    register uint32 name_id = 0;
    register uint32 id_max = DC_FILE_NAME_ID_MAX;
    wchar* save_path = MMIDC_GetPhotoSavePath();
    BOOLEAN is_first_create = TRUE;
    
    SCI_ASSERT(PNULL != save_path);
    SCI_TRACE_LOW("[MMIDC]: RealCreateOnePicName %d, %d", file_dev, (uint32)name_info_ptr);
    MMIDC_Assert(PNULL != name_info_ptr, __FILE__, __LINE__);
    
    SCI_MEMSET(name_info_ptr->ascii_name, 0, sizeof(name_info_ptr->ascii_name));
    name_id = MMIDC_GetPhotoNameID();
    if(!MMIFILE_IsFolderExist(save_path, MMIAPICOM_Wstrlen(save_path)))
    {
        SCI_MEMSET(save_path, 0, (MMIDC_FULL_PATH_LENGTH * sizeof(wchar)));
        MMIDC_CombinePath(save_path, MMIDC_FULL_PATH_LENGTH, file_dev, MMIMULTIM_DIR_PICTURE);
        if (!MMIFILE_CreateDir((const wchar*)save_path, MMIAPICOM_Wstrlen((const wchar*)save_path)))
        {
            return FALSE;
        }
    }
    
    while(1)
    {
        name_id += step;
        step = step * 2;
        if(name_id >= id_max)
        {
            circle ++;
            if(circle >= id_max)
            {
                circle = 1;
                id_max = id_max * 10;
            }
            name_id = circle;
            step = 1;
        }
        name_len = MMIDC_GetFileIDString(temp_photo_name, "DSC", name_id, id_max, "jpg");
        MMIAPICOM_StrToWstr(temp_photo_name, name_info_ptr->ucs2_name);
        
        full_path_len = MMIDC_CombinePathName(full_path_arr, MMIDC_FULL_PATH_LENGTH, save_path, name_info_ptr->ucs2_name);
        MMIDC_SAVE_DATA_PTR->sfs_handle = MMIFILE_CreateFile(full_path_arr, SFS_MODE_WRITE|SFS_MODE_CREATE_NEW, NULL, NULL);
		if(0 != MMIDC_SAVE_DATA_PTR->sfs_handle)
		{
			name_info_ptr->dev_file = file_dev;
            SCI_MEMCPY(name_info_ptr->ascii_name, temp_photo_name, name_len);        
			break;
		}
        if(is_first_create)
        {
            is_first_create = FALSE;
            if(!MMIFILE_IsFileExist(full_path_arr, full_path_len))
            {
                return FALSE;
            }
        }
    }
    MMIDC_SetPhotoNameID(name_id);
    return TRUE;    
}

/*****************************************************************************/
// 	Description : set photo storage device
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL void SetPhotoStoreDev(FILE_DEV_E_T file_dev)
{
    switch(file_dev)
    {
    case FS_UDISK:
        MMIDC_SetPhotoStorageDevice(DC_DEV_UDISK);
        break;
        
    case FS_MMC:
        if (MMIAPISD_GetStatus())
        {
            MMIDC_SetPhotoStorageDevice(DC_DEV_SD);
        }
        break;
        
    default:
        SCI_TRACE_LOW("[MMIDC] SetPhotoStoreDev DEFAULT !!!");
        break;
    }
}

/*****************************************************************************/
// 	Description : check the udisk and sd card before create capture photo name
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL DC_SAVE_RESULT_E CreateOnePicName(int32 photo_index)
{	
    FILE_DEV_E_T cur_file_dev = GetPhotoStoreDev();			
    DC_SAVE_RESULT_E ret = DC_SAVE_SUCCESS;
    DC_INFO_STRUCT* dc_infor = MMIDC_GetDCInfo();

    SCI_TRACE_LOW("[MMIDC] CreateOnePicName %d", photo_index);
    SCI_ASSERT(PNULL != dc_infor);
    SCI_ASSERT(PNULL != MMIDC_GetPhotoSavePath());
    if(FS_UDISK == cur_file_dev)
    {
        if(!CheckDevRoomSufficient(cur_file_dev, dc_infor->image_len[photo_index]))
        {
            if(CheckDevRoomSufficient(FS_MMC, dc_infor->image_len[photo_index]))
            {
                cur_file_dev = FS_MMC;
                MMIDC_CombinePath(MMIDC_GetPhotoSavePath(), MMIDC_FULL_PATH_LENGTH, FS_MMC, MMIMULTIM_DIR_PICTURE);
            }
            else
            {
                SCI_TRACE_LOW("[MMIDC]: CreateOnePicName not enough space");
                ret = DC_SAVE_ERROR;
            }
        }
    }
    else if(FS_MMC == cur_file_dev)
    {
        if(!CheckDevRoomSufficient(cur_file_dev, dc_infor->image_len[photo_index]))
        {
            if(CheckDevRoomSufficient(FS_UDISK, dc_infor->image_len[photo_index]))
            {
                cur_file_dev = FS_UDISK;
                MMIDC_CombinePath(MMIDC_GetPhotoSavePath(), MMIDC_FULL_PATH_LENGTH, FS_UDISK, MMIMULTIM_DIR_PICTURE);
            }
            else
            {
                SCI_TRACE_LOW("[MMIDC]: CreateOnePicName not enough space");
                ret = DC_SAVE_ERROR;
            }
        }
    }
    if(DC_SAVE_SUCCESS == ret)
    {
        /** create the photo name **/	
        if(RealCreateOnePicName(cur_file_dev, &(MMIDC_SAVE_DATA_PTR->photo_info[photo_index])))
        {
            SCI_TRACE_LOW("[MMIDC]: CreateOnePicName success");
            /** set new file dev **/
            SetPhotoStoreDev(cur_file_dev);
        }
        else
        {
            SCI_TRACE_LOW("[MMIDC]: CreateOnePicName fail");
            ret = DC_SAVE_ERROR;
        }
    }
    return ret;
}

/*****************************************************************************/
// 	Description : write photo buffer to the file
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL DC_SAVE_RESULT_E WriteToFile(uint8 * const photo_ptr, uint32 photo_size)
{
    uint32 write_size = 0;
	DC_SAVE_RESULT_E res = DC_SAVE_NO_ERROR;

	if(SFS_NO_ERROR == MMIFILE_WriteFile(MMIDC_SAVE_DATA_PTR->sfs_handle, photo_ptr, photo_size, &write_size, PNULL))
	{
		SCI_TRACE_LOW("[MMIDC]: WriteToFile success");
		res = DC_SAVE_NO_ERROR;
	}
	else
	{
		SCI_TRACE_LOW("[MMIDC]: WriteToFile failed");
		MMIDC_OpenAlertWin(MMIPUB_SOFTKEY_ONE, TXT_DC_SAVEFAILED, IMAGE_PUBWIN_FAIL, MMI_3SECONDS);
		res = DC_SAVE_ERROR;
	}
	SFS_CloseFile(MMIDC_SAVE_DATA_PTR->sfs_handle);
	MMIDC_SAVE_DATA_PTR->sfs_handle = 0;
	return res;
}

/*****************************************************************************/
// 	Description : save captured photos
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC DC_SAVE_RESULT_E MMIDC_SavePhotos(int32 photo_id)
{
    DC_SAVE_RESULT_E ret = DC_SAVE_SUCCESS;
    
    SCI_TRACE_LOW("[MMIDC]: SavePhoto +");
    if(NO_SAVE_PHOTO == photo_id)
    {
        return DC_SAVE_ERROR;
    }
    if(DC_SAVE_SUCCESS != CreateOnePicName(photo_id))
    {
        MMIDC_OpenAlertWin(MMIPUB_SOFTKEY_ONE, TXT_NO_SPACE, IMAGE_PUBWIN_FAIL, MMI_2SECONDS);
        ret = DC_SAVE_ERROR;
    }
    else
    {
        SCI_ASSERT(PNULL != MMIDC_GetDCInfo());
        ret = WriteToFile(MMIDC_GetDCInfo()->image_addr[photo_id], MMIDC_GetDCInfo()->image_len[photo_id]);
    }
    if(0 != MMIDC_SAVE_DATA_PTR->sfs_handle)
    {
        SFS_CloseFile(MMIDC_SAVE_DATA_PTR->sfs_handle);
        MMIDC_SAVE_DATA_PTR->sfs_handle = 0;
    }
    return ret;
}

/*****************************************************************************/
// 	Description : get record video storage device
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL FILE_DEV_E_T GetVideoStoreDev(void)
{
    FILE_DEV_E_T res = FS_INVALID;
    SCI_TRACE_LOW("[MMIDC]: MMIDC_GetVideoStoreDev +"); 
    switch(MMIDC_GetVideoStorageDevice())
    {
    case DC_DEV_UDISK:
        res = FS_UDISK;
        break;
    case DC_DEV_SD:
        res = FS_MMC;
        break;
    default:
        if (MMIAPISD_GetStatus())
        {
            res = FS_MMC;
        }
        else
        {
            res = FS_UDISK;
        }
        SCI_TRACE_LOW("[MMIDC] GetVideoStoreDev DEFAULT !!!");
        break;
    }
    SCI_TRACE_LOW("[MMIDC]: MMIDC_GetVideoStoreDev - %d", res); 
    return res;
}

/*****************************************************************************/
// 	Description : get record video subfix
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL char* GetVideoSubfix(void)
{	
    char* ret = PNULL;
    
    switch(MMIDC_GetRecordFileType())
    {
    case VIDEO_FILE_TYPE_MP4:
        SCI_TRACE_LOW("[MMIDC]: GetVideoSubfix DV_VIDEOEXT_MP4 -"); 
        ret = (char*)DV_VIDEOEXT_MP4;
        break;

    case VIDEO_FILE_TYPE_3GP:
        SCI_TRACE_LOW("[MMIDC]: GetVideoSubfix DV_VIDEOEXT_3GP -"); 
        ret = (char*)DV_VIDEOEXT_3GP;
        break;
        
    case VIDEO_FILE_TYPE_AVI:
        SCI_TRACE_LOW("[MMIDC]: GetVideoSubfix VIDEO_FILE_TYPE_MJPEG -"); 
        ret = (char*)DV_VIDEOEXT_AVI;
        break;
        
    default:
        SCI_TRACE_LOW("[MMIDC]: GetVideoSubfix DEFAULT !!!"); 
        ret = (char*)DV_VIDEOEXT_3GP;
        break;
    }	
    return ret;
}

/*****************************************************************************/
// 	Description : create record video name
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
LOCAL BOOLEAN CreateVideoName(wchar* ucs2_name_ptr, uint16 ucs2_size)
{	
    uint16 full_path_len = 0;
    uint32 udisk_free_high_word = 0;
    uint32 udisk_free_low_word = 0;
    uint32 sd_free_high_word = 0;
    uint32 sd_free_low_word = 0;
    register uint32 step = 1;
    uint32 circle = 1;
    register uint32 name_id = 0;
    register uint32 id_max = DC_FILE_NAME_ID_MAX;
    char temp_video_name[MMIDC_NAME_MAX_LEN] = {0};
    char* subfix = GetVideoSubfix();
    wchar* save_path = MMIDC_GetVideoSavePath();
    FILE_DEV_E_T file_dev = FS_MMC;
    
    SCI_ASSERT(PNULL != subfix);
    SCI_ASSERT(PNULL != save_path);
    SCI_TRACE_LOW("[MMIDC]: CreateVideoName");
    MMIDC_Assert(PNULL != ucs2_name_ptr, __FILE__, __LINE__);
    MMIDC_Assert(ucs2_size >= MMIDC_NAME_MAX_LEN, __FILE__, __LINE__);

    //add by @dream.chen for udisk
#if defined (MMI_UDISK_MEM_ENABLE) && !defined (MMI_DV_UDISK_ENABLE)
    if ((MMIDC_GetVideoStorageDevice() == DC_DEV_SD) 
        &&( !MMIFILE_GetDeviceStatus((uint16 *)MMIFILE_DEVICE_SDCARD, MMIFILE_DEVICE_SDCARD_LEN)))
    {
        return FALSE;
    }
#endif
    MMIFILE_GetDeviceFreeSpace(MMIFILE_DEVICE_UDISK, MMIFILE_DEVICE_UDISK_LEN, &udisk_free_high_word, &udisk_free_low_word);
    MMIFILE_GetDeviceFreeSpace(MMIFILE_DEVICE_SDCARD, MMIFILE_DEVICE_SDCARD_LEN, &sd_free_high_word, &sd_free_low_word);
    if((udisk_free_low_word <= 102400) && (0 == udisk_free_high_word) && MMIDC_GetVideoStorageDevice() == DC_DEV_UDISK)
    {
        if(sd_free_low_word <= 102400)
        {
            return FALSE;
        }
        else
        {
            MMIDC_SetVideoStorageDevice(DC_DEV_SD);
            MMIDC_CombinePath(save_path, MMIDC_FULL_PATH_LENGTH, GetVideoStoreDev(), MMIMULTIM_DIR_MOVIE);
        }
    }
    if((sd_free_low_word <= 102400) && (0 == sd_free_high_word) && MMIDC_GetVideoStorageDevice() == DC_DEV_SD)
    {
        if(udisk_free_low_word <= 102400)
        {
            return FALSE;
        }
    //add by @dream.chen for udisk
#if defined (MMI_UDISK_MEM_ENABLE) && !defined (MMI_DV_UDISK_ENABLE)
        else
        {
            return FALSE;
        }
#else
        else
        {
            MMIDC_SetVideoStorageDevice(DC_DEV_UDISK);
            MMIDC_CombinePath(save_path, MMIDC_FULL_PATH_LENGTH, GetVideoStoreDev(), MMIMULTIM_DIR_MOVIE);
        }
#endif
    }
    
    name_id = MMIDC_GetVideoNameID();
    if(!MMIFILE_IsFolderExist(save_path, MMIAPICOM_Wstrlen(save_path)))
    {
        SCI_MEMSET(save_path, 0, (MMIDC_FULL_PATH_LENGTH * sizeof(wchar)));
        if(MMIDC_GetVideoStorageDevice() == DC_DEV_UDISK)
        {
            file_dev = FS_UDISK;
        }
        else
        {
            file_dev = FS_MMC;
        }
        MMIDC_CombinePath(save_path, MMIDC_FULL_PATH_LENGTH, file_dev, MMIMULTIM_DIR_MOVIE);
        if (!MMIFILE_CreateDir((const wchar*)save_path, MMIAPICOM_Wstrlen((const wchar*)save_path)))
        {
            return FALSE;
        }
    }
    while(1)
    {		
        name_id += step;
        step = step * 2;
        if(name_id >= id_max)
        {
            circle ++;
            if(circle >= id_max)
            {
                circle = 1;
                id_max = id_max * 10;
            }
            name_id = circle;
            step = 1;
        }
        MMIDC_GetFileIDString(temp_video_name, "MOV", name_id, id_max, subfix);
        MMIAPICOM_StrToWstr(temp_video_name, ucs2_name_ptr);
        
        full_path_len = MMIDC_CombinePathName(MMIDC_SAVE_DATA_PTR->video_full_path_arr, MMIDC_FULL_PATH_LENGTH, save_path, ucs2_name_ptr);
        
        // check the exist of the file name.
        if(!MMIFILE_IsFileExist(MMIDC_SAVE_DATA_PTR->video_full_path_arr, full_path_len))
        {
            break;
        }
    }
    MMIDC_SetVideoNameID(name_id);
    return TRUE;	
}

/*****************************************************************************/
// 	Description : review video
//	Global resource dependence :
//  Author: gang.tong
//	Note:
/*****************************************************************************/
PUBLIC void MMIDC_ReviewVideo(void)
{    
    if (MMIAPICOM_Wstrlen(MMIDC_SAVE_DATA_PTR->video_full_path_arr) > 0)
    {
        MMIAPIMPEG4_ReviewFromPathName(GetVideoStoreDev(), 1, MMIDC_SAVE_DATA_PTR->video_full_path_arr, MMIAPICOM_Wstrlen(MMIDC_SAVE_DATA_PTR->video_full_path_arr));	
    }
}

/*****************************************************************************/
// 	Description : create recording video name
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC BOOLEAN MMIDC_CreateVideoName(DRECORDER_START_RECORD_T* recorder_start_t)
{
    SCI_TRACE_LOW("[MMIDC]: MMIDC_CreateVideoName +");
    
    SCI_MEMSET(MMIDC_SAVE_DATA_PTR->record_file_name_arr, 0x00, sizeof(MMIDC_SAVE_DATA_PTR->record_file_name_arr));
    SCI_MEMSET(MMIDC_SAVE_DATA_PTR->video_full_path_arr, 0x00, sizeof(MMIDC_SAVE_DATA_PTR->video_full_path_arr));
    
    if(!CreateVideoName(MMIDC_SAVE_DATA_PTR->record_file_name_arr, MMIDC_NAME_MAX_LEN))
    {
        SCI_TRACE_LOW("[MMIDC]: MMIDC_CreateVideoName - create dv name fail!");
        MMIDC_OpenAlertWin(MMIPUB_SOFTKEY_ONE, TXT_NO_SPACE, IMAGE_PUBWIN_FAIL, MMI_2SECONDS);
        return FALSE;
    }    
    recorder_start_t->file_name_ptr		 = (uint8*)MMIDC_SAVE_DATA_PTR->video_full_path_arr;
    recorder_start_t->file_name_byte_len = MMIAPICOM_Wstrlen(MMIDC_SAVE_DATA_PTR->video_full_path_arr) * 2;
    SCI_TRACE_LOW("[MMIDC]: MMIDC_CreateVideoName -");
    return TRUE;
}

/*****************************************************************************/
// 	Description : delete the recorded file
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC BOOLEAN MMIDC_DeleteNoSavedFile(void)
{	
    SCI_TRACE_LOW("[MMIDC]: MMIDC_DeleteNoSavedFile +-"); 

    if (MMIAPICOM_Wstrlen(MMIDC_SAVE_DATA_PTR->video_full_path_arr) > 0)
    {
        return MMIFILE_DeleteFileSyn(MMIDC_SAVE_DATA_PTR->video_full_path_arr, (uint16)MMIAPICOM_Wstrlen(MMIDC_SAVE_DATA_PTR->video_full_path_arr));
    }
    
    return TRUE;
}

/*****************************************************************************/
// 	Description : delete current photo
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC void MMIDC_DeletePhoto(void)
{
    uint16		full_path_arr[MMIDC_FULL_PATH_LENGTH] = {0};
    uint16		full_path_len = 0;
    
    SCI_ASSERT(PNULL != MMIDC_GetPhotoSavePath());
    full_path_len = MMIDC_CombinePathName(full_path_arr, MMIDC_FULL_PATH_LENGTH, MMIDC_GetPhotoSavePath(),
        MMIDC_SAVE_DATA_PTR->photo_info[MMIDC_GetCurrentPhotoID()].ucs2_name);

    if (full_path_len > 0)
    {
        if(MMIFILE_DeleteFileSyn(full_path_arr, full_path_len))
        {
            /** check whether this is wall paper **/
            if(MMIAPIMULTIM_IsWallPaperFile(full_path_arr, full_path_len)) //È«Â·¾¶Ãû
            {
                MMIAPIMULTIM_InitWallPaper(); 
            }		
            MMIDC_OpenAlertWin(MMIPUB_SOFTKEY_ONE, TXT_DELETED, IMAGE_PUBWIN_SUCCESS, MMI_2SECONDS);
        } 
    }
}

/*****************************************************************************/
// 	Description : get current photo name
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC char* MMIDC_GetPhotoName(uint32 id)
{
    return MMIDC_SAVE_DATA_PTR->photo_info[id].ascii_name;
}

/*****************************************************************************/
// 	Description : get current video name
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC wchar* MMIDC_GetVideoName(void)
{
    return MMIDC_SAVE_DATA_PTR->record_file_name_arr;
}

/*=====================unused function delete=====================*/
/*****************************************************************************/
// 	Description : is permit delete current picture or video
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
/*PUBLIC BOOLEAN MMIAPIDC_IsPermitDeleteFile(const wchar* file_name)
{
    BOOLEAN ret = TRUE;
    if(MMIDC_GetCurrentMode() == DC_SAVE_MODE)
    {
        uint8 photo_number = MMIDC_GetMulitShotNumber();
        uint8 i = 0;

        for(i = 0; i < photo_number; i ++)
        {
            ret = ret && (0 != MMIAPICOM_CompareTwoWstr(file_name, (uint16)MMIAPICOM_Wstrlen(file_name),
                MMIDC_SAVE_DATA_PTR->photo_info[i].ucs2_name, (uint16)MMIAPICOM_Wstrlen(MMIDC_SAVE_DATA_PTR->photo_info[i].ucs2_name)));
        }
    }
    else if(MMIDC_GetCurrentMode() == DV_REVIEW_MODE)
    {
        ret = (0 != MMIAPICOM_CompareTwoWstr(file_name, (uint16)MMIAPICOM_Wstrlen(file_name),
            MMIDC_SAVE_DATA_PTR->record_file_name_arr, (uint16)MMIAPICOM_Wstrlen(MMIDC_SAVE_DATA_PTR->record_file_name_arr)));
    }
    return ret;
}*/

/*****************************************************************************/
//  Description : combine path according to root, dir, name
//  Global resource dependence :                                
//  Author: ryan.xu
//  Note:
/*****************************************************************************/
PUBLIC uint16 MMIDC_CombinePath(wchar* path_ptr, uint16 path_len, FILE_DEV_E_T file_dev, const wchar* dir_ptr)
{
    wchar dev[] = {'D', ':', '\\', 0};

    if(FS_MMC == file_dev)
    {
        dev[0] = 'E';
    }
    SCI_ASSERT(PNULL != path_ptr);
    SCI_MEMSET(path_ptr, 0, (path_len * sizeof(wchar)));
    MMIAPICOM_Wstrcpy(path_ptr, dev);
    if(PNULL == dir_ptr)
    {
        *(path_ptr + 2) = 0;
    }
    else
    {
        MMIAPICOM_Wstrcpy(path_ptr + MMIAPICOM_Wstrlen(dev), dir_ptr);
    }
    return (uint16)MMIAPICOM_Wstrlen(path_ptr);
}

/*****************************************************************************/
//  Description : combine path according to root, dir, name
//  Global resource dependence :                                
//  Author: ryan.xu
//  Note:
/*****************************************************************************/
PUBLIC uint16 MMIDC_CombinePathName(wchar* path_name_ptr, uint16 path_name_len, const wchar* file_path,
                                    const wchar* file_name)
{
    uint16 file_path_len = 0;

    SCI_ASSERT(PNULL != path_name_ptr);
    SCI_MEMSET(path_name_ptr, 0, (path_name_len * sizeof(wchar)));
    MMIAPICOM_Wstrcpy(path_name_ptr, file_path);
    file_path_len = MMIAPICOM_Wstrlen(file_path);
    *(path_name_ptr + file_path_len) = '\\';
    MMIAPICOM_Wstrcpy(path_name_ptr + file_path_len + 1, file_name);
    return (uint16)MMIAPICOM_Wstrlen(path_name_ptr);
}

/*****************************************************************************/
// 	Description : is permit delete current picture or video
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC void MMIDC_OpenPictureWin(void)
{
    SCI_ASSERT(PNULL != MMIDC_GetPhotoSavePath());
    
    MMIAPIDC_SetChangeFolderState(FALSE); //added, @robert.wang, 09-9-8, cr151682
//    MMIAPIFMM_OpenExplorerExt(MMIDC_GetPhotoSavePath(), 
//        (uint16)MMIAPICOM_Wstrlen(MMIDC_GetPhotoSavePath()), MMIDC_SAVE_DATA_PTR->photo_info[MMIDC_GetCurrentPhotoID()].ucs2_name, 
//        (uint16)MMIAPICOM_Wstrlen(MMIDC_SAVE_DATA_PTR->photo_info[MMIDC_GetCurrentPhotoID()].ucs2_name), TRUE, MMIFMM_PICTURE_JPG);
    MMIAPIPICVIEW_OpenPicViewer();
}

/*****************************************************************************/
// 	Description : is permit delete current picture or video
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC void MMIDC_OpenMovieWin(void)
{
    SCI_ASSERT(PNULL != MMIDC_GetVideoSavePath());
    SCI_ASSERT(PNULL != MMIDC_GetVideoName());
    MMIAPIFMM_OpenExplorerExt(MMIDC_GetVideoSavePath(), 
        (uint16)MMIAPICOM_Wstrlen(MMIDC_GetVideoSavePath()), MMIDC_GetVideoName(),
        (uint16)MMIAPICOM_Wstrlen(MMIDC_GetVideoName()), TRUE, MMIFMM_MOVIE_MP4 | MMIFMM_MOVIE_AVI | MMIFMM_MOVIE_3GP);
}

/*****************************************************************************/
// 	Description : is permit delete current picture or video
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC const wchar* MMIDC_GetPicName(void)
{
    return MMIDC_SAVE_DATA_PTR->photo_info[MMIDC_GetCurrentPhotoID()].ucs2_name;
}

/*****************************************************************************/
// 	Description : get current save photo index
//	Global resource dependence : none
//  Author: ryan.xu
//	Note:
/*****************************************************************************/
PUBLIC uint32 MMIDC_GetSavePhotoIndex(
                                      uint32        pic_num,    //in
                                      FILEARRAY     pic_array   //in
                                      )
{
    uint16              file_name_len = 0;
    uint32              photo_index = 0;
    const wchar*        pic_name = MMIDC_GetPicName();
    wchar               file_name[MMIFILE_FILE_NAME_MAX_LEN + 1] = {0};
    FILEARRAY_DATA_T    file_info = {0};

    SCI_ASSERT(PNULL != pic_array);
        
    for (photo_index = 0; photo_index < pic_num; photo_index ++)
    {
        if(!MMIFILEARRAY_Read(pic_array,photo_index, &file_info))
		{
			break;
		}
        SCI_MEMSET(file_name, 0, ((MMIFILE_FILE_NAME_MAX_LEN+1)*sizeof(wchar)));
        MMIFILE_SplitFullPath(file_info.filename,
            file_info.name_len,
            PNULL,
            PNULL,
            PNULL,
            PNULL,
            file_name,
            &file_name_len);
        if(0 == MMIAPICOM_CompareTwoWstr(file_name, file_name_len, pic_name, file_name_len))
        {
            break;
        }
    }

    if(photo_index == pic_num)
    {
        photo_index = 0;
    }

    return (photo_index);
}

/*****************************************************************************/
// 	Description : get  video file size
//	Global resource dependence : none
//  Author: robert.wang
//  Param:
//       Return: file size
/*****************************************************************************/
PUBLIC uint32 MMIDC_GetVideoFileSize(void)
{
    uint32  file_size = 0;
    SFS_DATE_T	date = {0};
    SFS_TIME_T	time = {0};
							
    wchar *file_name = MMIDC_SAVE_DATA_PTR->video_full_path_arr;
    uint16 file_name_len = MMIAPICOM_Wstrlen(file_name);
    
    if (MMIFILE_IsFileExist(file_name, file_name_len))
    {
        MMIFILE_GetFileInfo(file_name, file_name_len, &file_size, &date, &time);
    }
    
    SCI_TRACE_LOW("[MMIDC] MMIDC_GetVideoFileSize file_size = %d, file_name_len = %d",\
            file_size, file_name_len);
    
    return file_size;
}


#endif  //#ifdef  CAMERA_SUPPORT

