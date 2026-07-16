import json
import logging
import re
import shutil
import subprocess
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path

TRTEXEC = Path("/usr/bin/trtexec")
LIBS_ROOT = Path("/app/libs")
WORKSPACE_MIB = 8192
DEFAULT_MODEL_ROOT = Path("/root/models")
DEFAULT_PRECISION = "fp16"
AVAILABLE_PRECISION = frozenset({"fp32", "fp16", "int8"})
PRECISION_FLAGS = {"fp16": "--fp16", "int8": "--int8"}
META_JSON_NAME = "meta.json"
LABELS_NAME = "labels.txt"
YOLO_PLUGIN_SUFFIX = {"detect": "", "pose": "_pose", "segment": "_seg"}


@dataclass(frozen=True)
class OnnxBundle:
    folder: Path
    onnx_path: Path
    labels_path: Path
    meta: dict

    @classmethod
    def load(cls, folder: Path) -> "OnnxBundle":
        if not folder.is_dir():
            raise ValueError(f"input not a directory: {folder}")

        onnx_paths = [
            path
            for path in folder.iterdir()
            if path.is_file() and path.suffix == ".onnx" and not path.name.endswith(".onnx.data")
        ]
        if len(onnx_paths) != 1:
            raise ValueError("folder must contain exactly one .onnx file")

        labels_path = folder / LABELS_NAME
        meta_path = folder / META_JSON_NAME
        if not labels_path.is_file():
            raise ValueError(f"missing {LABELS_NAME}: {labels_path}")
        if not meta_path.is_file():
            raise ValueError(f"missing {META_JSON_NAME}: {meta_path}")

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
        if not shape or shape[0] is None:
            return None
        return int(shape[0])

    def resolve_batch(self, batch_size: int | None) -> int:
        if self.input_batch is not None:
            if batch_size is not None:
                raise ValueError("batch_size must be None for static input batch")
            return self.input_batch
        if batch_size is None:
            raise ValueError("batch_size is required for dynamic input batch")
        return batch_size

    @property
    def task(self) -> str:
        return self.meta["task"]

    def uses_static_plugins(self) -> bool:
        return self.task == "segment"

    def yolo_plugin_path(self) -> Path:
        suffix = YOLO_PLUGIN_SUFFIX.get(self.task)
        if suffix is None:
            raise ValueError(f"unsupported task {self.task!r}")
        plugin_path = LIBS_ROOT / f"libnvdsinfer_custom_impl_Yolo{suffix}.so"
        if not plugin_path.is_file():
            raise ValueError(f"missing static plugin: {plugin_path}")
        return plugin_path

    def format_input_shape(self, batch: int) -> str:
        name = self.meta["input_tensor_name"]
        shape = self.meta["input_tensor_shape"]
        dims = [batch]
        for index, dim in enumerate(shape[1:], start=1):
            if dim is None:
                raise ValueError(f"unsupported dynamic dim at index {index} in shape {shape}")
            dims.append(int(dim))
        return f"{name}:{'x'.join(str(dim) for dim in dims)}"

    def build_trtexec_command(
        self,
        engine_path: Path,
        batch_size: int,
        gpu_id: int,
        precision: str,
    ) -> list[str]:
        command = [
            str(TRTEXEC),
            f"--onnx={self.onnx_path}",
            f"--saveEngine={engine_path}",
            f"--device={gpu_id}",
            f"--memPoolSize=workspace:{WORKSPACE_MIB}M",
        ]
        if self.uses_static_plugins():
            command.append(f"--staticPlugins={self.yolo_plugin_path()}")
        if flag := PRECISION_FLAGS.get(precision):
            command.append(flag)
        if self.input_batch is None:
            command.extend(
                [
                    f"--minShapes={self.format_input_shape(1)}",
                    f"--optShapes={self.format_input_shape(batch_size)}",
                    f"--maxShapes={self.format_input_shape(batch_size)}",
                ]
            )
        return command

    def build_output_meta(
        self,
        batch_size: int,
        gpu_id: int,
        precision: str,
        build_time: str,
    ) -> dict:
        meta = dict(self.meta)
        meta["batch_size"] = batch_size
        meta["input_tensor_shape"] = [batch_size, *self.meta["input_tensor_shape"][1:]]
        meta["output_tensor_shape"] = [batch_size, *self.meta["output_tensor_shape"][1:]]
        meta.update(
            {
                "precision": precision,
                "gpu_id": gpu_id,
                "cuda_version": probe_cuda_version(),
                "tensorrt_version": probe_version(
                    [str(TRTEXEC), "--version"], r"TensorRT v([\d.]+|\w+)"
                ),
                "build_time": build_time,
            }
        )
        return meta


def validate_precision(precision: str) -> str:
    assert precision in AVAILABLE_PRECISION
    return precision


def probe_version(command: list[str], pattern: str) -> str:
    result = subprocess.run(command, capture_output=True, text=True, check=False)
    match = re.search(pattern, result.stdout + result.stderr)
    return match.group(1) if match else "unknown"


def probe_cuda_version() -> str:
    version_path = Path("/usr/local/cuda/version.json")
    if version_path.is_file():
        version = json.loads(version_path.read_text(encoding="utf-8")).get("cuda", {}).get("version")
        if version:
            return str(version)
    nvcc = shutil.which("nvcc")
    if nvcc:
        return probe_version([nvcc, "--version"], r"release\s+([\d.]+)")
    return "unknown"


class ExportEngineRunner:
    def __init__(self, logger: logging.Logger, model_root: Path = DEFAULT_MODEL_ROOT) -> None:
        self.logger = logger
        self.model_root = model_root
        self.trt_root = model_root / "trt"

    def resolve_input(self, input_path: str) -> Path:
        path = Path(input_path).expanduser()
        if not path.is_absolute():
            path = self.model_root / path
        path = path.resolve()
        if not path.is_dir():
            raise ValueError(f"input not found or not a directory: {path}")
        return path

    def run_trtexec(self, bundle: OnnxBundle, engine_path: Path, batch_size: int, gpu_id: int, precision: str) -> str:
        command = bundle.build_trtexec_command(engine_path, batch_size, gpu_id, precision)
        engine_path.parent.mkdir(parents=True, exist_ok=True)
        self.logger.info("trtexec start cmd=%s", command)
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode != 0:
            raise RuntimeError((result.stderr or result.stdout or "trtexec failed").strip())
        if not engine_path.is_file():
            raise RuntimeError(f"trtexec did not produce engine: {engine_path}")

        build_time = datetime.now(timezone.utc).isoformat()
        self.logger.info("trtexec done engine=%s build_time=%s", engine_path, build_time)
        return build_time

    def run(
        self,
        input_path: str,
        batch_size: int | None,
        gpu_id: int,
        precision: str = DEFAULT_PRECISION,
    ) -> Path:
        precision = validate_precision(precision)
        bundle = OnnxBundle.load(self.resolve_input(input_path))
        resolved_batch = bundle.resolve_batch(batch_size)

        bundle_dir = self.trt_root / bundle.folder.name
        if bundle_dir.exists():
            shutil.rmtree(bundle_dir)
        bundle_dir.mkdir(parents=True)

        shutil.copy2(bundle.labels_path, bundle_dir / LABELS_NAME)
        engine_path = bundle_dir / f"{bundle.stem}.engine"
        build_time = self.run_trtexec(bundle, engine_path, resolved_batch, gpu_id, precision)
        (bundle_dir / META_JSON_NAME).write_text(
            json.dumps(
                bundle.build_output_meta(resolved_batch, gpu_id, precision, build_time),
                ensure_ascii=False,
                indent=2,
            )
            + "\n",
            encoding="utf-8",
        )

        self.logger.info("export done stem=%s bundle=%s", bundle.stem, bundle_dir)
        return bundle_dir
