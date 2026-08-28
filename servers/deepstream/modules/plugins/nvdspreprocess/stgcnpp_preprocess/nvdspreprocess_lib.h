#ifndef NVDSPREPROCESS_LIB_H
#define NVDSPREPROCESS_LIB_H

#include <string>
#include <unordered_map>
#include <vector>

#include "nvbufsurface.h"
#include "nvbufsurftransform.h"
#include "nvdspreprocess_interface.h"

extern "C" NvDsPreProcessStatus CustomTransformation(
    NvBufSurface *in_surf,
    NvBufSurface *out_surf,
    CustomTransformParams &params);

extern "C" NvDsPreProcessStatus CustomAsyncTransformation(
    NvBufSurface *in_surf,
    NvBufSurface *out_surf,
    CustomTransformParams &params);

extern "C" NvDsPreProcessStatus CustomTensorPreparation(
    CustomCtx *ctx,
    NvDsPreProcessBatch *batch,
    NvDsPreProcessCustomBuf *&buf,
    CustomTensorParams &tensorParam,
    NvDsPreProcessAcquirer *acquirer);

extern "C" CustomCtx *initLib(CustomInitParams initparams);

extern "C" void deInitLib(CustomCtx *ctx);

#endif
