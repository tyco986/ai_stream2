import json
import shutil
import threading
from datetime import datetime, timezone
from pathlib import Path

from django.conf import settings
from django.db import close_old_connections, transaction
from django.db.models import Q

from pages.models_page.clients import ExportOnnxClient, ExportTrtClient
from pages.models_page.models import (
    DEFAULT_CONF,
    DEFAULT_IOU,
    DEFAULT_OPTIMIZATION_LEVEL,
    OPTIMIZATION_LEVEL_CHOICES,
    PRECISION_FP16,
    STATUS_BUILT,
    STATUS_BUILDING,
    STATUS_NOT_BUILT,
    BATCH_MODE_DYNAMIC,
    MlModel,
)
from shared.http.exceptions import AppError
from shared.pagination import PaginationService


class ModelTypeService:
    def list_types(self):
        items = []
        try:
            items = ExportOnnxClient().list_types()
        except AppError:
            return {"items": []}
        return {"items": items}


class ModelService:
    def __init__(self):
        self.pagination = PaginationService()
        self.models_root = Path(settings.MODELS_ROOT)
        self.pt_dir = self.models_root / "pt"

    def maps(self):
        return {row.name: str(row.id) for row in MlModel.objects.all().order_by("name")}

    def list_models(self, search=None, page=1, page_size=20):
        queryset = MlModel.objects.all().order_by("name")
        if search:
            text = search.strip()
            if text:
                queryset = queryset.filter(
                    Q(name__icontains=text) | Q(source_file__icontains=text)
                )
        page_data = self.pagination.slice_queryset(queryset, page, page_size)
        items = [self.serialize(row) for row in page_data["items"]]
        return self.pagination.build(
            items,
            page_data["total"],
            page_data["page"],
            page_data["page_size"],
        )

    def get(self, model_id):
        return self.serialize(self.resolve(model_id))

    def create(
        self,
        name,
        batch_mode,
        batch_size,
        family,
        precision=None,
        optimization_level=None,
        conf=None,
        iou=None,
        upload=None,
    ):
        cleaned_name = (name or "").strip()
        if not cleaned_name:
            raise AppError("name is required", status_code=400)
        cleaned_family = (family or "").strip()
        if not cleaned_family:
            raise AppError("family is required", status_code=400)
        if MlModel.objects.filter(name=cleaned_name).exists():
            raise AppError("Name already exists", status_code=409)
        precision_value = (precision or PRECISION_FP16).strip() or PRECISION_FP16
        if precision_value != PRECISION_FP16:
            raise AppError("precision must be fp16", status_code=400)
        if batch_size < 1 or batch_size > 128:
            raise AppError("batch_size must be 1..128", status_code=400)
        level = (
            DEFAULT_OPTIMIZATION_LEVEL
            if optimization_level is None
            else int(optimization_level)
        )
        allowed_levels = {item[0] for item in OPTIMIZATION_LEVEL_CHOICES}
        if level not in allowed_levels:
            raise AppError("optimization_level must be 0..5", status_code=400)
        conf_value = DEFAULT_CONF if conf is None else float(conf)
        iou_value = DEFAULT_IOU if iou is None else float(iou)
        if not 0.0 < conf_value <= 1.0:
            raise AppError("conf must be in (0, 1]", status_code=400)
        if not 0.0 < iou_value <= 1.0:
            raise AppError("iou must be in (0, 1]", status_code=400)
        row = MlModel.objects.create(
            name=cleaned_name,
            family=cleaned_family,
            batch_size=int(batch_size),
            batch_mode=batch_mode,
            precision=precision_value,
            optimization_level=level,
            conf=conf_value,
            iou=iou_value,
            status=STATUS_NOT_BUILT,
        )
        if upload is not None:
            self.store_pt(row, upload)
            row.save()
        return self.serialize(row)

    def update(
        self,
        model_id,
        name,
        batch_mode,
        batch_size,
        family,
        precision=None,
        optimization_level=None,
        conf=None,
        iou=None,
        upload=None,
    ):
        row = self.resolve(model_id)
        if row.status == STATUS_BUILDING:
            raise AppError("Model is building", status_code=409)
        cleaned_name = (name or "").strip()
        if not cleaned_name:
            raise AppError("name is required", status_code=400)
        cleaned_family = (family or "").strip()
        if not cleaned_family:
            raise AppError("family is required", status_code=400)
        if MlModel.objects.filter(name=cleaned_name).exclude(pk=row.pk).exists():
            raise AppError("Name already exists", status_code=409)
        precision_value = (precision or PRECISION_FP16).strip() or PRECISION_FP16
        if precision_value != PRECISION_FP16:
            raise AppError("precision must be fp16", status_code=400)
        if batch_size < 1 or batch_size > 128:
            raise AppError("batch_size must be 1..128", status_code=400)
        level = (
            DEFAULT_OPTIMIZATION_LEVEL
            if optimization_level is None
            else int(optimization_level)
        )
        allowed_levels = {item[0] for item in OPTIMIZATION_LEVEL_CHOICES}
        if level not in allowed_levels:
            raise AppError("optimization_level must be 0..5", status_code=400)
        conf_value = DEFAULT_CONF if conf is None else float(conf)
        iou_value = DEFAULT_IOU if iou is None else float(iou)
        if not 0.0 < conf_value <= 1.0:
            raise AppError("conf must be in (0, 1]", status_code=400)
        if not 0.0 < iou_value <= 1.0:
            raise AppError("iou must be in (0, 1]", status_code=400)
        file_changed = upload is not None
        config_changed = (
            row.batch_mode != batch_mode
            or row.family != cleaned_family
            or int(row.batch_size) != int(batch_size)
            or row.precision != precision_value
            or int(row.optimization_level) != level
            or float(row.conf) != conf_value
            or float(row.iou) != iou_value
        )
        if file_changed:
            self.store_pt(row, upload)
        if file_changed or config_changed:
            self.clear_engine(row)
            self.reset_build_metadata(row)
            row.status = STATUS_NOT_BUILT
        row.name = cleaned_name
        row.family = cleaned_family
        row.batch_mode = batch_mode
        row.batch_size = int(batch_size)
        row.precision = precision_value
        row.optimization_level = level
        row.conf = conf_value
        row.iou = iou_value
        row.save()
        return self.serialize(row)

    def delete(self, model_id):
        row = self.resolve(model_id)
        if row.status == STATUS_BUILDING:
            raise AppError("Model is building", status_code=409)
        self.clear_files(row)
        row.delete()
        return {}

    def batch_delete(self, ids):
        deleted_count = 0
        failed_ids = []
        with transaction.atomic():
            for model_id in ids or []:
                row = MlModel.objects.filter(pk=model_id).first()
                if row is None:
                    failed_ids.append(str(model_id))
                    continue
                if row.status == STATUS_BUILDING:
                    failed_ids.append(str(row.id))
                    continue
                self.clear_files(row)
                row.delete()
                deleted_count += 1
        return {"deleted_count": deleted_count, "failed_ids": failed_ids}

    def resolve(self, model_id):
        row = MlModel.objects.filter(pk=model_id).first()
        if row is None:
            raise AppError("Model not found", status_code=404)
        return row

    def store_pt(self, row, upload):
        filename = Path(getattr(upload, "name", "") or "").name
        if not filename.lower().endswith(".pt"):
            raise AppError("source_file must be .pt", status_code=400)
        self.pt_dir.mkdir(parents=True, exist_ok=True)
        target = self.pt_dir / filename
        old_path = Path(row.source_path) if row.source_path else None
        if old_path is not None and old_path != target and old_path.is_file():
            old_path.unlink(missing_ok=True)
        if MlModel.objects.filter(source_path=str(target)).exclude(pk=row.pk).exists():
            raise AppError("source_file already exists", status_code=409)
        with target.open("wb") as handle:
            for chunk in upload.chunks():
                handle.write(chunk)
        row.source_path = str(target)
        row.source_file = str(target)

    def reset_build_metadata(self, row):
        row.version = None
        row.task = None
        row.num_class = None
        row.classes = None
        row.had_successful_build = False
        row.last_build_error = None

    def clear_engine(self, row):
        if row.engine_path:
            path = Path(row.engine_path)
            if path.is_dir():
                shutil.rmtree(path, ignore_errors=True)
            elif path.is_file():
                path.unlink(missing_ok=True)
        row.engine_path = None

    def clear_files(self, row):
        self.clear_engine(row)
        if row.source_path:
            path = Path(row.source_path)
            if path.is_file():
                path.unlink(missing_ok=True)

    def serialize(self, row):
        last_build_at = None
        if row.last_build_at is not None:
            last_build_at = row.last_build_at.strftime("%Y-%m-%dT%H:%M:%SZ")
        return {
            "id": str(row.id),
            "name": row.name,
            "family": row.family,
            "version": row.version,
            "batch_size": row.batch_size,
            "batch_mode": row.batch_mode,
            "precision": row.precision,
            "optimization_level": row.optimization_level,
            "conf": row.conf,
            "iou": row.iou,
            "task": row.task,
            "num_class": row.num_class,
            "classes": row.classes,
            "source_file": row.source_file,
            "status": row.status,
            "last_build_at": last_build_at,
        }


class ModelLogService:
    def __init__(self):
        self.log_dir = Path(settings.MODELS_BUILD_LOG_DIR)
        self.models = ModelService()

    def get_logs(self, model_id, tail=None):
        self.models.resolve(model_id)
        path = self.log_dir / f"{model_id}.log"
        content = ""
        if path.is_file():
            text = path.read_text(encoding="utf-8", errors="replace")
            lines = text.splitlines()
            if tail is not None:
                limit = int(tail)
                if limit > 0:
                    lines = lines[-limit:]
            content = "\n".join(lines)
            if content and not content.endswith("\n"):
                content = content + "\n"
        return {"content": content}

    def append(self, model_id, message):
        self.log_dir.mkdir(parents=True, exist_ok=True)
        path = self.log_dir / f"{model_id}.log"
        stamp = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S")
        with path.open("a", encoding="utf-8") as handle:
            handle.write(f"{stamp} {message}\n")


class BuildOrchestrator:
    def __init__(self):
        self.models = ModelService()
        self.logs = ModelLogService()
        self.export_onnx = ExportOnnxClient()
        self.export_trt = ExportTrtClient()

    def start(self, model_id):
        row = self.models.resolve(model_id)
        if not row.source_path or not Path(row.source_path).is_file():
            raise AppError("source_file is required before build", status_code=400)
        if row.status == STATUS_BUILDING:
            raise AppError("Model is building", status_code=409)
        row.status = STATUS_BUILDING
        row.last_build_error = None
        row.save(update_fields=["status", "last_build_error"])
        self.logs.append(row.id, "build accepted")
        thread = threading.Thread(
            target=self.run_build,
            args=(str(row.id),),
            daemon=True,
        )
        thread.start()
        return {"id": str(row.id), "status": STATUS_BUILDING}

    def get_status(self, model_id):
        row = self.models.resolve(model_id)
        data = {
            "id": str(row.id),
            "status": row.status,
            "last_build_at": None,
        }
        if row.last_build_at is not None:
            data["last_build_at"] = row.last_build_at.strftime("%Y-%m-%dT%H:%M:%SZ")
        if row.status != STATUS_BUILDING:
            data.update(self.build_result_fields(row))
        return data

    def build_result_fields(self, row):
        last_build_at = None
        if row.last_build_at is not None:
            last_build_at = row.last_build_at.strftime("%Y-%m-%dT%H:%M:%SZ")
        success = row.status == STATUS_BUILT and not row.last_build_error
        return {
            "success": success,
            "version": row.version,
            "task": row.task,
            "num_class": row.num_class,
            "classes": row.classes,
            "last_build_at": last_build_at,
            "error": row.last_build_error,
        }

    def run_build(self, model_id):
        close_old_connections()
        row = MlModel.objects.filter(pk=model_id).first()
        if row is None:
            return
        error = None
        try:
            self.logs.append(model_id, "export_onnx export start")
            onnx_dir = self.export_onnx.export_pt(
                Path(row.source_path),
                dynamic=(row.batch_mode == BATCH_MODE_DYNAMIC),
                batch_size=row.batch_size,
                family=row.family,
                conf=row.conf,
                iou=row.iou,
            )
            self.logs.append(model_id, f"export_onnx export done: {onnx_dir}")
            self.logs.append(model_id, "export_trt start")
            engine_dir = self.export_trt.export_engine(
                onnx_dir,
                batch_size=row.batch_size,
                dynamic=(row.batch_mode == BATCH_MODE_DYNAMIC),
                precision=row.precision or PRECISION_FP16,
                optimization_level=row.optimization_level,
            )
            self.logs.append(model_id, f"export_trt done: {engine_dir}")
            meta = self.load_meta(onnx_dir)
            classes = self.load_classes(onnx_dir)
            row.engine_path = engine_dir
            row.version = self.next_version(row)
            row.task = meta.get("task") or row.task
            row.classes = classes
            row.num_class = len(classes) if classes is not None else row.num_class
            row.status = STATUS_BUILT
            row.had_successful_build = True
            row.last_build_error = None
            row.last_build_at = datetime.now(timezone.utc)
            row.save()
            self.logs.append(model_id, "build succeeded")
        except Exception as exc:
            error = str(exc)
            self.logs.append(model_id, f"build failed: {error}")
            row = MlModel.objects.filter(pk=model_id).first()
            if row is not None:
                row.status = (
                    STATUS_BUILT if row.had_successful_build else STATUS_NOT_BUILT
                )
                row.last_build_error = error
                row.last_build_at = datetime.now(timezone.utc)
                row.save(
                    update_fields=[
                        "status",
                        "last_build_error",
                        "last_build_at",
                    ]
                )
        finally:
            close_old_connections()

    def load_meta(self, onnx_dir):
        path = Path(onnx_dir) / "meta.json"
        meta = {}
        if path.is_file():
            meta = json.loads(path.read_text(encoding="utf-8"))
        return meta if isinstance(meta, dict) else {}

    def load_classes(self, onnx_dir):
        path = Path(onnx_dir) / "labels.txt"
        classes = None
        if path.is_file():
            lines = [
                line.strip()
                for line in path.read_text(encoding="utf-8").splitlines()
                if line.strip()
            ]
            classes = [{"id": index, "name": name} for index, name in enumerate(lines)]
        return classes

    def next_version(self, row):
        version = "v1"
        if row.version:
            text = row.version.strip().lstrip("vV")
            if text.isdigit():
                version = f"v{int(text) + 1}"
        return version
