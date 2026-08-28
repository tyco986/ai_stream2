import shutil
from pathlib import Path

from ultralytics import YOLO

from utils.yolo_e2e.common import (
    fix_pose_batch_only_dynamic,
    pose_output_channels,
    validate_export_args,
    write_bundle_meta,
    write_labels,
)


class YoloPoseE2EExporter:
    def export(
        self,
        weights: Path,
        size: int,
        opset: int,
        batch: int,
        dynamic: bool,
        simplify: bool,
        max_det: int,
        conf: float,
        output_dir: Path,
    ) -> None:
        validate_export_args(weights, dynamic, batch)

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
        export_kwargs["batch"] = batch
        if dynamic:
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
        if dynamic:
            fix_pose_batch_only_dynamic(onnx_path, size, max_det, channels)
        write_bundle_meta(
            output_dir,
            onnx_path,
            output_dir / "labels.txt",
            version="yolo26",
            task="pose",
            yolo_export="e2e",
            max_det=max_det,
            conf=conf,
            dynamic=dynamic,
            batch=batch,
        )
