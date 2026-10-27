import shutil
from pathlib import Path
from typing import Literal

from ultralytics import YOLO

from utils.yolo_e2e.common import (
    fix_pose_batch_only_dynamic,
    pose_output_channels,
    validate_export_args,
    write_labels,
)


class YoloPoseE2EExporter:
    def export(
        self,
        weights: Path,
        size: int,
        opset: int,
        batch_size: int,
        batch_mode: Literal["dynamic", "static"],
        simplify: bool,
        max_det: int,
        conf: float,
        iou: float,
        output_dir: Path,
    ) -> None:
        validate_export_args(weights, batch_mode, batch_size)

        model = YOLO(str(weights))
        write_labels(model.names, output_dir / "labels.txt")

        export_kwargs = {
            "format": "onnx",
            "imgsz": size,
            "opset": opset,
            "simplify": simplify,
            "max_det": max_det,
            "conf": conf,
        }
        export_kwargs["batch"] = batch_size
        if batch_mode == "dynamic":
            export_kwargs["dynamic"] = True

        model.export(**export_kwargs)

        produced = weights.with_suffix(".onnx")
        onnx_path = output_dir / f"{weights.stem}.onnx"
        if produced.resolve() != onnx_path.resolve():
            shutil.move(produced, onnx_path)
        produced_data = weights.parent / f"{weights.stem}.onnx.data"
        if produced_data.is_file():
            shutil.move(produced_data, output_dir / produced_data.name)
        kpt_shape = getattr(model.model, "kpt_shape", None)
        num_keypoints = int(kpt_shape[0]) if kpt_shape else 17
        channels = pose_output_channels(num_keypoints)
        if batch_mode == "dynamic":
            fix_pose_batch_only_dynamic(onnx_path, size, max_det, channels)
