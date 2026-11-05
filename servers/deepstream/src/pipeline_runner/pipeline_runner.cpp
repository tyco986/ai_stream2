#include "pipeline_runner/pipeline_runner.hpp"

#include "constants/pipeline_runner_const.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <utility>
#include <vector>

#include <csignal>
#include <sys/wait.h>
#include <unistd.h>

PipelineRunner::PipelineRunner(std::string configDir)
    : configDir(std::move(configDir)),
      pid(-1) {
}

std::string PipelineRunner::loadPipelineName() const {
    const std::filesystem::path paramsPath =
        std::filesystem::path(configDir) / pipeline_runner::PARAMS_FILE;
    const YAML::Node params = YAML::LoadFile(paramsPath.string());
    return params[pipeline_runner::PIPELINE_NAME_KEY].as<std::string>();
}

YAML::Node PipelineRunner::loadPipelineYaml() const {
    const std::filesystem::path pipelinePath =
        std::filesystem::path(configDir) / pipeline_runner::PIPELINE_FILE;
    return YAML::LoadFile(pipelinePath.string());
}

void PipelineRunner::fixLink(deepstream::Pipeline& pipeline, const YAML::Node& pipelineYaml) const {
    const std::string sourcePrefix = pipeline_runner::SOURCE_PREFIX;
    const std::string queuePrefix = pipeline_runner::DEMUX_QUEUE_PREFIX;
    std::vector<std::pair<int, std::string>> sources;
    std::vector<std::pair<int, std::string>> demuxQueues;
    for (const YAML::Node& node : pipelineYaml["deepstream"]["nodes"]) {
        const std::string name = node["name"].as<std::string>();
        const bool sourceExact = name == sourcePrefix;
        const bool sourceIndexed = name.rfind(sourcePrefix, 0) == 0 && name.size() > sourcePrefix.size();
        bool sourceSuffixDigits = sourceIndexed;
        for (size_t index = sourcePrefix.size(); sourceSuffixDigits && index < name.size(); ++index) {
            sourceSuffixDigits = std::isdigit(static_cast<unsigned char>(name[index])) != 0;
        }
        if (sourceExact || sourceSuffixDigits) {
            const int order = sourceExact ? 0 : std::stoi(name.substr(sourcePrefix.size()));
            sources.emplace_back(order, name);
        }
        const bool queueIndexed = name.rfind(queuePrefix, 0) == 0 && name.size() > queuePrefix.size();
        bool queueSuffixDigits = queueIndexed;
        for (size_t index = queuePrefix.size(); queueSuffixDigits && index < name.size(); ++index) {
            queueSuffixDigits = std::isdigit(static_cast<unsigned char>(name[index])) != 0;
        }
        if (queueSuffixDigits) {
            demuxQueues.emplace_back(std::stoi(name.substr(queuePrefix.size())), name);
        }
    }
    std::sort(sources.begin(), sources.end());
    std::sort(demuxQueues.begin(), demuxQueues.end());

    const YAML::Node edges = pipelineYaml["deepstream"]["edges"];
    if (pipeline.find(pipeline_runner::MUX_NAME) != nullptr) {
        for (const std::pair<int, std::string>& source : sources) {
            const bool linked = edges && edges[source.second] && edges[source.second].IsScalar() &&
                edges[source.second].as<std::string>() == pipeline_runner::MUX_NAME;
            if (!linked) {
                pipeline.link(
                    std::make_pair(source.second, std::string(pipeline_runner::MUX_NAME)),
                    std::make_pair(std::string(""), std::string(pipeline_runner::MUX_SINK_PAD)));
            }
        }
    }
    if (pipeline.find(pipeline_runner::DEMUX_NAME) != nullptr) {
        for (const std::pair<int, std::string>& queue : demuxQueues) {
            pipeline.link(
                std::make_pair(std::string(pipeline_runner::DEMUX_NAME), queue.second),
                std::make_pair(std::string(pipeline_runner::DEMUX_SRC_PAD), std::string("")));
        }
    }
}

void PipelineRunner::run() {
    const pid_t child = fork();
    if (child == 0) {
        const std::string pipelineName = loadPipelineName();
        const YAML::Node pipelineYaml = loadPipelineYaml();
        const std::filesystem::path pipelinePath =
            std::filesystem::path(configDir) / pipeline_runner::PIPELINE_FILE;
        deepstream::Pipeline pipeline(pipelineName.c_str(), pipelinePath.string());
        fixLink(pipeline, pipelineYaml);
        pipeline.start().wait();
        _exit(0);
    }
    pid = child;
}

bool PipelineRunner::status() {
    int processStatus = 0;
    const pid_t result = pid > 0 ? waitpid(pid, &processStatus, WNOHANG) : -1;
    const bool alive = result == 0;
    pid = alive ? pid : -1;
    return alive;
}

void PipelineRunner::stop() {
    if (pid > 0) {
        kill(pid, SIGKILL);
        int processStatus = 0;
        waitpid(pid, &processStatus, 0);
        pid = -1;
    }
}
