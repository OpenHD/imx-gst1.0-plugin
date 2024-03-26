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
/**
 * SECTION:element-gstimxasrc
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 filesrc location=audio8k32b2c_sle.pcm ! audio/x-raw, rate=8000,
 * format=S32LE, channels=2, layout=interleaved ! imxasrc ! audio/x-raw, rate=16000,
 * format=S32LE, channels=2, layout=interleaved  ! filesink location=audio16k32b2c_sle.pcm
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/gstutils.h>
#include <gst/audio/audio.h>
#include <gst/base/gstbasetransform.h>
#include "gstimxasrc.h"
#include "gstimxcommon.h"

GST_DEBUG_CATEGORY_STATIC (gst_imxasrc_debug_category);
#define GST_CAT_DEFAULT gst_imxasrc_debug_category

/* prototypes */


static void gst_imxasrc_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_imxasrc_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_imxasrc_dispose (GObject * object);
static void gst_imxasrc_finalize (GObject * object);

static GstCaps *gst_imxasrc_transform_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, GstCaps * filter);
static GstCaps *gst_imxasrc_fixate_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, GstCaps * othercaps);
static gboolean gst_imxasrc_set_caps (GstBaseTransform * trans,
    GstCaps * incaps, GstCaps * outcaps);
static gboolean gst_imxasrc_transform_size (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, gsize size, GstCaps * othercaps,
    gsize * othersize);
static gboolean gst_imxasrc_start (GstBaseTransform * trans);
static gboolean gst_imxasrc_stop (GstBaseTransform * trans);
static gboolean gst_imxasrc_sink_event (GstBaseTransform * trans,
    GstEvent * event);
static gboolean gst_imxasrc_src_event (GstBaseTransform * trans,
    GstEvent * event);
static GstFlowReturn gst_imxasrc_transform (GstBaseTransform * trans,
    GstBuffer * inbuf, GstBuffer * outbuf);

enum
{
  PROP_0
};

/* pad templates */
#define GST_IMXASRC_FORMATS_SRC "{ " \
    "S16LE, U16LE, S24_32LE, S24LE, " \
    "U24_32LE, U24LE, S32LE, U32LE, " \
    "S20LE, U20LE, F32LE }"

#define GST_IMXASRC_FORMATS_SINK "{ " \
    "S16LE, U16LE, S24_32LE, S24LE, " \
    "U24_32LE, U24LE, S32LE, U32LE, " \
    "S20LE, U20LE, F32LE }"

#define SUPPORTED_CAPS_SRC \
  GST_AUDIO_CAPS_MAKE (GST_IMXASRC_FORMATS_SRC) \
  ", layout = (string) { interleaved, non-interleaved }"

#define SUPPORTED_CAPS_SINK \
  GST_AUDIO_CAPS_MAKE (GST_IMXASRC_FORMATS_SINK) \
  ", layout = (string) { interleaved, non-interleaved }"

static GstStaticPadTemplate gst_imxasrc_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (SUPPORTED_CAPS_SRC)
    );

static GstStaticPadTemplate gst_imxasrc_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (SUPPORTED_CAPS_SINK)
    );


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstImxASRC, gst_imxasrc, GST_TYPE_BASE_TRANSFORM,
  GST_DEBUG_CATEGORY_INIT (gst_imxasrc_debug_category, "imxasrc", 0,
  "debug category for imxasrc element"));

static void
gst_imxasrc_class_init (GstImxASRCClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS(klass),
      &gst_imxasrc_src_template);
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS(klass),
      &gst_imxasrc_sink_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS(klass),
      "i.MX ASRC", "Filter/Converter/Audio", "Resamples audio",
      "Chancel Liu <chancel.liu@nxp.com>");

  gobject_class->set_property = gst_imxasrc_set_property;
  gobject_class->get_property = gst_imxasrc_get_property;
  gobject_class->dispose = gst_imxasrc_dispose;
  gobject_class->finalize = gst_imxasrc_finalize;
  base_transform_class->transform_caps = GST_DEBUG_FUNCPTR (gst_imxasrc_transform_caps);
  base_transform_class->fixate_caps = GST_DEBUG_FUNCPTR (gst_imxasrc_fixate_caps);
  base_transform_class->set_caps = GST_DEBUG_FUNCPTR (gst_imxasrc_set_caps);
  base_transform_class->transform_size = GST_DEBUG_FUNCPTR (gst_imxasrc_transform_size);
  base_transform_class->start = GST_DEBUG_FUNCPTR (gst_imxasrc_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR (gst_imxasrc_stop);
  base_transform_class->sink_event = GST_DEBUG_FUNCPTR (gst_imxasrc_sink_event);
  base_transform_class->src_event = GST_DEBUG_FUNCPTR (gst_imxasrc_src_event);
  base_transform_class->transform = GST_DEBUG_FUNCPTR (gst_imxasrc_transform);
  base_transform_class->passthrough_on_same_caps = TRUE;

}

static void
gst_imxasrc_init (GstImxASRC *imxasrc)
{

}

void
gst_imxasrc_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstImxASRC *imxasrc = GST_IMXASRC (object);

  GST_DEBUG_OBJECT (imxasrc, "set_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_imxasrc_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstImxASRC *imxasrc = GST_IMXASRC (object);

  GST_DEBUG_OBJECT (imxasrc, "get_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_imxasrc_dispose (GObject * object)
{
  GstImxASRC *imxasrc = GST_IMXASRC (object);

  GST_DEBUG_OBJECT (imxasrc, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_imxasrc_parent_class)->dispose (object);
}

void
gst_imxasrc_finalize (GObject * object)
{
  GstImxASRC *imxasrc = GST_IMXASRC (object);

  GST_DEBUG_OBJECT (imxasrc, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_imxasrc_parent_class)->finalize (object);
}

static GstCaps *
gst_imxasrc_transform_caps (GstBaseTransform * trans, GstPadDirection direction,
    GstCaps * caps, GstCaps * filter)
{
  GstImxASRC *imxasrc = GST_IMXASRC (trans);
  const GValue *val;
  GstStructure *s;
  GstCaps *res;
  gint i, n;

  GST_DEBUG_OBJECT (imxasrc, "transform_caps");

  /* transform single caps into input_caps + input_caps with the rate
   * field set to our supported range. This ensures that upstream knows
   * about downstream's preferred rate(s) and can negotiate accordingly. */
  res = gst_caps_new_empty ();
  n = gst_caps_get_size (caps);
  for (i = 0; i < n; i++) {
    s = gst_caps_get_structure (caps, i);

    /* If this is already expressed by the existing caps
     * skip this structure */
    if (i > 0 && gst_caps_is_subset_structure (res, s))
      continue;

    /* first, however, check if the caps contain a range for the rate field, in
     * which case that side isn't going to care much about the exact sample rate
     * chosen and we should just assume things will get fixated to something sane
     * and we may just as well offer our full range instead of the range in the
     * caps. If the rate is not an int range value, it's likely to express a
     * real preference or limitation and we should maintain that structure as
     * preference by putting it first into the transformed caps, and only add
     * our full rate range as second option  */
    s = gst_structure_copy (s);
    val = gst_structure_get_value (s, "rate");
    if (val == NULL || GST_VALUE_HOLDS_INT_RANGE (val)) {
      /* overwrite existing range, or add field if it doesn't exist yet */
      gst_structure_set (s, "rate", GST_TYPE_INT_RANGE, 1, G_MAXINT, NULL);
    } else {
      /* append caps with full range to existing caps with non-range rate field */
      gst_caps_append_structure (res, gst_structure_copy (s));
      gst_structure_set (s, "rate", GST_TYPE_INT_RANGE, 1, G_MAXINT, NULL);
    }
    gst_caps_append_structure (res, s);
  }

  if (filter) {
    GstCaps *intersection;

    intersection =
        gst_caps_intersect_full (filter, res, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (res);
    res = intersection;
  }

  return res;
}

static GstCaps *
gst_imxasrc_fixate_caps (GstBaseTransform * trans, GstPadDirection direction,
    GstCaps * caps, GstCaps * othercaps)
{
  GstImxASRC *imxasrc = GST_IMXASRC (trans);
  GstStructure *s;
  gint rate;

  GST_DEBUG_OBJECT (imxasrc, "fixate_caps");

  s = gst_caps_get_structure (caps, 0);
  if (G_UNLIKELY (!gst_structure_get_int (s, "rate", &rate)))
    return othercaps;

  othercaps = gst_caps_truncate (othercaps);
  othercaps = gst_caps_make_writable (othercaps);
  s = gst_caps_get_structure (othercaps, 0);
  gst_structure_fixate_field_nearest_int (s, "rate", rate);

  return gst_caps_fixate (othercaps);
}

static void
gst_imxasrc_reset_state (GstImxASRC * imxasrc)
{
  if (imxasrc->is_hw_resample && imxasrc->hw_converter)
    gst_imxasrc_converter_reset (imxasrc->hw_converter);
  else if (imxasrc->sw_converter)
    gst_audio_converter_reset (imxasrc->sw_converter);
}

static gboolean
gst_imxasrc_update_state (GstImxASRC * imxasrc, GstAudioInfo * in,
    GstAudioInfo * out)
{
  GstStructure *options = NULL;

  if ((imxasrc->hw_converter == NULL || imxasrc->sw_converter == NULL) && in == NULL && out == NULL)
    return TRUE;

  if (in != NULL && (in->finfo != imxasrc->in.finfo ||
          in->channels != imxasrc->in.channels ||
          in->layout != imxasrc->in.layout)) {
    if (imxasrc->is_hw_resample && imxasrc->hw_converter) {
      gst_imxasrc_converter_free (imxasrc->hw_converter);
      imxasrc->hw_converter = NULL;
    }
    else if (imxasrc->sw_converter) {
      gst_audio_converter_free (imxasrc->sw_converter);
      imxasrc->sw_converter = NULL;
    }
  }
  if (imxasrc->is_hw_resample && imxasrc->hw_converter == NULL) {
    imxasrc->hw_converter = gst_imxasrc_converter_new (0,
                                                       in, out, options);
    if (imxasrc->hw_converter == NULL)
      goto resampler_failed;
  }
  else if (imxasrc->sw_converter == NULL) {
    imxasrc->sw_converter = gst_audio_converter_new (GST_AUDIO_CONVERTER_FLAG_VARIABLE_RATE,
                                                     in, out, options);
    if (imxasrc->sw_converter == NULL)
      goto resampler_failed;
  } else if (in && out) {
    gboolean ret;

    if (imxasrc->is_hw_resample)
      ret =
        gst_imxasrc_converter_update_config (imxasrc->hw_converter, in->rate,
                                             out->rate, options);
    else
      ret =
        gst_audio_converter_update_config (imxasrc->sw_converter, in->rate,
                                           out->rate, options);
    if (!ret)
      goto update_failed;
  } else {
    gst_structure_free (options);
  }

  return TRUE;

  /* ERRORS */
resampler_failed:
  {
    GST_ERROR_OBJECT (imxasrc, "failed to create resampler");
    return FALSE;
  }
update_failed:
  {
    GST_ERROR_OBJECT (imxasrc, "failed to update resampler");
    return FALSE;
  }
}

static gboolean
gst_imxasrc_set_caps (GstBaseTransform * trans, GstCaps * incaps,
    GstCaps * outcaps)
{
  GstImxASRC *imxasrc = GST_IMXASRC (trans);
  GstAudioInfo in, out;

  GST_LOG ("incaps %" GST_PTR_FORMAT ", outcaps %"
      GST_PTR_FORMAT, incaps, outcaps);

  if (!gst_audio_info_from_caps (&in, incaps))
    goto invalid_incaps;
  if (!gst_audio_info_from_caps (&out, outcaps))
    goto invalid_outcaps;

  if (in.rate / out.rate > 16 || out.rate / in.rate > 16) {
    GST_ERROR_OBJECT (trans, "sample rate conversion ratio should not exceed 16");
    return FALSE;
  }

  /* Reset timestamp tracking and drain the resampler if the audio format is
   * changing. Especially when changing the sample rate our timestamp tracking
   * will be completely off, but even otherwise we would usually lose the last
   * few samples if we don't drain here */
  if (!gst_audio_info_is_equal (&in, &imxasrc->in) ||
      !gst_audio_info_is_equal (&out, &imxasrc->out)) {

    gst_imxasrc_reset_state (imxasrc);
    imxasrc->num_gap_samples = 0;
    imxasrc->num_nongap_samples = 0;
    imxasrc->t0 = GST_CLOCK_TIME_NONE;
    imxasrc->in_offset0 = GST_BUFFER_OFFSET_NONE;
    imxasrc->out_offset0 = GST_BUFFER_OFFSET_NONE;
    imxasrc->samples_in = 0;
    imxasrc->samples_out = 0;
    imxasrc->need_discont = TRUE;
  /* TODO sw and hw select */
    imxasrc->is_hw_resample = TRUE;
  }

  if (!gst_imxasrc_update_state (imxasrc, &in, &out)) {
    GST_ERROR_OBJECT (trans, "gst_imxasrc_update_state failed");
    return FALSE;
  }

  imxasrc->in = in;
  imxasrc->out = out;

  return TRUE;

  /* ERROR */
invalid_incaps:
  {
    GST_ERROR_OBJECT (trans, "invalid incaps");
    return FALSE;
  }
invalid_outcaps:
  {
    GST_ERROR_OBJECT (trans, "invalid outcaps");
    return FALSE;
  }
}

/* transform size */
static gboolean
gst_imxasrc_transform_size (GstBaseTransform * trans, GstPadDirection direction,
    GstCaps * caps, gsize size, GstCaps * othercaps, gsize * othersize)
{
  GstImxASRC *imxasrc = GST_IMXASRC (trans);
  gboolean ret = TRUE;
  gint bpf;

  GST_LOG_OBJECT (trans, "asked to transform size %" G_GSIZE_FORMAT
      " in direction %s", size, direction == GST_PAD_SINK ? "SINK" : "SRC");

  /* Number of samples in either buffer is size / (width*channels) ->
   * calculate the factor */
  bpf = GST_AUDIO_INFO_BPF (&imxasrc->in);

  /* Convert source buffer size to samples */
  size /= bpf;

  if (direction == GST_PAD_SINK) {
    /* asked to convert size of an incoming buffer */
    if (imxasrc->is_hw_resample)
      *othersize = gst_imxasrc_converter_get_out_frames (imxasrc->hw_converter, size);
    else
      *othersize = gst_audio_converter_get_out_frames (imxasrc->sw_converter, size);
    *othersize *= bpf;
  } else {
    /* asked to convert size of an outgoing buffer */
    if (imxasrc->is_hw_resample)
      *othersize = 0;
    else
      *othersize = gst_audio_converter_get_in_frames (imxasrc->sw_converter, size);
    *othersize *= bpf;
  }

  GST_LOG_OBJECT (trans,
      "transformed size %" G_GSIZE_FORMAT " to %" G_GSIZE_FORMAT,
      size * bpf, *othersize);

  return ret;
}

/* states */
static gboolean
gst_imxasrc_start (GstBaseTransform * trans)
{
  GstImxASRC *imxasrc = GST_IMXASRC (trans);

  GST_DEBUG_OBJECT (imxasrc, "start");

  return TRUE;
}

static gboolean
gst_imxasrc_stop (GstBaseTransform * trans)
{
  GstImxASRC *imxasrc = GST_IMXASRC (trans);

  GST_DEBUG_OBJECT (imxasrc, "stop");

  return TRUE;
}

/* sink and src pad event handlers */
static gboolean
gst_imxasrc_sink_event (GstBaseTransform * trans, GstEvent * event)
{
  GstImxASRC *imxasrc = GST_IMXASRC (trans);

  GST_DEBUG_OBJECT (imxasrc, "sink_event");

  return GST_BASE_TRANSFORM_CLASS (gst_imxasrc_parent_class)->sink_event (
      trans, event);
}

static gboolean
gst_imxasrc_src_event (GstBaseTransform * trans, GstEvent * event)
{
  GstImxASRC *imxasrc = GST_IMXASRC (trans);

  GST_DEBUG_OBJECT (imxasrc, "src_event");

  return GST_BASE_TRANSFORM_CLASS (gst_imxasrc_parent_class)->src_event (
      trans, event);
}

static GstFlowReturn
gst_imxasrc_process (GstImxASRC * resample, GstBuffer * inbuf,
    GstBuffer * outbuf)
{
  GstAudioBuffer srcabuf, dstabuf;
  gsize outsize;
  gsize in_len;
  gsize out_len;
  gboolean inbuf_writable;

  inbuf_writable = gst_buffer_is_writable (inbuf)
      && gst_buffer_n_memory (inbuf) == 1
      && gst_memory_is_writable (gst_buffer_peek_memory (inbuf, 0));

  gst_audio_buffer_map (&srcabuf, &resample->in, inbuf,
      inbuf_writable ? GST_MAP_READWRITE : GST_MAP_READ);

  in_len = srcabuf.n_samples;
  if (resample->is_hw_resample)
    out_len = gst_imxasrc_converter_get_out_frames (resample->hw_converter, in_len);
  else
    out_len = gst_audio_converter_get_out_frames (resample->sw_converter, in_len);

  GST_DEBUG_OBJECT (resample, "in %" G_GSIZE_FORMAT " frames, out %"
      G_GSIZE_FORMAT " frames", in_len, out_len);

  /* ensure that the output buffer is not bigger than what we need */
  gst_buffer_set_size (outbuf, out_len * resample->in.bpf);

  if (GST_AUDIO_INFO_LAYOUT (&resample->out) ==
      GST_AUDIO_LAYOUT_NON_INTERLEAVED) {
    gst_buffer_add_audio_meta (outbuf, &resample->out, out_len, NULL);
  }

  gst_audio_buffer_map (&dstabuf, &resample->out, outbuf, GST_MAP_WRITE);

  if (resample->is_hw_resample)
    gst_imxasrc_converter_samples (resample->hw_converter, 0, srcabuf.planes,
      in_len, dstabuf.planes, out_len);
  else
    gst_audio_converter_samples (resample->sw_converter, 0, srcabuf.planes,
      in_len, dstabuf.planes, out_len);

  /* time */
  if (GST_CLOCK_TIME_IS_VALID (resample->t0)) {
    GST_BUFFER_TIMESTAMP (outbuf) = resample->t0 +
        gst_util_uint64_scale_int_round (resample->samples_out, GST_SECOND,
        resample->out.rate);
    GST_BUFFER_DURATION (outbuf) = resample->t0 +
        gst_util_uint64_scale_int_round (resample->samples_out + out_len,
        GST_SECOND, resample->out.rate) - GST_BUFFER_TIMESTAMP (outbuf);
  } else {
    GST_BUFFER_TIMESTAMP (outbuf) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_DURATION (outbuf) = GST_CLOCK_TIME_NONE;
  }
  /* offset */
  if (resample->out_offset0 != GST_BUFFER_OFFSET_NONE) {
    GST_BUFFER_OFFSET (outbuf) = resample->out_offset0 + resample->samples_out;
    GST_BUFFER_OFFSET_END (outbuf) = GST_BUFFER_OFFSET (outbuf) + out_len;
  } else {
    GST_BUFFER_OFFSET (outbuf) = GST_BUFFER_OFFSET_NONE;
    GST_BUFFER_OFFSET_END (outbuf) = GST_BUFFER_OFFSET_NONE;
  }
  /* move along */
  resample->samples_out += out_len;
  resample->samples_in += in_len;

  gst_audio_buffer_unmap (&srcabuf);
  gst_audio_buffer_unmap (&dstabuf);

  outsize = out_len * resample->in.bpf;

  GST_LOG_OBJECT (resample,
      "Converted to buffer of %" G_GSIZE_FORMAT
      " samples (%" G_GSIZE_FORMAT " bytes) with timestamp %" GST_TIME_FORMAT
      ", duration %" GST_TIME_FORMAT ", offset %" G_GUINT64_FORMAT
      ", offset_end %" G_GUINT64_FORMAT, out_len, outsize,
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (outbuf)),
      GST_TIME_ARGS (GST_BUFFER_DURATION (outbuf)),
      GST_BUFFER_OFFSET (outbuf), GST_BUFFER_OFFSET_END (outbuf));

  if (outsize == 0)
    return GST_BASE_TRANSFORM_FLOW_DROPPED;
  else
    return GST_FLOW_OK;
}

/* transform */
static GstFlowReturn
gst_imxasrc_transform (GstBaseTransform * trans, GstBuffer * inbuf,
    GstBuffer * outbuf)
{
  GstImxASRC *imxasrc = GST_IMXASRC (trans);
  GstFlowReturn ret;

  GST_LOG_OBJECT (imxasrc, "transforming buffer of %" G_GSIZE_FORMAT " bytes,"
      " ts %" GST_TIME_FORMAT ", duration %" GST_TIME_FORMAT ", offset %"
      G_GINT64_FORMAT ", offset_end %" G_GINT64_FORMAT,
      gst_buffer_get_size (inbuf), GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (inbuf)),
      GST_TIME_ARGS (GST_BUFFER_DURATION (inbuf)),
      GST_BUFFER_OFFSET (inbuf), GST_BUFFER_OFFSET_END (inbuf));

  ret = gst_imxasrc_process (imxasrc, inbuf, outbuf);
  if (G_UNLIKELY (ret != GST_FLOW_OK))
    return ret;

  GST_DEBUG_OBJECT (imxasrc, "input = samples [%" G_GUINT64_FORMAT ", %"
      G_GUINT64_FORMAT ") = [%" G_GUINT64_FORMAT ", %" G_GUINT64_FORMAT
      ") ns;  output = samples [%" G_GUINT64_FORMAT ", %" G_GUINT64_FORMAT
      ") = [%" G_GUINT64_FORMAT ", %" G_GUINT64_FORMAT ") ns",
      GST_BUFFER_OFFSET (inbuf), GST_BUFFER_OFFSET_END (inbuf),
      GST_BUFFER_TIMESTAMP (inbuf), GST_BUFFER_TIMESTAMP (inbuf) +
      GST_BUFFER_DURATION (inbuf), GST_BUFFER_OFFSET (outbuf),
      GST_BUFFER_OFFSET_END (outbuf), GST_BUFFER_TIMESTAMP (outbuf),
      GST_BUFFER_TIMESTAMP (outbuf) + GST_BUFFER_DURATION (outbuf));

  return GST_FLOW_OK;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (IS_IMX8MP())
    return gst_element_register (plugin, "imxasrc", GST_RANK_NONE,
        GST_TYPE_IMXASRC);

  return FALSE;
}

IMX_GST_PLUGIN_DEFINE (imxasrc, "i.MX ASRC Plugins", plugin_init);
