#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstStgcnppFadeDrawerWithTracker GstStgcnppFadeDrawerWithTracker;
typedef struct _GstStgcnppFadeDrawerWithTrackerClass GstStgcnppFadeDrawerWithTrackerClass;

#define GST_TYPE_STGCNPP_FADE_DRAWER_WITH_TRACKER (gst_stgcnpp_fade_drawer_with_tracker_get_type())
#define GST_STGCNPP_FADE_DRAWER_WITH_TRACKER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_STGCNPP_FADE_DRAWER_WITH_TRACKER, GstStgcnppFadeDrawerWithTracker))

struct _GstStgcnppFadeDrawerWithTracker {
    GstBaseTransform base;
    gpointer engine;
    gint interval;
    gint fadeTime;
    gboolean showLabel;
    gboolean showPose;
    gfloat poseThreshold;
    gchar* mode;
    gboolean showSnap;
};

struct _GstStgcnppFadeDrawerWithTrackerClass {
    GstBaseTransformClass parent;
};

GType gst_stgcnpp_fade_drawer_with_tracker_get_type(void);

G_END_DECLS
