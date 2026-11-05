#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <numeric>
#include <unordered_map>
#include <vector>

#include <gst/gst.h>

#include "gstnvsahipostprocess_pose.h"
#include "nvds_latency_meta.h"
#include "oks_pose_nms.h"
#include "spatial_grid.h"

#ifdef _OPENMP
#include <omp.h>
#endif

GST_DEBUG_CATEGORY_STATIC(gst_nvsahipostprocess_pose_debug);
#define GST_CAT_DEFAULT gst_nvsahipostprocess_pose_debug

enum {
  PROP_0,
  PROP_GIE_IDS,
  PROP_CLASS_AGNOSTIC,
  PROP_OKS_THRESHOLD,
  PROP_VIS_THRESHOLD,
  PROP_NUM_KEYPOINTS,
};

#define DEFAULT_GIE_IDS "-1"
#define DEFAULT_CLASS_AGNOSTIC FALSE
#define DEFAULT_OKS_THRESHOLD 0.5f
#define DEFAULT_VIS_THRESHOLD 0.0f
#define DEFAULT_NUM_KEYPOINTS 0

#define NVMM_CAPS GST_VIDEO_CAPS_MAKE_WITH_FEATURES("memory:NVMM", "{ NV12, RGBA, I420 }")
static GstStaticPadTemplate sink_tmpl = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(NVMM_CAPS));
static GstStaticPadTemplate src_tmpl = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(NVMM_CAPS));

#define gst_nvsahipostprocess_pose_parent_class parent_class
G_DEFINE_TYPE(GstNvSahiPostProcessPose, gst_nvsahipostprocess_pose, GST_TYPE_BASE_TRANSFORM);

static void gst_nvsahipostprocess_pose_set_property(GObject *, guint, const GValue *, GParamSpec *);
static void gst_nvsahipostprocess_pose_get_property(GObject *, guint, GValue *, GParamSpec *);
static void gst_nvsahipostprocess_pose_finalize(GObject *);
static GstFlowReturn gst_nvsahipostprocess_pose_transform_ip(GstBaseTransform *, GstBuffer *);

static void
parse_gie_ids(GstNvSahiPostProcessPose *self, const gchar *str)
{
  self->gie_ids->clear();
  self->gie_filter_all = FALSE;
  g_free(self->gie_ids_str);
  self->gie_ids_str = g_strdup(str ? str : DEFAULT_GIE_IDS);
  gchar **tokens = g_strsplit(self->gie_ids_str, ";", -1);
  for (guint i = 0; tokens[i]; i++) {
    g_strstrip(tokens[i]);
    if (tokens[i][0] == '\0')
      continue;
    gint val = atoi(tokens[i]);
    if (val < 0) {
      self->gie_filter_all = TRUE;
      self->gie_ids->clear();
      break;
    }
    self->gie_ids->insert(val);
  }
  g_strfreev(tokens);
  if (self->gie_ids->empty())
    self->gie_filter_all = TRUE;
}

static void
fill_pose_keypoints(GstNvSahiPostProcessPose *self, SahiPoseDetection *d,
                    NvDsObjectMeta *obj)
{
  guint k = 0;
  gfloat bw = obj->rect_params.width;
  gfloat bh = obj->rect_params.height;
  gboolean ok = obj->mask_params.data != nullptr &&
      obj->mask_params.width == 3 &&
      obj->mask_params.height >= 1 &&
      bw > 0.0f && bh > 0.0f;

  d->num_keypoints = 0;
  d->kpts = nullptr;
  if (ok) {
    k = (self->num_keypoints > 0)
            ? (guint)self->num_keypoints
            : obj->mask_params.height;
    ok = k >= 1 && obj->mask_params.height >= k;
  }
  if (ok) {
    d->kpts = (gfloat *)g_malloc(sizeof(gfloat) * k * 3);
    for (guint j = 0; j < k; j++) {
      d->kpts[j * 3 + 0] = obj->rect_params.left +
          obj->mask_params.data[j * 3 + 0] * bw;
      d->kpts[j * 3 + 1] = obj->rect_params.top +
          obj->mask_params.data[j * 3 + 1] * bh;
      d->kpts[j * 3 + 2] = obj->mask_params.data[j * 3 + 2];
    }
    d->num_keypoints = k;
  }
}

static void
free_detection_scratch(std::vector<SahiPoseDetection> &dets)
{
  for (SahiPoseDetection &d : dets) {
    if (d.kpts) {
      g_free(d.kpts);
      d.kpts = nullptr;
    }
  }
}

static void
process_frame(GstNvSahiPostProcessPose *self,
              NvDsBatchMeta *batch_meta, NvDsFrameMeta *frame_meta)
{
  std::vector<SahiPoseDetection> dets;
  std::vector<uint8_t> suppressed;
  std::vector<guint> order;
  SahiPoseSpatialGrid grid;

  dets.reserve(512);

  for (NvDsMetaList *l = frame_meta->obj_meta_list; l; l = l->next) {
    NvDsObjectMeta *obj = (NvDsObjectMeta *)l->data;
    SahiPoseDetection d;

    if (!self->gie_filter_all &&
        self->gie_ids->find(obj->unique_component_id) == self->gie_ids->end())
      continue;

    fill_pose_keypoints(self, &d, obj);
    if (!d.kpts)
      continue;

    d.left = obj->rect_params.left;
    d.top = obj->rect_params.top;
    d.right = d.left + obj->rect_params.width;
    d.bottom = d.top + obj->rect_params.height;
    d.score = obj->confidence;
    d.class_id = obj->class_id;
    d.area = obj->rect_params.width * obj->rect_params.height;
    d.obj_meta = obj;
    dets.push_back(d);
  }

  GST_LOG_OBJECT(self, "frame %u: collected %zu pose dets",
                 frame_meta->frame_num, dets.size());

  if (dets.size() <= 1) {
    free_detection_scratch(dets);
    return;
  }

  const guint n = dets.size();
  order.resize(n);
  std::iota(order.begin(), order.end(), 0);
  PoseScoreOrder score_order;
  score_order.dets = &dets;
  std::sort(order.begin(), order.end(), score_order);

  suppressed.assign(n, 0);

  gfloat fw = 0, fh = 0;
  if (frame_meta->source_frame_width > 0 &&
      frame_meta->source_frame_height > 0) {
    fw = (gfloat)frame_meta->source_frame_width;
    fh = (gfloat)frame_meta->source_frame_height;
  } else {
    for (guint i = 0; i < n; i++) {
      fw = MAX(fw, dets[i].right);
      fh = MAX(fh, dets[i].bottom);
    }
  }

  std::vector<SahiPoseGridRect> rects(n);
  for (guint i = 0; i < n; i++)
    rects[i] = {dets[i].left, dets[i].top, dets[i].right, dets[i].bottom};
  grid.build(rects, n, fw, fh);

  const gboolean agnostic = self->class_agnostic;
  if (!agnostic) {
    std::unordered_map<gint, std::vector<guint>> by_class;
    for (guint ii = 0; ii < order.size(); ii++)
      by_class[dets[order[ii]].class_id].push_back(order[ii]);
    for (auto &kv : by_class)
      run_oks_nms_on_group(dets, suppressed, grid, kv.second,
                           self->oks_threshold, self->vis_threshold, agnostic);
  } else {
    std::vector<guint> all(order.begin(), order.end());
    run_oks_nms_on_group(dets, suppressed, grid, all,
                         self->oks_threshold, self->vis_threshold, agnostic);
  }

  std::vector<NvDsObjectMeta *> to_remove;
  for (guint i = 0; i < n; i++) {
    if (suppressed[i])
      to_remove.push_back(dets[i].obj_meta);
  }

  nvds_acquire_meta_lock(batch_meta);
  for (auto *obj : to_remove)
    nvds_remove_obj_meta_from_frame(frame_meta, obj);
  nvds_release_meta_lock(batch_meta);

  free_detection_scratch(dets);
}

static GstFlowReturn
gst_nvsahipostprocess_pose_transform_ip(GstBaseTransform *btrans,
                                        GstBuffer *inbuf)
{
  GstNvSahiPostProcessPose *self = GST_NVSAHIPOSTPROCESS_POSE(btrans);
  GstFlowReturn flow = GST_FLOW_OK;
  NvDsBatchMeta *batch_meta = nullptr;
  std::vector<NvDsFrameMeta *> frames;

  nvds_set_input_system_timestamp(inbuf, GST_ELEMENT_NAME(self));
  batch_meta = gst_buffer_get_nvds_batch_meta(inbuf);
  if (batch_meta) {
    for (NvDsMetaList *l = batch_meta->frame_meta_list; l; l = l->next)
      frames.push_back((NvDsFrameMeta *)l->data);
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic) if (frames.size() > 1)
#endif
    for (int f = 0; f < (int)frames.size(); f++)
      process_frame(self, batch_meta, frames[f]);
  }
  nvds_set_output_system_timestamp(inbuf, GST_ELEMENT_NAME(self));
  return flow;
}

static void
gst_nvsahipostprocess_pose_class_init(GstNvSahiPostProcessPoseClass *klass)
{
  GObjectClass *go = (GObjectClass *)klass;
  GstElementClass *ge = (GstElementClass *)klass;
  GstBaseTransformClass *bt = (GstBaseTransformClass *)klass;

  go->set_property = GST_DEBUG_FUNCPTR(gst_nvsahipostprocess_pose_set_property);
  go->get_property = GST_DEBUG_FUNCPTR(gst_nvsahipostprocess_pose_get_property);
  go->finalize = GST_DEBUG_FUNCPTR(gst_nvsahipostprocess_pose_finalize);
  bt->transform_ip = GST_DEBUG_FUNCPTR(gst_nvsahipostprocess_pose_transform_ip);

#define RW (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
  g_object_class_install_property(
      go, PROP_GIE_IDS,
      g_param_spec_string("gie-ids", "GIE IDs",
                          "\"-1\"=all, or semicolon-separated ids", DEFAULT_GIE_IDS, RW));
  g_object_class_install_property(
      go, PROP_CLASS_AGNOSTIC,
      g_param_spec_boolean("class-agnostic", "Class Agnostic",
                           "Match across different class IDs", DEFAULT_CLASS_AGNOSTIC, RW));
  g_object_class_install_property(
      go, PROP_OKS_THRESHOLD,
      g_param_spec_float("oks-threshold", "OKS Threshold",
                         "OKS above this suppresses the lower-score pose",
                         0.0f, 1.0f, DEFAULT_OKS_THRESHOLD, RW));
  g_object_class_install_property(
      go, PROP_VIS_THRESHOLD,
      g_param_spec_float("vis-threshold", "Visibility Threshold",
                         "Keypoint score must exceed this to count in OKS",
                         0.0f, 1.0f, DEFAULT_VIS_THRESHOLD, RW));
  g_object_class_install_property(
      go, PROP_NUM_KEYPOINTS,
      g_param_spec_int("num-keypoints", "Num Keypoints",
                       "K for Kx3 keypoints (0=use mask_params.height)",
                       0, G_MAXINT, DEFAULT_NUM_KEYPOINTS, RW));

  gst_element_class_add_pad_template(ge, gst_static_pad_template_get(&src_tmpl));
  gst_element_class_add_pad_template(ge, gst_static_pad_template_get(&sink_tmpl));
  gst_element_class_set_details_simple(
      ge, "SAHI Pose Post-Process (OKS-NMS)",
      "Filter/Metadata",
      "Suppresses duplicate SAHI pose instances using OKS-NMS",
      "ai_stream2");
}

static void
gst_nvsahipostprocess_pose_init(GstNvSahiPostProcessPose *self)
{
  GstBaseTransform *bt = GST_BASE_TRANSFORM(self);
  gst_base_transform_set_in_place(bt, TRUE);
  gst_base_transform_set_passthrough(bt, FALSE);

  self->gie_ids_str = nullptr;
  self->gie_ids = new std::unordered_set<gint>();
  self->gie_filter_all = TRUE;
  parse_gie_ids(self, DEFAULT_GIE_IDS);
  self->class_agnostic = DEFAULT_CLASS_AGNOSTIC;
  self->oks_threshold = DEFAULT_OKS_THRESHOLD;
  self->vis_threshold = DEFAULT_VIS_THRESHOLD;
  self->num_keypoints = DEFAULT_NUM_KEYPOINTS;
}

static void
gst_nvsahipostprocess_pose_finalize(GObject *object)
{
  GstNvSahiPostProcessPose *self = GST_NVSAHIPOSTPROCESS_POSE(object);
  g_free(self->gie_ids_str);
  self->gie_ids_str = nullptr;
  delete self->gie_ids;
  self->gie_ids = nullptr;
  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void
gst_nvsahipostprocess_pose_set_property(GObject *object, guint prop_id,
                                        const GValue *value, GParamSpec *pspec)
{
  GstNvSahiPostProcessPose *s = GST_NVSAHIPOSTPROCESS_POSE(object);
  switch (prop_id) {
    case PROP_GIE_IDS:
      parse_gie_ids(s, g_value_get_string(value));
      break;
    case PROP_CLASS_AGNOSTIC:
      s->class_agnostic = g_value_get_boolean(value);
      break;
    case PROP_OKS_THRESHOLD:
      s->oks_threshold = g_value_get_float(value);
      break;
    case PROP_VIS_THRESHOLD:
      s->vis_threshold = g_value_get_float(value);
      break;
    case PROP_NUM_KEYPOINTS:
      s->num_keypoints = g_value_get_int(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static void
gst_nvsahipostprocess_pose_get_property(GObject *object, guint prop_id,
                                        GValue *value, GParamSpec *pspec)
{
  GstNvSahiPostProcessPose *s = GST_NVSAHIPOSTPROCESS_POSE(object);
  switch (prop_id) {
    case PROP_GIE_IDS:
      g_value_set_string(value, s->gie_ids_str);
      break;
    case PROP_CLASS_AGNOSTIC:
      g_value_set_boolean(value, s->class_agnostic);
      break;
    case PROP_OKS_THRESHOLD:
      g_value_set_float(value, s->oks_threshold);
      break;
    case PROP_VIS_THRESHOLD:
      g_value_set_float(value, s->vis_threshold);
      break;
    case PROP_NUM_KEYPOINTS:
      g_value_set_int(value, s->num_keypoints);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static gboolean
nvsahipostprocess_pose_plugin_init(GstPlugin *plugin)
{
  GST_DEBUG_CATEGORY_INIT(gst_nvsahipostprocess_pose_debug,
                          "nvsahipostprocess_pose", 0,
                          "SAHI pose OKS-NMS post-process plugin");
  return gst_element_register(plugin, "nvsahipostprocess_pose",
                              GST_RANK_PRIMARY, GST_TYPE_NVSAHIPOSTPROCESS_POSE);
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  nvdsgst_sahipostprocess_pose,
                  DESCRIPTION,
                  nvsahipostprocess_pose_plugin_init,
                  VERSION,
                  LICENSE,
                  BINARY_PACKAGE,
                  URL)
