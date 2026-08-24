import importlib
import threading
from pathlib import Path

import yaml


class PipelineManager:
    SCHEMA_DIR = Path(__file__).resolve().parents[2] / "schemas"
    RUNNER_MODULE = "utils.base_pipeline.utils.pipeline_runner"
    PIPELINES = {
        "BaseImagePipeline": "utils.base_pipeline.base_image",
        "BaseVideoPipeline": "utils.base_pipeline.base_video",
        "BaseRTSPPipeline": "utils.base_pipeline.base_rtsp",
        "DetImagePipeline": "utils.yolo_pipeline.det_image",
        "DetSahiImagePipeline": "utils.yolo_pipeline.det_sahi_image",
        "DetVideoPipeline": "utils.yolo_pipeline.det_video",
        "DetVisVideoPipeline": "utils.yolo_pipeline.det_vis_video",
        "DetSahiVideoPipeline": "utils.yolo_pipeline.det_sahi_video",
        "DetSahiVisVideoPipeline": "utils.yolo_pipeline.det_sahi_vis_video",
        "DetRTSPPipeline": "utils.yolo_pipeline.det_rtsp",
        "DetVisRTSPPipeline": "utils.yolo_pipeline.det_vis_rtsp",
        "DetSahiRTSPPipeline": "utils.yolo_pipeline.det_sahi_rtsp",
        "DetSahiVisRTSPPipeline": "utils.yolo_pipeline.det_sahi_vis_rtsp",
        "SegImagePipeline": "utils.yolo_pipeline.seg_image",
        "SegSahiImagePipeline": "utils.yolo_pipeline.seg_sahi_image",
        "SegVideoPipeline": "utils.yolo_pipeline.seg_video",
        "SegVisVideoPipeline": "utils.yolo_pipeline.seg_vis_video",
        "SegSahiVideoPipeline": "utils.yolo_pipeline.seg_sahi_video",
        "SegSahiVisVideoPipeline": "utils.yolo_pipeline.seg_sahi_vis_video",
        "SegRTSPPipeline": "utils.yolo_pipeline.seg_rtsp",
        "SegVisRTSPPipeline": "utils.yolo_pipeline.seg_vis_rtsp",
        "SegSahiRTSPPipeline": "utils.yolo_pipeline.seg_sahi_rtsp",
        "SegSahiVisRTSPPipeline": "utils.yolo_pipeline.seg_sahi_vis_rtsp",
        "PoseImagePipeline": "utils.yolo_pipeline.pose_image",
        "PoseSahiImagePipeline": "utils.yolo_pipeline.pose_sahi_image",
        "PoseVideoPipeline": "utils.yolo_pipeline.pose_video",
        "PoseVisVideoPipeline": "utils.yolo_pipeline.pose_vis_video",
        "PoseSahiVideoPipeline": "utils.yolo_pipeline.pose_sahi_video",
        "PoseSahiVisVideoPipeline": "utils.yolo_pipeline.pose_sahi_vis_video",
        "PoseRTSPPipeline": "utils.yolo_pipeline.pose_rtsp",
        "PoseVisRTSPPipeline": "utils.yolo_pipeline.pose_vis_rtsp",
        "PoseSahiRTSPPipeline": "utils.yolo_pipeline.pose_sahi_rtsp",
        "PoseSahiVisRTSPPipeline": "utils.yolo_pipeline.pose_sahi_vis_rtsp",
        "RtmposeImagePipeline": "utils.rtmpose_pipeline.rtmpose_image",
        "RtmposeSahiImagePipeline": "utils.rtmpose_pipeline.rtmpose_sahi_image",
        "RtmposeVideoPipeline": "utils.rtmpose_pipeline.rtmpose_video",
        "RtmposeVisVideoPipeline": "utils.rtmpose_pipeline.rtmpose_vis_video",
        "RtmposeSahiVideoPipeline": "utils.rtmpose_pipeline.rtmpose_sahi_video",
        "RtmposeSahiVisVideoPipeline": "utils.rtmpose_pipeline.rtmpose_sahi_vis_video",
        "RtmposeRTSPPipeline": "utils.rtmpose_pipeline.rtmpose_rtsp",
        "RtmposeVisRTSPPipeline": "utils.rtmpose_pipeline.rtmpose_vis_rtsp",
        "RtmposeSahiRTSPPipeline": "utils.rtmpose_pipeline.rtmpose_sahi_rtsp",
        "RtmposeSahiVisRTSPPipeline": "utils.rtmpose_pipeline.rtmpose_sahi_vis_rtsp",
        "StgcnppImagePipeline": "utils.stgcnpp_pipeline.stgcnpp_image",
        "StgcnppVideoPipeline": "utils.stgcnpp_pipeline.stgcnpp_video",
        "StgcnppVisVideoPipeline": "utils.stgcnpp_pipeline.stgcnpp_vis_video",
        "StgcnppSahiVideoPipeline": "utils.stgcnpp_pipeline.stgcnpp_sahi_video",
        "StgcnppSahiVisVideoPipeline": "utils.stgcnpp_pipeline.stgcnpp_sahi_vis_video",
        "StgcnppRTSPPipeline": "utils.stgcnpp_pipeline.stgcnpp_rtsp",
        "StgcnppVisRTSPPipeline": "utils.stgcnpp_pipeline.stgcnpp_vis_rtsp",
        "StgcnppSahiRTSPPipeline": "utils.stgcnpp_pipeline.stgcnpp_sahi_rtsp",
        "StgcnppSahiVisRTSPPipeline": "utils.stgcnpp_pipeline.stgcnpp_sahi_vis_rtsp",
        "PresenceVisVideoPipeline": "utils.event_pipeline.presence_vis_video",
        "PresenceSahiVisVideoPipeline": "utils.event_pipeline.presence_sahi_vis_video",
        "PresenceRTSPPipeline": "utils.event_pipeline.presence_rtsp",
        "PresenceVisRTSPPipeline": "utils.event_pipeline.presence_vis_rtsp",
        "PresenceSahiRTSPPipeline": "utils.event_pipeline.presence_sahi_rtsp",
        "PresenceSahiVisRTSPPipeline": "utils.event_pipeline.presence_sahi_vis_rtsp",
    }
    BASE_TYPES = frozenset(
        {
            "BaseImagePipeline",
            "BaseVideoPipeline",
            "BaseRTSPPipeline",
        }
    )
    PRESENCE_TYPES = frozenset(
        {
            "PresenceVisVideoPipeline",
            "PresenceSahiVisVideoPipeline",
            "PresenceRTSPPipeline",
            "PresenceVisRTSPPipeline",
            "PresenceSahiRTSPPipeline",
            "PresenceSahiVisRTSPPipeline",
        }
    )
    SCHEMAS = {}
    pipeline = None
    runner = None
    runner_thread = None
    pipeline_name = None
    pipeline_type = None

    @classmethod
    def types(cls) -> list[str]:
        names = list(cls.PIPELINES)
        return names

    @classmethod
    def build_schemas(cls) -> dict:
        schemas = {}
        for path in sorted(cls.SCHEMA_DIR.glob("*.yaml")):
            data = yaml.safe_load(path.read_text(encoding="utf-8"))
            if not isinstance(data, dict):
                raise RuntimeError(f"schema YAML must be a mapping: {path}")
            pipeline_type = data.get("type")
            if not pipeline_type:
                raise RuntimeError(f"schema YAML missing type: {path}")
            if pipeline_type in schemas:
                raise RuntimeError(f"duplicate schema type: {pipeline_type}")
            schemas[pipeline_type] = data
        registered = set(cls.PIPELINES)
        indexed = set(schemas)
        missing = sorted(registered - indexed)
        extra = sorted(indexed - registered)
        if missing or extra:
            raise RuntimeError(
                f"schema coverage mismatch missing={missing} extra={extra}"
            )
        return schemas

    @classmethod
    def is_running(cls) -> bool:
        running = cls.runner_thread is not None and cls.runner_thread.is_alive()
        return running

    @classmethod
    def status(cls) -> dict:
        data = {
            "pipeline_running": cls.is_running(),
            "name": cls.pipeline_name,
            "type": cls.pipeline_type,
        }
        return data

    @classmethod
    def build_kwargs(
        cls,
        type: str,
        logger: dict,
        messager: dict,
        drawer: dict | None,
        debouncer: dict | None,
        capturer: dict | None,
        parser: dict | None = None,
    ) -> dict:
        kwargs = {
            "logger": logger,
            "messager": messager,
        }
        if type in cls.PARSER_TYPES:
            kwargs["parser"] = parser if parser is not None else {}
        elif type in cls.PRESENCE_TYPES:
            if debouncer is not None:
                kwargs["debouncer"] = debouncer
            if drawer is not None:
                kwargs["drawer"] = drawer
            if capturer is not None:
                kwargs["capturer"] = capturer
        elif drawer is not None:
            kwargs["drawer"] = drawer
        return kwargs

    @classmethod
    def start(
        cls,
        type: str,
        name: str,
        config_dir: str,
        logger: dict,
        messager: dict,
        drawer: dict | None,
        debouncer: dict | None,
        capturer: dict | None,
        parser: dict | None = None,
    ) -> None:
        module = importlib.import_module(cls.PIPELINES[type])
        builder_cls = getattr(module, type)
        if type in cls.BASE_TYPES:
            builder = builder_cls(config_dir, name)
        else:
            builder = builder_cls(
                config_dir,
                name,
                **cls.build_kwargs(
                    type, logger, messager, drawer, debouncer, capturer, parser
                ),
            )
        runner_module = importlib.import_module(cls.RUNNER_MODULE)
        runner_cls = getattr(runner_module, "PipelineRunner")
        cls.pipeline = builder.build()
        cls.runner = runner_cls(cls.pipeline, logger=logger)
        cls.pipeline_name = name
        cls.pipeline_type = type
        cls.runner_thread = threading.Thread(target=cls.runner.start, daemon=True)
        cls.runner_thread.start()

    def schema(self, type: str) -> dict:
        data = self.SCHEMAS[type]
        return data


PipelineManager.PARSER_TYPES = frozenset(
    name
    for name in PipelineManager.PIPELINES
    if name not in PipelineManager.BASE_TYPES
    and name not in PipelineManager.PRESENCE_TYPES
    and "Image" not in name
    and "Vis" not in name
)
PipelineManager.SCHEMAS = PipelineManager.build_schemas()
