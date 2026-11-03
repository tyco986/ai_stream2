#include "stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose.hpp"

#include "constants/pipeline_const.hpp"
#include "subelement_generator/utils/default_gie/stgcnpp.hpp"
#include "subelement_generator/utils/nvtracker_parser.hpp"

#include <utility>

StgcnppYoloPose::StgcnppYoloPose(nlohmann::json stgcnpp, std::string pipelineName, int streamCount)
    : stgcnpp(std::move(stgcnpp)),
      pipelineName(std::move(pipelineName)),
      streamCount(streamCount) {
}

void StgcnppYoloPose::init(int pgieInterval) {
    const int interval = stgcnpp.value("interval", 1);
    stgcnpp = nlohmann::json{
        {"model_zip", stgcnpp.at("model_zip")},
        {"interval", interval},
    };
    parser.emplace(stgcnpp.at("model_zip").get<std::string>(), pipelineName, interval);
    const YAML::Node built = parser->build();
    const YAML::Node property = built["property"];
    generator.emplace(
        property["model-engine-file"].as<std::string>(),
        property["labelfile-path"].as<std::string>(),
        property["output-blob-names"].as<std::string>(),
        property["batch-size"].as<int>(),
        property["network-mode"].as<int>(),
        property["gpu-id"].as<int>(),
        interval,
        built["tensor-name"].as<std::string>(),
        built["num-person"].as<int>(),
        built["clip-len"].as<int>(),
        built["num-joints"].as<int>());
    generator->config = default_gie::stgcnpp();
    generator->updateConfig();
    config = generator->config;
    batchSize = config["property"]["batch-size"].as<int>();
    gpuId = config["property"]["gpu-id"].as<int>();
    uniqueId = config["property"]["gie-unique-id"].as<int>();
    metaText = parser->metaJson().dump(2);

    const NvtrackerParser tracker(pgieInterval);
    preprocessGenerator.emplace(
        batchSize,
        generator->clipLen,
        generator->numJoints,
        generator->numPerson,
        generator->tensorName,
        streamCount,
        tracker.maxShadowTrackingAge);
    preprocessIni = preprocessGenerator->render();
}

YAML::Node StgcnppYoloPose::params() const {
    YAML::Node node(YAML::NodeType::Map);
    node["model_zip"] = stgcnpp.at("model_zip").get<std::string>();
    node["interval"] = stgcnpp.at("interval").get<int>();
    return node;
}

void StgcnppYoloPose::applySavePaths(YAML::Node pipelineDocument, const std::filesystem::path& configSaveDir) const {
    for (auto node : pipelineDocument["deepstream"]["nodes"]) {
        const std::string name = node["name"].as<std::string>();
        YAML::Node properties = node["properties"] ? node["properties"] : YAML::Node(YAML::NodeType::Map);
        if (name == "sgie1") {
            properties["config-file-path"] = (configSaveDir / CONFIG_NAME).string();
        }
        if (name == "nvdspreprocess") {
            properties["config-file"] = (configSaveDir / PREPROCESS_CONFIG_NAME).string();
        }
        node["properties"] = properties;
    }
}

YAML::Node StgcnppYoloPose::drawerProperties(const nlohmann::json& drawer) {
    YAML::Node properties(YAML::NodeType::Map);
    properties["interval"] = drawer.value("interval", 0);
    properties["fade-time"] = drawer.value("fade_time", 0);
    properties["show-label"] = drawer.value("show_label", false);
    properties["show-pose"] = drawer.value("show_pose", true);
    properties["pose-threshold"] = drawer.value("pose_threshold", 0.0);
    properties["mode"] = drawer.value("mode", std::string(pipeline::POSE_MODE_COCO17));
    properties["show-snap"] = drawer.value("show_snap", true);
    return properties;
}
