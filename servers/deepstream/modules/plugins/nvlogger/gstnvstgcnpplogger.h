#pragma once

#include "gstnvdetlogger.h"

G_BEGIN_DECLS

typedef struct _GstNvStgcnppLogger GstNvStgcnppLogger;
typedef struct _GstNvStgcnppLoggerClass GstNvStgcnppLoggerClass;

#define GST_TYPE_NVSTGCNPPLOGGER (gst_nvstgcnpplogger_get_type())
#define GST_NVSTGCNPPLOGGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_NVSTGCNPPLOGGER, GstNvStgcnppLogger))

struct _GstNvStgcnppLogger
{
  GstNvDetLogger parent;
};

struct _GstNvStgcnppLoggerClass
{
  GstNvDetLoggerClass parent_class;
};

GType gst_nvstgcnpplogger_get_type(void);

G_END_DECLS
