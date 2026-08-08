import os
from pathlib import Path

PROJECT_NAME = os.environ.get("PROJECT_NAME", "ai_stream2")
LOG_FORMAT = "[%(asctime)s] %(levelname)s %(name)s: %(message)s"
LOGGER_NAME = "export_trt_api"
MODEL_ROOT = Path("/root/models")
LOG_ROOT = Path("/root/logs/export_trt")
DEFAULT_HOST = os.environ.get("HOST", "0.0.0.0")
DEFAULT_PORT = int(os.environ.get("PORT", "9000"))
TRTEXEC = Path("/usr/bin/trtexec")
LIBS_ROOT = Path("/opt/ai_stream2/servers/export_trt/libs")
WORKSPACE_MIB = 8192
DEFAULT_PRECISION = "fp16"
AVAILABLE_PRECISION = frozenset({"fp32", "fp16", "int8"})
PRECISION_FLAGS = {"fp16": "--fp16", "int8": "--int8"}
META_JSON_NAME = "meta.json"
LABELS_NAME = "labels.txt"
YOLO_PLUGIN_SUFFIX = {"detect": "", "segment": "_seg"}
