#pragma once

#include "base_generator/event_sahi_rtsp.hpp"

class VisEventSahiRtspGenerator : public EventSahiRtspGenerator {
public:
    using EventSahiRtspGenerator::EventSahiRtspGenerator;

protected:
    void appendAfterOsd(int index, int gpuId) override;
    void linkAfterOsd(YAML::Node edges, int index) override;
};
