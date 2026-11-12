#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

#include <sys/types.h>

#include "pipeline.hpp"
#include "yaml-cpp/yaml.h"

struct PipelineStatus {
    std::string status;
    std::optional<std::chrono::system_clock::time_point> startTime;
    std::optional<std::chrono::system_clock::time_point> endTime;
    std::optional<std::int64_t> duration;
};

class PipelineRunner {
public:
    explicit PipelineRunner(std::string configDir);

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
    std::optional<std::chrono::system_clock::time_point> started;
    std::optional<std::chrono::system_clock::time_point> ended;

    void recordEnded(std::chrono::system_clock::time_point time);

    std::string loadPipelineName() const;
    YAML::Node loadPipelineYaml() const;
    void fixLink(deepstream::Pipeline& pipeline, const YAML::Node& pipelineYaml) const;
    void prepareOutputs(const YAML::Node& pipelineYaml) const;
    void redirectLog() const;
    void runPipeline() const;
};
