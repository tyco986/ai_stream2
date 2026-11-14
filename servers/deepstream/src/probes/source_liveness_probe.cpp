#include "probes/source_liveness_probe.hpp"

#include <chrono>

SourceLivenessProbe::SourceLivenessProbe(SourceLivenessSlot* slot)
    : slot(slot) {
}

deepstream::probeReturn SourceLivenessProbe::handleBuffer(deepstream::BufferProbe&, const deepstream::Buffer&) {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
    slot->lastFrameNs.store(ns, std::memory_order_relaxed);
    return deepstream::probeReturn::Probe_Ok;
}
