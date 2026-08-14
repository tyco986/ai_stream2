import shutil
import zipfile
from pathlib import Path

import yaml
from fastapi import UploadFile

from utils.api.constants import DEFAULT_MODEL_ROOT
from utils.api.schemas import ApiEnvelope, AppError, TrtExportConfig
from utils.manager.trt_exporter_manager import TrtExporterManager


class ExportTrtService:
    def parse_config(self, upload: UploadFile) -> TrtExportConfig:
        payload = yaml.safe_load(upload.file.read())
        if not isinstance(payload, dict):
            raise AppError("config YAML must be a mapping")
        config = TrtExportConfig.model_validate(payload)
        return config

    def extract_safe(self, archive: zipfile.ZipFile, dest: Path) -> None:
        dest = dest.resolve()
        dest.mkdir(parents=True, exist_ok=True)
        for info in archive.infolist():
            if info.filename.startswith("__MACOSX/") or info.filename == "__MACOSX":
                continue
            name = Path(info.filename)
            if name.is_absolute() or ".." in name.parts:
                raise AppError(f"unsafe zip path: {info.filename}")
            target = (dest / name).resolve()
            if not target.is_relative_to(dest):
                raise AppError(f"unsafe zip path: {info.filename}")
            if info.is_dir() or info.filename.endswith("/"):
                target.mkdir(parents=True, exist_ok=True)
                continue
            target.parent.mkdir(parents=True, exist_ok=True)
            with archive.open(info) as src, target.open("wb") as handle:
                shutil.copyfileobj(src, handle)

    def flatten_root(self, staging: Path) -> Path:
        children = [path for path in staging.iterdir() if path.name != "__MACOSX"]
        root = staging
        if len(children) == 1 and children[0].is_dir():
            root = children[0]
        return root

    def unpack(self, zip_path: Path, dest: Path) -> None:
        staging = dest.parent / f".{dest.name}.staging"
        if staging.exists():
            shutil.rmtree(staging)
        if dest.exists():
            shutil.rmtree(dest)
        with zipfile.ZipFile(zip_path) as archive:
            self.extract_safe(archive, staging)
        source = self.flatten_root(staging)
        shutil.copytree(source, dest)
        shutil.rmtree(staging)

    def save_zip(self, upload: UploadFile) -> Path:
        filename = Path(upload.filename or "bundle.zip").name
        if Path(filename).suffix.lower() != ".zip":
            raise AppError(f"input must be a .zip file: {filename}")
        onnx_root = DEFAULT_MODEL_ROOT / "onnx"
        onnx_root.mkdir(parents=True, exist_ok=True)
        zip_path = onnx_root / filename
        zip_path.unlink(missing_ok=True)
        with zip_path.open("wb") as handle:
            shutil.copyfileobj(upload.file, handle)
        if not zipfile.is_zipfile(zip_path):
            zip_path.unlink(missing_ok=True)
            raise AppError(f"input is not a valid zip: {filename}")
        onnx_dir = onnx_root / zip_path.stem
        self.unpack(zip_path, onnx_dir)
        zip_path.unlink()
        return onnx_dir

    def export(self, input: UploadFile, config: UploadFile) -> ApiEnvelope:
        cfg = self.parse_config(config)
        if cfg.type not in TrtExporterManager.EXPORTERS:
            raise AppError(f"unknown type {cfg.type!r}")
        onnx_dir = self.save_zip(input)
        output_dir = DEFAULT_MODEL_ROOT / "trt" / onnx_dir.name
        if output_dir.exists():
            shutil.rmtree(output_dir)
        output_dir.mkdir(parents=True)
        TrtExporterManager().export(
            cfg.type,
            onnx_dir,
            output_dir,
            cfg.batch_size,
            cfg.gpu_id,
            cfg.precision,
            cfg.opt_level,
        )
        payload = ApiEnvelope.ok(data=str(output_dir))
        return payload
