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

#ifndef _IMXASRC_UTILS_H
#define _IMXASRC_UTILS_H

#include <gst/audio/audio.h>
#include <stdint.h>

/* maximum buffer time in ring buffer is 1000ms */
#define MAX_RING_BUFFER_TIME 1000
/* silence time in ring buffer is 20ms */
#define SILENCE_RING_BUFFER_TIME 20
/* 20 samples size 8ch/32bit */
#define TAIL_SIZE (20 * 8 * 32)

typedef struct _ASRCAudioInfo ASRCAudioInfo;

struct _ASRCAudioInfo {
  int channels;
  int input_sample_rate;
  int output_sample_rate;
  int in_bps;
  int out_bps;
  snd_pcm_format_t input_format;
  snd_pcm_format_t output_format;
};

typedef struct ring_buffer_struct {
  int block_size;
  int num_blocks;
  int index_in;
  int index_out;
  uint8_t *mem;
} RingBuffer;

int ring_buffer_create(RingBuffer *ringbuffer, int block_size, int num_blocks);
void ring_buffer_destroy(RingBuffer *ringbuffer);
int ring_buffer_avail(RingBuffer *ringbuffer);
int ring_buffer_get(RingBuffer *ringbuffer, int num_block_out, uint8_t *data);
int ring_buffer_put(RingBuffer *ringbuffer, int num_block_in, uint8_t *data);

snd_pcm_format_t gst_to_alsa_pcm_format(GstAudioFormat fmt);
GstAudioFormat alsa_pcm_to_gst_format(snd_pcm_format_t fmt);

#endif
