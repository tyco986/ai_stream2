#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstNvPresenceCoder GstNvPresenceCoder;
typedef struct _GstNvPresenceCoderClass GstNvPresenceCoderClass;

#define GST_TYPE_NVPRESENCECODER (gst_nvpresencecoder_get_type())
#define GST_NVPRESENCECODER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_NVPRESENCECODER, GstNvPresenceCoder))

struct _GstNvPresenceCoder
{
  GstBaseTransform base_trans;
  gpointer engine;
};

struct _GstNvPresenceCoderClass
{
  GstBaseTransformClass parent_class;
};

GType gst_nvpresencecoder_get_type(void);

G_END_DECLS
