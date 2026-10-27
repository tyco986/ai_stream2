import json
import shutil
import subprocess
import zipfile
from pathlib import Path

import yaml
from fastapi import UploadFile
from fastapi.responses import Response

from utils.api.constants import (
    ACCEPTED_SUFFIXES,
    LABEL_TYPES,
    HISTORY_BOUNDARY,
    MULTIPART_BOUNDARY,
    HISTORY_ROOT,
    TMP_ROOT,
)
from utils.api.schemas import AppError, OnnxExportConfig
from utils.manager.onnx_exporter_manager import OnnxExporterManager
from utils.yolo_e2e.common import write_bundle_meta


def multipart_body(data: dict, message: str, zip_path: Path, boundary: str) -> bytes:
    head = (
        f"--{boundary}\r\n"
        'Content-Disposition: form-data; name="success"\r\n\r\n'
        "true\r\n"
        f"--{boundary}\r\n"
        'Content-Disposition: form-data; name="data"\r\n'
        "Content-Type: application/json\r\n\r\n"
        f"{json.dumps(data, ensure_ascii=False)}\r\n"
        f"--{boundary}\r\n"
        'Content-Disposition: form-data; name="message"\r\n\r\n'
        f"{message}\r\n"
        f"--{boundary}\r\n"
        f'Content-Disposition: form-data; name="file"; filename="{zip_path.name}"\r\n'
        "Content-Type: application/zip\r\n\r\n"
    ).encode()
    tail = f"\r\n--{boundary}--\r\n".encode()
    body = head + zip_path.read_bytes() + tail
    return body


def history_data(parts: list[tuple[str, bytes]]) -> bytes:
    chunks = []
    for name, part in parts:
        chunks.append(
            (
                f"--{HISTORY_BOUNDARY}\r\n"
                f'Content-Disposition: form-data; name="{name}"\r\n'
                f"Content-Type: multipart/form-data; boundary={MULTIPART_BOUNDARY}-{name}\r\n\r\n"
            ).encode()
            + part
            + b"\r\n"
        )
    chunks.append(f"--{HISTORY_BOUNDARY}--\r\n".encode())
    body = b"".join(chunks)
    return body


class ExportOnnxService:
    def parse_config(self, upload: UploadFile) -> OnnxExportConfig:
        payload = yaml.safe_load(upload.file.read())
        if not isinstance(payload, dict):
            raise AppError("config YAML must be a mapping")
        config = OnnxExportConfig.model_validate(payload)
        return config

    def save_upload(self, upload: UploadFile, work_dir: Path) -> Path:
        filename = Path(upload.filename or "").name
        suffix = Path(filename).suffix.lower()
        if suffix not in ACCEPTED_SUFFIXES:
            raise AppError(f"file must be .pt or .onnx: {filename}")
        weights = work_dir / filename
        with weights.open("wb") as handle:
            shutil.copyfileobj(upload.file, handle)
        return weights

    def pack(self, work_dir: Path, stem: str) -> tuple[Path, dict]:
        onnx_path = work_dir / f"{stem}.onnx"
        labels_path = work_dir / "labels.txt"
        meta_path = work_dir / "meta.json"
        if not onnx_path.is_file() or not labels_path.is_file():
            raise AppError("export did not produce onnx and labels.txt")
        zip_path = HISTORY_ROOT / f"{stem}.zip"
        HISTORY_ROOT.mkdir(parents=True, exist_ok=True)
        zip_path.unlink(missing_ok=True)
        completed = subprocess.run(
            ["zip", "-j", str(zip_path), str(onnx_path), str(labels_path), str(meta_path)],
            capture_output=True,
            text=True,
        )
        if completed.returncode != 0 or not zip_path.is_file():
            raise AppError(completed.stderr.strip() or "zip failed")
        meta = json.loads(meta_path.read_text(encoding="utf-8"))
        shutil.rmtree(work_dir)
        return zip_path, meta

    def save_labels(self, upload: UploadFile | None, work_dir: Path) -> None:
        if upload is None:
            raise AppError("label file is required")
        text = upload.file.read().decode("utf-8")
        lines = [line.strip() for line in text.splitlines() if line.strip()]
        if not lines:
            raise AppError("label file is empty")
        (work_dir / "labels.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")

    def export(self, upload: UploadFile, config_file: UploadFile, label: UploadFile | None) -> Response:
        config = self.parse_config(config_file)
        if config.type not in OnnxExporterManager.EXPORTERS:
            raise AppError(f"unknown type {config.type!r}")
        filename = Path(upload.filename or "").name
        stem = Path(filename).stem
        if not stem:
            raise AppError("file name is required")
        work_dir = TMP_ROOT / stem
        if work_dir.exists():
            shutil.rmtree(work_dir)
        work_dir.mkdir(parents=True)
        weights = self.save_upload(upload, work_dir)
        if config.type in LABEL_TYPES:
            self.save_labels(label, work_dir)
        OnnxExporterManager().export(
            config.type,
            weights,
            config.size,
            config.opset,
            config.batch_size,
            config.batch_mode,
            config.simplify,
            config.max_det,
            config.conf,
            work_dir,
            config.iou,
        )
        write_bundle_meta(
            work_dir,
            work_dir / f"{stem}.onnx",
            work_dir / "labels.txt",
            version=config.type,
            max_det=config.max_det,
            conf=config.conf,
            iou=config.iou,
            batch_mode=config.batch_mode,
            batch_size=config.batch_size or 1,
        )
        zip_path, meta = self.pack(work_dir, stem)
        body = multipart_body(meta, "", zip_path, MULTIPART_BOUNDARY)
        payload = Response(
            content=body,
            media_type=f"multipart/form-data; boundary={MULTIPART_BOUNDARY}",
        )
        return payload

    def get_history(self, filename: str) -> dict:
        HISTORY_ROOT.mkdir(parents=True, exist_ok=True)
        name = filename.strip()
        paths = [path for path in sorted(HISTORY_ROOT.glob("*.zip")) if not name or path.name == name]
        data = {}
        for zip_path in paths:
            with zipfile.ZipFile(zip_path) as archive:
                meta = json.loads(archive.read("meta.json"))
            data[zip_path.name] = meta
        return data

    def history_paths(self, filename: str) -> list[Path]:
        HISTORY_ROOT.mkdir(parents=True, exist_ok=True)
        name = filename.strip()
        zip_path = HISTORY_ROOT / Path(name).name
        missing = bool(name) and (Path(name).name != name or zip_path.suffix.lower() != ".zip" or not zip_path.is_file())
        if missing:
            raise AppError(f"history file not found: {name}")
        paths = [zip_path] if name else sorted(HISTORY_ROOT.glob("*.zip"))
        return paths

    def download_history(self, filename: str) -> Response:
        parts = []
        for zip_path in self.history_paths(filename):
            with zipfile.ZipFile(zip_path) as archive:
                meta = json.loads(archive.read("meta.json"))
            part = multipart_body(meta, "", zip_path, f"{MULTIPART_BOUNDARY}-{zip_path.name}")
            parts.append((zip_path.name, part))
        data = history_data(parts)
        head = (
            f"--{MULTIPART_BOUNDARY}\r\n"
            'Content-Disposition: form-data; name="success"\r\n\r\n'
            "true\r\n"
            f"--{MULTIPART_BOUNDARY}\r\n"
            'Content-Disposition: form-data; name="data"\r\n'
            f"Content-Type: multipart/form-data; boundary={HISTORY_BOUNDARY}\r\n\r\n"
        ).encode()
        tail = (
            f"\r\n--{MULTIPART_BOUNDARY}\r\n"
            'Content-Disposition: form-data; name="message"\r\n\r\n'
            f"\r\n--{MULTIPART_BOUNDARY}--\r\n"
        ).encode()
        payload = Response(
            content=head + data + tail,
            media_type=f"multipart/form-data; boundary={MULTIPART_BOUNDARY}",
        )
        return payload

    def remove_history(self, filename: str) -> None:
        for zip_path in self.history_paths(filename):
            zip_path.unlink()
