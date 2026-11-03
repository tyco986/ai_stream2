from pathlib import Path
from typing import Literal

from utils.api.constants import FILE_LABELS, RTMPOSE_LABELS


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
        if not weights.is_file():
            raise ValueError(f"invalid onnx file: {weights}")
        (output_dir / FILE_LABELS).write_text(RTMPOSE_LABELS, encoding="utf-8")
