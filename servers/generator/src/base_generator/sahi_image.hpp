#pragma once

#include "base_generator/base_image.hpp"
#include "subelement_generator/nvsahipreprocess.hpp"
#include "subelement_generator/utils/sahi.hpp"

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

// Generate YOLO SAHI image pipeline YAML.
//
// mux batch size is the image count. pgie batch size is the SAHI tile count.
// Each image is one frame (num-buffers=1) and is written to its own jpeg after per-stream OSD.
//
// Set ``analyzer=None`` to disable nvdsanalytics rules.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
//         → nvdsanalytics → nvstreamdemux → queue_demux{N} → detfadedrawer{N} → nvosdbin{N} → nvvideoconvert{N}
//         │     → capsfilter{N} → detlogger{N} → nvjpegenc{N} → filesink{N}
class SahiImageGenerator : public BaseImageGenerator {
public:
    static constexpr const char* SAHI_PREPROCESS_CONFIG_NAME = "nvsahipreprocess.ini";
    static constexpr const char* SAHI_POSTPROCESS = "nvsahipostprocess";
    static constexpr const char* SAHI_INFO_NAME = "sahi_slice_info.json";
    static constexpr const char* SAHI_PREVIEW_NAME = "sahi_slice_preview.jpg";

    SahiImageGenerator(
        std::string pipelineName,
        nlohmann::json images,
        std::filesystem::path output,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json sahi,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void initImages() override;
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
    virtual std::string visDrawerElement() const;
    virtual YAML::Node visDrawerProperties(const nlohmann::json& drawer);
};
