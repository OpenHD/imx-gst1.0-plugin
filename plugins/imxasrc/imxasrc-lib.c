/* GStreamer
 * Copyright 2024 NXP
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
#include "imxasrc-lib.h"
#include <linux/mxc_asrc.h>

#define ASRC_DEVICE_NAME ("/dev/mxc_asrc")
#define ASRC_DMA_BUF_SIZE (4096 * 4)
/* maximum buffer time in ring buffer is 200ms */
#define MAX_RING_BUFFER_TIME 200
/* silence time in ring buffer is 20ms */
#define SILENCE_RING_BUFFER_TIME 20
/* 20 samples size 8ch/32bit */
#define TAIL_SIZE (20 * 8 * 32)

#ifndef GST_DISABLE_GST_DEBUG
#define GST_CAT_DEFAULT ensure_debug_category()
static GstDebugCategory *
ensure_debug_category (void)
{
  static size_t cat_gonce = 0;

  if (g_once_init_enter (&cat_gonce)) {
    size_t cat_done;

    cat_done = (size_t) _gst_debug_category_new ("imxasrc_lib", 0,
        "imxasrc_lib object");

    g_once_init_leave (&cat_gonce, cat_done);
  }

  return (GstDebugCategory *) cat_gonce;
}
#else
#define ensure_debug_category() /* NOOP */
#endif /* GST_DISABLE_GST_DEBUG */

/*
 * Function: get the output length of asrc by giving the input data length
 * , input rate and output rate.
 * The returned length is not accurate, but the method used in this way can
 * work well with DMA used in asrc.
 * To make sure the output length is enought to store the target audio data,
 * a tail will be added for output buffer of storing the whole audio data and
 * each asrc converting data.
 * input_format and output_format is added since the new ASRC in i.mx8MN has
 * the resample function like convert 16 bit input to 24 bit output.
 */
int imx_asrc_get_output_buffer_size(int input_buffer_size,
        int input_sample_rate, int output_sample_rate,
        snd_pcm_format_t input_format,
        snd_pcm_format_t output_format)
{
  int i = 0;
  int outbuffer_size = 0;
  int outsample = output_sample_rate;

  while (outsample >= input_sample_rate)
  {
    ++i;
    outsample -= input_sample_rate;
  }
  outbuffer_size = i * input_buffer_size;
  i = 1;
  while (((input_buffer_size >> i) > 2) && (outsample != 0))
  {
    if (((outsample << 1) - input_sample_rate) >= 0)
    {
      outsample = (outsample << 1) - input_sample_rate;
      outbuffer_size += (input_buffer_size >> i);
    }
    else
    {
      outsample = outsample << 1;
    }
    i++;
  }
  outbuffer_size = (outbuffer_size >> 3) << 3;
  outbuffer_size = outbuffer_size * snd_pcm_format_physical_width(output_format) / snd_pcm_format_physical_width(input_format);

  return outbuffer_size;
}

int imx_asrc_open(ASRCConfig *asrc)
{
  asrc->fd = open(ASRC_DEVICE_NAME, O_RDWR);

  if (asrc->fd < 0)
  {
    GST_ERROR ("failed to open ASRC");
    return -1;
  }

  return 0;
}

void imx_asrc_close(ASRCConfig *asrc)
{
  close(asrc->fd);
  ioctl(asrc->fd, ASRC_RELEASE_PAIR, asrc->pair_index);

  ring_buffer_destroy(&asrc->ring_buffer);

  return;
}

/* Request one available ASRC CONTEXT/PAIR then configure it */
int imx_asrc_config(ASRCConfig *asrc)
{
  int err = 0;
  int block_size;
  int num_blocks;
  struct asrc_req req;
  struct asrc_config config;
  uint8_t *silence;
  size_t silence_size;

  if (!asrc)
  {
    GST_ERROR ("asrc is invalid");
    return -1;
  }

  asrc->input_dma_size = ASRC_DMA_BUF_SIZE;
  asrc->output_dma_size = imx_asrc_get_output_buffer_size(
                            asrc->input_dma_size,
                            asrc->audio_info.input_sample_rate,
                            asrc->audio_info.output_sample_rate,
                            asrc->audio_info.input_format,
                            asrc->audio_info.output_format);

  req.chn_num = asrc->audio_info.channels;
  /* load the requested asrc PAIR information to req */
  err = ioctl(asrc->fd, ASRC_REQ_PAIR, &req);
  if (err < 0)
  {
    GST_ERROR ("Req ASRC pair FAILED");
    return err;
  }
  if (req.index == 0)
    GST_DEBUG("Pair A requested");
  else if (req.index == 1)
    GST_DEBUG("Pair B requested");
  else if (req.index == 2)
    GST_DEBUG("Pair C requested");
  else if (req.index == 3)
    GST_DEBUG("Pair D requested");

  /* get the supported formats for requested PAIR */
  asrc->supported_in_format = req.supported_in_format;
  asrc->supported_out_format = req.supported_out_format;

  config.pair = req.index;
  config.channel_num = req.chn_num;
  config.dma_buffer_size = asrc->input_dma_size;
  config.input_sample_rate = asrc->audio_info.input_sample_rate;
  config.output_sample_rate = asrc->audio_info.output_sample_rate;
  config.input_format = asrc->audio_info.input_format;
  config.output_format = asrc->audio_info.output_format;
  config.inclk = INCLK_NONE;
  config.outclk = OUTCLK_ASRCK1_CLK;
  asrc->pair_index = req.index;
  err = ioctl(asrc->fd, ASRC_CONFIG_PAIR, &config);
  if (err < 0)
  {
    GST_ERROR ("ioctl ASRC_CONFIG_PAIR failed");
    return err;
  }

  /* create ring buffer and feed silence data */
  block_size = asrc->out_bps * asrc->audio_info.channels;
  num_blocks = asrc->audio_info.output_sample_rate * MAX_RING_BUFFER_TIME / 1000 * block_size;

  err = ring_buffer_create(&asrc->ring_buffer, block_size, num_blocks);
  if (err < 0) {
    GST_ERROR ("ring_buffer_create failed");
    return err;
  }

  silence_size = asrc->audio_info.output_sample_rate * SILENCE_RING_BUFFER_TIME / 1000 * block_size;
  silence = malloc(silence_size);
  memset(silence, 0, silence_size);
  ring_buffer_put(&asrc->ring_buffer, silence_size / block_size, silence);

  free(silence);

  return err;
}

int imx_asrc_start(ASRCConfig *asrc)
{
  int err;
  err = ioctl(asrc->fd, ASRC_START_CONV, &asrc->pair_index);
  if (err < 0)
  {
    GST_ERROR ("ioctl ASRC_START_CONV failed");
    return err;
  }

  return err;
}

size_t imx_asrc_get_out_len(ASRCConfig *asrc_config, size_t in_len)
{
  return imx_asrc_get_output_buffer_size(in_len,
              asrc_config->audio_info.input_sample_rate,
              asrc_config->audio_info.output_sample_rate,
              get_alsa_pcm_format(asrc_config->audio_info.input_format),
              get_alsa_pcm_format(asrc_config->audio_info.output_format));
}

size_t imx_asrc_get_out_frames(ASRCConfig *asrc_config, size_t in_frames)
{
  return ring_buffer_avail(&asrc_config->ring_buffer);
}

int imx_asrc_resample(ASRCConfig *asrc_config, gpointer in[],
    size_t in_frames, gpointer out[], size_t out_frames)
{
  struct asrc_convert_buffer asrc_buf;
  size_t in_chunk, out_chunk;
  uint8_t *in_p = in[0];
  uint8_t *out_p = out[0];
  uint8_t *temp_buffer;
  int err;
  int num_blocks_in;

  out_chunk = imx_asrc_get_out_frames(asrc_config, in_frames);
  if (out_frames < out_chunk) {
    GST_ERROR ("no enough frames to get");
    return -1;
  }

  ring_buffer_get(&asrc_config->ring_buffer, out_frames, out_p);

  in_chunk = in_frames * asrc_config->in_bps * asrc_config->audio_info.channels;
  out_chunk = imx_asrc_get_out_len(asrc_config, in_chunk);
  /* increase tail size to make sure temp buffer has enough space for ASRC output */
  temp_buffer = malloc(out_chunk + TAIL_SIZE);

  asrc_buf.input_buffer_length = in_chunk;
  asrc_buf.input_buffer_vaddr = in_p;
  asrc_buf.output_buffer_length = out_chunk + TAIL_SIZE;
  asrc_buf.output_buffer_vaddr = temp_buffer;
  err = ioctl(asrc_config->fd, ASRC_CONVERT, &asrc_buf);
  if (err < 0)
  {
    GST_ERROR ("ioctl ASRC_CONVERT failed");
    return err;
  }

  num_blocks_in = asrc_buf.output_buffer_length / asrc_config->ring_buffer.block_size;
  ring_buffer_put(&asrc_config->ring_buffer, num_blocks_in, temp_buffer);
  GST_DEBUG("asrc convert: in %ld frames, out %d frames\n", in_frames, num_blocks_in);

  free(temp_buffer);
  return err;
}
