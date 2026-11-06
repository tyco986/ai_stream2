#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstStgcnppLogger GstStgcnppLogger;
typedef struct _GstStgcnppLoggerClass GstStgcnppLoggerClass;

#define GST_TYPE_STGCNPP_LOGGER (gst_stgcnpp_logger_get_type())
#define GST_STGCNPP_LOGGER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_STGCNPP_LOGGER, GstStgcnppLogger))

struct _GstStgcnppLogger {
    GstBaseTransform base;
    gpointer engine;
    gint interval;
};

struct _GstStgcnppLoggerClass {
    GstBaseTransformClass parent;
};

GType gst_stgcnpp_logger_get_type(void);

G_END_DECLS
