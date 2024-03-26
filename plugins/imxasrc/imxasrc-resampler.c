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

#include <string.h>
#include <stdio.h>
#include <math.h>
#include "imxasrc-resampler.h"
#include "imxasrc-lib.h"

GST_DEBUG_CATEGORY_STATIC (imxasrc_resampler_debug);
#define GST_CAT_DEFAULT imxasrc_resampler_debug

static void
gst_imxasrc_resampler_asrc_init (ASRCConfig *asrc)
{
  static gsize init_gonce = 0;

  if (g_once_init_enter (&init_gonce)) {

    GST_DEBUG_CATEGORY_INIT (imxasrc_resampler_debug, "imxasrc-resampler", 0,
        "imxasrc-resampler object");

    g_once_init_leave (&init_gonce, 1);
  }

  imx_asrc_open(asrc);
}

static gint
gst_imxasrc_resampler_asrc_configure (ASRCConfig *asrc, ASRCAudioInfo info)
{
  gint ret;
  asrc->audio_info = info;

  /* initialize ASRCConfig */
  ret = imx_asrc_config(asrc);
  if (ret) {
    GST_ERROR ("gst_imxasrc_resampler_asrc_configure failed");
    return -1;
  }

  return ret;
}

static gint
gst_imxasrc_resampler_asrc_start (ASRCConfig *asrc)
{
  gint ret;

  ret = imx_asrc_start(asrc);
  if (ret) {
    GST_ERROR ("gst_imxasrc_resampler_asrc_start failed");
    return -1;
  }

  return ret;
}

static gint
gst_imxasrc_resampler_asrc_resample (ASRCConfig * asrc, gpointer in[],
    gsize in_frames, gpointer out[], gsize out_frames)
{
  gint ret;

  ret = imx_asrc_resample (asrc, in, in_frames, out, out_frames);
  if (ret) {
    GST_ERROR ("gst_imxasrc_resampler_asrc_resample failed");
    return -1;
  }

  return ret;
}

/**
 * gst_imxasrc_resampler_get_out_frames:
 * @resampler: a #GstImxASRCResampler
 * @in_frames: number of input frames
 *
 * Get the number of output frames that would be currently available when
 * @in_frames are given to @resampler.
 *
 * Returns: The number of frames that would be available after giving
 * @in_frames as input to @resampler.
 */

gsize
gst_imxasrc_resampler_get_out_frames (GstImxASRCResampler *resampler,
                                      gsize in_frames)
{
  gsize out;

  GST_DEBUG("gst_imx_asrc_resampler_get_out_frames");
  g_return_val_if_fail (resampler != NULL, 0);

  if (in_frames)
    out = imx_asrc_get_out_frames(resampler->asrc_config, in_frames);
  else
    return 0;

  return out;
}

/**
 * gst_audio_resampler_reset:
 * @resampler: a #GstImxASRCResampler
 *
 * Reset @resampler to the state it was when it was first created, discarding
 * all sample history.
 */
void
gst_imxasrc_resampler_reset (GstImxASRCResampler * resampler)
{
  g_return_if_fail (resampler != NULL);

  if (resampler->samples) {
    gsize bytes;
    gint c, blocks, bpf;

    bpf = resampler->in_bps * resampler->inc;
    bytes = (resampler->n_taps / 2) * bpf;
    blocks = resampler->blocks;

    for (c = 0; c < blocks; c++)
      memset (resampler->sbuf[c], 0, bytes);
  }
  /* half of the filter is filled with 0 */
  resampler->samp_index = 0;
  resampler->samples_avail = resampler->n_taps / 2 - 1;
}

/**
 * gst_audio_resampler_update:
 * @resampler: a #GstImxASRCResampler
 * @in_rate: new input rate
 * @out_rate: new output rate
 * @options: new options or %NULL
 *
 * Update the resampler parameters for @resampler. This function should
 * not be called concurrently with any other function on @resampler.
 *
 * When @in_rate or @out_rate is 0, its value is unchanged.
 *
 * When @options is %NULL, the previously configured options are reused.
 *
 * Returns: %TRUE if the new parameters could be set
 */
gboolean
gst_imxasrc_resampler_update (GstImxASRCResampler * resampler,
    gint in_rate, gint out_rate, GstStructure * options)
{
  gint gcd, samp_phase;
  gint ret;

  g_return_val_if_fail (resampler != NULL, FALSE);

  if (in_rate <= 0)
    in_rate = resampler->in_rate;
  if (out_rate <= 0)
    out_rate = resampler->out_rate;

  if (resampler->out_rate > 0) {
    GST_INFO ("old phase %d/%d", resampler->samp_phase, resampler->out_rate);
    samp_phase =
        gst_util_uint64_scale_int (resampler->samp_phase, out_rate,
        resampler->out_rate);
  } else
    samp_phase = 0;

  gcd = gst_util_greatest_common_divisor (in_rate, out_rate);

  GST_INFO ("phase %d out_rate %d, in_rate %d, gcd %d", samp_phase, out_rate,
      in_rate, gcd);

  resampler->samp_phase = samp_phase /= gcd;
  resampler->in_rate = in_rate / gcd;
  resampler->out_rate = out_rate / gcd;

  GST_INFO ("new phase %d/%d", resampler->samp_phase, resampler->out_rate);

  resampler->samp_inc = in_rate / out_rate;
  resampler->samp_frac = in_rate % out_rate;

  resampler->resample = gst_imxasrc_resampler_asrc_resample;

  GST_DEBUG ("in_rate %d, out_rate %d, %d, %d", in_rate, out_rate, resampler->in_rate, resampler->out_rate);

  /* Update ASRCConfig settings */
  ASRCAudioInfo audio_info;
  audio_info.channels = resampler->channels;
  audio_info.input_sample_rate = in_rate;
  audio_info.output_sample_rate = out_rate;

  audio_info.input_format = get_alsa_pcm_format(resampler->in_format);
  audio_info.output_format = get_alsa_pcm_format(resampler->out_format);

  resampler->asrc_config->in_bps = resampler->in_bps;
  resampler->asrc_config->out_bps = resampler->out_bps;

  ret = gst_imxasrc_resampler_asrc_configure (resampler->asrc_config, audio_info);
  if (ret < 0) {
    GST_ERROR ("gst_imxasrc_resampler_asrc_configure failed");
    return FALSE;
  }
  ret = gst_imxasrc_resampler_asrc_start (resampler->asrc_config);
  if (ret < 0) {
    GST_ERROR ("gst_imxasrc_resampler_asrc_start failed");
    return FALSE;
  }

  return TRUE;
}

/**
 * gst_audio_resampler_new:
 * @method: a #GstAudioResamplerMethod
 * @flags: #GstAudioResamplerFlags
 * @format: the #GstAudioFormat
 * @channels: the number of channels
 * @in_rate: input rate
 * @out_rate: output rate
 * @options: extra options
 *
 * Make a new resampler.
 *
 * Returns: (skip) (transfer full): The new #GstAudioResampler.
 */
GstImxASRCResampler *
gst_imxasrc_resampler_new (GstAudioResamplerMethod method,
    GstAudioResamplerFlags flags,
    GstAudioFormat format, gint channels,
    gint in_rate, gint out_rate, GstStructure * options)
{
  GstImxASRCResampler *resampler;
  const GstAudioFormatInfo *info;

  g_return_val_if_fail (
      format == GST_AUDIO_FORMAT_S16LE || format == GST_AUDIO_FORMAT_U16LE ||
      format == GST_AUDIO_FORMAT_S24_32LE || format == GST_AUDIO_FORMAT_S24LE ||
      format == GST_AUDIO_FORMAT_U24LE || format == GST_AUDIO_FORMAT_U24LE ||
      format == GST_AUDIO_FORMAT_S32LE || format == GST_AUDIO_FORMAT_U32LE ||
      format == GST_AUDIO_FORMAT_S20LE || format == GST_AUDIO_FORMAT_S20LE ||
      format == GST_AUDIO_FORMAT_F32LE, NULL);
  g_return_val_if_fail (channels > 0, NULL);
  g_return_val_if_fail (in_rate > 0, NULL);
  g_return_val_if_fail (out_rate > 0, NULL);

  resampler = g_slice_new0 (GstImxASRCResampler);

  resampler->in_format = format;
  resampler->out_format = format;
  resampler->channels = channels;

  info = gst_audio_format_get_info (format);
  resampler->in_bps = GST_AUDIO_FORMAT_INFO_WIDTH (info) / 8;
  resampler->out_bps = resampler->in_bps;
  resampler->sbuf = g_malloc0 (sizeof (gpointer) * channels);

  resampler->asrc_config = g_slice_new0 (ASRCConfig);

  gst_imxasrc_resampler_asrc_init (resampler->asrc_config);

  if (!gst_imxasrc_resampler_update (resampler, in_rate, out_rate, NULL)) {
    GST_ERROR ("gst_imxasrc_resampler_update failed");
    g_slice_free (ASRCConfig, resampler->asrc_config);
    g_slice_free (GstImxASRCResampler, resampler);
    return NULL;
  }

  gst_imxasrc_resampler_reset (resampler);

  return resampler;
}

/**
 * gst_imxasrc_resampler_free:
 * @resampler: a #GstAudioResampler
 *
 * Free a previously allocated #GstAudioResampler @resampler.
 */
void
gst_imxasrc_resampler_free (GstImxASRCResampler * resampler)
{
  g_return_if_fail (resampler != NULL);

  imx_asrc_close(resampler->asrc_config);
  g_free (resampler->samples);
  g_free (resampler->sbuf);
  if (resampler->options)
    gst_structure_free (resampler->options);
  g_slice_free (ASRCConfig, resampler->asrc_config);
  g_slice_free (GstImxASRCResampler, resampler);
}

/**
 * gst_imxasrc_resampler_resample:
 * @resampler: a #GstImxASRCResampler
 * @in: input samples
 * @in_frames: number of input frames
 * @out: output samples
 * @out_frames: number of output frames
 *
 * Perform resampling on @in_frames frames in @in and write @out_frames to @out.
 *
 * In case the samples are interleaved, @in and @out must point to an
 * array with a single element pointing to a block of interleaved samples.
 *
 * If non-interleaved samples are used, @in and @out must point to an
 * array with pointers to memory blocks, one for each channel.
 *
 * @in may be %NULL, in which case @in_frames of silence samples are pushed
 * into the resampler.
 *
 * This function always produces @out_frames of output and consumes @in_frames of
 * input. Use gst_imxasrc_resampler_get_out_frames() and
 * gst_imxasrc_resampler_get_in_frames() to make sure @in_frames and @out_frames
 * are matching and @in and @out point to enough memory.
 */
void
gst_imxasrc_resampler_resample (GstImxASRCResampler * resampler,
    gpointer in[], gsize in_frames, gpointer out[], gsize out_frames)
{

  resampler->resample (resampler->asrc_config, in, in_frames, out, out_frames);

  return;
}
