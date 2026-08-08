from django.contrib.auth.models import Group, Permission
from django.contrib.contenttypes.models import ContentType
from django.db import transaction

from pages.users.models import GroupUuid, PermissionHost, User, UserAuditLog
from shared.http.exceptions import AppError
from shared.pagination import PaginationService
from shared.permissions_catalog import CATALOG_MODULES, HOST_CODENAME_TO_PUBLIC


class UserAuditService:
    def append(self, user, label, detail=""):
        row = None
        if user is not None and getattr(user, "pk", None) is not None:
            row = UserAuditLog.objects.create(
                user=user,
                label=label,
                detail=detail or label,
            )
        return row

    def as_sink(self):
        return self.record_sink

    def record_sink(self, user, action, detail=""):
        self.append(user, action, detail)


class CatalogService:
    def build(self):
        host_ct = ContentType.objects.get_for_model(PermissionHost)
        user_ct = ContentType.objects.get_for_model(User)
        stream_ct = ContentType.objects.filter(
            app_label="streams",
            model="stream",
        ).first()
        preview_ct = ContentType.objects.filter(
            app_label="previews",
            model="layoutpreset",
        ).first()
        recording_ct = ContentType.objects.filter(
            app_label="recordings",
            model="recording",
        ).first()
        model_ct = ContentType.objects.filter(
            app_label="models",
            model="mlmodel",
        ).first()
        pipeline_ct = ContentType.objects.filter(
            app_label="pipelines",
            model="pipeline",
        ).first()
        host_perms = {
            row.codename: row
            for row in Permission.objects.filter(content_type=host_ct)
        }
        user_perms = {
            row.codename: row
            for row in Permission.objects.filter(content_type=user_ct)
        }
        stream_perms = {}
        if stream_ct is not None:
            stream_perms = {
                row.codename: row
                for row in Permission.objects.filter(content_type=stream_ct)
            }
        preview_perms = {}
        if preview_ct is not None:
            preview_perms = {
                row.codename: row
                for row in Permission.objects.filter(content_type=preview_ct)
            }
        recording_perms = {}
        if recording_ct is not None:
            recording_perms = {
                row.codename: row
                for row in Permission.objects.filter(content_type=recording_ct)
            }
        model_perms = {}
        if model_ct is not None:
            model_perms = {
                row.codename: row
                for row in Permission.objects.filter(content_type=model_ct)
            }
        pipeline_perms = {}
        if pipeline_ct is not None:
            pipeline_perms = {
                row.codename: row
                for row in Permission.objects.filter(content_type=pipeline_ct)
            }
        modules = []
        for module in CATALOG_MODULES:
            actions = []
            for action_key, action_label, host_or_user_codename in module["actions"]:
                perm = None
                public_codename = None
                if module["source"] == "host":
                    perm = host_perms.get(host_or_user_codename)
                    public_codename = HOST_CODENAME_TO_PUBLIC.get(
                        host_or_user_codename,
                        f"users.{host_or_user_codename}",
                    )
                elif module["source"] == "stream":
                    perm = stream_perms.get(host_or_user_codename)
                    public_codename = f"streams.{host_or_user_codename}"
                elif module["source"] == "preview":
                    perm = preview_perms.get(host_or_user_codename)
                    public_codename = f"previews.{host_or_user_codename}"
                elif module["source"] == "recording":
                    perm = recording_perms.get(host_or_user_codename)
                    public_codename = f"recordings.{host_or_user_codename}"
                elif module["source"] == "model":
                    perm = model_perms.get(host_or_user_codename)
                    public_codename = f"models.{host_or_user_codename}"
                elif module["source"] == "pipeline":
                    perm = pipeline_perms.get(host_or_user_codename)
                    public_codename = f"pipelines.{host_or_user_codename}"
                else:
                    perm = user_perms.get(host_or_user_codename)
                    public_codename = f"users.{host_or_user_codename}"
                if perm is None:
                    continue
                actions.append(
                    {
                        "key": action_key,
                        "label": action_label,
                        "permission_id": perm.id,
                        "codename": public_codename,
                    }
                )
            if actions:
                modules.append(
                    {
                        "key": module["key"],
                        "label": module["label"],
                        "actions": actions,
                    }
                )
        return {"modules": modules}


class GroupService:
    def list_groups(self):
        rows = list(Group.objects.all().order_by("name").prefetch_related("permissions"))
        return {"items": [self.serialize(row) for row in rows]}

    def get(self, group_id):
        row = self.resolve(group_id)
        return self.serialize(row)

    def create(self, name, permission_ids=None):
        cleaned = (name or "").strip()
        if not cleaned:
            raise AppError("name is required", status_code=400)
        if Group.objects.filter(name=cleaned).exists():
            raise AppError("Group name already exists", status_code=400)
        with transaction.atomic():
            group = Group.objects.create(name=cleaned)
            GroupUuid.objects.create(group=group)
            if permission_ids is not None:
                self.set_permissions(group, permission_ids)
        return self.serialize(group)

    def patch(self, group_id, name=None, permission_ids=None):
        group = self.resolve(group_id)
        with transaction.atomic():
            if name is not None:
                cleaned = name.strip()
                if not cleaned:
                    raise AppError("name is required", status_code=400)
                if Group.objects.filter(name=cleaned).exclude(pk=group.pk).exists():
                    raise AppError("Group name already exists", status_code=400)
                group.name = cleaned
                group.save(update_fields=["name"])
            if permission_ids is not None:
                self.set_permissions(group, permission_ids)
        return self.serialize(group)

    def delete(self, group_id):
        group = self.resolve(group_id)
        group.delete()

    def resolve(self, group_id):
        mapping = GroupUuid.objects.filter(pk=group_id).select_related("group").first()
        group = None
        if mapping is not None:
            group = mapping.group
        if group is None:
            raise AppError("Group not found", status_code=404)
        return group

    def ensure_uuid(self, group):
        mapping, _created = GroupUuid.objects.get_or_create(group=group)
        return mapping

    def set_permissions(self, group, permission_ids):
        ids = list(permission_ids or [])
        perms = list(Permission.objects.filter(pk__in=ids))
        if len(perms) != len(set(ids)):
            raise AppError("Invalid permission_ids", status_code=400)
        group.permissions.set(perms)

    def serialize(self, group):
        mapping = self.ensure_uuid(group)
        return {
            "id": str(mapping.id),
            "name": group.name,
            "member_count": group.user_set.count(),
            "permission_ids": list(
                group.permissions.order_by("id").values_list("id", flat=True)
            ),
        }


class UserService:
    def __init__(self):
        self.groups = GroupService()
        self.audit = UserAuditService()
        self.pagination = PaginationService()

    def list_users(self, search=None, page=1, page_size=20):
        queryset = User.objects.all().order_by("username").prefetch_related("groups")
        if search:
            queryset = queryset.filter(username__icontains=search.strip())
        page_data = self.pagination.slice_queryset(queryset, page, page_size)
        items = [self.serialize(row) for row in page_data["items"]]
        return self.pagination.build(
            items,
            page_data["total"],
            page_data["page"],
            page_data["page_size"],
        )

    def get(self, user_id):
        user = self.resolve(user_id)
        return self.serialize(user)

    def create(self, actor, username, password, is_active=True, group_ids=None):
        cleaned = (username or "").strip()
        if not cleaned:
            raise AppError("username is required", status_code=400)
        if not password:
            raise AppError("password is required", status_code=400)
        if not group_ids:
            raise AppError("group_ids is required", status_code=400)
        if User.objects.filter(username=cleaned).exists():
            raise AppError("Username already exists", status_code=400)
        with transaction.atomic():
            user = User(username=cleaned, is_active=bool(is_active), is_superuser=False)
            user.set_password(password)
            user.save()
            self.set_groups(user, group_ids)
            self.audit.append(user, "Create", f"Create user {user.username}")
        return self.serialize(user)

    def patch(self, actor, user_id, username=None, password=None, is_active=None, group_ids=None):
        user = self.resolve(user_id)
        with transaction.atomic():
            if username is not None:
                cleaned = username.strip()
                if not cleaned:
                    raise AppError("username is required", status_code=400)
                if User.objects.filter(username=cleaned).exclude(pk=user.pk).exists():
                    raise AppError("Username already exists", status_code=400)
                user.username = cleaned
            if is_active is not None:
                user.is_active = bool(is_active)
            if password is not None and password != "":
                user.set_password(password)
            user.save()
            if group_ids is not None:
                if not group_ids:
                    raise AppError("group_ids is required", status_code=400)
                self.set_groups(user, group_ids)
            self.audit.append(user, "Edit", f"Edit user {user.username}")
        return self.serialize(user)

    def delete(self, actor, user_id):
        if str(actor.pk) == str(user_id):
            raise AppError("Cannot delete yourself", status_code=400)
        user = self.resolve(user_id)
        if user.is_superuser and User.objects.filter(is_superuser=True).count() <= 1:
            raise AppError("Cannot delete the last superuser", status_code=400)
        user.delete()

    def batch_delete(self, actor, user_ids):
        deleted_count = 0
        skipped_count = 0
        actor_id = str(actor.pk)
        for raw_id in user_ids or []:
            if str(raw_id) == actor_id:
                skipped_count += 1
                continue
            user = User.objects.filter(pk=raw_id).first()
            if user is None:
                skipped_count += 1
                continue
            if user.is_superuser and User.objects.filter(is_superuser=True).count() <= 1:
                skipped_count += 1
                continue
            user.delete()
            deleted_count += 1
        return {
            "deleted_count": deleted_count,
            "skipped_count": skipped_count,
        }

    def list_logs(self, user_id, page=1, page_size=20):
        user = self.resolve(user_id)
        queryset = UserAuditLog.objects.filter(user=user).order_by("-at")
        page_data = self.pagination.slice_queryset(queryset, page, page_size)
        items = [
            {
                "id": str(row.id),
                "at": row.at,
                "label": row.label,
                "detail": row.detail,
            }
            for row in page_data["items"]
        ]
        return self.pagination.build(
            items,
            page_data["total"],
            page_data["page"],
            page_data["page_size"],
        )

    def resolve(self, user_id):
        user = User.objects.filter(pk=user_id).prefetch_related("groups").first()
        if user is None:
            raise AppError("User not found", status_code=404)
        return user

    def set_groups(self, user, group_ids):
        groups = []
        for group_id in group_ids:
            groups.append(self.groups.resolve(group_id))
        user.groups.set(groups)

    def serialize(self, user):
        group_items = []
        for group in user.groups.all().order_by("name"):
            mapping = self.groups.ensure_uuid(group)
            group_items.append({"id": str(mapping.id), "name": group.name})
        latest = (
            UserAuditLog.objects.filter(user=user).order_by("-at").first()
        )
        last_action_at = None
        last_action_label = None
        if latest is not None:
            last_action_at = latest.at
            last_action_label = latest.label
        return {
            "id": str(user.pk),
            "username": user.username,
            "is_active": bool(user.is_active),
            "is_superuser": bool(user.is_superuser),
            "groups": group_items,
            "last_action_at": last_action_at,
            "last_action_label": last_action_label,
        }
