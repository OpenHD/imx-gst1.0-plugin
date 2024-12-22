/* GStreamer
 * Copyright 2024-2025 NXP
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

#ifndef __IMXASRC_LIB_HW_H__
#define __IMXASRC_LIB_HW_H__

#include <sound/compress_offload.h>
#include "imxasrc-utils.h"

typedef struct _ASRCHWConfig ASRCHWConfig;

struct _ASRCHWConfig {
  int fd;

  ASRCAudioInfo audio_info;

  int input_dma_size;
  int output_dma_size;

  void *bufin_start;
  void *bufout_start;
  struct snd_compr_task task;
  struct snd_compr_task_status status;

  RingBuffer ring_buffer;
};

int imx_asrc_hw_open (ASRCHWConfig *asrc_hw);
void imx_asrc_hw_close (ASRCHWConfig *asrc_hw);
int imx_asrc_hw_config (ASRCHWConfig *asrc_hw);
int imx_asrc_hw_resample (ASRCHWConfig *asrc_hw, gpointer in[],
                         size_t in_bytes, gpointer out[], size_t out_bytes);
size_t imx_asrc_hw_get_out_frames (ASRCHWConfig *asrc_hw, size_t in_frames);
GList *imx_asrc_hw_get_supported_fmts (void);
GList *imx_asrc_hw_get_supported_rates (void);
gint imx_asrc_hw_get_min_channels (void);
gint imx_asrc_hw_get_max_channels (void);
gboolean imx_asrc_hw_is_exist (void);

#endif
