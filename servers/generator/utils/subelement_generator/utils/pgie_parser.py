import json
from pathlib import Path


class PgieParser:
    """Parse a TensorRT bundle dir (not onnx): meta.json, labels.txt, one *.engine."""

    META_JSON_NAME = "meta.json"
    LABELS_NAME = "labels.txt"
    NETWORK_MODE_MAP = {
        "fp32": 0,
        "fp16": 2,
        "int8": 1,
    }
    SUPPORTED_VERSIONS = ("yolo8", "yolo11", "yolo10", "yolo26", "peoplenet")
    CLASS_ATTR_KEY_MAP = {
        "conf": "pre-cluster-threshold",
        "topk": "topk",
        "detected_min_w": "detected-min-w",
        "detected_min_h": "detected-min-h",
        "detected_max_w": "detected-max-w",
        "detected_max_h": "detected-max-h",
    }

    def __init__(
        self,
        model_dir: str | Path,  # TRT engine dir (export_trt output), not models/onnx
        runtime_batch_size: int,
        class_attrs: dict,
        interval: int = 0,
    ) -> None:
        self.model_dir = model_dir
        self.runtime_batch_size = runtime_batch_size
        self.class_attrs = class_attrs
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
        assert self.class_attrs, "class_attrs cannot be empty"

        has_all = False
        attr_class_ids = set()
        for class_key in self.class_attrs:
            if class_key == "all":
                has_all = True
                continue
            class_id = int(class_key)
            assert class_id not in attr_class_ids, (
                f"duplicate class id in class_attrs: {class_id}"
            )
            attr_class_ids.add(class_id)

        if attr_class_ids:
            assert max(attr_class_ids) <= self.num_classes - 1, (
                f"class_attrs max id must be <= {self.num_classes - 1}"
            )
            assert min(attr_class_ids) >= 0, "class_attrs ids must be >= 0"

        if has_all:
            self.class_ids = set(range(self.num_classes))
            self.filter_classes = False
        else:
            assert attr_class_ids, (
                "class_attrs must contain all or at least one class id"
            )
            self.class_ids = attr_class_ids
            self.filter_classes = True

    def build(self) -> dict:
        precision = self.meta.get("precision")
        assert precision in self.NETWORK_MODE_MAP, (
            f"unsupported precision in {self.META_JSON_NAME}: {precision}"
        )
        version = self.meta.get("version")
        assert version in self.SUPPORTED_VERSIONS, (
            f"unsupported version in {self.META_JSON_NAME}: {version}"
        )
        num_classes = self.num_classes
        batch_mode = self.meta["batch_mode"]
        engine_batch_size = self.meta["batch_size"]
        if batch_mode == "static":
            assert self.runtime_batch_size == engine_batch_size, (
                f"runtime_batch_size {self.runtime_batch_size} must equal "
                f"engine batch_size {engine_batch_size} for static model"
            )
        else:
            assert batch_mode == "dynamic", (
                f"unsupported batch_mode in {self.META_JSON_NAME}: {batch_mode}"
            )
            assert self.runtime_batch_size <= engine_batch_size, (
                f"runtime_batch_size {self.runtime_batch_size} must be <= "
                f"engine batch_size {engine_batch_size} for dynamic model"
            )

        for attrs in self.class_attrs.values():
            for key in attrs:
                assert key in self.CLASS_ATTR_KEY_MAP, (
                    f"unsupported class_attrs key: {key}"
                )

        class_attrs = {}
        for class_key, attrs in self.class_attrs.items():
            section = (
                "class-attrs-all" if class_key == "all" else f"class-attrs-{class_key}"
            )
            mapped = {}
            for key, value in attrs.items():
                if value is None:
                    continue
                if key.startswith("detected_") and int(value) < 0:
                    continue
                mapped[self.CLASS_ATTR_KEY_MAP[key]] = value
            class_attrs[section] = mapped

        filter_out_class_ids = (
            ";".join(
                str(class_id)
                for class_id in range(num_classes)
                if class_id not in self.class_ids
            )
            if self.filter_classes
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
