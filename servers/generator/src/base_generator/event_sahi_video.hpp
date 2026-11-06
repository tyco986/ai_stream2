#pragma once

#include "base_generator/event_video.hpp"
#include "subelement_generator/nvsahipreprocess.hpp"
#include "subelement_generator/utils/sahi.hpp"

// Generate YOLO SAHI event video pipeline. Capture branches stay on the batched stream.
// The encode tail ends at fakesink.
//
// mux batch size is the video count. pgie batch size is the SAHI tile count.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
//         → presenceeventcoder → tee_msg → … → presencelogger → fakesink
class EventSahiVideoGenerator : public EventVideoGenerator {
public:
    static constexpr const char* SAHI_PREPROCESS_CONFIG_NAME = "nvsahipreprocess.ini";
    static constexpr const char* SAHI_POSTPROCESS = "nvsahipostprocess";
    static constexpr const char* SAHI_INFO_NAME = "sahi_slice_info.json";
    static constexpr const char* SAHI_PREVIEW_NAME = "sahi_slice_preview.jpg";

    EventSahiVideoGenerator(
        std::string pipelineName,
        nlohmann::json videos,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json sahi,
        nlohmann::json eventCoder,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt,
        std::optional<nlohmann::json> capturer = std::nullopt);

    void initVideos() override;
    void initParams() override;
    void initPipeline() override;
    void applySavePaths(YAML::Node pipelineDocument, const std::filesystem::path& configSaveDir) override;

    nlohmann::json sahi;
    int muxBatchSize = 0;
    SahiInfo sahiInfo;
    std::optional<NvsahipreprocessGenerator> nvsahipreprocessGenerator;

protected:
    void initSahi();
    void appendInfer(int gpuId) override;
    void linkInfer(YAML::Node edges) override;
    std::string inferenceTail() const override;
};
