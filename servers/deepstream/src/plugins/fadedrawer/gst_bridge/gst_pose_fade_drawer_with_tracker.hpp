#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstPoseFadeDrawerWithTracker GstPoseFadeDrawerWithTracker;
typedef struct _GstPoseFadeDrawerWithTrackerClass GstPoseFadeDrawerWithTrackerClass;

#define GST_TYPE_POSE_FADE_DRAWER_WITH_TRACKER (gst_pose_fade_drawer_with_tracker_get_type())
#define GST_POSE_FADE_DRAWER_WITH_TRACKER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_POSE_FADE_DRAWER_WITH_TRACKER, GstPoseFadeDrawerWithTracker))

struct _GstPoseFadeDrawerWithTracker {
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

struct _GstPoseFadeDrawerWithTrackerClass {
    GstBaseTransformClass parent;
};

GType gst_pose_fade_drawer_with_tracker_get_type(void);

G_END_DECLS
