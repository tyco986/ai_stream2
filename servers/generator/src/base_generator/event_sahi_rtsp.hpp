#pragma once

#include "base_generator/event_rtsp.hpp"
#include "subelement_generator/nvsahipreprocess.hpp"
#include "subelement_generator/utils/sahi.hpp"

// mux batch size is the stream count. pgie batch size is the SAHI tile count times the stream count.
class EventSahiRtspGenerator : public EventRtspGenerator {
public:
    static constexpr const char* SAHI_PREPROCESS_CONFIG_NAME = "nvsahipreprocess.ini";
    static constexpr const char* SAHI_POSTPROCESS = "nvsahipostprocess";
    static constexpr const char* SAHI_INFO_NAME = "sahi_slice_info.json";
    static constexpr const char* SAHI_PREVIEW_NAME = "sahi_slice_preview.jpg";

    EventSahiRtspGenerator(
        std::string pipelineName,
        nlohmann::json streams,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json sahi,
        nlohmann::json eventCoder,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt,
        std::optional<nlohmann::json> capturer = std::nullopt);

    void initStreams() override;
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
