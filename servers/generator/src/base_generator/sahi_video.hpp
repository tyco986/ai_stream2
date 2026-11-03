#pragma once

#include "base_generator/base_video.hpp"
#include "subelement_generator/nvsahipreprocess.hpp"
#include "subelement_generator/utils/sahi.hpp"

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

// Generate YOLO SAHI video pipeline YAML (headless, ends at fakesink).
//
// mux batch size is the number of videos. pgie batch size is the SAHI tile count.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
//         → nvbboxsnapshot → nvtracker → nvdsanalytics → tee_msg
//         ├→ nvdetlogger → fakesink
//         └→ queue_msg → nvmsgconv → nvmsgbroker
class SahiVideoGenerator : public BaseVideoGenerator {
public:
    static constexpr const char* SAHI_PREPROCESS_CONFIG_NAME = "nvsahipreprocess.ini";
    static constexpr const char* SAHI_POSTPROCESS = "nvsahipostprocess";
    static constexpr const char* SAHI_INFO_NAME = "sahi_slice_info.json";
    static constexpr const char* SAHI_PREVIEW_NAME = "sahi_slice_preview.jpg";

    SahiVideoGenerator(
        std::string pipelineName,
        nlohmann::json videos,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json sahi,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void initVideos() override;
    void initParams() override;
    void initPipeline() override;
    void applySavePaths(YAML::Node pipelineDocument, const std::filesystem::path& configSaveDir) override;
    void add() override;
    void link() override;

    nlohmann::json sahi;
    std::string sahiPostprocess = SAHI_POSTPROCESS;
    int muxBatchSize = 0;
    SahiInfo sahiInfo;
    std::optional<NvsahipreprocessGenerator> nvsahipreprocessGenerator;

protected:
    void initSahi();
    virtual YAML::Node sahiPostprocessProperties(const nlohmann::json& postprocess);
};
