#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstDsLogger GstDsLogger;
typedef struct _GstDsLoggerClass GstDsLoggerClass;

#define GST_TYPE_DS_LOGGER (gst_ds_logger_get_type())
#define GST_DS_LOGGER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_DS_LOGGER, GstDsLogger))

struct _GstDsLogger {
    GstBaseTransform base;
    gpointer engine;
    gint interval;
    gchar* page;
};

struct _GstDsLoggerClass {
    GstBaseTransformClass parent;
};

GType gst_ds_logger_get_type(void);

G_END_DECLS
