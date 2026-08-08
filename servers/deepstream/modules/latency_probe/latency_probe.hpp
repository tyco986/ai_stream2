/*
 * Latency probe: end-to-end + per-component CompLatency into process-local map.
 */

#pragma once

#include "buffer_probe.hpp"

namespace deepstream {

constexpr int kMaxComponentName = 64;

class NvDsLatencyProbe : public BufferProbe::IBufferObserver {
 public:
  probeReturn handleBuffer(BufferProbe& probe, const Buffer& buffer) override;
};

}  // namespace deepstream

extern "C" int latency_probe_take(
    unsigned int source_id,
    unsigned int frame_num,
    double* latency_ms,
    char names[][deepstream::kMaxComponentName],
    double* values,
    int max_components,
    int* count);
