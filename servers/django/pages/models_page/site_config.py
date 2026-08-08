from django.db import transaction

from pages.models_page.models import (
    DEFAULT_CONF,
    DEFAULT_IOU,
    DEFAULT_OPTIMIZATION_LEVEL,
    PRECISION_FP16,
    STATUS_NOT_BUILT,
    BATCH_MODE_STATIC,
    MlModel,
)
from shared.site_config.registry import site_config_registry


class ModelsSiteConfigSlice:
    SLICE_NAME = "models"

    def export_slice(self):
        items = [
            {
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
                "source_path": row.source_path,
                "engine_path": row.engine_path,
                "status": row.status if row.status != "building" else STATUS_NOT_BUILT,
                "had_successful_build": bool(row.had_successful_build),
            }
            for row in MlModel.objects.all().order_by("name")
        ]
        return {"models": items}

    def import_slice(self, payload):
        items = list((payload or {}).get("models") or [])
        with transaction.atomic():
            keep_ids = set()
            for item in items:
                model_id = item.get("id")
                name = (item.get("name") or "").strip()
                if not model_id or not name:
                    continue
                status = item.get("status") or STATUS_NOT_BUILT
                if status == "building":
                    status = STATUS_NOT_BUILT
                MlModel.objects.update_or_create(
                    id=model_id,
                    defaults={
                        "name": name,
                        "family": item.get("family") or "yolo11",
                        "version": item.get("version"),
                        "batch_size": int(item.get("batch_size") or 1),
                        "batch_mode": item.get("batch_mode") or BATCH_MODE_STATIC,
                        "precision": item.get("precision") or PRECISION_FP16,
                        "optimization_level": int(
                            item.get("optimization_level")
                            if item.get("optimization_level") is not None
                            else DEFAULT_OPTIMIZATION_LEVEL
                        ),
                        "conf": float(
                            item.get("conf")
                            if item.get("conf") is not None
                            else DEFAULT_CONF
                        ),
                        "iou": float(
                            item.get("iou")
                            if item.get("iou") is not None
                            else DEFAULT_IOU
                        ),
                        "task": item.get("task"),
                        "num_class": item.get("num_class"),
                        "classes": item.get("classes"),
                        "source_file": item.get("source_file"),
                        "source_path": item.get("source_path"),
                        "engine_path": item.get("engine_path"),
                        "status": status,
                        "had_successful_build": bool(
                            item.get("had_successful_build", False)
                        ),
                    },
                )
                keep_ids.add(str(model_id))
            MlModel.objects.exclude(id__in=keep_ids).delete()

    def register(self):
        site_config_registry.register(
            self.SLICE_NAME,
            self.export_slice,
            self.import_slice,
        )


def register_models_site_config():
    ModelsSiteConfigSlice().register()
