#include "subelement_generator/nvtracker.hpp"

#include <fstream>

NvtrackerGenerator::NvtrackerGenerator(int maxShadowTrackingAge, int earlyTerminationAge, int probationAge)
    : maxShadowTrackingAge(maxShadowTrackingAge),
      earlyTerminationAge(earlyTerminationAge),
      probationAge(probationAge) {
    config["BaseConfig"]["minDetectorConfidence"] = 0;
    config["TargetManagement"]["enableBboxUnClipping"] = 1;
    config["TargetManagement"]["maxTargetsPerStream"] = 150;
    config["TargetManagement"]["minIouDiff4NewTarget"] = 0.5;
    config["TargetManagement"]["minTrackerConfidence"] = 0.2;
    config["TargetManagement"]["probationAge"] = this->probationAge;
    config["TargetManagement"]["maxShadowTrackingAge"] = this->maxShadowTrackingAge;
    config["TargetManagement"]["earlyTerminationAge"] = this->earlyTerminationAge;
    config["TrajectoryManagement"]["useUniqueID"] = 0;
    config["DataAssociator"]["dataAssociatorType"] = 0;
    config["DataAssociator"]["associationMatcherType"] = 0;
    config["DataAssociator"]["checkClassMatch"] = 1;
    config["DataAssociator"]["minMatchingScore4Overall"] = 0.0;
    config["DataAssociator"]["minMatchingScore4SizeSimilarity"] = 0.6;
    config["DataAssociator"]["minMatchingScore4Iou"] = 0.0;
    config["DataAssociator"]["minMatchingScore4VisualSimilarity"] = 0.7;
    config["DataAssociator"]["matchingScoreWeight4VisualSimilarity"] = 0.6;
    config["DataAssociator"]["matchingScoreWeight4SizeSimilarity"] = 0.0;
    config["DataAssociator"]["matchingScoreWeight4Iou"] = 0.4;
    config["StateEstimator"]["stateEstimatorType"] = 1;
    config["StateEstimator"]["processNoiseVar4Loc"] = 2.0;
    config["StateEstimator"]["processNoiseVar4Size"] = 1.0;
    config["StateEstimator"]["processNoiseVar4Vel"] = 0.1;
    config["StateEstimator"]["measurementNoiseVar4Detector"] = 4.0;
    config["StateEstimator"]["measurementNoiseVar4Tracker"] = 16.0;
    config["VisualTracker"]["visualTrackerType"] = 1;
    config["VisualTracker"]["useColorNames"] = 1;
    config["VisualTracker"]["useHog"] = 0;
    config["VisualTracker"]["featureImgSizeLevel"] = 2;
    config["VisualTracker"]["featureFocusOffsetFactor_y"] = -0.2;
    config["VisualTracker"]["filterLr"] = 0.075;
    config["VisualTracker"]["filterChannelWeightsLr"] = 0.1;
    config["VisualTracker"]["gaussianSigma"] = 0.75;
}

void NvtrackerGenerator::write(const std::filesystem::path& savePath) {
    const std::filesystem::path path = std::filesystem::absolute(savePath);
    std::filesystem::create_directories(path.parent_path());
    std::ofstream handle;
    handle.exceptions(std::ios::failbit | std::ios::badbit);
    handle.open(path);
    handle << YAML::Dump(config);
}
