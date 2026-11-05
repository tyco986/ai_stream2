#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstNvDetLogger GstNvDetLogger;
typedef struct _GstNvDetLoggerClass GstNvDetLoggerClass;

#define GST_TYPE_NVDETLOGGER (gst_nvdetlogger_get_type())
#define GST_NVDETLOGGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_NVDETLOGGER, GstNvDetLogger))

struct _GstNvDetLogger
{
  GstBaseTransform base_trans;
  gpointer engine;
};

struct _GstNvDetLoggerClass
{
  GstBaseTransformClass parent_class;
};

GType gst_nvdetlogger_get_type(void);

G_END_DECLS
