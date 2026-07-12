import shutil
from pathlib import Path

from .yolo_generator import (
    DetImageGenerator,
    DetSahiImageGenerator,
    DetVideoGenerator,
    SegVideoGenerator,
    PoseVideoGenerator,
    DetSahiVideoGenerator,
    SegImageGenerator,
    PoseImageGenerator,
    DetRTSPGenerator,
    DetVisRTSPGenerator,
    SegRTSPGenerator,
    SegVisRTSPGenerator,
    PoseRTSPGenerator,
    PoseVisRTSPGenerator,
    DetSahiRTSPGenerator,
    DetSahiVisRTSPGenerator,
)


class GeneratorManager:
    GENERATORS = {
        "DetImageGenerator": DetImageGenerator,
        "DetSahiImageGenerator": DetSahiImageGenerator,
        "DetVideoGenerator": DetVideoGenerator,
        "SegVideoGenerator": SegVideoGenerator,
        "PoseVideoGenerator": PoseVideoGenerator,
        "DetSahiVideoGenerator": DetSahiVideoGenerator,
        "SegImageGenerator": SegImageGenerator,
        "PoseImageGenerator": PoseImageGenerator,
        "DetRTSPGenerator": DetRTSPGenerator,
        "DetVisRTSPGenerator": DetVisRTSPGenerator,
        "SegRTSPGenerator": SegRTSPGenerator,
        "SegVisRTSPGenerator": SegVisRTSPGenerator,
        "PoseRTSPGenerator": PoseRTSPGenerator,
        "PoseVisRTSPGenerator": PoseVisRTSPGenerator,
        "DetSahiRTSPGenerator": DetSahiRTSPGenerator,
        "DetSahiVisRTSPGenerator": DetSahiVisRTSPGenerator,
    }

    def __init__(self, config: dict) -> None:
        self.config = dict(config)
        self.config_save_dir = Path(self.config.pop("config_save_dir"))
        self.generator_name = self.config.pop("generator")
        assert self.generator_name in self.GENERATORS, (
            f"unsupported generator: {self.generator_name!r} "
            f"(supported: {', '.join(self.GENERATORS)})"
        )
        self.generator = self.GENERATORS[self.generator_name](**self.config)

    def write(self) -> None:
        if self.config_save_dir.exists():
            assert self.config_save_dir.is_dir(), (
                f"config_save_dir is not a directory: {self.config_save_dir}"
            )
            shutil.rmtree(self.config_save_dir)
        self.config_save_dir.mkdir(parents=True, exist_ok=True)
        self.generator.write(self.config_save_dir)
