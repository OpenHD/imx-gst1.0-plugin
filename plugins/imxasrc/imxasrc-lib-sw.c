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

#include <gst/gst.h>
#include <sys/ioctl.h>
#include <alsa/asoundlib.h>
#include <dlfcn.h>
#include "imxasrc-lib-sw.h"

#ifndef GST_DISABLE_GST_DEBUG
#define GST_CAT_DEFAULT ensure_debug_category()
static GstDebugCategory *
ensure_debug_category (void)
{
  static size_t cat_gonce = 0;

  if (g_once_init_enter (&cat_gonce)) {
    size_t cat_done;

    cat_done = (size_t) _gst_debug_category_new ("imxasrc_lib_sw", 0,
        "imxasrc_lib_sw object");

    g_once_init_leave (&cat_gonce, cat_done);
  }

  return (GstDebugCategory *) cat_gonce;
}
#else
#define ensure_debug_category() /* NOOP */
#endif /* GST_DISABLE_GST_DEBUG */

int imx_asrc_sw_open (ASRCSWConfig *asrc_sw)
{
  char *error;

  /* open dynamic library */
  if (asrc_sw->lib_type == LIB_SSRC)
    asrc_sw->library = dlopen (SSRCLIBRARYDIR, RTLD_LAZY);
  else
    asrc_sw->library = dlopen (DSPCLIBRARYDIR, RTLD_LAZY);
  error = dlerror();
  if (error != NULL) {
    GST_ERROR ("ASRCSW: dlopen error %s", error);
    return -1;
  }

  asrc_sw->asrc_sw_wrapper_create = (ASRCSWWrapperCreate)dlsym (asrc_sw->library, "ssrc_wrapper_create");
  error = dlerror();
  if (error != NULL) {
    GST_ERROR ("ASRCSW: dlsym asrc_sw_wrapper_create error %s", error);
    return -1;
  }
  asrc_sw->asrc_sw_wrapper_destroy = (ASRCSWWrapperDestroy)dlsym (asrc_sw->library, "ssrc_wrapper_destroy");
  error = dlerror();
  if (error != NULL) {
    GST_ERROR ("ASRCSW: dlsym asrc_sw_wrapper_destroy error %s", error);
    return -1;
  }
  asrc_sw->asrc_sw_wrapper_config = (ASRCSWWrapperConfig)dlsym (asrc_sw->library, "ssrc_wrapper_config");
  error = dlerror();
  if (error != NULL) {
    GST_ERROR ("ASRCSW: dlsym asrc_sw_wrapper_config error %s", error);
    return -1;
  }
  asrc_sw->asrc_sw_wrapper_process = (ASRCSWWrapperProcess)dlsym (asrc_sw->library, "ssrc_wrapper_process");
  error = dlerror();
  if (error != NULL) {
    GST_ERROR ("ASRCSW: dlsym asrc_sw_wrapper_process error %s", error);
    return -1;
  }

  asrc_sw->instance = asrc_sw->asrc_sw_wrapper_create();

  return 0;
}

void imx_asrc_sw_close (ASRCSWConfig *asrc_sw)
{
  free (asrc_sw->temp_buffer);
  asrc_sw->asrc_sw_wrapper_destroy (asrc_sw->instance);
  dlclose (asrc_sw->library);
}


int imx_asrc_sw_config (ASRCSWConfig *asrc_sw)
{
  int err;
  int block_size;
  int num_blocks;
  uint8_t *silence;
  size_t silence_size;
  ASRCSWSampleFormat format;

  /* create ring buffer and feed silence data */
  block_size = asrc_sw->audio_info.out_bps * asrc_sw->audio_info.channels;
  num_blocks = asrc_sw->audio_info.output_sample_rate * MAX_RING_BUFFER_TIME / 1000 * block_size;

  err = ring_buffer_create (&asrc_sw->ring_buffer, block_size, num_blocks);
  if (err < 0) {
    GST_ERROR ("ASRCSW: ring_buffer_create failed");
    return err;
  }

  silence_size = asrc_sw->audio_info.output_sample_rate * SILENCE_RING_BUFFER_TIME / 1000 * block_size;
  silence = malloc (silence_size);
  memset (silence, 0, silence_size);
  ring_buffer_put (&asrc_sw->ring_buffer, silence_size / block_size, silence);

  free (silence);

  /* transfer to asrc_sw sample format */
  if (asrc_sw->audio_info.input_format == SND_PCM_FORMAT_S32_LE)
    format = SampleFormat_LE_PCM_32_bits;
  else if (asrc_sw->audio_info.input_format == SND_PCM_FORMAT_FLOAT_LE)
    format = SampleFormat_LE_FLOAT_32_bits;
  else {
    GST_ERROR ("ASRCSW: unsupported format");
    return -1;
  }
  err = asrc_sw->asrc_sw_wrapper_config (&asrc_sw->instance, format, asrc_sw->audio_info.channels,
                                         asrc_sw->audio_info.input_sample_rate, asrc_sw->audio_info.output_sample_rate,
                                         asrc_sw->quality);

  if (err) {
    GST_ERROR ("ASRCSW: asrc_sw_wrapper_config error");
    return err;
  }

  asrc_sw->temp_buffer = malloc (512 * 1024);
  return 0;
}

size_t imx_asrc_sw_get_out_frames (ASRCSWConfig *asrc_sw, size_t in_frames)
{
  return ring_buffer_avail (&asrc_sw->ring_buffer);
}

int imx_asrc_sw_resample (ASRCSWConfig *asrc_sw, gpointer in[],
    size_t in_frames, gpointer out[], size_t out_frames)
{
  size_t in_chunk, out_chunk;
  uint8_t *in_p = in[0];
  uint8_t *out_p = out[0];
  int num_blocks_in;
  int ret;

  out_chunk = imx_asrc_sw_get_out_frames (asrc_sw, in_frames);
  if (out_frames < out_chunk) {
    GST_ERROR ("ASRCSW: no enough frames to get");
    return -1;
  }

  ring_buffer_get (&asrc_sw->ring_buffer, out_frames, out_p);

  in_chunk = in_frames * asrc_sw->audio_info.in_bps * asrc_sw->audio_info.channels;

  ret = asrc_sw->asrc_sw_wrapper_process (asrc_sw->instance, in_chunk, &out_chunk, in_p, asrc_sw->temp_buffer);
  if (ret) {
    GST_ERROR ("ASRCSW: asrc_sw_wrapper_process error");
    return ret;
  }

  num_blocks_in = out_chunk / asrc_sw->ring_buffer.block_size;
  ring_buffer_put (&asrc_sw->ring_buffer, num_blocks_in, asrc_sw->temp_buffer);
  GST_DEBUG ("ASRCSW: convert in %ld frames, out %d frames\n", in_frames, num_blocks_in);

  return ret;
}
