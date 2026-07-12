from pyservicemaker import Probe

from utils.pipeline.base import BasePipeline
from utils.probe.yolo_probe import (
    DetImageProbe,
    DetRTSPProbe,
    DetSahiImageProbe,
    DetSahiRTSPProbe,
    DetSahiVideoProbe,
    DetVideoProbe,
    PoseImageProbe,
    PoseRTSPProbe,
    PoseVideoProbe,
    SegImageProbe,
    SegRTSPProbe,
    SegVideoProbe)

class DetRTSPPipeline(BasePipeline):
    def __init__(self, config_dir, pipeline_name, logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.logger = logger
        self.messager = messager

    def build(self):
        self.pipeline.attach(
            "analyzer",
            Probe("yolo", DetRTSPProbe(logger=self.logger, messager=self.messager)),
        )
        return self.pipeline


class SegRTSPPipeline(BasePipeline):
    def __init__(self, config_dir, pipeline_name, logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.logger = logger
        self.messager = messager

    def build(self):
        self.pipeline.attach(
            "analyzer",
            Probe("yolo", SegRTSPProbe(logger=self.logger, messager=self.messager)),
        )
        return self.pipeline


class PoseRTSPPipeline(BasePipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager

    def build(self):
        drawer = dict(self.drawer)
        if "infer_width" not in drawer or "infer_height" not in drawer:
            _, _, infer_width, infer_height = self.meta["input_tensor_shape"]
            drawer["infer_width"] = infer_width
            drawer["infer_height"] = infer_height
        self.pipeline.attach(
            "analyzer",
            Probe(
                "yolo",
                PoseRTSPProbe(drawer=drawer, logger=self.logger, messager=self.messager),
            ),
        )
        return self.pipeline


class DetImagePipeline(BasePipeline):
    def __init__(self, config_dir, pipeline_name, logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.logger = logger
        self.messager = messager

    def build(self):
        self.pipeline.attach(
            "analyzer",
            Probe("yolo", DetImageProbe(logger=self.logger, messager=self.messager)),
        )
        return self.pipeline


class SegImagePipeline(BasePipeline):
    def __init__(self, config_dir, pipeline_name, logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.logger = logger
        self.messager = messager

    def build(self):
        self.pipeline.attach(
            "analyzer",
            Probe("yolo", SegImageProbe(logger=self.logger, messager=self.messager)),
        )
        return self.pipeline


class PoseImagePipeline(BasePipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager

    def build(self):
        drawer = dict(self.drawer)
        if "infer_width" not in drawer or "infer_height" not in drawer:
            _, _, infer_width, infer_height = self.meta["input_tensor_shape"]
            drawer["infer_width"] = infer_width
            drawer["infer_height"] = infer_height
        self.pipeline.attach(
            "analyzer",
            Probe(
                "yolo",
                PoseImageProbe(drawer=drawer, logger=self.logger, messager=self.messager),
            ),
        )
        return self.pipeline


class DetVideoPipeline(BasePipeline):
    def __init__(self, config_dir, pipeline_name, logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.logger = logger
        self.messager = messager

    def build(self):
        self.pipeline.attach(
            "analyzer",
            Probe("yolo", DetVideoProbe(logger=self.logger, messager=self.messager)),
        )
        return self.pipeline


class SegVideoPipeline(BasePipeline):
    def __init__(self, config_dir, pipeline_name, logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.logger = logger
        self.messager = messager

    def build(self):
        self.pipeline.attach(
            "analyzer",
            Probe("yolo", SegVideoProbe(logger=self.logger, messager=self.messager)),
        )
        return self.pipeline


class PoseVideoPipeline(BasePipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager

    def build(self):
        drawer = dict(self.drawer)
        if "infer_width" not in drawer or "infer_height" not in drawer:
            _, _, infer_width, infer_height = self.meta["input_tensor_shape"]
            drawer["infer_width"] = infer_width
            drawer["infer_height"] = infer_height
        self.pipeline.attach(
            "analyzer",
            Probe(
                "yolo",
                PoseVideoProbe(drawer=drawer, logger=self.logger, messager=self.messager),
            ),
        )
        return self.pipeline


class DetSahiRTSPPipeline(BasePipeline):
    def __init__(self, config_dir, pipeline_name, logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.logger = logger
        self.messager = messager

    def build(self):
        self.pipeline.attach(
            "analyzer",
            Probe("yolo", DetSahiRTSPProbe(logger=self.logger, messager=self.messager)),
        )
        return self.pipeline


class DetSahiImagePipeline(BasePipeline):
    def __init__(self, config_dir, pipeline_name, logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.logger = logger
        self.messager = messager

    def build(self):
        self.pipeline.attach(
            "analyzer",
            Probe("yolo", DetSahiImageProbe(logger=self.logger, messager=self.messager)),
        )
        return self.pipeline


class DetSahiVideoPipeline(BasePipeline):
    def __init__(self, config_dir, pipeline_name, logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.logger = logger
        self.messager = messager

    def build(self):
        self.pipeline.attach(
            "analyzer",
            Probe("yolo", DetSahiVideoProbe(logger=self.logger, messager=self.messager)),
        )
        return self.pipeline
