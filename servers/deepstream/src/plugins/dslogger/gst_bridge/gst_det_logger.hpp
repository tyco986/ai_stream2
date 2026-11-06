#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstDetLogger GstDetLogger;
typedef struct _GstDetLoggerClass GstDetLoggerClass;

#define GST_TYPE_DET_LOGGER (gst_det_logger_get_type())
#define GST_DET_LOGGER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_DET_LOGGER, GstDetLogger))

struct _GstDetLogger {
    GstBaseTransform base;
    gpointer engine;
    gint interval;
};

struct _GstDetLoggerClass {
    GstBaseTransformClass parent;
};

GType gst_det_logger_get_type(void);

G_END_DECLS
