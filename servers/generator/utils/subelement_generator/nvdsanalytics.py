from pathlib import Path
from copy import deepcopy
import yaml

nvdsanalytics_default_config = {
        "property": {
            "enable": 0,
            "config-width": 1920,
            "config-height": 1080,
            "osd-mode": 0,
            "display-font-size": 12,
        },
        "roi-filtering-stream-0": {
            "enable": 0,
            "class-id": -1,
            "inverse-roi": 0,
            "roi-DOOR": "256;639;675;83;876;224;926;482;866;741",
        },
        "overcrowding-stream-0": {
            "enable": 0,
            "class-id": "1;2",
            "object-threshold": 5,
            "time-threshold": 3000,
            "roi-ENTRANCE": "282;347;987;843",
        },
        "line-crossing-stream-0": {
            "enable": 0,
            "extended": 0,
            "class-id": 0,
            "mode": "balanced",
            "line-crossing-Exit": "789;672;1084;900;851;773;1203;732",
        },
        "direction-detection-stream-0": {
            "enable": 0,
            "class-id": 0,
            "mode": "loose",
            "direction-South": "284;840;360;662",
        },
    }

class NvdsanalyticsConfigGenerator:
    """Write nvdsanalytics config-file YAML (full template, all rules disabled)."""

    def __init__(self, config=nvdsanalytics_default_config) -> None:
        self.config = deepcopy(config)

    def write(self, save_path: str | Path) -> None:
        with open(save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(self.config, handle, sort_keys=False, default_flow_style=False)

if __name__ == "__main__":
    nvdsanalytics_generator = NvdsanalyticsConfigGenerator()
    print(nvdsanalytics_generator.config)