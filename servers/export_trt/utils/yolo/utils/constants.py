from pathlib import Path

MODEL_ROOT = Path("/root/models")
LOGGER_NAME = "export_trt"
TRTEXEC = Path("/usr/bin/trtexec")
LIBS_ROOT = Path("/opt/ai_stream2/servers/export_trt/libs")
WORKSPACE_MIB = 8192
DEFAULT_PRECISION = "fp16"
PRECISION_FLAGS = {"fp16": "--fp16", "int8": "--int8"}
META_JSON_NAME = "meta.json"
LABELS_NAME = "labels.txt"
YOLO_SEG_PLUGIN_NAME = "libnvdsinfer_custom_impl_Yolo_seg.so"
IMAGE_SUFFIXES = {".jpg", ".jpeg", ".png", ".bmp", ".webp"}
NET_SCALE_FACTOR = 0.0039215697906911373
CALIB_COUNT = 500
CALIB_BATCH = 8
INT8_CALIB_ZIP = Path("/root/attachments/int8_calib.zip")
