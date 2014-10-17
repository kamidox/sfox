/*****************************************************************************
** File Name:      guiref_img.c                                              *
** Author:                                                                   *
** Date:           02/2007                                                   *
** Copyright:      2003 Spreadtrum, Incorporated. All Rights Reserved.       *
** Description:    This file is used to describe phone module                *
*****************************************************************************
**                         Important Edit History                            *
** --------------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                               *
** 02/2007       Jassmine           Create
******************************************************************************/

#ifndef _GUIREF_IMG_C_
#define _GUIREF_IMG_C_

/**--------------------------------------------------------------------------*
 **                         Include Files                                    *
 **--------------------------------------------------------------------------*/
#include "sci_types.h"
#include "sfs.h"
#include "os_api.h"
#include "image_proc.h"
#include "guianim.h"
#include "guiref_img.h"
#include "jpeg_interface.h"

/**--------------------------------------------------------------------------*
 **                         MACRO DEFINITION                                 *
 **--------------------------------------------------------------------------*/

/**--------------------------------------------------------------------------*
 **                         STATIC DEFINITION                                *
 **--------------------------------------------------------------------------*/
LOCAL BOOLEAN                   s_guiref_img_is_operate_file = FALSE;   //是否正在操作文件系统
LOCAL SFS_HANDLE                s_sfs_handle = 0;
LOCAL GUIREF_IMG_BLOCK_T        s_guiref_img_read_block = {0};
LOCAL GUIREF_IMG_BLOCK_T        s_guiref_img_dec_block = {0};
LOCAL GUIREF_IMG_BLOCK_T        s_guiref_img_save_block = {0};

LOCAL GUIREF_IMG_READ_DATA_T    s_guiref_read_data_info = {0};

/*---------------------------------------------------------------------------*/
/*                          TYPE AND CONSTANT                                */
/*---------------------------------------------------------------------------*/

/**--------------------------------------------------------------------------*
 **                         EXTERNAL DECLARE                                 *
 **--------------------------------------------------------------------------*/
// add by hyg 20100919
#if defined(TENCENT_APP_QQ) || defined(TENCENT_APP_BROWSER)
extern void QVM_NotifyGifDecodeOneFrame(IMGPROC_GIF_DISPLAY_INFO_T   *display_info_ptr);
extern BOOLEAN QVM_IsContinueDecodeGif(void);
#endif
// end of hyg 20100919
/**--------------------------------------------------------------------------*
 **                         GLOBAL DEFINITION                                *
 **--------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                          LOCAL FUNCTION DECLARE                           */
/*---------------------------------------------------------------------------*/
/*****************************************************************************/
//  Description : decode jpg file
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL BOOLEAN DecodeJpgFile(
                            uint8       *data_buffer_ptr,   //in/out:decode后图片数据buffer
                            uint32      data_buffer_size,   //in:decode后图片数据buffer的大小
                            uint32      dest_width,         //in:图片目标宽度
                            uint32      dest_height,        //in:图片目标高度
                            uint32      *actual_width_ptr,  //in/out:图片实际宽度
                            uint32      *actual_height_ptr, //in/out:图片实际高度
                            GUI_RECT_T  *crop_rect_ptr      //in:may PNULL
                            );

/*****************************************************************************/
//  Description : read jpg file data syn
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL BOOLEAN ReadJpgFileDataSyn(
                                 uint8      *data_buffer_ptr,   //in
                                 uint32     file_offset,        //in
                                 uint32     to_read_size,       //in
                                 uint32     *read_size_ptr      //in/out
                                 );

/*****************************************************************************/
//  Description : decode jpg data buffer
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL BOOLEAN DecodeJpgBuf(
                           uint8                *data_buffer_ptr,   //in/out:decode后图片数据buffer
                           uint32               data_buffer_size,   //in:decode后图片数据buffer的大小
                           uint32               dest_width,         //in:图片目标宽度
                           uint32               dest_height,        //in:图片目标高度
                           uint32               *actual_width_ptr,  //in/out:图片实际宽度
                           uint32               *actual_height_ptr, //in/out:图片实际高度
                           GUI_RECT_T           *crop_rect_ptr,     //in:may PNULL
                           GUIREF_IMG_DATA_T    *src_data_ptr       //in:传入的图片数据,may PNULL
                           );

/*****************************************************************************/
//  Description : encode jpg file
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL BOOLEAN EncodeJpgFile(
                            uint8       *data_buffer_ptr,       //in/out:decode后图片数据buffer
                            uint32      *data_buffer_size_ptr,  //in/out:decode后图片数据buffer的大小
                            uint32      dest_width,             //in:图片目标宽度
                            uint32      dest_height,            //in:图片目标高度
                            uint32      *actual_width_ptr,      //in/out:图片实际宽度
                            uint32      *actual_height_ptr      //in/out:图片实际高度
                            );

/*****************************************************************************/
//  Description : encode data buffer
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL BOOLEAN EncodeJpgBuf(
                           uint8                *data_buffer_ptr,       //in/out:decode后图片数据buffer
                           uint32               *data_buffer_size_ptr,  //in/out:decode后图片数据buffer的大小
                           uint32               dest_width,             //in:图片目标宽度
                           uint32               dest_height,            //in:图片目标高度
                           uint32               *actual_width_ptr,      //in/out:图片实际宽度
                           uint32               *actual_height_ptr,     //in/out:图片实际高度
                           GUIREF_IMG_DATA_T    *src_data_ptr           //in:传入的图片数据
                           );

/*****************************************************************************/
//  Description : decode bmp or wbmp file
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL BOOLEAN DecodeBmpWbmpFile(
                                BOOLEAN     is_bmp,             //in:TRUE:BMP;FALSE:WBMP
                                uint8       *data_buffer_ptr,   //in/out:decode后图片数据buffer
                                uint32      data_buffer_size,   //in:decode后图片数据buffer的大小
                                uint32      dest_width,         //in:图片目标宽度
                                uint32      dest_height,        //in:图片目标高度
                                uint32      *actual_width_ptr,  //in/out:图片实际宽度
                                uint32      *actual_height_ptr, //in/out:图片实际高度
                                GUI_RECT_T  *crop_rect_ptr      //in:may PNULL
                                );

/*****************************************************************************/
//  Description : decode bmp or wbmp data buffer
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL BOOLEAN DecodeBmpWbmpBuf(
                               BOOLEAN              is_bmp,             //in:TRUE:BMP;FALSE:WBMP
                               uint8                *data_buffer_ptr,   //in/out:decode后图片数据buffer
                               uint32               data_buffer_size,   //in:decode后图片数据buffer的大小
                               uint32               dest_width,         //in:图片目标宽度
                               uint32               dest_height,        //in:图片目标高度
                               uint32               *actual_width_ptr,  //in/out:图片实际宽度
                               uint32               *actual_height_ptr, //in/out:图片实际高度
                               GUI_RECT_T           *crop_rect_ptr,     //in:may PNULL
                               GUIREF_IMG_DATA_T    *src_data_ptr       //in:传入的图片数据,may PNULL
                               );

/*****************************************************************************/
//  Description : get image file size
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL SFS_ERROR_E GetImgFileSize(
                                 SFS_HANDLE    file_handle,
                                 uint32        *file_size_ptr
                                 );

/*****************************************************************************/
//  Description : read file data syn
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL void ReadFileDataSyn(
                           uint8    *data_buffer_ptr,   //in
                           uint32   to_read_size,       //in
                           uint32   *read_size_ptr      //in/out
                           );

/*****************************************************************************/
//  Description : close file
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL void CloseFile(void);

/*****************************************************************************/
//  Description : read image data buffer
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL void ReadDataBuffer(
                          uint8    *data_buffer_ptr,   //in
                          uint32   to_read_size,       //in
                          uint32   *read_size_ptr      //in/out
                          );

/*****************************************************************************/
//  Description : decode png file
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL BOOLEAN DecodePngFile(
                            uint8       *data_buffer_ptr,   //in/out:decode后图片数据buffer
                            uint32      data_buffer_size,   //in:decode后图片数据buffer的大小
                            uint32      dest_width,         //in:图片目标宽度
                            uint32      dest_height,        //in:图片目标高度
                            uint32      *actual_width_ptr,  //in/out:图片实际宽度
                            uint32      *actual_height_ptr, //in/out:图片实际高度
                            GUI_RECT_T  *crop_rect_ptr      //in:may PNULL
                            );

/*****************************************************************************/
//  Description : decode png data buffer
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL BOOLEAN DecodePngBuf(
                           uint8                *data_buffer_ptr,   //in/out:decode后图片数据buffer
                           uint32               data_buffer_size,   //in:decode后图片数据buffer的大小
                           uint32               dest_width,         //in:图片目标宽度
                           uint32               dest_height,        //in:图片目标高度
                           uint32               *actual_width_ptr,  //in/out:图片实际宽度
                           uint32               *actual_height_ptr, //in/out:图片实际高度
                           GUI_RECT_T           *crop_rect_ptr,     //in:may PNULL
                           GUIREF_IMG_DATA_T    *src_data_ptr       //in:传入的图片数据
                           );

/*****************************************************************************/
//  Description : decode gif image file
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL BOOLEAN DecodeGifFile(
                            uint8       *data_buffer_ptr,   //in/out:图片数据buffer
                            uint16      *total_frame_ptr,   //in/out:
                            uint32      data_buffer_size,   //in:decode后图片数据buffer的大小
                            uint32      dest_width,         //in:图片目标宽度
                            uint32      dest_height         //in:图片目标高度
                            );

/*****************************************************************************/
//  Description : decode gif image by buffer
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL BOOLEAN DecodeGifBuf(
                           uint8               *data_buffer_ptr,   //in/out:图片数据buffer
                           uint16              *total_frame_ptr,   //in/out:
                           uint32              data_buffer_size,    //in:decode后图片数据buffer的大小
                           uint32              dest_width,         //in:图片目标宽度
                           uint32              dest_height,        //in:图片目标高度
                           GUIREF_IMG_DATA_T   *src_data_ptr       //in:传入的图片数据
                           );

/*****************************************************************************/
//  Description : decode gif image
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL void NotifyGifDecodeOneFrame(
                                   IMGPROC_GIF_DISPLAY_INFO_T   *display_info_ptr
                                   );

/**--------------------------------------------------------------------------*
 **                         FUNCTION DEFINITION                              *
 **--------------------------------------------------------------------------*/
/*****************************************************************************/
//  Description : decode jpg image
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
PUBLIC BOOLEAN GUIREF_DecodeJpg(
                                uint8               *data_buffer_ptr,   //in/out:decode后图片数据buffer
                                uint16              *full_path_ptr,     //in:文件名,may PNULL
                                uint32              data_buffer_size,   //in:decode后图片数据buffer的大小
                                uint32              dest_width,         //in:图片目标宽度
                                uint32              dest_height,        //in:图片目标高度
                                uint32              *actual_width_ptr,  //in/out:图片实际宽度
                                uint32              *actual_height_ptr, //in/out:图片实际高度
                                GUI_RECT_T          *crop_rect_ptr,     //in:may PNULL
                                GUIREF_IMG_DATA_T   *src_data_ptr       //in:传入的图片数据,may PNULL
                                )
{
    BOOLEAN             result = FALSE;
    BOOLEAN             is_file = FALSE;

    SCI_ASSERT(PNULL != data_buffer_ptr);
    SCI_ASSERT((PNULL != src_data_ptr) || (PNULL != full_path_ptr));
    SCI_ASSERT(PNULL != actual_width_ptr);
    SCI_ASSERT(PNULL != actual_height_ptr);

    //judge is file or image data buffer
    if (PNULL != src_data_ptr)
    {
        is_file = FALSE;
    }
    else if (PNULL != full_path_ptr)
    {
        is_file = TRUE;
    }
    else
    {
        SCI_PASSERT(FALSE,("GUIREF_DecodeJpg:src data %d,full path %d are error!",src_data_ptr,full_path_ptr));
    }
    
    if (is_file)
    {
        //decode jpg file image
        result = DecodeJpgFile(data_buffer_ptr,
                        data_buffer_size,
                        dest_width,
                        dest_height,
                        actual_width_ptr,
                        actual_height_ptr,
                        crop_rect_ptr);
    }
    else
    {
        //decode jpg image buffer
        result = DecodeJpgBuf(data_buffer_ptr,
                        data_buffer_size,
                        dest_width,
                        dest_height,
                        actual_width_ptr,
                        actual_height_ptr,
                        crop_rect_ptr,
                        src_data_ptr);
    }

    return (result);
}

/*****************************************************************************/
//  Description : decode jpg file
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL BOOLEAN DecodeJpgFile(
                            uint8       *data_buffer_ptr,   //in/out:decode后图片数据buffer
                            uint32      data_buffer_size,   //in:decode后图片数据buffer的大小
                            uint32      dest_width,         //in:图片目标宽度
                            uint32      dest_height,        //in:图片目标高度
                            uint32      *actual_width_ptr,  //in/out:图片实际宽度
                            uint32      *actual_height_ptr, //in/out:图片实际高度
                            GUI_RECT_T  *crop_rect_ptr      //in:may PNULL
                            )
{
    BOOLEAN                 result = FALSE;
    uint32                  file_size = 0;
    JINF_RET_E              jpg_result = JINF_SUCCESS;
    JINF_DEC_IN_PARAM_T     jpg_in = {0};
    JINF_DEC_OUT_PARAM_T    jpg_out = {0};

    SCI_ASSERT(PNULL != data_buffer_ptr);
    SCI_ASSERT(PNULL != actual_width_ptr);
    SCI_ASSERT(PNULL != actual_height_ptr);
    
    //open file in mmi
    if (0 != s_sfs_handle)
    {
        //get file size
        if(SFS_ERROR_NONE == GetImgFileSize(s_sfs_handle, &file_size))
        {
            //set crop rect
            if ((PNULL != crop_rect_ptr) &&
                (crop_rect_ptr->right > crop_rect_ptr->left) &&
                (crop_rect_ptr->bottom > crop_rect_ptr->top))
            {
                jpg_in.crop_rect.x = crop_rect_ptr->left;
                jpg_in.crop_rect.y = crop_rect_ptr->top;
                jpg_in.crop_rect.w = crop_rect_ptr->right - crop_rect_ptr->left + 1;
                jpg_in.crop_rect.h = crop_rect_ptr->bottom - crop_rect_ptr->top + 1;
            }

            //set jpg dest jpg info
            jpg_in.target_width = dest_width;
            jpg_in.target_height = dest_height;
            jpg_in.target_data_type = JINF_DATA_TYPE_RGB;
            jpg_in.target_rgb_format = JINF_FORMAT_RGB565;

            //set decode buffer
            jpg_in.decode_buf_ptr = s_guiref_img_dec_block.block_ptr;
            jpg_in.decode_buf_size = s_guiref_img_dec_block.block_size;

            //set out buffer
            jpg_in.target_buf_ptr = data_buffer_ptr;
            jpg_in.target_buf_size = data_buffer_size;
            
            //read file
            jpg_in.read_file_func = ReadJpgFileDataSyn;

            //decode jpg
            jpg_result = IMGJPEG_Decode(&jpg_in,&jpg_out);
            SCI_TRACE_LOW("DecodeJpgFile:jpg decode result is %d!",jpg_result);
            switch (jpg_result)
            {
            case JINF_SUCCESS:
                *actual_width_ptr = jpg_out.output_width;
                *actual_height_ptr = jpg_out.output_height;
                
                result = TRUE;
                break;

            default:
                result = FALSE;
                break;
            }
        }
        else
        {
            result = FALSE;
            SCI_TRACE_LOW("DecodeJpgFile:get file size error!");
        }
    }
    else
    {
        result = FALSE;
        SCI_TRACE_LOW("DecodeJpgFile:sfs_handle %d is error!",s_sfs_handle);
    }

    //free dec file buffer
    MMITHEME_FreeJpgDecBuf(s_guiref_img_dec_block.block_ptr);
    s_guiref_img_dec_block.block_ptr = PNULL;
    jpg_in.decode_buf_ptr = PNULL;

    //close file
    CloseFile();

    return (result);
}

/*****************************************************************************/
//  Description : read jpg file data syn
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL BOOLEAN ReadJpgFileDataSyn(
                                 uint8      *data_buffer_ptr,   //in
                                 uint32     file_offset,        //in
                                 uint32     to_read_size,       //in
                                 uint32     *read_size_ptr      //in/out
                                 )
{
    BOOLEAN     result = TRUE;

    SCI_ASSERT(PNULL != data_buffer_ptr);
    SCI_ASSERT(PNULL != read_size_ptr);

    s_guiref_img_is_operate_file = TRUE;
    
    //seek file position
    if(SFS_ERROR_NONE != SFS_SetFilePointer(s_sfs_handle,file_offset,SFS_SEEK_BEGIN))
    {
        result = FALSE;
    }

    if (result)
    {
        if (SFS_ERROR_NONE != SFS_ReadFile(s_sfs_handle,data_buffer_ptr,to_read_size,read_size_ptr,PNULL))
        {
            result = FALSE;
        }
    }

    //close file
    if (!result)
    {
        CloseFile();
    }

    s_guiref_img_is_operate_file = FALSE;
    
    return (result);
}

/*****************************************************************************/
//  Description : decode jpg data buffer
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL BOOLEAN DecodeJpgBuf(
                           uint8                *data_buffer_ptr,   //in/out:decode后图片数据buffer
                           uint32               data_buffer_size,   //in:decode后图片数据buffer的大小
                           uint32               dest_width,         //in:图片目标宽度
                           uint32               dest_height,        //in:图片目标高度
                           uint32               *actual_width_ptr,  //in/out:图片实际宽度
                           uint32               *actual_height_ptr, //in/out:图片实际高度
                           GUI_RECT_T           *crop_rect_ptr,     //in:may PNULL
                           GUIREF_IMG_DATA_T    *src_data_ptr       //in:传入的图片数据,may PNULL
                           )
{
    BOOLEAN                 result = FALSE;
    JINF_RET_E              jpg_result = JINF_SUCCESS;
    JINF_DEC_IN_PARAM_T     jpg_in = {0};
    JINF_DEC_OUT_PARAM_T    jpg_out = {0};

    SCI_ASSERT(PNULL != data_buffer_ptr);
    SCI_ASSERT(PNULL != src_data_ptr);
    SCI_ASSERT(PNULL != actual_width_ptr);
    SCI_ASSERT(PNULL != actual_height_ptr);

    //set jpg input buffer
    jpg_in.jpeg_buf_ptr = src_data_ptr->src_buf_ptr;
    jpg_in.jpeg_buf_size = src_data_ptr->src_data_size;
    
    //set jpg decode buffer
    jpg_in.decode_buf_ptr = s_guiref_img_dec_block.block_ptr;
    jpg_in.decode_buf_size = s_guiref_img_dec_block.block_size;

    //set crop rect
    if ((PNULL != crop_rect_ptr) &&
        (crop_rect_ptr->right > crop_rect_ptr->left) &&
        (crop_rect_ptr->bottom > crop_rect_ptr->top))
    {
        jpg_in.crop_rect.x = crop_rect_ptr->left;
        jpg_in.crop_rect.y = crop_rect_ptr->top;
        jpg_in.crop_rect.w = crop_rect_ptr->right - crop_rect_ptr->left + 1;
        jpg_in.crop_rect.h = crop_rect_ptr->bottom - crop_rect_ptr->top + 1;
    }

    //set param,dest jpg info
    jpg_in.target_width = dest_width;
    jpg_in.target_height = dest_height;
    jpg_in.target_data_type = JINF_DATA_TYPE_RGB;
    jpg_in.target_rgb_format = JINF_FORMAT_RGB565;
    jpg_in.target_buf_ptr = data_buffer_ptr;
    jpg_in.target_buf_size = data_buffer_size;

    //decode jpg
    jpg_result = IMGJPEG_Decode(&jpg_in,&jpg_out);
    SCI_TRACE_LOW("DecodeJpgBuf:jpg decode result is %d!",jpg_result);
    switch (jpg_result)
    {
    case JINF_SUCCESS:
        *actual_width_ptr = jpg_out.output_width;
        *actual_height_ptr = jpg_out.output_height;
        
        result = TRUE;
        break;

    default:
        result = FALSE;
        break;
    }
            
    //free dec file buffer
    MMITHEME_FreeJpgDecBuf(jpg_in.decode_buf_ptr);
    jpg_in.decode_buf_ptr = PNULL;
    s_guiref_img_dec_block.block_ptr = PNULL;

    return (result);
}

/*****************************************************************************/
//  Description : encode jpg image
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
PUBLIC BOOLEAN GUIREF_EncodeJpg(
                                uint8               *data_buffer_ptr,       //in/out:encode后图片数据buffer
                                uint16              *full_path_ptr,         //in:文件名,may PNULL
                                uint32              *data_buffer_size_ptr,  //in/out:encode后图片数据buffer的大小
                                uint32              dest_width,             //in:图片目标宽度
                                uint32              dest_height,            //in:图片目标高度
                                uint32              *actual_width_ptr,      //in/out:图片实际宽度
                                uint32              *actual_height_ptr,     //in/out:图片实际高度
                                GUIREF_IMG_DATA_T   *src_data_ptr           //in:传入的图片数据,may PNULL
                                )
{
    BOOLEAN             result = FALSE;
    BOOLEAN             is_file = FALSE;

    SCI_ASSERT(PNULL != data_buffer_ptr);
    SCI_ASSERT((PNULL != src_data_ptr) || (PNULL != full_path_ptr));
    SCI_ASSERT(PNULL != data_buffer_size_ptr);
    SCI_ASSERT(PNULL != actual_width_ptr);
    SCI_ASSERT(PNULL != actual_height_ptr);

    //judge is file or image data buffer
    if (PNULL != src_data_ptr)
    {
        is_file = FALSE;
    }
    else if (PNULL != full_path_ptr)
    {
        is_file = TRUE;
    }
    else
    {
        SCI_PASSERT(FALSE,("GUIREF_EncodeJpg:src data %d,full path %d are error!",src_data_ptr,full_path_ptr));
    }
    
    if (is_file)
    {
        //encode jpg file image
        result = EncodeJpgFile(data_buffer_ptr,
                        data_buffer_size_ptr,
                        dest_width,
                        dest_height,
                        actual_width_ptr,
                        actual_height_ptr);
    }
    else
    {
        //encode jpg image buffer
        result = EncodeJpgBuf(data_buffer_ptr,
                        data_buffer_size_ptr,
                        dest_width,
                        dest_height,
                        actual_width_ptr,
                        actual_height_ptr,
                        src_data_ptr);
    }

    return (result);
}

/*****************************************************************************/
//  Description : encode jpg file
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL BOOLEAN EncodeJpgFile(
                            uint8       *data_buffer_ptr,       //in/out:decode后图片数据buffer
                            uint32      *data_buffer_size_ptr,  //in/out:decode后图片数据buffer的大小
                            uint32      dest_width,             //in:图片目标宽度
                            uint32      dest_height,            //in:图片目标高度
                            uint32      *actual_width_ptr,      //in/out:图片实际宽度
                            uint32      *actual_height_ptr      //in/out:图片实际高度
                            )
{
    BOOLEAN                 result = FALSE;
    uint32                  file_size = 0;
    JINF_RET_E              jpg_result = JINF_SUCCESS;
    JINF_MINI_IN_PARAM_T    jpg_in = {0};
    JINF_MINI_OUT_PARAM_T   jpg_out = {0};

    SCI_ASSERT(PNULL != data_buffer_ptr);
    SCI_ASSERT(PNULL != data_buffer_size_ptr);
    SCI_ASSERT(PNULL != actual_width_ptr);
    SCI_ASSERT(PNULL != actual_height_ptr);

    //open file in mmi
    if (0 != s_sfs_handle)
    {
        //get file size
        if(SFS_ERROR_NONE == GetImgFileSize(s_sfs_handle, &file_size))
        {
            //set jpg decode buffer
            jpg_in.decode_buf_ptr = s_guiref_img_dec_block.block_ptr;
            jpg_in.decode_buf_size = s_guiref_img_dec_block.block_size;

            jpg_in.target_width = dest_width;
            jpg_in.target_height = dest_height;

            jpg_in.quality = JINF_QUALITY_MIDDLE_HIGH;

            jpg_in.target_buf_ptr = data_buffer_ptr;
            jpg_in.target_buf_size = *data_buffer_size_ptr;

            jpg_in.read_file_func = ReadJpgFileDataSyn;

            //encode jpg
            jpg_result = IMGJPEG_CreateMiniature(&jpg_in,&jpg_out);
            SCI_TRACE_LOW("EncodeJpgFile:jpg encode result is %d!",jpg_result);
            switch (jpg_result)
            {
            case JINF_SUCCESS:
                result = TRUE;
                break;

            default:
                result = FALSE;
                break;
            }
        }
        else
        {
            result = FALSE;
            SCI_TRACE_LOW("EncodeJpgFile:get file size error!");
        }
    }
    else
    {
        result = FALSE;
        SCI_TRACE_LOW("EncodeJpgFile:sfs_handle %d is error!",s_sfs_handle);
    }

    //free dec file buffer
    MMITHEME_FreeJpgDecBuf(s_guiref_img_dec_block.block_ptr);
    s_guiref_img_dec_block.block_ptr = PNULL;
    jpg_in.decode_buf_ptr = PNULL;

    //close file
    CloseFile();
    
    *actual_width_ptr = jpg_out.output_width;
    *actual_height_ptr = jpg_out.output_height;

    *data_buffer_size_ptr = jpg_out.jpeg_buf_size;

    return (result);
}

/*****************************************************************************/
//  Description : encode data buffer
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL BOOLEAN EncodeJpgBuf(
                           uint8                *data_buffer_ptr,       //in/out:decode后图片数据buffer
                           uint32               *data_buffer_size_ptr,  //in/out:decode后图片数据buffer的大小
                           uint32               dest_width,             //in:图片目标宽度
                           uint32               dest_height,            //in:图片目标高度
                           uint32               *actual_width_ptr,      //in/out:图片实际宽度
                           uint32               *actual_height_ptr,     //in/out:图片实际高度
                           GUIREF_IMG_DATA_T    *src_data_ptr           //in:传入的图片数据
                           )
{
    BOOLEAN                 result = FALSE;
    JINF_RET_E              jpg_result = JINF_SUCCESS;
    JINF_MINI_IN_PARAM_T    jpg_in = {0};
    JINF_MINI_OUT_PARAM_T   jpg_out = {0};

    SCI_ASSERT(PNULL != data_buffer_ptr);
    SCI_ASSERT(PNULL != actual_width_ptr);
    SCI_ASSERT(PNULL != actual_height_ptr);
    SCI_ASSERT(PNULL != src_data_ptr);
    SCI_ASSERT(PNULL != src_data_ptr->src_buf_ptr);

    //set input param
    jpg_in.jpeg_buf_ptr = src_data_ptr->src_buf_ptr;
    jpg_in.jpeg_buf_size = src_data_ptr->src_data_size;

    jpg_in.target_width = dest_width;
    jpg_in.target_height = dest_height;

    jpg_in.quality = JINF_QUALITY_MIDDLE_HIGH;

    //set jpg decode buffer
    jpg_in.decode_buf_ptr = s_guiref_img_dec_block.block_ptr;
    jpg_in.decode_buf_size = s_guiref_img_dec_block.block_size;

    jpg_in.target_buf_ptr = data_buffer_ptr;
    jpg_in.target_buf_size = *data_buffer_size_ptr;

    //encode jpg
    jpg_result = IMGJPEG_CreateMiniature(&jpg_in,&jpg_out);
    SCI_TRACE_LOW("EncodeJpgBuf:jpg encode result is %d!",jpg_result);
    switch (jpg_result)
    {
    case JINF_SUCCESS:
        result = TRUE;
        break;

    default:
        result = FALSE;
        break;
    }

    *actual_width_ptr = jpg_out.output_width;
    *actual_height_ptr = jpg_out.output_height;
    
    *data_buffer_size_ptr = jpg_out.jpeg_buf_size;

    //free dec file buffer
    MMITHEME_FreeJpgDecBuf(s_guiref_img_dec_block.block_ptr);
    s_guiref_img_dec_block.block_ptr = PNULL;
    jpg_in.decode_buf_ptr = PNULL;

    return (result);
}

/*****************************************************************************/
//  Description : decode bmp or wbmp image
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
PUBLIC BOOLEAN GUIREF_DecodeBmpWbmp(
                                    BOOLEAN             is_bmp,             //in:TRUE:BMP;FALSE:WBMP
                                    uint8               *data_buffer_ptr,   //in/out:decode后图片数据buffer
                                    uint16              *full_path_ptr,     //in:文件名,may PNULL
                                    uint32              data_buffer_size,   //in:decode后图片数据buffer的大小
                                    uint32              dest_width,         //in:图片目标宽度
                                    uint32              dest_height,        //in:图片目标高度
                                    uint32              *actual_width_ptr,  //in/out:图片实际宽度
                                    uint32              *actual_height_ptr, //in/out:图片实际高度
                                    GUI_RECT_T          *crop_rect_ptr,     //in:may PNULL
                                    GUIREF_IMG_DATA_T   *src_data_ptr       //in:传入的图片数据,may PNULL
                                    )
{
    BOOLEAN             result = FALSE;
    BOOLEAN             is_file = FALSE;

    SCI_ASSERT(PNULL != data_buffer_ptr);
    SCI_ASSERT((PNULL != src_data_ptr) || (PNULL != full_path_ptr));
    SCI_ASSERT(PNULL != actual_width_ptr);
    SCI_ASSERT(PNULL != actual_height_ptr);

    //judge is file or image data buffer
    if (PNULL != src_data_ptr)
    {
        is_file = FALSE;
    }
    else if (PNULL != full_path_ptr)
    {
        is_file = TRUE;
    }
    else
    {
        SCI_PASSERT(FALSE,("GUIREF_DecodeBmpWbmp:src data %d,full path %d are error!",src_data_ptr,full_path_ptr));
    }
    
#ifdef BMP_DEC_SUPPORT
    if (is_file)
    {
        //decode bmp/wbmp file image
        result = DecodeBmpWbmpFile(is_bmp,
                        data_buffer_ptr,
                        data_buffer_size,
                        dest_width,
                        dest_height,
                        actual_width_ptr,
                        actual_height_ptr,
                        crop_rect_ptr);
    }
    else
    {
        //decode bmp/wbmp image buffer
        result = DecodeBmpWbmpBuf(is_bmp,
                        data_buffer_ptr,
                        data_buffer_size,
                        dest_width,
                        dest_height,
                        actual_width_ptr,
                        actual_height_ptr,
                        crop_rect_ptr,
                        src_data_ptr);
    }
#endif

    return (result);
}

/*****************************************************************************/
//  Description : decode bmp or wbmp file
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL BOOLEAN DecodeBmpWbmpFile(
                                BOOLEAN     is_bmp,             //in:TRUE:BMP;FALSE:WBMP
                                uint8       *data_buffer_ptr,   //in/out:decode后图片数据buffer
                                uint32      data_buffer_size,   //in:decode后图片数据buffer的大小
                                uint32      dest_width,         //in:图片目标宽度
                                uint32      dest_height,        //in:图片目标高度
                                uint32      *actual_width_ptr,  //in/out:图片实际宽度
                                uint32      *actual_height_ptr, //in/out:图片实际高度
                                GUI_RECT_T  *crop_rect_ptr      //in:may PNULL
                                )
{
    BOOLEAN                     result = FALSE;
    uint32                      src_bmp_width = 0;
    uint32                      src_bmp_height = 0;
    uint32                      src_bmp_color_depth = 0;
    uint32                      file_size = 0;
    IMGPROC_BMP_APP_ERROR_E     bmp_result = IMGPROC_BMP_APP_SUCCESS;
    IMGPROC_BMP_DECODE_APP_T    bmp_decode = {0};

    SCI_ASSERT(PNULL != data_buffer_ptr);
    SCI_ASSERT(PNULL != actual_width_ptr);
    SCI_ASSERT(PNULL != actual_height_ptr);
    
    //open file in mmi
    if (0 != s_sfs_handle)
    {
        //get file size
        if(SFS_ERROR_NONE == GetImgFileSize(s_sfs_handle, &file_size))
        {
            //set param,decode buffer
            bmp_decode.scalingdown_buf_ptr = s_guiref_img_dec_block.block_ptr;
            bmp_decode.scalingdown_buf_ptr_size = s_guiref_img_dec_block.block_size;
        
            //set read file buffer
            bmp_decode.file_buf_ptr = s_guiref_img_read_block.block_ptr;
            bmp_decode.file_buf_ptr_size = s_guiref_img_read_block.block_size;

            //set param,src bmp info
            bmp_decode.src_width_ptr = (int32 *)&src_bmp_width;
            bmp_decode.src_height_ptr = (int32 *)&src_bmp_height;
            bmp_decode.scr_color_depth_ptr = (int32 *)&src_bmp_color_depth;
            bmp_decode.src_img_size = file_size;

            //set param,dest bmp info
            bmp_decode.target_width = dest_width;
            bmp_decode.target_height = dest_height;
            bmp_decode.target_type = RGB565_CUS;
            bmp_decode.target_buf_ptr = data_buffer_ptr;
            bmp_decode.target_buf_ptr_size = data_buffer_size;

            bmp_decode.ret_target_width_ptr = (int32 *)actual_width_ptr;
            bmp_decode.ret_target_height_ptr = (int32 *)actual_height_ptr;

            //set crop rect
            if ((PNULL != crop_rect_ptr) &&
                (crop_rect_ptr->right > crop_rect_ptr->left) &&
                (crop_rect_ptr->bottom > crop_rect_ptr->top))
            {
                bmp_decode.imgrect.left   = crop_rect_ptr->left;
                bmp_decode.imgrect.top    = crop_rect_ptr->top;
                bmp_decode.imgrect.right  = crop_rect_ptr->right;
                bmp_decode.imgrect.bottom = crop_rect_ptr->bottom;
            }

            //read file
            bmp_decode.app_read_data = ReadFileDataSyn;

            if (is_bmp)
            {
                //decode bmp
                bmp_result = BMPAPP_BmpDecode(&bmp_decode);
            }
            else
            {
                bmp_result = BMPAPP_WbmpDecode(&bmp_decode);
            }
            SCI_TRACE_LOW("DecodeBmpWbmpFile:bmp decode result is %d!",bmp_result);
            switch (bmp_result)
            {
            case IMGPROC_BMP_APP_SUCCESS:
                result = TRUE;
                break;

            default:
                result = FALSE;
                break;
            }
        }
        else
        {
            result = FALSE;
            SCI_TRACE_LOW("DecodeBmpWbmpFile:get file size error!");
        }
    }
    else
    {
        result = FALSE;
        SCI_TRACE_LOW("DecodeBmpWbmpFile:sfs_handle %d is error!",s_sfs_handle);
    }

    //free blcok buffer
    MMITHEME_FreeDecBuf(s_guiref_img_dec_block.block_ptr);
    s_guiref_img_dec_block.block_ptr = PNULL;
    bmp_decode.scalingdown_buf_ptr = PNULL;

    //free read file buffer
    MMITHEME_FreeReadFileBuf(s_guiref_img_read_block.block_ptr);
    s_guiref_img_read_block.block_ptr = PNULL;
    bmp_decode.file_buf_ptr = PNULL;

    //close file
    CloseFile();

    return (result);
}

/*****************************************************************************/
//  Description : decode bmp or wbmp data buffer
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL BOOLEAN DecodeBmpWbmpBuf(
                               BOOLEAN              is_bmp,             //in:TRUE:BMP;FALSE:WBMP
                               uint8                *data_buffer_ptr,   //in/out:decode后图片数据buffer
                               uint32               data_buffer_size,   //in:decode后图片数据buffer的大小
                               uint32               dest_width,         //in:图片目标宽度
                               uint32               dest_height,        //in:图片目标高度
                               uint32               *actual_width_ptr,  //in/out:图片实际宽度
                               uint32               *actual_height_ptr, //in/out:图片实际高度
                               GUI_RECT_T           *crop_rect_ptr,     //in:may PNULL
                               GUIREF_IMG_DATA_T    *src_data_ptr       //in:传入的图片数据,may PNULL
                               )
{
    BOOLEAN                     result = FALSE;
    uint32                      src_bmp_width = 0;
    uint32                      src_bmp_height = 0;
    uint32                      src_bmp_color_depth = 0;
    IMGPROC_BMP_APP_ERROR_E     bmp_result = IMGPROC_BMP_APP_SUCCESS;
    IMGPROC_BMP_DECODE_APP_T    bmp_decode = {0};

    SCI_ASSERT(PNULL != data_buffer_ptr);
    SCI_ASSERT(PNULL != src_data_ptr);
    SCI_ASSERT(PNULL != actual_width_ptr);
    SCI_ASSERT(PNULL != actual_height_ptr);

    //set param,decode buffer
    bmp_decode.scalingdown_buf_ptr = s_guiref_img_dec_block.block_ptr;
    bmp_decode.scalingdown_buf_ptr_size = s_guiref_img_dec_block.block_size;

    //set read file buffer
    bmp_decode.file_buf_ptr = s_guiref_img_read_block.block_ptr;
    bmp_decode.file_buf_ptr_size = s_guiref_img_read_block.block_size;

    //set param,src bmp info
    bmp_decode.src_width_ptr = (int32 *)&src_bmp_width;
    bmp_decode.src_height_ptr = (int32 *)&src_bmp_height;
    bmp_decode.scr_color_depth_ptr = (int32 *)&src_bmp_color_depth;
    bmp_decode.src_img_size = src_data_ptr->src_data_size;

    //set param,dest bmp info
    bmp_decode.target_width = dest_width;
    bmp_decode.target_height = dest_height;
    bmp_decode.target_type = RGB565_CUS;
    bmp_decode.target_buf_ptr = data_buffer_ptr;
    bmp_decode.target_buf_ptr_size = data_buffer_size;

    bmp_decode.ret_target_width_ptr = (int32 *)actual_width_ptr;
    bmp_decode.ret_target_height_ptr = (int32 *)actual_height_ptr;

    //set crop rect
    if ((PNULL != crop_rect_ptr) &&
        (crop_rect_ptr->right > crop_rect_ptr->left) &&
        (crop_rect_ptr->bottom > crop_rect_ptr->top))
    {
        bmp_decode.imgrect.left   = crop_rect_ptr->left;
        bmp_decode.imgrect.top    = crop_rect_ptr->top;
        bmp_decode.imgrect.right  = crop_rect_ptr->right;
        bmp_decode.imgrect.bottom = crop_rect_ptr->bottom;
    }

    //set read data buffer info
    SCI_MEMSET(&s_guiref_read_data_info,0,sizeof(GUIREF_IMG_READ_DATA_T));
    s_guiref_read_data_info.src_data_info.src_buf_ptr = src_data_ptr->src_buf_ptr;
    s_guiref_read_data_info.src_data_info.src_data_size = src_data_ptr->src_data_size;

    //read data buffer
    bmp_decode.app_read_data = ReadDataBuffer;

    if (is_bmp)
    {
        //decode bmp
        bmp_result = BMPAPP_BmpDecode(&bmp_decode);
    }
    else
    {
        bmp_result = BMPAPP_WbmpDecode(&bmp_decode);
    }
    SCI_TRACE_LOW("DecodeBmpWbmpBuf:bmp decode result is %d!",bmp_result);
    switch (bmp_result)
    {
    case IMGPROC_BMP_APP_SUCCESS:
        result = TRUE;
        break;

    default:
        result = FALSE;
        break;
    }

    //free blcok buffer
    MMITHEME_FreeDecBuf(bmp_decode.scalingdown_buf_ptr);
    bmp_decode.scalingdown_buf_ptr = PNULL;
    s_guiref_img_dec_block.block_ptr = PNULL;

    //free read file buffer
    MMITHEME_FreeReadFileBuf(bmp_decode.file_buf_ptr);
    bmp_decode.file_buf_ptr = PNULL;
    s_guiref_img_read_block.block_ptr = PNULL;

    return (result);
}

/*****************************************************************************/
//  Description : get image file size
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL SFS_ERROR_E GetImgFileSize(
                                 SFS_HANDLE    file_handle,
                                 uint32        *file_size_ptr
                                 )
{
    SFS_ERROR_E     result = SFS_ERROR_NONE;

    SCI_ASSERT(PNULL != file_size_ptr);

    s_guiref_img_is_operate_file = TRUE;
    result = SFS_GetFileSize(file_handle, file_size_ptr);
    s_guiref_img_is_operate_file = FALSE;
    
    return (result);
}

/*****************************************************************************/
//  Description : read file data syn
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL void ReadFileDataSyn(
                           uint8    *data_buffer_ptr,   //in
                           uint32   to_read_size,       //in
                           uint32   *read_size_ptr      //in/out
                           )
{

		SFS_ERROR_E errCode;
       SCI_ASSERT(PNULL != data_buffer_ptr);
       SCI_ASSERT(PNULL != read_size_ptr);

       s_guiref_img_is_operate_file = TRUE;

	errCode = SFS_ReadFile(s_sfs_handle,data_buffer_ptr,to_read_size,read_size_ptr,PNULL);
       if (SFS_ERROR_NONE != errCode)
       {
		SCI_TRACE_LOW("ReadFileDataSyn:read file error!");
		*read_size_ptr = 0;
       }

	 s_guiref_img_is_operate_file = FALSE;
}

/*****************************************************************************/
//  Description : close file
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL void CloseFile(void)
{
    s_guiref_img_is_operate_file = TRUE;
    
    //close file
    SFS_CloseFile(s_sfs_handle);
    s_sfs_handle = 0;

    s_guiref_img_is_operate_file = FALSE;
}

/*****************************************************************************/
//  Description : read image data buffer
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/

LOCAL void ReadDataBuffer(
                          uint8    *data_buffer_ptr,   //in
                          uint32   to_read_size,       //in
                          uint32   *read_size_ptr      //in/out
                          )
{
    uint8   *read_data_ptr = PNULL;
    uint32  read_size  = 0;
    uint32  not_read_size = 0;

    SCI_ASSERT(PNULL != data_buffer_ptr);
    SCI_ASSERT(PNULL != read_size_ptr);

    //set not read size
    if (s_guiref_read_data_info.src_data_info.src_data_size >= s_guiref_read_data_info.read_pos)
    {
        not_read_size = s_guiref_read_data_info.src_data_info.src_data_size - s_guiref_read_data_info.read_pos;

        //set read size
        if (0 < not_read_size)
        {
            if (not_read_size > to_read_size)
            {
                read_size = to_read_size;
            }
            else
            {
                read_size = not_read_size;
            }

            read_data_ptr = s_guiref_read_data_info.src_data_info.src_buf_ptr + s_guiref_read_data_info.read_pos;
            MMI_MEMCPY(data_buffer_ptr,to_read_size,read_data_ptr,read_size,read_size);

            s_guiref_read_data_info.read_pos += read_size;
        }
    }

    *read_size_ptr = read_size;

    return ;
}

/*****************************************************************************/
//  Description : decode png image
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
PUBLIC BOOLEAN GUIREF_DecodePng(
                                uint8               *data_buffer_ptr,   //in/out:decode后图片数据buffer
                                uint16              *full_path_ptr,     //in:文件名,may PNULL
                                uint32              data_buffer_size,   //in:decode后图片数据buffer的大小
                                uint32              dest_width,         //in:图片目标宽度
                                uint32              dest_height,        //in:图片目标高度
                                uint32              *actual_width_ptr,  //in/out:图片实际宽度
                                uint32              *actual_height_ptr, //in/out:图片实际高度
                                GUI_RECT_T          *crop_rect_ptr,     //in:may PNULL
                                GUIREF_IMG_DATA_T   *src_data_ptr       //in:传入的图片数据,may PNULL
                                )
{
    BOOLEAN             result = FALSE;
    BOOLEAN             is_file = FALSE;

    SCI_ASSERT(PNULL != data_buffer_ptr);
    SCI_ASSERT((PNULL != src_data_ptr) || (PNULL != full_path_ptr));
    SCI_ASSERT(PNULL != actual_width_ptr);
    SCI_ASSERT(PNULL != actual_height_ptr);

    //judge is file or image data buffer
    if (PNULL != src_data_ptr)
    {
        is_file = FALSE;
    }
    else if (PNULL != full_path_ptr)
    {
        is_file = TRUE;
    }
    else
    {
        SCI_PASSERT(FALSE,("GUIREF_DecodePng:src data %d,full path %d are error!",src_data_ptr,full_path_ptr));
    }
    
#ifdef PNG_DEC_SUPPORT
    if (is_file)
    {
        //decode png file image
        result = DecodePngFile(data_buffer_ptr,
                        data_buffer_size,
                        dest_width,
                        dest_height,
                        actual_width_ptr,
                        actual_height_ptr,
                        crop_rect_ptr);
    }
    else
    {
        //decode png image buffer
        result = DecodePngBuf(data_buffer_ptr,
                        data_buffer_size,
                        dest_width,
                        dest_height,
                        actual_width_ptr,
                        actual_height_ptr,
                        crop_rect_ptr,
                        src_data_ptr);
    }
#endif

    return (result);
}

/*****************************************************************************/
//  Description : decode png file
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL BOOLEAN DecodePngFile(
                            uint8       *data_buffer_ptr,   //in/out:decode后图片数据buffer
                            uint32      data_buffer_size,   //in:decode后图片数据buffer的大小
                            uint32      dest_width,         //in:图片目标宽度
                            uint32      dest_height,        //in:图片目标高度
                            uint32      *actual_width_ptr,  //in/out:图片实际宽度
                            uint32      *actual_height_ptr, //in/out:图片实际高度
                            GUI_RECT_T  *crop_rect_ptr      //in:may PNULL
                            )
{
    BOOLEAN                             result = FALSE;
    uint32                              file_size = 0;
    IMGPORC_PNG_APP_RETURN_E            png_result = IMGPROC_PNG_APP_SUCCESS;
    IMGPROC_PNG_DECODE_APP_INPUT_T      png_decode_input = {0};
    IMGPROC_PNG_DECODE_APP_OUTPUT_T     png_decode_output = {0};

    SCI_ASSERT(PNULL != data_buffer_ptr);
    SCI_ASSERT(PNULL != actual_width_ptr);
    SCI_ASSERT(PNULL != actual_height_ptr);

    //open file in mmi
    if (0 != s_sfs_handle)
    {
        //get file size
        if(SFS_ERROR_NONE == GetImgFileSize(s_sfs_handle, &file_size))
        {
            //set input param
            png_decode_input.read_buf_ptr = s_guiref_img_read_block.block_ptr;
            png_decode_input.read_buf_size = s_guiref_img_read_block.block_size;

            png_decode_input.scalingdown_buf_ptr = s_guiref_img_dec_block.block_ptr;
            png_decode_input.scalingdown_buf_size = s_guiref_img_dec_block.block_size;
            
            png_decode_input.target_buf_ptr = data_buffer_ptr;
            png_decode_input.target_buf_size = data_buffer_size;

            png_decode_input.save_buf_ptr = s_guiref_img_save_block.block_ptr;
            png_decode_input.save_buf_size = s_guiref_img_save_block.block_size;
    
            png_decode_input.target_width = dest_width;
            png_decode_input.target_height = dest_height;

            png_decode_input.target_type = RGB565_CUS;

	     png_decode_input.img_file_size = file_size;
            //set crop rect
            if ((PNULL != crop_rect_ptr) &&
                (crop_rect_ptr->right > crop_rect_ptr->left) &&
                (crop_rect_ptr->bottom > crop_rect_ptr->top))
            {
                png_decode_input.imgrect.left   = crop_rect_ptr->left;
                png_decode_input.imgrect.top    = crop_rect_ptr->top;
                png_decode_input.imgrect.right  = crop_rect_ptr->right;
                png_decode_input.imgrect.bottom = crop_rect_ptr->bottom;
            }
    
            //read file
            png_decode_input.app_read_data = ReadFileDataSyn;

            png_result = PNGAPP_Decode(&png_decode_input,&png_decode_output);
            SCI_TRACE_LOW("DecodePngFile:png decode result is %d,data_buffer_ptr = 0x%x!",png_result,data_buffer_ptr);
            switch (png_result)
            {
            case IMGPROC_PNG_APP_SUCCESS:
                result = TRUE;
                break;

            default:
                result = FALSE;
                break;
            }
        }
        else
        {
            result = FALSE;
            SCI_TRACE_LOW("DecodePngFile:get file size error!");
        }
    }
    else
    {
        result = FALSE;
        SCI_TRACE_LOW("DecodePngFile:sfs_handle %d is error!",s_sfs_handle);
    }

    //free block buffer
    MMITHEME_FreeReadFileBuf(s_guiref_img_read_block.block_ptr);
    s_guiref_img_read_block.block_ptr = PNULL;
    png_decode_input.read_buf_ptr = PNULL;

    MMITHEME_FreeDecBuf(s_guiref_img_dec_block.block_ptr);
    s_guiref_img_dec_block.block_ptr = PNULL;
    png_decode_input.scalingdown_buf_size = PNULL;

    MMITHEME_FreeSaveGifFrameBuf(s_guiref_img_save_block.block_ptr);
    s_guiref_img_save_block.block_ptr = PNULL;
    png_decode_input.save_buf_ptr = PNULL;

    //close file
    CloseFile();
    
    *actual_width_ptr = png_decode_output.image_disaplay_width;
    *actual_height_ptr = png_decode_output.image_disaplay_height;

    return (result);
}

/*****************************************************************************/
//  Description : decode png data buffer
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL BOOLEAN DecodePngBuf(
                           uint8                *data_buffer_ptr,   //in/out:decode后图片数据buffer
                           uint32               data_buffer_size,   //in:decode后图片数据buffer的大小
                           uint32               dest_width,         //in:图片目标宽度
                           uint32               dest_height,        //in:图片目标高度
                           uint32               *actual_width_ptr,  //in/out:图片实际宽度
                           uint32               *actual_height_ptr, //in/out:图片实际高度
                           GUI_RECT_T           *crop_rect_ptr,     //in:may PNULL
                           GUIREF_IMG_DATA_T    *src_data_ptr       //in:传入的图片数据
                           )
{
    BOOLEAN                             result = FALSE;
    IMGPORC_PNG_APP_RETURN_E            png_result = IMGPROC_PNG_APP_SUCCESS;
    IMGPROC_PNG_DECODE_APP_INPUT_T      png_decode_input = {0};
    IMGPROC_PNG_DECODE_APP_OUTPUT_T     png_decode_output = {0};

    SCI_ASSERT(PNULL != data_buffer_ptr);
    SCI_ASSERT(PNULL != actual_width_ptr);
    SCI_ASSERT(PNULL != actual_height_ptr);
    SCI_ASSERT(PNULL != src_data_ptr);
    SCI_ASSERT(PNULL != src_data_ptr->src_buf_ptr);

    //set input param
    png_decode_input.read_buf_ptr = s_guiref_img_read_block.block_ptr;
    png_decode_input.read_buf_size = s_guiref_img_read_block.block_size;

    png_decode_input.scalingdown_buf_ptr = s_guiref_img_dec_block.block_ptr;
    png_decode_input.scalingdown_buf_size = s_guiref_img_dec_block.block_size;

    png_decode_input.target_buf_ptr = data_buffer_ptr;
    png_decode_input.target_buf_size = data_buffer_size;

    png_decode_input.save_buf_ptr = s_guiref_img_save_block.block_ptr;
    png_decode_input.save_buf_size = s_guiref_img_save_block.block_size;

    png_decode_input.target_width = dest_width;
    png_decode_input.target_height = dest_height;

    png_decode_input.target_type = RGB565_CUS;

    //set crop rect
    if ((PNULL != crop_rect_ptr) &&
        (crop_rect_ptr->right > crop_rect_ptr->left) &&
        (crop_rect_ptr->bottom > crop_rect_ptr->top))
    {
        png_decode_input.imgrect.left   = crop_rect_ptr->left;
        png_decode_input.imgrect.top    = crop_rect_ptr->top;
        png_decode_input.imgrect.right  = crop_rect_ptr->right;
        png_decode_input.imgrect.bottom = crop_rect_ptr->bottom;
    }

    //read file
    png_decode_input.app_read_data = ReadDataBuffer;

    //output

    //set read data buffer info
    SCI_MEMSET(&s_guiref_read_data_info,0,sizeof(GUIREF_IMG_READ_DATA_T));
    s_guiref_read_data_info.src_data_info.src_buf_ptr = src_data_ptr->src_buf_ptr;
    s_guiref_read_data_info.src_data_info.src_data_size = src_data_ptr->src_data_size;
    png_decode_input.img_file_size =  src_data_ptr->src_data_size;

    png_result = PNGAPP_Decode(&png_decode_input,&png_decode_output);
    SCI_TRACE_LOW("DecodePngBuf:gif decode result is %d,data_buffer_ptr = 0x%x!",png_result,data_buffer_ptr);
    switch (png_result)
    {
    case IMGPROC_PNG_APP_SUCCESS:
        result = TRUE;
        break;

    default:
        result = FALSE;
        break;
    }

    //free block buffer
    MMITHEME_FreeReadFileBuf(png_decode_input.read_buf_ptr);
    png_decode_input.read_buf_ptr = PNULL;
    s_guiref_img_read_block.block_ptr = PNULL;

    MMITHEME_FreeDecBuf(png_decode_input.scalingdown_buf_ptr);
    png_decode_input.scalingdown_buf_ptr = PNULL;
    s_guiref_img_dec_block.block_ptr = PNULL;

    MMITHEME_FreeSaveGifFrameBuf(png_decode_input.save_buf_ptr);
    png_decode_input.save_buf_ptr = PNULL;
    s_guiref_img_save_block.block_ptr = PNULL;

    *actual_width_ptr = png_decode_output.image_disaplay_width;
    *actual_height_ptr = png_decode_output.image_disaplay_height;

    return (result);
}

/*****************************************************************************/
//  Description : decode gif image
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
PUBLIC BOOLEAN GUIREF_DecodeGif(
                                uint8               *data_buffer_ptr,   //in/out:图片数据buffer
                                uint16              *full_path_ptr,     //in:文件名,may PNULL
                                uint16              *total_frame_ptr,   //in/out:
                                uint32              data_buffer_size,   //in:decode后图片数据buffer的大小
                                uint32              dest_width,         //in:图片目标宽度
                                uint32              dest_height,        //in:图片目标高度
                                GUIREF_IMG_DATA_T   *src_data_ptr       //in:传入的图片数据,may PNULL
                                )
{
    BOOLEAN             result = FALSE;
    BOOLEAN             is_file = FALSE;

    SCI_ASSERT(PNULL != data_buffer_ptr);
    SCI_ASSERT((PNULL != src_data_ptr) || (PNULL != full_path_ptr));
    SCI_ASSERT(PNULL != total_frame_ptr);

    //judge is file or image data buffer
    if (PNULL != src_data_ptr)
    {
        is_file = FALSE;
    }
    else if (PNULL != full_path_ptr)
    {
        is_file = TRUE;
    }
    else
    {
        SCI_PASSERT(FALSE,("GUIREF_DecodeGif:src data %d,full path %d are error!",src_data_ptr,full_path_ptr));
    }
    
#ifdef GIF_DEC_SUPPORT
    if (is_file)
    {
        //decode gif file image
        result = DecodeGifFile(data_buffer_ptr,total_frame_ptr,data_buffer_size,dest_width,dest_height);
    }
    else
    {
        //decode gif image buffer
        result = DecodeGifBuf(data_buffer_ptr,total_frame_ptr,data_buffer_size,dest_width,dest_height,src_data_ptr);
    }
#endif

    return (result);
}

/*****************************************************************************/
//  Description : decode gif image file
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL BOOLEAN DecodeGifFile(
                            uint8       *data_buffer_ptr,   //in/out:图片数据buffer
                            uint16      *total_frame_ptr,   //in/out:
                            uint32      data_buffer_size,   //in:decode后图片数据buffer的大小
                            uint32      dest_width,         //in:图片目标宽度
                            uint32      dest_height         //in:图片目标高度
                            )
{
    BOOLEAN                             result = FALSE;
    uint32                              file_size = 0;
    IMGPROC_GIF_APP_RETURN_E            gif_result = IMGPROC_GIF_APP_SUCCESS;
    IMGPROC_GIF_DECODE_APP_INPUT_T      gif_decode_input = {0};
    IMGPROC_GIF_DECODE_APP_OUTPUT_T     gif_decode_output = {0};

    SCI_ASSERT(PNULL != data_buffer_ptr);
    SCI_ASSERT(PNULL != total_frame_ptr);

    //open file in mmi
    if (0 != s_sfs_handle)
    {
        //get file size
        if(SFS_ERROR_NONE == GetImgFileSize(s_sfs_handle, &file_size))
        {
            //set input param
            gif_decode_input.read_file_buf_ptr = s_guiref_img_read_block.block_ptr;
            gif_decode_input.read_file_buf_size = s_guiref_img_read_block.block_size;

            gif_decode_input.scalingdown_buf_ptr = s_guiref_img_dec_block.block_ptr;
            gif_decode_input.scalingdown_buf_size = s_guiref_img_dec_block.block_size;

            gif_decode_input.target_buf_ptr = data_buffer_ptr;
            gif_decode_input.target_buf_size = data_buffer_size;

            gif_decode_input.save_pre_frame_buf_ptr = s_guiref_img_save_block.block_ptr;
            gif_decode_input.save_pre_frame_buf_size = s_guiref_img_save_block.block_size;
    
            gif_decode_input.target_width = (uint16)dest_width;
            gif_decode_input.target_height = (uint16)dest_height;

            gif_decode_input.target_type = RGB565_CUS;
    
            //read file
            gif_decode_input.app_read_data = ReadFileDataSyn;

            //display info
            gif_decode_input.app_notice_display = NotifyGifDecodeOneFrame;

            //get decode status
            gif_decode_input.app_is_continue_next_frame = GUIANIM_IsContinueDecodeGif;

            gif_result = GIFDECODEAPP_Decode(&gif_decode_input,&gif_decode_output);
            SCI_TRACE_LOW("DecodeGifFile:gif decode result is %d,data_buffer_ptr = 0x%x!",gif_result,data_buffer_ptr);
            switch (gif_result)
            {
            case IMGPROC_GIF_APP_SUCCESS:
                result = TRUE;
                break;

            default:
                result = FALSE;
                break;
            }
        }
        else
        {
            result = FALSE;
//            SCI_TRACE_LOW("DecodeGifFile:get file size error!");
        }
    }
    else
    {
        result = FALSE;
//        SCI_TRACE_LOW("DecodeGifFile:sfs_handle %d is error!",s_sfs_handle);
    }

    //free block buffer
    MMITHEME_FreeReadFileBuf(s_guiref_img_read_block.block_ptr);
    s_guiref_img_read_block.block_ptr = PNULL;
    gif_decode_input.read_file_buf_ptr = PNULL;

    MMITHEME_FreeDecBuf(s_guiref_img_dec_block.block_ptr);
    s_guiref_img_dec_block.block_ptr = PNULL;
    gif_decode_input.scalingdown_buf_ptr = PNULL;

    MMITHEME_FreeSaveGifFrameBuf(s_guiref_img_save_block.block_ptr);
    s_guiref_img_save_block.block_ptr = PNULL;
    gif_decode_input.save_pre_frame_buf_ptr = PNULL;

    //close file
    CloseFile();

    *total_frame_ptr = gif_decode_output.frame_num;

    return (result);
}

/*****************************************************************************/
//  Description : decode gif image by buffer
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
// SFOX_SUPPORT
extern BOOLEAN SFOX_IsSFOXInUsing( void );
extern void SFOX_NotifyGifDecodeOneFrame(IMGPROC_GIF_DISPLAY_INFO_T *display_info_ptr);
extern BOOLEAN SFOX_IsContinueDecodeGif(void);

LOCAL BOOLEAN DecodeGifBuf(
                           uint8               *data_buffer_ptr,   //in/out:图片数据buffer
                           uint16              *total_frame_ptr,   //in/out:
                           uint32              data_buffer_size,    //in:decode后图片数据buffer的大小
                           uint32              dest_width,         //in:图片目标宽度
                           uint32              dest_height,        //in:图片目标高度
                           GUIREF_IMG_DATA_T   *src_data_ptr       //in:传入的图片数据
                           )
{
    BOOLEAN                             result = FALSE;
    IMGPROC_GIF_APP_RETURN_E            gif_result = IMGPROC_GIF_APP_SUCCESS;
    IMGPROC_GIF_DECODE_APP_INPUT_T      gif_decode_input = {0};
    IMGPROC_GIF_DECODE_APP_OUTPUT_T     gif_decode_output = {0};

    SCI_ASSERT(PNULL != data_buffer_ptr);
    SCI_ASSERT(PNULL != src_data_ptr);
    SCI_ASSERT(PNULL != src_data_ptr->src_buf_ptr);
    SCI_ASSERT(PNULL != total_frame_ptr);

    //set input param
    gif_decode_input.read_file_buf_ptr = s_guiref_img_read_block.block_ptr;
    gif_decode_input.read_file_buf_size = s_guiref_img_read_block.block_size;

    gif_decode_input.scalingdown_buf_ptr = s_guiref_img_dec_block.block_ptr;
    gif_decode_input.scalingdown_buf_size = s_guiref_img_dec_block.block_size;

    gif_decode_input.target_buf_ptr = data_buffer_ptr;
    gif_decode_input.target_buf_size = data_buffer_size;

    gif_decode_input.save_pre_frame_buf_ptr = s_guiref_img_save_block.block_ptr;
    gif_decode_input.save_pre_frame_buf_size = s_guiref_img_save_block.block_size;

    gif_decode_input.target_width = (uint16)dest_width;
    gif_decode_input.target_height = (uint16)dest_height;

    gif_decode_input.target_type = RGB565_CUS;

    //read file
    gif_decode_input.app_read_data = ReadDataBuffer;

    //display info
    gif_decode_input.app_notice_display = NotifyGifDecodeOneFrame;

    //get decode status
    gif_decode_input.app_is_continue_next_frame = GUIANIM_IsContinueDecodeGif;

	// SFOX_SUPPORT
	if(SFOX_IsSFOXInUsing())
	{
		//display info
		gif_decode_input.app_notice_display = SFOX_NotifyGifDecodeOneFrame;
	
		//get decode status
		gif_decode_input.app_is_continue_next_frame = SFOX_IsContinueDecodeGif;
	}
	
    // add by hyg 20100919
#if defined(TENCENT_APP_QQ) || defined(TENCENT_APP_BROWSER)
			if(QVM_IsQQAppDecode())
			{
				//display info
	            gif_decode_input.app_notice_display = QVM_NotifyGifDecodeOneFrame;

	            //get decode status
	            gif_decode_input.app_is_continue_next_frame = QVM_IsContinueDecodeGif;
			}
#endif
    // end of hyg 20100919
    //set read data buffer info
    SCI_MEMSET(&s_guiref_read_data_info,0,sizeof(GUIREF_IMG_READ_DATA_T));
    s_guiref_read_data_info.src_data_info.src_buf_ptr = src_data_ptr->src_buf_ptr;
    s_guiref_read_data_info.src_data_info.src_data_size = src_data_ptr->src_data_size;

    gif_result = GIFDECODEAPP_Decode(&gif_decode_input,&gif_decode_output);
    SCI_TRACE_LOW("DecodeGifBuf:gif decode result is %d,data_buffer_ptr = 0x%x!",gif_result,data_buffer_ptr);
    switch (gif_result)
    {
    case IMGPROC_GIF_APP_SUCCESS:
        result = TRUE;
        break;

    default:
        result = FALSE;
        break;
    }

    //free block buffer
    MMITHEME_FreeReadFileBuf(gif_decode_input.read_file_buf_ptr);
    gif_decode_input.read_file_buf_ptr = PNULL;
    s_guiref_img_read_block.block_ptr = PNULL;

    MMITHEME_FreeDecBuf(gif_decode_input.scalingdown_buf_ptr);
    gif_decode_input.scalingdown_buf_ptr = PNULL;
    s_guiref_img_dec_block.block_ptr = PNULL;

    MMITHEME_FreeSaveGifFrameBuf(gif_decode_input.save_pre_frame_buf_ptr);
    gif_decode_input.save_pre_frame_buf_ptr = PNULL;
    s_guiref_img_save_block.block_ptr = PNULL;
       
    *total_frame_ptr = gif_decode_output.frame_num;

    return (result);
}

/*****************************************************************************/
//  Description : decode gif image
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 
/*****************************************************************************/
LOCAL void NotifyGifDecodeOneFrame(
                                   IMGPROC_GIF_DISPLAY_INFO_T   *display_info_ptr
                                   )
{
    SCI_ASSERT(PNULL != display_info_ptr);

    //set display info
    MMITHEME_NotifyDecodeGifOneFrame(display_info_ptr->image_disaplay_width,
        display_info_ptr->image_disaplay_height,
        display_info_ptr->current_frame_number,
        display_info_ptr->delay_time);
}

/*****************************************************************************/
//  Description : for decode , encode task alloc memory
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 必须在MMI申请,否则在低优先级别task申请,MMI task中断,释放会导致内存泄漏
/*****************************************************************************/
BOOLEAN GUIREF_AllocMemory(
                           BOOLEAN     is_jpg,
                           BOOLEAN     is_need_read_buf,
                           BOOLEAN     is_need_dec_buf,
                           BOOLEAN     is_need_save_buf
                           )
{
    BOOLEAN     result = TRUE;
    BOOLEAN     is_alloc_read = FALSE;
    BOOLEAN     is_alloc_dec = FALSE;
    BOOLEAN     is_alloc_save = FALSE;
    uint8       *block_ptr = PNULL;
    uint32      block_size = 0;

    if (is_need_read_buf)
    {
        //alloc read file buffer
        block_ptr = MMITHEME_AllocReadFileBuf(&block_size);
        if (PNULL == block_ptr)
        {
            result        = FALSE;
            is_alloc_read = FALSE;
        }
        else
        {
            is_alloc_read = TRUE;
            s_guiref_img_read_block.block_ptr  = block_ptr;
            s_guiref_img_read_block.block_size = block_size;
        }
    }

    if ((result) && 
        (is_need_dec_buf))
    {
        //alloc decode buffer
        if (is_jpg)
        {
            block_ptr = MMITHEME_AllocJpgDecBuf(&block_size);
        }
        else
        {
            block_ptr = MMITHEME_AllocDecBuf(&block_size);
        }
        if (PNULL == block_ptr)
        {
            result       = FALSE;
            is_alloc_dec = FALSE;
        }
        else
        {
            is_alloc_dec = TRUE;
            s_guiref_img_dec_block.block_ptr  = block_ptr;
            s_guiref_img_dec_block.block_size = block_size;
        }
    }

    if ((result) && 
        (is_need_save_buf))
    {
        //alloc save buffer
        block_ptr = MMITHEME_AllocSaveGifFrameBuf(&block_size);
        if (PNULL == block_ptr)
        {
            result        = FALSE;
            is_alloc_save = FALSE;
        }
        else
        {
            is_alloc_save = TRUE;
            s_guiref_img_save_block.block_ptr  = block_ptr;
            s_guiref_img_save_block.block_size = block_size;
        }
    }

    if (!result)
    {
        //free buffer
        if (is_alloc_read)
        {
            MMITHEME_FreeReadFileBuf(s_guiref_img_read_block.block_ptr);
            s_guiref_img_read_block.block_ptr = PNULL;
        }

        if (is_alloc_dec)
        {
            MMITHEME_FreeDecBuf(s_guiref_img_dec_block.block_ptr);
            MMITHEME_FreeJpgDecBuf(s_guiref_img_dec_block.block_ptr);
            s_guiref_img_dec_block.block_ptr = PNULL;
        }

        if (is_alloc_save)
        {
            MMITHEME_FreeSaveGifFrameBuf(s_guiref_img_save_block.block_ptr);
            s_guiref_img_save_block.block_ptr = PNULL;
        }
    }

    return (result);
}

/*****************************************************************************/
//  Description : for decode , encode task creat file
//  Global resource dependence : 
//  Author:jassmine.meng
//  Note: 必须在MMI申请,否则在低优先级别task申请,MMI task中断,会导致文件没有关闭
/*****************************************************************************/
void GUIREF_CreatFile(
                      wchar     *full_path_ptr    //in:文件名
                      )
{
    SCI_ASSERT(PNULL != full_path_ptr);
    
    //open file
    s_sfs_handle = SFS_CreateFile(full_path_ptr,SFS_MODE_READ|SFS_MODE_OPEN_EXISTING,0,0);
}

/*****************************************************************************/
//  Description : 强行销毁Decode,Encode Task时，必须关闭相应的文件,释放相关的buffer
//  Global resource dependence : 
//  Author: jassmine
//  Note: 非强行销毁Decode,Encode Task请勿调用
/*****************************************************************************/


void GUIREF_FreeEncOrDecBuf(void)
{
    //正在读取文件时无法释放read buffer
    SCI_TRACE_LOW("GUIREF_FreeEncOrDecBuf:1 s_guiref_img_is_operate_file = %d",s_guiref_img_is_operate_file);
//	SCI_Sleep(10);
    while (s_guiref_img_is_operate_file)
    {
        SCI_Sleep(30);
    }
    SCI_TRACE_LOW("GUIREF_FreeEncOrDecBuf:2 s_guiref_img_is_operate_file = %d",s_guiref_img_is_operate_file);

	//free jpg,must free before free buffer cr140479
	IMGJPEG_FreeRes();
	
    //close file
    IMG_EnterCritical();
    CloseFile();
    IMG_LeaveCritical();

    //free file data buffer
    GUIANIM_FreeFileDataBuf();
    
    //free jpg decode/encode buffer
    MMITHEME_FreeJpgDecBuf(MMITHEME_GetJpgDecBuf());

    //free read buffer
    MMITHEME_FreeReadFileBuf(s_guiref_img_read_block.block_ptr);
    s_guiref_img_read_block.block_ptr = PNULL;
    SCI_MEMSET(&s_guiref_img_read_block,0,sizeof(GUIREF_IMG_BLOCK_T));

    //free decode buffer
    MMITHEME_FreeDecBuf(s_guiref_img_dec_block.block_ptr);
    MMITHEME_FreeJpgDecBuf(s_guiref_img_dec_block.block_ptr);
    s_guiref_img_dec_block.block_ptr = PNULL;
    SCI_MEMSET(&s_guiref_img_dec_block,0,sizeof(GUIREF_IMG_BLOCK_T));
    
    //free save buffer
    MMITHEME_FreeSaveGifFrameBuf(s_guiref_img_save_block.block_ptr);
    s_guiref_img_save_block.block_ptr = PNULL;
    SCI_MEMSET(&s_guiref_img_save_block,0,sizeof(GUIREF_IMG_BLOCK_T));


    //free buffer for ref bmp/wbmp,gif,png,jpg
#ifdef BMP_DEC_SUPPORT
    BMPAPP_Bmp_Destroy_App();
    BMPAPP_Wbmp_Destroy_App();
#endif

#ifdef GIF_DEC_SUPPORT
    GIFDECODEAPP_FreeDecodeRes();
#endif


#ifdef PNG_DEC_SUPPORT
    PNGAPP_FreeDecodeRes();

#endif
}

#ifdef __LIB_SUPPORT__
PUBLIC void LIB_NotifyGifDecodeOneFrame(IMGPROC_GIF_DISPLAY_INFO_T   *display_info_ptr){}
PUBLIC BOOLEAN LIB_IsContinueDecodeGif(void){return FALSE;}   
PUBLIC BOOLEAN LIB_DecodeGifFile(uint8*data_buffer_ptr,uint16*total_frame_ptr,uint32 data_buffer_size,uint32 dest_width,uint32 dest_height)
{ 
    BOOLEAN                             result = FALSE;
    uint32                              file_size = 0;
    IMGPROC_GIF_APP_RETURN_E            gif_result = IMGPROC_GIF_APP_SUCCESS;
    IMGPROC_GIF_DECODE_APP_INPUT_T      gif_decode_input = {0};
    IMGPROC_GIF_DECODE_APP_OUTPUT_T     gif_decode_output = {0};

    SCI_ASSERT(PNULL != data_buffer_ptr);
    SCI_ASSERT(PNULL != total_frame_ptr);
    if (0 != s_sfs_handle)                   
    {
        if(SFS_ERROR_NONE == GetImgFileSize(s_sfs_handle, &file_size))
        {
            gif_decode_input.read_file_buf_ptr = s_guiref_img_read_block.block_ptr;
            gif_decode_input.read_file_buf_size = s_guiref_img_read_block.block_size;
            gif_decode_input.scalingdown_buf_ptr = s_guiref_img_dec_block.block_ptr;
            gif_decode_input.scalingdown_buf_size = s_guiref_img_dec_block.block_size;
            gif_decode_input.target_buf_ptr = data_buffer_ptr;
            gif_decode_input.target_buf_size = data_buffer_size;
            gif_decode_input.save_pre_frame_buf_ptr = s_guiref_img_save_block.block_ptr;
            gif_decode_input.save_pre_frame_buf_size = s_guiref_img_save_block.block_size;
            gif_decode_input.target_width = (uint16)dest_width;
            gif_decode_input.target_height = (uint16)dest_height;
            gif_decode_input.target_type = RGB565_CUS;
            gif_decode_input.app_read_data = ReadFileDataSyn;
            gif_decode_input.app_notice_display = LIB_NotifyGifDecodeOneFrame;
            gif_decode_input.app_is_continue_next_frame = LIB_IsContinueDecodeGif;
            gif_result = GIFDECODEAPP_Decode(&gif_decode_input,&gif_decode_output);
            switch (gif_result)
            {
            case IMGPROC_GIF_APP_SUCCESS:
                result = TRUE;
                break;
            default:
                result = FALSE;
                break;
            }
        }
        else
        {
            result = FALSE;
        }
    }
    else
    {
        result = FALSE;
    }
    MMITHEME_FreeReadFileBuf(s_guiref_img_read_block.block_ptr);
    s_guiref_img_read_block.block_ptr = PNULL;
    gif_decode_input.read_file_buf_ptr = PNULL;
    MMITHEME_FreeDecBuf(s_guiref_img_dec_block.block_ptr);
    s_guiref_img_dec_block.block_ptr = PNULL;
    gif_decode_input.scalingdown_buf_ptr = PNULL;
    MMITHEME_FreeSaveGifFrameBuf(s_guiref_img_save_block.block_ptr);
    s_guiref_img_save_block.block_ptr = PNULL;
    gif_decode_input.save_pre_frame_buf_ptr = PNULL;
    CloseFile();
    *total_frame_ptr = gif_decode_output.frame_num;
    return (result);
}
#endif


#endif
