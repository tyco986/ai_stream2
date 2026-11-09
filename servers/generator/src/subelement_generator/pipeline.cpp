#include "subelement_generator/pipeline.hpp"

#include <fstream>

PipelineGenerator::PipelineGenerator() {
    pipeline["deepstream"]["nodes"] = YAML::Node(YAML::NodeType::Sequence);
    pipeline["deepstream"]["edges"] = YAML::Node(YAML::NodeType::Map);
}

void PipelineGenerator::add() {
}

void PipelineGenerator::link() {
}

const YAML::Node& PipelineGenerator::document() const {
    return pipeline;
}

void PipelineGenerator::write(const std::filesystem::path& savePath) {
    const std::filesystem::path path = std::filesystem::absolute(savePath);
    std::filesystem::create_directories(path.parent_path());
    std::ofstream handle;
    handle.exceptions(std::ios::failbit | std::ios::badbit);
    handle.open(path);
    handle << YAML::Dump(pipeline);
}

void PipelineGenerator::appendNode(const std::string& elementType, const std::string& name, const YAML::Node& properties) {
    YAML::Node node;
    node["type"] = elementType;
    node["name"] = name;
    if (properties && !properties.IsNull() && properties.size() > 0) {
        node["properties"] = properties;
    }
    pipeline["deepstream"]["nodes"].push_back(node);
}

YAML::Node PipelineGenerator::addNvurisrcbin(const std::string& uri, bool disableAudio, std::optional<int> numBuffers, int selectRtpProtocol) {
    YAML::Node properties;
    properties["uri"] = uri;
    properties["disable-audio"] = disableAudio;
    properties["select-rtp-protocol"] = selectRtpProtocol;
    if (numBuffers.has_value()) {
        properties["num-buffers"] = *numBuffers;
    }
    return properties;
}

YAML::Node PipelineGenerator::addNvstreammux(int batchSize, int width, int height, bool liveSource, bool enablePadding, int batchedPushTimeout, int gpuId, bool syncInputs) {
    YAML::Node properties;
    properties["batch-size"] = batchSize;
    properties["width"] = width;
    properties["height"] = height;
    properties["enable-padding"] = enablePadding;
    properties["batched-push-timeout"] = batchedPushTimeout;
    properties["gpu-id"] = gpuId;
    properties["live-source"] = liveSource;
    properties["sync-inputs"] = syncInputs;
    return properties;
}

YAML::Node PipelineGenerator::addNvinfer(const std::string& configFilePath, int batchSize, int gpuId, bool inputTensorMeta) {
    YAML::Node properties;
    properties["config-file-path"] = configFilePath;
    properties["batch-size"] = batchSize;
    properties["gpu-id"] = gpuId;
    if (inputTensorMeta) {
        properties["input-tensor-meta"] = true;
        properties["clip-object-outside-roi"] = false;
    }
    return properties;
}

YAML::Node PipelineGenerator::addNvsahipreprocess(const std::string& configFile, int sliceWidth, int sliceHeight, double overlapWidthRatio, double overlapHeightRatio, bool enableFullFrame, int gpuId, int uniqueId, const std::string& targetUniqueIds) {
    YAML::Node properties;
    properties["config-file"] = configFile;
    properties["slice-width"] = sliceWidth;
    properties["slice-height"] = sliceHeight;
    properties["overlap-width-ratio"] = overlapWidthRatio;
    properties["overlap-height-ratio"] = overlapHeightRatio;
    properties["enable-full-frame"] = enableFullFrame;
    properties["gpu-id"] = gpuId;
    properties["unique-id"] = uniqueId;
    properties["target-unique-ids"] = targetUniqueIds;
    return properties;
}

YAML::Node PipelineGenerator::addNvdspreprocess(const std::string& configFile) {
    YAML::Node properties;
    properties["config-file"] = configFile;
    return properties;
}

YAML::Node PipelineGenerator::addNvsahipostprocess(const std::string& gieIds, int matchMetric, double matchThreshold, bool classAgnostic, bool enableMerge, bool twoPhaseNmm, bool preferFullFrame) {
    YAML::Node properties;
    properties["gie-ids"] = gieIds;
    properties["match-metric"] = matchMetric;
    properties["match-threshold"] = matchThreshold;
    properties["class-agnostic"] = classAgnostic;
    properties["enable-merge"] = enableMerge;
    properties["two-phase-nmm"] = twoPhaseNmm;
    properties["prefer-full-frame"] = preferFullFrame;
    return properties;
}

YAML::Node PipelineGenerator::addNvrtmposepostprocess(int sgieUniqueId) {
    YAML::Node properties;
    properties["sgie-unique-id"] = sgieUniqueId;
    return properties;
}

YAML::Node PipelineGenerator::addStgcnppactionlabel(int classifierUniqueId) {
    YAML::Node properties;
    properties["classifier-unique-id"] = classifierUniqueId;
    return properties;
}

YAML::Node PipelineGenerator::addNvsahipostprocessPose(const std::string& gieIds, bool classAgnostic, int matchMetric, double matchThreshold, bool preferFullFrame, int maxDetections, double oksThreshold, double visThreshold, int numKeypoints) {
    YAML::Node properties;
    properties["gie-ids"] = gieIds;
    properties["class-agnostic"] = classAgnostic;
    properties["match-metric"] = matchMetric;
    properties["match-threshold"] = matchThreshold;
    properties["prefer-full-frame"] = preferFullFrame;
    properties["max-detections"] = maxDetections;
    properties["oks-threshold"] = oksThreshold;
    properties["vis-threshold"] = visThreshold;
    properties["num-keypoints"] = numKeypoints;
    return properties;
}

YAML::Node PipelineGenerator::addNvtracker(const std::string& llLibFile, const std::string& llConfigFile, int trackerWidth, int trackerHeight, int gpuId, const std::string& operateOnClassIds) {
    YAML::Node properties;
    properties["ll-lib-file"] = llLibFile;
    properties["ll-config-file"] = llConfigFile;
    properties["tracker-width"] = trackerWidth;
    properties["tracker-height"] = trackerHeight;
    properties["gpu-id"] = gpuId;
    properties["operate-on-class-ids"] = operateOnClassIds;
    return properties;
}

YAML::Node PipelineGenerator::addBoxsnapshot() {
    return YAML::Node(YAML::NodeType::Map);
}

YAML::Node PipelineGenerator::addNvdsanalytics(const std::string& configFile) {
    YAML::Node properties;
    properties["config-file"] = configFile;
    return properties;
}

YAML::Node PipelineGenerator::addDetlogger(int interval) {
    YAML::Node properties;
    properties["interval"] = interval;
    return properties;
}

YAML::Node PipelineGenerator::addPresencelogger(int interval) {
    return addDetlogger(interval);
}

YAML::Node PipelineGenerator::addStgcnpplogger(int interval) {
    return addDetlogger(interval);
}

YAML::Node PipelineGenerator::addDetfadedrawer(int interval, int fadeTime, bool showLabel) {
    YAML::Node properties;
    properties["interval"] = interval;
    properties["fade-time"] = fadeTime;
    properties["show-label"] = showLabel;
    return properties;
}

YAML::Node PipelineGenerator::addDetfadedrawerwithtracker(int interval, int fadeTime, bool showLabel, bool showSnap) {
    YAML::Node properties;
    properties["interval"] = interval;
    properties["fade-time"] = fadeTime;
    properties["show-label"] = showLabel;
    properties["show-snap"] = showSnap;
    return properties;
}

YAML::Node PipelineGenerator::addPosefadedrawer(int interval, int fadeTime, bool showLabel, bool showPose, double poseThreshold, const std::string& mode) {
    YAML::Node properties;
    properties["interval"] = interval;
    properties["fade-time"] = fadeTime;
    properties["show-label"] = showLabel;
    properties["show-pose"] = showPose;
    properties["pose-threshold"] = poseThreshold;
    properties["mode"] = mode;
    return properties;
}

YAML::Node PipelineGenerator::addPosefadedrawerwithtracker(int interval, int fadeTime, bool showLabel, bool showPose, double poseThreshold, const std::string& mode, bool showSnap) {
    YAML::Node properties;
    properties["interval"] = interval;
    properties["fade-time"] = fadeTime;
    properties["show-label"] = showLabel;
    properties["show-pose"] = showPose;
    properties["pose-threshold"] = poseThreshold;
    properties["mode"] = mode;
    properties["show-snap"] = showSnap;
    return properties;
}

YAML::Node PipelineGenerator::addStgcnppfadedrawerwithtracker(int interval, int fadeTime, bool showLabel, bool showPose, double poseThreshold, const std::string& mode, bool showSnap) {
    return addPosefadedrawerwithtracker(interval, fadeTime, showLabel, showPose, poseThreshold, mode, showSnap);
}

YAML::Node PipelineGenerator::addSegfadedrawer(int interval, int fadeTime, bool showLabel, bool showMask) {
    YAML::Node properties;
    properties["interval"] = interval;
    properties["fade-time"] = fadeTime;
    properties["show-label"] = showLabel;
    properties["show-mask"] = showMask;
    return properties;
}

YAML::Node PipelineGenerator::addSegfadedrawerwithtracker(int interval, int fadeTime, bool showLabel, bool showMask, bool showSnap) {
    YAML::Node properties;
    properties["interval"] = interval;
    properties["fade-time"] = fadeTime;
    properties["show-label"] = showLabel;
    properties["show-mask"] = showMask;
    properties["show-snap"] = showSnap;
    return properties;
}

YAML::Node PipelineGenerator::addPresenceeventcoder(const std::string& classEventMap, int length, double threshold, const std::string& mode) {
    YAML::Node properties;
    properties["class-event-map"] = classEventMap;
    properties["length"] = length;
    properties["threshold"] = threshold;
    properties["mode"] = mode;
    return properties;
}

YAML::Node PipelineGenerator::addRawcapturer(const std::string& captureCodes) {
    YAML::Node properties;
    properties["capture-codes"] = captureCodes;
    return properties;
}

YAML::Node PipelineGenerator::addViscapturer(const std::string& captureCodes, const std::string& labelTask) {
    YAML::Node properties;
    properties["capture-codes"] = captureCodes;
    properties["label-task"] = labelTask;
    return properties;
}

YAML::Node PipelineGenerator::addTee() {
    return YAML::Node(YAML::NodeType::Map);
}

YAML::Node PipelineGenerator::addQueue(int leaky, int maxSizeBuffers) {
    YAML::Node properties;
    properties["leaky"] = leaky;
    properties["max-size-buffers"] = maxSizeBuffers;
    return properties;
}

YAML::Node PipelineGenerator::addNvmsgconv(const std::string& config, const std::string& msg2pLib, int payloadType, bool msg2pNewapi, int frameInterval) {
    YAML::Node properties;
    properties["config"] = config;
    properties["payload-type"] = payloadType;
    properties["msg2p-lib"] = msg2pLib;
    properties["msg2p-newapi"] = msg2pNewapi;
    properties["frame-interval"] = frameInterval;
    return properties;
}

YAML::Node PipelineGenerator::addNvmsgbroker(const std::string& protoLib, const std::string& connStr, const std::string& topic, const std::string& brokerConfig, bool sync, bool asyncFlag) {
    YAML::Node properties;
    properties["proto-lib"] = protoLib;
    properties["conn-str"] = connStr;
    properties["topic"] = topic;
    properties["config"] = brokerConfig;
    properties["sync"] = sync;
    properties["async"] = asyncFlag;
    return properties;
}

YAML::Node PipelineGenerator::addNvstreamdemux() {
    return YAML::Node(YAML::NodeType::Map);
}

YAML::Node PipelineGenerator::addNvosdbin(int gpuId, bool displayBbox, bool displayText, bool displayMask) {
    YAML::Node properties;
    properties["gpu-id"] = gpuId;
    properties["display-bbox"] = displayBbox;
    properties["display-text"] = displayText;
    properties["display-mask"] = displayMask;
    return properties;
}

YAML::Node PipelineGenerator::addNvvideoconvert(int gpuId, bool disablePassthrough) {
    YAML::Node properties;
    properties["gpu-id"] = gpuId;
    properties["disable-passthrough"] = disablePassthrough;
    return properties;
}

YAML::Node PipelineGenerator::addCapsfilter(const std::string& caps) {
    YAML::Node properties;
    properties["caps"] = caps;
    return properties;
}

YAML::Node PipelineGenerator::addNvv4l2h264enc(int bitrate, int iframeInterval, int presetId, int gpuId) {
    YAML::Node properties;
    properties["bitrate"] = bitrate;
    properties["iframeinterval"] = iframeInterval;
    properties["preset-id"] = presetId;
    properties["gpu-id"] = gpuId;
    return properties;
}

YAML::Node PipelineGenerator::addH264parse() {
    return YAML::Node(YAML::NodeType::Map);
}

YAML::Node PipelineGenerator::addMp4mux() {
    return YAML::Node(YAML::NodeType::Map);
}

YAML::Node PipelineGenerator::addRtspclientsink(const std::string& location, bool sync, bool asyncFlag) {
    YAML::Node properties;
    properties["location"] = location;
    properties["sync"] = sync;
    properties["async"] = asyncFlag;
    return properties;
}

YAML::Node PipelineGenerator::addFakesink(bool sync, bool asyncFlag) {
    YAML::Node properties;
    properties["sync"] = sync;
    properties["async"] = asyncFlag;
    return properties;
}

YAML::Node PipelineGenerator::addNvjpegenc(int quality) {
    YAML::Node properties;
    properties["quality"] = quality;
    return properties;
}

YAML::Node PipelineGenerator::addFilesink(const std::filesystem::path& location, bool sync, bool asyncFlag) {
    YAML::Node properties;
    properties["location"] = std::filesystem::absolute(location).string();
    properties["sync"] = sync;
    properties["async"] = asyncFlag;
    return properties;
}
