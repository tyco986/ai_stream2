#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstSegFadeDrawer GstSegFadeDrawer;
typedef struct _GstSegFadeDrawerClass GstSegFadeDrawerClass;

#define GST_TYPE_SEG_FADE_DRAWER (gst_seg_fade_drawer_get_type())
#define GST_SEG_FADE_DRAWER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SEG_FADE_DRAWER, GstSegFadeDrawer))

struct _GstSegFadeDrawer {
    GstBaseTransform base;
    gpointer engine;
    gint interval;
    gint fadeTime;
    gboolean showLabel;
    gboolean showMask;
};

struct _GstSegFadeDrawerClass {
    GstBaseTransformClass parent;
};

GType gst_seg_fade_drawer_get_type(void);

G_END_DECLS
