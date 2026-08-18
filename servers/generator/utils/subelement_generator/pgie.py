import copy
import os
from pathlib import Path

import yaml

from .utils.nvinfer_interval import nvinfer_skip


class PgieGenerator:
    def __init__(
        self,
        model_engine_file: str,
        labelfile_path: str,
        batch_size=1,
        network_mode: int = 2,
        gpu_id: int = 0,
        interval: int = 0,
        class_attrs=None,
        filter_out_class_ids: str | None = None,
        num_detected_classes: int | None = None,
    ):

        self.gpu_id = gpu_id
        self.interval = interval
        self.class_attrs = class_attrs
        self.filter_out_class_ids = filter_out_class_ids
        self.batch_size = batch_size
        self.network_mode = network_mode
        self.num_detected_classes = num_detected_classes
        self.model_engine_file = model_engine_file
        self.labelfile_path = labelfile_path
        self.config = {"property": {}}

    def update_config(self):
        self.config["property"]["batch-size"] = self.batch_size
        self.config["property"]["network-mode"] = self.network_mode
        self.config["property"]["num-detected-classes"] = self.num_detected_classes
        self.config["property"]["gpu-id"] = self.gpu_id
        self.config["property"]["interval"] = nvinfer_skip(self.interval)
        self.config["property"]["model-engine-file"] = self.model_engine_file
        self.config["property"]["labelfile-path"] = self.labelfile_path
        if self.filter_out_class_ids is not None:
            self.config["property"]["filter-out-class-ids"] = self.filter_out_class_ids
        else:
            self.config["property"].pop("filter-out-class-ids", None)

        if self.class_attrs is not None:
            for section, attrs in self.class_attrs.items():
                merged = dict(self.config.get(section, {}))
                merged.update(attrs)
                self.config[section] = merged

    def write(self, save_path: str | os.PathLike[str]) -> None:
        with open(save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(self.config, handle, sort_keys=False, default_flow_style=False)

if __name__ == "__main__":
    pgie_config = dict(
        model_engine_file="models/yolo/yolo_det.engine",
        labelfile_path="models/yolo/labels.txt",
        batch_size=1,
        network_mode=2,
        gpu_id=0,   
        interval=0,
        class_attrs=None,
        filter_out_class_ids=None,
        num_detected_classes=80,
    )
    pgie_generator = PgieGenerator(**pgie_config)
    print(pgie_generator.config)