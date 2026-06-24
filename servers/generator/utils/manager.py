import shutil
from pathlib import Path

import yaml

from .yolo_generator import (
    YoloDetImageConfigGenerator,
    YoloDetRTSPConfigGenerator,
    YoloDetSahiConfigGenerator,
    YoloDetSahiImageConfigGenerator,
    YoloDetSahiVideoConfigGenerator,
    YoloDetVideoConfigGenerator,
    YoloPoseImageConfigGenerator,
    YoloPoseRTSPConfigGenerator,
    YoloPoseVideoConfigGenerator,
    YoloSegImageConfigGenerator,
    YoloSegRTSPConfigGenerator,
    YoloSegVideoConfigGenerator,
)

CONTAINER_PATH_PREFIXES = (
    ("models/", "/root/models/"),
    ("configs/", "/root/configs/"),
    ("attachments/", "/root/attachments/"),
)


class ConfigGeneratorManager:
    GENERATORS = {
        "YoloDetImageConfigGenerator": YoloDetImageConfigGenerator,
        "YoloSegImageConfigGenerator": YoloSegImageConfigGenerator,
        "YoloPoseImageConfigGenerator": YoloPoseImageConfigGenerator,
        "YoloDetSahiImageConfigGenerator": YoloDetSahiImageConfigGenerator,
        "YoloDetVideoConfigGenerator": YoloDetVideoConfigGenerator,
        "YoloSegVideoConfigGenerator": YoloSegVideoConfigGenerator,
        "YoloPoseVideoConfigGenerator": YoloPoseVideoConfigGenerator,
        "YoloDetSahiVideoConfigGenerator": YoloDetSahiVideoConfigGenerator,
        "YoloDetRTSPConfigGenerator": YoloDetRTSPConfigGenerator,
        "YoloSegRTSPConfigGenerator": YoloSegRTSPConfigGenerator,
        "YoloPoseRTSPConfigGenerator": YoloPoseRTSPConfigGenerator,
        "YoloDetSahiConfigGenerator": YoloDetSahiConfigGenerator,
    }

    def __init__(self, config: dict) -> None:
        config = dict(config)
        generator_name = config.pop("generator")
        assert generator_name in self.GENERATORS, (
            f"unsupported generator: {generator_name!r} "
            f"(supported: {', '.join(self.GENERATORS)})"
        )
        self.generator = self.GENERATORS[generator_name](**config)

    @classmethod
    def from_yaml(cls, content: bytes) -> "ConfigGeneratorManager":
        config = yaml.safe_load(content)
        if not isinstance(config, dict):
            raise ValueError("YAML root must be a mapping")
        if "generator" not in config:
            raise ValueError("generator is required")
        return cls(cls.normalize_paths(config))

    @classmethod
    def normalize_paths(cls, value):
        if isinstance(value, dict):
            return {key: cls.normalize_paths(item) for key, item in value.items()}
        if isinstance(value, list):
            return [cls.normalize_paths(item) for item in value]
        if not isinstance(value, str) or value.startswith("/root/"):
            return value
        for prefix, container_prefix in CONTAINER_PATH_PREFIXES:
            if value.startswith(prefix):
                return container_prefix + value[len(prefix) :]
        return value

    @staticmethod
    def prepare_config_save_dir(path: str | Path) -> Path:
        save_dir = Path(path)
        if save_dir.exists():
            assert save_dir.is_dir(), f"config_save_dir is not a directory: {save_dir}"
            shutil.rmtree(save_dir)
        save_dir.mkdir(parents=True, exist_ok=True)
        return save_dir

    def write(self) -> None:
        save_dir = getattr(self.generator, "config_save_dir", None)
        if save_dir is not None:
            self.prepare_config_save_dir(save_dir)
        self.generator.write()
