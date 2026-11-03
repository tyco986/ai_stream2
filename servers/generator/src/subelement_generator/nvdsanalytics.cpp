#include "subelement_generator/nvdsanalytics.hpp"

#include <fstream>
#include <utility>

YAML::Node NvdsanalyticsGenerator::defaultConfig() {
    YAML::Node config;
    config["property"]["enable"] = 0;
    config["property"]["config-width"] = 1920;
    config["property"]["config-height"] = 1080;
    config["property"]["osd-mode"] = 0;
    config["property"]["display-font-size"] = 12;
    config["roi-filtering-stream-0"]["enable"] = 0;
    config["roi-filtering-stream-0"]["class-id"] = -1;
    config["roi-filtering-stream-0"]["inverse-roi"] = 0;
    config["roi-filtering-stream-0"]["roi-DOOR"] = "256;639;675;83;876;224;926;482;866;741";
    config["overcrowding-stream-0"]["enable"] = 0;
    config["overcrowding-stream-0"]["class-id"] = "1;2";
    config["overcrowding-stream-0"]["object-threshold"] = 5;
    config["overcrowding-stream-0"]["time-threshold"] = 3000;
    config["overcrowding-stream-0"]["roi-ENTRANCE"] = "282;347;987;843";
    config["line-crossing-stream-0"]["enable"] = 0;
    config["line-crossing-stream-0"]["extended"] = 0;
    config["line-crossing-stream-0"]["class-id"] = 0;
    config["line-crossing-stream-0"]["mode"] = "balanced";
    config["line-crossing-stream-0"]["line-crossing-Exit"] = "789;672;1084;900;851;773;1203;732";
    config["direction-detection-stream-0"]["enable"] = 0;
    config["direction-detection-stream-0"]["class-id"] = 0;
    config["direction-detection-stream-0"]["mode"] = "loose";
    config["direction-detection-stream-0"]["direction-South"] = "284;840;360;662";
    return config;
}

NvdsanalyticsGenerator::NvdsanalyticsGenerator(std::optional<YAML::Node> config)
    : config(config.has_value() ? std::move(*config) : defaultConfig()) {
}

void NvdsanalyticsGenerator::write(const std::filesystem::path& savePath) {
    const std::filesystem::path path = std::filesystem::absolute(savePath);
    std::filesystem::create_directories(path.parent_path());
    std::ofstream handle;
    handle.exceptions(std::ios::failbit | std::ios::badbit);
    handle.open(path);
    handle << YAML::Dump(config);
}
