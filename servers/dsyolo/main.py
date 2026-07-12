import argparse
import hashlib
import json
import logging
import os
import shutil
import subprocess
import time
import traceback
from dataclasses import dataclass
from logging.handlers import RotatingFileHandler
from pathlib import Path

import onnx
import uvicorn
from fastapi import FastAPI, File, Form, UploadFile
from fastapi.exceptions import RequestValidationError
from fastapi.responses import JSONResponse
from pydantic import BaseModel
from starlette.middleware.base import BaseHTTPMiddleware
from starlette.requests import Request
from starlette.responses import Response
from ultralytics import YOLO

PROJECT_NAME = "ai_stream2"
LOG_FORMAT = "[%(asctime)s] %(levelname)s %(name)s: %(message)s"
APP_ROOT = Path(__file__).resolve().parent
SRC_ROOT = Path("/root/src")
DEFAULT_MODEL_ROOT = Path("/root/models")
DEFAULT_LOG_ROOT = Path(os.environ.get("LOG_ROOT", "/root/logs/dsyolo"))
DEFAULT_HOST = os.environ.get("HOST", "0.0.0.0")
DEFAULT_PORT = int(os.environ.get("PORT", "8090"))

ONNX_PRECISION = {
    int(onnx.TensorProto.FLOAT): "fp32",
    int(onnx.TensorProto.FLOAT16): "fp16",
    int(onnx.TensorProto.INT8): "int8",
    int(onnx.TensorProto.UINT8): "uint8",
}


class ApiErrorResponse(BaseModel):
    success: bool = False
    message: str
    output: None = None


class ApiJsonResponse(BaseModel):
    success: bool = True
    message: str = ""
    output: None = None


class RequestLogMiddleware(BaseHTTPMiddleware):
    def __init__(self, app, logger: logging.Logger) -> None:
        super().__init__(app)
        self.log = logger

    async def dispatch(self, request: Request, call_next) -> Response:
        start = time.perf_counter()
        method, path = request.method, request.url.path
        self.log.info("request start %s %s", method, path)
        response = await call_next(request)
        self.log.info(
            "request %s %s -> %s (%.1f ms)",
            method,
            path,
            response.status_code,
            (time.perf_counter() - start) * 1000,
        )
        return response


def configure_file_logger(log_root: Path, name: str = "dsyolo_api") -> logging.Logger:
    log_root.mkdir(parents=True, exist_ok=True)
    logger = logging.getLogger(name)
    logger.setLevel(logging.DEBUG)
    logger.handlers.clear()
    logger.propagate = False
    handler = RotatingFileHandler(
        log_root / "app.log",
        maxBytes=1024 * 1024,
        backupCount=10,
        encoding="utf-8",
    )
    handler.setFormatter(logging.Formatter(LOG_FORMAT))
    logger.addHandler(handler)
    return logger


def error_response(message: str, status_code: int = 400) -> JSONResponse:
    return JSONResponse(
        status_code=status_code,
        content=ApiErrorResponse(message=message).model_dump(),
    )


async def handle_validation_error(
    _: Request, exc: RequestValidationError
) -> JSONResponse:
    return error_response(str(exc.errors()), status_code=422)


def json_ok(message: str = "") -> dict:
    return ApiJsonResponse(message=message).model_dump()


@dataclass(frozen=True)
class ExportSpec:
    script: Path
    family: str
    task: str
    yolo_export: str = "deepstream_yolo"
    bundle_suffix: str = ""


EXPORT_SPECS: dict[str, ExportSpec] = {
    "export_yolo26": ExportSpec(
        SRC_ROOT / "DeepStream-Yolo-master/utils/export_yolo26.py",
        "yolo26",
        "detect",
    ),
    "export_yolo26_sahi": ExportSpec(
        APP_ROOT / "utils/export_yolo26_sahi.py",
        "yolo26",
        "detect",
        yolo_export="sahi",
        bundle_suffix="-sahi",
    ),
    "export_yolo11": ExportSpec(
        SRC_ROOT / "DeepStream-Yolo-master/utils/export_yolo11.py",
        "yolo11",
        "detect",
    ),
    "export_yolo11_pose": ExportSpec(
        SRC_ROOT / "DeepStream-Yolo-Pose-master/utils/export_yolo11_pose.py",
        "yolo11",
        "pose",
    ),
    "export_yolo11_seg": ExportSpec(
        APP_ROOT / "utils/export_yolo11_seg.py",
        "yolo11",
        "segment",
    ),
}


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def validate_pt(pt_path: Path, family: str, task: str) -> None:
    model = YOLO(str(pt_path))
    if model.task != task:
        raise ValueError(f"expected task {task!r}, got {model.task!r}")

    parts = [str(model.model.yaml)]
    ckpt = model.ckpt
    if isinstance(ckpt, dict):
        parts.extend([str(ckpt.get("model", "")), str(ckpt.get("train_args", ""))])
    blob = " ".join(parts).lower()

    if family == "yolo26" and "yolo26" not in blob:
        raise ValueError("weights are not YOLO26")
    if family == "yolo11" and "yolo11" not in blob:
        raise ValueError("weights are not YOLO11")
    if family == "yolo11" and "yolo26" in blob:
        raise ValueError("weights are YOLO26, not YOLO11")


def parse_onnx_tensor(value_info) -> dict:
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


def resolve_shape(tensor: dict, batch_size: int | None) -> list[int | None]:
    shape: list[int | None] = []
    for index, (dim, is_dynamic) in enumerate(
        zip(tensor["dims"], tensor["dynamic"], strict=True)
    ):
        if is_dynamic and index == 0:
            shape.append(batch_size)
            continue
        shape.append(dim if dim > 0 else None)
    return shape


def resolve_batch_size(input_t: dict, export_batch: int, is_dynamic: bool) -> int | None:
    if is_dynamic:
        return None
    if input_t["dynamic"][0]:
        return export_batch
    return input_t["dims"][0]


def validate_sahi_meta(meta: dict) -> None:
    if meta["input_tensor_name"] != "images":
        raise ValueError(
            f"SAHI export expected input tensor 'images', got {meta['input_tensor_name']!r}"
        )
    if meta["output_tensor_name"] != "output0":
        raise ValueError(
            f"SAHI export expected output tensor 'output0', got {meta['output_tensor_name']!r}"
        )
    output_shape = meta["output_tensor_shape"]
    if len(output_shape) < 2 or output_shape[1] != 300:
        raise ValueError(
            f"SAHI export expected output shape [batch, 300, 6], got {output_shape}"
        )


def build_meta(
    spec: ExportSpec,
    onnx_path: Path,
    labels_path: Path,
    export_batch: int,
) -> dict:
    graph = onnx.load(str(onnx_path)).graph
    if not graph.input or not graph.output:
        raise ValueError("onnx missing input or output")

    input_t = parse_onnx_tensor(graph.input[0])
    output_t = parse_onnx_tensor(graph.output[0])
    is_dynamic = any(input_t["dynamic"]) or any(output_t["dynamic"])
    batch_size = resolve_batch_size(input_t, export_batch, is_dynamic)

    classes = [
        line.strip()
        for line in labels_path.read_text(encoding="utf-8").splitlines()
        if line.strip()
    ]
    meta = {
        "model_type": "dynamic" if is_dynamic else "static",
        "input_tensor_name": input_t["name"],
        "output_tensor_name": output_t["name"],
        "classes": classes,
        "input_tensor_shape": resolve_shape(input_t, batch_size),
        "output_tensor_shape": resolve_shape(output_t, batch_size),
        "batch_size": batch_size,
        "precision": input_t["precision"],
        "version": spec.family,
        "task": spec.task,
        "yolo_export": spec.yolo_export,
    }
    if spec.yolo_export == "sahi":
        validate_sahi_meta(meta)
    return meta


def build_bundle_fingerprint(bundle_dir: Path, stem: str) -> dict:
    onnx_path = bundle_dir / f"{stem}.onnx"
    onnx_data_path = bundle_dir / f"{stem}.onnx.data"
    bundle_files = {onnx_path.name: file_sha256(onnx_path)}
    if onnx_data_path.is_file():
        bundle_files[onnx_data_path.name] = file_sha256(onnx_data_path)
    lines = [f"{name}:{bundle_files[name]}" for name in sorted(bundle_files)]
    return {
        "bundle_sha256": hashlib.sha256("\n".join(lines).encode("utf-8")).hexdigest(),
        "bundle_files": bundle_files,
    }


class ExportRunner:
    def __init__(self, logger: logging.Logger, model_root: Path = DEFAULT_MODEL_ROOT) -> None:
        self.logger = logger
        self.pt_root = model_root / "pt"
        self.onnx_root = model_root / "onnx"
        self.pt_root.mkdir(parents=True, exist_ok=True)
        self.onnx_root.mkdir(parents=True, exist_ok=True)

    def save_pt(self, upload: UploadFile) -> Path:
        filename = upload.filename or "model.pt"
        if Path(filename).suffix.lower() != ".pt":
            raise ValueError(f"input must be a .pt file: {filename}")
        pt_path = self.pt_root / Path(filename).name
        with pt_path.open("wb") as handle:
            shutil.copyfileobj(upload.file, handle)
        return pt_path

    def prepare_bundle_dir(self, stem: str, bundle_suffix: str = "") -> Path:
        bundle_dir = self.onnx_root / f"{stem}{bundle_suffix}"
        if bundle_dir.exists():
            shutil.rmtree(bundle_dir)
        bundle_dir.mkdir(parents=True)
        return bundle_dir

    def run_export_script(
        self,
        spec: ExportSpec,
        pt_path: Path,
        bundle_dir: Path,
        size: int,
        opset: int,
        batch: int,
        dynamic: bool,
        simplify: bool,
    ) -> None:
        command = [
            "python3",
            str(spec.script),
            "-w",
            str(pt_path),
            "-s",
            str(size),
            "--opset",
            str(opset),
            "--batch",
            str(batch),
        ]
        if dynamic:
            command.append("--dynamic")
        if simplify:
            command.append("--simplify")

        self.logger.info("export cmd=%s cwd=%s", command, bundle_dir)
        result = subprocess.run(
            command, cwd=bundle_dir, capture_output=True, text=True
        )
        if result.returncode != 0:
            raise RuntimeError((result.stderr or result.stdout or "export failed").strip())

    def move_onnx_into_bundle(self, pt_path: Path, bundle_dir: Path) -> None:
        stem = pt_path.stem
        target_onnx = bundle_dir / f"{stem}.onnx"
        target_data = bundle_dir / f"{stem}.onnx.data"
        source_onnx = pt_path.with_suffix(".onnx")
        source_data = pt_path.parent / f"{stem}.onnx.data"

        if not source_onnx.is_file():
            raise RuntimeError(f"missing export artifact: {target_onnx}")
        shutil.move(source_onnx, target_onnx)
        if source_data.is_file():
            shutil.move(source_data, target_data)

    def write_meta(
        self,
        spec: ExportSpec,
        bundle_dir: Path,
        stem: str,
        batch: int,
    ) -> None:
        labels_path = bundle_dir / "labels.txt"
        if not labels_path.is_file():
            raise RuntimeError(f"missing export artifact: {labels_path}")

        meta = build_meta(spec, bundle_dir / f"{stem}.onnx", labels_path, batch)
        meta.update(build_bundle_fingerprint(bundle_dir, stem))
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
    ) -> Path:
        pt_path = self.save_pt(upload)
        stem = pt_path.stem
        validate_pt(pt_path, spec.family, spec.task)

        bundle_dir = self.prepare_bundle_dir(stem, spec.bundle_suffix)
        self.run_export_script(
            spec, pt_path, bundle_dir, size, opset, batch, dynamic, simplify
        )
        self.move_onnx_into_bundle(pt_path, bundle_dir)
        self.write_meta(spec, bundle_dir, stem, batch)
        self.logger.info("export done stem=%s bundle_dir=%s", stem, bundle_dir)
        return bundle_dir


class DsYoloServer:
    def __init__(
        self,
        log_root: Path,
        model_root: Path = DEFAULT_MODEL_ROOT,
    ) -> None:
        self.logger = configure_file_logger(log_root)
        self.runner = ExportRunner(self.logger, model_root)

        self.app = FastAPI(
            title="DeepStream YOLO Export API",
            description=(
                f"Upload .pt to {model_root}/pt/, export ONNX to {model_root}/onnx/{{name}}/. "
                "Details: servers/dsyolo/README.md"
            ),
            version="1.0.0",
        )
        self.app.add_middleware(RequestLogMiddleware, logger=self.logger)
        self.app.add_exception_handler(
            RequestValidationError, handle_validation_error
        )
        self.register_routes()
        self.logger.info(
            "DsYolo API initialized log_root=%s model_root=%s",
            log_root,
            model_root,
        )

    def register_routes(self) -> None:
        prefix = f"/{PROJECT_NAME}/dsyolo"
        self.app.add_api_route(
            f"{prefix}/hello_world",
            lambda: json_ok(),
            methods=["GET"],
            summary="Health check",
            response_model=ApiJsonResponse,
        )
        for route in EXPORT_SPECS:
            self.app.add_api_route(
                f"{prefix}/{route}",
                self.export_handler(route),
                methods=["POST"],
                summary=f"Export {route}",
                response_model=ApiJsonResponse,
                responses={400: {"model": ApiErrorResponse}, 422: {"model": ApiErrorResponse}},
            )

    def export_handler(self, route: str):
        def handler(
            input: UploadFile = File(...),
            size: int = Form(640),
            dynamic: bool = Form(False),
            simplify: bool = Form(False),
            batch: int = Form(1),
            opset: int = Form(18),
        ) -> Response:
            return self.handle_export(route, input, size, dynamic, simplify, batch, opset)

        handler.__name__ = route
        return handler

    def handle_export(
        self,
        route: str,
        input: UploadFile,
        size: int,
        dynamic: bool,
        simplify: bool,
        batch: int,
        opset: int,
    ) -> Response:
        try:
            if dynamic and batch > 1:
                raise ValueError("dynamic batch and static batch > 1 are incompatible")
            bundle_dir = self.runner.run(
                EXPORT_SPECS[route],
                input,
                size,
                opset,
                batch,
                dynamic,
                simplify,
            )
            return JSONResponse(json_ok(message=str(bundle_dir)))
        except Exception:
            message = traceback.format_exc()
            self.logger.error("export failed route=%s\n%s", route, message)
            return error_response(message)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="DeepStream YOLO export API")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--log-root", default=str(DEFAULT_LOG_ROOT))
    parser.add_argument("--model-root", default=str(DEFAULT_MODEL_ROOT))
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    server = DsYoloServer(
        log_root=Path(args.log_root),
        model_root=Path(args.model_root),
    )
    uvicorn.run(server.app, host=args.host, port=args.port)
