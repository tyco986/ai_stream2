#pragma once

#include "base_generator/base_event.hpp"
#include "constants/subelement_const.hpp"
#include "subelement_generator/kafka.hpp"
#include "subelement_generator/nvmsgconv.hpp"
#include "subelement_generator/pgie.hpp"
#include "subelement_generator/utils/pgie_parser.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

// Generate YOLO event image pipeline.
//
// Images must share width and height. Each image is one frame (num-buffers=1).
// Capture stays on the batched stream. JPEG output is one file per image.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → nvdsanalytics → presenceeventcoder → tee_msg
//       ─┬→ nvvideoconvert → tee_raw
//       │     ─┬→ queue_raw → nvvideoconvert_raw → capsfilter_raw → rawcapturer → fakesink_raw
//       │     └→ queue_osd → nvvideoconvert_osd → capsfilter_osd → detfadedrawer → nvosdbin → tee_vis
//       │           ─┬→ queue_vis → nvvideoconvert_vis → capsfilter_vis → viscapturer → fakesink_vis
//       │           └→ queue_enc → nvvideoconvert_enc → capsfilter_jpeg → presencelogger
//       │                 → nvstreamdemux → queue_demux{N} → nvjpegenc{N} → filesink{N}
//       └→ queue_msg → nvmsgconv → nvmsgbroker
class EventImageGenerator : public BaseEventGenerator {
public:
    static constexpr const char* PIPELINE_CONFIG_NAME = "pipeline.yml";
    static constexpr const char* PGIE_CONFIG_NAME = "pgie.yml";
    static constexpr const char* ANALYTICS_CONFIG_NAME = "nvdsanalytics.yml";
    static constexpr const char* MSGCONV_CONFIG_NAME = "nvmsgconv.yml";
    static constexpr const char* KAFKA_CONFIG_NAME = "kafka.txt";
    static constexpr const char* PARAMS_CONFIG_NAME = "params.yml";

    using ConfigFile = std::variant<YAML::Node, std::string>;

    EventImageGenerator(
        std::string pipelineName,
        nlohmann::json images,
        std::filesystem::path output,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json eventCoder,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt,
        std::optional<nlohmann::json> capturer = std::nullopt);

    virtual void applyPgieConfig() = 0;
    void init();
    void initImages();
    void initParams();
    void initPipeline();
    void initPgie();
    void initNvdsanalytics();
    std::vector<std::string> kafkaStreams() const;
    void initNvmsgconv();
    void initKafka();
    void applySavePaths(YAML::Node pipelineDocument, const std::filesystem::path& configSaveDir);
    std::map<std::string, std::string> documents(const std::filesystem::path& configSaveDir);
    void write(const std::filesystem::path& configSaveDir);
    std::string nvdetDrawerElement() const;
    YAML::Node nvdetDrawerProperties(const nlohmann::json& drawer);
    void add() override;
    void link() override;

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
    std::string msg2pLib = subelement::MSG2P_LIB_PRESENCE;
    bool enableNvdsanalytics = false;
    std::optional<PgieParser> pgieConfigParser;
    std::optional<PgieGenerator> pgieGenerator;
    std::optional<NvmsgconvGenerator> nvmsgconvGenerator;
    std::optional<KafkaGenerator> kafkaGenerator;
    std::map<std::string, ConfigFile> files;

protected:
    static std::string fileUri(const std::filesystem::path& path);
};
