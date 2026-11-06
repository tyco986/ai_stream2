#pragma once

#include "subelement_generator/kafka.hpp"
#include "subelement_generator/nvdsanalytics.hpp"
#include "subelement_generator/nvmsgconv.hpp"
#include "subelement_generator/nvtracker.hpp"
#include "subelement_generator/pgie.hpp"
#include "subelement_generator/pipeline.hpp"
#include "subelement_generator/utils/pgie_parser.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

// Generate YOLO video pipeline YAML (headless, ends at fakesink).
//
// Reads a batch of video files. Width and height must match. A short file leaves
// the batch on EOS; the pipeline ends when the longest file ends. sync-inputs is
// off, so frames are not aligned by PTS.
//
// Set ``analyzer=None`` to disable nvdsanalytics rules. Set ``tracker=None`` to skip nvtracker.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → boxsnapshot → nvtracker → nvdsanalytics → tee_msg
//         ├→ detlogger → fakesink
//         └→ queue_msg → nvmsgconv → nvmsgbroker
class BaseVideoGenerator : public PipelineGenerator {
public:
    static constexpr const char* PIPELINE_CONFIG_NAME = "pipeline.yml";
    static constexpr const char* PGIE_CONFIG_NAME = "pgie.yml";
    static constexpr const char* TRACKER_CONFIG_NAME = "nvtracker.yml";
    static constexpr const char* ANALYTICS_CONFIG_NAME = "nvdsanalytics.yml";
    static constexpr const char* MSGCONV_CONFIG_NAME = "nvmsgconv.yml";
    static constexpr const char* KAFKA_CONFIG_NAME = "kafka.txt";
    static constexpr const char* PARAMS_CONFIG_NAME = "params.yml";

    using ConfigFile = std::variant<YAML::Node, std::string>;

    BaseVideoGenerator(
        std::string pipelineName,
        nlohmann::json videos,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    virtual void applyPgieConfig() = 0;
    void init();
    virtual void initVideos();
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
    nlohmann::json eventOsdKwargs(int gpuId) const;
    std::string nvdetDrawerElement() const;
    std::string nvposeDrawerElement() const;
    std::string nvsegDrawerElement() const;
    YAML::Node nvdetDrawerProperties(const nlohmann::json& drawer);
    YAML::Node nvposeDrawerProperties(const nlohmann::json& drawer);
    YAML::Node nvsegDrawerProperties(const nlohmann::json& drawer);
    void add() override;
    void link() override;

protected:
    static std::string fileUri(const std::filesystem::path& path);

    std::string pipelineName;
    nlohmann::json videos;
    std::optional<nlohmann::json> analyzer;
    std::optional<nlohmann::json> tracker;
    nlohmann::json pgie;
    nlohmann::json logger;
    std::optional<nlohmann::json> drawer;
    std::string generator;
    int width = 0;
    int height = 0;
    int fps = 0;
    std::vector<int> videoFps;
    int runtimeBatchSize = 0;
    std::string kafkaTopic;
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
};
