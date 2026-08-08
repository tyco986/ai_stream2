from django.db import transaction

from pages.pipelines.models import (
    PIPELINE_STATUS_OFFLINE,
    PIPELINE_STATUS_ONLINE,
    PIPELINE_STATUS_RUNNING,
    PIPELINE_STATUS_STARTING,
    PIPELINE_STATUS_STOPPED,
    AnalyzerTemplate,
    GieTemplate,
    Pipeline,
)
from shared.site_config.registry import site_config_registry


class PipelinesSiteConfigSlice:
    SLICE_NAME = "pipelines"

    def export_slice(self):
        pipelines = [
            {
                "id": str(row.id),
                "name": row.name,
                "type": row.type,
                "status": row.status,
                "config": row.config or {},
                "gie_id": str(row.gie_id) if row.gie_id else None,
                "status_message": row.status_message or "",
                "created_at": row.created_at.isoformat() if row.created_at else None,
                "updated_at": row.updated_at.isoformat() if row.updated_at else None,
            }
            for row in Pipeline.objects.all().order_by("name")
        ]
        gie_templates = [
            {
                "id": str(row.id),
                "name": row.name,
                "model_id": str(row.model_id),
                "model_name": row.model_name,
                "class_attrs": row.class_attrs or [],
                "created_at": row.created_at.isoformat() if row.created_at else None,
                "updated_at": row.updated_at.isoformat() if row.updated_at else None,
            }
            for row in GieTemplate.objects.all().order_by("name")
        ]
        analyzer_templates = [
            {
                "id": str(row.id),
                "name": row.name,
                "source_kind": row.source_kind,
                "source_stream_id": (
                    str(row.source_stream_id) if row.source_stream_id else None
                ),
                "source_file_name": row.source_file_name,
                "source_image_path": row.source_image_path,
                "config_width": row.config_width,
                "config_height": row.config_height,
                "captured_at": row.captured_at.isoformat() if row.captured_at else None,
                "annotations": row.annotations or [],
                "updated_at": row.updated_at.isoformat() if row.updated_at else None,
            }
            for row in AnalyzerTemplate.objects.all().order_by("name")
        ]
        return {
            "pipelines": pipelines,
            "gie_templates": gie_templates,
            "analyzer_templates": analyzer_templates,
        }

    def import_slice(self, payload):
        data = payload or {}
        pipelines_payload = list(data.get("pipelines") or [])
        gie_payload = list(data.get("gie_templates") or [])
        analyzer_payload = list(data.get("analyzer_templates") or [])
        with transaction.atomic():
            keep_gie_ids = set()
            for item in gie_payload:
                gie_id = item.get("id")
                name = (item.get("name") or "").strip()
                model_id = item.get("model_id")
                if not gie_id or not name or not model_id:
                    continue
                GieTemplate.objects.update_or_create(
                    id=gie_id,
                    defaults={
                        "name": name,
                        "model_id": model_id,
                        "model_name": item.get("model_name"),
                        "class_attrs": item.get("class_attrs") or [],
                    },
                )
                keep_gie_ids.add(str(gie_id))
            GieTemplate.objects.exclude(id__in=keep_gie_ids).delete()

            keep_analyzer_ids = set()
            for item in analyzer_payload:
                analyzer_id = item.get("id")
                name = (item.get("name") or "").strip()
                source_kind = item.get("source_kind")
                if not analyzer_id or not name or not source_kind:
                    continue
                AnalyzerTemplate.objects.update_or_create(
                    id=analyzer_id,
                    defaults={
                        "name": name,
                        "source_kind": source_kind,
                        "source_stream_id": item.get("source_stream_id"),
                        "source_file_name": item.get("source_file_name"),
                        "source_image_path": item.get("source_image_path"),
                        "config_width": int(item.get("config_width") or 1920),
                        "config_height": int(item.get("config_height") or 1080),
                        "captured_at": None,
                        "annotations": item.get("annotations") or [],
                    },
                )
                keep_analyzer_ids.add(str(analyzer_id))
            AnalyzerTemplate.objects.exclude(id__in=keep_analyzer_ids).delete()

            keep_pipeline_ids = set()
            for item in pipelines_payload:
                pipeline_id = item.get("id")
                name = (item.get("name") or "").strip()
                pipeline_type = item.get("type")
                if not pipeline_id or not name or not pipeline_type:
                    continue
                status = item.get("status") or PIPELINE_STATUS_STOPPED
                if status in (
                    PIPELINE_STATUS_ONLINE,
                    PIPELINE_STATUS_RUNNING,
                    PIPELINE_STATUS_STARTING,
                    PIPELINE_STATUS_OFFLINE,
                ):
                    status = PIPELINE_STATUS_STOPPED
                Pipeline.objects.update_or_create(
                    id=pipeline_id,
                    defaults={
                        "name": name,
                        "type": pipeline_type,
                        "status": status,
                        "config": item.get("config") or {},
                        "gie_id": item.get("gie_id"),
                        "status_message": item.get("status_message") or "",
                    },
                )
                keep_pipeline_ids.add(str(pipeline_id))
            Pipeline.objects.exclude(id__in=keep_pipeline_ids).delete()

    def register(self):
        site_config_registry.register(
            self.SLICE_NAME,
            self.export_slice,
            self.import_slice,
        )


def register_pipelines_site_config():
    PipelinesSiteConfigSlice().register()
