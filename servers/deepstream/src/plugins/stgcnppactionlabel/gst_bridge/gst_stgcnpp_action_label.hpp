#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstStgcnppActionLabel GstStgcnppActionLabel;
typedef struct _GstStgcnppActionLabelClass GstStgcnppActionLabelClass;

#define GST_TYPE_STGCNPP_ACTION_LABEL (gst_stgcnpp_action_label_get_type())
#define GST_STGCNPP_ACTION_LABEL(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_STGCNPP_ACTION_LABEL, GstStgcnppActionLabel))

struct _GstStgcnppActionLabel {
    GstBaseTransform base;
    gpointer engine;
};

struct _GstStgcnppActionLabelClass {
    GstBaseTransformClass parent;
};

GType gst_stgcnpp_action_label_get_type(void);

G_END_DECLS
