#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstVisCapturer GstVisCapturer;
typedef struct _GstVisCapturerClass GstVisCapturerClass;

#define GST_TYPE_VIS_CAPTURER (gst_vis_capturer_get_type())
#define GST_VIS_CAPTURER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_VIS_CAPTURER, GstVisCapturer))

struct _GstVisCapturer {
    GstBaseTransform base;
    gpointer engine;
    gchar* captureCodes;
    gchar* labelTask;
};

struct _GstVisCapturerClass {
    GstBaseTransformClass parent;
};

GType gst_vis_capturer_get_type(void);

G_END_DECLS
