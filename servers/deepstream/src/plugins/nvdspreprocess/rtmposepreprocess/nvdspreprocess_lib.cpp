#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "nvbufsurface.h"
#include "nvbufsurftransform.h"
#include "nvdspreprocess_interface.h"
#include "nvdspreprocess_impl.h"

#include "constants/rtmpose_const.hpp"
#include "plugins/nvdspreprocess/rtmposepreprocess/engine/rect_expand.hpp"

namespace {

int parseInt(const std::unordered_map<std::string, std::string>& configs, const char* key, int fallback) {
    int value = fallback;
    auto it = configs.find(key);
    if (it != configs.end() && !it->second.empty()) {
        value = std::atoi(it->second.c_str());
    }
    return value;
}

float parseFloat(const std::unordered_map<std::string, std::string>& configs, const char* key, float fallback) {
    float value = fallback;
    auto it = configs.find(key);
    if (it != configs.end() && !it->second.empty()) {
        value = static_cast<float>(std::atof(it->second.c_str()));
    }
    return value;
}

RectExpand expandFrom(const std::unordered_map<std::string, std::string>& configs) {
    RectExpand expand(
        parseInt(configs, rtmpose::KEY_INFER_WIDTH, rtmpose::INFER_WIDTH),
        parseInt(configs, rtmpose::KEY_INFER_HEIGHT, rtmpose::INFER_HEIGHT),
        parseFloat(configs, rtmpose::KEY_PADDING, rtmpose::PADDING));
    return expand;
}

std::vector<float> parseOffsets(const std::unordered_map<std::string, std::string>& configs) {
    std::vector<float> offsets{rtmpose::OFFSET_R, rtmpose::OFFSET_G, rtmpose::OFFSET_B};
    auto it = configs.find(rtmpose::KEY_OFFSETS);
    std::vector<float> parsed;
    if (it != configs.end() && !it->second.empty()) {
        std::stringstream stream(it->second);
        std::string token;
        while (std::getline(stream, token, ';')) {
            if (!token.empty()) {
                parsed.push_back(static_cast<float>(std::atof(token.c_str())));
            }
        }
    }
    if (parsed.size() >= 3) {
        offsets = {parsed[0], parsed[1], parsed[2]};
    }
    return offsets;
}

}  // namespace

class RtmposeCrop {
public:
    static RectExpand expand;

    NvDsPreProcessStatus apply(NvBufSurface* inSurf, NvBufSurface* outSurf, CustomTransformParams& params) {
        NvDsPreProcessStatus status = NVDSPREPROCESS_SUCCESS;
        if (inSurf == nullptr || outSurf == nullptr || params.transform_params.src_rect == nullptr ||
            params.transform_params.dst_rect == nullptr) {
            status = NVDSPREPROCESS_CUSTOM_TRANSFORMATION_FAILED;
        }
        if (status == NVDSPREPROCESS_SUCCESS) {
            NvBufSurfaceMemSet(outSurf, -1, -1, 0);
            guint batchSize = inSurf->numFilled;
            for (guint i = 0; i < batchSize; ++i) {
                NvBufSurfTransformRect src = params.transform_params.src_rect[i];
                CropGeom geom = expand.compute(
                    static_cast<float>(src.left),
                    static_cast<float>(src.top),
                    static_cast<float>(src.width),
                    static_cast<float>(src.height),
                    static_cast<int>(inSurf->surfaceList[i].width),
                    static_cast<int>(inSurf->surfaceList[i].height));
                params.transform_params.src_rect[i].left = static_cast<uint32_t>(geom.srcLeft);
                params.transform_params.src_rect[i].top = static_cast<uint32_t>(geom.srcTop);
                params.transform_params.src_rect[i].width = static_cast<uint32_t>(geom.srcWidth);
                params.transform_params.src_rect[i].height = static_cast<uint32_t>(geom.srcHeight);
                params.transform_params.dst_rect[i].left = static_cast<uint32_t>(geom.offsetLeft);
                params.transform_params.dst_rect[i].top = static_cast<uint32_t>(geom.offsetTop);
                params.transform_params.dst_rect[i].width = static_cast<uint32_t>(geom.destWidth);
                params.transform_params.dst_rect[i].height = static_cast<uint32_t>(geom.destHeight);
            }
            NvBufSurfTransform_Error err = NvBufSurfTransform(inSurf, outSurf, &params.transform_params);
            if (err != NvBufSurfTransformError_Success) {
                status = NVDSPREPROCESS_CUSTOM_TRANSFORMATION_FAILED;
            }
        }
        return status;
    }
};

RectExpand RtmposeCrop::expand(rtmpose::INFER_WIDTH, rtmpose::INFER_HEIGHT, rtmpose::PADDING);

struct CustomCtx {
    RectExpand expand;
    std::unique_ptr<NvDsPreProcessTensorImpl> tensor;

    CustomCtx(RectExpand rectExpand, std::unique_ptr<NvDsPreProcessTensorImpl> tensorImpl)
        : expand(rectExpand),
          tensor(std::move(tensorImpl)) {
    }
};

extern "C" NvDsPreProcessStatus CustomTransformation(
    NvBufSurface* inSurf,
    NvBufSurface* outSurf,
    CustomTransformParams& params) {
    RtmposeCrop crop;
    NvDsPreProcessStatus status = crop.apply(inSurf, outSurf, params);
    return status;
}

extern "C" NvDsPreProcessStatus CustomAsyncTransformation(
    NvBufSurface* inSurf,
    NvBufSurface* outSurf,
    CustomTransformParams& params) {
    NvDsPreProcessStatus status = CustomTransformation(inSurf, outSurf, params);
    return status;
}

extern "C" NvDsPreProcessStatus CustomTensorPreparation(
    CustomCtx* ctx,
    NvDsPreProcessBatch* batch,
    NvDsPreProcessCustomBuf*& buf,
    CustomTensorParams& tensorParam,
    NvDsPreProcessAcquirer* acquirer) {
    NvDsPreProcessStatus status = NVDSPREPROCESS_CUSTOM_TENSOR_FAILED;
    buf = nullptr;
    if (ctx != nullptr && ctx->tensor != nullptr && acquirer != nullptr && batch != nullptr) {
        buf = acquirer->acquire();
    }
    if (buf != nullptr) {
        status = ctx->tensor->prepare_tensor(batch, tensorParam, buf->memory_ptr);
    }
    if (status == NVDSPREPROCESS_SUCCESS) {
        status = ctx->tensor->syncStream();
    }
    if (status == NVDSPREPROCESS_SUCCESS && !tensorParam.params.network_input_shape.empty()) {
        tensorParam.params.network_input_shape[0] = static_cast<int>(batch->units.size());
    }
    if (status != NVDSPREPROCESS_SUCCESS && buf != nullptr) {
        acquirer->release(buf);
    }
    return status;
}

extern "C" CustomCtx* initLib(CustomInitParams initparams) {
    RectExpand expand = expandFrom(initparams.user_configs);
    RtmposeCrop::expand = expand;
    CustomMeanSubandNormParams meanNorm;
    meanNorm.pixel_normalization_factor = parseFloat(
        initparams.user_configs,
        rtmpose::KEY_PIXEL_NORMALIZATION_FACTOR,
        rtmpose::PIXEL_NORMALIZATION_FACTOR);
    meanNorm.offsets = parseOffsets(initparams.user_configs);
    std::unique_ptr<NvDsPreProcessTensorImpl> tensor;
    NvDsPreProcessStatus status = normalization_mean_subtraction_impl_initialize(
        &meanNorm,
        &initparams.tensor_params,
        tensor,
        static_cast<int>(initparams.unique_id));
    CustomCtx* ctx = nullptr;
    if (status == NVDSPREPROCESS_SUCCESS) {
        ctx = new CustomCtx(expand, std::move(tensor));
    }
    return ctx;
}

extern "C" void deInitLib(CustomCtx* ctx) {
    delete ctx;
}
