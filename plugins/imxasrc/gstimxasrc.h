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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _GST_IMXASRC_H_
#define _GST_IMXASRC_H_

#include <gst/base/gstbasetransform.h>
#include "imxasrc-converter.h"

G_BEGIN_DECLS

#define GST_TYPE_IMXASRC   (gst_imxasrc_get_type())
#define GST_IMXASRC(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_IMXASRC,GstImxASRC))
#define GST_IMXASRC_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_IMXASRC,GstImxASRCClass))
#define GST_IS_IMXASRC(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_IMXASRC))
#define GST_IS_IMXASRC_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_IMXASRC))

typedef struct _GstImxASRC GstImxASRC;
typedef struct _GstImxASRCClass GstImxASRCClass;

struct _GstImxASRC
{
  GstBaseTransform base_imxasrc;

    /* <private> */
  gboolean need_discont;

  GstClockTime t0;
  guint64 in_offset0;
  guint64 out_offset0;
  guint64 samples_in;
  guint64 samples_out;

  guint64 num_gap_samples;
  guint64 num_nongap_samples;

    /* state */
  GstAudioInfo in;
  GstAudioInfo out;

    /* Converter */
  gboolean is_hw_resample;
  GstAudioConverter *sw_converter;
  GstImxASRCConverter *hw_converter;
};

struct _GstImxASRCClass
{
  GstBaseTransformClass base_imxasrc_class;
};

GType gst_imxasrc_get_type (void);

G_END_DECLS

#endif
