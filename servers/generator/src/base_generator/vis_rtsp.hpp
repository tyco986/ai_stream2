#pragma once

#include "base_generator/base_rtsp.hpp"

// Generate YOLO RTSP pipeline with OSD preview sink.
//
// Set ``analyzer=None`` to disable nvdsanalytics rules. Set ``tracker=None`` to skip nvtracker.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → boxsnapshot → nvtracker → nvdsanalytics → tee_msg
//           ─┬→ nvstreamdemux → queue_demux{N} → nvvideoconvert{N} → nvosdbin{N}
//           │                → queue_enc{N} → detlogger{N} → nvv4l2h264enc{N} → h264parse{N} → rtspclientsink{N}
//           └→ queue_msg → nvmsgconv → nvmsgbroker
class VisRtspGenerator : public BaseRtspGenerator {
public:
    using BaseRtspGenerator::BaseRtspGenerator;

    void add() override;
    void link() override;
};
