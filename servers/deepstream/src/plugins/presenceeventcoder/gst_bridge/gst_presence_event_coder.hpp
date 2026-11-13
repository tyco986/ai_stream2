#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstPresenceEventCoder GstPresenceEventCoder;
typedef struct _GstPresenceEventCoderClass GstPresenceEventCoderClass;

#define GST_TYPE_PRESENCE_EVENT_CODER (gst_presence_event_coder_get_type())
#define GST_PRESENCE_EVENT_CODER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_PRESENCE_EVENT_CODER, GstPresenceEventCoder))

struct _GstPresenceEventCoder {
    GstBaseTransform base;
    gpointer engine;
    // Slot, class id, event name. Example: "0:0:Fire;1:1:Smoke"
    gchar* eventSlotMap;
    gint length;
    gfloat threshold;
    gchar* mode;
};

struct _GstPresenceEventCoderClass {
    GstBaseTransformClass parent;
};

GType gst_presence_event_coder_get_type(void);

G_END_DECLS
