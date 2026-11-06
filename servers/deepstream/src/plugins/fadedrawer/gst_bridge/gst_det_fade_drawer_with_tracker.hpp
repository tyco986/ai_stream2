#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstDetFadeDrawerWithTracker GstDetFadeDrawerWithTracker;
typedef struct _GstDetFadeDrawerWithTrackerClass GstDetFadeDrawerWithTrackerClass;

#define GST_TYPE_DET_FADE_DRAWER_WITH_TRACKER (gst_det_fade_drawer_with_tracker_get_type())
#define GST_DET_FADE_DRAWER_WITH_TRACKER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_DET_FADE_DRAWER_WITH_TRACKER, GstDetFadeDrawerWithTracker))

struct _GstDetFadeDrawerWithTracker {
    GstBaseTransform base;
    gpointer engine;
    gint interval;
    gint fadeTime;
    gboolean showLabel;
    gboolean showSnap;
};

struct _GstDetFadeDrawerWithTrackerClass {
    GstBaseTransformClass parent;
};

GType gst_det_fade_drawer_with_tracker_get_type(void);

G_END_DECLS
