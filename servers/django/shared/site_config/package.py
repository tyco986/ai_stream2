import hashlib
import io
import json
import zipfile
from datetime import datetime, timezone

from django.conf import settings

from shared.http.exceptions import AppError
from shared.site_config.registry import site_config_registry


class SiteConfigPackage:
    MANIFEST_NAME = "manifest.json"
    SLICES_PREFIX = "slices/"

    def __init__(self, schema_version=None, app_version=None, site_id=None):
        self.schema_version = (
            schema_version
            if schema_version is not None
            else settings.SITE_CONFIG_SCHEMA_VERSION
        )
        self.app_version = app_version if app_version is not None else settings.APP_VERSION
        self.site_id = site_id if site_id is not None else settings.SITE_ID

    def build_zip_bytes(self):
        slices = site_config_registry.export_all()
        slice_files = {
            f"{self.SLICES_PREFIX}{name}.json": json.dumps(
                data, ensure_ascii=False, sort_keys=True
            ).encode("utf-8")
            for name, data in sorted(slices.items())
        }
        checksum = self.compute_checksum(slice_files)
        manifest = {
            "schema_version": self.schema_version,
            "app_version": self.app_version,
            "site_id": self.site_id or None,
            "exported_at": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
            "checksum": f"sha256:{checksum}",
        }
        buffer = io.BytesIO()
        with zipfile.ZipFile(buffer, mode="w", compression=zipfile.ZIP_DEFLATED) as archive:
            archive.writestr(
                self.MANIFEST_NAME,
                json.dumps(manifest, ensure_ascii=False, indent=2).encode("utf-8"),
            )
            for path, content in slice_files.items():
                archive.writestr(path, content)
        return buffer.getvalue()

    def parse_and_validate(self, zip_bytes):
        result = {"manifest": None, "slices": {}}
        if not zipfile.is_zipfile(io.BytesIO(zip_bytes)):
            raise AppError("Invalid site config package", status_code=400)
        with zipfile.ZipFile(io.BytesIO(zip_bytes), mode="r") as archive:
            names = archive.namelist()
            if self.MANIFEST_NAME not in names:
                raise AppError("Missing manifest.json", status_code=400)
            manifest = json.loads(archive.read(self.MANIFEST_NAME).decode("utf-8"))
            slice_files = {
                name: archive.read(name)
                for name in names
                if name.startswith(self.SLICES_PREFIX) and name.endswith(".json")
            }
        expected = manifest.get("checksum", "")
        actual = f"sha256:{self.compute_checksum(slice_files)}"
        if expected != actual:
            raise AppError("Package checksum mismatch", status_code=400)
        if int(manifest.get("schema_version", -1)) != settings.SITE_CONFIG_SCHEMA_VERSION:
            raise AppError("schema_version mismatch", status_code=400)
        slices = {}
        for path, content in slice_files.items():
            name = path[len(self.SLICES_PREFIX) : -len(".json")]
            slices[name] = json.loads(content.decode("utf-8"))
        result["manifest"] = manifest
        result["slices"] = slices
        return result

    def apply_slices(self, slices):
        site_config_registry.import_all(slices)

    def compute_checksum(self, slice_files):
        digest = hashlib.sha256()
        for path in sorted(slice_files.keys()):
            digest.update(path.encode("utf-8"))
            digest.update(b"\0")
            digest.update(slice_files[path])
            digest.update(b"\0")
        return digest.hexdigest()
