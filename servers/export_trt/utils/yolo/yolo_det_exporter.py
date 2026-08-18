import json
import shutil
import subprocess
from datetime import datetime
from pathlib import Path

from utils.yolo.utils.constants import DEFAULT_PRECISION, LABELS_NAME, META_JSON_NAME
from utils.yolo.utils.onnx_bundle import OnnxBundle


class YoloDetExporter:
    default_precision = DEFAULT_PRECISION

    def build_trtexec_command(
        self,
        bundle: OnnxBundle,
        engine_path: Path,
        batch_size: int,
        gpu_id: int,
        precision: str,
        opt_level: int | None = None,
    ) -> list[str]:
        command = bundle.build_trtexec_command(
            engine_path, batch_size, gpu_id, precision, opt_level
        )
        return command

    def export(
        self,
        onnx_dir: Path,
        output_dir: Path,
        batch_size: int | None,
        gpu_id: int,
        precision: str,
        opt_level: int | None,
    ) -> None:
        if precision not in {"fp32", "fp16", "int8"}:
            raise ValueError(f"unsupported precision: {precision}")
        bundle = OnnxBundle.load(onnx_dir)
        resolved_batch = bundle.resolve_batch(batch_size)

        shutil.copy2(bundle.labels_path, output_dir / LABELS_NAME)
        engine_path = output_dir / f"{bundle.stem}.engine"
        command = self.build_trtexec_command(
            bundle, engine_path, resolved_batch, gpu_id, precision, opt_level
        )
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode != 0:
            raise ValueError((result.stderr or result.stdout or "trtexec failed").strip())
        if not engine_path.is_file():
            raise ValueError(f"trtexec did not produce engine: {engine_path}")

        meta = dict(bundle.meta)
        meta["batch_size"] = resolved_batch
        meta["input_tensor_shape"] = [resolved_batch, *bundle.meta["input_tensor_shape"][1:]]
        meta["output_tensor_shape"] = [
            resolved_batch,
            *bundle.meta["output_tensor_shape"][1:],
        ]
        if "output_tensor_shapes" in bundle.meta:
            meta["output_tensor_shapes"] = [
                [resolved_batch, *shape[1:]]
                for shape in bundle.meta["output_tensor_shapes"]
            ]
        meta.update(
            {
                "precision": precision,
                "gpu_id": gpu_id,
                "opt_level": opt_level,
                "build_time": datetime.now().astimezone().isoformat(),
            }
        )
        (output_dir / META_JSON_NAME).write_text(
            json.dumps(meta, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8",
        )
