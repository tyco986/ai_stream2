import httpx
from django.conf import settings

from shared.http.exceptions import AppError


class ExportOnnxClient:
    def __init__(self, base_url=None, timeout=None, route=None):
        self.base_url = (
            base_url if base_url is not None else settings.EXPORT_ONNX_BASE_URL
        ).rstrip("/")
        self.timeout = (
            timeout if timeout is not None else settings.MODELS_BUILD_TIMEOUT
        )
        self.route = route if route is not None else settings.MODELS_EXPORT_ONNX_ROUTE
        self.prefix = f"/{settings.PROJECT_NAME}/export_onnx"

    def list_types(self):
        url = f"{self.base_url}{self.prefix}/types"
        http_timeout = httpx.Timeout(30.0, connect=10.0)
        try:
            with httpx.Client(timeout=http_timeout) as client:
                response = client.get(url)
        except httpx.HTTPError as exc:
            raise AppError(f"export_onnx unreachable: {exc}", status_code=502) from exc
        payload = {}
        if response.content:
            payload = response.json()
        if not isinstance(payload, dict):
            raise AppError("Invalid export_onnx types response", status_code=502)
        if response.status_code >= 400 or not payload.get("success"):
            message = payload.get("message") or f"export_onnx HTTP {response.status_code}"
            raise AppError(message, status_code=502)
        data = payload.get("data") or {}
        items = data.get("items") if isinstance(data, dict) else None
        if not isinstance(items, list):
            raise AppError("export_onnx types missing items", status_code=502)
        return items

    def export_pt(self, pt_path, dynamic, batch_size, family=None, conf=None, iou=None):
        route = self.route
        if family:
            route = f"export_{family}"
            stem = pt_path.stem.lower()
            if stem.endswith("-seg") or stem.endswith("_seg"):
                route = f"{route}_seg"
        url = f"{self.base_url}{self.prefix}/{route}"
        data = {
            "dynamic": "true" if dynamic else "false",
        }
        # Dynamic ONNX uses batch=1 default; max batch is applied at export_trt.
        if not dynamic:
            data["batch"] = str(int(batch_size))
        if conf is not None:
            data["conf"] = str(float(conf))
        if iou is not None:
            data["iou"] = str(float(iou))
        payload = {}
        http_timeout = httpx.Timeout(self.timeout, connect=10.0)
        try:
            with pt_path.open("rb") as handle:
                files = {
                    "input": (pt_path.name, handle, "application/octet-stream"),
                }
                with httpx.Client(timeout=http_timeout) as client:
                    response = client.post(url, data=data, files=files)
        except httpx.HTTPError as exc:
            raise AppError(f"export_onnx unreachable: {exc}", status_code=502) from exc
        if response.content:
            payload = response.json()
        if not isinstance(payload, dict):
            raise AppError("Invalid export_onnx response", status_code=502)
        if response.status_code >= 400 or not payload.get("success"):
            message = payload.get("message") or f"export_onnx HTTP {response.status_code}"
            raise AppError(message, status_code=502)
        onnx_dir = payload.get("message") or ""
        if not onnx_dir:
            raise AppError("export_onnx did not return onnx path", status_code=502)
        return onnx_dir


class ExportTrtClient:
    def __init__(self, base_url=None, timeout=None):
        self.base_url = (
            base_url if base_url is not None else settings.EXPORT_TRT_BASE_URL
        ).rstrip("/")
        self.timeout = (
            timeout if timeout is not None else settings.MODELS_BUILD_TIMEOUT
        )
        self.prefix = f"/{settings.PROJECT_NAME}/export_trt"

    def export_engine(
        self,
        onnx_dir,
        batch_size,
        dynamic,
        precision="fp16",
        optimization_level=None,
        family=None,
        task=None,
    ):
        if not family:
            raise AppError("family is required", status_code=400)
        route = f"export_{family}"
        if task == "segment":
            route = f"{route}_seg"
        url = f"{self.base_url}{self.prefix}/{route}"
        data = {
            "input": onnx_dir,
            "precision": precision,
        }
        if dynamic:
            data["batch_size"] = str(int(batch_size))
        if optimization_level is not None:
            data["opt_level"] = str(int(optimization_level))
        payload = {}
        http_timeout = httpx.Timeout(self.timeout, connect=10.0)
        try:
            with httpx.Client(timeout=http_timeout) as client:
                response = client.post(url, data=data)
        except httpx.HTTPError as exc:
            raise AppError(f"export_trt unreachable: {exc}", status_code=502) from exc
        if response.content:
            payload = response.json()
        if not isinstance(payload, dict):
            raise AppError("Invalid export_trt response", status_code=502)
        if response.status_code >= 400 or not payload.get("success"):
            message = payload.get("message") or f"export_trt HTTP {response.status_code}"
            raise AppError(message, status_code=502)
        engine_dir = payload.get("data") or payload.get("message") or ""
        if not engine_dir:
            raise AppError("export_trt did not return engine path", status_code=502)
        return str(engine_dir)
