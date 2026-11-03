from pathlib import Path

PROJECT_NAME = "ai_stream2"
LOG_ROOT = Path("/root/logs/export_onnx")
LOG_SUFFIX = ".jsonl"
LOG_MAX_DAYS = 90
LOG_HEADERS = ("authorization", "accept-language")
HOST = "0.0.0.0"
PORT = 8090
SOURCE_DIR = Path("/root/models/source")
TMP_ROOT = Path("/root/tmp")
HISTORY_ROOT = Path("/root/history/export_onnx")
FILE_LABELS = "labels.txt"
RTMPOSE_LABELS = "person\n"
SOURCE_SUFFIXES = {
    "YOLO10-DET": (".pt", ".pth"),
    "YOLO11-DET": (".pt", ".pth"),
    "YOLO11-SEG": (".pt", ".pth"),
    "YOLO26-DET": (".pt", ".pth"),
    "YOLO26-SEG": (".pt", ".pth"),
    "YOLO26-POSE": (".pt", ".pth"),
    "STGCNPP": (".zip",),
    "RTMPOSE": (".onnx",),
}
MULTIPART_BOUNDARY = "export-onnx-boundary"
FIELD_INPUT = "input"
STATE_INPUT = "input"
FIELD_FILENAME = "filename"
FIELD_CONFIG = "config"
HISTORY_BOUNDARY = "export-onnx-history"
