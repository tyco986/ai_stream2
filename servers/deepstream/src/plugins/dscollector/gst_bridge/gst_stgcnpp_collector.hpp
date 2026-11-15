#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstStgcnppCollector GstStgcnppCollector;
typedef struct _GstStgcnppCollectorClass GstStgcnppCollectorClass;

#define GST_TYPE_STGCNPP_COLLECTOR (gst_stgcnpp_collector_get_type())
#define GST_STGCNPP_COLLECTOR(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_STGCNPP_COLLECTOR, GstStgcnppCollector))

struct _GstStgcnppCollector {
    GstBaseTransform base;
    gpointer engine;
};

struct _GstStgcnppCollectorClass {
    GstBaseTransformClass parent;
};

GType gst_stgcnpp_collector_get_type(void);

G_END_DECLS
