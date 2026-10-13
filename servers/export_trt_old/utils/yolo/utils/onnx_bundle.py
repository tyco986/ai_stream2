import json
from dataclasses import dataclass
from pathlib import Path

from utils.yolo.utils.constants import (
    LABELS_NAME,
    META_JSON_NAME,
    PRECISION_FLAGS,
    TRTEXEC,
    WORKSPACE_MIB,
)


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
            path for path in folder.iterdir() if path.is_file() and path.suffix == ".onnx"
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
        batch = None
        if shape and shape[0] is not None:
            batch = int(shape[0])
        return batch

    def resolve_batch(self, batch_size: int | None) -> int:
        resolved = batch_size
        if self.input_batch is not None:
            if batch_size is not None:
                raise ValueError("batch_size must be None for static input batch")
            resolved = self.input_batch
        elif batch_size is None:
            raise ValueError("batch_size is required for dynamic input batch")
        return resolved

    @property
    def task(self) -> str:
        return self.meta["task"]

    def format_input_shape(self, batch: int) -> str:
        name = self.meta["input_tensor_name"]
        shape = self.meta["input_tensor_shape"]
        if any(dim is None for dim in shape[1:]):
            raise ValueError(f"unsupported dynamic dim in shape {shape}")
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
        if opt_level is not None:
            command.append(f"--builderOptimizationLevel={opt_level}")
        flag = PRECISION_FLAGS.get(precision)
        if flag:
            command.append(flag)
        if self.input_batch is None:
            opt_shape = self.format_input_shape(batch_size)
            command.extend(
                [
                    f"--minShapes={self.format_input_shape(1)}",
                    f"--optShapes={opt_shape}",
                    f"--maxShapes={opt_shape}",
                ]
            )
        return command
