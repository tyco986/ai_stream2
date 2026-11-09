#pragma once

#include "base_generator/event_sahi_rtsp.hpp"

// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
//         → boxsnapshot → nvtracker → nvdsanalytics → presenceeventcoder → tee_msg
//       ─┬→ nvstreamdemux → queue_demux{N} → nvvideoconvert{N} → tee_raw{N}
//       │     ─┬→ queue_raw{N} → nvvideoconvert_raw{N} → capsfilter_raw{N} → rawcapturer{N} → fakesink_raw{N}
//       │     └→ queue_osd{N} → nvvideoconvert_osd{N} → detfadedrawerwithtracker{N} → nvosdbin{N} → tee_vis{N}
//       │           ─┬→ queue_vis{N} → nvvideoconvert_vis{N} → capsfilter_vis{N} → viscapturer{N} → fakesink_vis{N}
//       │           └→ queue_enc{N} → presencelogger{N} → nvvideoconvert_enc{N} → capsfilter_enc{N}
//       │                 → nvv4l2h264enc{N} → h264parse{N} → rtspclientsink{N}
//       └→ queue_msg → nvmsgconv → nvmsgbroker
class VisEventSahiRtspGenerator : public EventSahiRtspGenerator {
public:
    using EventSahiRtspGenerator::EventSahiRtspGenerator;

protected:
    void appendAfterOsd(int index, int gpuId) override;
    void linkAfterOsd(YAML::Node edges, int index) override;
};
