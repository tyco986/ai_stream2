from datetime import datetime, timezone
from pathlib import Path

from django.conf import settings
from django.db import transaction
from django.db.models import Q
from django.utils import timezone as django_timezone

from pages.streams.clients import FFmpegClient, MediaMTXClient
from pages.streams.models import (
    ALL_GROUP_ID,
    STREAM_STATUS_OFFLINE,
    STREAM_STATUS_ONLINE,
    Group,
    Stream,
)
from shared.http.exceptions import AppError
from shared.pagination import PaginationService


class StreamLogService:
    def __init__(self, log_dir=None):
        self.log_dir = Path(
            log_dir if log_dir is not None else settings.STREAM_LOG_DIR
        )

    def append(self, stream_id, message):
        self.log_dir.mkdir(parents=True, exist_ok=True)
        path = self.log_dir / f"{stream_id}.log"
        stamp = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S")
        with path.open("a", encoding="utf-8") as handle:
            handle.write(f"{stamp} {message}\n")

    def get_logs(self, stream_id):
        path = self.log_dir / f"{stream_id}.log"
        content = ""
        if path.is_file():
            content = path.read_text(encoding="utf-8")
        return {"content": content}

    def clear(self, stream_id):
        path = self.log_dir / f"{stream_id}.log"
        if path.is_file():
            path.unlink()


class GroupService:
    def build_tree(self):
        root = self.resolve(ALL_GROUP_ID)
        groups = list(Group.objects.all().order_by("name"))
        streams = list(Stream.objects.select_related("group").order_by("name"))
        children_by_parent = {}
        for group in groups:
            parent_id = str(group.parent_id) if group.parent_id else None
            children_by_parent.setdefault(parent_id, []).append(group)
        streams_by_group = {}
        for stream in streams:
            streams_by_group.setdefault(str(stream.group_id), []).append(stream)
        return {
            "root": self.build_group_node(root, children_by_parent, streams_by_group)
        }

    def build_group_node(self, group, children_by_parent, streams_by_group):
        child_nodes = [
            self.build_group_node(child, children_by_parent, streams_by_group)
            for child in children_by_parent.get(str(group.id), [])
        ]
        child_nodes.extend(
            [
                {
                    "id": str(stream.id),
                    "name": stream.name,
                    "type": "stream",
                    "enabled": bool(stream.enabled),
                    "status": stream.status,
                    "recording": bool(stream.recording),
                }
                for stream in streams_by_group.get(str(group.id), [])
            ]
        )
        return {
            "id": str(group.id),
            "name": group.name,
            "type": "group",
            "children": child_nodes,
        }

    def groups_map(self):
        return {row.name: str(row.id) for row in Group.objects.all().order_by("name")}

    def list_groups(self):
        items = [
            {
                "id": str(row.id),
                "name": row.name,
                "parent_id": str(row.parent_id) if row.parent_id else None,
            }
            for row in Group.objects.all().order_by("name")
        ]
        return {"items": items}

    def create(self, parent_group_id, name):
        parent = self.resolve(parent_group_id)
        cleaned = (name or "").strip()
        if not cleaned:
            raise AppError("name is required", status_code=400)
        if Group.objects.filter(name=cleaned).exists():
            raise AppError("Group name already exists", status_code=400)
        group = Group.objects.create(name=cleaned, parent=parent)
        return self.serialize(group)

    def rename(self, group_id, name):
        group = self.resolve(group_id)
        if group.id == ALL_GROUP_ID:
            raise AppError("Cannot rename All group", status_code=400)
        cleaned = (name or "").strip()
        if not cleaned:
            raise AppError("name is required", status_code=400)
        if Group.objects.filter(name=cleaned).exclude(pk=group.pk).exists():
            raise AppError("Group name already exists", status_code=400)
        group.name = cleaned
        group.save(update_fields=["name"])
        return self.serialize(group)

    def delete(self, group_id):
        group = self.resolve(group_id)
        if group.id == ALL_GROUP_ID:
            raise AppError("Cannot delete All group", status_code=400)
        subtree_ids = self.collect_subtree_ids(group.id)
        all_group = self.resolve(ALL_GROUP_ID)
        with transaction.atomic():
            Stream.objects.filter(group_id__in=subtree_ids).update(group=all_group)
            deleted_count, _details = Group.objects.filter(id__in=subtree_ids).delete()
        return {
            "deleted_group_ids": [str(item) for item in subtree_ids],
            "deleted_count": deleted_count,
        }

    def members(self, group_id):
        group = self.resolve(group_id)
        items = [
            self.serialize_summary(row)
            for row in Stream.objects.filter(group=group)
            .select_related("group")
            .order_by("name")
        ]
        return {
            "group_id": str(group.id),
            "group_name": group.name,
            "items": items,
        }

    def candidates(self, group_id, search=None):
        group = self.resolve(group_id)
        queryset = (
            Stream.objects.exclude(group=group)
            .select_related("group")
            .order_by("name")
        )
        if search:
            queryset = queryset.filter(name__icontains=search.strip())
        return {"items": [self.serialize_summary(row) for row in queryset]}

    def set_members(self, group_id, stream_ids):
        group = self.resolve(group_id)
        all_group = self.resolve(ALL_GROUP_ID)
        wanted = [str(item) for item in (stream_ids or [])]
        with transaction.atomic():
            current_ids = set(
                str(item)
                for item in Stream.objects.filter(group=group).values_list(
                    "id", flat=True
                )
            )
            wanted_set = set(wanted)
            to_add = wanted_set - current_ids
            to_remove = current_ids - wanted_set
            if to_add:
                found = Stream.objects.filter(id__in=to_add)
                if found.count() != len(to_add):
                    raise AppError("Invalid stream_ids", status_code=400)
                found.update(group=group)
            if to_remove:
                Stream.objects.filter(id__in=to_remove, group=group).update(
                    group=all_group
                )
        return {
            "updated_count": len(to_add),
            "removed_count": len(to_remove),
        }

    def collect_subtree_ids(self, root_id):
        ids = [root_id]
        queue = [root_id]
        while queue:
            parent_id = queue.pop(0)
            children = list(
                Group.objects.filter(parent_id=parent_id).values_list("id", flat=True)
            )
            ids.extend(children)
            queue.extend(children)
        return ids

    def resolve(self, group_id):
        group = Group.objects.filter(pk=group_id).first()
        if group is None:
            raise AppError("Group not found", status_code=404)
        return group

    def serialize(self, group):
        return {
            "id": str(group.id),
            "name": group.name,
            "parent_id": str(group.parent_id) if group.parent_id else None,
        }

    def serialize_summary(self, stream):
        return {
            "id": str(stream.id),
            "name": stream.name,
            "group_id": str(stream.group_id),
            "group_name": stream.group.name,
        }


class StreamService:
    def __init__(self):
        self.groups = GroupService()
        self.mediamtx = MediaMTXClient()
        self.logs = StreamLogService()
        self.pagination = PaginationService()

    def normalize_url(self, url):
        return (url or "").strip() or None

    def publish_or_unpublish(self, stream, old_name=None):
        if old_name and old_name != stream.name:
            self.mediamtx.delete_path(old_name)
        if stream.enabled and self.normalize_url(stream.url):
            self.mediamtx.upsert_path(stream.name, stream.url, stream.recording)
        else:
            self.mediamtx.delete_path(stream.name)

    def list_streams(self, group_id=None, stream_id=None, search=None, page=1, page_size=20):
        queryset = Stream.objects.select_related("group").order_by("name")
        if stream_id:
            queryset = queryset.filter(pk=stream_id)
        else:
            target_group_id = group_id or ALL_GROUP_ID
            subtree = self.groups.collect_subtree_ids(target_group_id)
            queryset = queryset.filter(group_id__in=subtree)
        if search:
            term = search.strip()
            queryset = queryset.filter(
                Q(name__icontains=term)
                | Q(url__icontains=term)
                | Q(group__name__icontains=term)
            )
        page_data = self.pagination.slice_queryset(queryset, page, page_size)
        items = [self.serialize(row) for row in page_data["items"]]
        return self.pagination.build(
            items,
            page_data["total"],
            page_data["page"],
            page_data["page_size"],
        )

    def streams_map(self):
        return {
            row.name: str(row.id) for row in Stream.objects.all().order_by("name")
        }

    def get(self, stream_id):
        stream = self.resolve(stream_id)
        return self.serialize(stream)

    def apply_probe_snapshot(self, stream, resolution, fps):
        stream.resolution = resolution
        stream.fps = fps
        stream.status = STREAM_STATUS_ONLINE
        stream.last_probe_at = django_timezone.now()
        stream.save(
            update_fields=["resolution", "fps", "status", "last_probe_at"]
        )

    def create(
        self,
        name,
        url,
        group_id=None,
        enabled=True,
        recording=False,
        resolution=None,
        fps=None,
    ):
        cleaned_name = (name or "").strip()
        cleaned_url = self.normalize_url(url)
        if not cleaned_name:
            raise AppError("name is required", status_code=400)
        if Stream.objects.filter(name=cleaned_name).exists():
            raise AppError("Stream name already exists", status_code=400)
        if cleaned_url and Stream.objects.filter(url=cleaned_url).exists():
            raise AppError("Stream URL already exists", status_code=400)
        group = self.groups.resolve(group_id or ALL_GROUP_ID)
        with transaction.atomic():
            stream = Stream.objects.create(
                name=cleaned_name,
                url=cleaned_url,
                group=group,
                enabled=bool(enabled),
                recording=bool(recording),
                status=STREAM_STATUS_OFFLINE,
            )
            if resolution is not None or fps is not None:
                self.apply_probe_snapshot(stream, resolution, fps)
                self.logs.append(stream.id, "INFO create with probe snapshot")
            self.publish_or_unpublish(stream)
            self.logs.append(stream.id, f"INFO create enabled={stream.enabled}")
        return self.serialize(stream)

    def patch(
        self,
        stream_id,
        name=None,
        group_id=None,
        url=None,
        enabled=None,
        recording=None,
        resolution=None,
        fps=None,
    ):
        stream = self.resolve(stream_id)
        old_name = stream.name
        with transaction.atomic():
            if name is not None:
                cleaned = name.strip()
                if not cleaned:
                    raise AppError("name is required", status_code=400)
                if Stream.objects.filter(name=cleaned).exclude(pk=stream.pk).exists():
                    raise AppError("Stream name already exists", status_code=400)
                stream.name = cleaned
            if group_id is not None:
                stream.group = self.groups.resolve(group_id)
            if url is not None:
                cleaned_url = self.normalize_url(url)
                if cleaned_url and (
                    Stream.objects.filter(url=cleaned_url)
                    .exclude(pk=stream.pk)
                    .exists()
                ):
                    raise AppError("Stream URL already exists", status_code=400)
                stream.url = cleaned_url
            if enabled is not None:
                stream.enabled = bool(enabled)
            if recording is not None:
                stream.recording = bool(recording)
            stream.save()
            if resolution is not None or fps is not None:
                self.apply_probe_snapshot(stream, resolution, fps)
                self.logs.append(stream.id, "INFO patch with probe snapshot")
            self.publish_or_unpublish(stream, old_name=old_name)
            self.logs.append(stream.id, "INFO patch")
        return self.serialize(stream)

    def delete(self, stream_id):
        stream = self.resolve(stream_id)
        with transaction.atomic():
            self.mediamtx.delete_path(stream.name)
            stream_id_value = stream.id
            stream.delete()
            self.logs.clear(stream_id_value)

    def batch_delete(self, ids):
        deleted_count = 0
        for stream_id in ids or []:
            stream = Stream.objects.filter(pk=stream_id).first()
            if stream is None:
                continue
            self.delete(stream.id)
            deleted_count += 1
        return {"deleted_count": deleted_count}

    def batch_enable(self, ids):
        updated_count = 0
        for stream_id in ids or []:
            stream = Stream.objects.filter(pk=stream_id).select_related("group").first()
            if stream is None or stream.enabled:
                continue
            with transaction.atomic():
                stream.enabled = True
                stream.save(update_fields=["enabled"])
                self.publish_or_unpublish(stream)
                self.logs.append(stream.id, "INFO enable")
            updated_count += 1
        return {"updated_count": updated_count}

    def batch_disable(self, ids):
        updated_count = 0
        for stream_id in ids or []:
            stream = Stream.objects.filter(pk=stream_id).first()
            if stream is None or not stream.enabled:
                continue
            with transaction.atomic():
                stream.enabled = False
                stream.save(update_fields=["enabled"])
                self.publish_or_unpublish(stream)
                self.logs.append(stream.id, "INFO disable")
            updated_count += 1
        return {"updated_count": updated_count}

    def batch_record(self, ids):
        updated_count = 0
        for stream_id in ids or []:
            stream = Stream.objects.filter(pk=stream_id).select_related("group").first()
            if stream is None or not stream.enabled or stream.recording:
                continue
            with transaction.atomic():
                stream.recording = True
                stream.save(update_fields=["recording"])
                self.publish_or_unpublish(stream)
                self.logs.append(stream.id, "INFO record")
            updated_count += 1
        return {"updated_count": updated_count}

    def batch_unrecord(self, ids):
        updated_count = 0
        for stream_id in ids or []:
            stream = Stream.objects.filter(pk=stream_id).select_related("group").first()
            if stream is None or not stream.recording:
                continue
            with transaction.atomic():
                stream.recording = False
                stream.save(update_fields=["recording"])
                self.publish_or_unpublish(stream)
                self.logs.append(stream.id, "INFO unrecord")
            updated_count += 1
        return {"updated_count": updated_count}

    def resolve(self, stream_id):
        stream = Stream.objects.filter(pk=stream_id).select_related("group").first()
        if stream is None:
            raise AppError("Stream not found", status_code=404)
        return stream

    def serialize(self, stream):
        return {
            "id": str(stream.id),
            "name": stream.name,
            "group_id": str(stream.group_id),
            "group_name": stream.group.name,
            "url": stream.url or "",
            "resolution": stream.resolution,
            "fps": stream.fps,
            "status": stream.status,
            "enabled": bool(stream.enabled),
            "recording": bool(stream.recording),
            "last_probe_at": (
                stream.last_probe_at.strftime("%Y-%m-%dT%H:%M:%SZ")
                if stream.last_probe_at is not None
                else None
            ),
        }


class StreamProbeService:
    def __init__(self):
        self.ffmpeg = FFmpegClient()
        self.mediamtx = MediaMTXClient()
        self.logs = StreamLogService()

    def probe(self, url):
        cleaned = (url or "").strip()
        result = {
            "success": False,
            "error": "url is required",
            "resolution": None,
            "fps": None,
        }
        if cleaned:
            try:
                result = self.ffmpeg.probe(cleaned)
            except AppError as exc:
                result = {
                    "success": False,
                    "error": str(exc.detail),
                    "resolution": None,
                    "fps": None,
                }
        return result

    def probe_url(self, url):
        probe = self.probe(url)
        result = {
            "id": "",
            "success": False,
            "error": probe.get("error") or "probe failed",
        }
        if probe.get("success"):
            result = {
                "id": "",
                "success": True,
                "resolution": probe.get("resolution"),
                "fps": probe.get("fps"),
            }
        return result

    def resolve_mount(self, stream_name, paths_index=None):
        mount = {
            "reachable": False,
            "enabled": False,
            "recording": False,
            "detail": "",
        }
        if paths_index is not None:
            mount["reachable"] = bool(paths_index.get("reachable"))
            mount["detail"] = paths_index.get("detail") or ""
            entry = (paths_index.get("paths") or {}).get(stream_name)
            if entry is not None:
                mount["enabled"] = True
                mount["recording"] = bool(entry.get("recording"))
        else:
            mount = self.mediamtx.inspect_path(stream_name)
        return mount

    def sync_mount_fields(self, stream, mount):
        stream.enabled = bool(mount["enabled"])
        stream.recording = bool(mount["recording"])
        stream.save(update_fields=["enabled", "recording"])
        if not mount["reachable"]:
            self.logs.append(
                stream.id,
                f"WARN mediamtx unreachable; enabled/recording cleared "
                f"({mount.get('detail') or 'offline'})",
            )
        else:
            self.logs.append(
                stream.id,
                f"INFO mediamtx mount enabled={stream.enabled} "
                f"recording={stream.recording}",
            )

    def probe_one(self, stream_id):
        stream = Stream.objects.filter(pk=stream_id).first()
        if stream is None:
            raise AppError("Stream not found", status_code=404)
        self.sync_mount_fields(stream, self.resolve_mount(stream.name))
        return self.apply_rtsp_result(stream, self.probe(stream.url))

    def probe_many(self, ids):
        results = []
        paths_index = self.mediamtx.list_paths_index()
        for stream_id in ids or []:
            stream = Stream.objects.filter(pk=stream_id).first()
            if stream is None:
                results.append(
                    {
                        "id": str(stream_id),
                        "success": False,
                        "error": "Stream not found",
                        "enabled": False,
                        "recording": False,
                    }
                )
                continue
            self.sync_mount_fields(
                stream,
                self.resolve_mount(stream.name, paths_index=paths_index),
            )
            results.append(self.apply_rtsp_result(stream, self.probe(stream.url)))
        return {"results": results}

    def apply_rtsp_result(self, stream, probe):
        result = {
            "id": str(stream.id),
            "success": False,
            "error": probe.get("error") or "probe failed",
            "enabled": bool(stream.enabled),
            "recording": bool(stream.recording),
            "status": stream.status,
        }
        if probe.get("success"):
            StreamService().apply_probe_snapshot(
                stream, probe.get("resolution"), probe.get("fps")
            )
            self.logs.append(stream.id, "INFO probe success")
            result = {
                "id": str(stream.id),
                "success": True,
                "resolution": stream.resolution,
                "fps": stream.fps,
                "status": stream.status,
                "last_probe_at": stream.last_probe_at.strftime("%Y-%m-%dT%H:%M:%SZ"),
                "enabled": bool(stream.enabled),
                "recording": bool(stream.recording),
            }
        else:
            stream.status = STREAM_STATUS_OFFLINE
            stream.save(update_fields=["status"])
            self.logs.append(
                stream.id,
                f"ERROR probe failed: {probe.get('error') or 'probe failed'}",
            )
            result["status"] = stream.status
            result["error"] = probe.get("error") or "probe failed"
        return result


class StreamPublisherService:
    def __init__(self):
        self.ffmpeg = FFmpegClient()

    def publish(self, upload, name=None):
        cleaned_name = (name or "").strip() or None
        if upload is None:
            raise AppError("input is required", status_code=400)
        return self.ffmpeg.publish(upload, cleaned_name)
