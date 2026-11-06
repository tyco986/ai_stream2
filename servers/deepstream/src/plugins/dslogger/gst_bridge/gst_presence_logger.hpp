#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstPresenceLogger GstPresenceLogger;
typedef struct _GstPresenceLoggerClass GstPresenceLoggerClass;

#define GST_TYPE_PRESENCE_LOGGER (gst_presence_logger_get_type())
#define GST_PRESENCE_LOGGER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_PRESENCE_LOGGER, GstPresenceLogger))

struct _GstPresenceLogger {
    GstBaseTransform base;
    gpointer engine;
    gint interval;
};

struct _GstPresenceLoggerClass {
    GstBaseTransformClass parent;
};

GType gst_presence_logger_get_type(void);

G_END_DECLS
