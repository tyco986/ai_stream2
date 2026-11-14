#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <sys/types.h>

#include "pipeline.hpp"
#include "probes/source_liveness_probe.hpp"
#include "yaml-cpp/yaml.h"

struct StreamStatus {
    int sourceId;
    std::string status;
};

struct PipelineStatus {
    std::string status;
    std::optional<std::chrono::system_clock::time_point> startTime;
    std::optional<std::chrono::system_clock::time_point> endTime;
    std::optional<std::int64_t> duration;
    std::optional<std::vector<StreamStatus>> streams;
};

class PipelineRunner {
public:
    explicit PipelineRunner(std::string configDir);
    ~PipelineRunner();

    // pipeline.stop() does not finish a live source: nvurisrcbin never emits EOS, so the
    // streaming thread stays blocked. Fork before any DeepStream call and run
    // start().wait() in the child. The parent only keeps the pid.
    void run();
    // Records end_time when the child has exited, then returns status, times, and duration.
    PipelineStatus status();
    // SIGKILL the child. SIGTERM is caught by GStreamer and takes the same stuck path as stop().
    void stop();

private:
    std::string configDir;
    std::string pipelineName;
    pid_t pid;
    SourceLivenessTable* table;
    std::mutex mutex;
    std::optional<std::chrono::system_clock::time_point> started;
    std::optional<std::chrono::system_clock::time_point> ended;

    void recordEnded(std::chrono::system_clock::time_point time);

    std::string loadPipelineName() const;
    YAML::Node loadPipelineYaml() const;
    void fixLink(deepstream::Pipeline& pipeline, const YAML::Node& pipelineYaml) const;
    void prepareOutputs(const YAML::Node& pipelineYaml) const;
    void redirectLog() const;
    void attachSourceProbes(deepstream::Pipeline& pipeline, const YAML::Node& pipelineYaml);
    const char* slotStatus(const SourceLivenessSlot& slot, std::int64_t nowNs, int intervalSec) const;
    void runPipeline();
};
