from pathlib import Path

MODEL_ROOT = Path("/root/models")
LOGGER_NAME = "export_trt"
TRTEXEC = Path("/usr/bin/trtexec")
WORKSPACE_MIB = 8192
DEFAULT_PRECISION = "int8"
PRECISION_FLAGS = {"fp16": "--fp16", "int8": "--int8"}
META_JSON_NAME = "meta.json"
LABELS_NAME = "labels.txt"
ONNX_PRECISION = {
    1: "fp32",
    10: "fp16",
    3: "int8",
    2: "uint8",
}
PEOPLENET_CLASSES = ("person", "bag", "face")
PEOPLENET_VERSION = "peoplenet"
PEOPLENET_TASK = "detect"
PEOPLENET_INPUT_NAME = "input_1:0"
PEOPLENET_OUTPUT_NAMES = (
    "output_cov/Sigmoid:0",
    "output_bbox/BiasAdd:0",
)
PEOPLENET_INPUT_CHW = (3, 544, 960)
PEOPLENET_GRID_HW = (34, 60)
PEOPLENET_STRIDE = 16
PEOPLENET_BBOX_NORM = 35.0
PEOPLENET_GRID_OFFSET = 0.5
PEOPLENET_E2E_OUTPUT_NAME = "output0"
PEOPLENET_MAX_DET = 300
PEOPLENET_NMS_CONF = 0.25
PEOPLENET_NMS_IOU = 0.5
