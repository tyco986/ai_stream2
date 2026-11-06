#include <cstdlib>
#include <string>
#include <unordered_map>
#include <vector>

#include "nvbufsurface.h"
#include "nvdspreprocess_interface.h"

#include "constants/stgcnpp_const.hpp"
#include "plugins/nvdspreprocess/stgcnpppreprocess/engine/stgcnpp_preprocess.hpp"

namespace {

int parseInt(const std::unordered_map<std::string, std::string>& configs, const char* key, int fallback) {
    int value = fallback;
    auto it = configs.find(key);
    if (it != configs.end() && !it->second.empty()) {
        value = std::atoi(it->second.c_str());
    }
    return value;
}

}  // namespace

struct CustomCtx {
    StgcnppPreprocess preprocess;

    CustomCtx(int clipLen, int numJoints, int numPerson, int patience)
        : preprocess(clipLen, numJoints, numPerson, patience) {
    }
};

extern "C" NvDsPreProcessStatus CustomTransformation(
    NvBufSurface* inSurf,
    NvBufSurface* outSurf,
    CustomTransformParams& params) {
    (void)inSurf;
    (void)outSurf;
    (void)params;
    return NVDSPREPROCESS_SUCCESS;
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
    NvDsPreProcessStatus status = ctx->preprocess.prepare(batch, buf, tensorParam, acquirer);
    return status;
}

extern "C" CustomCtx* initLib(CustomInitParams initparams) {
    int clipLen = parseInt(initparams.user_configs, stgcnpp::KEY_CLIP_LEN, stgcnpp::CLIP_LEN);
    if (clipLen < stgcnpp::MIN_CLIP_LEN || clipLen > stgcnpp::MAX_CLIP_LEN) {
        clipLen = stgcnpp::CLIP_LEN;
    }
    int numJoints = parseInt(initparams.user_configs, stgcnpp::KEY_NUM_JOINTS, stgcnpp::NUM_JOINTS);
    int numPerson = parseInt(initparams.user_configs, stgcnpp::KEY_NUM_PERSON, stgcnpp::NUM_PERSON);
    if (numPerson < 1) {
        numPerson = stgcnpp::NUM_PERSON;
    }
    int patience = parseInt(initparams.user_configs, stgcnpp::KEY_PATIENCE, stgcnpp::PATIENCE);
    if (patience < 0) {
        patience = stgcnpp::PATIENCE;
    }
    return new CustomCtx(clipLen, numJoints, numPerson, patience);
}

extern "C" void deInitLib(CustomCtx* ctx) {
    delete ctx;
}
