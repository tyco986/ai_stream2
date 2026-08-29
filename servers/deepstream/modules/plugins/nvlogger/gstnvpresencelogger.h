#pragma once

#include "gstnvdetlogger.h"

G_BEGIN_DECLS

typedef struct _GstNvPresenceLogger GstNvPresenceLogger;
typedef struct _GstNvPresenceLoggerClass GstNvPresenceLoggerClass;

#define GST_TYPE_NVPRESENCELOGGER (gst_nvpresencelogger_get_type())
#define GST_NVPRESENCELOGGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_NVPRESENCELOGGER, GstNvPresenceLogger))

struct _GstNvPresenceLogger
{
  GstNvDetLogger parent;
};

struct _GstNvPresenceLoggerClass
{
  GstNvDetLoggerClass parent_class;
};

GType gst_nvpresencelogger_get_type(void);

G_END_DECLS
