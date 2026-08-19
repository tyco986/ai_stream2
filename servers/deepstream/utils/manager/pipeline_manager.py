import importlib
import threading
from pathlib import Path

import yaml


class PipelineManager:
    SCHEMA_DIR = Path(__file__).resolve().parents[2] / "schemas"
    RUNNER_MODULE = "utils.base_pipeline.pipeline_runner"
    PIPELINES = {
        "BaseImagePipeline": "utils.base_pipeline.base_image",
        "BaseVideoPipeline": "utils.base_pipeline.base_video",
        "BaseRTSPPipeline": "utils.base_pipeline.base_rtsp",
        "DetVisRTSPPipeline": "utils.yolo_pipeline.det_vis_rtsp",
        "SegVisRTSPPipeline": "utils.yolo_pipeline.seg_vis_rtsp",
        "DetImagePipeline": "utils.yolo_pipeline.det_image",
        "Pose2DImagePipeline": "utils.pose_pipeline.pose2d_image",
        "Pose2DVideoPipeline": "utils.pose_pipeline.pose2d_video",
        "Pose2DRTSPPipeline": "utils.pose_pipeline.pose2d_rtsp",
        "Pose2DVisRTSPPipeline": "utils.pose_pipeline.pose2d_vis_rtsp",
        "SegImagePipeline": "utils.yolo_pipeline.seg_image",
        "SegSahiImagePipeline": "utils.yolo_pipeline.seg_sahi_image",
        "SegSahiVideoPipeline": "utils.yolo_pipeline.seg_sahi_video",
        "SegSahiVisRTSPPipeline": "utils.yolo_pipeline.seg_sahi_vis_rtsp",
        "DetVideoPipeline": "utils.yolo_pipeline.det_video",
        "SegVideoPipeline": "utils.yolo_pipeline.seg_video",
        "DetSahiVisRTSPPipeline": "utils.yolo_pipeline.det_sahi_vis_rtsp",
        "DetSahiImagePipeline": "utils.yolo_pipeline.det_sahi_image",
        "DetSahiVideoPipeline": "utils.yolo_pipeline.det_sahi_video",
        "PresenceRTSPPipeline": "utils.event_pipeline.presence_rtsp",
        "PresenceVideoPipeline": "utils.event_pipeline.presence_video",
    }
    BASE_TYPES = frozenset(
        {
            "BaseImagePipeline",
            "BaseVideoPipeline",
            "BaseRTSPPipeline",
        }
    )
    PRESENCE_TYPES = frozenset({"PresenceRTSPPipeline", "PresenceVideoPipeline"})
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
    ) -> dict:
        kwargs = {
            "logger": logger,
            "messager": messager,
        }
        if type in cls.PRESENCE_TYPES:
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
    ) -> None:
        module = importlib.import_module(cls.PIPELINES[type])
        builder_cls = getattr(module, type)
        if type in cls.BASE_TYPES:
            builder = builder_cls(config_dir, name)
        else:
            builder = builder_cls(
                config_dir,
                name,
                **cls.build_kwargs(type, logger, messager, drawer, debouncer, capturer),
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


PipelineManager.SCHEMAS = PipelineManager.build_schemas()
