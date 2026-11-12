#include "base_generator/base_image.hpp"

#include "base_generator/config_files.hpp"

#include "constants/generator_pipeline_const.hpp"
#include "constants/pipeline_const.hpp"
#include "constants/subelement_const.hpp"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <utility>
#include <variant>

#include <opencv2/imgcodecs.hpp>

static YAML::Node jsonToYaml(const nlohmann::json& value) {
    YAML::Node node;
    if (value.is_object()) {
        node = YAML::Node(YAML::NodeType::Map);
        for (auto item = value.begin(); item != value.end(); ++item) {
            node[item.key()] = jsonToYaml(item.value());
        }
    } else if (value.is_array()) {
        node = YAML::Node(YAML::NodeType::Sequence);
        for (const nlohmann::json& item : value) {
            node.push_back(jsonToYaml(item));
        }
    } else if (value.is_string()) {
        node = value.get<std::string>();
    } else if (value.is_boolean()) {
        node = value.get<bool>();
    } else if (value.is_null()) {
        node = YAML::Node(YAML::NodeType::Null);
    } else if (value.is_number_unsigned()) {
        node = value.get<unsigned int>();
    } else if (value.is_number_integer()) {
        node = value.get<int>();
    } else {
        node = value.get<double>();
    }
    return node;
}

std::string BaseImageGenerator::fileUri(const std::filesystem::path& path) {
    const std::filesystem::path absolute = std::filesystem::absolute(path);
    if (!std::filesystem::is_regular_file(absolute)) {
        throw std::invalid_argument("input image not found: " + absolute.string());
    }
    std::string uri = "file://";
    const std::string text = absolute.generic_string();
    for (const unsigned char character : text) {
        const bool plain = std::isalnum(character) || character == '-' || character == '_' || character == '.' || character == '~' || character == '/';
        if (plain) {
            uri.push_back(static_cast<char>(character));
        } else {
            constexpr char hex[] = "0123456789ABCDEF";
            uri.push_back('%');
            uri.push_back(hex[character >> 4]);
            uri.push_back(hex[character & 0x0f]);
        }
    }
    return uri;
}

static void probeImage(const std::filesystem::path& path, int& width, int& height) {
    const cv::Mat image = cv::imread(path.string(), cv::IMREAD_UNCHANGED);
    if (image.empty()) {
        throw std::runtime_error("input image not found: " + path.string());
    }
    width = image.cols;
    height = image.rows;
    if (width <= 0 || height <= 0) {
        throw std::runtime_error("invalid image size: " + path.string());
    }
}

BaseImageGenerator::BaseImageGenerator(
    std::string pipelineName,
    nlohmann::json images,
    std::filesystem::path output,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer)
    : pipelineName(std::move(pipelineName)),
      images(std::move(images)),
      output(std::filesystem::absolute(std::move(output))),
      analyzer(std::move(analyzer)),
      pgie(std::move(pgie)),
      logger({{"interval", 1}}),
      drawer(std::move(drawer)) {
    if (logger.has_value()) {
        this->logger.update(*logger);
    }
}

void BaseImageGenerator::init() {
    initImages();
    initPgie();
    initNvdsanalytics();
    initKafka();
    initParams();
    initPipeline();
}

void BaseImageGenerator::initImages() {
    if (!images.is_array() || images.empty()) {
        throw std::invalid_argument("images cannot be empty");
    }
    int index = 0;
    for (const nlohmann::json& item : images) {
        int imageWidth = 0;
        int imageHeight = 0;
        probeImage(item.get<std::string>(), imageWidth, imageHeight);
        if (index == 0) {
            width = imageWidth;
            height = imageHeight;
        } else if (imageWidth != width || imageHeight != height) {
            throw std::invalid_argument("images have inconsistent resolution");
        }
        index += 1;
    }
    runtimeBatchSize = static_cast<int>(images.size());
    std::filesystem::create_directories(output);
}

void BaseImageGenerator::initParams() {
    const auto pipelineType = generator_pipeline::pipelineOf(generator);
    if (!pipelineType) {
        throw std::invalid_argument("unknown generator: " + generator);
    }
    YAML::Node params(YAML::NodeType::Map);
    params["pipeline_name"] = pipelineName;
    params["images"] = jsonToYaml(images);
    params["output"] = output.string();
    params["type"] = *pipelineType;
    params["generator"] = generator;
    params["pgie"] = jsonToYaml(pgie);
    params["analyzer"] = analyzer.has_value() ? jsonToYaml(*analyzer) : YAML::Node(YAML::NodeType::Null);
    params["logger"] = jsonToYaml(logger);
    params["drawer"] = drawer.has_value() ? jsonToYaml(*drawer) : YAML::Node(YAML::NodeType::Null);
    files.insert_or_assign(PARAMS_CONFIG_NAME, ConfigFile(params));
}

void BaseImageGenerator::initPipeline() {
    add();
    link();
    files.insert_or_assign(PIPELINE_CONFIG_NAME, ConfigFile(pipeline));
}

void BaseImageGenerator::initPgie() {
    const nlohmann::json classAttrs = pgie.at("class_attrs");
    const int interval = pgie.value("interval", 1);
    pgie = nlohmann::json{
        {"model_zip", pgie.at("model_zip")},
        {"class_attrs", classAttrs},
        {"interval", interval},
    };
    pgieConfigParser.emplace(pgie.at("model_zip").get<std::string>(), runtimeBatchSize, jsonToYaml(classAttrs), interval);
    const YAML::Node built = pgieConfigParser->build();
    const YAML::Node property = built["property"];
    YAML::Node mappedClassAttrs(YAML::NodeType::Map);
    for (const auto& section : built) {
        const std::string name = section.first.as<std::string>();
        if (name != "property") {
            mappedClassAttrs[name] = section.second;
        }
    }
    const std::optional<std::string> filterOutClassIds = property["filter-out-class-ids"]
        ? std::optional<std::string>(property["filter-out-class-ids"].as<std::string>())
        : std::nullopt;
    pgieGenerator.emplace(
        property["model-engine-file"].as<std::string>(),
        property["labelfile-path"].as<std::string>(),
        property["batch-size"].as<int>(),
        property["network-mode"].as<int>(),
        property["gpu-id"].as<int>(),
        interval,
        mappedClassAttrs,
        filterOutClassIds,
        property["num-detected-classes"].as<int>());
    applyPgieConfig();
    files.insert_or_assign(PGIE_CONFIG_NAME, ConfigFile(pgieGenerator->config));
}

void BaseImageGenerator::initNvdsanalytics() {
    enableNvdsanalytics = analyzer.has_value();
    YAML::Node analytics = enableNvdsanalytics ? NvdsanalyticsGenerator(jsonToYaml(*analyzer)).config : NvdsanalyticsGenerator().config;
    if (!enableNvdsanalytics) {
        analytics["property"]["config-width"] = width;
        analytics["property"]["config-height"] = height;
    }
    files.insert_or_assign(ANALYTICS_CONFIG_NAME, ConfigFile(analytics));
}

void BaseImageGenerator::initKafka() {
    kafkaGenerator.emplace();
    kafkaTopic = std::string(subelement::KAFKA_TOPIC_PREFIX) + pipelineName;
    files.insert_or_assign(KAFKA_CONFIG_NAME, ConfigFile(kafkaGenerator->config));
}

void BaseImageGenerator::applySavePaths(YAML::Node pipelineDocument, const std::filesystem::path& configSaveDir) {
    for (auto node : pipelineDocument["deepstream"]["nodes"]) {
        const std::string name = node["name"].as<std::string>();
        YAML::Node properties = node["properties"] ? node["properties"] : YAML::Node(YAML::NodeType::Map);
        if (name == "pgie") {
            properties["config-file-path"] = (configSaveDir / PGIE_CONFIG_NAME).string();
        }
        if (name == "nvdsanalytics") {
            properties["config-file"] = (configSaveDir / ANALYTICS_CONFIG_NAME).string();
        }
        node["properties"] = properties;
    }
}

std::map<std::string, std::string> BaseImageGenerator::documents(const std::filesystem::path& configSaveDir) {
    YAML::Node pipelineDocument = YAML::Clone(std::get<YAML::Node>(files.at(PIPELINE_CONFIG_NAME)));
    applySavePaths(pipelineDocument, configSaveDir);
    std::map<std::string, std::string> rendered = renderConfigFiles(files, pipelineDocument, PIPELINE_CONFIG_NAME);
    return rendered;
}

void BaseImageGenerator::write(const std::filesystem::path& configSaveDir) {
    std::filesystem::create_directories(configSaveDir);
    const std::map<std::string, std::string> rendered = documents(configSaveDir);
    for (const auto& item : rendered) {
        const std::filesystem::path path = configSaveDir / item.first;
        std::ofstream handle;
        handle.exceptions(std::ios::failbit | std::ios::badbit);
        handle.open(path, std::ios::binary);
        handle << item.second;
    }
}

nlohmann::json BaseImageGenerator::eventOsdKwargs(int gpuId) const {
    const nlohmann::json kwargs = {
        {"gpu_id", gpuId},
        {"display_bbox", true},
        {"display_text", true},
    };
    return kwargs;
}

std::string BaseImageGenerator::nvdetDrawerElement() const {
    const std::string element = "detfadedrawer";
    return element;
}

std::string BaseImageGenerator::nvposeDrawerElement() const {
    const std::string element = "posefadedrawer";
    return element;
}

std::string BaseImageGenerator::nvsegDrawerElement() const {
    const std::string element = "segfadedrawer";
    return element;
}

YAML::Node BaseImageGenerator::nvdetDrawerProperties(const nlohmann::json& drawerNode) {
    const YAML::Node properties = addDetfadedrawer(drawerNode.value("interval", 1), drawerNode.value("fade_time", 0), drawerNode.value("show_label", false));
    return properties;
}

YAML::Node BaseImageGenerator::nvposeDrawerProperties(const nlohmann::json& drawerNode) {
    const YAML::Node properties = addPosefadedrawer(
        drawerNode.value("interval", 1),
        drawerNode.value("fade_time", 0),
        drawerNode.value("show_label", false),
        drawerNode.value("show_pose", true),
        drawerNode.value("pose_threshold", 0.0),
        drawerNode.value("mode", std::string(pipeline::POSE_MODE_COCO17)));
    return properties;
}

YAML::Node BaseImageGenerator::nvsegDrawerProperties(const nlohmann::json& drawerNode) {
    const YAML::Node properties = addSegfadedrawer(
        drawerNode.value("interval", 1),
        drawerNode.value("fade_time", 0),
        drawerNode.value("show_label", false),
        drawerNode.value("show_mask", true));
    return properties;
}

void BaseImageGenerator::add() {
    int index = 0;
    const int gpuId = pgieGenerator->config["property"]["gpu-id"].as<int>();
    for (const nlohmann::json& item : images) {
        appendNode("nvurisrcbin", "nvurisrcbin" + std::to_string(index), addNvurisrcbin(fileUri(item.get<std::string>()), true, 1));
        index += 1;
    }
    appendNode("nvstreammux", "nvstreammux", addNvstreammux(runtimeBatchSize, width, height, false, false, pipeline::BATCHED_PUSH_TIMEOUT_US, gpuId, false));
    appendNode("nvinfer", "pgie", addNvinfer(PGIE_CONFIG_NAME, pgieGenerator->config["property"]["batch-size"].as<int>(), gpuId));
    appendNode("nvdsanalytics", "nvdsanalytics", addNvdsanalytics(ANALYTICS_CONFIG_NAME));
    appendNode("nvstreamdemux", "nvstreamdemux", addNvstreamdemux());
    for (int streamIndex = 0; streamIndex < runtimeBatchSize; ++streamIndex) {
        const std::string suffix = std::to_string(streamIndex);
        const std::filesystem::path source(images.at(streamIndex).get<std::string>());
        const std::filesystem::path sink = output / (source.stem().string() + ".jpg");
        appendNode("queue", "queue_demux" + suffix, addQueue());
        if (drawer.has_value()) {
            appendNode(nvdetDrawerElement(), "detfadedrawer" + suffix, nvdetDrawerProperties(*drawer));
        }
        appendNode("nvosdbin", "nvosdbin" + suffix, addNvosdbin(gpuId, true, true));
        appendNode("nvvideoconvert", "nvvideoconvert" + suffix, addNvvideoconvert(gpuId));
        appendNode("capsfilter", "capsfilter" + suffix, addCapsfilter());
        appendNode(
            "detlogger",
            "detlogger" + suffix,
            addDetlogger(logger.value("interval", 1)));
        appendNode("nvjpegenc", "nvjpegenc" + suffix, addNvjpegenc());
        appendNode("filesink", "filesink" + suffix, addFilesink(sink, false, false));
    }
}

void BaseImageGenerator::link() {
    YAML::Node edges(YAML::NodeType::Map);
    for (int index = 0; index < runtimeBatchSize; ++index) {
        edges["nvurisrcbin" + std::to_string(index)] = "nvstreammux";
    }
    edges["nvstreammux"] = "pgie";
    edges["pgie"] = "nvdsanalytics";
    edges["nvdsanalytics"] = "nvstreamdemux";
    for (int index = 0; index < runtimeBatchSize; ++index) {
        const std::string suffix = std::to_string(index);
        const std::string osdPrev = drawer.has_value() ? "detfadedrawer" + suffix : "nvosdbin" + suffix;
        edges["queue_demux" + suffix] = osdPrev;
        if (drawer.has_value()) {
            edges["detfadedrawer" + suffix] = "nvosdbin" + suffix;
        }
        edges["nvosdbin" + suffix] = "nvvideoconvert" + suffix;
        edges["nvvideoconvert" + suffix] = "capsfilter" + suffix;
        edges["capsfilter" + suffix] = "detlogger" + suffix;
        edges["detlogger" + suffix] = "nvjpegenc" + suffix;
        edges["nvjpegenc" + suffix] = "filesink" + suffix;
    }
    pipeline["deepstream"]["edges"] = edges;
}
