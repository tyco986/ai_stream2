#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstPresenceCollector GstPresenceCollector;
typedef struct _GstPresenceCollectorClass GstPresenceCollectorClass;

#define GST_TYPE_PRESENCE_COLLECTOR (gst_presence_collector_get_type())
#define GST_PRESENCE_COLLECTOR(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_PRESENCE_COLLECTOR, GstPresenceCollector))

struct _GstPresenceCollector {
    GstBaseTransform base;
    gpointer engine;
};

struct _GstPresenceCollectorClass {
    GstBaseTransformClass parent;
};

GType gst_presence_collector_get_type(void);

G_END_DECLS
