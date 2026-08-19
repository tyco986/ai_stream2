import json
import shutil
import subprocess
from datetime import datetime
from pathlib import Path

import onnx

from utils.stgcnpp.utils.constants import (
    DEFAULT_PRECISION,
    LABELS_NAME,
    META_JSON_NAME,
    ONNX_PRECISION,
    PRECISION_FLAGS,
    STGCNPP_INPUT_NAME,
    STGCNPP_INPUT_TAIL,
    STGCNPP_NUM_CLASSES,
    STGCNPP_OUTPUT_NAME,
    STGCNPP_TASK,
    STGCNPP_VERSION,
    TRTEXEC,
    WORKSPACE_MIB,
)


class StgcnppExporter:
    default_precision = DEFAULT_PRECISION

    def parse_tensor(self, value_info) -> dict:
        dims = [
            -1 if dim.dim_param or not dim.dim_value else int(dim.dim_value)
            for dim in value_info.type.tensor_type.shape.dim
        ]
        return {
            "name": value_info.name,
            "precision": ONNX_PRECISION.get(
                int(value_info.type.tensor_type.elem_type),
                f"dtype_{value_info.type.tensor_type.elem_type}",
            ),
            "dims": dims,
        }

    def resolve_shape(self, tensor: dict, batch_size: int | None) -> list[int | None]:
        shape = [dim if dim > 0 else None for dim in tensor["dims"]]
        if tensor["dims"] and tensor["dims"][0] < 0:
            shape[0] = batch_size
        return shape

    def parse_model(self, model) -> tuple[list[dict], list[dict]]:
        graph = model.graph
        initializer_names = {item.name for item in graph.initializer}
        inputs = [
            self.parse_tensor(item)
            for item in graph.input
            if item.name not in initializer_names
        ]
        outputs = [self.parse_tensor(item) for item in graph.output]
        return inputs, outputs

    def validate_graph(self, inputs: list[dict], outputs: list[dict]) -> None:
        if len(inputs) != 1:
            raise ValueError(f"stgcnpp expected one graph input, got {len(inputs)}")
        input_t = inputs[0]
        if input_t["name"] != STGCNPP_INPUT_NAME:
            raise ValueError(
                f"stgcnpp expected input {STGCNPP_INPUT_NAME!r}, got {input_t['name']!r}"
            )
        if len(input_t["dims"]) != 5:
            raise ValueError(
                f"stgcnpp expected rank-5 input (N,M,T,V,C), got {input_t['dims']}"
            )
        tail = tuple(input_t["dims"][1:])
        if tail != STGCNPP_INPUT_TAIL:
            raise ValueError(
                f"stgcnpp expected input tail {STGCNPP_INPUT_TAIL}, got {tail}"
            )
        if len(outputs) != 1:
            raise ValueError(f"stgcnpp expected one graph output, got {len(outputs)}")
        output_t = outputs[0]
        if output_t["name"] != STGCNPP_OUTPUT_NAME:
            raise ValueError(
                f"stgcnpp expected output {STGCNPP_OUTPUT_NAME!r}, got {output_t['name']!r}"
            )
        if len(output_t["dims"]) != 2:
            raise ValueError(
                f"stgcnpp expected rank-2 output (N,C), got {output_t['dims']}"
            )
        num_classes = output_t["dims"][1]
        if num_classes > 0 and num_classes != STGCNPP_NUM_CLASSES:
            raise ValueError(
                f"stgcnpp expected C={STGCNPP_NUM_CLASSES}, got {num_classes}"
            )

    def find_onnx(self, folder: Path) -> Path:
        onnx_paths = [
            path for path in folder.iterdir() if path.is_file() and path.suffix == ".onnx"
        ]
        if len(onnx_paths) != 1:
            raise ValueError("folder must contain exactly one .onnx file")
        return onnx_paths[0]

    def load_labels(self, labels_path: Path) -> list[str]:
        if not labels_path.is_file():
            raise ValueError(f"missing {LABELS_NAME}: {labels_path}")
        labels = [
            line.strip()
            for line in labels_path.read_text(encoding="utf-8").splitlines()
            if line.strip()
        ]
        if len(labels) != STGCNPP_NUM_CLASSES:
            raise ValueError(
                f"stgcnpp expected {STGCNPP_NUM_CLASSES} labels, got {len(labels)}"
            )
        return labels

    def build_meta(self, inputs: list[dict], outputs: list[dict], classes: list[str]) -> dict:
        input_t = inputs[0]
        is_dynamic = any(dim < 0 for dim in input_t["dims"]) or any(
            any(dim < 0 for dim in tensor["dims"]) for tensor in outputs
        )
        batch_size = None if is_dynamic else input_t["dims"][0]
        output_t = outputs[0]
        if output_t["dims"][1] <= 0:
            output_t["dims"][1] = STGCNPP_NUM_CLASSES
        return {
            "batch_mode": "dynamic" if is_dynamic else "static",
            "input_tensor_name": input_t["name"],
            "output_tensor_name": output_t["name"],
            "output_tensor_names": [tensor["name"] for tensor in outputs],
            "classes": classes,
            "input_tensor_shape": self.resolve_shape(input_t, batch_size),
            "output_tensor_shape": self.resolve_shape(output_t, batch_size),
            "output_tensor_shapes": [
                self.resolve_shape(tensor, batch_size) for tensor in outputs
            ],
            "batch_size": batch_size,
            "precision": input_t["precision"],
            "version": STGCNPP_VERSION,
            "task": STGCNPP_TASK,
        }

    def resolve_batch(self, meta: dict, batch_size: int | None) -> int:
        input_batch = meta["input_tensor_shape"][0]
        resolved = batch_size
        if input_batch is not None:
            if batch_size is not None:
                raise ValueError("batch_size must be None for static input batch")
            resolved = int(input_batch)
        elif batch_size is None:
            raise ValueError("batch_size is required for dynamic input batch")
        return resolved

    def format_input_shape(self, meta: dict, batch: int) -> str:
        shape = meta["input_tensor_shape"]
        if any(dim is None for dim in shape[1:]):
            raise ValueError(f"unsupported dynamic dim in shape {shape}")
        dims = [batch, *[int(dim) for dim in shape[1:]]]
        return f"{meta['input_tensor_name']}:{'x'.join(str(dim) for dim in dims)}"

    def build_trtexec_command(
        self,
        onnx_path: Path,
        engine_path: Path,
        meta: dict,
        batch_size: int,
        gpu_id: int,
        precision: str,
        opt_level: int | None,
    ) -> list[str]:
        command = [
            str(TRTEXEC),
            f"--onnx={onnx_path}",
            f"--saveEngine={engine_path}",
            f"--device={gpu_id}",
            f"--memPoolSize=workspace:{WORKSPACE_MIB}M",
        ]
        if opt_level is not None:
            command.append(f"--builderOptimizationLevel={opt_level}")
        flag = PRECISION_FLAGS.get(precision)
        if flag:
            command.append(flag)
        if meta["input_tensor_shape"][0] is None:
            opt_shape = self.format_input_shape(meta, batch_size)
            command.extend(
                [
                    f"--minShapes={self.format_input_shape(meta, 1)}",
                    f"--optShapes={opt_shape}",
                    f"--maxShapes={opt_shape}",
                ]
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
        if not onnx_dir.is_dir():
            raise ValueError(f"input not found or not a directory: {onnx_dir}")
        onnx_path = self.find_onnx(onnx_dir)
        labels_path = onnx_dir / LABELS_NAME
        classes = self.load_labels(labels_path)
        model = onnx.load(str(onnx_path), load_external_data=False)
        inputs, outputs = self.parse_model(model)
        self.validate_graph(inputs, outputs)
        meta = self.build_meta(inputs, outputs, classes)
        resolved_batch = self.resolve_batch(meta, batch_size)

        engine_path = output_dir / f"{onnx_path.stem}.engine"
        command = self.build_trtexec_command(
            onnx_path,
            engine_path,
            meta,
            resolved_batch,
            gpu_id,
            precision,
            opt_level,
        )
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode != 0:
            raise ValueError((result.stderr or result.stdout or "trtexec failed").strip())
        if not engine_path.is_file():
            raise ValueError(f"trtexec did not produce engine: {engine_path}")

        meta["batch_size"] = resolved_batch
        meta["input_tensor_shape"] = [resolved_batch, *meta["input_tensor_shape"][1:]]
        meta["output_tensor_shape"] = [resolved_batch, *meta["output_tensor_shape"][1:]]
        meta["output_tensor_shapes"] = [
            [resolved_batch, *shape[1:]] for shape in meta["output_tensor_shapes"]
        ]
        meta.update(
            {
                "precision": precision,
                "gpu_id": gpu_id,
                "opt_level": opt_level,
                "build_time": datetime.now().astimezone().isoformat(),
            }
        )
        shutil.copy2(labels_path, output_dir / LABELS_NAME)
        (output_dir / META_JSON_NAME).write_text(
            json.dumps(meta, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8",
        )
