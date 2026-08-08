from django.db import transaction

from pages.preview.models import DEFAULT_LAYOUT_ID, DEFAULT_LAYOUT_NAME, LayoutPreset
from shared.site_config.registry import site_config_registry


class PreviewSiteConfigSlice:
    SLICE_NAME = "preview"

    def export_slice(self):
        items = [
            {
                "id": str(row.id),
                "name": row.name,
                "layout": row.layout,
                "view_mode": row.view_mode,
                "slots": list(row.slots or []),
            }
            for row in LayoutPreset.objects.all().order_by("name")
        ]
        return {"layouts": items}

    def import_slice(self, payload):
        layouts = list((payload or {}).get("layouts") or [])
        with transaction.atomic():
            keep_ids = set()
            for item in layouts:
                layout_id = item.get("id")
                name = (item.get("name") or "").strip()
                layout = item.get("layout") or "2x2"
                view_mode = item.get("view_mode") or "grid"
                slots = item.get("slots")
                if not layout_id or not name:
                    continue
                if slots is None:
                    slots = [None, None, None, None]
                LayoutPreset.objects.update_or_create(
                    id=layout_id,
                    defaults={
                        "name": name,
                        "layout": layout,
                        "view_mode": view_mode,
                        "slots": slots,
                    },
                )
                keep_ids.add(str(layout_id))
            LayoutPreset.objects.exclude(id=DEFAULT_LAYOUT_ID).exclude(
                id__in=keep_ids
            ).delete()
            LayoutPreset.objects.update_or_create(
                id=DEFAULT_LAYOUT_ID,
                defaults={
                    "name": DEFAULT_LAYOUT_NAME,
                    "layout": "2x2",
                    "view_mode": "grid",
                    "slots": [None, None, None, None],
                },
            )

    def register(self):
        site_config_registry.register(
            self.SLICE_NAME,
            self.export_slice,
            self.import_slice,
        )


def register_preview_site_config():
    PreviewSiteConfigSlice().register()
