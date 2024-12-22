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
#include "imxasrc-device.h"

G_BEGIN_DECLS

#define GST_IMXASRC(obj)   ((GstImxASRC *)(obj))

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
  GstImxASRCMethod  method;
  IMXASRCResamplerQuality quality;
  GstImxASRCConverter *converter;
};

struct _GstImxASRCClass
{
  GstBaseTransformClass base_imxasrc_class;
  GstImxASRCMethod method;
};

G_END_DECLS

#endif
