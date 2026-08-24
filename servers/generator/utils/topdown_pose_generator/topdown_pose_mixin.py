import shutil
from pathlib import Path

import yaml

from ..subelement_generator.sgie import SgieGenerator
from ..subelement_generator.utils.default_gie.manager import PgieManager, SgieManager
from ..subelement_generator.utils.sgie_parser import SgieParser


class TopdownPoseMixin:
    SGIE_CONFIG_NAME = "sgie0.yml"
    SGIE_META_NAME = "sgie0_meta.json"

    def init_pipeline(self) -> None:
        self.init_sgie()
        super().init_pipeline()

    def init_params(self) -> None:
        super().init_params()
        self.params_yml["sgie"] = self.sgie

    def init_sgie(self) -> None:
        self.sgie = {
            "model_dir": self.sgie["model_dir"],
            "interval": int(self.sgie.get("interval", 1)),
        }
        self.sgie_config_parser = SgieParser(
            self.sgie["model_dir"],
            self.sgie["interval"],
        )
        self.sgie_generator = SgieGenerator(**self.sgie_config_parser.build())
        self.apply_sgie_config()
        self.params_yml["sgie"] = self.sgie

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = PgieManager().config(
            self.pgie_config_parser.meta["version"]
        )
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config

    def apply_sgie_config(self) -> None:
        self.sgie_generator.config = SgieManager().config(
            self.sgie_config_parser.meta["version"]
        )
        self.sgie_generator.update_config()
        self.sgie_yml = self.sgie_generator.config

    def append_sgie_node(self) -> None:
        self._append_node(
            "nvinfer",
            "sgie0",
            self._add_nvinfer(
                config_file_path=self.SGIE_CONFIG_NAME,
                batch_size=self.sgie_generator.batch_size,
                gpu_id=self.sgie_generator.gpu_id,
            ),
        )

    def apply_save_paths(self, config_save_dir: Path) -> None:
        super().apply_save_paths(config_save_dir)
        for node in self.pipeline_yml["deepstream"]["nodes"]:
            if node["name"] == "sgie0":
                node.setdefault("properties", {})["config-file-path"] = str(
                    config_save_dir / self.SGIE_CONFIG_NAME
                )

    def write(self, config_save_dir: str | Path) -> None:
        super().write(config_save_dir)
        config_save_dir = Path(config_save_dir)
        shutil.copy2(
            self.sgie_config_parser.meta_path,
            config_save_dir / self.SGIE_META_NAME,
        )
        with open(config_save_dir / self.SGIE_CONFIG_NAME, "w", encoding="utf-8") as handle:
            yaml.safe_dump(
                self.sgie_yml, handle, sort_keys=False, default_flow_style=False
            )
