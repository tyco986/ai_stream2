#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstPoseFadeDrawer GstPoseFadeDrawer;
typedef struct _GstPoseFadeDrawerClass GstPoseFadeDrawerClass;

#define GST_TYPE_POSE_FADE_DRAWER (gst_pose_fade_drawer_get_type())
#define GST_POSE_FADE_DRAWER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_POSE_FADE_DRAWER, GstPoseFadeDrawer))

struct _GstPoseFadeDrawer {
    GstBaseTransform base;
    gpointer engine;
    gint interval;
    gint fadeTime;
    gboolean showLabel;
    gboolean showPose;
    gfloat poseThreshold;
    gchar* mode;
};

struct _GstPoseFadeDrawerClass {
    GstBaseTransformClass parent;
};

GType gst_pose_fade_drawer_get_type(void);

G_END_DECLS
