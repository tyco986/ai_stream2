from pathlib import Path
from copy import deepcopy
import yaml


nvtracker_default_config = {
        "BaseConfig": {
            "minDetectorConfidence": 0,
        },
        "TargetManagement": {
            "enableBboxUnClipping": 1,
            "maxTargetsPerStream": 150,
            "minIouDiff4NewTarget": 0.5,
            "minTrackerConfidence": 0.2,
            "probationAge": 3,
            "maxShadowTrackingAge": 30,
            "earlyTerminationAge": 1,
        },
        "TrajectoryManagement": {
            "useUniqueID": 0,
        },
        "DataAssociator": {
            "dataAssociatorType": 0,
            "associationMatcherType": 0,
            "checkClassMatch": 1,
            "minMatchingScore4Overall": 0.0,
            "minMatchingScore4SizeSimilarity": 0.6,
            "minMatchingScore4Iou": 0.0,
            "minMatchingScore4VisualSimilarity": 0.7,
            "matchingScoreWeight4VisualSimilarity": 0.6,
            "matchingScoreWeight4SizeSimilarity": 0.0,
            "matchingScoreWeight4Iou": 0.4,
        },
        "StateEstimator": {
            "stateEstimatorType": 1,
            "processNoiseVar4Loc": 2.0,
            "processNoiseVar4Size": 1.0,
            "processNoiseVar4Vel": 0.1,
            "measurementNoiseVar4Detector": 4.0,
            "measurementNoiseVar4Tracker": 16.0,
        },
        "VisualTracker": {
            "visualTrackerType": 1,
            "useColorNames": 1,
            "useHog": 0,
            "featureImgSizeLevel": 2,
            "featureFocusOffsetFactor_y": -0.2,
            "filterLr": 0.075,
            "filterChannelWeightsLr": 0.1,
            "gaussianSigma": 0.75,
        },
    }

class NvtrackerGenerator:
    def __init__(
        self,
        maxShadowTrackingAge: int = 30,
        earlyTerminationAge: int = 1,
        probationAge: int = 3,
    ) -> None:
        self.maxShadowTrackingAge = maxShadowTrackingAge
        self.earlyTerminationAge = earlyTerminationAge
        self.probationAge = probationAge
        self.config = deepcopy(nvtracker_default_config)
        self.config["TargetManagement"]["maxShadowTrackingAge"] = self.maxShadowTrackingAge
        self.config["TargetManagement"]["earlyTerminationAge"] = self.earlyTerminationAge
        self.config["TargetManagement"]["probationAge"] = self.probationAge

    def write(self, save_path: str | Path) -> None:
        with open(save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(self.config, handle, sort_keys=False, default_flow_style=False)


if __name__ == "__main__":
    nvtracker_generator = NvtrackerGenerator()
    print(nvtracker_generator.config)