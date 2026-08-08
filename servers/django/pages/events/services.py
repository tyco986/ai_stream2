import base64
import csv
import io
import json
import logging
import struct
import uuid
import zipfile
from datetime import datetime, time, timedelta
from pathlib import Path

import pyzipper
from django.conf import settings
from django.db.models import Q
from django.utils import timezone as django_timezone
from django.utils.dateparse import parse_datetime

from pages.events.models import STATUS_ACKED, STATUS_NEW, Event
from shared.http.exceptions import AppError
from shared.pagination import PaginationService

logger = logging.getLogger(__name__)


class EventQueryService:
    def __init__(self):
        self.pagination = PaginationService()

    def list_events(self, filters, page=1, page_size=20):
        queryset = self.filtered_queryset(filters)
        page_data = self.pagination.slice_queryset(queryset, page, page_size)
        items = [self.serialize_list_item(row) for row in page_data["items"]]
        data = self.pagination.build(
            items,
            page_data["total"],
            page_data["page"],
            page_data["page_size"],
        )
        return data

    def get_detail(self, event_id, filters):
        row = self.resolve(event_id)
        neighbors = self.neighbors(row, filters)
        data = self.serialize_detail(row, neighbors["prev_id"], neighbors["next_id"])
        return data

    def options_events(self, pipeline_id=None):
        queryset = Event.objects.all()
        if pipeline_id:
            queryset = queryset.filter(pipeline_id=pipeline_id)
        rows = (
            queryset.order_by("pipeline_name", "pipeline_id", "event_code", "event_label")
            .values("pipeline_id", "pipeline_name", "event_code", "event_label")
            .distinct()
        )
        groups_map = {}
        for row in rows:
            key = str(row["pipeline_id"])
            group = groups_map.get(key)
            if group is None:
                group = {
                    "pipeline_id": key,
                    "pipeline_name": row["pipeline_name"],
                    "items": [],
                }
                groups_map[key] = group
            group["items"].append(
                {"value": row["event_code"], "label": row["event_label"]}
            )
        data = {"groups": list(groups_map.values())}
        return data

    def calendar(self, year, month, filters):
        day_filters = dict(filters)
        day_filters["from_date"] = None
        day_filters["to_date"] = None
        queryset = self.filtered_queryset(day_filters)
        start = datetime(year, month, 1, tzinfo=django_timezone.get_current_timezone())
        if month == 12:
            end = datetime(year + 1, 1, 1, tzinfo=django_timezone.get_current_timezone())
        else:
            end = datetime(
                year, month + 1, 1, tzinfo=django_timezone.get_current_timezone()
            )
        queryset = queryset.filter(occurred_at__gte=start, occurred_at__lt=end)
        dates = sorted(
            {
                django_timezone.localtime(row.occurred_at).strftime("%Y-%m-%d")
                for row in queryset.only("occurred_at")
            }
        )
        data = {"dates": dates}
        return data

    def filtered_queryset(self, filters):
        queryset = Event.objects.all()
        from_date = filters.get("from_date")
        to_date = filters.get("to_date")
        if from_date is not None:
            start = self.day_start(from_date)
            queryset = queryset.filter(occurred_at__gte=start)
        if to_date is not None:
            end = self.day_end_exclusive(to_date)
            queryset = queryset.filter(occurred_at__lt=end)
        stream_id = filters.get("stream_id")
        if stream_id:
            queryset = queryset.filter(stream_id=stream_id)
        pipeline_id = filters.get("pipeline_id")
        if pipeline_id:
            queryset = queryset.filter(pipeline_id=pipeline_id)
        event_code = filters.get("event")
        if event_code:
            queryset = queryset.filter(event_code=event_code)
        status = filters.get("status")
        if status:
            queryset = queryset.filter(status=status)
        search = (filters.get("search") or "").strip()
        if search:
            queryset = queryset.filter(
                Q(stream_name__icontains=search)
                | Q(pipeline_name__icontains=search)
                | Q(event_label__icontains=search)
                | Q(event_code__icontains=search)
            )
        return queryset.order_by("-occurred_at", "-id")

    def neighbors(self, row, filters):
        queryset = self.filtered_queryset(filters)
        newer = (
            queryset.filter(
                Q(occurred_at__gt=row.occurred_at)
                | Q(occurred_at=row.occurred_at, id__gt=row.id)
            )
            .order_by("occurred_at", "id")
            .last()
        )
        older = (
            queryset.filter(
                Q(occurred_at__lt=row.occurred_at)
                | Q(occurred_at=row.occurred_at, id__lt=row.id)
            )
            .order_by("-occurred_at", "-id")
            .first()
        )
        prev_id = str(newer.id) if newer is not None else None
        next_id = str(older.id) if older is not None else None
        data = {"prev_id": prev_id, "next_id": next_id}
        return data

    def parse_filters(self, query_params):
        from_raw = query_params.get("from")
        to_raw = query_params.get("to")
        from_date = self.parse_date(from_raw, "from") if from_raw else None
        to_date = self.parse_date(to_raw, "to") if to_raw else None
        if from_date and to_date and from_date > to_date:
            raise AppError("from must be <= to", status_code=400)
        status = query_params.get("status") or None
        if status and status not in (STATUS_NEW, STATUS_ACKED):
            raise AppError("Invalid status", status_code=400)
        stream_id = query_params.get("stream_id") or None
        pipeline_id = query_params.get("pipeline_id") or None
        if stream_id:
            stream_id = self.parse_uuid(stream_id, "stream_id")
        if pipeline_id:
            pipeline_id = self.parse_uuid(pipeline_id, "pipeline_id")
        filters = {
            "from_date": from_date,
            "to_date": to_date,
            "stream_id": stream_id,
            "pipeline_id": pipeline_id,
            "event": query_params.get("event") or None,
            "status": status,
            "search": query_params.get("search") or None,
        }
        return filters

    def parse_date(self, raw, field_name):
        value = None
        try:
            value = datetime.strptime(raw, "%Y-%m-%d").date()
        except (TypeError, ValueError) as exc:
            raise AppError(f"Invalid {field_name}", status_code=400) from exc
        return value

    def parse_uuid(self, raw, field_name):
        value = None
        try:
            value = uuid.UUID(str(raw))
        except (TypeError, ValueError) as exc:
            raise AppError(f"Invalid {field_name}", status_code=400) from exc
        return value

    def day_start(self, day):
        tz = django_timezone.get_current_timezone()
        return datetime.combine(day, time.min, tzinfo=tz)

    def day_end_exclusive(self, day):
        return self.day_start(day + timedelta(days=1))

    def resolve(self, event_id):
        row = Event.objects.filter(pk=event_id).first()
        if row is None:
            raise AppError("Event not found", status_code=404)
        return row

    def media_url(self, rel_path):
        url = None
        if rel_path:
            url = f"/{settings.PROJECT_NAME}/media/events/{rel_path.lstrip('/')}"
        return url

    def format_dt(self, value):
        formatted = None
        if value is not None:
            formatted = value.strftime("%Y-%m-%dT%H:%M:%SZ")
        return formatted

    def serialize_list_item(self, row):
        return {
            "id": str(row.id),
            "occurred_at": self.format_dt(row.occurred_at),
            "stream_id": str(row.stream_id),
            "stream_name": row.stream_name,
            "pipeline_id": str(row.pipeline_id),
            "pipeline_name": row.pipeline_name,
            "event": row.event_label,
            "status": row.status,
        }

    def serialize_detail(self, row, prev_id, next_id):
        data = self.serialize_list_item(row)
        data["raw_url"] = self.media_url(row.raw_path)
        data["visualization_url"] = self.media_url(row.visualization_path)
        data["prev_id"] = prev_id
        data["next_id"] = next_id
        data["payload"] = row.payload
        data["event_code"] = row.event_code
        return data

    def resolve_media_path(self, rel):
        root = Path(settings.EVENTS_MEDIA_DIR).resolve()
        target = (root / rel).resolve()
        if root not in target.parents and target != root:
            raise AppError("Invalid media path", status_code=400)
        if not target.is_file():
            raise AppError("Media not found", status_code=404)
        return target


class EventAckService:
    def __init__(self):
        self.queries = EventQueryService()

    def ack_one(self, event_id, action):
        row = self.queries.resolve(event_id)
        target = STATUS_ACKED if action == "ack" else STATUS_NEW
        if row.status != target:
            row.status = target
            row.save(update_fields=["status"])
        data = self.queries.serialize_list_item(row)
        return data

    def ack_batch(self, event_ids, action):
        target = STATUS_ACKED if action == "ack" else STATUS_NEW
        source = STATUS_NEW if action == "ack" else STATUS_ACKED
        updated_count = 0
        for event_id in event_ids or []:
            row = Event.objects.filter(pk=event_id).first()
            if row is None:
                continue
            if row.status == source:
                row.status = target
                row.save(update_fields=["status"])
                updated_count += 1
        acked_count = updated_count if action == "ack" else 0
        unacked_count = updated_count if action == "unack" else 0
        data = {
            "updated_count": updated_count,
            "acked_count": acked_count,
            "unacked_count": unacked_count,
        }
        return data


class EventExportService:
    def __init__(self):
        self.queries = EventQueryService()
        self.media_dir = Path(settings.EVENTS_MEDIA_DIR)
        self.export_max = int(settings.EVENTS_EXPORT_MAX)

    def export_zip(self, filters):
        rows = self.load_rows(filters)
        buffer = io.BytesIO()
        with zipfile.ZipFile(buffer, "w", compression=zipfile.ZIP_DEFLATED) as archive:
            self.write_csv(archive, rows)
            self.write_images(archive, rows)
        buffer.seek(0)
        return buffer

    def collect_zip(self, filters, passphrase):
        rows = self.load_rows(filters)
        buffer = io.BytesIO()
        with pyzipper.AESZipFile(
            buffer,
            "w",
            compression=pyzipper.ZIP_DEFLATED,
            encryption=pyzipper.WZ_AES,
        ) as archive:
            archive.setpassword(passphrase.encode("utf-8"))
            self.write_csv(archive, rows)
            self.write_images(archive, rows)
            self.write_labelme(archive, rows)
            self.write_yolo(archive, rows)
        buffer.seek(0)
        return buffer

    def load_rows(self, filters):
        queryset = self.queries.filtered_queryset(filters)
        total = queryset.count()
        if total == 0:
            raise AppError("No events to export", status_code=400)
        if total > self.export_max:
            raise AppError(
                f"Too many events to export (max {self.export_max})",
                status_code=400,
            )
        rows = list(queryset[: self.export_max])
        return rows

    def write_csv(self, archive, rows):
        output = io.StringIO()
        writer = csv.writer(output)
        writer.writerow(
            [
                "id",
                "occurred_at",
                "stream_id",
                "stream_name",
                "pipeline_id",
                "pipeline_name",
                "event",
                "event_code",
                "status",
            ]
        )
        for row in rows:
            writer.writerow(
                [
                    str(row.id),
                    self.queries.format_dt(row.occurred_at),
                    str(row.stream_id),
                    row.stream_name,
                    str(row.pipeline_id),
                    row.pipeline_name,
                    row.event_label,
                    row.event_code,
                    row.status,
                ]
            )
        archive.writestr("events.csv", output.getvalue())

    def write_images(self, archive, rows):
        for row in rows:
            self.add_media_file(archive, "raw", row.id, row.raw_path)
            self.add_media_file(
                archive, "visualization", row.id, row.visualization_path
            )

    def add_media_file(self, archive, folder, event_id, rel_path):
        if not rel_path:
            return
        source = self.media_dir / rel_path
        if not source.is_file():
            return
        suffix = source.suffix or ".jpg"
        archive.write(source, arcname=f"{folder}/{event_id}{suffix}")

    def write_labelme(self, archive, rows):
        for row in rows:
            image_name, image_bytes, width, height = self.pick_image(row)
            if image_bytes is None:
                continue
            archive.writestr(f"labelme/{image_name}", image_bytes)
            shapes = self.labelme_shapes(row)
            payload = {
                "version": "5.0.1",
                "flags": {},
                "shapes": shapes,
                "imagePath": image_name,
                "imageData": None,
                "imageHeight": height,
                "imageWidth": width,
            }
            stem = Path(image_name).stem
            archive.writestr(
                f"labelme/{stem}.json",
                json.dumps(payload, ensure_ascii=False, indent=2),
            )

    def write_yolo(self, archive, rows):
        class_names = self.collect_class_names(rows)
        class_index = {name: idx for idx, name in enumerate(class_names)}
        archive.writestr("yolo/classes.txt", "\n".join(class_names) + ("\n" if class_names else ""))
        for row in rows:
            image_name, image_bytes, width, height = self.pick_image(row)
            if image_bytes is None:
                continue
            archive.writestr(f"yolo/images/{image_name}", image_bytes)
            lines = self.yolo_lines(row, width, height, class_index)
            stem = Path(image_name).stem
            archive.writestr(f"yolo/labels/{stem}.txt", "\n".join(lines) + ("\n" if lines else ""))

    def collect_class_names(self, rows):
        names = []
        seen = set()
        for row in rows:
            for obj in self.objects_of(row):
                label = str(obj[6]) if len(obj) > 6 else str(obj[5])
                if label not in seen:
                    seen.add(label)
                    names.append(label)
        return names

    def objects_of(self, row):
        payload = row.payload or {}
        objects = payload.get("objects") or []
        return objects

    def labelme_shapes(self, row):
        shapes = [
            {
                "label": str(obj[6]) if len(obj) > 6 else "object",
                "points": [
                    [float(obj[0]), float(obj[1])],
                    [float(obj[2]), float(obj[3])],
                ],
                "group_id": None,
                "shape_type": "rectangle",
                "flags": {},
            }
            for obj in self.objects_of(row)
            if len(obj) >= 4
        ]
        return shapes

    def yolo_lines(self, row, width, height, class_index):
        lines = []
        if width <= 0 or height <= 0:
            return lines
        for obj in self.objects_of(row):
            if len(obj) < 4:
                continue
            x1, y1, x2, y2 = [float(obj[0]), float(obj[1]), float(obj[2]), float(obj[3])]
            label = str(obj[6]) if len(obj) > 6 else str(obj[5]) if len(obj) > 5 else "object"
            class_id = class_index.get(label, 0)
            cx = ((x1 + x2) / 2.0) / width
            cy = ((y1 + y2) / 2.0) / height
            bw = abs(x2 - x1) / width
            bh = abs(y2 - y1) / height
            lines.append(f"{class_id} {cx:.6f} {cy:.6f} {bw:.6f} {bh:.6f}")
        return lines

    def pick_image(self, row):
        image_name = None
        image_bytes = None
        width = 1920
        height = 1080
        rel_path = row.raw_path or row.visualization_path
        if rel_path:
            source = self.media_dir / rel_path
            if source.is_file():
                image_bytes = source.read_bytes()
                suffix = source.suffix or ".jpg"
                image_name = f"{row.id}{suffix}"
                width, height = self.read_image_size(source)
        return image_name, image_bytes, width, height

    def read_image_size(self, path):
        width = 1920
        height = 1080
        data = path.read_bytes()
        if len(data) >= 24 and data[:8] == b"\x89PNG\r\n\x1a\n":
            width, height = struct.unpack(">II", data[16:24])
        elif len(data) >= 2 and data[:2] == b"\xff\xd8":
            offset = 2
            while offset + 9 < len(data):
                if data[offset] != 0xFF:
                    break
                marker = data[offset + 1]
                if marker in (0xC0, 0xC1, 0xC2):
                    height, width = struct.unpack(">HH", data[offset + 5 : offset + 9])
                    break
                length = struct.unpack(">H", data[offset + 2 : offset + 4])[0]
                offset += 2 + length
        return width, height


class EventIngestService:
    def __init__(self):
        self.media_dir = Path(settings.EVENTS_MEDIA_DIR)

    def ingest(self, envelope):
        if not isinstance(envelope, dict):
            raise AppError("Invalid event envelope", status_code=400)
        occurred_at = self.parse_occurred_at(envelope.get("occurred_at"))
        stream_id = self.require_uuid(envelope.get("stream_id"), "stream_id")
        pipeline_id = self.require_uuid(envelope.get("pipeline_id"), "pipeline_id")
        stream_name = (envelope.get("stream_name") or "").strip()
        pipeline_name = (envelope.get("pipeline_name") or "").strip()
        event_code = str(envelope.get("event_code") or "").strip()
        event_label = (envelope.get("event_label") or "").strip()
        if not stream_name:
            raise AppError("stream_name is required", status_code=400)
        if not pipeline_name:
            raise AppError("pipeline_name is required", status_code=400)
        if not event_code:
            raise AppError("event_code is required", status_code=400)
        if not event_label:
            raise AppError("event_label is required", status_code=400)
        event_id = uuid.uuid4()
        raw_path = self.write_image(
            event_id, "raw", envelope.get("raw_image_b64")
        )
        visualization_path = self.write_image(
            event_id, "visualization", envelope.get("visualization_image_b64")
        )
        payload = {
            "objects": envelope.get("objects") or [],
        }
        row = Event.objects.create(
            id=event_id,
            occurred_at=occurred_at,
            stream_id=stream_id,
            stream_name=stream_name,
            pipeline_id=pipeline_id,
            pipeline_name=pipeline_name,
            event_code=event_code,
            event_label=event_label,
            status=STATUS_NEW,
            raw_path=raw_path,
            visualization_path=visualization_path,
            payload=payload,
        )
        return row

    def try_ingest_message(self, raw_bytes):
        ingested = False
        try:
            payload = json.loads(raw_bytes.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError):
            logger.warning("events consumer skipped non-json message")
            return ingested
        if not isinstance(payload, dict):
            logger.warning("events consumer skipped non-envelope message")
            return ingested
        required = (
            "stream_id",
            "pipeline_id",
            "stream_name",
            "pipeline_name",
            "event_code",
            "event_label",
        )
        missing = [key for key in required if not payload.get(key)]
        if missing:
            logger.warning("events consumer skipped incomplete envelope: %s", missing)
            return ingested
        self.ingest(payload)
        ingested = True
        return ingested

    def parse_occurred_at(self, raw):
        occurred_at = django_timezone.now()
        if raw:
            parsed = parse_datetime(str(raw))
            if parsed is None:
                raise AppError("Invalid occurred_at", status_code=400)
            if django_timezone.is_naive(parsed):
                parsed = django_timezone.make_aware(
                    parsed, django_timezone.get_current_timezone()
                )
            occurred_at = parsed
        return occurred_at

    def require_uuid(self, raw, field_name):
        value = None
        try:
            value = uuid.UUID(str(raw))
        except (TypeError, ValueError) as exc:
            raise AppError(f"Invalid {field_name}", status_code=400) from exc
        return value

    def write_image(self, event_id, kind, b64_data):
        rel_path = None
        if not b64_data:
            return rel_path
        raw = base64.b64decode(b64_data)
        suffix = ".jpg"
        if raw[:8] == b"\x89PNG\r\n\x1a\n":
            suffix = ".png"
        target_dir = self.media_dir / str(event_id)
        target_dir.mkdir(parents=True, exist_ok=True)
        filename = f"{kind}{suffix}"
        target = target_dir / filename
        target.write_bytes(raw)
        rel_path = f"{event_id}/{filename}"
        return rel_path
