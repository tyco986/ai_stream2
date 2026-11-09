#pragma once

#include "base_generator/event_rtsp.hpp"

// Per-stream capture plus an RTSP preview sink.
//
// Topology::
//
//     nvosdbin{N} → tee_vis{N}
//       ─┬→ queue_vis{N} → nvvideoconvert_vis{N} → capsfilter_vis{N} → viscapturer{N} → fakesink_vis{N}
//       └→ queue_enc{N} → presencelogger{N} → nvvideoconvert_enc{N} → capsfilter_enc{N}
//         → nvv4l2h264enc{N} → h264parse{N} → rtspclientsink{N}
class VisEventRtspGenerator : public EventRtspGenerator {
public:
    using EventRtspGenerator::EventRtspGenerator;

protected:
    void appendAfterOsd(int index, int gpuId) override;
    void linkAfterOsd(YAML::Node edges, int index) override;
};
