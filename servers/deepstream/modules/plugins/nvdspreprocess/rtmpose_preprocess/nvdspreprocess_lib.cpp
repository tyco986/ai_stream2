#include "nvdspreprocess_lib.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

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
      float offset_b)
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
      float offset_b)
      : preprocess(width, height, channels, scale, offset_r, offset_g, offset_b)
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
      float left = 0.0f;
      float top = 0.0f;
      float width = 0.0f;
      float height = 0.0f;
      expand.expand(
          static_cast<float>(src.left),
          static_cast<float>(src.top),
          static_cast<float>(src.width),
          static_cast<float>(src.height),
          frame_width,
          frame_height,
          &left,
          &top,
          &width,
          &height);
      int src_left = 0;
      int src_top = 0;
      int src_width = 0;
      int src_height = 0;
      expand.even_src(left, top, width, height, &src_left, &src_top, &src_width, &src_height);
      if (src_left + src_width > frame_width) {
        src_width = std::max(2, round_down_2(frame_width - src_left));
      }
      if (src_top + src_height > frame_height) {
        src_height = std::max(2, round_down_2(frame_height - src_top));
      }
      int dest_width = 0;
      int dest_height = 0;
      int offset_left = 0;
      int offset_top = 0;
      expand.letterbox(src_width, src_height, &dest_width, &dest_height, &offset_left, &offset_top);
      params.transform_params.src_rect[i].left = static_cast<uint32_t>(src_left);
      params.transform_params.src_rect[i].top = static_cast<uint32_t>(src_top);
      params.transform_params.src_rect[i].width = static_cast<uint32_t>(src_width);
      params.transform_params.src_rect[i].height = static_cast<uint32_t>(src_height);
      params.transform_params.dst_rect[i].left = static_cast<uint32_t>(offset_left);
      params.transform_params.dst_rect[i].top = static_cast<uint32_t>(offset_top);
      params.transform_params.dst_rect[i].width = static_cast<uint32_t>(dest_width);
      params.transform_params.dst_rect[i].height = static_cast<uint32_t>(dest_height);
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

  int round_down_2(int value) const
  {
    return value & ~1;
  }
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
  CustomCtx *ctx = new CustomCtx(width, height, channels, scale, offset_r, offset_g, offset_b);
  return ctx;
}

extern "C" void deInitLib(CustomCtx *ctx)
{
  delete ctx;
}
