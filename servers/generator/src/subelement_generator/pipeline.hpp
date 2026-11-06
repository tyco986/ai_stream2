#pragma once

#include "constants/pipeline_const.hpp"

#include <filesystem>
#include <optional>
#include <string>

#include <yaml-cpp/yaml.h>

class PipelineGenerator {
public:
    PipelineGenerator();
    virtual ~PipelineGenerator() = default;

    virtual void add();
    virtual void link();
    YAML::Node pipeline;
    const YAML::Node& document() const;
    void write(const std::filesystem::path& savePath);

protected:
    void appendNode(const std::string& elementType, const std::string& name, const YAML::Node& properties);

    YAML::Node addNvurisrcbin(const std::string& uri, bool disableAudio = true, std::optional<int> numBuffers = std::nullopt, int selectRtpProtocol = pipeline::RTP_PROTOCOL_TCP);
    YAML::Node addNvstreammux(int batchSize, int width, int height, bool liveSource = true, bool enablePadding = false, int batchedPushTimeout = pipeline::BATCHED_PUSH_TIMEOUT_US, int gpuId = 0, bool syncInputs = true);
    YAML::Node addNvinfer(const std::string& configFilePath, int batchSize, int gpuId = 0, bool inputTensorMeta = false);
    YAML::Node addNvsahipreprocess(const std::string& configFile, int sliceWidth = pipeline::SLICE_SIZE, int sliceHeight = pipeline::SLICE_SIZE, double overlapWidthRatio = pipeline::OVERLAP_RATIO, double overlapHeightRatio = pipeline::OVERLAP_RATIO, bool enableFullFrame = true, int gpuId = 0, int uniqueId = pipeline::PREPROCESS_UNIQUE_ID, const std::string& targetUniqueIds = pipeline::TARGET_UNIQUE_IDS_DEFAULT);
    YAML::Node addNvdspreprocess(const std::string& configFile);
    YAML::Node addNvsahipostprocess(const std::string& gieIds = pipeline::GIE_IDS_DEFAULT, int matchMetric = pipeline::MATCH_METRIC_IOS, double matchThreshold = pipeline::MATCH_THRESHOLD, bool classAgnostic = false, bool enableMerge = true, bool twoPhaseNmm = true);
    YAML::Node addNvrtmposepostprocess(int sgieUniqueId = pipeline::POSE_SGIE_UNIQUE_ID);
    YAML::Node addStgcnppactionlabel(int classifierUniqueId = pipeline::CLASSIFIER_UNIQUE_ID);
    YAML::Node addNvsahipostprocessPose(const std::string& gieIds = pipeline::GIE_IDS_DEFAULT, bool classAgnostic = false, double oksThreshold = pipeline::OKS_THRESHOLD, double visThreshold = 0.0, int numKeypoints = 0);
    YAML::Node addNvtracker(const std::string& llLibFile, const std::string& llConfigFile, int trackerWidth, int trackerHeight, int gpuId = 0, const std::string& operateOnClassIds = "");
    YAML::Node addBoxsnapshot();
    YAML::Node addNvdsanalytics(const std::string& configFile, int gpuId = 0);
    YAML::Node addDetlogger(const std::string& root = "", int interval = 0);
    YAML::Node addPresencelogger(const std::string& root = "", int interval = 0);
    YAML::Node addStgcnpplogger(const std::string& root = "", int interval = 0);
    YAML::Node addDetfadedrawer(int interval = 0, int fadeTime = 0, bool showLabel = false);
    YAML::Node addDetfadedrawerwithtracker(int interval = 0, int fadeTime = 0, bool showLabel = false, bool showSnap = true);
    YAML::Node addPosefadedrawer(int interval = 0, int fadeTime = 0, bool showLabel = false, bool showPose = true, double poseThreshold = 0.0, const std::string& mode = pipeline::POSE_MODE_COCO17);
    YAML::Node addPosefadedrawerwithtracker(int interval = 0, int fadeTime = 0, bool showLabel = false, bool showPose = true, double poseThreshold = 0.0, const std::string& mode = pipeline::POSE_MODE_COCO17, bool showSnap = true);
    YAML::Node addStgcnppfadedrawerwithtracker(int interval = 0, int fadeTime = 0, bool showLabel = false, bool showPose = true, double poseThreshold = 0.0, const std::string& mode = pipeline::POSE_MODE_COCO17, bool showSnap = true);
    YAML::Node addSegfadedrawer(int interval = 0, int fadeTime = 0, bool showLabel = false, bool showMask = true);
    YAML::Node addSegfadedrawerwithtracker(int interval = 0, int fadeTime = 0, bool showLabel = false, bool showMask = true, bool showSnap = true);
    YAML::Node addPresenceeventcoder(const std::string& classIds = "", const std::string& eventNames = "", int length = pipeline::PRESENCE_LENGTH, double threshold = pipeline::PRESENCE_THRESHOLD, const std::string& mode = pipeline::PRESENCE_MODE_FOLD);
    YAML::Node addRawcapturer(const std::string& outputDir, const std::string& captureCodes = pipeline::CAPTURE_CODES_DEFAULT);
    YAML::Node addViscapturer(const std::string& outputDir, const std::string& captureCodes = pipeline::CAPTURE_CODES_DEFAULT, const std::string& labelTask = pipeline::LABEL_TASK_DET);
    YAML::Node addTee();
    YAML::Node addQueue(int leaky = pipeline::QUEUE_LEAKY_DOWNSTREAM, int maxSizeBuffers = pipeline::QUEUE_MAX_BUFFERS);
    YAML::Node addNvmsgconv(const std::string& config, int payloadType = pipeline::PAYLOAD_DEEPSTREAM_MINIMAL, bool msg2pNewapi = true, int frameInterval = pipeline::FRAME_INTERVAL);
    YAML::Node addNvmsgbroker(const std::string& protoLib, const std::string& connStr, const std::string& topic, const std::string& brokerConfig, bool sync = false, bool asyncFlag = false);
    YAML::Node addNvstreamdemux();
    YAML::Node addNvosdbin(int gpuId = 0, bool displayBbox = true, bool displayText = true, bool displayMask = false);
    YAML::Node addNvvideoconvert(int gpuId = 0);
    YAML::Node addCapsfilter(const std::string& caps = pipeline::DEFAULT_CAPS);
    YAML::Node addNvv4l2h264enc(int bitrate = pipeline::H264_BITRATE, int iframeInterval = pipeline::H264_IFRAME_INTERVAL, int presetId = pipeline::H264_PRESET_ID, int gpuId = 0);
    YAML::Node addH264parse();
    YAML::Node addMp4mux();
    YAML::Node addRtspclientsink(const std::string& location, bool sync = false, bool asyncFlag = false);
    YAML::Node addFakesink(bool sync = false, bool asyncFlag = false);
    YAML::Node addNvjpegenc(int quality = pipeline::JPEG_QUALITY);
    YAML::Node addFilesink(const std::filesystem::path& location, bool sync = false, bool asyncFlag = false);
};
