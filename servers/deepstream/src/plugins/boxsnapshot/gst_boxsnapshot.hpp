#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstBoxSnapshot GstBoxSnapshot;
typedef struct _GstBoxSnapshotClass GstBoxSnapshotClass;

#define GST_TYPE_BOXSNAPSHOT (gst_boxsnapshot_get_type())

struct _GstBoxSnapshot {
    GstBaseTransform base;
};

struct _GstBoxSnapshotClass {
    GstBaseTransformClass parent;
};

GType gst_boxsnapshot_get_type(void);

G_END_DECLS
