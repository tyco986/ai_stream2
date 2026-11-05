#pragma once

#include <string>

#include <sys/types.h>

#include "pipeline.hpp"
#include "yaml-cpp/yaml.h"

class PipelineRunner {
public:
    explicit PipelineRunner(std::string configDir);

    // pipeline.stop() does not finish a live source: nvurisrcbin never emits EOS, so the
    // streaming thread stays blocked. Fork before any DeepStream call and run
    // start().wait() in the child. The parent only keeps the pid.
    void run();
    // True while the child is still running.
    bool status();
    // SIGKILL the child. SIGTERM is caught by GStreamer and takes the same stuck path as stop().
    void stop();

private:
    std::string configDir;
    pid_t pid;

    std::string loadPipelineName() const;
    YAML::Node loadPipelineYaml() const;
    void fixLink(deepstream::Pipeline& pipeline, const YAML::Node& pipelineYaml) const;
};
