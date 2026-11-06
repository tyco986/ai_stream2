#pragma once

#include "base_generator/base_rtsp.hpp"
#include "subelement_generator/nvsahipreprocess.hpp"
#include "subelement_generator/utils/sahi.hpp"

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

// Generate YOLO SAHI RTSP pipeline (headless, ends at fakesink).
//
// mux batch size is the stream count. pgie batch size is the SAHI tile count times the stream count.
//
// Set ``analyzer=None`` to disable nvdsanalytics rules. Set ``tracker=None`` to skip nvtracker.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
//           → boxsnapshot → nvtracker → nvdsanalytics → tee_msg
//             ─┬→ nvstreamdemux → queue_demux{N} → detlogger{N} → fakesink{N}
//             └→ queue_msg → nvmsgconv → nvmsgbroker
class SahiRtspGenerator : public BaseRtspGenerator {
public:
    static constexpr const char* SAHI_PREPROCESS_CONFIG_NAME = "nvsahipreprocess.ini";
    static constexpr const char* SAHI_POSTPROCESS = "nvsahipostprocess";
    static constexpr const char* SAHI_INFO_NAME = "sahi_slice_info.json";
    static constexpr const char* SAHI_PREVIEW_NAME = "sahi_slice_preview.jpg";

    SahiRtspGenerator(
        std::string pipelineName,
        nlohmann::json streams,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json sahi,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void initStreams() override;
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
