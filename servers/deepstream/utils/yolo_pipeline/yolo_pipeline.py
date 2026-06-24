import json
import os
from pathlib import Path

import yaml
from pyservicemaker import Pipeline, Probe

from utils.yolo_pipeline.yolo_drawer import YoloDrawerBase, YoloPoseDrawer, YoloSegDrawer
from utils.yolo_pipeline.yolo_logger import DEFAULT_INFERENCE_LOG_INTERVAL, YoloInferenceProbe

PIPELINE_YML = "pipeline.yml"
PAD_LINKS_YML = "pad_links.yml"
META_JSON = "meta.json"

TASK_DRAWER_MAP = {
    "detect": YoloDrawerBase,
    "segment": YoloSegDrawer,
    "pose": YoloPoseDrawer,
}


class YoloPipeline:
    def __init__(self, config_dir, pipeline_name):
        self.config_dir = Path(config_dir)
        self.pipeline = Pipeline(pipeline_name, str(self.config_dir / PIPELINE_YML))
        self.yolo_task, self.meta = self.load_yolo_task()
        self.link_demux_pads()

    def link_demux_pads(self):
        pad_links_path = self.config_dir / PAD_LINKS_YML
        if not pad_links_path.is_file():
            return
        pad_links = yaml.safe_load(pad_links_path.read_text())
        for target in pad_links.get("demux", []):
            self.pipeline.link(("demux", target), ("src_%u", ""))

    def load_yolo_task(self):
        meta = json.loads((self.config_dir / META_JSON).read_text())
        task = meta["yolo_task"]
        if task not in TASK_DRAWER_MAP:
            raise ValueError(f"unsupported yolo_task {task!r}")
        return task, meta

    def create_drawer(self, **drawer_kwargs):
        drawer_cls = TASK_DRAWER_MAP[self.yolo_task]
        if self.yolo_task == "pose":
            shape = self.meta["input_tensor_shape"]
            return drawer_cls(infer_width=shape[3], infer_height=shape[2], **drawer_kwargs)
        return drawer_cls(**drawer_kwargs)

    def build(
        self,
        element_name="pgie",
        enable_drawer=True,
        logger=None,
        inference_interval=DEFAULT_INFERENCE_LOG_INTERVAL,
        **drawer_kwargs,
    ):
        self.pipeline.attach(
            element_name,
            Probe("yolo-inference", YoloInferenceProbe(self.yolo_task, inference_interval, logger)),
        )
        if enable_drawer:
            self.pipeline.attach(
                element_name,
                Probe("yolo-drawer", self.create_drawer(**drawer_kwargs)),
            )
        return self.pipeline
