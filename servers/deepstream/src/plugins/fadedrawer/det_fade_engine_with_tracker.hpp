#pragma once

#include <cstdint>
#include <unordered_map>

#include "plugins/boxsnapshot/boxsnapshot.hpp"
#include "plugins/fadedrawer/det_fade_engine.hpp"

// Live tracker boxes stay opaque. Snapshot boxes fade between inference frames.
class DetFadeEngineWithTracker : public virtual DetFadeEngine {
public:
    DetFadeEngineWithTracker(int interval, int fadeTime, bool showLabel, bool showSnap);
    void dropStream(int padIndex) override;
    void processFrame(NvDsBatchMeta* batch, NvDsFrameMeta* frame) override;

protected:
    struct TrackCache {
        float confidence = 0.0f;
    };

    void writeLabel(NvDsObjectMeta* object) const override;

    std::unordered_map<int, std::unordered_map<std::uint64_t, TrackCache>> tracks;

private:
    const BoxSnapshotMeta* findSnapshot(NvDsFrameMeta* frame) const;
    void cacheLiveTracks(NvDsFrameMeta* frame);
    void applyTrackCache(NvDsObjectMeta* object, int padIndex) const;
    void renewSnapshot(StreamState& state, const BoxSnapshotMeta* snapshot) const;
    void drawSnapshot(NvDsBatchMeta* batch, NvDsFrameMeta* frame, const StreamState& state, const NvOSD_ColorParams& border, float alpha);
    void drawLive(NvDsBatchMeta* batch, NvDsFrameMeta* frame);

    bool showSnap;
};
