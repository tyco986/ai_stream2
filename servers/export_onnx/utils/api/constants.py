import os
from dataclasses import dataclass
from pathlib import Path

import onnx

from utils.yolo_e2e.yolo_det_e2e_exporter import YoloDetE2EExporter
from utils.yolo_e2e.yolo_seg_e2e_exporter import YoloSegE2EExporter
from utils.yolo_non_e2e.yolo_det_non_e2e_exporter import YoloDetNonE2EExporter
from utils.yolo_non_e2e.yolo_seg_non_e2e_exporter import YoloSegNonE2EExporter

PROJECT_NAME = os.environ.get("PROJECT_NAME", "ai_stream2")
LOG_FORMAT = "[%(asctime)s] %(levelname)s %(name)s: %(message)s"
LOGGER_NAME = "export_onnx_api"
DEFAULT_MODEL_ROOT = Path("/root/models")
DEFAULT_LOG_ROOT = Path("/root/logs/export_onnx")
DEFAULT_HOST = os.environ.get("HOST", "0.0.0.0")
DEFAULT_PORT = int(os.environ.get("PORT", "8090"))
DEFAULT_MAX_DET = 30
DEFAULT_CONF = 0.25
DEFAULT_IOU = 0.45

ONNX_PRECISION = {
    int(onnx.TensorProto.FLOAT): "fp32",
    int(onnx.TensorProto.FLOAT16): "fp16",
    int(onnx.TensorProto.INT8): "int8",
    int(onnx.TensorProto.UINT8): "uint8",
}

TASK_LABELS = {
    "detect": "DET",
    "segment": "SEG",
}


@dataclass(frozen=True)
class ExportSpec:
    exporter_cls: type
    family: str
    task: str
    yolo_export: str = "e2e"
    default_max_det: int = DEFAULT_MAX_DET

    @property
    def uses_iou(self) -> bool:
        return self.yolo_export in {"non_e2e", "non_e2e_seg"}

    @property
    def label(self) -> str:
        return f"{self.family.upper()}-{TASK_LABELS[self.task]}"


EXPORT_SPECS: dict[str, ExportSpec] = {
    "export_yolo10": ExportSpec(
        YoloDetE2EExporter,
        "yolo10",
        "detect",
        yolo_export="e2e",
    ),
    "export_yolo26": ExportSpec(
        YoloDetE2EExporter,
        "yolo26",
        "detect",
        yolo_export="e2e",
    ),
    "export_yolo26_seg": ExportSpec(
        YoloSegE2EExporter,
        "yolo26",
        "segment",
        yolo_export="e2e_seg",
    ),
    "export_yolo11": ExportSpec(
        YoloDetNonE2EExporter,
        "yolo11",
        "detect",
        yolo_export="non_e2e",
    ),
    "export_yolo11_seg": ExportSpec(
        YoloSegNonE2EExporter,
        "yolo11",
        "segment",
        yolo_export="non_e2e_seg",
    ),
}
