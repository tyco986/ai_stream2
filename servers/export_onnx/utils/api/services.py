import hashlib
import json
import logging
import shutil
from pathlib import Path

import onnx
from fastapi import UploadFile
from ultralytics import YOLO

from utils.api.constants import (
    DEFAULT_CONF,
    DEFAULT_IOU,
    DEFAULT_MODEL_ROOT,
    EXPORT_SPECS,
    ExportSpec,
    LOGGER_NAME,
    ONNX_PRECISION,
)
from utils.api.schemas import ApiJsonResponse


class AppError(Exception):
    def __init__(self, message: str, status_code: int = 400) -> None:
        self.message = message
        self.status_code = status_code
        super().__init__(message)


class OnnxMetaBuilder:
    def parse_onnx_tensor(self, value_info) -> dict:
        tensor_type = value_info.type.tensor_type
        dims: list[int] = []
        dynamic: list[bool] = []
        for dim in tensor_type.shape.dim:
            if dim.dim_param or not dim.dim_value:
                dims.append(-1)
                dynamic.append(True)
                continue
            dims.append(int(dim.dim_value))
            dynamic.append(False)
        return {
            "name": value_info.name,
            "precision": ONNX_PRECISION.get(
                int(tensor_type.elem_type), f"dtype_{tensor_type.elem_type}"
            ),
            "dims": dims,
            "dynamic": dynamic,
        }

    def resolve_shape(self, tensor: dict, batch_size: int | None) -> list[int | None]:
        shape: list[int | None] = []
        for index, (dim, is_dynamic) in enumerate(
            zip(tensor["dims"], tensor["dynamic"], strict=True)
        ):
            if is_dynamic and index == 0:
                shape.append(batch_size)
                continue
            shape.append(dim if dim > 0 else None)
        return shape

    def resolve_batch_size(
        self, input_t: dict, export_batch: int, is_dynamic: bool
    ) -> int | None:
        batch_size = export_batch
        if is_dynamic:
            batch_size = None
        elif not input_t["dynamic"][0]:
            batch_size = input_t["dims"][0]
        return batch_size

    def validate_detect_meta(self, meta: dict, max_det: int) -> None:
        if meta["input_tensor_name"] != "images":
            raise AppError(
                f"detect export expected input tensor 'images', got {meta['input_tensor_name']!r}"
            )
        if meta["output_tensor_name"] != "output0":
            raise AppError(
                f"detect export expected output tensor 'output0', got {meta['output_tensor_name']!r}"
            )
        output_shape = meta["output_tensor_shape"]
        if len(output_shape) < 2 or output_shape[1] != max_det:
            raise AppError(
                f"detect export expected output shape [batch, {max_det}, 6], got {output_shape}"
            )

    def validate_non_e2e_seg_meta(self, meta: dict) -> None:
        if meta["input_tensor_name"] != "images":
            raise AppError(
                f"non-e2e seg export expected input tensor 'images', got {meta['input_tensor_name']!r}"
            )
        output_names = meta["output_tensor_names"]
        expected = ["num_dets", "det_boxes", "det_scores", "det_classes", "det_masks"]
        if output_names != expected:
            raise AppError(
                f"non-e2e seg export expected outputs {expected}, got {output_names}"
            )

    def build(
        self,
        spec: ExportSpec,
        onnx_path: Path,
        labels_path: Path,
        export_batch: int,
        max_det: int,
        conf: float,
        iou: float | None,
    ) -> dict:
        graph = onnx.load(str(onnx_path)).graph
        if not graph.input or not graph.output:
            raise AppError("onnx missing input or output")

        input_t = self.parse_onnx_tensor(graph.input[0])
        output_tensors = [self.parse_onnx_tensor(value) for value in graph.output]
        output_t = output_tensors[0]
        is_dynamic = any(input_t["dynamic"]) or any(
            any(tensor["dynamic"]) for tensor in output_tensors
        )
        batch_size = self.resolve_batch_size(input_t, export_batch, is_dynamic)

        classes = [
            line.strip()
            for line in labels_path.read_text(encoding="utf-8").splitlines()
            if line.strip()
        ]
        meta = {
            "batch_mode": "dynamic" if is_dynamic else "static",
            "input_tensor_name": input_t["name"],
            "output_tensor_name": output_t["name"],
            "classes": classes,
            "input_tensor_shape": self.resolve_shape(input_t, batch_size),
            "output_tensor_shape": self.resolve_shape(output_t, batch_size),
            "batch_size": batch_size,
            "precision": input_t["precision"],
            "version": spec.family,
            "task": spec.task,
            "yolo_export": spec.yolo_export,
            "max_det": max_det,
            "conf": conf,
        }
        if iou is not None:
            meta["iou"] = iou
        if spec.task == "segment":
            meta["output_tensor_names"] = [tensor["name"] for tensor in output_tensors]
            meta["output_tensor_shapes"] = [
                self.resolve_shape(tensor, batch_size) for tensor in output_tensors
            ]
        if spec.yolo_export in {"e2e", "non_e2e"} and spec.task == "detect":
            self.validate_detect_meta(meta, max_det)
        if spec.yolo_export in {"non_e2e_seg", "e2e_seg"}:
            self.validate_non_e2e_seg_meta(meta)
        return meta


class BundleFingerprintBuilder:
    def file_sha256(self, path: Path) -> str:
        digest = hashlib.sha256()
        with path.open("rb") as handle:
            chunk = handle.read(1024 * 1024)
            while chunk:
                digest.update(chunk)
                chunk = handle.read(1024 * 1024)
        return digest.hexdigest()

    def build(self, bundle_dir: Path, stem: str) -> dict:
        onnx_path = bundle_dir / f"{stem}.onnx"
        onnx_data_path = bundle_dir / f"{stem}.onnx.data"
        bundle_files = {onnx_path.name: self.file_sha256(onnx_path)}
        if onnx_data_path.is_file():
            bundle_files[onnx_data_path.name] = self.file_sha256(onnx_data_path)
        lines = [f"{name}:{bundle_files[name]}" for name in sorted(bundle_files)]
        return {
            "bundle_sha256": hashlib.sha256("\n".join(lines).encode("utf-8")).hexdigest(),
            "bundle_files": bundle_files,
        }


class PtValidator:
    def validate(self, pt_path: Path, family: str, task: str) -> None:
        model = YOLO(str(pt_path))
        if model.task != task:
            raise AppError(f"expected task {task!r}, got {model.task!r}")

        parts = [str(model.model.yaml)]
        ckpt = model.ckpt
        if isinstance(ckpt, dict):
            parts.extend([str(ckpt.get("model", "")), str(ckpt.get("train_args", ""))])
        blob = " ".join(parts).lower()

        if family == "yolo10" and "yolov10" not in blob and "v10detect" not in blob:
            raise AppError("weights are not YOLOv10")
        if family == "yolo26" and "yolo26" not in blob:
            raise AppError("weights are not YOLO26")
        if family == "yolo11" and "yolo11" not in blob:
            raise AppError("weights are not YOLO11")
        if family == "yolo11" and "yolo26" in blob:
            raise AppError("weights are YOLO26, not YOLO11")


class ExportRunner:
    def __init__(self, model_root: Path = DEFAULT_MODEL_ROOT) -> None:
        self.model_root = model_root
        self.logger = logging.getLogger(LOGGER_NAME)
        self.pt_root = model_root / "pt"
        self.onnx_root = model_root / "onnx"
        self.meta_builder = OnnxMetaBuilder()
        self.fingerprint_builder = BundleFingerprintBuilder()
        self.pt_validator = PtValidator()
        self.pt_root.mkdir(parents=True, exist_ok=True)
        self.onnx_root.mkdir(parents=True, exist_ok=True)

    def save_pt(self, upload: UploadFile) -> Path:
        filename = upload.filename or "model.pt"
        if Path(filename).suffix.lower() != ".pt":
            raise AppError(f"input must be a .pt file: {filename}")
        pt_path = self.pt_root / Path(filename).name
        with pt_path.open("wb") as handle:
            shutil.copyfileobj(upload.file, handle)
        return pt_path

    def prepare_bundle_dir(self, stem: str) -> Path:
        bundle_dir = self.onnx_root / stem
        if bundle_dir.exists():
            shutil.rmtree(bundle_dir)
        bundle_dir.mkdir(parents=True)
        return bundle_dir

    def run_exporter(
        self,
        spec: ExportSpec,
        pt_path: Path,
        bundle_dir: Path,
        size: int,
        opset: int,
        batch: int,
        dynamic: bool,
        simplify: bool,
        max_det: int,
        conf: float,
        iou: float,
    ) -> None:
        kwargs = {
            "weights": pt_path,
            "size": size,
            "opset": opset,
            "batch": batch,
            "dynamic": dynamic,
            "simplify": simplify,
            "max_det": max_det,
            "conf": conf,
            "output_dir": bundle_dir,
        }
        if spec.uses_iou:
            kwargs["iou"] = iou
        self.logger.info(
            "export start exporter=%s stem=%s",
            spec.exporter_cls.__name__,
            pt_path.stem,
        )
        spec.exporter_cls().export(**kwargs)
        onnx_path = bundle_dir / f"{pt_path.stem}.onnx"
        if not onnx_path.is_file():
            raise AppError(f"missing export artifact: {onnx_path}")

    def write_meta(
        self,
        spec: ExportSpec,
        bundle_dir: Path,
        stem: str,
        batch: int,
        max_det: int,
        conf: float,
        iou: float | None,
    ) -> None:
        labels_path = bundle_dir / "labels.txt"
        if not labels_path.is_file():
            raise AppError(f"missing export artifact: {labels_path}")

        meta = self.meta_builder.build(
            spec,
            bundle_dir / f"{stem}.onnx",
            labels_path,
            batch,
            max_det,
            conf,
            iou,
        )
        meta.update(self.fingerprint_builder.build(bundle_dir, stem))
        meta_path = bundle_dir / "meta.json"
        meta_path.write_text(
            json.dumps(meta, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8",
        )

    def run(
        self,
        spec: ExportSpec,
        upload: UploadFile,
        size: int,
        opset: int,
        batch: int,
        dynamic: bool,
        simplify: bool,
        max_det: int,
        conf: float,
        iou: float,
    ) -> Path:
        pt_path = self.save_pt(upload)
        stem = pt_path.stem
        self.pt_validator.validate(pt_path, spec.family, spec.task)

        bundle_dir = self.prepare_bundle_dir(stem)
        self.run_exporter(
            spec,
            pt_path,
            bundle_dir,
            size,
            opset,
            batch,
            dynamic,
            simplify,
            max_det,
            conf,
            iou,
        )
        meta_iou = iou if spec.uses_iou else None
        self.write_meta(spec, bundle_dir, stem, batch, max_det, conf, meta_iou)
        self.logger.info("export done stem=%s bundle_dir=%s", stem, bundle_dir)
        return bundle_dir


class ExportService:
    def __init__(self, runner: ExportRunner) -> None:
        self.runner = runner

    def list_types(self) -> ApiJsonResponse:
        items = [
            {
                "label": spec.label,
                "route": route,
                "family": spec.family,
                "task": spec.task,
            }
            for route, spec in EXPORT_SPECS.items()
        ]
        return ApiJsonResponse.ok(data={"items": items})

    def export(
        self,
        route: str,
        upload: UploadFile,
        size: int,
        dynamic: bool,
        simplify: bool,
        batch: int,
        opset: int,
        max_det: int | None,
        conf: float = DEFAULT_CONF,
        iou: float = DEFAULT_IOU,
    ) -> ApiJsonResponse:
        if dynamic and batch > 1:
            raise AppError("dynamic batch and static batch > 1 are incompatible")
        if not 0.0 < conf <= 1.0:
            raise AppError("conf must be in (0, 1]")
        if not 0.0 < iou <= 1.0:
            raise AppError("iou must be in (0, 1]")
        spec = EXPORT_SPECS[route]
        resolved_max_det = max_det if max_det is not None else spec.default_max_det
        bundle_dir = self.runner.run(
            spec,
            upload,
            size,
            opset,
            batch,
            dynamic,
            simplify,
            resolved_max_det,
            conf,
            iou,
        )
        return ApiJsonResponse.ok(message=str(bundle_dir))
