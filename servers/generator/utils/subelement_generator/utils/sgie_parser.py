import json
from pathlib import Path


class SgieParser:
    """Parse a TensorRT SGIE bundle dir: meta.json, labels.txt, one *.engine."""

    META_JSON_NAME = "meta.json"
    LABELS_NAME = "labels.txt"
    NETWORK_MODE_MAP = {
        "fp32": 0,
        "fp16": 2,
        "int8": 1,
    }
    SUPPORTED_VERSIONS = ("rtmpose",)

    def __init__(
        self,
        model_dir: str | Path,
        runtime_batch_size: int,
        interval: int = 0,
    ) -> None:
        self.model_dir = model_dir
        self.runtime_batch_size = runtime_batch_size
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

    def build(self) -> dict:
        precision = self.meta.get("precision")
        assert precision in self.NETWORK_MODE_MAP, (
            f"unsupported precision in {self.META_JSON_NAME}: {precision}"
        )
        version = self.meta.get("version")
        assert version in self.SUPPORTED_VERSIONS, (
            f"unsupported version in {self.META_JSON_NAME}: {version}"
        )
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

        input_shape = self.meta["input_tensor_shape"]
        assert len(input_shape) == 4, f"expected NCHW input_tensor_shape, got {input_shape}"
        infer_dims = f"{input_shape[1]};{input_shape[2]};{input_shape[3]}"
        output_blob_names = ";".join(self.meta["output_tensor_names"])

        return {
            "model_engine_file": str(self.engine_path),
            "labelfile_path": str(self.labels_path),
            "batch_size": self.runtime_batch_size,
            "network_mode": self.NETWORK_MODE_MAP[precision],
            "gpu_id": self.meta["gpu_id"],
            "interval": self.interval,
            "infer_dims": infer_dims,
            "output_blob_names": output_blob_names,
        }
