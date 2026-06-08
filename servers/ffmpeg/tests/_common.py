import json
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[3]
DEFAULT_HOST_VIDEO = PROJECT_ROOT / "attachments" / "videos" / "video1.mp4"
DEFAULT_API_VIDEO = "/app/video/video1.mp4"


def print_ok(data: dict, *, file_path: Path | None = None) -> None:
    print("OK")
    print(json.dumps(data, indent=2, ensure_ascii=False))
    if file_path is not None:
        print(f"file_size={file_path.stat().st_size}")
