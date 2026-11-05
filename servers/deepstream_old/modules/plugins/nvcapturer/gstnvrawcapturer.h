#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstNvRawCapturer GstNvRawCapturer;
typedef struct _GstNvRawCapturerClass GstNvRawCapturerClass;

#define GST_TYPE_NVRAWCAPTURER (gst_nvrawcapturer_get_type())
#define GST_NVRAWCAPTURER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_NVRAWCAPTURER, GstNvRawCapturer))

struct _GstNvRawCapturer
{
  GstBaseTransform base_trans;
  gpointer engine;
};

struct _GstNvRawCapturerClass
{
  GstBaseTransformClass parent_class;
};

GType gst_nvrawcapturer_get_type(void);

G_END_DECLS
