import json
from pathlib import Path


class PgieParser:
    META_JSON_NAME = "meta.json"
    LABELS_NAME = "labels.txt"
    NETWORK_MODE_MAP = {
        "fp32": 0,
        "fp16": 2,
        "int8": 1,
    }
    SUPPORTED_YOLO_VERSIONS = ("yolo8", "yolo11", "yolo10", "yolo26")
    CLASS_ATTR_KEY_MAP = {
        "conf": "pre-cluster-threshold",
        "iou": "nms-iou-threshold",
        "detected_min_w": "detected-min-w",
        "detected_min_h": "detected-min-h",
        "detected_max_w": "detected-max-w",
        "detected_max_h": "detected-max-h",
    }

    def __init__(
        self,
        model_dir: str | Path,
        runtime_batch_size: int,
        class_attr: dict,
        class_on: list[int] | None = None,
        interval: int = 0,
    ) -> None:
        self.model_dir = model_dir
        self.runtime_batch_size = runtime_batch_size
        self.class_attr = class_attr
        self.class_on = class_on
        self.interval = interval

        path = Path(model_dir).expanduser().resolve()
        assert path.is_dir(), f"model_dir not found: {path}"
        self.path = path

        self.meta_path = path / self.META_JSON_NAME
        self.labels_path = path / self.LABELS_NAME
        assert self.meta_path.is_file(), f"{self.META_JSON_NAME} not found in {path}"
        assert self.labels_path.is_file(), f"{self.LABELS_NAME} not found in {path}"

        engine_files = sorted(path.glob("*.engine"))
        assert len(engine_files) == 1, (
            f"expected one .engine in {path}, found {len(engine_files)}"
        )

        self.meta = json.loads(self.meta_path.read_text(encoding="utf-8"))
        self.engine_path = engine_files[0]
        self.init_class()

    def init_class(self) -> None:
        self.num_classes = len(self.meta["classes"])
        self.class_ids = set(range(self.num_classes))
        if self.class_on is not None:
            self.class_ids = set(self.class_on)

    def build(self) -> dict:
        precision = self.meta.get("precision")
        assert precision in self.NETWORK_MODE_MAP, (
            f"unsupported precision in {self.META_JSON_NAME}: {precision}"
        )
        version = self.meta.get("version")
        assert version in self.SUPPORTED_YOLO_VERSIONS, (
            f"unsupported version in {self.META_JSON_NAME}: {version}"
        )
        num_classes = self.num_classes
        model_type = self.meta["model_type"]
        engine_batch_size = self.meta["batch_size"]
        if model_type == "static":
            assert self.runtime_batch_size == engine_batch_size, (
                f"runtime_batch_size {self.runtime_batch_size} must equal "
                f"engine batch_size {engine_batch_size} for static model"
            )
        else:
            assert model_type == "dynamic", (
                f"unsupported model_type in {self.META_JSON_NAME}: {model_type}"
            )
            assert self.runtime_batch_size <= engine_batch_size, (
                f"runtime_batch_size {self.runtime_batch_size} must be <= "
                f"engine batch_size {engine_batch_size} for dynamic model"
            )

        assert self.class_on is None or self.class_on, "class_on cannot be empty list"
        if self.class_on is not None:
            assert max(self.class_on) <= num_classes - 1, (
                f"class_on max id must be <= {num_classes - 1}"
            )
            assert min(self.class_on) >= 0, "class_on ids must be >= 0"
            attr_class_ids = {
                int(class_key) for class_key in self.class_attr if class_key != "all"
            }
            assert attr_class_ids <= self.class_ids, (
                f"class_attr class ids {sorted(attr_class_ids - self.class_ids)} "
                f"not in class_on"
            )

        if any("iou" in attrs for attrs in self.class_attr.values()):
            assert version in ("yolo8", "yolo11"), (
                f"iou not supported for {version}"
            )
        class_attrs = {
            ("class-attrs-all" if class_key == "all" else f"class-attrs-{class_key}"): {
                self.CLASS_ATTR_KEY_MAP[key]: value for key, value in attrs.items()
            }
            for class_key, attrs in self.class_attr.items()
        }

        filter_out_class_ids = (
            ";".join(
                str(class_id)
                for class_id in range(num_classes)
                if class_id not in self.class_ids
            )
            if self.class_on is not None
            else None
        )

        return {
            "model_engine_file": str(self.engine_path),
            "labelfile_path": str(self.labels_path),
            "batch_size": self.runtime_batch_size,
            "network_mode": self.NETWORK_MODE_MAP[precision],
            "gpu_id": self.meta["gpu_id"],
            "interval": self.interval,
            "class_attrs": class_attrs,
            "filter_out_class_ids": filter_out_class_ids,
            "num_detected_classes": num_classes,
        }
