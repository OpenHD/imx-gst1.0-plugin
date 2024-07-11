/* GStreamer IMX Video 2D device
 * Copyright (c) 2014-2015, Freescale Semiconductor, Inc. All rights reserved.
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "imx_2d_device.h"

GST_DEBUG_CATEGORY (imx2ddevice_debug);
#define GST_CAT_DEFAULT imx2ddevice_debug

#ifdef USE_IPU
extern Imx2DDevice * imx_ipu_create(Imx2DDeviceType  device_type);
extern gint imx_ipu_destroy(Imx2DDevice *device);
extern gboolean imx_ipu_is_exist (void);
#endif

#ifdef USE_G2D
extern Imx2DDevice * imx_g2d_create(Imx2DDeviceType  device_type);
extern gint imx_g2d_destroy(Imx2DDevice *device);
extern gboolean imx_g2d_is_exist (void);
#endif

#ifdef USE_PXP
extern Imx2DDevice * imx_pxp_create(Imx2DDeviceType  device_type);
extern gint imx_pxp_destroy(Imx2DDevice *device);
extern gboolean imx_pxp_is_exist (void);
#endif

#ifdef USE_OCL
extern Imx2DDevice * imx_ocl_create(Imx2DDeviceType  device_type);
extern gint imx_ocl_destroy(Imx2DDevice *device);
extern gboolean imx_ocl_is_exist (void);
#endif

static const Imx2DDeviceInfo Imx2DDevices[] = {
#ifdef USE_IPU
    { .name                     ="ipu",
      .device_type              =IMX_2D_DEVICE_IPU,
      .create                   =imx_ipu_create,
      .destroy                  =imx_ipu_destroy,
      .is_exist                 =imx_ipu_is_exist
    },
#endif

#ifdef USE_G2D
    { .name                     ="g2d",
      .device_type              =IMX_2D_DEVICE_G2D,
      .create                   =imx_g2d_create,
      .destroy                  =imx_g2d_destroy,
      .is_exist                 =imx_g2d_is_exist
    },
#endif

#ifdef USE_PXP
    { .name                     ="pxp",
      .device_type              =IMX_2D_DEVICE_PXP,
      .create                   =imx_pxp_create,
      .destroy                  =imx_pxp_destroy,
      .is_exist                 =imx_pxp_is_exist
    },
#endif

#ifdef USE_OCL
    { .name                     ="ocl",
      .device_type              =IMX_2D_DEVICE_OCL,
      .create                   =imx_ocl_create,
      .destroy                  =imx_ocl_destroy,
      .is_exist                 =imx_ocl_is_exist
    },
#endif
    {
      NULL
    }
};

const Imx2DDeviceInfo * imx_get_2d_devices(void)
{
  static gint debug_init = 0;
  if (debug_init == 0) {
    GST_DEBUG_CATEGORY_INIT (imx2ddevice_debug, "imx2ddevice", 0,
                           "Freescale IMX 2D Devices");
    debug_init = 1;
  }

  return &Imx2DDevices[0];
}

Imx2DDevice * imx_2d_device_create(Imx2DDeviceType  device_type)
{
  const Imx2DDeviceInfo *dev_info = imx_get_2d_devices();
  while (dev_info->name) {
    if (dev_info->device_type == device_type) {
      if (dev_info->is_exist()) {
        return dev_info->create(device_type);
      } else {
        GST_ERROR("device %s not exist", dev_info->name);
        return NULL;
      }
    }
    dev_info++;
  }

  GST_ERROR("Unknown 2D device type %d\n", device_type);
  return NULL;
}

gint imx_2d_device_destroy(Imx2DDevice *device)
{
  if (!device)
    return -1;

  const Imx2DDeviceInfo *dev_info = imx_get_2d_devices();
  while (dev_info->name) {
    if (dev_info->device_type == device->device_type)
      return dev_info->destroy(device);
    dev_info++;
  }

  GST_ERROR("Unknown 2D device type %d\n", device->device_type);
  return -1;
}

GstVideoFormat imx_g2d_device_get_fixed_format (GstCaps * caps)
{
  gint i, caps_size;
  GstStructure *st;
  const GValue *format;
  const gchar *fmt_name;
  GstVideoFormat out_fmt = GST_VIDEO_FORMAT_UNKNOWN;

  caps_size = gst_caps_get_size (caps);
  for (i = 0; i < caps_size; i++) {
    st = gst_caps_get_structure(caps, i);

    if (!g_strcmp0 (gst_structure_get_string (st, "format"), "DMA_DRM")) {
      format = gst_structure_get_value (st, "drm-format");
    } else {
      format = gst_structure_get_value (st, "format");
    }

    /* Check the selected caps if it has the fixed format */
    if (GST_VALUE_HOLDS_LIST (format)) {
      if (gst_value_list_get_size (format) == 1) {
        const GValue *val;
        val = gst_value_list_get_value (format, 0);
        if (!G_VALUE_HOLDS_STRING (val)) {
          out_fmt = GST_VIDEO_FORMAT_UNKNOWN;
          GST_TRACE ("No valid format in the list");
          break;
        }
        /* Has the fixed format and get it below */
        format = val;
      } else {
        out_fmt = GST_VIDEO_FORMAT_UNKNOWN;
        GST_TRACE ("No fixed format in the list");
        break;
      }
    }

    /* Get the fixed format in the selected caps */
    if (G_VALUE_HOLDS_STRING (format)) {
      fmt_name = g_value_get_string (format);

      if (out_fmt == GST_VIDEO_FORMAT_UNKNOWN) {
        /* Record the first fixed format */
        out_fmt = gst_video_format_from_string(fmt_name);
      } else if (out_fmt != gst_video_format_from_string(fmt_name)) {
        out_fmt = GST_VIDEO_FORMAT_UNKNOWN;
        GST_TRACE ("No fixed format in the caps");
        break;
      }
    }
  }

  return out_fmt;
}

static gboolean imx_2d_device_probe_warp_header (Imx2DDevice *device,
     FILE *fp, gsize file_size, Imx2DVideoWarp *video_warp)
{
  guint32 header_size = 0;
  guint8 file_version = 0;
  guint32 width,height;
  GstBuffer *gstbuf;
  GstMapInfo map;
  guint32 map_data_size = 0;
  Imx2DVidoWarpArbitrary *p_arb_info = NULL;
  gboolean ret = FALSE;

  #define WARP_HEADER_WIDTH           4
  #define WARP_FILE_VERSION_WIDTH     1
  #define WARP_ALOGITHMS_OFFSET       5
  #define WARP_WIDTH_OFFSET           8
  #define WARP_HEIGHT_OFFSET          12
  #define WARP_ARB_START_X_OFFSET     16
  #define WARP_ARB_START_Y_OFFSET     20
  #define WARP_ARB_DELTA_XX_OFFSET    24
  #define WARP_ARB_DELTA_XY_OFFSET    28
  #define WARP_ARB_DELTA_YX_OFFSET    32
  #define WARP_ARB_DELTA_YY_OFFSET    36
  #define WARP_VERSION_1_HEADER_SZIE  40

  if (!device || !fp || !file_size || !video_warp ||
      file_size < WARP_HEADER_WIDTH) {
    goto exit;
  }

  /* Check header size */
  if (fread(&header_size, 1, WARP_HEADER_WIDTH,
      fp) != WARP_HEADER_WIDTH) {
    GST_DEBUG ("Can't read header size");
    goto exit;
  }

  if (!header_size || file_size < header_size) {
    goto exit;
  }

  if (fread(&file_version, 1, WARP_FILE_VERSION_WIDTH,
      fp) != WARP_FILE_VERSION_WIDTH) {
    GST_DEBUG ("Can't read file format version");
    goto exit;
  } else {
    if (file_version == 1) {
      /* The header size is fixed for file version 1 */
      if (header_size != WARP_VERSION_1_HEADER_SZIE) {
        goto exit;
      }
    } else {
      if (header_size < WARP_VERSION_1_HEADER_SZIE) {
        goto exit;
      }
    }
  }

  gstbuf = gst_buffer_new_and_alloc (header_size);
  gst_buffer_map (gstbuf, &map, GST_MAP_WRITE);
  if (fread(map.data + WARP_ALOGITHMS_OFFSET, 1,
      header_size - WARP_ALOGITHMS_OFFSET, fp) !=
      header_size - WARP_ALOGITHMS_OFFSET) {
    GST_DEBUG ("Can't read header data");
    goto done;
  }

  switch (map.data[WARP_ALOGITHMS_OFFSET]) {
    case IMX_2D_WARP_PNT_32BPP:
      video_warp->map_format = IMX_2D_WARP_MAP_PNT;
      video_warp->bpp = 32;
      ret = TRUE;
      break;
    case IMX_2D_WARP_DPNT_32BPP:
      video_warp->map_format = IMX_2D_WARP_MAP_DPNT;
      video_warp->bpp = 32;
      ret = TRUE;
      break;
    case IMX_2D_WARP_DPNT_16BPP:
      video_warp->map_format = IMX_2D_WARP_MAP_DPNT;
      video_warp->bpp = 16;
      ret = TRUE;
      break;
    case IMX_2D_WARP_DPNT_8BPP:
      video_warp->map_format = IMX_2D_WARP_MAP_DPNT;
      video_warp->bpp = 8;
      ret = TRUE;
      break;
    case IMX_2D_WARP_DDPNT_32BPP:
      video_warp->map_format = IMX_2D_WARP_MAP_DDPNT;
      video_warp->bpp = 32;
      ret = TRUE;
      break;
    case IMX_2D_WARP_DDPNT_16BPP:
      video_warp->map_format = IMX_2D_WARP_MAP_DDPNT;
      video_warp->bpp = 16;
      ret = TRUE;
      break;
    case IMX_2D_WARP_DDPNT_8BPP:
      video_warp->map_format = IMX_2D_WARP_MAP_DDPNT;
      video_warp->bpp = 8;
      ret = TRUE;
      break;
    case IMX_2D_WARP_DDPNT_4BPP:
      video_warp->map_format = IMX_2D_WARP_MAP_DDPNT;
      video_warp->bpp = 4;
      ret = TRUE;
      break;
    default:
      GST_ERROR ("Invalid algorithms type");
      break;
  }

  /* Check file integrity */
  width = GST_READ_UINT32_LE(map.data + WARP_WIDTH_OFFSET);
  height = GST_READ_UINT32_LE(map.data + WARP_HEIGHT_OFFSET);
  map_data_size = width * height * video_warp->bpp / 8;
  if (!ret ||
      ((map_data_size + header_size) != (guint32)file_size)) {
    GST_DEBUG ("Invalid header data");
    goto done;
  }

  video_warp->width = width;
  video_warp->height = height;
  p_arb_info = &video_warp->arb_info;
  switch (video_warp->map_format) {
    case IMX_2D_WARP_MAP_PNT:
      p_arb_info->arb_start_x = 0;
      p_arb_info->arb_start_y = 0;
      ret = TRUE;
      break;
    case IMX_2D_WARP_MAP_DPNT:
      p_arb_info->arb_start_x = GST_READ_UINT32_LE(map.data + WARP_ARB_START_X_OFFSET);
      p_arb_info->arb_start_y = GST_READ_UINT32_LE(map.data + WARP_ARB_START_Y_OFFSET);
      video_warp->arb_num = 2;
      ret = TRUE;
      break;
    case IMX_2D_WARP_MAP_DDPNT:
      p_arb_info->arb_start_x = GST_READ_UINT32_LE(map.data + WARP_ARB_START_X_OFFSET);
      p_arb_info->arb_start_y = GST_READ_UINT32_LE(map.data + WARP_ARB_START_Y_OFFSET);
      p_arb_info->arb_delta_xx = GST_READ_UINT32_LE(map.data + WARP_ARB_DELTA_XX_OFFSET);
      p_arb_info->arb_delta_xy = GST_READ_UINT32_LE(map.data + WARP_ARB_DELTA_XY_OFFSET);
      p_arb_info->arb_delta_yx = GST_READ_UINT32_LE(map.data + WARP_ARB_DELTA_YX_OFFSET);
      p_arb_info->arb_delta_yy = GST_READ_UINT32_LE(map.data + WARP_ARB_DELTA_YY_OFFSET);
      video_warp->arb_num = 6;
      ret = TRUE;
      break;
    default:
      GST_ERROR ("Invalid warp map format");
      ret = FALSE;
      break;
  }

done:
  gst_buffer_unmap (gstbuf, &map);
  gst_buffer_unref (gstbuf);

exit:
  if (ret) {
    GST_DEBUG ("Get header data");
    video_warp->coordinates_size = file_size - header_size;
  } else {
    GST_DEBUG ("No header size info");
    video_warp->coordinates_size = file_size;
  }

  return ret;
}

gboolean imx_2d_device_read_warp_coordinates_file (Imx2DDevice *device,
    const char* file_name, Imx2DVideoWarp *video_warp)
{
  FILE *fp;
  gint ret = 0;
  gsize size = 0;
  PhyMemBlock *mem_blk;
  gsize w_aligned, h_aligned, size_aligned;

  if (!device || !file_name || !video_warp)
    return FALSE;

  mem_blk = &video_warp->coordinates_mem;

  do {
    fp = fopen (file_name, "rb");
    if (fp == NULL) {
      ret = -1;
      GST_DEBUG("Can't open file, file name: %s", file_name);
      break;
    }

    ret = fseek(fp, 0, SEEK_END);
    if (ret) {
      break;
    }

    size = ftell(fp);
    if (size == 0) {
      ret = -1;
      break;
    }

    ret = fseek(fp, 0, SEEK_SET);
    if (ret) {
      break;
    }

    /* Probe header data*/
    if (!imx_2d_device_probe_warp_header (device, fp, size, video_warp)) {
      ret = fseek(fp, 0, SEEK_SET);
      if (ret) {
        break;
      }
    }
    size = video_warp->coordinates_size;

    w_aligned = ALIGNTO (video_warp->width, ALIGNMENT);
    h_aligned = ALIGNTO (video_warp->height, ALIGNMENT);
    size_aligned = w_aligned * h_aligned * video_warp->bpp / 8;
    size_aligned = PAGE_ALIGN (size_aligned);

    if (mem_blk->size) {
      device->free_mem (device, mem_blk);
    }
    mem_blk->size = (size > size_aligned) ? size: size_aligned;
    if (device->alloc_mem (device, mem_blk)) {
      ret = -1;
      break;
    }

    if(fread(mem_blk->vaddr, 1, size, fp) != size) {
      ret = -1;
      break;
    } else {
      ret = 0;
    }
  } while (0);

  if (fp)
    fclose(fp);

  if (ret) {
    if (mem_blk->vaddr)
      device->free_mem (device, mem_blk);
    GST_DEBUG("read file failed: %s", file_name);
    video_warp->coordinates_size = 0;
    return FALSE;
  } else {
    return TRUE;
  }
}

void imx_2d_device_set_warp_controls (const GstStructure * config,
    Imx2DVideoWarp *video_warp)
{
  g_return_if_fail (config != NULL);
  g_return_if_fail (video_warp != NULL);

  if (gst_structure_has_field(config, "map-format")) {
    gst_structure_get(config, "map-format",
        G_TYPE_INT, &video_warp->map_format, NULL);
  }

  if (gst_structure_has_field(config, "width")) {
    gst_structure_get(config, "width",
        G_TYPE_INT, &video_warp->width, NULL);
  }

  if (gst_structure_has_field(config, "height")) {
    gst_structure_get(config, "height",
        G_TYPE_INT, &video_warp->height, NULL);
  }

  if (gst_structure_has_field(config, "bpp")) {
    gst_structure_get(config, "bpp",
        G_TYPE_INT, &video_warp->bpp, NULL);
  }

  if (gst_structure_has_field(config, "arb_start_x")) {
    gst_structure_get(config, "arb_start_x",
        G_TYPE_INT, &video_warp->arb_info.arb_start_x, NULL);
    video_warp->arb_num++;
  }

  if (gst_structure_has_field(config, "arb_start_y")) {
    gst_structure_get(config, "arb_start_y",
        G_TYPE_INT, &video_warp->arb_info.arb_start_y, NULL);
    video_warp->arb_num++;
  }

  if (gst_structure_has_field(config, "arb_delta_xx")) {
    gst_structure_get(config, "arb_delta_xx",
        G_TYPE_INT, &video_warp->arb_info.arb_delta_xx, NULL);
    video_warp->arb_num++;
  }

  if (gst_structure_has_field(config, "arb_delta_xy")) {
    gst_structure_get(config, "arb_delta_xy",
        G_TYPE_INT, &video_warp->arb_info.arb_delta_xy, NULL);
    video_warp->arb_num++;
  }

  if (gst_structure_has_field(config, "arb_delta_yx")) {
    gst_structure_get(config, "arb_delta_yx",
        G_TYPE_INT, &video_warp->arb_info.arb_delta_yx, NULL);
    video_warp->arb_num++;
  }

  if (gst_structure_has_field(config, "arb_delta_yy")) {
    gst_structure_get(config, "arb_delta_yy",
        G_TYPE_INT, &video_warp->arb_info.arb_delta_yy, NULL);
    video_warp->arb_num++;
  }
}