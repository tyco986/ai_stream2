#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstDetFadeDrawer GstDetFadeDrawer;
typedef struct _GstDetFadeDrawerClass GstDetFadeDrawerClass;

#define GST_TYPE_DET_FADE_DRAWER (gst_det_fade_drawer_get_type())
#define GST_DET_FADE_DRAWER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_DET_FADE_DRAWER, GstDetFadeDrawer))

struct _GstDetFadeDrawer {
    GstBaseTransform base;
    gpointer engine;
    gint interval;
    gint fadeTime;
    gboolean showLabel;
};

struct _GstDetFadeDrawerClass {
    GstBaseTransformClass parent;
};

GType gst_det_fade_drawer_get_type(void);

G_END_DECLS
