#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstNvVisCapturer GstNvVisCapturer;
typedef struct _GstNvVisCapturerClass GstNvVisCapturerClass;

#define GST_TYPE_NVVISCAPTURER (gst_nvviscapturer_get_type())
#define GST_NVVISCAPTURER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_NVVISCAPTURER, GstNvVisCapturer))

struct _GstNvVisCapturer
{
  GstBaseTransform base_trans;
  gpointer engine;
};

struct _GstNvVisCapturerClass
{
  GstBaseTransformClass parent_class;
};

GType gst_nvviscapturer_get_type(void);

G_END_DECLS
