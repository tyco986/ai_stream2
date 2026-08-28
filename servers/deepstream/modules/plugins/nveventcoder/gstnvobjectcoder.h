#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstNvObjectCoder GstNvObjectCoder;
typedef struct _GstNvObjectCoderClass GstNvObjectCoderClass;

#define GST_TYPE_NVOBJECTCODER (gst_nvobjectcoder_get_type())
#define GST_NVOBJECTCODER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_NVOBJECTCODER, GstNvObjectCoder))

struct _GstNvObjectCoder
{
  GstBaseTransform base_trans;
};

struct _GstNvObjectCoderClass
{
  GstBaseTransformClass parent_class;
};

GType gst_nvobjectcoder_get_type(void);

G_END_DECLS
