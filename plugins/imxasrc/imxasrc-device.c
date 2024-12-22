/* GStreamer
 * Copyright 2025 NXP
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
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */

#include <unistd.h>
#include "imxasrc-device.h"

GST_DEBUG_CATEGORY (imxasrcdevice_debug);
#define GST_CAT_DEFAULT imxasrcdevice_debug

extern GList *imx_asrc_hw_get_supported_fmts (void);
extern GList *imx_asrc_hw_get_supported_rates (void);
extern gint imx_asrc_hw_get_min_channels (void);
extern gint imx_asrc_hw_get_max_channels (void);
extern gboolean imx_asrc_hw_is_exist (void);

gboolean imx_asrc_ssrc_is_exist (void)
{
  if (!access (SSRCLIBRARYDIR, F_OK))
    return TRUE;

  return FALSE;
}

gboolean imx_asrc_dspc_is_exist (void)
{
  if (!access (DSPCLIBRARYDIR, F_OK))
    return TRUE;

  return FALSE;
}

static GstAudioFormat imx_asrc_sw_supported_fmts[] = {
  GST_AUDIO_FORMAT_S32LE,
};

static gint imx_asrc_ssrc_supported_rates[] = {
  8000, 11025, 16000, 22050, 32000, 44100, 48000, 64000, 88200, 96000,
  176400, 192000, 352800, 384000,
};

static gint imx_asrc_dspc_supported_rates[] = {
  44100, 48000, 88200, 96000, 176400, 192000, 352800, 384000,
};

GList *imx_asrc_ssrc_get_supported_rates (void)
{
  GList* list = NULL;
  int i;

  i = sizeof(imx_asrc_ssrc_supported_rates) / sizeof(gint);
  for (i = i - 1; i >= 0 ; i--) {
    list = g_list_append (list, GINT_TO_POINTER(imx_asrc_ssrc_supported_rates[i]));
  }

  return list;
}

GList *imx_asrc_dspc_get_supported_rates (void)
{
  GList* list = NULL;
  int i;

  i = sizeof(imx_asrc_dspc_supported_rates) / sizeof(gint);
  for (i = i - 1; i >= 0; i--) {
    list = g_list_append (list, GINT_TO_POINTER(imx_asrc_dspc_supported_rates[i]));
  }

  return list;
}

GList *imx_asrc_sw_get_supported_fmts (void)
{
  GList* list = NULL;
  int i;
  for (i = 0; i < sizeof(imx_asrc_sw_supported_fmts) / sizeof(GstAudioFormat); i++) {
    list = g_list_append (list, GINT_TO_POINTER(imx_asrc_sw_supported_fmts[i]));
  }

  return list;
}

gint imx_asrc_sw_get_min_channels (void)
{
  return 1;
}

gint imx_asrc_sw_get_max_channels (void)
{
  return 2;
}

static const ImxASRCDeviceInfo ImxASRCDevices[] = {
    {
      .name                        = "hw",
      .method                      = GST_IMXASRC_METHOD_HW,
      .is_exist                    = imx_asrc_hw_is_exist,
      .get_supported_fmts          = imx_asrc_hw_get_supported_fmts,
      .get_supported_rates         = imx_asrc_hw_get_supported_rates,
      .get_min_channels            = imx_asrc_hw_get_min_channels,
      .get_max_channels            = imx_asrc_hw_get_max_channels,
    },

    {
      .name                        = "ssrc",
      .method                      = GST_IMXASRC_METHOD_SSRC,
      .is_exist                    = imx_asrc_ssrc_is_exist,
      .get_supported_fmts          = imx_asrc_sw_get_supported_fmts,
      .get_supported_rates         = imx_asrc_ssrc_get_supported_rates,
      .get_min_channels            = imx_asrc_sw_get_min_channels,
      .get_max_channels            = imx_asrc_sw_get_max_channels,
    },

    {
      .name                        = "dspc",
      .method                      = GST_IMXASRC_METHOD_DSPC,
      .is_exist                    = imx_asrc_dspc_is_exist,
      .get_supported_fmts          = imx_asrc_sw_get_supported_fmts,
      .get_supported_rates         = imx_asrc_dspc_get_supported_rates,
      .get_min_channels            = imx_asrc_sw_get_min_channels,
      .get_max_channels            = imx_asrc_sw_get_max_channels,
    },

    {
      NULL
    }
};

const ImxASRCDeviceInfo *imx_get_asrc_devices (void)
{
  static gint debug_init = 0;
  if (debug_init == 0) {
    GST_DEBUG_CATEGORY_INIT (imxasrcdevice_debug, "imxasrcdevice", 0,
                             "IMX ASRC Devices");
    debug_init = 1;
  }

  return ImxASRCDevices;
}