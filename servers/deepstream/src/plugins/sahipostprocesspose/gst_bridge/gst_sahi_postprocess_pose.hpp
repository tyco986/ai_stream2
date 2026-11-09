#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

typedef struct _GstSahiPostProcessPose GstSahiPostProcessPose;
typedef struct _GstSahiPostProcessPoseClass GstSahiPostProcessPoseClass;

#define GST_TYPE_SAHI_POSTPROCESS_POSE (gst_sahi_postprocess_pose_get_type())
#define GST_SAHI_POSTPROCESS_POSE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SAHI_POSTPROCESS_POSE, GstSahiPostProcessPose))

struct _GstSahiPostProcessPose {
    GstBaseTransform base;
    gpointer engine;
    gchar* gieIds;
    gboolean classAgnostic;
    gint matchMetric;
    gfloat matchThreshold;
    gboolean preferFullFrame;
    gint maxDetections;
    gfloat oksThreshold;
    gfloat visThreshold;
    gint numKeypoints;
};

struct _GstSahiPostProcessPoseClass {
    GstBaseTransformClass parent;
};

GType gst_sahi_postprocess_pose_get_type(void);

G_END_DECLS
