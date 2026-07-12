from pathlib import Path
from copy import deepcopy
import yaml


nvmsgconv_default_config = {"sensor0": {
            "enable": 1,
            "type": "Camera",
            "id": "",
            "description": "",}}

class NvmsgconvGenerator:
    def __init__(self, streams):
        self.streams = streams
        self.config = {}

        for index, stream in enumerate(self.streams):
            self.config[f"sensor{index}"] = {
                "enable": 1,
                "type": "Camera",
                "id": f"stream_{index}",
                "description": stream,
            }

    def write(self, save_path: str | Path) -> None:
        with open(save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(self.config, handle, sort_keys=False, default_flow_style=False)

if __name__ == "__main__":
    nvmsgconv_generator = NvmsgconvGenerator(streams=["stream_0", "stream_1"])
    print(nvmsgconv_generator.config)