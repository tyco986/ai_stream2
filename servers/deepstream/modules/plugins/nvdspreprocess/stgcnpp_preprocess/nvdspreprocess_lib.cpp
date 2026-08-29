#include "nvdspreprocess_lib.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <cuda_runtime_api.h>

#include "nvds_stgcnpp_ready_meta.h"
#include "nvdsmeta.h"

namespace {

constexpr int kMinClipLen = 3;
constexpr int kMaxClipLen = 300;
constexpr int kDefaultClipLen = 100;
constexpr int kDefaultNumJoints = 17;
constexpr int kDefaultNumPerson = 2;
constexpr int kDefaultChannels = 3;

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
        int num_person);

    NvDsPreProcessStatus prepare(
        NvDsPreProcessBatch *batch,
        NvDsPreProcessCustomBuf *&buf,
        CustomTensorParams &tensorParam,
        NvDsPreProcessAcquirer *acquirer);

    NvDsObjectMeta *unit_object_meta(const NvDsPreProcessUnit &unit);
    NvDsFrameMeta *unit_frame_meta(const NvDsPreProcessUnit &unit);
    TrackKey unit_key(const NvDsPreProcessUnit &unit);
    const float *keypoints_from_mask(NvDsObjectMeta *object_meta);
    void map_mask_to_frame(
        const float *mask_xy_score,
        NvDsObjectMeta *object_meta,
        float *frame_xy_score);
    std::pair<int, int> frame_hw(NvDsFrameMeta *frame_meta);
    void prenormalize_2d(float *frame_xy_score, int frame_width, int frame_height);
    void collect_current_ids(NvDsFrameMeta *frame_meta, std::unordered_set<guint64> *current_ids);
    void drop_dead_tracks(gint source_id, const std::unordered_set<guint64> &current_ids);
    void append_pose(const TrackKey &key, const float *frame_xy_score);
    bool track_ready(const TrackKey &key);
    int track_clip_length(const TrackKey &key);
    void write_object_ready(
        NvDsBatchMeta *batch_meta,
        NvDsObjectMeta *object_meta,
        gboolean ready,
        gint length);
    void write_frame_ready(NvDsFrameMeta *frame_meta);
    void write_clip(float *dst, const std::deque<std::vector<float>> &clip);

private:
    int clip_len;
    int num_joints;
    int num_person;
    int channels;
    int frame_floats;
    int person_floats;
    int sample_floats;
    std::mutex mutex;
    std::unordered_map<TrackKey, std::deque<std::vector<float>>, TrackKeyHash> poses;
};

struct CustomCtx {
    StgcnppPreprocess preprocess;

    CustomCtx(
        int clip_len,
        int num_joints,
        int num_person)
        : preprocess(clip_len, num_joints, num_person)
    {
    }
};

StgcnppPreprocess::StgcnppPreprocess(
    int clip_len,
    int num_joints,
    int num_person)
{
    this->clip_len = clip_len;
    this->num_joints = num_joints;
    this->num_person = num_person;
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

const float *StgcnppPreprocess::keypoints_from_mask(NvDsObjectMeta *object_meta)
{
    const float *data = nullptr;
    if (object_meta->mask_params.data != nullptr &&
        object_meta->mask_params.width == 3 &&
        object_meta->mask_params.height >= static_cast<guint>(num_joints) &&
        object_meta->rect_params.width > 0.0f &&
        object_meta->rect_params.height > 0.0f) {
        data = object_meta->mask_params.data;
    }
    return data;
}

void StgcnppPreprocess::map_mask_to_frame(
    const float *mask_xy_score,
    NvDsObjectMeta *object_meta,
    float *frame_xy_score)
{
    float left = object_meta->rect_params.left;
    float top = object_meta->rect_params.top;
    float box_w = object_meta->rect_params.width;
    float box_h = object_meta->rect_params.height;
    for (int joint = 0; joint < num_joints; ++joint) {
        frame_xy_score[joint * 3 + 0] = left + mask_xy_score[joint * 3 + 0] * box_w;
        frame_xy_score[joint * 3 + 1] = top + mask_xy_score[joint * 3 + 1] * box_h;
        frame_xy_score[joint * 3 + 2] = mask_xy_score[joint * 3 + 2];
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

int StgcnppPreprocess::track_clip_length(const TrackKey &key)
{
    int length = 0;
    auto it = poses.find(key);
    if (it != poses.end()) {
        length = static_cast<int>(it->second.size());
    }
    return length;
}

bool StgcnppPreprocess::track_ready(const TrackKey &key)
{
    return track_clip_length(key) == clip_len;
}

void StgcnppPreprocess::write_object_ready(
    NvDsBatchMeta *batch_meta,
    NvDsObjectMeta *object_meta,
    gboolean ready,
    gint length)
{
    NvDsStgcnppReadyMeta *existing = nvds_stgcnpp_ready_meta_from_obj(object_meta);
    NvDsStgcnppReadyMeta *meta = existing;
    if (meta == nullptr) {
        NvDsUserMeta *user_meta = nvds_acquire_user_meta_from_pool(batch_meta);
        if (user_meta != nullptr) {
            meta = static_cast<NvDsStgcnppReadyMeta *>(g_malloc0(sizeof(NvDsStgcnppReadyMeta)));
            user_meta->user_meta_data = meta;
            user_meta->base_meta.meta_type = NVDS_STGCNPP_READY_USER_META;
            user_meta->base_meta.copy_func = nvds_stgcnpp_ready_meta_copy;
            user_meta->base_meta.release_func = nvds_stgcnpp_ready_meta_release;
            nvds_add_user_meta_to_obj(object_meta, user_meta);
        }
    }
    if (meta != nullptr) {
        meta->ready = ready;
        meta->length = length;
    }
}

void StgcnppPreprocess::write_frame_ready(NvDsFrameMeta *frame_meta)
{
    NvDsBatchMeta *batch_meta = nullptr;
    if (frame_meta != nullptr) {
        batch_meta = frame_meta->base_meta.batch_meta;
    }
    if (frame_meta != nullptr && batch_meta != nullptr) {
        for (NvDsMetaList *item = frame_meta->obj_meta_list; item != nullptr; item = item->next) {
            auto *object_meta = static_cast<NvDsObjectMeta *>(item->data);
            if (object_meta == nullptr) {
                continue;
            }
            TrackKey key;
            key.source_id = frame_meta->source_id;
            key.object_id = object_meta->object_id;
            gint length = 0;
            if (object_meta->object_id != UNTRACKED_OBJECT_ID) {
                length = track_clip_length(key);
            }
            write_object_ready(batch_meta, object_meta, length == clip_len, length);
        }
    }
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
            const float *mask = keypoints_from_mask(object_meta);
            if (mask == nullptr || hw.first <= 0 || hw.second <= 0) {
                continue;
            }
            std::vector<float> frame_xy_score(static_cast<size_t>(frame_floats), 0.0f);
            map_mask_to_frame(mask, object_meta, frame_xy_score.data());
            prenormalize_2d(frame_xy_score.data(), hw.first, hw.second);
            append_pose(key, frame_xy_score.data());
        }
        write_frame_ready(group.frame_meta);
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
    CustomCtx *ctx = new CustomCtx(clip_len, num_joints, num_person);
    return ctx;
}

extern "C" void deInitLib(CustomCtx *ctx)
{
    delete ctx;
}
