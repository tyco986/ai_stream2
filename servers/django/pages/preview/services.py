import uuid
from pathlib import Path

from django.apps import apps
from django.conf import settings
from django.db import transaction

from pages.preview.models import (
    DEFAULT_LAYOUT_ID,
    DEFAULT_LAYOUT_NAME,
    LAYOUT_SLOT_COUNT,
    ActiveLayout,
    LayoutPreset,
)
from shared.http.exceptions import AppError


class ActiveLayoutService:
    def get_preset_id(self, user):
        row = ActiveLayout.objects.filter(user=user).first()
        preset_id = DEFAULT_LAYOUT_ID
        if row is not None:
            preset_id = row.preset_id
            if not LayoutPreset.objects.filter(pk=preset_id).exists():
                preset_id = DEFAULT_LAYOUT_ID
        return {"preset_id": str(preset_id)}

    def set_preset_id(self, user, preset_id):
        if not LayoutPreset.objects.filter(pk=preset_id).exists():
            raise AppError("Layout preset not found", status_code=404)
        ActiveLayout.objects.update_or_create(
            user=user,
            defaults={"preset_id": preset_id},
        )
        return {"preset_id": str(preset_id)}


class LayoutPresetService:
    def __init__(self):
        self.active = ActiveLayoutService()

    def list_presets(self, user, search=None):
        queryset = LayoutPreset.objects.all().order_by("name")
        if search:
            queryset = queryset.filter(name__icontains=search.strip())
        items = [self.serialize_summary(row) for row in queryset]
        active = self.active.get_preset_id(user)
        return {"items": items, "preset_id": active["preset_id"]}

    def layouts_map(self):
        return {
            row.name: str(row.id) for row in LayoutPreset.objects.all().order_by("name")
        }

    def get(self, preset_id):
        preset = self.resolve(preset_id)
        return self.serialize(preset, soft_clean=True)

    def create(self, name, layout, view_mode, slots):
        cleaned_name = (name or "").strip()
        if not cleaned_name:
            raise AppError("name is required", status_code=400)
        if cleaned_name == DEFAULT_LAYOUT_NAME:
            raise AppError("Cannot use reserved name Default", status_code=400)
        if LayoutPreset.objects.filter(name=cleaned_name).exists():
            raise AppError("Name already exists", status_code=409)
        normalized_slots = self.validate_slots(layout, slots)
        preset = LayoutPreset.objects.create(
            name=cleaned_name,
            layout=layout,
            view_mode=view_mode,
            slots=normalized_slots,
        )
        return self.serialize(preset)

    def patch(self, preset_id, name=None, layout=None, view_mode=None, slots=None):
        preset = self.resolve(preset_id)
        if name is not None:
            cleaned_name = name.strip()
            if not cleaned_name:
                raise AppError("name is required", status_code=400)
            if preset.id == DEFAULT_LAYOUT_ID:
                raise AppError("Cannot modify Default name", status_code=403)
            if cleaned_name == DEFAULT_LAYOUT_NAME:
                raise AppError("Cannot use reserved name Default", status_code=400)
            if (
                LayoutPreset.objects.filter(name=cleaned_name)
                .exclude(pk=preset.pk)
                .exists()
            ):
                raise AppError("Name already exists", status_code=409)
            preset.name = cleaned_name
        next_layout = layout if layout is not None else preset.layout
        if layout is not None:
            preset.layout = layout
        if view_mode is not None:
            preset.view_mode = view_mode
        if slots is not None or layout is not None:
            source_slots = slots if slots is not None else preset.slots
            preset.slots = self.validate_slots(next_layout, source_slots)
        preset.save()
        return self.serialize(preset)

    def delete(self, user, preset_id):
        preset = self.resolve(preset_id)
        if preset.id == DEFAULT_LAYOUT_ID:
            raise AppError("Cannot delete Default", status_code=403)
        active = self.active.get_preset_id(user)
        was_active = active["preset_id"] == str(preset.id)
        self.clear_shot_file(preset)
        deleted_id = str(preset.id)
        preset.delete()
        return {"deleted_id": deleted_id, "was_active": was_active}

    def batch_delete(self, user, ids):
        id_list = [str(item) for item in (ids or [])]
        if str(DEFAULT_LAYOUT_ID) in id_list:
            raise AppError("Cannot delete Default", status_code=403)
        active = self.active.get_preset_id(user)
        active_deleted = False
        deleted_ids = []
        with transaction.atomic():
            for preset_id in id_list:
                preset = LayoutPreset.objects.filter(pk=preset_id).first()
                if preset is None:
                    continue
                if str(preset.id) == active["preset_id"]:
                    active_deleted = True
                self.clear_shot_file(preset)
                deleted_ids.append(str(preset.id))
                preset.delete()
        return {"deleted_ids": deleted_ids, "active_deleted": active_deleted}

    def validate_slots(self, layout, slots):
        expected = LAYOUT_SLOT_COUNT.get(layout)
        if expected is None:
            raise AppError("Invalid layout", status_code=400)
        if not isinstance(slots, list):
            raise AppError("slots must be a list", status_code=400)
        if len(slots) != expected:
            raise AppError(
                f"slots length must be {expected} for layout {layout}",
                status_code=400,
            )
        seen = set()
        normalized = []
        stream_model = apps.get_model("streams", "Stream")
        for item in slots:
            if item is None:
                normalized.append(None)
                continue
            try:
                stream_id = uuid.UUID(str(item))
            except (TypeError, ValueError) as exc:
                raise AppError("Invalid stream id in slots", status_code=400) from exc
            key = str(stream_id)
            if key in seen:
                raise AppError("Duplicate stream id in slots", status_code=400)
            seen.add(key)
            if not stream_model.objects.filter(pk=stream_id).exists():
                raise AppError(f"Stream not found: {key}", status_code=400)
            normalized.append(key)
        return normalized

    def soft_clean_slots(self, slots):
        stream_model = apps.get_model("streams", "Stream")
        missing = []
        cleaned = []
        for item in slots or []:
            if item is None:
                cleaned.append(None)
                continue
            key = str(item)
            if stream_model.objects.filter(pk=key).exists():
                cleaned.append(key)
            else:
                missing.append(key)
                cleaned.append(None)
        return cleaned, missing

    def resolve(self, preset_id):
        preset = LayoutPreset.objects.filter(pk=preset_id).first()
        if preset is None:
            raise AppError("Layout preset not found", status_code=404)
        return preset

    def clear_shot_file(self, preset):
        if not preset.shot_path:
            return
        path = Path(settings.PREVIEW_SHOT_DIR) / preset.shot_path
        if path.is_file():
            path.unlink()

    def shot_url(self, preset):
        url = None
        if preset.shot_path:
            url = (
                f"/{settings.PROJECT_NAME}/backend/preview/layouts/{preset.id}/shot"
            )
        return url

    def serialize_summary(self, preset):
        stream_count = sum(1 for item in (preset.slots or []) if item is not None)
        return {
            "id": str(preset.id),
            "name": preset.name,
            "layout": preset.layout,
            "stream_count": stream_count,
        }

    def serialize(self, preset, soft_clean=False):
        slots = list(preset.slots or [])
        missing = []
        if soft_clean:
            slots, missing = self.soft_clean_slots(slots)
        stream_count = sum(1 for item in slots if item is not None)
        data = {
            "id": str(preset.id),
            "name": preset.name,
            "layout": preset.layout,
            "view_mode": preset.view_mode,
            "slots": slots,
            "stream_count": stream_count,
            "shot_url": self.shot_url(preset),
        }
        if missing:
            data["missing_stream_ids"] = missing
        return data


class ShotService:
    def __init__(self):
        self.presets = LayoutPresetService()
        self.shot_dir = Path(settings.PREVIEW_SHOT_DIR)

    def put_shot(self, preset_id, upload):
        preset = self.presets.resolve(preset_id)
        if upload is None:
            raise AppError("Missing file", status_code=400)
        self.shot_dir.mkdir(parents=True, exist_ok=True)
        relative = f"{preset.id}.jpg"
        target = self.shot_dir / relative
        with target.open("wb") as handle:
            for chunk in upload.chunks():
                handle.write(chunk)
        preset.shot_path = relative
        preset.save(update_fields=["shot_path"])
        return self.presets.serialize(preset)

    def get_shot_path(self, preset_id):
        preset = self.presets.resolve(preset_id)
        path = None
        if preset.shot_path:
            candidate = self.shot_dir / preset.shot_path
            if candidate.is_file():
                path = candidate
        if path is None:
            raise AppError("Shot not found", status_code=404)
        return path
