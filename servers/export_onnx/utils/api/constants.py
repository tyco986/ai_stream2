from pathlib import Path

PROJECT_NAME = "ai_stream2"
LOG_ROOT = Path("/root/logs/export_onnx")
LOG_PAGE = "export_onnx"
LOG_SUFFIX = ".jsonl"
LOG_MAX_DAYS = 90
LOG_HEADERS = ("authorization", "accept-language")
HOST = "0.0.0.0"
PORT = 8090
TMP_ROOT = Path("/root/tmp")
HISTORY_ROOT = Path("/root/history")
ACCEPTED_SUFFIXES = {".pt", ".onnx"}
MULTIPART_BOUNDARY = "export-onnx-boundary"
FIELD_INPUT = "input"
FIELD_FILENAME = "filename"
FIELD_CONFIG = "config"
FIELD_LABEL = "label"
HISTORY_BOUNDARY = "export-onnx-history"
LABEL_TYPES = {"STGCNPP", "RTMPOSE"}
