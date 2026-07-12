import json
from pathlib import Path

import yaml
from pyservicemaker import Pipeline

PIPELINE_YML = "pipeline.yml"
PAD_LINKS_YML = "pad_links.yml"
META_JSON = "meta.json"
PGIE_YML = "pgie.yml"


class BasePipeline:
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

    def link_demux_pads(self):
        for target in self.pad_links.get("demux", []):
            self.pipeline.link(("demux", target), ("src_%u", ""))
