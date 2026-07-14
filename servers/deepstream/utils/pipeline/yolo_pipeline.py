from pyservicemaker import Probe

from utils.pipeline.base import BaseImagePipeline, BaseRTSPPipeline, BaseVideoPipeline
from utils.probe.det_probe import DetImageProbe, DetRTSPProbe, DetVideoProbe
from utils.probe.det_sahi_probe import DetSahiImageProbe, DetSahiRTSPProbe, DetSahiVideoProbe
from utils.probe.pose_probe import PoseImageProbe, PoseRTSPProbe, PoseVideoProbe
from utils.probe.seg_probe import SegImageProbe, SegRTSPProbe, SegVideoProbe


def validate_log_interval(pgie_interval, logger):
    log_interval = int(logger.get("interval", 0))
    if pgie_interval > 0:
        assert log_interval % pgie_interval == 0, "log_interval must be a multiple of pgie_interval"


class DetRTSPPipeline(BaseRTSPPipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        validate_log_interval(self.pgie_interval, logger)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager

    def build(self):
        self.attach_branch_probes(
            "yolo",
            lambda index: DetRTSPProbe(
                drawer=self.drawer,
                logger=self.logger,
                messager=self.messager,
            ),
        )
        return self.pipeline


class SegRTSPPipeline(BaseRTSPPipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        validate_log_interval(self.pgie_interval, logger)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager

    def build(self):
        self.attach_branch_probes(
            "yolo",
            lambda index: SegRTSPProbe(
                drawer=self.drawer,
                logger=self.logger,
                messager=self.messager,
            ),
        )
        return self.pipeline


class PoseRTSPPipeline(BaseRTSPPipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        validate_log_interval(self.pgie_interval, logger)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager
        self.drawer = self.resolve_drawer(drawer)

    def resolve_drawer(self, drawer):
        drawer = dict(drawer)
        if "infer_width" not in drawer or "infer_height" not in drawer:
            _, _, infer_width, infer_height = self.meta["input_tensor_shape"]
            drawer["infer_width"] = infer_width
            drawer["infer_height"] = infer_height
        return drawer

    def build(self):
        self.attach_branch_probes(
            "yolo",
            lambda index: PoseRTSPProbe(
                drawer=self.drawer,
                logger=self.logger,
                messager=self.messager,
            ),
        )
        return self.pipeline


class DetImagePipeline(BaseImagePipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        validate_log_interval(self.pgie_interval, logger)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager

    def build(self):
        self.pipeline.attach(
            "pgie",
            Probe(
                "yolo",
                DetImageProbe(drawer=self.drawer, logger=self.logger, messager=self.messager),
            ),
        )
        return self.pipeline


class SegImagePipeline(BaseImagePipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        validate_log_interval(self.pgie_interval, logger)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager

    def build(self):
        self.pipeline.attach(
            "pgie",
            Probe(
                "yolo",
                SegImageProbe(drawer=self.drawer, logger=self.logger, messager=self.messager),
            ),
        )
        return self.pipeline


class PoseImagePipeline(BaseImagePipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        validate_log_interval(self.pgie_interval, logger)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager
        self.drawer = self.resolve_drawer(drawer)

    def resolve_drawer(self, drawer):
        drawer = dict(drawer)
        if "infer_width" not in drawer or "infer_height" not in drawer:
            _, _, infer_width, infer_height = self.meta["input_tensor_shape"]
            drawer["infer_width"] = infer_width
            drawer["infer_height"] = infer_height
        return drawer

    def build(self):
        self.pipeline.attach(
            "pgie",
            Probe(
                "yolo",
                PoseImageProbe(drawer=self.drawer, logger=self.logger, messager=self.messager),
            ),
        )
        return self.pipeline


class DetVideoPipeline(BaseVideoPipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        validate_log_interval(self.pgie_interval, logger)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager

    def build(self):
        self.pipeline.attach(
            "pgie",
            Probe(
                "yolo",
                DetVideoProbe(drawer=self.drawer, logger=self.logger, messager=self.messager),
            ),
        )
        return self.pipeline


class SegVideoPipeline(BaseVideoPipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        validate_log_interval(self.pgie_interval, logger)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager

    def build(self):
        self.pipeline.attach(
            "pgie",
            Probe(
                "yolo",
                SegVideoProbe(drawer=self.drawer, logger=self.logger, messager=self.messager),
            ),
        )
        return self.pipeline


class PoseVideoPipeline(BaseVideoPipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        validate_log_interval(self.pgie_interval, logger)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager
        self.drawer = self.resolve_drawer(drawer)

    def resolve_drawer(self, drawer):
        drawer = dict(drawer)
        if "infer_width" not in drawer or "infer_height" not in drawer:
            _, _, infer_width, infer_height = self.meta["input_tensor_shape"]
            drawer["infer_width"] = infer_width
            drawer["infer_height"] = infer_height
        return drawer

    def build(self):
        self.pipeline.attach(
            "pgie",
            Probe(
                "yolo",
                PoseVideoProbe(drawer=self.drawer, logger=self.logger, messager=self.messager),
            ),
        )
        return self.pipeline


class DetSahiRTSPPipeline(BaseRTSPPipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        validate_log_interval(self.pgie_interval, logger)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager

    def build(self):
        self.attach_branch_probes(
            "yolo",
            lambda index: DetSahiRTSPProbe(
                drawer=self.drawer,
                logger=self.logger,
                messager=self.messager,
            ),
        )
        return self.pipeline


class DetSahiImagePipeline(BaseImagePipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        validate_log_interval(self.pgie_interval, logger)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager

    def build(self):
        self.pipeline.attach(
            "sahi_postprocess",
            Probe(
                "yolo",
                DetSahiImageProbe(drawer=self.drawer, logger=self.logger, messager=self.messager),
            ),
        )
        return self.pipeline


class DetSahiVideoPipeline(BaseVideoPipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        validate_log_interval(self.pgie_interval, logger)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager

    def build(self):
        self.pipeline.attach(
            "sahi_postprocess",
            Probe(
                "yolo",
                DetSahiVideoProbe(drawer=self.drawer, logger=self.logger, messager=self.messager),
            ),
        )
        return self.pipeline
