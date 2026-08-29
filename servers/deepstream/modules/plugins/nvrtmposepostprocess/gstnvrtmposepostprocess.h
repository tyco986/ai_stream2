#pragma once

#include <gst/base/gstbasetransform.h>
#include <vector>

G_BEGIN_DECLS

typedef struct _GstNvRtmposePostprocess GstNvRtmposePostprocess;
typedef struct _GstNvRtmposePostprocessClass GstNvRtmposePostprocessClass;

#define GST_TYPE_NVRTMPOSEPOSTPROCESS (gst_nvrtmposepostprocess_get_type())
#define GST_NVRTMPOSEPOSTPROCESS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_NVRTMPOSEPOSTPROCESS, GstNvRtmposePostprocess))

struct _GstNvRtmposePostprocess
{
  GstBaseTransform base_trans;
  gint sgie_unique_id;
  std::vector<float> *host_scratch;
};

struct _GstNvRtmposePostprocessClass
{
  GstBaseTransformClass parent_class;
};

GType gst_nvrtmposepostprocess_get_type(void);

G_END_DECLS
