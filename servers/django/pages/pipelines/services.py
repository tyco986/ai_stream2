import shutil
import threading
from datetime import datetime, timezone
from pathlib import Path

import yaml
from django.apps import apps
from django.conf import settings
from django.db import close_old_connections
from django.db.models import Q
from django.utils import timezone as django_timezone

from pages.pipelines.clients import DeepStreamClient, GeneratorClient, SnapshotClient
from pages.pipelines.container_service import DeepStreamContainerService
from pages.pipelines.models import (
    PIPELINE_STATUS_ERROR,
    PIPELINE_STATUS_OFFLINE,
    PIPELINE_STATUS_ONLINE,
    PIPELINE_STATUS_RUNNING,
    PIPELINE_STATUS_STARTING,
    PIPELINE_STATUS_STOPPED,
    PIPELINE_STATUS_STOPPING,
    SOURCE_KIND_FILE,
    SOURCE_KIND_STREAM,
    AnalyzerTemplate,
    GieTemplate,
    Pipeline,
)
from pages.pipelines.port_manager import DeepStreamPortManager
from pages.pipelines.type_registry import (
    PARSER_PIPELINE_TYPES,
    RTSP_PIPELINE_TYPES,
    TypeRegistry,
)
from shared.http.exceptions import AppError
from shared.models_lookup import model_built_resolver
from shared.pagination import PaginationService
from shared.streams_lookup import stream_name_resolver

MUTATION_BLOCKED_STATUSES = frozenset(
    {
        PIPELINE_STATUS_ONLINE,
        PIPELINE_STATUS_RUNNING,
        PIPELINE_STATUS_STARTING,
    }
)

ANNOTATION_TYPES = (
    "roi_filtering",
    "overcrowding",
    "line_crossing",
    "direction_detection",
)


class PipelineLogService:
    def __init__(self, log_dir=None):
        self.log_dir = Path(
            log_dir if log_dir is not None else settings.PIPELINES_LOG_DIR
        )

    def append(self, pipeline_id, message):
        self.log_dir.mkdir(parents=True, exist_ok=True)
        path = self.log_dir / f"{pipeline_id}.log"
        stamp = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S")
        with path.open("a", encoding="utf-8") as handle:
            handle.write(f"{stamp} {message}\n")

    def get_logs(self, pipeline_id, tail=None, offset=None):
        path = self.log_dir / f"{pipeline_id}.log"
        lines = []
        updated_at = None
        if path.is_file():
            stat = path.stat()
            updated_at = datetime.fromtimestamp(stat.st_mtime, tz=timezone.utc).strftime(
                "%Y-%m-%dT%H:%M:%SZ"
            )
            text = path.read_text(encoding="utf-8", errors="replace")
            lines = text.splitlines()
            if offset is not None:
                start = int(offset)
                if start < 0:
                    start = 0
                lines = lines[start:]
            if tail is not None:
                limit = int(tail)
                if limit > 0:
                    lines = lines[-limit:]
        data = {"lines": lines, "updated_at": updated_at}
        return data


class PipelineService:
    def __init__(self):
        self.pagination = PaginationService()
        self.registry = TypeRegistry()
        self.logs = PipelineLogService()
        self.containers = DeepStreamContainerService()
        self.ports = DeepStreamPortManager()
        self.generator = GeneratorClient()

    def list_pipelines(self, search=None, page=1, page_size=20):
        queryset = Pipeline.objects.all().order_by("name")
        if search:
            term = search.strip()
            if term:
                queryset = queryset.filter(name__icontains=term)
        page_data = self.pagination.slice_queryset(queryset, page, page_size)
        items = [self.serialize_list_item(row) for row in page_data["items"]]
        return self.pagination.build(
            items,
            page_data["total"],
            page_data["page"],
            page_data["page_size"],
        )

    def pipelines_map(self):
        return {str(row.id): row.name for row in Pipeline.objects.all().order_by("name")}

    def get(self, pipeline_id):
        return self.serialize(self.resolve(pipeline_id))

    def create(self, body):
        cleaned_name = (body.get("name") or "").strip()
        if not cleaned_name:
            raise AppError("name is required", status_code=400)
        self.containers.validate_pipeline_name(cleaned_name)
        if Pipeline.objects.filter(name=cleaned_name).exists():
            raise AppError("Pipeline name already exists", status_code=409)
        pipeline_type = (body.get("type") or "").strip()
        self.validate_type(pipeline_type)
        self.validate_body(body)
        config = self.extract_config(body)
        gie_id = body.get("gie_id")
        host_port = self.ports.allocate()
        row = Pipeline.objects.create(
            name=cleaned_name,
            type=pipeline_type,
            status=PIPELINE_STATUS_STOPPED,
            config=config,
            gie_id=gie_id,
            host_port=host_port,
        )
        try:
            self.provision_configs(row)
            self.containers.create(row.id, row.name, row.host_port)
        except Exception:
            self.cleanup_resources(row.name)
            row.delete()
            raise
        self.logs.append(str(row.id), f"created port={host_port}")
        return self.serialize(row)

    def update(self, pipeline_id, body):
        row = self.resolve(pipeline_id)
        self.reject_if_running(row)
        cleaned_name = (body.get("name") or "").strip()
        if not cleaned_name:
            raise AppError("name is required", status_code=400)
        self.containers.validate_pipeline_name(cleaned_name)
        if Pipeline.objects.filter(name=cleaned_name).exclude(pk=row.pk).exists():
            raise AppError("Pipeline name already exists", status_code=409)
        pipeline_type = (body.get("type") or "").strip()
        self.validate_type(pipeline_type)
        self.validate_body(body)
        old_name = row.name
        row.name = cleaned_name
        row.type = pipeline_type
        row.config = self.extract_config(body)
        row.gie_id = body.get("gie_id")
        if row.host_port is None:
            row.host_port = self.ports.allocate()
        row.save()
        if old_name != cleaned_name:
            self.containers.remove(old_name)
            self.cleanup_config_files(old_name)
            self.provision_configs(row)
            self.containers.create(row.id, row.name, row.host_port)
        else:
            self.provision_configs(row)
        self.logs.append(str(row.id), "updated")
        return self.serialize(row)

    def delete(self, pipeline_id):
        row = self.resolve(pipeline_id)
        self.reject_if_running(row)
        self.logs.append(str(row.id), "deleted")
        self.containers.remove(row.name)
        self.cleanup_config_files(row.name)
        row.delete()
        return {}

    def batch_delete(self, ids):
        deleted_count = 0
        failed_ids = []
        for pipeline_id in ids or []:
            row = Pipeline.objects.filter(pk=pipeline_id).first()
            if row is None:
                failed_ids.append(str(pipeline_id))
                continue
            if row.status in MUTATION_BLOCKED_STATUSES:
                failed_ids.append(str(row.id))
                continue
            self.logs.append(str(row.id), "deleted")
            self.containers.remove(row.name)
            self.cleanup_config_files(row.name)
            row.delete()
            deleted_count += 1
        return {"deleted_count": deleted_count, "failed_ids": failed_ids}

    def ensure_runtime(self, row):
        changed = False
        if row.host_port is None:
            row.host_port = self.ports.allocate()
            changed = True
        if changed:
            row.save(update_fields=["host_port", "updated_at"])
        yaml_path = self.containers.deepstream_config_path(row.name)
        generator_dir = self.containers.generator_config_dir(row.name)
        need_configs = not yaml_path.is_file() or not generator_dir.is_dir()
        if need_configs:
            self.provision_configs(row)
        if self.containers.exists(row.name):
            if not self.containers.has_required_binds(row.name):
                self.containers.remove(row.name)
                self.containers.create(row.id, row.name, row.host_port)
        else:
            self.containers.create(row.id, row.name, row.host_port)
        self.logs.append(
            str(row.id),
            f"runtime ready port={row.host_port} container={self.containers.container_name(row.name)}",
        )

    def provision_configs(self, row):
        orchestrator = StartStopOrchestrator()
        generator_yaml = orchestrator.build_generator_yaml(row)
        self.generator.generate(generator_yaml.encode("utf-8"))
        start_yaml = orchestrator.build_start_yaml(row)
        path = self.containers.deepstream_config_path(row.name)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(start_yaml, encoding="utf-8")

    def cleanup_config_files(self, pipeline_name):
        generator_dir = self.containers.generator_config_dir(pipeline_name)
        if generator_dir.is_dir():
            shutil.rmtree(generator_dir, ignore_errors=True)
        yaml_path = self.containers.deepstream_config_path(pipeline_name)
        if yaml_path.is_file():
            yaml_path.unlink()

    def cleanup_resources(self, pipeline_name):
        self.containers.remove(pipeline_name)
        self.cleanup_config_files(pipeline_name)

    def validate_type(self, pipeline_type):
        if not pipeline_type:
            raise AppError("type is required", status_code=400)
        if TypeRegistry.get_schema(pipeline_type) is None:
            raise AppError(f"Unknown pipeline type: {pipeline_type}", status_code=400)

    def validate_body(self, body):
        gie_id = body.get("gie_id")
        if gie_id is None:
            raise AppError("gie_id is required", status_code=400)
        gie = GieTemplate.objects.filter(pk=gie_id).first()
        if gie is None:
            raise AppError("GIE template not found", status_code=400)
        if not model_built_resolver.is_built(gie.model_id):
            raise AppError("GIE model is not built", status_code=400)
        pipeline_type = body.get("type")
        schema = TypeRegistry.get_schema(pipeline_type)
        generator_params = schema["generator"]["params"]
        streams = body.get("streams") or []
        if generator_params["streams"]["available"] and not streams:
            raise AppError("streams is required for this pipeline type", status_code=400)
        if generator_params["input"]["available"] and not (body.get("input") or "").strip():
            raise AppError("input is required for this pipeline type", status_code=400)
        if generator_params["output"]["available"] and not (body.get("output") or "").strip():
            raise AppError("output is required for this pipeline type", status_code=400)
        if generator_params["sahi"]["available"] and not body.get("sahi"):
            raise AppError("sahi is required for this pipeline type", status_code=400)
        analyzer = body.get("analyzer")
        if analyzer:
            analyzer_streams = analyzer.get("streams") or []
            stream_set = {str(item) for item in streams}
            for stream_id in analyzer_streams:
                if str(stream_id) not in stream_set:
                    raise AppError(
                        "analyzer.streams must be subset of pipeline streams",
                        status_code=400,
                    )
            template_id = analyzer.get("template")
            if template_id and not AnalyzerTemplate.objects.filter(pk=template_id).exists():
                raise AppError("Analyzer template not found", status_code=400)

    def extract_config(self, body):
        input_value = body.get("input")
        output_value = body.get("output")
        config = {
            "drawer": body.get("drawer") or {},
            "parser": body.get("parser") or {},
            "logger": body.get("logger") or {"interval": 50},
            "messager": body.get("messager") or {"interval": 0},
            "debouncer": body.get("debouncer"),
            "streams": [str(item) for item in (body.get("streams") or [])],
            "interval": int(body.get("interval") or 0),
            "tracker": body.get("tracker"),
            "analyzer": body.get("analyzer"),
            "sahi": body.get("sahi"),
            "input": input_value.strip() if isinstance(input_value, str) and input_value.strip() else None,
            "output": output_value.strip() if isinstance(output_value, str) and output_value.strip() else None,
        }
        return config

    def reject_if_running(self, row):
        if row.status in MUTATION_BLOCKED_STATUSES:
            raise AppError("Pipeline is online", status_code=409)

    def resolve(self, pipeline_id):
        row = Pipeline.objects.filter(pk=pipeline_id).first()
        if row is None:
            raise AppError("Pipeline not found", status_code=404)
        return row

    def serialize_list_item(self, row):
        return {
            "id": str(row.id),
            "name": row.name,
            "type": row.type,
            "status": row.status,
            "last_refresh_at": self.format_dt(row.last_refresh_at),
            "updated_at": self.format_dt(row.updated_at),
        }

    def serialize(self, row):
        config = row.config or {}
        data = {
            "id": str(row.id),
            "name": row.name,
            "type": row.type,
            "status": row.status,
            "drawer": config.get("drawer") or {},
            "parser": config.get("parser") or {},
            "logger": config.get("logger") or {"interval": 50},
            "messager": config.get("messager") or {"interval": 0},
            "debouncer": config.get("debouncer"),
            "streams": config.get("streams") or [],
            "gie_id": str(row.gie_id) if row.gie_id else None,
            "interval": config.get("interval", 0),
            "tracker": config.get("tracker"),
            "analyzer": config.get("analyzer"),
            "sahi": config.get("sahi"),
            "input": config.get("input"),
            "output": config.get("output"),
            "created_at": self.format_dt(row.created_at),
            "updated_at": self.format_dt(row.updated_at),
        }
        return data

    def format_dt(self, value):
        formatted = None
        if value is not None:
            formatted = value.strftime("%Y-%m-%dT%H:%M:%SZ")
        return formatted


class GieTemplateService:
    def __init__(self):
        self.pagination = PaginationService()

    def list_templates(self, search=None, page=1, page_size=20):
        queryset = GieTemplate.objects.all().order_by("name")
        if search:
            term = search.strip()
            if term:
                queryset = queryset.filter(
                    Q(name__icontains=term) | Q(model_name__icontains=term)
                )
        page_data = self.pagination.slice_queryset(queryset, page, page_size)
        items = [self.serialize_list_item(row) for row in page_data["items"]]
        return self.pagination.build(
            items,
            page_data["total"],
            page_data["page"],
            page_data["page_size"],
        )

    def get(self, gie_id):
        return self.serialize(self.resolve(gie_id))

    def create(self, body):
        cleaned_name = (body.get("name") or "").strip()
        if not cleaned_name:
            raise AppError("name is required", status_code=400)
        if GieTemplate.objects.filter(name=cleaned_name).exists():
            raise AppError("GIE template name already exists", status_code=409)
        model_id = body.get("model_id")
        self.validate_model(model_id)
        class_attrs = body.get("class_attrs") or []
        self.validate_class_attrs(class_attrs)
        model_name = self.model_name(model_id)
        row = GieTemplate.objects.create(
            name=cleaned_name,
            model_id=model_id,
            model_name=model_name,
            class_attrs=class_attrs,
        )
        return self.serialize(row)

    def update(self, gie_id, body):
        row = self.resolve(gie_id)
        cleaned_name = (body.get("name") or "").strip()
        if not cleaned_name:
            raise AppError("name is required", status_code=400)
        if GieTemplate.objects.filter(name=cleaned_name).exclude(pk=row.pk).exists():
            raise AppError("GIE template name already exists", status_code=409)
        model_id = body.get("model_id")
        self.validate_model(model_id)
        class_attrs = body.get("class_attrs") or []
        self.validate_class_attrs(class_attrs)
        row.name = cleaned_name
        row.model_id = model_id
        row.model_name = self.model_name(model_id)
        row.class_attrs = class_attrs
        row.save()
        return self.serialize(row)

    def delete(self, gie_id):
        row = self.resolve(gie_id)
        if Pipeline.objects.filter(gie_id=row.id).exists():
            raise AppError("GIE template is referenced by pipelines", status_code=409)
        row.delete()
        return {}

    def batch_delete(self, ids):
        deleted_count = 0
        failed_ids = []
        for gie_id in ids or []:
            row = GieTemplate.objects.filter(pk=gie_id).first()
            if row is None:
                failed_ids.append(str(gie_id))
                continue
            if Pipeline.objects.filter(gie_id=row.id).exists():
                failed_ids.append(str(row.id))
                continue
            row.delete()
            deleted_count += 1
        return {"deleted_count": deleted_count, "failed_ids": failed_ids}

    def validate_model(self, model_id):
        if not model_built_resolver.is_built(model_id):
            raise AppError("Model is not built", status_code=400)

    def validate_class_attrs(self, class_attrs):
        if not class_attrs:
            raise AppError("class_attrs must not be empty", status_code=400)
        seen = set()
        for row in class_attrs:
            class_key = str(row.get("class"))
            if class_key in seen:
                raise AppError("class_attrs class must be unique", status_code=400)
            seen.add(class_key)

    def model_name(self, model_id):
        info = model_built_resolver.resolve(model_id)
        name = None
        if info is not None:
            name = info.get("name")
        return name

    def referenced_pipelines(self, gie_id):
        return [
            {"id": str(row.id), "name": row.name}
            for row in Pipeline.objects.filter(gie_id=gie_id).order_by("name")
        ]

    def class_attrs_summary(self, class_attrs):
        parts = []
        for row in class_attrs or []:
            class_label = row.get("class")
            parts.append(
                f"{class_label}: conf={row.get('conf')}, topk={row.get('topk')}"
            )
        summary = ", ".join(parts)
        return summary

    def resolve(self, gie_id):
        row = GieTemplate.objects.filter(pk=gie_id).first()
        if row is None:
            raise AppError("GIE template not found", status_code=404)
        return row

    def serialize_list_item(self, row):
        return {
            "id": str(row.id),
            "name": row.name,
            "model_id": str(row.model_id),
            "model_name": row.model_name,
            "class_attrs": row.class_attrs,
            "class_attrs_summary": self.class_attrs_summary(row.class_attrs),
            "pipelines": self.referenced_pipelines(row.id),
            "created_at": self.format_dt(row.created_at),
            "updated_at": self.format_dt(row.updated_at),
        }

    def serialize(self, row):
        return self.serialize_list_item(row)

    def format_dt(self, value):
        formatted = None
        if value is not None:
            formatted = value.strftime("%Y-%m-%dT%H:%M:%SZ")
        return formatted


class AnalyzerTemplateService:
    def __init__(self):
        self.pagination = PaginationService()
        self.media_dir = Path(settings.PIPELINES_MEDIA_DIR)
        self.snapshots = SnapshotClient()

    def list_templates(self, search=None, page=1, page_size=20):
        queryset = AnalyzerTemplate.objects.all().order_by("name")
        if search:
            term = search.strip()
            if term:
                queryset = queryset.filter(name__icontains=term)
        page_data = self.pagination.slice_queryset(queryset, page, page_size)
        items = [self.serialize_list_item(row) for row in page_data["items"]]
        return self.pagination.build(
            items,
            page_data["total"],
            page_data["page"],
            page_data["page_size"],
        )

    def get(self, analyzer_id):
        return self.serialize(self.resolve(analyzer_id))

    def create(self, body):
        cleaned_name = (body.get("name") or "").strip()
        if not cleaned_name:
            raise AppError("name is required", status_code=400)
        if AnalyzerTemplate.objects.filter(name=cleaned_name).exists():
            raise AppError("Analyzer template name already exists", status_code=409)
        row = AnalyzerTemplate.objects.create(
            name=cleaned_name,
            source_kind=body.get("source_kind") or SOURCE_KIND_FILE,
            source_stream_id=body.get("source_stream_id"),
            source_file_name=body.get("source_file_name"),
            config_width=int(body.get("config_width") or 1920),
            config_height=int(body.get("config_height") or 1080),
            annotations=body.get("annotations") or [],
        )
        return self.serialize(row)

    def update(self, analyzer_id, body):
        row = self.resolve(analyzer_id)
        cleaned_name = (body.get("name") or "").strip()
        if not cleaned_name:
            raise AppError("name is required", status_code=400)
        if AnalyzerTemplate.objects.filter(name=cleaned_name).exclude(pk=row.pk).exists():
            raise AppError("Analyzer template name already exists", status_code=409)
        row.name = cleaned_name
        if "source_kind" in body:
            row.source_kind = body["source_kind"]
        if "source_stream_id" in body:
            row.source_stream_id = body.get("source_stream_id")
        if "source_file_name" in body:
            row.source_file_name = body.get("source_file_name")
        if "config_width" in body:
            row.config_width = int(body["config_width"])
        if "config_height" in body:
            row.config_height = int(body["config_height"])
        if "annotations" in body:
            row.annotations = body.get("annotations") or []
        row.save()
        return self.serialize(row)

    def delete(self, analyzer_id):
        row = self.resolve(analyzer_id)
        if self.is_referenced(row.id):
            raise AppError("Analyzer template is referenced by pipelines", status_code=409)
        self.remove_media(row)
        row.delete()
        return {}

    def batch_delete(self, ids):
        deleted_count = 0
        failed_ids = []
        for analyzer_id in ids or []:
            row = AnalyzerTemplate.objects.filter(pk=analyzer_id).first()
            if row is None:
                failed_ids.append(str(analyzer_id))
                continue
            if self.is_referenced(row.id):
                failed_ids.append(str(row.id))
                continue
            self.remove_media(row)
            row.delete()
            deleted_count += 1
        return {"deleted_count": deleted_count, "failed_ids": failed_ids}

    def upload_source_file(self, analyzer_id, upload):
        row = self.resolve(analyzer_id)
        if upload is None:
            raise AppError("Missing file", status_code=400)
        filename = getattr(upload, "name", "") or ""
        lower = filename.lower()
        if not lower.endswith((".jpg", ".jpeg", ".png")):
            raise AppError("Unsupported image format", status_code=400)
        target_dir = self.media_dir / str(row.id)
        target_dir.mkdir(parents=True, exist_ok=True)
        suffix = Path(filename).suffix.lower() or ".jpg"
        rel_path = f"{row.id}/snapshot{suffix}"
        target_path = self.media_dir / rel_path
        with target_path.open("wb") as handle:
            for chunk in upload.chunks():
                handle.write(chunk)
        width, height = self.read_image_size(target_path)
        now = django_timezone.now()
        row.source_kind = SOURCE_KIND_FILE
        row.source_stream_id = None
        row.source_file_name = filename
        row.source_image_path = rel_path
        row.config_width = width
        row.config_height = height
        row.captured_at = now
        row.save()
        return self.source_result(row)

    def capture_source_stream(self, analyzer_id, stream_id):
        row = self.resolve(analyzer_id)
        stream_name = stream_name_resolver.resolve(stream_id) or str(stream_id)
        try:
            self.snapshots.capture(stream_id)
        except AppError:
            raise AppError(f"Snapshot failed for {stream_name}", status_code=502)
        now = django_timezone.now()
        row.source_kind = SOURCE_KIND_STREAM
        row.source_stream_id = stream_id
        row.source_file_name = None
        row.captured_at = now
        row.save()
        return self.source_result(row)

    def is_referenced(self, analyzer_id):
        referenced = False
        analyzer_key = str(analyzer_id)
        for pipeline in Pipeline.objects.all().only("config"):
            config = pipeline.config or {}
            analyzer = config.get("analyzer") or {}
            template_id = analyzer.get("template")
            if template_id and str(template_id) == analyzer_key:
                referenced = True
                break
        return referenced

    def referenced_pipelines(self, analyzer_id):
        analyzer_key = str(analyzer_id)
        items = []
        for pipeline in Pipeline.objects.all().order_by("name"):
            config = pipeline.config or {}
            analyzer = config.get("analyzer") or {}
            template_id = analyzer.get("template")
            if template_id and str(template_id) == analyzer_key:
                items.append({"id": str(pipeline.id), "name": pipeline.name})
        return items

    def annotation_counts(self, annotations):
        counts = {item: 0 for item in ANNOTATION_TYPES}
        for row in annotations or []:
            annotation_type = row.get("type")
            if annotation_type in counts:
                counts[annotation_type] += 1
        return counts

    def source_image_url(self, row):
        url = ""
        if row.source_image_path:
            url = (
                f"/{settings.PROJECT_NAME}/media/analyzer-templates/"
                f"{row.source_image_path.lstrip('/')}"
            )
        return url

    def source_result(self, row):
        return {
            "source_kind": row.source_kind,
            "source_stream_id": str(row.source_stream_id) if row.source_stream_id else None,
            "source_file_name": row.source_file_name,
            "source_image_url": self.source_image_url(row),
            "config_width": row.config_width,
            "config_height": row.config_height,
            "captured_at": self.format_dt(row.captured_at),
        }

    def read_image_size(self, path):
        width = 1920
        height = 1080
        try:
            from PIL import Image

            with Image.open(path) as image:
                width, height = image.size
        except ImportError:
            pass
        return width, height

    def remove_media(self, row):
        target_dir = self.media_dir / str(row.id)
        if target_dir.is_dir():
            shutil.rmtree(target_dir, ignore_errors=True)

    def resolve(self, analyzer_id):
        row = AnalyzerTemplate.objects.filter(pk=analyzer_id).first()
        if row is None:
            raise AppError("Analyzer template not found", status_code=404)
        return row

    def serialize_list_item(self, row):
        counts = self.annotation_counts(row.annotations)
        return {
            "id": str(row.id),
            "name": row.name,
            "roi_filtering_count": counts["roi_filtering"],
            "overcrowding_count": counts["overcrowding"],
            "line_crossing_count": counts["line_crossing"],
            "direction_detection_count": counts["direction_detection"],
            "pipelines": self.referenced_pipelines(row.id),
            "updated_at": self.format_dt(row.updated_at),
        }

    def serialize(self, row):
        data = {
            "id": str(row.id),
            "name": row.name,
            "source_kind": row.source_kind,
            "source_stream_id": str(row.source_stream_id) if row.source_stream_id else None,
            "source_file_name": row.source_file_name,
            "source_image_url": self.source_image_url(row),
            "config_width": row.config_width,
            "config_height": row.config_height,
            "captured_at": self.format_dt(row.captured_at),
            "annotations": row.annotations or [],
        }
        return data

    def format_dt(self, value):
        formatted = None
        if value is not None:
            formatted = value.strftime("%Y-%m-%dT%H:%M:%SZ")
        return formatted


class StartStopOrchestrator:
    def __init__(self):
        self.pipelines = PipelineService()
        self.logs = PipelineLogService()
        self.generator = GeneratorClient()
        self.containers = DeepStreamContainerService()

    def start(self, pipeline_id):
        row = self.pipelines.resolve(pipeline_id)
        if row.status in (PIPELINE_STATUS_RUNNING, PIPELINE_STATUS_STARTING):
            raise AppError("Pipeline is already started", status_code=409)
        self.pipelines.ensure_runtime(row)
        row.refresh_from_db()
        row.status = PIPELINE_STATUS_STARTING
        row.status_message = ""
        row.save(update_fields=["status", "status_message", "updated_at"])
        thread = threading.Thread(
            target=self.run_start,
            args=(str(row.id),),
            daemon=True,
        )
        thread.start()
        data = {"id": str(row.id), "status": PIPELINE_STATUS_STARTING}
        return data

    def stop(self, pipeline_id):
        row = self.pipelines.resolve(pipeline_id)
        row.status = PIPELINE_STATUS_STOPPING
        row.save(update_fields=["status", "updated_at"])
        self.logs.append(str(row.id), "stop accepted")
        self.containers.stop(row.name)
        row.status = PIPELINE_STATUS_STOPPED
        row.status_message = ""
        row.save(update_fields=["status", "status_message", "updated_at"])
        self.logs.append(str(row.id), "stopped")
        data = {"id": str(row.id), "status": row.status}
        return data

    def get_status(self, pipeline_id):
        row = self.pipelines.resolve(pipeline_id)
        update_fields = ["last_refresh_at", "updated_at"]
        sync_statuses = (
            PIPELINE_STATUS_ONLINE,
            PIPELINE_STATUS_OFFLINE,
            PIPELINE_STATUS_RUNNING,
        )
        if row.status in sync_statuses:
            probe = self.containers.probe(row.name)
            if not probe["ok"]:
                next_status = PIPELINE_STATUS_OFFLINE
                next_message = probe["detail"] or "offline"
            elif probe["pipeline_running"]:
                next_status = PIPELINE_STATUS_RUNNING
                next_message = ""
            else:
                next_status = PIPELINE_STATUS_ONLINE
                next_message = ""
            if row.status != next_status or row.status_message != next_message:
                row.status = next_status
                row.status_message = next_message
                update_fields.extend(["status", "status_message"])
                if next_status == PIPELINE_STATUS_OFFLINE:
                    self.logs.append(str(row.id), f"status sync: {next_message}")
        row.last_refresh_at = datetime.now(timezone.utc)
        row.save(update_fields=update_fields)
        data = {
            "id": str(row.id),
            "status": row.status,
            "message": row.status_message or "",
            "last_refresh_at": self.pipelines.format_dt(row.last_refresh_at),
        }
        return data

    def run_start(self, pipeline_id):
        close_old_connections()
        row = Pipeline.objects.filter(pk=pipeline_id).first()
        if row is None:
            return
        error_message = ""
        started_container = False
        try:
            self.logs.append(pipeline_id, "start accepted")
            self.containers.start(row.name)
            started_container = True
            self.logs.append(pipeline_id, "container started")
            self.containers.wait_healthy(row.name)
            self.logs.append(pipeline_id, "container healthy")
            yaml_path = self.containers.deepstream_config_path(row.name)
            start_yaml = yaml_path.read_text(encoding="utf-8")
            client = DeepStreamClient(self.containers.base_url(row.name))
            client.start_pipeline(start_yaml.encode("utf-8"))
            row.status = PIPELINE_STATUS_RUNNING
            row.status_message = ""
            row.save(update_fields=["status", "status_message", "updated_at"])
            self.logs.append(pipeline_id, "deepstream started")
        except AppError as exc:
            error_message = str(exc.detail)
            self.logs.append(pipeline_id, f"start failed: {error_message}")
            if started_container:
                self.containers.stop(row.name)
            row.status = PIPELINE_STATUS_ERROR
            row.status_message = error_message
            row.save(update_fields=["status", "status_message", "updated_at"])
        except Exception as exc:
            error_message = str(exc)
            self.logs.append(pipeline_id, f"start failed: {error_message}")
            if started_container:
                self.containers.stop(row.name)
            row.status = PIPELINE_STATUS_ERROR
            row.status_message = error_message
            row.save(update_fields=["status", "status_message", "updated_at"])
        finally:
            close_old_connections()

    def build_generator_yaml(self, row):
        config = row.config or {}
        gie = GieTemplate.objects.filter(pk=row.gie_id).first()
        if gie is None:
            raise AppError("GIE template not found", status_code=400)
        ml_model = apps.get_model("models", "MlModel")
        model = ml_model.objects.filter(pk=gie.model_id).first()
        if model is None or not model.engine_path:
            raise AppError("GIE model engine not found", status_code=400)
        config_dir = Path(settings.GENERATOR_CONFIG_ROOT) / row.name
        body = {
            "generator": TypeRegistry.generator_type(row.type),
            "pipeline_name": row.name,
            "config_save_dir": str(config_dir),
            "interval": config.get("interval", 0),
            "pgie": {
                "model_dir": model.engine_path,
                "class_attrs": self.format_class_attrs(gie.class_attrs),
            },
            "tracker": config.get("tracker"),
            "analyzer": self.build_analyzer_payload(config.get("analyzer")),
        }
        if row.type in RTSP_PIPELINE_TYPES:
            body["streams"] = self.build_streams_map(config.get("streams") or [])
        if config.get("input"):
            body["input"] = config["input"]
        if config.get("output"):
            body["output"] = config["output"]
        if config.get("sahi"):
            body["sahi"] = config["sahi"]
        payload = yaml.safe_dump(body, sort_keys=False)
        return payload

    def build_start_yaml(self, row):
        config = row.config or {}
        config_dir = Path(settings.GENERATOR_CONFIG_ROOT) / row.name
        body = {
            "type": row.type,
            "name": row.name,
            "config_dir": str(config_dir),
            "logger": {
                "root": str(Path(settings.DEEPSTREAM_LOG_ROOT) / row.name / "probe"),
                "interval": (config.get("logger") or {}).get("interval", 50),
            },
            "messager": {
                "topic": f"{settings.PROJECT_NAME}_{row.name}",
                "host": f"{settings.PROJECT_NAME}_kafka",
                "port": settings.DEEPSTREAM_KAFKA_PORT,
            },
        }
        if row.type in PARSER_PIPELINE_TYPES:
            parser = config.get("parser")
            if parser is not None:
                body["parser"] = parser
        else:
            drawer = config.get("drawer")
            if drawer is not None:
                body["drawer"] = drawer
        debouncer = config.get("debouncer")
        if debouncer is not None:
            body["debouncer"] = debouncer
        payload = yaml.safe_dump(body, sort_keys=False)
        return payload

    def build_streams_map(self, stream_ids):
        stream_model = apps.get_model("streams", "Stream")
        streams_map = {}
        for stream_id in stream_ids:
            stream = stream_model.objects.filter(pk=stream_id).first()
            if stream is None:
                continue
            width, height = self.parse_resolution(stream.resolution)
            streams_map[stream.name] = {
                "url": stream.url,
                "width": width,
                "height": height,
                "fps": stream.fps or 25,
            }
        return streams_map

    def build_analyzer_payload(self, analyzer_config):
        payload = None
        if analyzer_config:
            template_id = analyzer_config.get("template")
            template = AnalyzerTemplate.objects.filter(pk=template_id).first()
            if template is not None:
                payload = {
                    "template_id": str(template.id),
                    "config_width": template.config_width,
                    "config_height": template.config_height,
                    "annotations": template.annotations or [],
                    "streams": analyzer_config.get("streams") or [],
                    "roi_filtering": analyzer_config.get("roi_filtering"),
                    "overcrowding": analyzer_config.get("overcrowding"),
                    "line_crossing": analyzer_config.get("line_crossing"),
                    "direction_detection": analyzer_config.get("direction_detection"),
                }
        return payload

    def format_class_attrs(self, class_attrs):
        mapping = {}
        size_keys = (
            "detected_min_w",
            "detected_min_h",
            "detected_max_w",
            "detected_max_h",
        )
        for row in class_attrs or []:
            class_key = row.get("class")
            key = class_key
            if str(class_key).lower() == "all":
                key = "all"
            attrs = {}
            conf = row.get("conf")
            topk = row.get("topk")
            if conf is not None:
                attrs["conf"] = conf
            if topk is not None:
                attrs["topk"] = topk
            for size_key in size_keys:
                value = row.get(size_key)
                if value is not None and int(value) >= 0:
                    attrs[size_key] = value
            mapping[key] = attrs
        return mapping

    def parse_resolution(self, resolution):
        width = 1920
        height = 1080
        if resolution and "x" in resolution:
            parts = resolution.lower().split("x", 1)
            if len(parts) == 2 and parts[0].isdigit() and parts[1].isdigit():
                width = int(parts[0])
                height = int(parts[1])
        return width, height
