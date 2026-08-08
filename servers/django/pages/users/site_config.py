from django.contrib.auth.models import Group
from django.db import transaction

from pages.users.models import GroupUuid
from pages.users.services import GroupService
from shared.site_config.registry import site_config_registry


class UsersSiteConfigSlice:
    SLICE_NAME = "users_groups"

    def export_slice(self):
        service = GroupService()
        items = []
        for group in Group.objects.all().order_by("name").prefetch_related("permissions"):
            mapping = service.ensure_uuid(group)
            items.append(
                {
                    "id": str(mapping.id),
                    "name": group.name,
                    "permission_ids": list(
                        group.permissions.order_by("id").values_list("id", flat=True)
                    ),
                }
            )
        return {"groups": items}

    def import_slice(self, payload):
        groups_payload = list((payload or {}).get("groups") or [])
        service = GroupService()
        with transaction.atomic():
            keep_names = set()
            for item in groups_payload:
                name = (item.get("name") or "").strip()
                if not name:
                    continue
                keep_names.add(name)
                group = Group.objects.filter(name=name).first()
                if group is None:
                    group = Group.objects.create(name=name)
                    GroupUuid.objects.get_or_create(group=group)
                else:
                    GroupUuid.objects.get_or_create(group=group)
                permission_ids = item.get("permission_ids")
                if permission_ids is not None:
                    service.set_permissions(group, permission_ids)
            for group in Group.objects.exclude(name__in=keep_names):
                group.delete()

    def register(self):
        site_config_registry.register(
            self.SLICE_NAME,
            self.export_slice,
            self.import_slice,
        )


def register_users_site_config():
    UsersSiteConfigSlice().register()
