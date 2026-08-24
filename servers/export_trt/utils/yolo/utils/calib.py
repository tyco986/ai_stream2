import zipfile
from pathlib import Path

from utils.yolo.utils.constants import IMAGE_SUFFIXES


class CalibImageSet:
    def __init__(self, folder, count):
        self.folder = Path(folder)
        self.count = count

    def paths(self) -> list[Path]:
        if not self.folder.is_dir():
            raise ValueError(f"calib_dir is not a directory: {self.folder}")
        found = sorted(
            path
            for path in self.folder.rglob("*")
            if path.is_file() and path.suffix.lower() in IMAGE_SUFFIXES
        )
        if not found:
            raise ValueError(f"no calibration images in {self.folder}")
        selected = found[: self.count]
        return selected


class Int8CalibZip:
    def __init__(self, zip_path, count):
        self.zip_path = Path(zip_path)
        self.count = count

    def extract_paths(self, dest: Path) -> list[Path]:
        if not self.zip_path.is_file():
            raise ValueError(f"missing int8 calib zip: {self.zip_path}")
        dest.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(self.zip_path) as archive:
            archive.extractall(dest)
        paths = CalibImageSet(dest, self.count).paths()
        return paths
