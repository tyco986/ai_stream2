import importlib


class GeneratorRegistry:
    # name -> importable module path (lazy-loaded on resolve)
    MODULES = {
        "DetImageGenerator": "utils.yolo_generator.det_image",
        "DetSahiImageGenerator": "utils.yolo_generator.det_sahi_image",
        "SegSahiImageGenerator": "utils.yolo_generator.seg_sahi_image",
        "SegSahiVideoGenerator": "utils.yolo_generator.seg_sahi_video",
        "DetVideoGenerator": "utils.yolo_generator.det_video",
        "DetVideoPresenceGenerator": "utils.event_generator.det_video_presence",
        "DetSahiVideoPresenceGenerator": "utils.event_generator.det_sahi_video_presence",
        "DetVisRTSPPresenceGenerator": "utils.event_generator.det_vis_rtsp_presence",
        "DetSahiVisRTSPPresenceGenerator": "utils.event_generator.det_sahi_vis_rtsp_presence",

        "SegVideoGenerator": "utils.yolo_generator.seg_video",
        "DetSahiVideoGenerator": "utils.yolo_generator.det_sahi_video",
        "SegImageGenerator": "utils.yolo_generator.seg_image",
        "DetRTSPGenerator": "utils.yolo_generator.det_rtsp",
        "DetVisRTSPGenerator": "utils.yolo_generator.det_vis_rtsp",
        "SegRTSPGenerator": "utils.yolo_generator.seg_rtsp",
        "SegVisRTSPGenerator": "utils.yolo_generator.seg_vis_rtsp",
        "DetSahiRTSPGenerator": "utils.yolo_generator.det_sahi_rtsp",
        "DetSahiVisRTSPGenerator": "utils.yolo_generator.det_sahi_vis_rtsp",
        "SegSahiRTSPGenerator": "utils.yolo_generator.seg_sahi_rtsp",
        "SegSahiVisRTSPGenerator": "utils.yolo_generator.seg_sahi_vis_rtsp",
    }

    def __init__(self) -> None:
        self.loaded = {}

    def names(self) -> list:
        return sorted(self.MODULES)

    def contains(self, generator_name: str) -> bool:
        return generator_name in self.MODULES

    def resolve(self, generator_name: str):
        cls = self.loaded.get(generator_name)
        if cls is None:
            module = importlib.import_module(self.MODULES[generator_name])
            cls = getattr(module, generator_name)
            self.loaded[generator_name] = cls
        return cls
