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

#include <alsa/asoundlib.h>
#include "imxasrc-utils.h"

int ring_buffer_create(RingBuffer *ringbuffer, int block_size, int num_blocks)
{
  int ret = -ENOMEM;

  ringbuffer->block_size = block_size;
  ringbuffer->num_blocks = num_blocks;
  ringbuffer->index_in = 0;
  ringbuffer->index_out = 0;
  ringbuffer->mem = malloc(block_size * num_blocks);
  if (ringbuffer->mem == NULL) {
    printf("Allocate ring buffer failed %d", ret);
    return ret;
  }

  return 0;
}

void ring_buffer_destroy(RingBuffer *ringbuffer)
{
  ringbuffer->block_size = 0;
  ringbuffer->num_blocks = 0;
  ringbuffer->index_in = 0;
  ringbuffer->index_out = 0;
  free(ringbuffer->mem);
}

int ring_buffer_avail(RingBuffer *ringbuffer)
{
  int count;

  count = ringbuffer->index_in - ringbuffer->index_out;
  if (count < 0) {
    count += ringbuffer->num_blocks;
  }

  return count;
}

int ring_buffer_get(RingBuffer *ringbuffer, int num_block_out, uint8_t *data)
{
  int ret = 0;
  int block_out1, block_out2;

  if (ringbuffer->num_blocks - ringbuffer->index_out < num_block_out) {
    block_out1 = ringbuffer->num_blocks - ringbuffer->index_out;
    block_out2 = num_block_out - block_out1;
  } else {
    block_out1 = num_block_out;
    block_out2 = 0;
  }

  memcpy(data, ringbuffer->mem + ringbuffer->index_out * ringbuffer->block_size,
         block_out1 * ringbuffer->block_size);
  ringbuffer->index_out += block_out1;
  if (ringbuffer->index_out >= ringbuffer->num_blocks)
    ringbuffer->index_out -= ringbuffer->num_blocks;

  if (block_out2) {
    memcpy(data + block_out1 * ringbuffer->block_size, ringbuffer->mem,
           block_out2 * ringbuffer->block_size);
    ringbuffer->index_out += block_out2;
    if (ringbuffer->index_out >= ringbuffer->num_blocks)
      ringbuffer->index_out -= ringbuffer->num_blocks;
  }

  return ret;
}

int ring_buffer_put(RingBuffer *ringbuffer, int num_block_in, uint8_t *data)
{
  int ret = 0;
  int block_in1, block_in2;

  if (ringbuffer->num_blocks - ringbuffer->index_in < num_block_in) {
    block_in1 = ringbuffer->num_blocks - ringbuffer->index_in;
    block_in2 = num_block_in - block_in1;
  } else {
    block_in1 = num_block_in;
    block_in2 = 0;
  }

  memcpy(ringbuffer->mem + ringbuffer->index_in * ringbuffer->block_size, data,
         block_in1 * ringbuffer->block_size);
  ringbuffer->index_in += block_in1;
  if (ringbuffer->index_in >= ringbuffer->num_blocks)
    ringbuffer->index_in -= ringbuffer->num_blocks;

  if (block_in2) {
    memcpy(ringbuffer->mem, data + block_in1 * ringbuffer->block_size,
           block_in2 * ringbuffer->block_size);
    ringbuffer->index_in += block_in2;
    if (ringbuffer->index_in >= ringbuffer->num_blocks)
      ringbuffer->index_in -= ringbuffer->num_blocks;
  }

  return ret;
}

typedef struct {
  GstAudioFormat gst_fmt;
  snd_pcm_format_t pcm_fmt;
} FormatMap;

static FormatMap format_map[] = {
    {GST_AUDIO_FORMAT_S16LE,     SND_PCM_FORMAT_S16_LE},
    {GST_AUDIO_FORMAT_U16LE,     SND_PCM_FORMAT_U16_LE},
    {GST_AUDIO_FORMAT_S20LE,     SND_PCM_FORMAT_S20_3LE},
    {GST_AUDIO_FORMAT_U20LE,     SND_PCM_FORMAT_U20_3LE},
    {GST_AUDIO_FORMAT_S24LE,     SND_PCM_FORMAT_S24_3LE},
    {GST_AUDIO_FORMAT_U24LE,     SND_PCM_FORMAT_U24_3LE},
    {GST_AUDIO_FORMAT_S24_32LE,  SND_PCM_FORMAT_S24_LE},
    {GST_AUDIO_FORMAT_U24_32LE,  SND_PCM_FORMAT_U24_LE},
    {GST_AUDIO_FORMAT_S32LE,     SND_PCM_FORMAT_S32_LE},
    {GST_AUDIO_FORMAT_U32LE,     SND_PCM_FORMAT_U32_LE},
    {GST_AUDIO_FORMAT_F32LE,     SND_PCM_FORMAT_FLOAT_LE},
    {GST_AUDIO_FORMAT_UNKNOWN,   SND_PCM_FORMAT_UNKNOWN}
};

snd_pcm_format_t gst_to_alsa_pcm_format(GstAudioFormat fmt)
{
  int i;
  for (i = 0; i < sizeof(format_map) / sizeof(FormatMap); i++) {
    if (format_map[i].gst_fmt == fmt)
      return format_map[i].pcm_fmt;
  }

  return SND_PCM_FORMAT_UNKNOWN;
}

GstAudioFormat alsa_pcm_to_gst_format(snd_pcm_format_t fmt)
{
  int i;
  for (i = 0; i < sizeof(format_map) / sizeof(FormatMap); i++) {
    if (format_map[i].pcm_fmt == fmt)
      return format_map[i].gst_fmt;
  }

  return GST_AUDIO_FORMAT_UNKNOWN;
}
