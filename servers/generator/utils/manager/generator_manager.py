import importlib
from pathlib import Path

import yaml


class GeneratorManager:
    SCHEMA_DIR = Path(__file__).resolve().parents[2] / "schemas"
    GENERATORS = {
        "DetImageGenerator": "utils.yolo_generator.det_image",
        "DetSahiImageGenerator": "utils.yolo_generator.det_sahi_image",
        "SegSahiImageGenerator": "utils.yolo_generator.seg_sahi_image",
        "SegSahiVideoGenerator": "utils.yolo_generator.seg_sahi_video",
        "SegSahiVisVideoGenerator": "utils.yolo_generator.seg_sahi_vis_video",
        "DetVideoGenerator": "utils.yolo_generator.det_video",
        "DetVisVideoGenerator": "utils.yolo_generator.det_vis_video",
        "DetVisVideoPresenceGenerator": "utils.event_generator.det_vis_video_presence",
        "DetSahiVisVideoPresenceGenerator": "utils.event_generator.det_sahi_vis_video_presence",
        "DetRTSPPresenceGenerator": "utils.event_generator.det_rtsp_presence",
        "DetSahiRTSPPresenceGenerator": "utils.event_generator.det_sahi_rtsp_presence",
        "DetVisRTSPPresenceGenerator": "utils.event_generator.det_vis_rtsp_presence",
        "DetSahiVisRTSPPresenceGenerator": "utils.event_generator.det_sahi_vis_rtsp_presence",
        "SegVideoGenerator": "utils.yolo_generator.seg_video",
        "SegVisVideoGenerator": "utils.yolo_generator.seg_vis_video",
        "DetSahiVideoGenerator": "utils.yolo_generator.det_sahi_video",
        "DetSahiVisVideoGenerator": "utils.yolo_generator.det_sahi_vis_video",
        "SegImageGenerator": "utils.yolo_generator.seg_image",
        "DetRTSPGenerator": "utils.yolo_generator.det_rtsp",
        "DetVisRTSPGenerator": "utils.yolo_generator.det_vis_rtsp",
        "SegRTSPGenerator": "utils.yolo_generator.seg_rtsp",
        "SegVisRTSPGenerator": "utils.yolo_generator.seg_vis_rtsp",
        "DetSahiRTSPGenerator": "utils.yolo_generator.det_sahi_rtsp",
        "DetSahiVisRTSPGenerator": "utils.yolo_generator.det_sahi_vis_rtsp",
        "SegSahiRTSPGenerator": "utils.yolo_generator.seg_sahi_rtsp",
        "SegSahiVisRTSPGenerator": "utils.yolo_generator.seg_sahi_vis_rtsp",
        "PoseImageGenerator": "utils.yolo_generator.pose_image",
        "PoseVideoGenerator": "utils.yolo_generator.pose_video",
        "PoseVisVideoGenerator": "utils.yolo_generator.pose_vis_video",
        "PoseRTSPGenerator": "utils.yolo_generator.pose_rtsp",
        "PoseVisRTSPGenerator": "utils.yolo_generator.pose_vis_rtsp",
        "PoseSahiImageGenerator": "utils.yolo_generator.pose_sahi_image",
        "PoseSahiVideoGenerator": "utils.yolo_generator.pose_sahi_video",
        "PoseSahiVisVideoGenerator": "utils.yolo_generator.pose_sahi_vis_video",
        "PoseSahiRTSPGenerator": "utils.yolo_generator.pose_sahi_rtsp",
        "PoseSahiVisRTSPGenerator": "utils.yolo_generator.pose_sahi_vis_rtsp",
        "TopdownPoseImageGenerator": "utils.topdown_pose_generator.topdown_pose_image_generator",
        "TopdownPoseVideoGenerator": "utils.topdown_pose_generator.topdown_pose_video_generator",
        "TopdownPoseVisVideoGenerator": "utils.topdown_pose_generator.topdown_pose_vis_video_generator",
        "TopdownPoseRTSPGenerator": "utils.topdown_pose_generator.topdown_pose_rtsp_generator",
        "TopdownPoseVisRTSPGenerator": "utils.topdown_pose_generator.topdown_pose_vis_rtsp_generator",
        "TopdownPoseSahiImageGenerator": "utils.topdown_pose_generator.topdown_pose_sahi_image_generator",
        "TopdownPoseSahiVideoGenerator": "utils.topdown_pose_generator.topdown_pose_sahi_video_generator",
        "TopdownPoseSahiVisVideoGenerator": "utils.topdown_pose_generator.topdown_pose_sahi_vis_video_generator",
        "TopdownPoseSahiRTSPGenerator": "utils.topdown_pose_generator.topdown_pose_sahi_rtsp_generator",
        "TopdownPoseSahiVisRTSPGenerator": "utils.topdown_pose_generator.topdown_pose_sahi_vis_rtsp_generator",
        "StgcnppRtmposeVideoGenerator": "utils.stgcnpp_rtmpose_generator.stgcnpp_rtmpose_video_generator",
        "StgcnppRtmposeVisVideoGenerator": "utils.stgcnpp_rtmpose_generator.stgcnpp_rtmpose_vis_video_generator",
        "StgcnppRtmposeRTSPGenerator": "utils.stgcnpp_rtmpose_generator.stgcnpp_rtmpose_rtsp_generator",
        "StgcnppRtmposeVisRTSPGenerator": "utils.stgcnpp_rtmpose_generator.stgcnpp_rtmpose_vis_rtsp_generator",
        "StgcnppRtmposeSahiVideoGenerator": "utils.stgcnpp_rtmpose_generator.stgcnpp_rtmpose_sahi_video_generator",
        "StgcnppRtmposeSahiVisVideoGenerator": "utils.stgcnpp_rtmpose_generator.stgcnpp_rtmpose_sahi_vis_video_generator",
        "StgcnppRtmposeSahiRTSPGenerator": "utils.stgcnpp_rtmpose_generator.stgcnpp_rtmpose_sahi_rtsp_generator",
        "StgcnppRtmposeSahiVisRTSPGenerator": "utils.stgcnpp_rtmpose_generator.stgcnpp_rtmpose_sahi_vis_rtsp_generator",
        "StgcnppYoloPoseVideoGenerator": "utils.stgcnpp_yolo_pose_generator.stgcnpp_yolo_pose_video_generator",
        "StgcnppYoloPoseVisVideoGenerator": "utils.stgcnpp_yolo_pose_generator.stgcnpp_yolo_pose_vis_video_generator",
        "StgcnppYoloPoseRTSPGenerator": "utils.stgcnpp_yolo_pose_generator.stgcnpp_yolo_pose_rtsp_generator",
        "StgcnppYoloPoseVisRTSPGenerator": "utils.stgcnpp_yolo_pose_generator.stgcnpp_yolo_pose_vis_rtsp_generator",
        "StgcnppYoloPoseSahiVideoGenerator": "utils.stgcnpp_yolo_pose_generator.stgcnpp_yolo_pose_sahi_video_generator",
        "StgcnppYoloPoseSahiVisVideoGenerator": "utils.stgcnpp_yolo_pose_generator.stgcnpp_yolo_pose_sahi_vis_video_generator",
        "StgcnppYoloPoseSahiRTSPGenerator": "utils.stgcnpp_yolo_pose_generator.stgcnpp_yolo_pose_sahi_rtsp_generator",
        "StgcnppYoloPoseSahiVisRTSPGenerator": "utils.stgcnpp_yolo_pose_generator.stgcnpp_yolo_pose_sahi_vis_rtsp_generator",
    }
    SCHEMAS = {}

    @classmethod
    def types(cls) -> list[str]:
        names = list(cls.GENERATORS)
        return names

    @classmethod
    def build_schemas(cls) -> dict:
        schemas = {}
        for path in sorted(cls.SCHEMA_DIR.glob("*.yaml")):
            data = yaml.safe_load(path.read_text(encoding="utf-8"))
            if not isinstance(data, dict):
                raise RuntimeError(f"schema YAML must be a mapping: {path}")
            generator_name = data.get("type")
            if not generator_name:
                raise RuntimeError(f"schema YAML missing type: {path}")
            if generator_name in schemas:
                raise RuntimeError(f"duplicate schema generator: {generator_name}")
            schemas[generator_name] = data
        registered = set(cls.GENERATORS)
        indexed = set(schemas)
        missing = sorted(registered - indexed)
        extra = sorted(indexed - registered)
        if missing or extra:
            raise RuntimeError(
                f"schema coverage mismatch missing={missing} extra={extra}"
            )
        return schemas

    def generate(self, type: str, config_save_dir: Path, **kwargs) -> None:
        module = importlib.import_module(self.GENERATORS[type])
        generator_cls = getattr(module, type)
        generator_cls(**kwargs).write(config_save_dir)

    def schema(self, type: str) -> dict:
        data = self.SCHEMAS[type]
        return data


GeneratorManager.SCHEMAS = GeneratorManager.build_schemas()
