import hashlib
import re
from pathlib import Path

import jwt
from cryptography.hazmat.primitives.serialization import load_pem_public_key
from django.conf import settings
from django.contrib.auth import logout
from django.db import transaction

from pages.shell.models import PageSettings, SiteConfigVersion, UsedAuthTicket, VERSION_PATTERN
from shared.audit import AuditService
from shared.http.exceptions import AppError
from shared.permissions_catalog import PermissionCodenameMapper
from shared.site_config import AgeCrypto, SiteConfigPackage

SEED_VERSION = "0"
SEED_DESCRIPTION = "image default"
SEED_PAYLOAD_NAME = "version-0.age"


class SessionUserService:
    def get_session_user(self, user):
        permissions = ["*"]
        if not user.is_superuser:
            permissions = PermissionCodenameMapper().map_many(user.get_all_permissions())
        return {
            "id": str(user.pk),
            "username": user.username,
            "is_superuser": bool(user.is_superuser),
            "permissions": permissions,
        }


class SettingsService:
    def get_mode(self, user):
        row = PageSettings.objects.filter(user=user).first()
        mode = settings.DEFAULT_SHELL_MODE
        if row is not None:
            mode = row.mode
        return {"mode": mode}

    def set_mode(self, user, mode):
        row, _created = PageSettings.objects.update_or_create(
            user=user,
            defaults={"mode": mode},
        )
        return {"mode": row.mode}


class LogoutService:
    def logout(self, request):
        logout(request)


class AuthTicketService:
    def __init__(self, ticket_pub_path=None):
        self.ticket_pub_path = Path(
            ticket_pub_path if ticket_pub_path is not None else settings.TICKET_PUB_PATH
        )

    def validate(self, token, action, pkg_hash=None, site_id=None):
        if not self.ticket_pub_path.is_file():
            raise AppError(
                f"Missing ticket public key: {self.ticket_pub_path}",
                status_code=500,
            )
        if not token:
            raise AppError("Missing or invalid auth ticket", status_code=401)
        public_key = load_pem_public_key(self.ticket_pub_path.read_bytes())
        required = ["exp", "jti", "action"]
        if pkg_hash is not None:
            required.append("pkg_hash")
        claims = None
        try:
            claims = jwt.decode(
                token,
                key=public_key,
                algorithms=["EdDSA"],
                options={"require": required},
            )
        except jwt.PyJWTError as exc:
            raise AppError("Missing or invalid auth ticket", status_code=401) from exc
        if claims.get("action") != action:
            raise AppError("Missing or invalid auth ticket", status_code=401)
        if pkg_hash is not None:
            if str(claims.get("pkg_hash", "")).lower() != str(pkg_hash).lower():
                raise AppError("Missing or invalid auth ticket", status_code=401)
        expected_site = site_id if site_id is not None else settings.SITE_ID
        ticket_site = claims.get("site_id")
        if ticket_site and expected_site and ticket_site != expected_site:
            raise AppError("Missing or invalid auth ticket", status_code=401)
        jti = claims.get("jti")
        if not jti:
            raise AppError("Missing or invalid auth ticket", status_code=401)
        if UsedAuthTicket.objects.filter(jti=jti).exists():
            raise AppError("Missing or invalid auth ticket", status_code=401)
        return {"jti": jti, "action": action, "claims": claims}

    def consume(self, jti, action):
        UsedAuthTicket.objects.create(jti=jti, action=action)


class SiteConfigOrchestrator:
    def __init__(self):
        self.age = AgeCrypto(
            settings.AGE_SITE_KEY_PATH,
            settings.AGE_SITE_PUB_PATH,
            settings.AGE_DEV_PUB_PATH,
        )
        self.package = SiteConfigPackage()
        self.tickets = AuthTicketService()
        self.audit = AuditService()
        self.payload_dir = Path(settings.SITE_CONFIG_PAYLOAD_DIR)

    def ensure_seed(self):
        self.payload_dir.mkdir(parents=True, exist_ok=True)
        seed_path = self.payload_dir / SEED_PAYLOAD_NAME
        if not seed_path.is_file():
            zip_bytes = self.package.build_zip_bytes()
            ciphertext = self.age.seal_to_site(zip_bytes)
            seed_path.write_bytes(ciphertext)
        if not SiteConfigVersion.objects.filter(version=SEED_VERSION).exists():
            SiteConfigVersion.objects.create(
                version=SEED_VERSION,
                description=SEED_DESCRIPTION,
                is_current=True,
                payload_path=str(seed_path),
            )

    def list_versions(self):
        rows = list(SiteConfigVersion.objects.all().order_by("-created_at"))
        return {
            "items": [
                {
                    "id": str(row.id),
                    "version": row.version,
                    "description": row.description,
                    "created_at": row.created_at,
                    "is_current": row.is_current,
                }
                for row in rows
            ]
        }

    def export_current(self, user):
        zip_bytes = self.package.build_zip_bytes()
        ciphertext = self.age.seal_to_vendor(zip_bytes)
        self.audit.record(user, "Export", "export current site config")
        return ciphertext

    def import_current(self, user, file_bytes, auth_ticket):
        pkg_hash = hashlib.sha256(file_bytes).hexdigest()
        ticket = self.tickets.validate(auth_ticket, "import", pkg_hash)
        plaintext = self.age.open_with_site_key(file_bytes)
        parsed = self.package.parse_and_validate(plaintext)
        with transaction.atomic():
            self.package.apply_slices(parsed["slices"])
            self.tickets.consume(ticket["jti"], "import")
        self.audit.record(user, "Import", "import site config to running db")
        return {
            "schema_version": parsed["manifest"]["schema_version"],
            "applied": True,
        }

    def backup_version(self, user, version, description):
        if not VERSION_PATTERN.match(version):
            raise AppError("Invalid version format", status_code=400)
        if SiteConfigVersion.objects.filter(version=version).exists():
            raise AppError("Version already exists", status_code=400)
        zip_bytes = self.package.build_zip_bytes()
        ciphertext = self.age.seal_to_site(zip_bytes)
        self.payload_dir.mkdir(parents=True, exist_ok=True)
        safe_version = re.sub(r"[^\d.]", "_", version)
        payload_path = self.payload_dir / f"version-{safe_version}.age"
        payload_path.write_bytes(ciphertext)
        row = SiteConfigVersion.objects.create(
            version=version,
            description=description,
            is_current=False,
            payload_path=str(payload_path),
        )
        self.audit.record(user, "Backup", f"backup version {version}")
        return self.serialize_version(row)

    def import_version_payload(self, user, version_id, file_bytes, auth_ticket):
        row = self.get_version(version_id)
        pkg_hash = hashlib.sha256(file_bytes).hexdigest()
        ticket = self.tickets.validate(auth_ticket, "import", pkg_hash)
        plaintext = self.age.open_with_site_key(file_bytes)
        self.package.parse_and_validate(plaintext)
        path = Path(row.payload_path)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(file_bytes)
        self.tickets.consume(ticket["jti"], "import")
        self.audit.record(user, "Import", f"import payload for version {row.version}")
        return self.serialize_version(row)

    def export_version(self, user, version_id):
        row = self.get_version(version_id)
        path = Path(row.payload_path)
        if not path.is_file():
            raise AppError("Version payload missing", status_code=500)
        plaintext = self.age.open_with_site_key(path.read_bytes())
        ciphertext = self.age.seal_to_vendor(plaintext)
        self.audit.record(user, "Export", f"export version {row.version}")
        return ciphertext

    def apply_version(self, user, version_id, auth_ticket):
        row = self.get_version(version_id)
        if row.is_current:
            raise AppError("Version is already current", status_code=400)
        path = Path(row.payload_path)
        if not path.is_file():
            raise AppError("Version payload missing", status_code=400)
        file_bytes = path.read_bytes()
        pkg_hash = hashlib.sha256(file_bytes).hexdigest()
        ticket = self.tickets.validate(auth_ticket, "apply", pkg_hash)
        plaintext = self.age.open_with_site_key(file_bytes)
        parsed = self.package.parse_and_validate(plaintext)
        with transaction.atomic():
            self.package.apply_slices(parsed["slices"])
            SiteConfigVersion.objects.filter(is_current=True).update(is_current=False)
            row.is_current = True
            row.save(update_fields=["is_current"])
            self.tickets.consume(ticket["jti"], "apply")
        self.audit.record(user, "Apply", f"apply version {row.version}")
        return {
            "id": str(row.id),
            "version": row.version,
            "applied": True,
        }

    def get_version(self, version_id):
        row = SiteConfigVersion.objects.filter(pk=version_id).first()
        if row is None:
            raise AppError("Version not found", status_code=404)
        return row

    def serialize_version(self, row):
        return {
            "id": str(row.id),
            "version": row.version,
            "description": row.description,
            "created_at": row.created_at,
            "is_current": row.is_current,
        }
