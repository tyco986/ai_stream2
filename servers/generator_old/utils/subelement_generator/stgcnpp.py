import os

import yaml

from .utils.nvinfer_interval import nvinfer_skip


class StgcnppGenerator:
    def __init__(
        self,
        model_engine_file: str,
        labelfile_path: str,
        output_blob_names: str,
        batch_size=1,
        network_mode: int = 2,
        gpu_id: int = 0,
        interval: int = 0,
        tensor_name: str = "input",
        num_person: int = 2,
        clip_len: int = 100,
        num_joints: int = 17,
    ) -> None:
        self.model_engine_file = model_engine_file
        self.labelfile_path = labelfile_path
        self.output_blob_names = output_blob_names
        self.batch_size = batch_size
        self.network_mode = network_mode
        self.gpu_id = gpu_id
        self.interval = interval
        self.tensor_name = tensor_name
        self.num_person = num_person
        self.clip_len = clip_len
        self.num_joints = num_joints
        self.config = {"property": {}}

    def update_config(self) -> None:
        self.config["property"]["batch-size"] = self.batch_size
        self.config["property"]["network-mode"] = self.network_mode
        self.config["property"]["gpu-id"] = self.gpu_id
        self.config["property"]["interval"] = nvinfer_skip(self.interval)
        self.config["property"]["model-engine-file"] = self.model_engine_file
        self.config["property"]["labelfile-path"] = self.labelfile_path
        self.config["property"]["output-blob-names"] = self.output_blob_names

    def write(self, save_path: str | os.PathLike[str]) -> None:
        with open(save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(self.config, handle, sort_keys=False, default_flow_style=False)
