from pyservicemaker import Probe

from utils.pipeline.base import BaseImagePipeline, BaseRTSPPipeline, BaseVideoPipeline, validate_probe_interval
from utils.probe.det_probe import DetImageProbe, DetVisRTSPProbe, DetVideoProbe
from utils.probe.det_sahi_probe import DetSahiImageProbe, DetSahiVisRTSPProbe, DetSahiVideoProbe
from utils.probe.seg_probe import SegImageProbe, SegVisRTSPProbe, SegVideoProbe
from utils.probe.seg_sahi_probe import SegSahiImageProbe, SegSahiVisRTSPProbe, SegSahiVideoProbe


class DetVisRTSPPipeline(BaseRTSPPipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager
        validate_probe_interval(self.pgie_interval, self.messager.get("interval", 0))
        validate_probe_interval(self.pgie_interval, self.logger.get("interval", 0))

    def build(self):
        self.attach_analyzer_probe(
            "yolo",
            DetVisRTSPProbe(
                drawer=self.drawer,
                logger=self.logger,
                messager=self.messager,
            ),
        )
        return self.pipeline


class SegVisRTSPPipeline(BaseRTSPPipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager
        validate_probe_interval(self.pgie_interval, self.messager.get("interval", 0))
        validate_probe_interval(self.pgie_interval, self.logger.get("interval", 0))

    def build(self):
        self.attach_analyzer_probe(
            "yolo",
            SegVisRTSPProbe(
                drawer=self.drawer,
                logger=self.logger,
                messager=self.messager,
            ),
        )
        return self.pipeline


class DetImagePipeline(BaseImagePipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager
        validate_probe_interval(self.pgie_interval, self.messager.get("interval", 0))
        validate_probe_interval(self.pgie_interval, self.logger.get("interval", 0))

    def build(self):
        self.pipeline.attach(
            "analyzer",
            Probe(
                "yolo",
                DetImageProbe(drawer=self.drawer, logger=self.logger, messager=self.messager),
            ),
        )
        return self.pipeline


class SegImagePipeline(BaseImagePipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager
        validate_probe_interval(self.pgie_interval, self.messager.get("interval", 0))
        validate_probe_interval(self.pgie_interval, self.logger.get("interval", 0))

    def build(self):
        self.pipeline.attach(
            "analyzer",
            Probe(
                "yolo",
                SegImageProbe(drawer=self.drawer, logger=self.logger, messager=self.messager),
            ),
        )
        return self.pipeline


class DetVideoPipeline(BaseVideoPipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager
        validate_probe_interval(self.pgie_interval, self.messager.get("interval", 0))
        validate_probe_interval(self.pgie_interval, self.logger.get("interval", 0))

    def build(self):
        self.pipeline.attach(
            "analyzer",
            Probe(
                "yolo",
                DetVideoProbe(drawer=self.drawer, logger=self.logger, messager=self.messager),
            ),
        )
        return self.pipeline


class SegVideoPipeline(BaseVideoPipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager
        validate_probe_interval(self.pgie_interval, self.messager.get("interval", 0))
        validate_probe_interval(self.pgie_interval, self.logger.get("interval", 0))

    def build(self):
        assert self.pgie_interval == 0, "pgie interval other than 0 is not supported"
        self.pipeline.attach(
            "analyzer",
            Probe(
                "yolo",
                SegVideoProbe(drawer=self.drawer, logger=self.logger, messager=self.messager),
            ),
        )
        return self.pipeline


class DetSahiVisRTSPPipeline(BaseRTSPPipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager
        validate_probe_interval(self.pgie_interval, self.messager.get("interval", 0))
        validate_probe_interval(self.pgie_interval, self.logger.get("interval", 0))

    def build(self):
        self.attach_analyzer_probe(
            "yolo",
            DetSahiVisRTSPProbe(
                drawer=self.drawer,
                logger=self.logger,
                messager=self.messager,
            ),
        )
        return self.pipeline


class DetSahiImagePipeline(BaseImagePipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager
        validate_probe_interval(self.pgie_interval, self.messager.get("interval", 0))
        validate_probe_interval(self.pgie_interval, self.logger.get("interval", 0))

    def build(self):
        self.pipeline.attach(
            "analyzer",
            Probe(
                "yolo",
                DetSahiImageProbe(drawer=self.drawer, logger=self.logger, messager=self.messager),
            ),
        )
        return self.pipeline


class SegSahiImagePipeline(BaseImagePipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager
        validate_probe_interval(self.pgie_interval, self.messager.get("interval", 0))
        validate_probe_interval(self.pgie_interval, self.logger.get("interval", 0))

    def build(self):
        self.pipeline.attach(
            "analyzer",
            Probe(
                "yolo",
                SegSahiImageProbe(drawer=self.drawer, logger=self.logger, messager=self.messager),
            ),
        )
        return self.pipeline


class SegSahiVideoPipeline(BaseVideoPipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager
        validate_probe_interval(self.pgie_interval, self.messager.get("interval", 0))
        validate_probe_interval(self.pgie_interval, self.logger.get("interval", 0))

    def build(self):
        self.pipeline.attach(
            "analyzer",
            Probe(
                "yolo",
                SegSahiVideoProbe(drawer=self.drawer, logger=self.logger, messager=self.messager),
            ),
        )
        return self.pipeline


class SegSahiVisRTSPPipeline(BaseRTSPPipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager
        validate_probe_interval(self.pgie_interval, self.messager.get("interval", 0))
        validate_probe_interval(self.pgie_interval, self.logger.get("interval", 0))

    def build(self):
        self.attach_analyzer_probe(
            "yolo",
            SegSahiVisRTSPProbe(
                drawer=self.drawer,
                logger=self.logger,
                messager=self.messager,
            ),
        )
        return self.pipeline


class DetSahiVideoPipeline(BaseVideoPipeline):
    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.drawer = drawer
        self.logger = logger
        self.messager = messager
        validate_probe_interval(self.pgie_interval, self.messager.get("interval", 0))
        validate_probe_interval(self.pgie_interval, self.logger.get("interval", 0))

    def build(self):
        self.pipeline.attach(
            "analyzer",
            Probe(
                "yolo",
                DetSahiVideoProbe(drawer=self.drawer, logger=self.logger, messager=self.messager),
            ),
        )
        return self.pipeline
