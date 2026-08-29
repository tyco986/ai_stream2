#include "nvdspreprocess_lib.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "nvds_rtmpose_crop_meta.h"
#include "gstnvdsmeta.h"
#include "nvdsmeta.h"
#include "rect_expand.hpp"
#include "rgba_to_nchw.h"

namespace {

constexpr int kDefaultInferWidth = 192;
constexpr int kDefaultInferHeight = 256;
constexpr float kDefaultPadding = 1.25f;
constexpr float kDefaultScale = 0.017124753831663668f;
constexpr float kDefaultOffsetR = 123.675f;
constexpr float kDefaultOffsetG = 116.28f;
constexpr float kDefaultOffsetB = 103.53f;

int parse_int(
    const std::unordered_map<std::string, std::string> &configs,
    const char *key,
    int fallback)
{
  int value = fallback;
  auto it = configs.find(key);
  if (it != configs.end() && !it->second.empty()) {
    value = atoi(it->second.c_str());
  }
  return value;
}

float parse_float(
    const std::unordered_map<std::string, std::string> &configs,
    const char *key,
    float fallback)
{
  float value = fallback;
  auto it = configs.find(key);
  if (it != configs.end() && !it->second.empty()) {
    value = static_cast<float>(atof(it->second.c_str()));
  }
  return value;
}

void parse_offsets(
    const std::unordered_map<std::string, std::string> &configs,
    float *offset_r,
    float *offset_g,
    float *offset_b)
{
  *offset_r = kDefaultOffsetR;
  *offset_g = kDefaultOffsetG;
  *offset_b = kDefaultOffsetB;
  auto it = configs.find("offsets");
  if (it == configs.end() || it->second.empty()) {
    return;
  }
  std::vector<float> values;
  std::stringstream stream(it->second);
  std::string token;
  while (std::getline(stream, token, ';')) {
    if (!token.empty()) {
      values.push_back(static_cast<float>(atof(token.c_str())));
    }
  }
  if (values.size() >= 3) {
    *offset_r = values[0];
    *offset_g = values[1];
    *offset_b = values[2];
  }
}

RectExpand expand_from_configs(const std::unordered_map<std::string, std::string> &configs)
{
  RectExpand expand(
      parse_int(configs, "infer-width", kDefaultInferWidth),
      parse_int(configs, "infer-height", kDefaultInferHeight),
      parse_float(configs, "padding", kDefaultPadding));
  return expand;
}

}  // namespace

class RtmposePreprocess {
 public:
  RtmposePreprocess(
      int width,
      int height,
      int channels,
      float scale,
      float offset_r,
      float offset_g,
      float offset_b,
      RectExpand expand)
      : expand(expand)
  {
    this->width = width;
    this->height = height;
    this->channels = channels;
    this->scale = scale;
    this->offset_r = offset_r;
    this->offset_g = offset_g;
    this->offset_b = offset_b;
    this->sample_floats = this->channels * this->height * this->width;
  }

  NvDsPreProcessStatus prepare(
      NvDsPreProcessBatch *batch,
      NvDsPreProcessCustomBuf *&buf,
      CustomTensorParams &tensorParam,
      NvDsPreProcessAcquirer *acquirer)
  {
    buf = acquirer->acquire();
    int count = static_cast<int>(batch->units.size());
    NvDsPreProcessStatus status = NVDSPREPROCESS_SUCCESS;
    if (!tensorParam.params.network_input_shape.empty()) {
      tensorParam.params.network_input_shape[0] = count;
    }
    tensorParam.params.buffer_size =
        static_cast<guint64>(count) * static_cast<guint64>(sample_floats) * sizeof(float);
    for (int i = 0; i < count; ++i) {
      attach_crop_meta(batch->units[i]);
      NvBufSurfaceParams *converted = batch->units[i].roi_meta.converted_buffer;
      unsigned char *src = nullptr;
      int pitch = width * 4;
      if (converted != nullptr) {
        src = static_cast<unsigned char *>(converted->dataPtr);
        pitch = static_cast<int>(converted->pitch);
      }
      if (src == nullptr && batch->units[i].converted_frame_ptr != nullptr) {
        src = static_cast<unsigned char *>(batch->units[i].converted_frame_ptr);
      }
      if (src == nullptr) {
        status = NVDSPREPROCESS_CUSTOM_TENSOR_FAILED;
        break;
      }
      float *dst = static_cast<float *>(buf->memory_ptr) + i * sample_floats;
      rgba_to_nchw(
          src,
          pitch,
          width,
          height,
          dst,
          scale,
          offset_r,
          offset_g,
          offset_b);
    }
    return status;
  }

 private:
  NvDsObjectMeta *unit_object_meta(const NvDsPreProcessUnit &unit)
  {
    NvDsObjectMeta *object_meta = unit.obj_meta;
    if (object_meta == nullptr) {
      object_meta = unit.roi_meta.object_meta;
    }
    return object_meta;
  }

  NvDsFrameMeta *unit_frame_meta(const NvDsPreProcessUnit &unit)
  {
    NvDsFrameMeta *frame_meta = unit.frame_meta;
    if (frame_meta == nullptr) {
      frame_meta = unit.roi_meta.frame_meta;
    }
    return frame_meta;
  }

  std::pair<int, int> frame_hw(NvDsFrameMeta *frame_meta)
  {
    int width = 0;
    int height = 0;
    if (frame_meta != nullptr) {
      width = static_cast<int>(frame_meta->pipeline_width);
      height = static_cast<int>(frame_meta->pipeline_height);
      if (width <= 0) {
        width = static_cast<int>(frame_meta->source_frame_width);
      }
      if (height <= 0) {
        height = static_cast<int>(frame_meta->source_frame_height);
      }
    }
    return {width, height};
  }

  void attach_crop_meta(const NvDsPreProcessUnit &unit)
  {
    NvDsObjectMeta *object_meta = unit_object_meta(unit);
    NvDsFrameMeta *frame_meta = unit_frame_meta(unit);
    std::pair<int, int> hw = frame_hw(frame_meta);
    NvDsBatchMeta *batch_meta = nullptr;
    if (frame_meta != nullptr) {
      batch_meta = frame_meta->base_meta.batch_meta;
    }
    if (object_meta != nullptr && batch_meta != nullptr && hw.first > 0 && hw.second > 0) {
      CropGeom geom = expand.compute_crop(
          object_meta->rect_params.left,
          object_meta->rect_params.top,
          object_meta->rect_params.width,
          object_meta->rect_params.height,
          hw.first,
          hw.second);
      auto *crop = static_cast<NvDsRtmposeCropMeta *>(g_malloc0(sizeof(NvDsRtmposeCropMeta)));
      crop->src_left = geom.src_left;
      crop->src_top = geom.src_top;
      crop->src_width = geom.src_width;
      crop->src_height = geom.src_height;
      crop->dest_width = geom.dest_width;
      crop->dest_height = geom.dest_height;
      crop->offset_left = geom.offset_left;
      crop->offset_top = geom.offset_top;
      crop->infer_width = expand.infer_width;
      crop->infer_height = expand.infer_height;
      NvDsUserMeta *existing = nullptr;
      for (NvDsMetaList *item = object_meta->obj_user_meta_list; item != nullptr;
           item = item->next) {
        auto *user_meta = static_cast<NvDsUserMeta *>(item->data);
        if (user_meta != nullptr &&
            user_meta->base_meta.meta_type == NVDS_RTMPOSE_CROP_USER_META) {
          existing = user_meta;
        }
      }
      if (existing != nullptr) {
        g_free(existing->user_meta_data);
        existing->user_meta_data = crop;
      } else {
        NvDsUserMeta *user_meta = nvds_acquire_user_meta_from_pool(batch_meta);
        if (user_meta != nullptr) {
          user_meta->user_meta_data = crop;
          user_meta->base_meta.meta_type = NVDS_RTMPOSE_CROP_USER_META;
          user_meta->base_meta.copy_func = nvds_rtmpose_crop_meta_copy;
          user_meta->base_meta.release_func = nvds_rtmpose_crop_meta_release;
          nvds_add_user_meta_to_obj(object_meta, user_meta);
        } else {
          g_free(crop);
        }
      }
    }
  }

  RectExpand expand;
  int width;
  int height;
  int channels;
  int sample_floats;
  float scale;
  float offset_r;
  float offset_g;
  float offset_b;
};

struct CustomCtx {
  RtmposePreprocess preprocess;

  CustomCtx(
      int width,
      int height,
      int channels,
      float scale,
      float offset_r,
      float offset_g,
      float offset_b,
      RectExpand expand)
      : preprocess(width, height, channels, scale, offset_r, offset_g, offset_b, expand)
  {
  }
};

class RtmposeTransformConfig {
 public:
  static std::unordered_map<std::string, std::string> user_configs;
};

std::unordered_map<std::string, std::string> RtmposeTransformConfig::user_configs;

class RtmposeCropper {
 public:
  RtmposeCropper(const std::unordered_map<std::string, std::string> &configs)
      : expand(kDefaultInferWidth, kDefaultInferHeight, kDefaultPadding)
  {
    this->expand = expand_from_configs(configs);
  }

  NvDsPreProcessStatus apply(
      NvBufSurface *in_surf,
      NvBufSurface *out_surf,
      CustomTransformParams &params)
  {
    NvDsPreProcessStatus status = NVDSPREPROCESS_SUCCESS;
    if (in_surf == nullptr || out_surf == nullptr || params.transform_params.src_rect == nullptr ||
        params.transform_params.dst_rect == nullptr) {
      status = NVDSPREPROCESS_CUSTOM_TRANSFORMATION_FAILED;
    }
    if (status == NVDSPREPROCESS_SUCCESS) {
    NvBufSurfaceMemSet(out_surf, -1, -1, 0);
    guint batch_size = in_surf->numFilled;
    for (guint i = 0; i < batch_size; ++i) {
      int frame_width = static_cast<int>(in_surf->surfaceList[i].width);
      int frame_height = static_cast<int>(in_surf->surfaceList[i].height);
      NvBufSurfTransformRect src = params.transform_params.src_rect[i];
      CropGeom geom = expand.compute_crop(
          static_cast<float>(src.left),
          static_cast<float>(src.top),
          static_cast<float>(src.width),
          static_cast<float>(src.height),
          frame_width,
          frame_height);
      params.transform_params.src_rect[i].left = static_cast<uint32_t>(geom.src_left);
      params.transform_params.src_rect[i].top = static_cast<uint32_t>(geom.src_top);
      params.transform_params.src_rect[i].width = static_cast<uint32_t>(geom.src_width);
      params.transform_params.src_rect[i].height = static_cast<uint32_t>(geom.src_height);
      params.transform_params.dst_rect[i].left = static_cast<uint32_t>(geom.offset_left);
      params.transform_params.dst_rect[i].top = static_cast<uint32_t>(geom.offset_top);
      params.transform_params.dst_rect[i].width = static_cast<uint32_t>(geom.dest_width);
      params.transform_params.dst_rect[i].height = static_cast<uint32_t>(geom.dest_height);
    }
    NvBufSurfTransform_Error err =
        NvBufSurfTransform(in_surf, out_surf, &params.transform_params);
    if (err != NvBufSurfTransformError_Success) {
      status = NVDSPREPROCESS_CUSTOM_TRANSFORMATION_FAILED;
    }
    }
    return status;
  }

 private:
  RectExpand expand;
};

extern "C" NvDsPreProcessStatus CustomTransformation(
    NvBufSurface *in_surf,
    NvBufSurface *out_surf,
    CustomTransformParams &params)
{
  RtmposeCropper cropper(RtmposeTransformConfig::user_configs);
  NvDsPreProcessStatus status = cropper.apply(in_surf, out_surf, params);
  return status;
}

extern "C" NvDsPreProcessStatus CustomAsyncTransformation(
    NvBufSurface *in_surf,
    NvBufSurface *out_surf,
    CustomTransformParams &params)
{
  NvDsPreProcessStatus status = CustomTransformation(in_surf, out_surf, params);
  return status;
}

extern "C" NvDsPreProcessStatus CustomTensorPreparation(
    CustomCtx *ctx,
    NvDsPreProcessBatch *batch,
    NvDsPreProcessCustomBuf *&buf,
    CustomTensorParams &tensorParam,
    NvDsPreProcessAcquirer *acquirer)
{
  NvDsPreProcessStatus status = ctx->preprocess.prepare(batch, buf, tensorParam, acquirer);
  return status;
}

extern "C" CustomCtx *initLib(CustomInitParams initparams)
{
  int channels = 3;
  int height = kDefaultInferHeight;
  int width = kDefaultInferWidth;
  const auto &shape = initparams.tensor_params.network_input_shape;
  if (initparams.tensor_params.network_input_order == NvDsPreProcessNetworkInputOrder_kNCHW &&
      shape.size() >= 4) {
    channels = shape[1];
    height = shape[2];
    width = shape[3];
  }
  float scale = parse_float(
      initparams.user_configs,
      "pixel-normalization-factor",
      kDefaultScale);
  float offset_r = kDefaultOffsetR;
  float offset_g = kDefaultOffsetG;
  float offset_b = kDefaultOffsetB;
  parse_offsets(initparams.user_configs, &offset_r, &offset_g, &offset_b);
  RtmposeTransformConfig::user_configs = initparams.user_configs;
  RectExpand expand = expand_from_configs(initparams.user_configs);
  CustomCtx *ctx = new CustomCtx(
      width, height, channels, scale, offset_r, offset_g, offset_b, expand);
  return ctx;
}

extern "C" void deInitLib(CustomCtx *ctx)
{
  delete ctx;
}
