#include "nvdspreprocess_lib.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <cuda_runtime_api.h>

#include "gstnvdsinfer.h"
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
constexpr char kKeypointsLayer[] = "keypoints";

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

int round_up_2(int value)
{
    return (value + 1) & ~1;
}

int round_down_2(int value)
{
    return value & ~1;
}

struct TrackKey {
    gint source_id = 0;
    guint64 object_id = UNTRACKED_OBJECT_ID;

    bool operator==(const TrackKey &other) const
    {
        bool same = source_id == other.source_id && object_id == other.object_id;
        return same;
    }
};

struct TrackKeyHash {
    size_t operator()(const TrackKey &key) const
    {
        size_t hashed = static_cast<size_t>(key.source_id);
        hashed ^= static_cast<size_t>(key.object_id);
        hashed ^= static_cast<size_t>(key.object_id >> 32);
        return hashed;
    }
};

struct FrameGroup {
    gint source_id = 0;
    guint frame_num = 0;
    NvDsFrameMeta *frame_meta = nullptr;
    std::vector<int> unit_indices;
};

}  // namespace

class StgcnppPreprocess {
public:
    StgcnppPreprocess(
        int clip_len,
        int num_joints,
        int num_person,
        int pose_gie_id,
        int infer_width,
        int infer_height);

    NvDsPreProcessStatus prepare(
        NvDsPreProcessBatch *batch,
        NvDsPreProcessCustomBuf *&buf,
        CustomTensorParams &tensorParam,
        NvDsPreProcessAcquirer *acquirer);

    NvDsObjectMeta *unit_object_meta(const NvDsPreProcessUnit &unit);
    NvDsFrameMeta *unit_frame_meta(const NvDsPreProcessUnit &unit);
    TrackKey unit_key(const NvDsPreProcessUnit &unit);
    const float *keypoints_from_tensor(NvDsInferTensorMeta *tensor_meta);
    const float *keypoints_from_object(NvDsObjectMeta *object_meta);
    void map_crop_to_frame(
        const float *crop_xy_score,
        NvDsObjectMeta *object_meta,
        float *frame_xy_score);
    std::pair<int, int> frame_hw(NvDsFrameMeta *frame_meta);
    void prenormalize_2d(float *frame_xy_score, int frame_width, int frame_height);
    void collect_current_ids(NvDsFrameMeta *frame_meta, std::unordered_set<guint64> *current_ids);
    void drop_dead_tracks(gint source_id, const std::unordered_set<guint64> &current_ids);
    void append_pose(const TrackKey &key, const float *frame_xy_score);
    bool track_ready(const TrackKey &key);
    void write_clip(float *dst, const std::deque<std::vector<float>> &clip);

private:
    int clip_len;
    int num_joints;
    int num_person;
    int pose_gie_id;
    int infer_width;
    int infer_height;
    int channels;
    int frame_floats;
    int person_floats;
    int sample_floats;
    std::mutex mutex;
    std::unordered_map<TrackKey, std::deque<std::vector<float>>, TrackKeyHash> poses;
    std::vector<float> device_scratch;
};

struct CustomCtx {
    StgcnppPreprocess preprocess;

    CustomCtx(
        int clip_len,
        int num_joints,
        int num_person,
        int pose_gie_id,
        int infer_width,
        int infer_height)
        : preprocess(
              clip_len,
              num_joints,
              num_person,
              pose_gie_id,
              infer_width,
              infer_height)
    {
    }
};

StgcnppPreprocess::StgcnppPreprocess(
    int clip_len,
    int num_joints,
    int num_person,
    int pose_gie_id,
    int infer_width,
    int infer_height)
{
    this->clip_len = clip_len;
    this->num_joints = num_joints;
    this->num_person = num_person;
    this->pose_gie_id = pose_gie_id;
    this->infer_width = infer_width;
    this->infer_height = infer_height;
    this->channels = kDefaultChannels;
    this->frame_floats = this->num_joints * this->channels;
    this->person_floats = this->clip_len * this->frame_floats;
    this->sample_floats = this->num_person * this->person_floats;
}

NvDsObjectMeta *StgcnppPreprocess::unit_object_meta(const NvDsPreProcessUnit &unit)
{
    NvDsObjectMeta *object_meta = unit.obj_meta;
    if (object_meta == nullptr) {
        object_meta = unit.roi_meta.object_meta;
    }
    return object_meta;
}

NvDsFrameMeta *StgcnppPreprocess::unit_frame_meta(const NvDsPreProcessUnit &unit)
{
    NvDsFrameMeta *frame_meta = unit.frame_meta;
    if (frame_meta == nullptr) {
        frame_meta = unit.roi_meta.frame_meta;
    }
    return frame_meta;
}

TrackKey StgcnppPreprocess::unit_key(const NvDsPreProcessUnit &unit)
{
    TrackKey key;
    NvDsFrameMeta *frame_meta = unit_frame_meta(unit);
    NvDsObjectMeta *object_meta = unit_object_meta(unit);
    if (frame_meta != nullptr) {
        key.source_id = frame_meta->source_id;
    }
    if (object_meta != nullptr) {
        key.object_id = object_meta->object_id;
    }
    return key;
}

const float *StgcnppPreprocess::keypoints_from_tensor(NvDsInferTensorMeta *tensor_meta)
{
    const float *data = nullptr;
    if (tensor_meta != nullptr && static_cast<int>(tensor_meta->unique_id) == pose_gie_id) {
        for (guint i = 0; i < tensor_meta->num_output_layers; ++i) {
            const NvDsInferLayerInfo &layer = tensor_meta->output_layers_info[i];
            if (layer.layerName == nullptr || strcmp(layer.layerName, kKeypointsLayer) != 0) {
                continue;
            }
            int elements = static_cast<int>(layer.inferDims.numElements);
            if (elements < frame_floats) {
                continue;
            }
            data = static_cast<const float *>(tensor_meta->out_buf_ptrs_host[i]);
            if (data == nullptr && tensor_meta->out_buf_ptrs_dev[i] != nullptr) {
                device_scratch.resize(static_cast<size_t>(elements));
                cudaMemcpy(
                    device_scratch.data(),
                    tensor_meta->out_buf_ptrs_dev[i],
                    static_cast<size_t>(elements) * sizeof(float),
                    cudaMemcpyDeviceToHost);
                data = device_scratch.data();
            }
        }
    }
    return data;
}

const float *StgcnppPreprocess::keypoints_from_object(NvDsObjectMeta *object_meta)
{
    const float *data = nullptr;
    for (NvDsMetaList *user = object_meta->obj_user_meta_list; user != nullptr; user = user->next) {
        NvDsUserMeta *user_meta = static_cast<NvDsUserMeta *>(user->data);
        if (user_meta->base_meta.meta_type != NVDSINFER_TENSOR_OUTPUT_META) {
            continue;
        }
        auto *tensor_meta = static_cast<NvDsInferTensorMeta *>(user_meta->user_meta_data);
        data = keypoints_from_tensor(tensor_meta);
        if (data != nullptr) {
            break;
        }
    }
    return data;
}

void StgcnppPreprocess::map_crop_to_frame(
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
    float fit_height = infer_width * src_height / static_cast<float>(src_width);
    int dest_width = infer_width;
    int dest_height = static_cast<int>(fit_height);
    if (fit_height > infer_height) {
        dest_width = static_cast<int>(infer_height * src_width / static_cast<float>(src_height));
        dest_height = infer_height;
    }
    int offset_left = (infer_width - dest_width) / 2;
    int offset_top = (infer_height - dest_height) / 2;
    float ratio_x = dest_width / static_cast<float>(src_width);
    float ratio_y = dest_height / static_cast<float>(src_height);
    for (int joint = 0; joint < num_joints; ++joint) {
        float x = crop_xy_score[joint * 3 + 0];
        float y = crop_xy_score[joint * 3 + 1];
        float score = crop_xy_score[joint * 3 + 2];
        frame_xy_score[joint * 3 + 0] = src_left + (x - offset_left) / ratio_x;
        frame_xy_score[joint * 3 + 1] = src_top + (y - offset_top) / ratio_y;
        frame_xy_score[joint * 3 + 2] = score;
    }
}

std::pair<int, int> StgcnppPreprocess::frame_hw(NvDsFrameMeta *frame_meta)
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

void StgcnppPreprocess::prenormalize_2d(float *frame_xy_score, int frame_width, int frame_height)
{
    float half_w = 0.5f * static_cast<float>(frame_width);
    float half_h = 0.5f * static_cast<float>(frame_height);
    for (int joint = 0; joint < num_joints; ++joint) {
        frame_xy_score[joint * 3 + 0] = (frame_xy_score[joint * 3 + 0] - half_w) / half_w;
        frame_xy_score[joint * 3 + 1] = (frame_xy_score[joint * 3 + 1] - half_h) / half_h;
    }
}

void StgcnppPreprocess::collect_current_ids(
    NvDsFrameMeta *frame_meta,
    std::unordered_set<guint64> *current_ids)
{
    if (frame_meta == nullptr) {
        return;
    }
    for (NvDsMetaList *item = frame_meta->obj_meta_list; item != nullptr; item = item->next) {
        auto *object_meta = static_cast<NvDsObjectMeta *>(item->data);
        if (object_meta != nullptr && object_meta->object_id != UNTRACKED_OBJECT_ID) {
            current_ids->insert(object_meta->object_id);
        }
    }
}

void StgcnppPreprocess::drop_dead_tracks(
    gint source_id,
    const std::unordered_set<guint64> &current_ids)
{
    for (auto it = poses.begin(); it != poses.end();) {
        if (it->first.source_id == source_id && current_ids.count(it->first.object_id) == 0) {
            it = poses.erase(it);
        } else {
            ++it;
        }
    }
}

void StgcnppPreprocess::append_pose(const TrackKey &key, const float *frame_xy_score)
{
    std::deque<std::vector<float>> &clip = poses[key];
    clip.emplace_back(frame_xy_score, frame_xy_score + frame_floats);
    while (static_cast<int>(clip.size()) > clip_len) {
        clip.pop_front();
    }
}

bool StgcnppPreprocess::track_ready(const TrackKey &key)
{
    bool ready = false;
    auto it = poses.find(key);
    if (it != poses.end() && static_cast<int>(it->second.size()) == clip_len) {
        ready = true;
    }
    return ready;
}

void StgcnppPreprocess::write_clip(float *dst, const std::deque<std::vector<float>> &clip)
{
    int t = 0;
    for (const std::vector<float> &frame : clip) {
        memcpy(dst + t * frame_floats, frame.data(), static_cast<size_t>(frame_floats) * sizeof(float));
        t += 1;
    }
    if (num_person > 1) {
        memset(
            dst + person_floats,
            0,
            static_cast<size_t>(sample_floats - person_floats) * sizeof(float));
    }
}

NvDsPreProcessStatus StgcnppPreprocess::prepare(
    NvDsPreProcessBatch *batch,
    NvDsPreProcessCustomBuf *&buf,
    CustomTensorParams &tensorParam,
    NvDsPreProcessAcquirer *acquirer)
{
    buf = acquirer->acquire();
    std::unique_lock<std::mutex> lock(mutex);
    std::map<std::pair<gint, guint>, FrameGroup> groups;
    int units = static_cast<int>(batch->units.size());
    for (int i = 0; i < units; ++i) {
        NvDsFrameMeta *frame_meta = unit_frame_meta(batch->units[i]);
        gint source_id = 0;
        guint frame_num = static_cast<guint>(batch->units[i].frame_num);
        if (frame_meta != nullptr) {
            source_id = frame_meta->source_id;
            frame_num = frame_meta->frame_num;
        }
        FrameGroup &group = groups[std::make_pair(source_id, frame_num)];
        group.source_id = source_id;
        group.frame_num = frame_num;
        group.frame_meta = frame_meta;
        group.unit_indices.push_back(i);
    }
    for (auto &entry : groups) {
        FrameGroup &group = entry.second;
        std::unordered_set<guint64> current_ids;
        collect_current_ids(group.frame_meta, &current_ids);
        drop_dead_tracks(group.source_id, current_ids);
        std::pair<int, int> hw = frame_hw(group.frame_meta);
        for (int index : group.unit_indices) {
            NvDsObjectMeta *object_meta = unit_object_meta(batch->units[index]);
            TrackKey key = unit_key(batch->units[index]);
            if (object_meta == nullptr || key.object_id == UNTRACKED_OBJECT_ID) {
                continue;
            }
            const float *crop = keypoints_from_object(object_meta);
            if (crop == nullptr || hw.first <= 0 || hw.second <= 0) {
                continue;
            }
            std::vector<float> frame_xy_score(static_cast<size_t>(frame_floats), 0.0f);
            map_crop_to_frame(crop, object_meta, frame_xy_score.data());
            prenormalize_2d(frame_xy_score.data(), hw.first, hw.second);
            append_pose(key, frame_xy_score.data());
        }
    }

    std::vector<NvDsPreProcessUnit> ready_units;
    std::vector<NvDsRoiMeta> ready_rois;
    for (int i = 0; i < units; ++i) {
        TrackKey key = unit_key(batch->units[i]);
        if (key.object_id == UNTRACKED_OBJECT_ID || !track_ready(key)) {
            continue;
        }
        ready_units.push_back(batch->units[i]);
        ready_rois.push_back(batch->units[i].roi_meta);
    }
    int ready_count = static_cast<int>(ready_units.size());
    std::vector<float> host(static_cast<size_t>(std::max(ready_count, 0) * sample_floats), 0.0f);
    for (int i = 0; i < ready_count; ++i) {
        TrackKey key = unit_key(ready_units[i]);
        write_clip(host.data() + i * sample_floats, poses[key]);
    }
    if (ready_count > 0) {
        cudaMemcpy(
            buf->memory_ptr,
            host.data(),
            static_cast<size_t>(ready_count * sample_floats) * sizeof(float),
            cudaMemcpyHostToDevice);
    }
    batch->units = std::move(ready_units);
    tensorParam.seq_params.roi_vector = std::move(ready_rois);
    if (!tensorParam.params.network_input_shape.empty()) {
        tensorParam.params.network_input_shape[0] = ready_count;
    }
    tensorParam.params.buffer_size = static_cast<guint64>(ready_count) * static_cast<guint64>(sample_floats) *
        sizeof(float);
    NvDsPreProcessStatus status = NVDSPREPROCESS_SUCCESS;
    return status;
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
    NvDsPreProcessStatus status = ctx->preprocess.prepare(batch, buf, tensorParam, acquirer);
    return status;
}

extern "C" CustomCtx *initLib(CustomInitParams initparams)
{
    int clip_len = parse_int(initparams.user_configs, "frames-sequence-length", kDefaultClipLen);
    if (clip_len < kMinClipLen || clip_len > kMaxClipLen) {
        clip_len = kDefaultClipLen;
    }
    int num_joints = parse_int(initparams.user_configs, "num-joints", kDefaultNumJoints);
    int num_person = parse_int(initparams.user_configs, "num-person", kDefaultNumPerson);
    if (num_person < 1) {
        num_person = kDefaultNumPerson;
    }
    int pose_gie_id = parse_int(initparams.user_configs, "pose-gie-id", kDefaultPoseGieId);
    int infer_width = parse_int(initparams.user_configs, "infer-width", kDefaultInferWidth);
    int infer_height = parse_int(initparams.user_configs, "infer-height", kDefaultInferHeight);
    CustomCtx *ctx = new CustomCtx(
        clip_len,
        num_joints,
        num_person,
        pose_gie_id,
        infer_width,
        infer_height);
    return ctx;
}

extern "C" void deInitLib(CustomCtx *ctx)
{
    delete ctx;
}
