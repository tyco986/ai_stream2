#include "gstnvrtmposepostprocess.h"

#include <string>
#include <utility>

#include <gst/video/video.h>

#include "constants.hpp"
#include "gstnvdsinfer.h"
#include "gstnvdsmeta.h"
#include "nvds_rtmpose_crop_meta.h"
#include "nvdsinfer.h"
#include "nvdspreprocess_meta.h"

#include <cuda_runtime_api.h>

GST_DEBUG_CATEGORY_STATIC(gst_nvrtmposepostprocess_debug);
#define GST_CAT_DEFAULT gst_nvrtmposepostprocess_debug

enum {
  PROP_0,
  PROP_SGIE_UNIQUE_ID,
};

#define NVMM_CAPS GST_VIDEO_CAPS_MAKE_WITH_FEATURES("memory:NVMM", "{ NV12, RGBA, I420 }")
static GstStaticPadTemplate sink_tmpl = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(NVMM_CAPS));
static GstStaticPadTemplate src_tmpl = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(NVMM_CAPS));

#define gst_nvrtmposepostprocess_parent_class parent_class
G_DEFINE_TYPE(GstNvRtmposePostprocess, gst_nvrtmposepostprocess, GST_TYPE_BASE_TRANSFORM);

static void gst_nvrtmposepostprocess_set_property(GObject *, guint, const GValue *, GParamSpec *);
static void gst_nvrtmposepostprocess_get_property(GObject *, guint, GValue *, GParamSpec *);
static void gst_nvrtmposepostprocess_finalize(GObject *);
static GstFlowReturn gst_nvrtmposepostprocess_transform_ip(GstBaseTransform *, GstBuffer *);

static NvDsRtmposeCropMeta *crop_from_object(NvDsObjectMeta *object_meta)
{
  NvDsRtmposeCropMeta *crop = nullptr;
  for (NvDsMetaList *item = object_meta->obj_user_meta_list; item != nullptr; item = item->next) {
    auto *user_meta = static_cast<NvDsUserMeta *>(item->data);
    if (user_meta != nullptr &&
        user_meta->base_meta.meta_type == NVDS_RTMPOSE_CROP_USER_META) {
      crop = static_cast<NvDsRtmposeCropMeta *>(user_meta->user_meta_data);
    }
  }
  return crop;
}

static NvDsInferTensorMeta *tensor_from_roi(NvDsRoiMeta *roi)
{
  NvDsInferTensorMeta *tensor = nullptr;
  if (roi != nullptr) {
    for (NvDsMetaList *item = roi->roi_user_meta_list; item != nullptr; item = item->next) {
      auto *user_meta = static_cast<NvDsUserMeta *>(item->data);
      if (user_meta != nullptr &&
          user_meta->base_meta.meta_type == NVDSINFER_TENSOR_OUTPUT_META) {
        tensor = static_cast<NvDsInferTensorMeta *>(user_meta->user_meta_data);
      }
    }
  }
  return tensor;
}

static std::pair<const float *, int> keypoints_from_tensor(
    GstNvRtmposePostprocess *self,
    NvDsInferTensorMeta *tensor_meta)
{
  const float *data = nullptr;
  int joints = 0;
  if (tensor_meta != nullptr &&
      static_cast<int>(tensor_meta->unique_id) == self->sgie_unique_id) {
    for (guint i = 0; i < tensor_meta->num_output_layers; ++i) {
      const NvDsInferLayerInfo &layer = tensor_meta->output_layers_info[i];
      if (layer.layerName == nullptr || std::string(layer.layerName) != kKeypointsLayer) {
        continue;
      }
      int elements = static_cast<int>(layer.inferDims.numElements);
      if (elements < 3) {
        continue;
      }
      data = static_cast<const float *>(tensor_meta->out_buf_ptrs_host[i]);
      if (data == nullptr && tensor_meta->out_buf_ptrs_dev[i] != nullptr) {
        self->host_scratch->resize(static_cast<size_t>(elements));
        cudaMemcpy(
            self->host_scratch->data(),
            tensor_meta->out_buf_ptrs_dev[i],
            static_cast<size_t>(elements) * sizeof(float),
            cudaMemcpyDeviceToHost);
        data = self->host_scratch->data();
      }
      joints = elements / 3;
    }
  }
  return {data, joints};
}

static void write_mask_params(
    NvDsObjectMeta *object_meta,
    const NvDsRtmposeCropMeta *crop,
    const float *crop_xy_score,
    int num_joints)
{
  float box_w = object_meta->rect_params.width;
  float box_h = object_meta->rect_params.height;
  float ratio_x = crop->dest_width / static_cast<float>(crop->src_width);
  float ratio_y = crop->dest_height / static_cast<float>(crop->src_height);
  g_free(object_meta->mask_params.data);
  object_meta->mask_params.data = nullptr;
  if (num_joints >= 1 && box_w >= 1.0f && box_h >= 1.0f && ratio_x > 0.0f && ratio_y > 0.0f) {
    object_meta->mask_params.width = 3;
    object_meta->mask_params.height = static_cast<guint>(num_joints);
    object_meta->mask_params.size = sizeof(float) * static_cast<guint>(num_joints) * 3;
    object_meta->mask_params.data =
        static_cast<gfloat *>(g_malloc(object_meta->mask_params.size));
    for (int joint = 0; joint < num_joints; ++joint) {
      float frame_x =
          static_cast<float>(crop->src_left) +
          (crop_xy_score[joint * 3 + 0] - static_cast<float>(crop->offset_left)) / ratio_x;
      float frame_y =
          static_cast<float>(crop->src_top) +
          (crop_xy_score[joint * 3 + 1] - static_cast<float>(crop->offset_top)) / ratio_y;
      object_meta->mask_params.data[joint * 3 + 0] =
          (frame_x - object_meta->rect_params.left) / box_w;
      object_meta->mask_params.data[joint * 3 + 1] =
          (frame_y - object_meta->rect_params.top) / box_h;
      object_meta->mask_params.data[joint * 3 + 2] = crop_xy_score[joint * 3 + 2];
    }
  }
}

static void process_roi(GstNvRtmposePostprocess *self, NvDsRoiMeta *roi)
{
  NvDsObjectMeta *object_meta = roi != nullptr ? roi->object_meta : nullptr;
  NvDsRtmposeCropMeta *crop = object_meta != nullptr ? crop_from_object(object_meta) : nullptr;
  std::pair<const float *, int> keypoints = keypoints_from_tensor(self, tensor_from_roi(roi));
  if (object_meta != nullptr && crop != nullptr && keypoints.first != nullptr &&
      keypoints.second >= 1) {
    write_mask_params(object_meta, crop, keypoints.first, keypoints.second);
  }
}

static bool batch_targets_sgie(const GstNvDsPreProcessBatchMeta *batch, int sgie_id)
{
  bool hit = false;
  if (batch != nullptr) {
    for (guint64 id : batch->target_unique_ids) {
      hit = hit || static_cast<int>(id) == sgie_id;
    }
  }
  return hit;
}

static GstFlowReturn
gst_nvrtmposepostprocess_transform_ip(GstBaseTransform *btrans, GstBuffer *inbuf)
{
  GstNvRtmposePostprocess *self = GST_NVRTMPOSEPOSTPROCESS(btrans);
  NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(inbuf);
  if (batch_meta != nullptr) {
    for (NvDsMetaList *item = batch_meta->batch_user_meta_list; item != nullptr;
         item = item->next) {
      auto *user_meta = static_cast<NvDsUserMeta *>(item->data);
      if (user_meta == nullptr ||
          user_meta->base_meta.meta_type != NVDS_PREPROCESS_BATCH_META) {
        continue;
      }
      auto *batch = static_cast<GstNvDsPreProcessBatchMeta *>(user_meta->user_meta_data);
      if (batch == nullptr || !batch_targets_sgie(batch, self->sgie_unique_id)) {
        continue;
      }
      for (NvDsRoiMeta &roi : batch->roi_vector) {
        process_roi(self, &roi);
      }
    }
  }
  return GST_FLOW_OK;
}

static void
gst_nvrtmposepostprocess_class_init(GstNvRtmposePostprocessClass *klass)
{
  GObjectClass *go = (GObjectClass *)klass;
  GstElementClass *ge = (GstElementClass *)klass;
  GstBaseTransformClass *bt = (GstBaseTransformClass *)klass;

  go->set_property = GST_DEBUG_FUNCPTR(gst_nvrtmposepostprocess_set_property);
  go->get_property = GST_DEBUG_FUNCPTR(gst_nvrtmposepostprocess_get_property);
  go->finalize = GST_DEBUG_FUNCPTR(gst_nvrtmposepostprocess_finalize);
  bt->transform_ip = GST_DEBUG_FUNCPTR(gst_nvrtmposepostprocess_transform_ip);

  g_object_class_install_property(
      go,
      PROP_SGIE_UNIQUE_ID,
      g_param_spec_int(
          "sgie-unique-id",
          "SGIE unique-id",
          "Pose SGIE unique-id whose keypoints tensor to map",
          0,
          G_MAXINT,
          kDefaultSgieUniqueId,
          (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  gst_element_class_add_pad_template(ge, gst_static_pad_template_get(&src_tmpl));
  gst_element_class_add_pad_template(ge, gst_static_pad_template_get(&sink_tmpl));
  gst_element_class_set_details_simple(
      ge,
      "NvRtmposePostprocess",
      "Filter/Metadata",
      "Map RTMPose crop keypoints onto object mask_params",
      "ai_stream2");
  GST_DEBUG_CATEGORY_INIT(
      gst_nvrtmposepostprocess_debug, "nvrtmposepostprocess", 0, "nvrtmposepostprocess");
}

static void
gst_nvrtmposepostprocess_init(GstNvRtmposePostprocess *self)
{
  GstBaseTransform *bt = GST_BASE_TRANSFORM(self);
  gst_base_transform_set_in_place(bt, TRUE);
  gst_base_transform_set_passthrough(bt, FALSE);
  self->sgie_unique_id = kDefaultSgieUniqueId;
  self->host_scratch = new std::vector<float>();
}

static void
gst_nvrtmposepostprocess_finalize(GObject *object)
{
  GstNvRtmposePostprocess *self = GST_NVRTMPOSEPOSTPROCESS(object);
  delete self->host_scratch;
  self->host_scratch = nullptr;
  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void
gst_nvrtmposepostprocess_set_property(
    GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
  GstNvRtmposePostprocess *self = GST_NVRTMPOSEPOSTPROCESS(object);
  switch (prop_id) {
    case PROP_SGIE_UNIQUE_ID:
      self->sgie_unique_id = g_value_get_int(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static void
gst_nvrtmposepostprocess_get_property(
    GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  GstNvRtmposePostprocess *self = GST_NVRTMPOSEPOSTPROCESS(object);
  switch (prop_id) {
    case PROP_SGIE_UNIQUE_ID:
      g_value_set_int(value, self->sgie_unique_id);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}
