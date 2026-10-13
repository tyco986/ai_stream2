from pathlib import Path

MODEL_ROOT = Path("/root/models")
LOGGER_NAME = "export_trt"
TRTEXEC = Path("/usr/bin/trtexec")
WORKSPACE_MIB = 8192
DEFAULT_PRECISION = "fp32"
PRECISION_FLAGS = {"fp16": "--fp16", "int8": "--int8"}
META_JSON_NAME = "meta.json"
LABELS_NAME = "labels.txt"
ONNX_PRECISION = {
    1: "fp32",
    10: "fp16",
    3: "int8",
    2: "uint8",
}
RTMPOSE_VERSION = "rtmpose"
RTMPOSE_TASK = "pose"
RTMPOSE_INPUT_NAME = "input"
RTMPOSE_BACKBONE_OUTPUT_NAMES = ("simcc_x", "simcc_y")
RTMPOSE_OUTPUT_NAME = "keypoints"
COCO_BODY_17 = (
    "nose",
    "left_eye",
    "right_eye",
    "left_ear",
    "right_ear",
    "left_shoulder",
    "right_shoulder",
    "left_elbow",
    "right_elbow",
    "left_wrist",
    "right_wrist",
    "left_hip",
    "right_hip",
    "left_knee",
    "right_knee",
    "left_ankle",
    "right_ankle",
)
COCO_WHOLEBODY_FOOT_6 = (
    "left_big_toe",
    "left_small_toe",
    "left_heel",
    "right_big_toe",
    "right_small_toe",
    "right_heel",
)
COCO_WHOLEBODY_133 = (
    *COCO_BODY_17,
    *COCO_WHOLEBODY_FOOT_6,
    *[f"face_{index}" for index in range(68)],
    *[f"left_hand_{index}" for index in range(21)],
    *[f"right_hand_{index}" for index in range(21)],
)
KEYPOINT_NAMES = {
    17: COCO_BODY_17,
    133: COCO_WHOLEBODY_133,
}
