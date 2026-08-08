from django.db import transaction

from pages.streams.models import ALL_GROUP_ID, Group, Stream
from pages.streams.services import StreamService
from shared.site_config.registry import site_config_registry


class StreamsSiteConfigSlice:
    SLICE_NAME = "streams"

    def __init__(self):
        self.streams = StreamService()

    def export_slice(self):
        groups = [
            {
                "id": str(row.id),
                "name": row.name,
                "parent_id": str(row.parent_id) if row.parent_id else None,
            }
            for row in Group.objects.all().order_by("name")
        ]
        streams = [
            {
                "id": str(row.id),
                "name": row.name,
                "group_id": str(row.group_id),
                "url": row.url or "",
                "resolution": row.resolution,
                "fps": row.fps,
                "status": row.status,
                "enabled": bool(row.enabled),
                "recording": bool(row.recording),
                "last_probe_at": (
                    row.last_probe_at.isoformat() if row.last_probe_at else None
                ),
            }
            for row in Stream.objects.all().order_by("name")
        ]
        return {"groups": groups, "streams": streams}

    def import_slice(self, payload):
        groups_payload = list((payload or {}).get("groups") or [])
        streams_payload = list((payload or {}).get("streams") or [])
        with transaction.atomic():
            keep_group_ids = set()
            for item in groups_payload:
                group_id = item.get("id")
                name = (item.get("name") or "").strip()
                if not group_id or not name:
                    continue
                parent_id = item.get("parent_id")
                group, _created = Group.objects.update_or_create(
                    id=group_id,
                    defaults={"name": name, "parent_id": parent_id},
                )
                keep_group_ids.add(str(group.id))
            Group.objects.exclude(id=ALL_GROUP_ID).exclude(
                id__in=keep_group_ids
            ).delete()
            Group.objects.update_or_create(
                id=ALL_GROUP_ID,
                defaults={"name": "All", "parent_id": None},
            )
            keep_stream_ids = set()
            for item in streams_payload:
                stream_id = item.get("id")
                name = (item.get("name") or "").strip()
                cleaned_url = self.streams.normalize_url(item.get("url"))
                group_id = item.get("group_id") or str(ALL_GROUP_ID)
                if not stream_id or not name:
                    continue
                if not Group.objects.filter(pk=group_id).exists():
                    group_id = ALL_GROUP_ID
                stream, _created = Stream.objects.update_or_create(
                    id=stream_id,
                    defaults={
                        "name": name,
                        "url": cleaned_url,
                        "group_id": group_id,
                        "resolution": item.get("resolution"),
                        "fps": item.get("fps"),
                        "status": item.get("status") or "offline",
                        "enabled": bool(item.get("enabled", True)),
                        "recording": bool(item.get("recording", False)),
                        "last_probe_at": None,
                    },
                )
                keep_stream_ids.add(str(stream.id))
            for stream in Stream.objects.exclude(id__in=keep_stream_ids):
                self.streams.mediamtx.delete_path(stream.name)
                stream.delete()
            for stream in Stream.objects.all():
                self.streams.publish_or_unpublish(stream)

    def register(self):
        site_config_registry.register(
            self.SLICE_NAME,
            self.export_slice,
            self.import_slice,
        )


def register_streams_site_config():
    StreamsSiteConfigSlice().register()
