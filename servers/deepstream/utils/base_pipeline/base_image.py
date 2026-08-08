import json
from pathlib import Path

import yaml
from pyservicemaker import Pipeline, Probe

from utils.base_pipeline.times_attach import LatencyTimesAttach

PIPELINE_YML = "pipeline.yml"
META_JSON = "meta.json"
PGIE_YML = "pgie.yml"
SINK_PATH_YML = "sink_path.yml"


class BaseImagePipeline(LatencyTimesAttach):
    def __init__(self, config_dir, pipeline_name):
        self.config_dir = Path(config_dir)
        self.pipeline_name = pipeline_name
        self.meta, self.pgie, self.sink_path = self.load_config(config_dir)
        self.yolo_task = self.meta["task"]
        self.pgie_interval = int(self.pgie["property"].get("interval", 0))
        self.pipeline = Pipeline(pipeline_name, str(self.config_dir / PIPELINE_YML))

    @classmethod
    def load_config(cls, config_dir):
        config_dir = Path(config_dir)
        meta = json.loads((config_dir / META_JSON).read_text(encoding="utf-8"))
        pgie = yaml.safe_load((config_dir / PGIE_YML).read_text(encoding="utf-8"))
        sink_path = yaml.safe_load((config_dir / SINK_PATH_YML).read_text(encoding="utf-8"))
        return meta, pgie, sink_path

    def build(self):
        return self.pipeline

    def attach_latency_probe(self, target="nvvideoconvert"):
        self.pipeline.attach(target, "latency_probe", "latency")

    def attach_nvdsanalytics_probe(self, name, operator):
        self.pipeline.attach("nvdsanalytics", Probe(name, operator))
