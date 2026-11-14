#include "pipeline_runner/pipeline_runner.hpp"

#include "constants/pipeline_const.hpp"
#include "constants/pipeline_runner_const.hpp"

#include <chrono>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <utility>
#include <vector>

#include <csignal>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "buffer_probe.hpp"

PipelineRunner::PipelineRunner(std::string configDir)
    : configDir(std::move(configDir)),
      pipelineName(),
      pid(-1),
      table(nullptr),
      started(std::nullopt),
      ended(std::nullopt) {
}

PipelineRunner::~PipelineRunner() {
    std::lock_guard<std::mutex> lock(mutex);
    if (table != nullptr) {
        munmap(table, sizeof(SourceLivenessTable));
        table = nullptr;
    }
}

void PipelineRunner::recordEnded(std::chrono::system_clock::time_point time) {
    if (!ended.has_value()) {
        ended = time;
    }
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

void PipelineRunner::prepareOutputs(const YAML::Node& pipelineYaml) const {
    for (const YAML::Node& node : pipelineYaml["deepstream"]["nodes"]) {
        const bool sink = node["type"].as<std::string>() == pipeline_runner::FILE_SINK;
        const YAML::Node properties = node["properties"];
        const bool located = sink && properties && properties[pipeline_runner::LOCATION_KEY];
        if (located) {
            const std::filesystem::path location = properties[pipeline_runner::LOCATION_KEY].as<std::string>();
            std::filesystem::create_directories(location.parent_path());
        }
    }
}

void PipelineRunner::attachSourceProbes(deepstream::Pipeline& pipeline, const YAML::Node& pipelineYaml) {
    const std::string sourcePrefix = pipeline_runner::SOURCE_PREFIX;
    std::vector<std::pair<int, std::string>> sources;
    for (const YAML::Node& node : pipelineYaml["deepstream"]["nodes"]) {
        const std::string name = node["name"].as<std::string>();
        const bool sourceExact = name == sourcePrefix;
        const bool sourceIndexed = name.rfind(sourcePrefix, 0) == 0 && name.size() > sourcePrefix.size();
        bool sourceSuffixDigits = sourceIndexed;
        for (size_t index = sourcePrefix.size(); sourceSuffixDigits && index < name.size(); ++index) {
            sourceSuffixDigits = std::isdigit(static_cast<unsigned char>(name[index])) != 0;
        }
        const YAML::Node properties = node["properties"];
        const std::string uri = properties && properties[pipeline_runner::URI_KEY]
            ? properties[pipeline_runner::URI_KEY].as<std::string>()
            : "";
        const bool rtsp = uri.rfind(pipeline_runner::URI_RTSP, 0) == 0;
        if ((sourceExact || sourceSuffixDigits) && rtsp) {
            const int order = sourceExact ? 0 : std::stoi(name.substr(sourcePrefix.size()));
            sources.emplace_back(order, name);
        }
    }
    std::sort(sources.begin(), sources.end());
    unsigned configured = 0;
    int count = 0;
    for (const std::pair<int, std::string>& source : sources) {
        deepstream::Element* element = pipeline.find(source.second);
        const std::string queueName = std::string(pipeline_runner::DEMUX_QUEUE_PREFIX) + std::to_string(source.first);
        deepstream::Element* queue = pipeline.find(queueName);
        const bool room = element != nullptr && queue != nullptr && count < pipeline_runner::SLOT_COUNT && table != nullptr;
        if (room && count == 0) {
            element->getProperty(pipeline_runner::RECONNECT_INTERVAL, configured);
        }
        if (room) {
            auto* handler = new SourceLivenessProbe(&table->slots[count]);
            auto* probe = new deepstream::BufferProbe("source-liveness-" + std::to_string(count), handler);
            queue->addProbe(probe, pipeline_runner::SRC_PAD);
            count += 1;
        }
    }
    const int interval = configured > 0 ? static_cast<int>(configured) : pipeline_runner::STALE_SEC;
    if (table != nullptr) {
        table->count.store(count, std::memory_order_relaxed);
        table->reconnectIntervalSec.store(interval, std::memory_order_relaxed);
    }
}

const char* PipelineRunner::slotStatus(const SourceLivenessSlot& slot, std::int64_t nowNs, int intervalSec) const {
    const bool exceeded = slot.exceeded.load(std::memory_order_relaxed) != 0;
    const std::int64_t last = slot.lastFrameNs.load(std::memory_order_relaxed);
    const bool fresh = last > 0 && nowNs - last <= static_cast<std::int64_t>(intervalSec) * 1000000000LL;
    const char* status = pipeline::STATUS_RUNNING;
    if (exceeded) {
        status = pipeline::STATUS_DEAD;
    } else if (!fresh) {
        status = pipeline::STATUS_RECONNECTING;
    }
    return status;
}

void PipelineRunner::redirectLog() const {
    const std::filesystem::path directory = std::filesystem::path(pipeline_runner::LOG_DIR) / pipelineName;
    std::filesystem::create_directories(directory);
    const std::filesystem::path file = directory / pipeline_runner::LOG_FILE;
    const int fd = ::open(file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    const bool opened = fd >= 0;
    if (opened) {
        ::dup2(fd, STDOUT_FILENO);
        ::dup2(fd, STDERR_FILENO);
        ::close(fd);
        std::setvbuf(stdout, nullptr, _IOLBF, 0);
        std::setvbuf(stderr, nullptr, _IOLBF, 0);
    }
}

void PipelineRunner::runPipeline() {
    const YAML::Node pipelineYaml = loadPipelineYaml();
    const std::filesystem::path pipelinePath =
        std::filesystem::path(configDir) / pipeline_runner::PIPELINE_FILE;
    prepareOutputs(pipelineYaml);
    deepstream::Pipeline pipeline(pipelineName.c_str(), pipelinePath.string());
    fixLink(pipeline, pipelineYaml);
    attachSourceProbes(pipeline, pipelineYaml);
    pipeline.start().wait();
}

void PipelineRunner::run() {
    table = static_cast<SourceLivenessTable*>(mmap(nullptr, sizeof(SourceLivenessTable), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    if (table == MAP_FAILED) {
        table = nullptr;
    }
    if (table != nullptr) {
        new (table) SourceLivenessTable();
    }
    const pid_t child = fork();
    int code = 0;
    if (child == 0) {
        // The child inherits the HTTP server. An exception must exit here, otherwise
        // it is reported as a second /start/local response and the child stays alive.
        pipelineName = loadPipelineName();
        redirectLog();
        try {
            runPipeline();
        } catch (const std::exception& error) {
            std::fprintf(stderr, "%s\n", error.what());
            code = 1;
        }
        std::fflush(stdout);
        std::fflush(stderr);
        _exit(code);
    }
    pid = child;
    if (child > 0) {
        started = std::chrono::system_clock::now();
        ended = std::nullopt;
    }
}

PipelineStatus PipelineRunner::status() {
    std::lock_guard<std::mutex> lock(mutex);
    int processStatus = 0;
    const pid_t result = pid > 0 ? waitpid(pid, &processStatus, WNOHANG) : -1;
    const bool alive = result == 0;
    const auto now = std::chrono::system_clock::now();
    if (pid > 0 && !alive) {
        recordEnded(now);
    }
    pid = alive ? pid : -1;
    const auto end = ended.value_or(now);
    const auto nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    const int intervalSec = table != nullptr ? table->reconnectIntervalSec.load(std::memory_order_relaxed) : pipeline_runner::STALE_SEC;
    const int count = table != nullptr ? table->count.load(std::memory_order_relaxed) : 0;
    bool degraded = false;
    std::vector<StreamStatus> streams;
    for (int index = 0; alive && table != nullptr && index < count && index < pipeline_runner::SLOT_COUNT; ++index) {
        const char* slot = slotStatus(table->slots[index], nowNs, intervalSec);
        degraded = degraded || slot != pipeline::STATUS_RUNNING;
        streams.push_back(StreamStatus{index, slot});
    }
    PipelineStatus report;
    report.status = !alive ? pipeline::STATUS_STOPPED : (degraded ? pipeline::STATUS_DEGRADED : pipeline::STATUS_RUNNING);
    report.streams = alive && table != nullptr ? std::optional<std::vector<StreamStatus>>(std::move(streams)) : std::nullopt;
    report.startTime = started;
    report.endTime = ended;
    report.duration = started.has_value()
        ? std::optional<std::int64_t>(std::chrono::duration_cast<std::chrono::seconds>(end - *started).count())
        : std::nullopt;
    return report;
}

void PipelineRunner::stop() {
    if (pid > 0) {
        kill(pid, SIGKILL);
        int processStatus = 0;
        waitpid(pid, &processStatus, 0);
        recordEnded(std::chrono::system_clock::now());
        pid = -1;
    }
}

