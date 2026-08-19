/*
 * SPDX-FileCopyrightText: Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: LicenseRef-NvidiaProprietary
 *
 * Adapted from deepstream_tao_apps pose-classification nvdspreprocess_lib.
 * Reads RTMPose nvinfer tensor meta (layer "keypoints", COCO-17, x/y/score),
 * maps crop coords to the frame, and packs ST-GCN++ input (N, M, T, V, C).
 */

#include "nvdspreprocess_lib.h"

#include <cuda_runtime_api.h>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <sys/time.h>
#include <unordered_map>
#include <vector>

#include "gstnvdsinfer.h"
#include "gstnvdsmeta.h"
#include "nvdsinfer.h"
#include "nvdsmeta.h"

namespace {

constexpr int kMinClipLen = 3;
constexpr int kMaxClipLen = 300;
constexpr int kDefaultClipLen = 100;
constexpr int kDefaultNumJoints = 17;
constexpr int kDefaultNumPerson = 2;
constexpr int kDefaultChannels = 3;
constexpr int kDefaultPoseGieId = 2;
constexpr int kDefaultInferWidth = 192;
constexpr int kDefaultInferHeight = 256;
constexpr int kObjectTimeoutSec = 2;
constexpr char kKeypointsLayer[] = "keypoints";

template <typename ConfigMap>
int parse_int(const ConfigMap &configs, const char *key, int fallback)
{
    int value = fallback;
    auto it = configs.find(key);
    if (it != configs.end() && !it->second.empty()) {
        value = atoi(it->second.c_str());
    }
    return value;
}

int round_up_2(int value)
{
    return (value + 1) & ~1;
}

int round_down_2(int value)
{
    return value & ~1;
}

struct ObjectKey {
    gint source_id = 0;
    guint64 object_id = UNTRACKED_OBJECT_ID;

    bool operator==(const ObjectKey &other) const
    {
        bool same = source_id == other.source_id && object_id == other.object_id;
        return same;
    }
};

struct ObjectKeyHash {
    size_t operator()(const ObjectKey &key) const
    {
        size_t hashed = static_cast<size_t>(key.source_id);
        hashed ^= static_cast<size_t>(key.object_id);
        hashed ^= static_cast<size_t>(key.object_id >> 32);
        return hashed;
    }
};

struct TrackBuffer {
    ObjectKey key;
    std::vector<float> sequence;
    long tv_sec = 0;
};

}  // namespace

struct CustomCtx {
    std::mutex mutex;
    std::unordered_map<ObjectKey, TrackBuffer, ObjectKeyHash> tracks;
    int clip_len = kDefaultClipLen;
    int num_joints = kDefaultNumJoints;
    int num_person = kDefaultNumPerson;
    int channels = kDefaultChannels;
    int pose_gie_id = kDefaultPoseGieId;
    int infer_width = kDefaultInferWidth;
    int infer_height = kDefaultInferHeight;
    int frame_floats = 0;
    int person_floats = 0;
    int sample_floats = 0;

    int frame_count() const
    {
        return num_joints * channels;
    }
};

void map_crop_to_frame(
    CustomCtx *ctx,
    const float *crop_xy_score,
    NvDsObjectMeta *object_meta,
    float *frame_xy_score)
{
    int src_left = round_up_2(static_cast<int>(object_meta->rect_params.left));
    int src_top = round_up_2(static_cast<int>(object_meta->rect_params.top));
    int src_width = round_down_2(static_cast<int>(object_meta->rect_params.width));
    int src_height = round_down_2(static_cast<int>(object_meta->rect_params.height));
    if (src_width < 2) {
        src_width = 2;
    }
    if (src_height < 2) {
        src_height = 2;
    }
    float fit_height = ctx->infer_width * src_height / static_cast<float>(src_width);
    int dest_width = ctx->infer_width;
    int dest_height = static_cast<int>(fit_height);
    if (fit_height > ctx->infer_height) {
        dest_width = static_cast<int>(ctx->infer_height * src_width / static_cast<float>(src_height));
        dest_height = ctx->infer_height;
    }
    int offset_left = (ctx->infer_width - dest_width) / 2;
    int offset_top = (ctx->infer_height - dest_height) / 2;
    float ratio_x = dest_width / static_cast<float>(src_width);
    float ratio_y = dest_height / static_cast<float>(src_height);
    for (int joint = 0; joint < ctx->num_joints; ++joint) {
        float x = crop_xy_score[joint * 3 + 0];
        float y = crop_xy_score[joint * 3 + 1];
        float score = crop_xy_score[joint * 3 + 2];
        frame_xy_score[joint * 3 + 0] = src_left + (x - offset_left) / ratio_x;
        frame_xy_score[joint * 3 + 1] = src_top + (y - offset_top) / ratio_y;
        frame_xy_score[joint * 3 + 2] = score;
    }
}

const float *keypoints_from_tensor(CustomCtx *ctx, NvDsInferTensorMeta *tensor_meta)
{
    const float *data = nullptr;
    if (tensor_meta == nullptr || static_cast<int>(tensor_meta->unique_id) != ctx->pose_gie_id) {
        data = nullptr;
    } else {
        for (guint i = 0; i < tensor_meta->num_output_layers; ++i) {
            const NvDsInferLayerInfo &layer = tensor_meta->output_layers_info[i];
            if (layer.layerName == nullptr || strcmp(layer.layerName, kKeypointsLayer) != 0) {
                continue;
            }
            int elements = static_cast<int>(layer.inferDims.numElements);
            if (elements < ctx->frame_count()) {
                continue;
            }
            data = static_cast<const float *>(tensor_meta->out_buf_ptrs_host[i]);
            if (data == nullptr && tensor_meta->out_buf_ptrs_dev[i] != nullptr) {
                continue;
            }
        }
    }
    return data;
}

const float *keypoints_from_object(CustomCtx *ctx, NvDsObjectMeta *object_meta)
{
    const float *data = nullptr;
    for (NvDsMetaList *user = object_meta->obj_user_meta_list; user != nullptr; user = user->next) {
        NvDsUserMeta *user_meta = static_cast<NvDsUserMeta *>(user->data);
        if (user_meta->base_meta.meta_type != NVDSINFER_TENSOR_OUTPUT_META) {
            continue;
        }
        auto *tensor_meta = static_cast<NvDsInferTensorMeta *>(user_meta->user_meta_data);
        data = keypoints_from_tensor(ctx, tensor_meta);
        if (data != nullptr) {
            break;
        }
    }
    return data;
}

void push_frame(CustomCtx *ctx, TrackBuffer *track, const float *frame_xy_score)
{
    int shift = ctx->person_floats - ctx->frame_count();
    if (shift > 0) {
        memmove(track->sequence.data(), track->sequence.data() + ctx->frame_count(), shift * sizeof(float));
    }
    memcpy(
        track->sequence.data() + shift,
        frame_xy_score,
        static_cast<size_t>(ctx->frame_count()) * sizeof(float));
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    track->tv_sec = tv.tv_sec;
}

TrackBuffer *ensure_track(CustomCtx *ctx, const ObjectKey &key)
{
    auto it = ctx->tracks.find(key);
    TrackBuffer *track = nullptr;
    if (it != ctx->tracks.end()) {
        track = &it->second;
    } else {
        TrackBuffer created;
        created.key = key;
        created.sequence.assign(static_cast<size_t>(ctx->person_floats), 0.0f);
        auto inserted = ctx->tracks.emplace(key, std::move(created));
        track = &inserted.first->second;
    }
    return track;
}

void drop_stale_tracks(CustomCtx *ctx)
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    for (auto it = ctx->tracks.begin(); it != ctx->tracks.end();) {
        if (it->second.key.object_id != UNTRACKED_OBJECT_ID &&
            (tv.tv_sec - it->second.tv_sec) > kObjectTimeoutSec) {
            it = ctx->tracks.erase(it);
        } else {
            ++it;
        }
    }
}

void write_sample(CustomCtx *ctx, float *dst, const TrackBuffer *track)
{
    if (track != nullptr) {
        cudaMemcpy(dst, track->sequence.data(), ctx->person_floats * sizeof(float), cudaMemcpyHostToDevice);
    } else {
        cudaMemset(dst, 0, ctx->person_floats * sizeof(float));
    }
    float *person1 = dst + ctx->person_floats;
    cudaMemset(person1, 0, (ctx->sample_floats - ctx->person_floats) * sizeof(float));
}

extern "C" NvDsPreProcessStatus CustomTransformation(
    NvBufSurface *in_surf,
    NvBufSurface *out_surf,
    CustomTransformParams &params)
{
    (void)in_surf;
    (void)out_surf;
    (void)params;
    NvDsPreProcessStatus status = NVDSPREPROCESS_SUCCESS;
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
    (void)tensorParam;
    buf = acquirer->acquire();
    float *dst = static_cast<float *>(buf->memory_ptr);
    std::vector<float> frame_xy_score(static_cast<size_t>(ctx->frame_count()), 0.0f);

    std::unique_lock<std::mutex> lock(ctx->mutex);
    int units = static_cast<int>(batch->units.size());
    for (int i = 0; i < units; ++i) {
        NvDsObjectMeta *object_meta = batch->units[i].roi_meta.object_meta;
        NvDsFrameMeta *frame_meta = batch->units[i].roi_meta.frame_meta;
        ObjectKey key;
        key.source_id = frame_meta != nullptr ? frame_meta->source_id : 0;
        key.object_id = object_meta != nullptr ? object_meta->object_id : UNTRACKED_OBJECT_ID;
        TrackBuffer *track = nullptr;
        if (object_meta != nullptr) {
            const float *crop = keypoints_from_object(ctx, object_meta);
            if (crop != nullptr) {
                map_crop_to_frame(ctx, crop, object_meta, frame_xy_score.data());
                track = ensure_track(ctx, key);
                push_frame(ctx, track, frame_xy_score.data());
            } else {
                auto it = ctx->tracks.find(key);
                if (it != ctx->tracks.end()) {
                    track = &it->second;
                }
            }
        }
        write_sample(ctx, dst + i * ctx->sample_floats, track);
    }
    drop_stale_tracks(ctx);
    NvDsPreProcessStatus status = NVDSPREPROCESS_SUCCESS;
    return status;
}

extern "C" CustomCtx *initLib(CustomInitParams initparams)
{
    CustomCtx *ctx = new CustomCtx();
    ctx->clip_len = parse_int(initparams.user_configs, NVDSPREPROCESS_USER_CONFIGS_FRAMES_SEQUENCE_LENGTH, kDefaultClipLen);
    if (ctx->clip_len < kMinClipLen || ctx->clip_len > kMaxClipLen) {
        ctx->clip_len = kDefaultClipLen;
    }
    ctx->num_joints = parse_int(initparams.user_configs, "num-joints", kDefaultNumJoints);
    ctx->num_person = parse_int(initparams.user_configs, "num-person", kDefaultNumPerson);
    ctx->pose_gie_id = parse_int(initparams.user_configs, "pose-gie-id", kDefaultPoseGieId);
    ctx->infer_width = parse_int(initparams.user_configs, "infer-width", kDefaultInferWidth);
    ctx->infer_height = parse_int(initparams.user_configs, "infer-height", kDefaultInferHeight);
    if (ctx->num_person < 1) {
        ctx->num_person = kDefaultNumPerson;
    }
    ctx->channels = kDefaultChannels;
    ctx->frame_floats = ctx->num_joints * ctx->channels;
    ctx->person_floats = ctx->clip_len * ctx->frame_floats;
    ctx->sample_floats = ctx->num_person * ctx->person_floats;
    return ctx;
}

extern "C" void deInitLib(CustomCtx *ctx)
{
    delete ctx;
}
