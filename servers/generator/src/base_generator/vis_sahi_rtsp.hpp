#pragma once

#include "base_generator/sahi_rtsp.hpp"

// Generate YOLO SAHI RTSP pipeline with OSD preview sink.
//
// mux batch size is the stream count. pgie batch size is the SAHI tile count times the stream count.
//
// Set ``analyzer=None`` to disable nvdsanalytics rules. Set ``tracker=None`` to skip nvtracker.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
//           → nvbboxsnapshot → nvtracker → nvdsanalytics → tee_msg
//             ─┬→ nvstreamdemux → queue_demux{N} → nvvideoconvert{N} → nvosdbin{N}
//             │     → queue_enc{N} → nvdetlogger{N} → nvv4l2h264enc{N} → h264parse{N} → rtspclientsink{N}
//             └→ queue_msg → nvmsgconv → nvmsgbroker
class VisSahiRtspGenerator : public SahiRtspGenerator {
public:
    using SahiRtspGenerator::SahiRtspGenerator;

    void add() override;
    void link() override;

protected:
    virtual std::string visDrawerElement() const;
    virtual YAML::Node visDrawerProperties(const nlohmann::json& drawer);
};
