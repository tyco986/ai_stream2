from pathlib import Path

from utils.stgcnpp.stgcnpp_exporter import StgcnppExporter
from utils.yolo_e2e.yolo_det_e2e_exporter import YoloDetE2EExporter
from utils.yolo_e2e.yolo_seg_e2e_exporter import YoloSegE2EExporter
from utils.yolo_non_e2e.yolo_det_non_e2e_exporter import YoloDetNonE2EExporter
from utils.yolo_non_e2e.yolo_seg_non_e2e_exporter import YoloSegNonE2EExporter

IOU_EXPORTERS = {YoloDetNonE2EExporter, YoloSegNonE2EExporter}


class OnnxExporterManager:
    EXPORTERS = {
        "YOLO10-DET": YoloDetE2EExporter,
        "YOLO11-DET": YoloDetNonE2EExporter,
        "YOLO11-SEG": YoloSegNonE2EExporter,
        "YOLO26-DET": YoloDetE2EExporter,
        "YOLO26-SEG": YoloSegE2EExporter,
        "STGCNPP": StgcnppExporter,
    }

    @classmethod
    def types(cls) -> list[str]:
        names = list(cls.EXPORTERS)
        return names

    def export(
        self,
        type: str,
        weights: Path,
        size: int,
        opset: int,
        batch: int | None,
        dynamic: bool,
        simplify: bool,
        max_det: int,
        conf: float,
        output_dir: Path,
        iou: float | None = None,
    ) -> None:
        exporter_cls = self.EXPORTERS[type]
        kwargs = {
            "weights": weights,
            "size": size,
            "opset": opset,
            "batch": batch,
            "dynamic": dynamic,
            "simplify": simplify,
            "max_det": max_det,
            "conf": conf,
            "output_dir": output_dir,
        }
        if exporter_cls in IOU_EXPORTERS:
            kwargs["iou"] = iou
        exporter_cls().export(**kwargs)
