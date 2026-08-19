/*
 * SPDX-FileCopyrightText: Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: LicenseRef-NvidiaProprietary
 *
 * Adapted from deepstream_tao_apps pose-classification nvdspreprocess_lib
 * for PYSKL ST-GCN++: input layout (N, M, T, V, C) = (N, 2, 100, 17, 3).
 */

#ifndef NVDSPREPROCESS_LIB_H
#define NVDSPREPROCESS_LIB_H

#include <string>
#include <vector>

#include "nvbufsurface.h"
#include "nvbufsurftransform.h"
#include "nvdspreprocess_interface.h"

#define NVDSPREPROCESS_USER_CONFIGS_FRAMES_SEQUENCE_LENGTH "frames-sequence-length"
#define NVDSPREPROCESS_USER_CONFIGS_FRAMES_SEQUENCE_LENGHTH \
    NVDSPREPROCESS_USER_CONFIGS_FRAMES_SEQUENCE_LENGTH

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
