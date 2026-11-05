#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstNvBboxSnapshot GstNvBboxSnapshot;
typedef struct _GstNvBboxSnapshotClass GstNvBboxSnapshotClass;

#define GST_TYPE_NVBBOXSNAPSHOT (gst_nvbboxsnapshot_get_type())
#define GST_NVBBOXSNAPSHOT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_NVBBOXSNAPSHOT, GstNvBboxSnapshot))

struct _GstNvBboxSnapshot
{
  GstBaseTransform base_trans;
};

struct _GstNvBboxSnapshotClass
{
  GstBaseTransformClass parent_class;
};

GType gst_nvbboxsnapshot_get_type(void);

G_END_DECLS
