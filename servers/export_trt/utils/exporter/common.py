import argparse
import json
import re
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path

from utils.api.constants import (
    AVAILABLE_PRECISION,
    DEFAULT_PRECISION,
    LABELS_NAME,
    LIBS_ROOT,
    META_JSON_NAME,
    PRECISION_FLAGS,
    TRTEXEC,
    WORKSPACE_MIB,
    YOLO_PLUGIN_SUFFIX,
)


class AppError(Exception):
    def __init__(self, message: str, status_code: int = 400) -> None:
        self.message = message
        self.status_code = status_code
        super().__init__(message)


@dataclass(frozen=True)
class ExportEngineResult:
    bundle_dir: Path
    command: list[str]


@dataclass(frozen=True)
class OnnxBundle:
    folder: Path
    onnx_path: Path
    labels_path: Path
    meta: dict

    @classmethod
    def load(cls, folder: Path) -> "OnnxBundle":
        if not folder.is_dir():
            raise AppError(f"input not a directory: {folder}")

        onnx_paths = [
            path for path in folder.iterdir() if path.is_file() and path.suffix == ".onnx"
        ]
        if len(onnx_paths) != 1:
            raise AppError("folder must contain exactly one .onnx file")

        labels_path = folder / LABELS_NAME
        meta_path = folder / META_JSON_NAME
        if not labels_path.is_file():
            raise AppError(f"missing {LABELS_NAME}: {labels_path}")
        if not meta_path.is_file():
            raise AppError(f"missing {META_JSON_NAME}: {meta_path}")

        return cls(
            folder=folder,
            onnx_path=onnx_paths[0],
            labels_path=labels_path,
            meta=json.loads(meta_path.read_text(encoding="utf-8")),
        )

    @property
    def stem(self) -> str:
        return self.onnx_path.stem

    @property
    def input_batch(self) -> int | None:
        shape = self.meta["input_tensor_shape"]
        batch = None
        if shape and shape[0] is not None:
            batch = int(shape[0])
        return batch

    def resolve_batch(self, batch_size: int | None) -> int:
        resolved = batch_size
        if self.input_batch is not None:
            if batch_size is not None:
                raise AppError("batch_size must be None for static input batch")
            resolved = self.input_batch
        elif batch_size is None:
            raise AppError("batch_size is required for dynamic input batch")
        return resolved

    @property
    def task(self) -> str:
        return self.meta["task"]

    def yolo_plugin_path(self) -> Path:
        suffix = YOLO_PLUGIN_SUFFIX.get(self.task)
        if suffix is None:
            raise AppError(f"unsupported task {self.task!r}")
        plugin_path = LIBS_ROOT / f"libnvdsinfer_custom_impl_Yolo{suffix}.so"
        if not plugin_path.is_file():
            raise AppError(f"missing static plugin: {plugin_path}")
        return plugin_path

    def format_input_shape(self, batch: int) -> str:
        name = self.meta["input_tensor_name"]
        shape = self.meta["input_tensor_shape"]
        if any(dim is None for dim in shape[1:]):
            raise AppError(f"unsupported dynamic dim in shape {shape}")
        dims = [batch, *[int(dim) for dim in shape[1:]]]
        return f"{name}:{'x'.join(str(dim) for dim in dims)}"

    def build_trtexec_command(
        self,
        engine_path: Path,
        batch_size: int,
        gpu_id: int,
        precision: str,
        opt_level: int | None = None,
    ) -> list[str]:
        command = [
            str(TRTEXEC),
            f"--onnx={self.onnx_path}",
            f"--saveEngine={engine_path}",
            f"--device={gpu_id}",
            f"--memPoolSize=workspace:{WORKSPACE_MIB}M",
        ]
        if self.task == "segment":
            command.append(f"--staticPlugins={self.yolo_plugin_path()}")
        if opt_level is not None:
            command.append(f"--builderOptimizationLevel={opt_level}")
        if flag := PRECISION_FLAGS.get(precision):
            command.append(flag)
        if self.input_batch is None:
            shape = self.format_input_shape(batch_size)
            command.extend(
                [
                    f"--minShapes={self.format_input_shape(1)}",
                    f"--optShapes={shape}",
                    f"--maxShapes={shape}",
                ]
            )
        return command

    def build_output_meta(
        self,
        batch_size: int,
        gpu_id: int,
        precision: str,
        build_time: str,
        opt_level: int | None = None,
    ) -> dict:
        meta = dict(self.meta)
        meta["batch_size"] = batch_size
        meta["input_tensor_shape"] = [batch_size, *self.meta["input_tensor_shape"][1:]]
        meta["output_tensor_shape"] = [batch_size, *self.meta["output_tensor_shape"][1:]]
        meta.update(
            {
                "precision": precision,
                "gpu_id": gpu_id,
                "opt_level": opt_level,
                "cuda_version": probe_cuda_version(),
                "tensorrt_version": probe_version(
                    [str(TRTEXEC), "--version"], r"TensorRT v([\d.]+|\w+)"
                ),
                "build_time": build_time,
            }
        )
        return meta


def validate_precision(precision: str) -> str:
    if precision not in AVAILABLE_PRECISION:
        raise AppError(f"unsupported precision: {precision}")
    return precision


def probe_version(command: list[str], pattern: str) -> str:
    result = subprocess.run(command, capture_output=True, text=True, check=False)
    match = re.search(pattern, result.stdout + result.stderr)
    version = "unknown"
    if match:
        version = match.group(1)
    return version


def probe_cuda_version() -> str:
    version = "unknown"
    version_path = Path("/usr/local/cuda/version.json")
    if version_path.is_file():
        cuda_version = (
            json.loads(version_path.read_text(encoding="utf-8")).get("cuda", {}).get("version")
        )
        if cuda_version:
            version = str(cuda_version)
    if version == "unknown":
        nvcc = shutil.which("nvcc")
        if nvcc:
            version = probe_version([nvcc, "--version"], r"release\s+([\d.]+)")
    return version


def run_export_cli(exporter, description: str) -> None:
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument("-i", "--input", required=True, help="ONNX bundle directory")
    parser.add_argument(
        "--batch-size",
        type=int,
        default=None,
        help="Batch size for dynamic ONNX input",
    )
    parser.add_argument("--gpu-id", type=int, default=0, help="GPU device id")
    parser.add_argument(
        "--precision",
        default=DEFAULT_PRECISION,
        help=f"Engine precision ({', '.join(sorted(AVAILABLE_PRECISION))})",
    )
    parser.add_argument(
        "--opt-level",
        type=int,
        default=None,
        help="trtexec builderOptimizationLevel",
    )
    args = parser.parse_args()
    exporter.export_engine(
        input_path=args.input,
        batch_size=args.batch_size,
        gpu_id=args.gpu_id,
        precision=args.precision,
        opt_level=args.opt_level,
    )
