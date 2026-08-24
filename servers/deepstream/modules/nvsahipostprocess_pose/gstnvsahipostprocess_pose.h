#ifndef __GST_NVSAHIPOSTPROCESS_POSE_H__
#define __GST_NVSAHIPOSTPROCESS_POSE_H__

#include <gst/base/gstbasetransform.h>
#include <gst/video/video.h>
#include <unordered_set>
#include "gstnvdsmeta.h"
#include "nvdsmeta.h"

#define PACKAGE "nvsahipostprocess_pose"
#define VERSION "1.0"
#define LICENSE "Apache-2.0"
#define DESCRIPTION "SAHI pose OKS-NMS post-process plugin"
#define BINARY_PACKAGE "DeepStream SAHI Pose Post-Process"
#define URL "https://github.com"

G_BEGIN_DECLS

typedef struct _GstNvSahiPostProcessPose GstNvSahiPostProcessPose;
typedef struct _GstNvSahiPostProcessPoseClass GstNvSahiPostProcessPoseClass;

#define GST_TYPE_NVSAHIPOSTPROCESS_POSE (gst_nvsahipostprocess_pose_get_type())
#define GST_NVSAHIPOSTPROCESS_POSE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_NVSAHIPOSTPROCESS_POSE, GstNvSahiPostProcessPose))

struct _GstNvSahiPostProcessPose
{
  GstBaseTransform base_trans;

  gchar *gie_ids_str;
  std::unordered_set<gint> *gie_ids;
  gboolean gie_filter_all;
  gboolean class_agnostic;
  gfloat oks_threshold;
  gfloat vis_threshold;
  gint num_keypoints;
};

struct _GstNvSahiPostProcessPoseClass
{
  GstBaseTransformClass parent_class;
};

GType gst_nvsahipostprocess_pose_get_type(void);

G_END_DECLS

typedef struct {
  gfloat left, top, right, bottom;
  gfloat score;
  gint class_id;
  gfloat area;
  NvDsObjectMeta *obj_meta;
  guint num_keypoints;
  gfloat *kpts;
} SahiPoseDetection;

#endif
