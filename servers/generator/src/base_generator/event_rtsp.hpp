#pragma once

#include "base_generator/base_event.hpp"
#include "constants/subelement_const.hpp"
#include "subelement_generator/kafka.hpp"
#include "subelement_generator/nvmsgconv.hpp"
#include "subelement_generator/nvtracker.hpp"
#include "subelement_generator/pgie.hpp"
#include "subelement_generator/utils/pgie_parser.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

// Generate YOLO event RTSP pipeline.
//
// Streams must share width, height, and fps. After demux each stream has its own raw and vis capture branch.
// The headless tail ends at the vis capturer.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → boxsnapshot → nvtracker → nvdsanalytics → presenceeventcoder → tee_msg
//       ─┬→ nvstreamdemux → queue_demux{N} → nvvideoconvert{N} → tee_raw{N}
//       │     ─┬→ queue_raw{N} → nvvideoconvert_raw{N} → capsfilter_raw{N} → rawcapturer{N} → fakesink_raw{N}
//       │     └→ queue_osd{N} → nvvideoconvert_osd{N} → capsfilter_osd{N} → detfadedrawerwithtracker{N} → nvosdbin{N} → presencelogger{N}
//       │           → queue_vis{N} → nvvideoconvert_vis{N} → capsfilter_vis{N} → viscapturer{N} → fakesink_vis{N}
//       └→ queue_msg → nvmsgconv → nvmsgbroker
class EventRtspGenerator : public BaseEventGenerator {
public:
    static constexpr const char* PIPELINE_CONFIG_NAME = "pipeline.yml";
    static constexpr const char* PGIE_CONFIG_NAME = "pgie.yml";
    static constexpr const char* TRACKER_CONFIG_NAME = "nvtracker.yml";
    static constexpr const char* ANALYTICS_CONFIG_NAME = "nvdsanalytics.yml";
    static constexpr const char* MSGCONV_CONFIG_NAME = "nvmsgconv.yml";
    static constexpr const char* KAFKA_CONFIG_NAME = "kafka.txt";
    static constexpr const char* PARAMS_CONFIG_NAME = "params.yml";

    using ConfigFile = std::variant<YAML::Node, std::string>;

    EventRtspGenerator(
        std::string pipelineName,
        nlohmann::json streams,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json eventCoder,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt,
        std::optional<nlohmann::json> capturer = std::nullopt);

    virtual void applyPgieConfig() = 0;
    void init();
    virtual void initStreams();
    virtual void initParams();
    virtual void initPipeline();
    void initPgie();
    void initNvdsanalytics();
    void initNvtracker();
    std::vector<std::string> kafkaStreams() const;
    void initNvmsgconv();
    void initKafka();
    virtual void applySavePaths(YAML::Node pipelineDocument, const std::filesystem::path& configSaveDir);
    std::map<std::string, std::string> documents(const std::filesystem::path& configSaveDir);
    virtual void write(const std::filesystem::path& configSaveDir);
    std::string nvdetDrawerElement() const;
    YAML::Node nvdetDrawerProperties(const nlohmann::json& drawer);
    std::string visualizedSinkUri(const std::string& sourceUri) const;
    void add() override;
    void link() override;

    nlohmann::json streams;
    std::optional<nlohmann::json> analyzer;
    std::optional<nlohmann::json> tracker;
    nlohmann::json pgie;
    nlohmann::json logger;
    std::optional<nlohmann::json> drawer;
    std::string generator;
    int width = 0;
    int height = 0;
    int fps = 0;
    int runtimeBatchSize = 0;
    std::string kafkaTopic;
    std::string msg2pLib = subelement::MSG2P_LIB_PRESENCE;
    bool enableNvdsanalytics = false;
    bool enableNvtracker = false;
    int trackerWidth = 0;
    int trackerHeight = 0;
    std::string operateOnClassIds;
    std::optional<PgieParser> pgieConfigParser;
    std::optional<PgieGenerator> pgieGenerator;
    std::optional<NvtrackerGenerator> nvtrackerGenerator;
    std::optional<NvmsgconvGenerator> nvmsgconvGenerator;
    std::optional<KafkaGenerator> kafkaGenerator;
    std::map<std::string, ConfigFile> files;

protected:
    virtual void appendInfer(int gpuId);
    virtual void linkInfer(YAML::Node edges);
    virtual std::string inferenceTail() const;
    virtual void appendAfterOsd(int index, int gpuId);
    virtual void linkAfterOsd(YAML::Node edges, int index);
};
