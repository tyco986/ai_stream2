#pragma once

#include "subelement_generator/kafka.hpp"
#include "subelement_generator/nvdsanalytics.hpp"
#include "subelement_generator/pgie.hpp"
#include "constants/subelement_const.hpp"
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

// Generate YOLO image pipeline YAML.
//
// Reads a batch of images. Width and height must match. Each image is one frame
// (num-buffers=1) and is written to its own jpeg after per-stream OSD.
//
// Set ``analyzer=None`` to disable nvdsanalytics rules.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → nvdsanalytics → nvstreamdemux → queue_demux{N} → detfadedrawer{N} → nvosdbin{N} → nvvideoconvert{N}
//         │     → capsfilter{N} → detlogger{N} → nvjpegenc{N} → filesink{N}
class BaseImageGenerator : public PipelineGenerator {
public:
    static constexpr const char* PIPELINE_CONFIG_NAME = "pipeline.yml";
    static constexpr const char* PGIE_CONFIG_NAME = "pgie.yml";
    static constexpr const char* ANALYTICS_CONFIG_NAME = "nvdsanalytics.yml";
    static constexpr const char* KAFKA_CONFIG_NAME = "kafka.txt";
    static constexpr const char* PARAMS_CONFIG_NAME = "params.yml";

    using ConfigFile = std::variant<YAML::Node, std::string>;

    BaseImageGenerator(
        std::string pipelineName,
        nlohmann::json images,
        std::filesystem::path output,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    virtual void applyPgieConfig() = 0;
    void init();
    virtual void initImages();
    virtual void initParams();
    virtual void initPipeline();
    void initPgie();
    void initNvdsanalytics();
    void initKafka();
    virtual void applySavePaths(YAML::Node pipelineDocument, const std::filesystem::path& configSaveDir);
    std::map<std::string, std::string> documents(const std::filesystem::path& configSaveDir);
    void write(const std::filesystem::path& configSaveDir);
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
    nlohmann::json images;
    std::filesystem::path output;
    std::optional<nlohmann::json> analyzer;
    nlohmann::json pgie;
    nlohmann::json logger;
    std::optional<nlohmann::json> drawer;
    std::string generator;
    int width = 0;
    int height = 0;
    int runtimeBatchSize = 0;
    std::string kafkaTopic;
    bool enableNvdsanalytics = false;
    std::optional<PgieParser> pgieConfigParser;
    std::optional<PgieGenerator> pgieGenerator;
    std::optional<KafkaGenerator> kafkaGenerator;
    std::map<std::string, ConfigFile> files;
};
