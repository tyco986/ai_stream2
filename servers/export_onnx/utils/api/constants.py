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
TYPE_STGCNPP = "STGCNPP"
WEIGHT_SUFFIXES = (".pt", ".pth")
RECIPES = {
    "YOLO10-DET": {
        "source_suffixes": [".pt", ".pth"],
        "export_onnx": {"size": 640, "opset": 18, "batch_size": None, "batch_mode": "dynamic", "simplify": False, "max_det": 30, "conf": 0.25, "iou": None},
        "export_trt": {"batch_size": 18, "device": 0, "precision": "fp16", "opt_level": 0},
    },
    "YOLO11-DET": {
        "source_suffixes": [".pt", ".pth"],
        "export_onnx": {"size": 640, "opset": 18, "batch_size": None, "batch_mode": "dynamic", "simplify": False, "max_det": 30, "conf": 0.25, "iou": 0.45},
        "export_trt": {"batch_size": 18, "device": 0, "precision": "fp16", "opt_level": 3},
    },
    "YOLO11-SEG": {
        "source_suffixes": [".pt", ".pth"],
        "export_onnx": {"size": 640, "opset": 18, "batch_size": None, "batch_mode": "dynamic", "simplify": False, "max_det": 30, "conf": 0.25, "iou": 0.45},
        "export_trt": {"batch_size": 18, "device": 0, "precision": "fp16", "opt_level": 3},
    },
    "YOLO26-DET": {
        "source_suffixes": [".pt", ".pth"],
        "export_onnx": {"size": 640, "opset": 18, "batch_size": None, "batch_mode": "dynamic", "simplify": False, "max_det": 30, "conf": 0.25, "iou": None},
        "export_trt": {"batch_size": 18, "device": 0, "precision": "fp16", "opt_level": 3},
    },
    "YOLO26-SEG": {
        "source_suffixes": [".pt", ".pth"],
        "export_onnx": {"size": 640, "opset": 18, "batch_size": None, "batch_mode": "dynamic", "simplify": False, "max_det": 30, "conf": 0.25, "iou": None},
        "export_trt": {"batch_size": 18, "device": 0, "precision": "fp16", "opt_level": 3},
    },
    "YOLO26-POSE": {
        "source_suffixes": [".pt", ".pth"],
        "export_onnx": {"size": 640, "opset": 18, "batch_size": None, "batch_mode": "dynamic", "simplify": False, "max_det": 30, "conf": 0.25, "iou": None},
        "export_trt": {"batch_size": 18, "device": 0, "precision": "fp16", "opt_level": 3},
    },
    "STGCNPP": {
        "source_suffixes": [".zip"],
        "export_onnx": {"size": None, "opset": 18, "batch_size": None, "batch_mode": "dynamic", "simplify": False, "max_det": None, "conf": None, "iou": None},
        "export_trt": {"batch_size": 64, "device": 0, "precision": "fp16", "opt_level": None},
    },
    "RTMPOSE": {
        "source_suffixes": [".onnx"],
        "export_onnx": {"size": None, "opset": 18, "batch_size": None, "batch_mode": "dynamic", "simplify": False, "max_det": None, "conf": None, "iou": None},
        "export_trt": {"batch_size": 64, "device": 0, "precision": "fp16", "opt_level": None},
    },
}
MULTIPART_BOUNDARY = "export-onnx-boundary"
FIELD_INPUT = "input"
STATE_INPUT = "input"
FIELD_FILENAME = "filename"
FIELD_CONFIG = "config"
HISTORY_BOUNDARY = "export-onnx-history"
