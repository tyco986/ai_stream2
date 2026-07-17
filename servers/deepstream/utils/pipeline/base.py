import json
from pathlib import Path

import yaml
from pyservicemaker import Pipeline, Probe

PIPELINE_YML = "pipeline.yml"
PAD_LINKS_YML = "pad_links.yml"
META_JSON = "meta.json"
PGIE_YML = "pgie.yml"


def validate_probe_interval(pgie_interval: int, probe_interval: int) -> None:
    probe_interval = int(probe_interval)
    if pgie_interval > 0 and probe_interval > 0:
        assert probe_interval % pgie_interval == 0, (
            f"probe interval ({probe_interval}) must be a multiple of "
            f"pgie interval ({pgie_interval})"
        )


class BaseImagePipeline:
    def __init__(self, config_dir, pipeline_name):
        self.config_dir = Path(config_dir)
        self.pipeline_name = pipeline_name
        self.meta, self.pgie = self.load_config(config_dir)
        self.yolo_task = self.meta["task"]
        self.pgie_interval = int(self.pgie["property"].get("interval", 0))
        self.pipeline = Pipeline(pipeline_name, str(self.config_dir / PIPELINE_YML))

    @classmethod
    def load_config(cls, config_dir):
        config_dir = Path(config_dir)
        meta = json.loads((config_dir / META_JSON).read_text(encoding="utf-8"))
        pgie = yaml.safe_load((config_dir / PGIE_YML).read_text(encoding="utf-8"))
        return meta, pgie

    def build(self):
        return self.pipeline


class BaseVideoPipeline:
    def __init__(self, config_dir, pipeline_name):
        self.config_dir = Path(config_dir)
        self.pipeline_name = pipeline_name
        self.meta, self.pgie = self.load_config(config_dir)
        self.yolo_task = self.meta["task"]
        self.pgie_interval = int(self.pgie["property"].get("interval", 0))
        self.pipeline = Pipeline(pipeline_name, str(self.config_dir / PIPELINE_YML))

    @classmethod
    def load_config(cls, config_dir):
        config_dir = Path(config_dir)
        meta = json.loads((config_dir / META_JSON).read_text(encoding="utf-8"))
        pgie = yaml.safe_load((config_dir / PGIE_YML).read_text(encoding="utf-8"))
        return meta, pgie

    def build(self):
        return self.pipeline


class BaseRTSPPipeline:
    def __init__(self, config_dir, pipeline_name):
        self.config_dir = Path(config_dir)
        self.pipeline_name = pipeline_name
        self.meta, self.pgie, self.pad_links = self.load_config(config_dir)
        self.yolo_task = self.meta["task"]
        self.pgie_interval = int(self.pgie["property"].get("interval", 0))
        self.pipeline = Pipeline(pipeline_name, str(self.config_dir / PIPELINE_YML))
        self.link_demux_pads()

    @classmethod
    def load_config(cls, config_dir):
        config_dir = Path(config_dir)
        meta = json.loads((config_dir / META_JSON).read_text(encoding="utf-8"))
        pgie = yaml.safe_load((config_dir / PGIE_YML).read_text(encoding="utf-8"))
        pad_links = yaml.safe_load((config_dir / PAD_LINKS_YML).read_text(encoding="utf-8"))
        return meta, pgie, pad_links

    def link_mux_pads(self):
        for src in self.pad_links.get("mux", []):
            self.pipeline.link((src, "mux"), ("", "sink_%u"))

    def link_demux_pads(self):
        for target in self.pad_links.get("demux", []):
            self.pipeline.link(("demux", target), ("src_%u", ""))

    def stream_indices(self):
        queue_names = self.pad_links.get("demux", [])
        indices = [int(name.removeprefix("queue_demux")) for name in queue_names]
        return indices

    def build(self):
        return self.pipeline

    def attach_analyzer_probe(self, name, operator):
        self.pipeline.attach("analyzer", Probe(name, operator))


class BaseRTSPFakeSinkPipeline:
    def __init__(self, config_dir, pipeline_name):
        self.config_dir = Path(config_dir)
        self.pipeline_name = pipeline_name
        self.meta, self.pgie = self.load_config(config_dir)
        self.yolo_task = self.meta["task"]
        self.pgie_interval = int(self.pgie["property"].get("interval", 0))
        self.pipeline = Pipeline(pipeline_name, str(self.config_dir / PIPELINE_YML))

    @classmethod
    def load_config(cls, config_dir):
        config_dir = Path(config_dir)
        meta = json.loads((config_dir / META_JSON).read_text(encoding="utf-8"))
        pgie = yaml.safe_load((config_dir / PGIE_YML).read_text(encoding="utf-8"))
        return meta, pgie

    def build(self):
        return self.pipeline
