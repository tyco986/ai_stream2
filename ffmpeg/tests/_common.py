import json
from pathlib import Path


def print_ok(data: dict, *, file_path: Path | None = None) -> None:
    print("OK")
    print(json.dumps(data, indent=2, ensure_ascii=False))
    if file_path is not None:
        print(f"file_size={file_path.stat().st_size}")
