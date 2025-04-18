/*
 * Copyright 2023-2025 NXP
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#ifndef __IMX_OPENCL_CONVERT_H__
#define __IMX_OPENCL_CONVERT_H__

/***************************************************************************************
* This is the head file of opencl conversion library, it provide function to convert
* video buffer format. Support both cpu virtual buffer address and cma physical buffer address.
* Supported conversion:
* NV12TILE to NV12. Including deinterlace, full range to limited range.
* NV12 to NV12. Including deinterlace, full range to limited range.
* RGBA32 to NV12. Support output buffer of BT601 and BT709, limited and full range.
* NV12 to RGB24. Support input buffer of BT601 and BT709, limited and full range.
* YUYV to RGB24. Support input buffer of BT601 and BT709, limited and full range.
* NV15TILE_TO_NV12.
* RGBA32 to RGB24.
*
* width and height should be alignment to 4 bytes when convert from RGBA32 to NV12.
* or convert from NV12/YUYV to RGB24 format.
***************************************************************************************/

#ifdef __cplusplus
extern "C"  {
#endif

#define OCL_MAX_PLANE_NUM (3)

typedef enum ocl_pixel_format{
    OCL_FORMAT_UNKNOWN = 0,
    OCL_FORMAT_RGBA8888 = 1,
    OCL_FORMAT_RGBX8888 = 2,
    OCL_FORMAT_RGB888 = 3,
    OCL_FORMAT_RGB565 = 4,
    OCL_FORMAT_BGRA8888 = 5,
    OCL_FORMAT_BGR888 = 6,
    OCL_FORMAT_BGRX8888 = 7,
    OCL_FORMAT_P010 = 10,
    OCL_FORMAT_I420 = 11,
    OCL_FORMAT_NV12 = 12,
    OCL_FORMAT_NV12_TILED = 13,
    OCL_FORMAT_YV12 = 14,
    OCL_FORMAT_NV15 = 15,
    OCL_FORMAT_NV15_TILED = 16,
    OCL_FORMAT_YUYV = 17,
    OCL_FORMAT_NV16 = 18,
}OCL_PIXEL_FORMAT;

typedef struct {
    OCL_PIXEL_FORMAT input_format;
    OCL_PIXEL_FORMAT output_format;
    /* A combination of one or more #OCL_MAP_TYPE types */
    unsigned int map_flag;
}OCL_PIXEL_FORMAT_GROUP;

typedef enum ocl_colorspace{
    OCL_COLORSPACE_DEFAULT = 0,//default: bt601_625
    OCL_COLORSPACE_BT601_625,
    OCL_COLORSPACE_BT601_525,
    OCL_COLORSPACE_BT709,
    OCL_COLORSPACE_BT2020,
}OCL_COLORSPACE;

typedef enum ocl_range{
    OCL_RANGE_DEFAULT = 0,//default: limited range
    OCL_RANGE_FULL,
    OCL_RANGE_LIMITED,
}OCL_RANGE;

typedef enum ocl_port{
    OCL_PORT_TYPE_INPUT = 0,
    OCL_PORT_TYPE_OUTPUT,
    OCL_PORT_TYPE_WARP,
    OCL_PORT_TYPE_INPUT_DUP,
    OCL_PORT_TYPE_TOTAL,
}OCL_PORT;

typedef struct ocl_format{
    OCL_PIXEL_FORMAT format;
    int width;
    int height;
    int stride;
    int sliceheight;
    int interlace;
    int left;
    int top;
    int right;
    int bottom;
    OCL_COLORSPACE colorspace;
    OCL_RANGE range;
    int alpha;
    int reserved[4];
}OCL_FORMAT;

typedef struct ocl_buffer_plane{
    int fd;
    long long vaddr;
    long long paddr;
    int size;
    int length;
    int offset;//only used when memory type is dma
    int reserved[4];
}OCL_BUFFER_PLANE;


typedef enum ocl_memory_type{
    OCL_MEM_TYPE_CPU,   //host virtual memory
    OCL_MEM_TYPE_GPU,   //physical address
    OCL_MEM_TYPE_DEVICE,//for dma buffer fd
}OCL_MEMORY_TYPE;

typedef struct ocl_buffer{
    OCL_MEMORY_TYPE mem_type;
    int plane_num;
    OCL_BUFFER_PLANE planes[OCL_MAX_PLANE_NUM];
    int reserved[4];
}OCL_BUFFER;

typedef struct ocl_run_time{
    int run_time;   //total run time
    int kernel_time;//opencl kernel run time
}OCL_RUN_TIME;

typedef enum ocl_result{
    OCL_SUCCESS = 0,
    OCL_FAIL = -1,
    OCL_NO_MEMORY = -2,
    OCL_BAD_PARAM = -3,
    OCL_INVALID_OPS = -4,
}OCL_RESULT;

typedef struct ocl_fmt_plane_info{
    OCL_FORMAT *ocl_format;//in
    int plane_num;          //out
    int plane_size[OCL_MAX_PLANE_NUM];//out
}OCL_FORMAT_PLANE_INFO;

typedef struct ocl_align_info{
    int width_align;
    int height_align;
    int size_align;
}OCL_ALIGN_INFO;

typedef void* OCL_HANDLE;

typedef enum ocl_param_index{
    OCL_PARAM_INDEX_INPUT_FORMAT = 0,
    OCL_PARAM_INDEX_OUTPUT_FORMAT,
    OCL_PARAM_INDEX_RUN_TIME,
    OCL_PARAM_INDEX_FORMAT_PLANE_INFO,
    OCL_PARAM_INDEX_ALIGN_INFO,
    OCL_PARAM_INDEX_WARP_PARAM,
}OCL_PARAM_INDEX;

typedef enum {
    OCL_WARP_MAP_NULL = 0,
    OCL_WARP_MAP_PNT,
    OCL_WARP_MAP_DPNT,
    OCL_WARP_MAP_DDPNT,
} OCL_WARP_MAP;

typedef struct {
    int enable;
    int width;
    int height;
    OCL_BUFFER buf;
    OCL_PIXEL_FORMAT in_fmt;
    OCL_WARP_MAP map;
}OCL_WARP_PARAM;

typedef struct {
    void *vaddr;
    int paddr;
    int size;
    int fd;
    void *user_data;
} OCL_MEM_BLOCK;

typedef enum {
    OCL_ALLOCATOR_NULL = 0,
    OCL_ALLOCATOR_CACHED_DMABUF,
    OCL_ALLOCATOR_UNCACHED_DMABUF,
} OCL_ALLOCATOR_TYPE;

typedef enum {
    OCL_MAP_NULL = 0,
    OCL_MAP_CSC = 1,
    OCL_MAP_CSC_DOWNSCALE = 2,
    OCL_MAP_WARP = 4,
    OCL_MAP_WARP_DOWNSCALE = 8,
}OCL_MAP_TYPE;

typedef enum {
    OCL_OPCODE_NULL = 0,
    OCL_OPCODE_CSC,
    OCL_OPCODE_WARP,
} OCL_OPCODE_TYPE;

/*
 * Function to get the opencl convert library version.
 *
 * this function did not return opencl profile level.
 * the return value is an internal string.
 *
 * @return Version string.
 */
const char* OCL_QueryVersion();

/**
 * Function to get supported input/output format
 * Before user call OCL_Open(), user can query supporeted format to see if
 * the pixel format of input or output buffer meets their requirements.
 * Then decice whether to use this library to do format conversion.
 *
 * @param port [in] set which format need to query
 * @param num_of_fmt [out] number of supported pixel formats.
 * @param fmt [out] pointer to pixel format array which contain each formats.
 * @return value in OCL_RESULT.
 */
OCL_RESULT OCL_QuerySupportFormat(OCL_PORT port, int * num_of_fmt, OCL_PIXEL_FORMAT ** fmt);

/**
 * Function to get the map of all supported convertible groups
 * Before user call OCL_Open(), user can query all the groups to see if
 * the pixel format of input and output buffer can meets their requirements.
 * Then decice whether to use this library to do format conversion.
 *
 * @param num_of_group [out] number of supported pixel format groups.
 * @param fmt_group [out] pointer to convertible group array which contain input and output formats.
 * @return value in OCL_RESULT.
 */
OCL_RESULT OCL_QuerySupportMap(int * num_of_group, OCL_PIXEL_FORMAT_GROUP ** fmt_group);

/**
 * Function to get the warp map of all supported convertible groups
 * Before user call OCL_Open(), user can query all the groups to see if
 * the pixel format of input and output buffer can meets their requirements.
 * Then decice whether to use this library to do warp operation.
 *
 * @param num_of_group [out] number of supported pixel format groups.
 * @param fmt_group [out] pointer to convertible group array which contain input and output formats.
 * @return value in OCL_RESULT.
 */
OCL_RESULT OCL_QuerySupportWarpMap(int * num_of_group, OCL_PIXEL_FORMAT_GROUP ** fmt_group);

/**
 * Function to check the specify conversion by opcode
 * The #left, #right, #top and #bottom variables in the #OCL_FORMAT
 * data structure is used to check whether the conversion is downscale
 * case or not. If any of these variables is zero, the function will
 * check both common case and downscale case. The #format variable in
 * the #OCL_FORMAT data structure is used to check video format. User
 * can check the conversion is supported or not. Then decice whether
 * to use this library to do conversion.
 *
 * @param opcode [in] value in OCL_OPCODE_TYPE.
 * @param src_fmt [in] ocl format of input.
 * @param dst_fmt [in] ocl format of output.
 * @return value in OCL_RESULT.
 */
OCL_RESULT OCL_CheckConversion (OCL_OPCODE_TYPE opcode, OCL_FORMAT *src_fmt, OCL_FORMAT *dst_fmt);

typedef enum ocl_align_flag
{
    OCL_ALIGN_FLAG_DEFAULT = 0,
    OCL_ALIGN_FLAG_DOWNSCALE = 1 << 0,
    OCL_ALIGN_FLAG_WARP = 1 << 1,
}OCL_ALIGN_FLAG;
/**
 * Function to query alignment infomation based on given input and output formats
 *
 * @param flag [in] extra flag when query the information
 * @param out [out] pointer to OCL_ALIGN_INFO array which contain alignment information.
 * @return value in OCL_RESULT.
 */
OCL_RESULT OCL_QueryAlignmentInfo(OCL_ALIGN_FLAG flag, OCL_ALIGN_INFO * out);

typedef enum ocl_open_flag
{
    OCL_OPEN_FLAG_DEFAULT = 0,
    OCL_OPEN_FLAG_PROFILE = 1 << 0,//get OCL_PARAM_INDEX_RUN_TIME to for performance envaluation
}OCL_OPEN_FLAG;
/**
 * Function to create the opencl conversion instance
 * it will create opencl context, command queue, program and return the handle
 *
 * @param flag [in] extra flag when open opencl conversion instance.
 * @param handle [out] Handle of opencl conversion instance if success, NULL for failure.
 * @return value in OCL_RESULT.
 */
OCL_RESULT OCL_Open(OCL_OPEN_FLAG flag, OCL_HANDLE * handle);

/**
 * Function to delete opencl conversion instance
 *
 * @param handle [in] handle which was created in OCL_Open().
 * @return value in OCL_RESULT.
 */
OCL_RESULT OCL_Close(OCL_HANDLE handle);

/**
 * Function to set opencl conversion parameter
 * Note: OCL_PARAM_INDEX_INPUT_FORMAT & OCL_PARAM_INDEX_OUTPUT_FORMAT must be set
 * before calling OCL_Convert().
 *
 * @param handle [in] Handle of opencl conversion.
 * @param index [in] opencl conversion parameter index.
 * @param param [in] structure pointer according to opencl conversion parameter index.
 * @return value in OCL_RESULT.
 */
OCL_RESULT OCL_SetParam(OCL_HANDLE handle, OCL_PARAM_INDEX index, void * param);

/**
 * Function to get opencl conversion parameter
 *
 *
 * @param handle [in] Handle of opencl conversion.
 * @param index [in] opencl conversion parameter index.
 * @param param [in/out] structure pointer according to opencl conversion parameter index.
 * @return value in OCL_RESULT.
 */
OCL_RESULT OCL_GetParam(OCL_HANDLE handle, OCL_PARAM_INDEX index, void * param);

/**
 * Function to use opencl convert from source buffer to destination buffer
 * This function must be called after input and output OCL_FORMAT parameter set.
 * Opencl conversion library will check the input and output buffer format
 * and read from source buffer to convert format. Destiation buffer is written
 * after the function finished.
 * This function can be called multiple times when input and output buffer formats
 * equal to previous setting. If any information of OCL_FORMAT changed in either
 * input or output buffer, user should call OCL_SetParam() before this function called.
 *
 * @param handle [in] Handle of opencl conversion.
 * @param in_buf [in] source buffer for opencl to read from for convert.
 * @param out_buf [in] destination buffer for opencl to write after conversion.
 * @return value in OCL_RESULT.
 */
OCL_RESULT OCL_Convert(OCL_HANDLE handle, OCL_BUFFER * in_buf, OCL_BUFFER * out_buf);

/**
 * Function to open the opencl allocator
 * it will open the specified allocator, create the instance and return the handle
 *
 * @param handle [out] Handle of opencl allocator instance if success, NULL for failure.
 * @param type [in] allocator memory type, vaule in OCL_ALLOCATOR_TYPE.
 * @return value in OCL_RESULT.
 */
OCL_RESULT OCL_Allocator_Open (void **handle, OCL_ALLOCATOR_TYPE type);

/**
 * Function to allocate the memory according to the specified size
 * it will create one memory block and return the address
 *
 * @param handle [in] Handle of the opencl allocator instance.
 * @param size [in] memory size.
 * @return opencl memory block address.
 */
OCL_MEM_BLOCK *OCL_Allocator_Alloc (void *handle, int size);

/**
 * Function to mmap the opencl memory block
 *
 * @param handle [in] Handle of the opencl allocator instance.
 * @param mem_blk [in] address of the opencl memory block.
 * @return value in OCL_RESULT.
 */
OCL_RESULT OCL_Allocator_Mmap (void *handle, OCL_MEM_BLOCK *mem_blk);

/**
 * Function to munmap the opencl memory block
 *
 * @param handle [in] Handle of the opencl allocator instance.
 * @param mem_blk [in] address of the opencl memory block.
 * @return value in OCL_RESULT.
 */
OCL_RESULT OCL_Allocator_Munmap (void *handle, OCL_MEM_BLOCK *mem_blk);

/**
 * Function to free the specified opencl memory
 *
 * @param handle [in] Handle of the opencl allocator instance.
 * @param mem_blk [in] address of the opencl memory block.
 * @return value in OCL_RESULT.
 */
OCL_RESULT OCL_Allocator_Free (void *handle, OCL_MEM_BLOCK *mem_blk);

/**
 * Function to close the opencl allocator
 * it will close allocator and free the handle
 *
 * @param handle [in] Handle of the opencl allocator instance.
 * @return value in OCL_RESULT.
 */
OCL_RESULT OCL_Allocator_Close (void *handle);

#ifdef __cplusplus
}
#endif

#endif//__IMX_OPENCL_CONVERT_H__
