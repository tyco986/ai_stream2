#include "topdown_pose_generator/topdown_pose.hpp"

#include "subelement_generator/utils/default_gie/rtmpose.hpp"

#include <utility>

TopdownPose::TopdownPose(nlohmann::json sgie, std::string pipelineName, int streamCount)
    : sgie(std::move(sgie)),
      pipelineName(std::move(pipelineName)),
      streamCount(streamCount) {
}

void TopdownPose::init() {
    const int interval = sgie.value("interval", 1);
    sgie = nlohmann::json{
        {"model_zip", sgie.at("model_zip")},
        {"interval", interval},
    };
    sgieConfigParser.emplace(sgie.at("model_zip").get<std::string>(), pipelineName, interval);
    const YAML::Node built = sgieConfigParser->build();
    const YAML::Node property = built["property"];
    sgieGenerator.emplace(
        property["model-engine-file"].as<std::string>(),
        property["labelfile-path"].as<std::string>(),
        property["infer-dims"].as<std::string>(),
        property["output-blob-names"].as<std::string>(),
        property["batch-size"].as<int>(),
        property["network-mode"].as<int>(),
        property["gpu-id"].as<int>(),
        interval);
    sgieGenerator->config = default_gie::rtmPose();
    sgieGenerator->updateConfig();
    sgieConfig = sgieGenerator->config;
    batchSize = sgieGenerator->config["property"]["batch-size"].as<int>();
    gpuId = sgieGenerator->config["property"]["gpu-id"].as<int>();
    uniqueId = sgieGenerator->config["property"]["gie-unique-id"].as<int>();

    const nlohmann::json& inputTensors = sgieConfigParser->metaJson().at("input_tensors");
    const auto input = inputTensors.begin();
    const nlohmann::json& shape = input.value();
    const std::optional<std::string> tensorName = input.key();
    preprocessGenerator.emplace(
        batchSize,
        shape.at(3).get<int>(),
        shape.at(2).get<int>(),
        shape.at(1).get<int>(),
        streamCount,
        tensorName);
    preprocessIni = preprocessGenerator->render();
}

YAML::Node TopdownPose::sgieParams() const {
    YAML::Node params(YAML::NodeType::Map);
    params["model_zip"] = sgie.at("model_zip").get<std::string>();
    params["interval"] = sgie.at("interval").get<int>();
    return params;
}

void TopdownPose::applySavePaths(YAML::Node pipelineDocument, const std::filesystem::path& configSaveDir) const {
    for (auto node : pipelineDocument["deepstream"]["nodes"]) {
        const std::string name = node["name"].as<std::string>();
        YAML::Node properties = node["properties"] ? node["properties"] : YAML::Node(YAML::NodeType::Map);
        if (name == "sgie0") {
            properties["config-file-path"] = (configSaveDir / SGIE_CONFIG_NAME).string();
        }
        if (name == "nvdspreprocess_rtmpose") {
            properties["config-file"] = (configSaveDir / RTMPOSE_PREPROCESS_CONFIG_NAME).string();
        }
        node["properties"] = properties;
    }
}
