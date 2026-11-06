#pragma once

#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "gstnvdsmeta.h"
#include "nvdspreprocess_interface.h"

struct StgcnppTrackKey {
    gint sourceId = 0;
    guint64 objectId = UNTRACKED_OBJECT_ID;

    bool operator==(const StgcnppTrackKey& other) const {
        bool same = sourceId == other.sourceId && objectId == other.objectId;
        return same;
    }
};

struct StgcnppTrackKeyHash {
    size_t operator()(const StgcnppTrackKey& key) const {
        size_t hashed = static_cast<size_t>(key.sourceId);
        hashed ^= static_cast<size_t>(key.objectId);
        hashed ^= static_cast<size_t>(key.objectId >> 32);
        return hashed;
    }
};

class StgcnppPreprocess {
public:
    StgcnppPreprocess(int clipLen, int numJoints, int numPerson, int patience);

    NvDsPreProcessStatus prepare(
        NvDsPreProcessBatch* batch,
        NvDsPreProcessCustomBuf*& buf,
        CustomTensorParams& tensorParam,
        NvDsPreProcessAcquirer* acquirer);

private:
    struct TrackState {
        std::deque<std::vector<float>> clip;
        int miss = 0;
    };

    NvDsObjectMeta* unitObject(const NvDsPreProcessUnit& unit);
    NvDsFrameMeta* unitFrame(const NvDsPreProcessUnit& unit);
    StgcnppTrackKey unitKey(const NvDsPreProcessUnit& unit);
    const float* keypointsFromMask(NvDsObjectMeta* object);
    void mapMaskToFrame(const float* maskXyScore, NvDsObjectMeta* object, float* frameXyScore);
    std::pair<int, int> frameSize(NvDsFrameMeta* frame);
    void prenormalize(float* frameXyScore, int frameWidth, int frameHeight);
    void appendPose(const StgcnppTrackKey& key, const float* frameXyScore);
    int clipLength(const StgcnppTrackKey& key);
    void writeClip(float* dst, const std::deque<std::vector<float>>& clip);

    int clipLen;
    int numJoints;
    int numPerson;
    int patience;
    int channels;
    int frameFloats;
    int personFloats;
    int sampleFloats;
    std::mutex mutex;
    std::unordered_map<StgcnppTrackKey, TrackState, StgcnppTrackKeyHash> tracks;
};
