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

#ifndef __IMXASRC_LIB_SW_H__
#define __IMXASRC_LIB_SW_H__

#include "imxasrc-utils.h"
#include "imxasrc-device.h"

typedef enum
{
    SampleFormat_LE_8_bits,
    SampleFormat_LE_PCM_16_bits,
    SampleFormat_LE_PCM_24_bits,
    SampleFormat_LE_PCM_32_bits,
    SampleFormat_LE_PCM_64_bits,
    SampleFormat_LE_FLOAT_32_bits,
    SampleFormat_LE_FLOAT_64_bits,
    SampleFormat_BE_PCM_16_bits,
    SampleFormat_BE_PCM_24_bits,
    SampleFormat_BE_PCM_32_bits,
    SampleFormat_BE_PCM_64_bits,
    SampleFormat_BE_FLOAT_32_bits,
    SampleFormat_BE_FLOAT_64_bits
} ASRCSWSampleFormat;

typedef void* (* ASRCSWWrapperCreate) (void);

typedef void (* ASRCSWWrapperDestroy) (void *instance);

typedef int (* ASRCSWWrapperConfig) (void **instance, ASRCSWSampleFormat sampleFormat, int channels, int input_rate, int output_rate, IMXASRCResamplerQuality quality);

typedef int (* ASRCSWWrapperProcess) (void *instance, size_t in_chunk, size_t *out_chunk, uint8_t *in_p, uint8_t *out_p);

typedef struct _ASRCSWCConfig ASRCSWConfig;

struct _ASRCSWCConfig {
  ASRCAudioInfo audio_info;
  RingBuffer ring_buffer;

  void *library;
  void *instance;
  uint8_t *temp_buffer;
  ASRCSWWrapperCreate asrc_sw_wrapper_create;
  ASRCSWWrapperDestroy asrc_sw_wrapper_destroy;
  ASRCSWWrapperConfig asrc_sw_wrapper_config;
  ASRCSWWrapperProcess asrc_sw_wrapper_process;

  ASRCSWLibType lib_type;
  IMXASRCResamplerQuality quality;
};

int imx_asrc_sw_open (ASRCSWConfig *asrc_sw);
void imx_asrc_sw_close (ASRCSWConfig *asrc_sw);
int imx_asrc_sw_config (ASRCSWConfig *asrc_sw);
int imx_asrc_sw_resample (ASRCSWConfig *asrc_sw, gpointer in[],
                      size_t in_bytes, gpointer out[], size_t out_bytes);
size_t imx_asrc_sw_get_out_frames (ASRCSWConfig *asrc_sw, size_t in_frames);

#endif