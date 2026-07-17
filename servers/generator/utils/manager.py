import shutil
from pathlib import Path

from .event_generator.det_video_presence import DetVideoPresenceGenerator
from .event_generator.det_sahi_video_presence import DetSahiVideoPresenceGenerator
from .event_generator.det_sahi_rtsp_presence import DetSahiRTSPPresenceGenerator
from .yolo_generator.det_image import DetImageGenerator
from .yolo_generator.det_sahi_image import DetSahiImageGenerator
from .yolo_generator.seg_sahi_image import SegSahiImageGenerator
from .yolo_generator.seg_sahi_video import SegSahiVideoGenerator
from .yolo_generator.det_video import DetVideoGenerator
from .yolo_generator.seg_video import SegVideoGenerator
from .yolo_generator.det_sahi_video import DetSahiVideoGenerator
from .yolo_generator.seg_image import SegImageGenerator
from .yolo_generator.det_rtsp import DetRTSPGenerator, DetVisRTSPGenerator
from .yolo_generator.seg_rtsp import SegRTSPGenerator, SegVisRTSPGenerator
from .yolo_generator.det_sahi_rtsp import DetSahiRTSPGenerator, DetSahiVisRTSPGenerator
from .yolo_generator.seg_sahi_rtsp import SegSahiRTSPGenerator, SegSahiVisRTSPGenerator


class GeneratorManager:
    GENERATORS = {
        "DetImageGenerator": DetImageGenerator,
        "DetSahiImageGenerator": DetSahiImageGenerator,
        "SegSahiImageGenerator": SegSahiImageGenerator,
        "SegSahiVideoGenerator": SegSahiVideoGenerator,
        "DetVideoGenerator": DetVideoGenerator,
        "DetVideoPresenceGenerator": DetVideoPresenceGenerator,
        "DetSahiVideoPresenceGenerator": DetSahiVideoPresenceGenerator,
        "DetSahiRTSPPresenceGenerator": DetSahiRTSPPresenceGenerator,
        "SegVideoGenerator": SegVideoGenerator,
        "DetSahiVideoGenerator": DetSahiVideoGenerator,
        "SegImageGenerator": SegImageGenerator,
        "DetRTSPGenerator": DetRTSPGenerator,
        "DetVisRTSPGenerator": DetVisRTSPGenerator,
        "SegRTSPGenerator": SegRTSPGenerator,
        "SegVisRTSPGenerator": SegVisRTSPGenerator,
        "DetSahiRTSPGenerator": DetSahiRTSPGenerator,
        "DetSahiVisRTSPGenerator": DetSahiVisRTSPGenerator,
        "SegSahiRTSPGenerator": SegSahiRTSPGenerator,
        "SegSahiVisRTSPGenerator": SegSahiVisRTSPGenerator,
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
