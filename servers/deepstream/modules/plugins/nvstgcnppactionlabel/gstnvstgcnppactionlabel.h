#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstNvStgcnppActionLabel GstNvStgcnppActionLabel;
typedef struct _GstNvStgcnppActionLabelClass GstNvStgcnppActionLabelClass;

#define GST_TYPE_NVSTGCNPPACTIONLABEL (gst_nvstgcnppactionlabel_get_type())
#define GST_NVSTGCNPPACTIONLABEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_NVSTGCNPPACTIONLABEL, GstNvStgcnppActionLabel))

struct _GstNvStgcnppActionLabel
{
  GstBaseTransform base_trans;
  gpointer engine;
};

struct _GstNvStgcnppActionLabelClass
{
  GstBaseTransformClass parent_class;
};

GType gst_nvstgcnppactionlabel_get_type(void);

G_END_DECLS
