from django.conf import settings
from django.contrib.auth import get_user_model
from django.contrib.auth.models import Group, Permission
from django.core.management.base import BaseCommand
from django.db import transaction

from pages.users.models import GroupUuid
from pages.users.services import CatalogService


class Command(BaseCommand):
    help = "Ensure seed superuser and Admin group exist when needed"

    def handle(self, *args, **options):
        user_model = get_user_model()
        username = settings.SEED_ADMIN_USERNAME
        if not user_model.objects.filter(is_superuser=True).exists():
            user = user_model.objects.filter(username=username).first()
            if user is None:
                user = user_model.objects.create_superuser(
                    username=username,
                    email=f"{username}@localhost",
                    password=settings.SEED_ADMIN_PASSWORD,
                )
                user.must_change_password = True
                user.save(update_fields=["must_change_password"])
                self.stdout.write(f"Created seed superuser: {username}")
            else:
                user.is_superuser = True
                user.is_staff = True
                user.is_active = True
                user.must_change_password = True
                user.save(
                    update_fields=[
                        "is_superuser",
                        "is_staff",
                        "is_active",
                        "must_change_password",
                    ]
                )
                self.stdout.write(
                    f"Promoted existing user to seed superuser: {username}"
                )
        else:
            self.stdout.write("Seed admin user skipped: superuser already exists")
        group = self.ensure_seed_admin_group()
        seed_user = user_model.objects.filter(
            username=username,
            is_superuser=True,
        ).first()
        if seed_user is None:
            seed_user = (
                user_model.objects.filter(is_superuser=True)
                .order_by("date_joined")
                .first()
            )
        if seed_user is not None:
            seed_user.groups.add(group)
            self.stdout.write(
                f"Linked superuser {seed_user.username} to group {group.name}"
            )

    def ensure_seed_admin_group(self):
        group_name = getattr(settings, "SEED_ADMIN_GROUP", "admin") or "admin"
        with transaction.atomic():
            group, created = Group.objects.get_or_create(name=group_name)
            GroupUuid.objects.get_or_create(group=group)
            permission_ids = self.catalog_permission_ids()
            perms = list(Permission.objects.filter(pk__in=permission_ids))
            group.permissions.set(perms)
        if created:
            self.stdout.write(f"Created seed admin group: {group_name}")
        else:
            self.stdout.write(f"Ensured seed admin group: {group_name}")
        return group

    def catalog_permission_ids(self):
        catalog = CatalogService().build()
        ids = [
            action["permission_id"]
            for module in catalog["modules"]
            for action in module["actions"]
        ]
        return ids
