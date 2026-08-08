import shutil
from pathlib import Path

from ultralytics import YOLO

from utils.yolo_e2e.common import fix_batch_only_dynamic, run_export_cli, write_labels


class YoloDetE2EExporter:
    """Ultralytics e2e ONNX detect → images/output0."""

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
        if dynamic:
            export_kwargs["dynamic"] = True
        else:
            export_kwargs["batch"] = batch

        model.export(**export_kwargs)

        produced = weights.with_suffix(".onnx")
        onnx_path = output_dir / f"{weights.stem}.onnx"
        if produced.resolve() != onnx_path.resolve():
            shutil.move(produced, onnx_path)
        produced_data = weights.parent / f"{weights.stem}.onnx.data"
        if produced_data.is_file():
            shutil.move(produced_data, output_dir / produced_data.name)
        if dynamic:
            fix_batch_only_dynamic(onnx_path, size, max_det)


if __name__ == "__main__":
    run_export_cli(
        YoloDetE2EExporter(),
        "Export e2e ONNX detect (images/output0)",
    )
