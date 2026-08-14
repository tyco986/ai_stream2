from pathlib import Path

from utils.peoplenet.peoplenet_exporter import PeopleNetExporter
from utils.rtmpose.rtmpose_exporter import RtmposeExporter
from utils.yolo.yolo_det_exporter import YoloDetExporter
from utils.yolo.yolo_seg_exporter import YoloSegExporter


class TrtExporterManager:
    EXPORTERS = {
        "YOLO10-DET": YoloDetExporter,
        "YOLO11-DET": YoloDetExporter,
        "YOLO11-SEG": YoloSegExporter,
        "YOLO26-DET": YoloDetExporter,
        "YOLO26-SEG": YoloSegExporter,
        "PEOPLENET": PeopleNetExporter,
        "RTMPOSE": RtmposeExporter,
    }

    @classmethod
    def types(cls) -> list[str]:
        names = list(cls.EXPORTERS)
        return names

    def export(
        self,
        type: str,
        onnx_dir: Path,
        output_dir: Path,
        batch_size: int | None,
        gpu_id: int,
        precision: str | None,
        opt_level: int | None,
    ) -> None:
        exporter_cls = self.EXPORTERS[type]
        resolved_precision = precision or exporter_cls.default_precision
        exporter_cls().export(
            onnx_dir,
            output_dir,
            batch_size,
            gpu_id,
            resolved_precision,
            opt_level,
        )
