#include "plugins/nvdspreprocess/stgcnpppreprocess/engine/stgcnpp_preprocess.hpp"

#include <algorithm>
#include <cstring>
#include <map>
#include <unordered_set>

#include <cuda_runtime_api.h>

#include "constants/stgcnpp_const.hpp"
#include "plugins/nvdspreprocess/stgcnpppreprocess/engine/stgcnpp_ready_meta.hpp"

namespace {

struct FrameGroup {
    gint sourceId = 0;
    guint frameNum = 0;
    NvDsFrameMeta* frame = nullptr;
    std::vector<int> unitIndices;
};

}  // namespace

StgcnppPreprocess::StgcnppPreprocess(int clipLen, int numJoints, int numPerson, int patience) {
    this->clipLen = clipLen;
    this->numJoints = numJoints;
    this->numPerson = numPerson;
    this->patience = patience;
    this->channels = stgcnpp::CHANNELS;
    this->frameFloats = this->numJoints * this->channels;
    this->personFloats = this->clipLen * this->frameFloats;
    this->sampleFloats = this->numPerson * this->personFloats;
}

NvDsObjectMeta* StgcnppPreprocess::unitObject(const NvDsPreProcessUnit& unit) {
    NvDsObjectMeta* object = unit.obj_meta;
    if (object == nullptr) {
        object = unit.roi_meta.object_meta;
    }
    return object;
}

NvDsFrameMeta* StgcnppPreprocess::unitFrame(const NvDsPreProcessUnit& unit) {
    NvDsFrameMeta* frame = unit.frame_meta;
    if (frame == nullptr) {
        frame = unit.roi_meta.frame_meta;
    }
    return frame;
}

StgcnppTrackKey StgcnppPreprocess::unitKey(const NvDsPreProcessUnit& unit) {
    StgcnppTrackKey key;
    NvDsFrameMeta* frame = unitFrame(unit);
    NvDsObjectMeta* object = unitObject(unit);
    if (frame != nullptr) {
        key.sourceId = frame->source_id;
    }
    if (object != nullptr) {
        key.objectId = object->object_id;
    }
    return key;
}

const float* StgcnppPreprocess::keypointsFromMask(NvDsObjectMeta* object) {
    const float* data = nullptr;
    if (object->mask_params.data != nullptr && object->mask_params.width == 3 &&
        object->mask_params.height >= static_cast<guint>(numJoints) && object->rect_params.width > 0.0f &&
        object->rect_params.height > 0.0f) {
        data = object->mask_params.data;
    }
    return data;
}

void StgcnppPreprocess::mapMaskToFrame(const float* maskXyScore, NvDsObjectMeta* object, float* frameXyScore) {
    float left = object->rect_params.left;
    float top = object->rect_params.top;
    float boxWidth = object->rect_params.width;
    float boxHeight = object->rect_params.height;
    for (int joint = 0; joint < numJoints; ++joint) {
        frameXyScore[joint * 3 + 0] = left + maskXyScore[joint * 3 + 0] * boxWidth;
        frameXyScore[joint * 3 + 1] = top + maskXyScore[joint * 3 + 1] * boxHeight;
        frameXyScore[joint * 3 + 2] = maskXyScore[joint * 3 + 2];
    }
}

std::pair<int, int> StgcnppPreprocess::frameSize(NvDsFrameMeta* frame) {
    int width = 0;
    int height = 0;
    if (frame != nullptr) {
        width = static_cast<int>(frame->pipeline_width);
        height = static_cast<int>(frame->pipeline_height);
        if (width <= 0) {
            width = static_cast<int>(frame->source_frame_width);
        }
        if (height <= 0) {
            height = static_cast<int>(frame->source_frame_height);
        }
    }
    return {width, height};
}

void StgcnppPreprocess::prenormalize(float* frameXyScore, int frameWidth, int frameHeight) {
    float halfWidth = 0.5f * static_cast<float>(frameWidth);
    float halfHeight = 0.5f * static_cast<float>(frameHeight);
    for (int joint = 0; joint < numJoints; ++joint) {
        frameXyScore[joint * 3 + 0] = (frameXyScore[joint * 3 + 0] - halfWidth) / halfWidth;
        frameXyScore[joint * 3 + 1] = (frameXyScore[joint * 3 + 1] - halfHeight) / halfHeight;
    }
}

void StgcnppPreprocess::appendPose(const StgcnppTrackKey& key, const float* frameXyScore) {
    TrackState& track = tracks[key];
    track.miss = 0;
    track.clip.emplace_back(frameXyScore, frameXyScore + frameFloats);
    while (static_cast<int>(track.clip.size()) > clipLen) {
        track.clip.pop_front();
    }
}

int StgcnppPreprocess::clipLength(const StgcnppTrackKey& key) {
    int length = 0;
    auto it = tracks.find(key);
    if (it != tracks.end()) {
        length = static_cast<int>(it->second.clip.size());
    }
    return length;
}

void StgcnppPreprocess::writeClip(float* dst, const std::deque<std::vector<float>>& clip) {
    int time = 0;
    for (const std::vector<float>& frame : clip) {
        std::memcpy(dst + time * frameFloats, frame.data(), static_cast<size_t>(frameFloats) * sizeof(float));
        time += 1;
    }
    if (numPerson > 1) {
        std::memset(dst + personFloats, 0, static_cast<size_t>(sampleFloats - personFloats) * sizeof(float));
    }
}

NvDsPreProcessStatus StgcnppPreprocess::prepare(
    NvDsPreProcessBatch* batch,
    NvDsPreProcessCustomBuf*& buf,
    CustomTensorParams& tensorParam,
    NvDsPreProcessAcquirer* acquirer) {
    buf = acquirer->acquire();
    std::unique_lock<std::mutex> lock(mutex);
    std::map<std::pair<gint, guint>, FrameGroup> groups;
    int units = static_cast<int>(batch->units.size());
    for (int i = 0; i < units; ++i) {
        NvDsFrameMeta* frame = unitFrame(batch->units[i]);
        gint sourceId = 0;
        guint frameNum = static_cast<guint>(batch->units[i].frame_num);
        if (frame != nullptr) {
            sourceId = frame->source_id;
            frameNum = frame->frame_num;
        }
        FrameGroup& group = groups[std::make_pair(sourceId, frameNum)];
        group.sourceId = sourceId;
        group.frameNum = frameNum;
        group.frame = frame;
        group.unitIndices.push_back(i);
    }
    for (auto& entry : groups) {
        FrameGroup& group = entry.second;
        std::unordered_set<guint64> currentIds;
        if (group.frame != nullptr) {
            for (NvDsMetaList* item = group.frame->obj_meta_list; item != nullptr; item = item->next) {
                auto* object = static_cast<NvDsObjectMeta*>(item->data);
                if (object != nullptr && object->object_id != UNTRACKED_OBJECT_ID) {
                    currentIds.insert(object->object_id);
                }
            }
        }
        for (auto it = tracks.begin(); it != tracks.end();) {
            if (it->first.sourceId != group.sourceId) {
                ++it;
            } else if (currentIds.count(it->first.objectId) != 0) {
                it->second.miss = 0;
                ++it;
            } else {
                it->second.miss += 1;
                if (it->second.miss > patience) {
                    it = tracks.erase(it);
                } else {
                    ++it;
                }
            }
        }
        std::pair<int, int> size = frameSize(group.frame);
        for (int index : group.unitIndices) {
            NvDsObjectMeta* object = unitObject(batch->units[index]);
            StgcnppTrackKey key = unitKey(batch->units[index]);
            const float* mask = nullptr;
            if (object != nullptr && key.objectId != UNTRACKED_OBJECT_ID) {
                mask = keypointsFromMask(object);
            }
            if (mask != nullptr && size.first > 0 && size.second > 0) {
                std::vector<float> frameXyScore(static_cast<size_t>(frameFloats), 0.0f);
                mapMaskToFrame(mask, object, frameXyScore.data());
                prenormalize(frameXyScore.data(), size.first, size.second);
                appendPose(key, frameXyScore.data());
            }
        }
        NvDsBatchMeta* batchMeta = nullptr;
        if (group.frame != nullptr) {
            batchMeta = group.frame->base_meta.batch_meta;
        }
        if (group.frame != nullptr && batchMeta != nullptr) {
            for (NvDsMetaList* item = group.frame->obj_meta_list; item != nullptr; item = item->next) {
                auto* object = static_cast<NvDsObjectMeta*>(item->data);
                if (object != nullptr) {
                    StgcnppTrackKey key;
                    key.sourceId = group.frame->source_id;
                    key.objectId = object->object_id;
                    gint length = 0;
                    if (object->object_id != UNTRACKED_OBJECT_ID) {
                        length = clipLength(key);
                    }
                    StgcnppReadyMetaStore::write(batchMeta, object, length == clipLen, length);
                }
            }
        }
    }

    std::vector<NvDsPreProcessUnit> readyUnits;
    std::vector<NvDsRoiMeta> readyRois;
    for (int i = 0; i < units; ++i) {
        StgcnppTrackKey key = unitKey(batch->units[i]);
        if (key.objectId != UNTRACKED_OBJECT_ID && clipLength(key) == clipLen) {
            readyUnits.push_back(batch->units[i]);
            readyRois.push_back(batch->units[i].roi_meta);
        }
    }
    int readyCount = static_cast<int>(readyUnits.size());
    std::vector<float> host(static_cast<size_t>(std::max(readyCount, 0) * sampleFloats), 0.0f);
    for (int i = 0; i < readyCount; ++i) {
        writeClip(host.data() + i * sampleFloats, tracks[unitKey(readyUnits[i])].clip);
    }
    if (readyCount > 0) {
        cudaMemcpy(
            buf->memory_ptr,
            host.data(),
            static_cast<size_t>(readyCount * sampleFloats) * sizeof(float),
            cudaMemcpyHostToDevice);
    }
    batch->units = std::move(readyUnits);
    tensorParam.seq_params.roi_vector = std::move(readyRois);
    if (!tensorParam.params.network_input_shape.empty()) {
        tensorParam.params.network_input_shape[0] = readyCount;
    }
    tensorParam.params.buffer_size =
        static_cast<guint64>(readyCount) * static_cast<guint64>(sampleFloats) * sizeof(float);
    return NVDSPREPROCESS_SUCCESS;
}
