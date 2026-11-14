#pragma once

#include <atomic>
#include <cstdint>

#include "buffer_probe.hpp"
#include "constants/pipeline_runner_const.hpp"

struct SourceLivenessSlot {
    std::atomic<std::int64_t> lastFrameNs;
    std::atomic<std::int32_t> exceeded;
};

struct SourceLivenessTable {
    std::atomic<std::int32_t> count;
    std::atomic<std::int32_t> reconnectIntervalSec;
    SourceLivenessSlot slots[pipeline_runner::SLOT_COUNT];
};

class SourceLivenessProbe : public deepstream::BufferProbe::IBufferObserver {
public:
    explicit SourceLivenessProbe(SourceLivenessSlot* slot);

    deepstream::probeReturn handleBuffer(deepstream::BufferProbe& probe, const deepstream::Buffer& buffer) override;

private:
    SourceLivenessSlot* slot;
};
