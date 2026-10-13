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
STGCNPP_VERSION = "stgcnpp"
STGCNPP_TASK = "action"
STGCNPP_INPUT_NAME = "input"
STGCNPP_OUTPUT_NAME = "output"
STGCNPP_INPUT_TAIL = (2, 100, 17, 3)
STGCNPP_NUM_CLASSES = 60
