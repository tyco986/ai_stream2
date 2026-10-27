from pathlib import Path
from typing import Literal


class RtmposeExporter:
    def export(
        self,
        weights: Path,
        size: int,
        opset: int,
        batch_size: int | None,
        batch_mode: Literal["dynamic", "static"],
        simplify: bool,
        max_det: int,
        conf: float,
        output_dir: Path,
    ) -> None:
        if weights.suffix.lower() != ".onnx" or not weights.is_file():
            raise ValueError(f"invalid onnx file: {weights}")
