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

#ifndef __GST_IMXASRC_RESAMPLER_H__
#define __GST_IMXASRC_RESAMPLER_H__

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include "imxasrc-lib.h"

G_BEGIN_DECLS

typedef struct _GstImxASRCResampler GstImxASRCResampler;

typedef int (*ResampleFunc) (ASRCConfig * asrc, gpointer in[],
    gsize in_frames, gpointer out[], gsize out_frames);

struct _GstImxASRCResampler
{
  GstAudioFormat in_format;
  GstAudioFormat out_format;
  GstStructure *options;
  gint format_index;
  gint channels;
  gint in_rate;
  gint out_rate;


  gint in_bps;
  gint out_bps;
  gint ostride;

  gint n_taps;
  ResampleFunc resample;

  gint blocks;
  gint inc;
  gint samp_inc;
  gint samp_frac;
  gint samp_index;
  gint samp_phase;
  gint skip;

  gpointer samples;
  gsize samples_len;
  gsize samples_avail;
  gpointer *sbuf;
  ASRCConfig *asrc_config;
};

GST_AUDIO_API
gsize gst_imxasrc_resampler_get_out_frames (GstImxASRCResampler *resampler,
                                            gsize in_frames);

GST_AUDIO_API
gsize gst_imxasrc_resampler_get_in_frames (GstImxASRCResampler *resampler,
                                           gsize out_frames);

GST_AUDIO_API
gsize gst_imxasrc_resampler_get_max_latency (GstImxASRCResampler *resampler);

GST_AUDIO_API
void gst_imxasrc_resampler_reset (GstImxASRCResampler * resampler);

GST_AUDIO_API
gboolean gst_imxasrc_resampler_update (GstImxASRCResampler * resampler,
                                       gint in_rate, gint out_rate,
                                       GstStructure * options);

GST_AUDIO_API
GstImxASRCResampler *gst_imxasrc_resampler_new (GstAudioResamplerMethod method,
                                                GstAudioResamplerFlags flags,
                                                GstAudioFormat format, gint channels,
                                                gint in_rate, gint out_rate,
                                                GstStructure * options);

GST_AUDIO_API
void gst_imxasrc_resampler_free (GstImxASRCResampler * resampler);

GST_AUDIO_API
void gst_imxasrc_resampler_resample (GstImxASRCResampler * resampler,
                                     gpointer in[], gsize in_frames,
                                     gpointer out[], gsize out_frames);

G_END_DECLS

#endif /* __GST_IMXASRC_RESAMPLER_H__ */
