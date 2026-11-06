#include <cstdlib>
#include <string>
#include <unordered_map>
#include <vector>

#include "nvbufsurface.h"
#include "nvbufsurftransform.h"
#include "nvdspreprocess_interface.h"

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

    explicit CustomCtx(RectExpand rectExpand) : expand(rectExpand) {
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

extern "C" CustomCtx* initLib(CustomInitParams initparams) {
    RectExpand expand = expandFrom(initparams.user_configs);
    RtmposeCrop::expand = expand;
    CustomCtx* ctx = new CustomCtx(expand);
    return ctx;
}

extern "C" void deInitLib(CustomCtx* ctx) {
    delete ctx;
}
