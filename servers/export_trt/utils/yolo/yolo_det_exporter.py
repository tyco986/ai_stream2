import json
import shutil
import subprocess
from datetime import datetime
from pathlib import Path

from utils.yolo.utils.calib import Int8CalibZip
from utils.yolo.utils.constants import (
    CALIB_BATCH,
    CALIB_COUNT,
    DEFAULT_PRECISION,
    INT8_CALIB_ZIP,
    LABELS_NAME,
    META_JSON_NAME,
)
from utils.yolo.utils.onnx_bundle import OnnxBundle


class YoloDetExporter:
    default_precision = DEFAULT_PRECISION

    def plugin_path(self) -> Path | None:
        return None

    def build_trtexec_command(
        self,
        bundle: OnnxBundle,
        engine_path: Path,
        batch_size: int,
        gpu_id: int,
        precision: str,
        opt_level: int | None = None,
    ) -> list[str]:
        if precision == "int8":
            raise ValueError("int8 requires MinMax calibration")
        command = bundle.build_trtexec_command(
            engine_path, batch_size, gpu_id, precision, opt_level
        )
        return command

    def resolve_int8(
        self,
        bundle: OnnxBundle,
        output_dir: Path,
        resolved_batch: int,
    ) -> tuple[Path, list[Path], int]:
        output_cache = output_dir / f"{bundle.stem}.cache"
        if output_cache.is_file():
            output_cache.unlink()
        extract_dir = output_dir / "_int8_calib"
        image_paths = Int8CalibZip(INT8_CALIB_ZIP, CALIB_COUNT).extract_paths(
            extract_dir
        )
        used_batch = resolved_batch
        return output_cache, image_paths, used_batch

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
        cache_path = None
        image_paths = []
        used_calib_batch = min(CALIB_BATCH, resolved_batch)
        if precision == "int8":
            assert resolved_batch >= CALIB_BATCH, (
                f"int8 batch_size must be >= {CALIB_BATCH}, got {resolved_batch}"
            )
            shape = bundle.meta["input_tensor_shape"]
            if any(dim is None for dim in shape[1:]):
                raise ValueError(f"unsupported dynamic dim in shape {shape}")
            cache_path, image_paths, used_calib_batch = self.resolve_int8(
                bundle, output_dir, resolved_batch
            )

        shutil.copy2(bundle.labels_path, output_dir / LABELS_NAME)
        engine_path = output_dir / f"{bundle.stem}.engine"
        if precision == "int8":
            # tensorrt Python 仅 INT8 需要，避免未装绑定时拖垮 fp16 导出
            from utils.yolo.utils.int8 import YoloInt8EngineBuilder

            YoloInt8EngineBuilder(
                bundle.onnx_path,
                engine_path,
                cache_path,
                image_paths,
                resolved_batch,
                used_calib_batch,
                gpu_id,
                opt_level,
                bundle.meta["input_tensor_name"],
                int(bundle.meta["input_tensor_shape"][1]),
                int(bundle.meta["input_tensor_shape"][2]),
                int(bundle.meta["input_tensor_shape"][3]),
                self.plugin_path(),
                bundle.input_batch is None,
            ).build()
            shutil.rmtree(output_dir / "_int8_calib", ignore_errors=True)
        else:
            command = self.build_trtexec_command(
                bundle, engine_path, resolved_batch, gpu_id, precision, opt_level
            )
            result = subprocess.run(command, capture_output=True, text=True)
            if result.returncode != 0:
                raise ValueError(
                    (result.stderr or result.stdout or "trtexec failed").strip()
                )

        if not engine_path.is_file():
            raise ValueError(f"export did not produce engine: {engine_path}")

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
        if precision == "int8":
            meta["calib_cache"] = str(cache_path)
            meta["calib_count"] = len(image_paths)
            meta["calib_batch"] = used_calib_batch
        (output_dir / META_JSON_NAME).write_text(
            json.dumps(meta, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8",
        )
