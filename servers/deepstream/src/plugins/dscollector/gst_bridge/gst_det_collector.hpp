#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstDetCollector GstDetCollector;
typedef struct _GstDetCollectorClass GstDetCollectorClass;

#define GST_TYPE_DET_COLLECTOR (gst_det_collector_get_type())
#define GST_DET_COLLECTOR(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_DET_COLLECTOR, GstDetCollector))

struct _GstDetCollector {
    GstBaseTransform base;
    gpointer engine;
};

struct _GstDetCollectorClass {
    GstBaseTransformClass parent;
};

GType gst_det_collector_get_type(void);

G_END_DECLS
