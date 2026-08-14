from copy import deepcopy

from .peoplenet import PeopleNet
from .yolo_det import YoloDet


class PgieManager:
    CONFIGS = {
        "yolo8": YoloDet,
        "yolo10": YoloDet,
        "yolo11": YoloDet,
        "yolo26": YoloDet,
        "peoplenet": PeopleNet,
    }

    def config(self, version: str) -> dict:
        template = self.CONFIGS.get(version)
        if template is None:
            raise ValueError(f"unsupported pgie version: {version}")
        config = deepcopy(template)
        return config
