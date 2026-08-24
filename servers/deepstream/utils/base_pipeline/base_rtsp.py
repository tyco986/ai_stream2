import json
from pathlib import Path

import yaml
from pyservicemaker import Pipeline

from utils.base_pipeline.utils.detection_attach import DetectionAttach
from utils.base_pipeline.utils.times_attach import LatencyTimesAttach
from utils.base_pipeline.utils.validate import nvinfer_period

PIPELINE_YML = "pipeline.yml"
PAD_LINKS_YML = "pad_links.yml"
META_JSON = "pgie_meta.json"
PGIE_YML = "pgie.yml"
SINK_PATH_YML = "sink_path.yml"


class BaseRTSPPipeline(LatencyTimesAttach, DetectionAttach):
    def __init__(self, config_dir, pipeline_name):
        self.config_dir = Path(config_dir)
        self.pipeline_name = pipeline_name
        self.meta, self.pgie, self.pad_links, self.sink_path = self.load_config(config_dir)
        self.yolo_task = self.meta["task"]
        self.pgie_interval = nvinfer_period(int(self.pgie["property"].get("interval", 0)))
        self.pipeline = Pipeline(pipeline_name, str(self.config_dir / PIPELINE_YML))
        self.link_demux_pads()

    @classmethod
    def load_config(cls, config_dir):
        config_dir = Path(config_dir)
        meta = json.loads((config_dir / META_JSON).read_text(encoding="utf-8"))
        pgie = yaml.safe_load((config_dir / PGIE_YML).read_text(encoding="utf-8"))
        pad_links = yaml.safe_load((config_dir / PAD_LINKS_YML).read_text(encoding="utf-8"))
        sink_path = yaml.safe_load((config_dir / SINK_PATH_YML).read_text(encoding="utf-8"))
        return meta, pgie, pad_links, sink_path

    def link_mux_pads(self):
        for src in self.pad_links.get("nvstreammux", []):
            self.pipeline.link((src, "nvstreammux"), ("", "sink_%u"))

    def link_demux_pads(self):
        for target in self.pad_links.get("nvstreamdemux", []):
            self.pipeline.link(("nvstreamdemux", target), ("src_%u", ""))

    def stream_indices(self):
        queue_names = self.pad_links.get("nvstreamdemux", [])
        indices = [int(name.removeprefix("queue_demux")) for name in queue_names]
        return indices

    def build(self):
        return self.pipeline

    def attach_latency_probe(self, target="nvvideoconvert"):
        self.pipeline.attach(target, "latency_probe", "latency")

    def attach_latency_and_times(self, logger, target="nvvideoconvert"):
        self.attach_latency_and_times_indexed(logger, self.stream_indices(), target)
