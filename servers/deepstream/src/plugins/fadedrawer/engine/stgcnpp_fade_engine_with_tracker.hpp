#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "plugins/fadedrawer/engine/pose_fade_engine_with_tracker.hpp"

// Pose fade with tracker, plus the latest ST-GCN++ action on each track.
class StgcnppFadeEngineWithTracker : public PoseFadeEngineWithTracker {
public:
    StgcnppFadeEngineWithTracker(int interval, int fadeTime, bool showLabel, bool showPose, float poseThreshold, bool showSnap);
    void processFrame(NvDsBatchMeta* batch, NvDsFrameMeta* frame) override;

protected:
    void writeLabel(NvDsObjectMeta* object) const override;

private:
    struct ActionMeta {
        float conf;
        int classId;
        int ready;
        char label[MAX_LABEL_SIZE];
    };

    struct ReadyMeta {
        int ready;
        int length;
    };

    struct LastAction {
        std::string name;
        float confidence = 0.0f;
    };

    static const ActionMeta* actionMeta(NvDsObjectMeta* object);
    static const ReadyMeta* readyMeta(NvDsObjectMeta* object);
    void cacheLiveActions(NvDsFrameMeta* frame);

    std::unordered_map<std::uint64_t, LastAction> actions;
};
