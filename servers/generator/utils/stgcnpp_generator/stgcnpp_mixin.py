import shutil
from pathlib import Path

import yaml

from ..topdown_pose_generator.topdown_pose_mixin import TopdownPoseMixin
from ..subelement_generator.stgcnpp import StgcnppGenerator
from ..subelement_generator.stgcnpp_preprocess import StgcnppPreprocessGenerator
from ..subelement_generator.utils.default_gie.manager import StgcnppManager
from ..subelement_generator.utils.stgcnpp_parser import StgcnppParser


class StgcnppMixin(TopdownPoseMixin):
    STGCNPP_CONFIG_NAME = "sgie1.yml"
    STGCNPP_META_NAME = "sgie1_meta.json"
    PREPROCESS_CONFIG_NAME = "preprocess_stgcnpp.txt"

    def init_sgie(self) -> None:
        super().init_sgie()
        self.init_stgcnpp()

    def init_params(self) -> None:
        super().init_params()
        self.params_yml["stgcnpp"] = self.stgcnpp

    def stream_count(self) -> int:
        count = 1
        streams = getattr(self, "streams", None)
        if streams is not None:
            count = len(streams)
        return count

    def init_stgcnpp(self) -> None:
        self.stgcnpp = {
            "model_dir": self.stgcnpp["model_dir"],
            "interval": int(self.stgcnpp.get("interval", 0)),
        }
        self.stgcnpp_config_parser = StgcnppParser(
            self.stgcnpp["model_dir"],
            self.stgcnpp["interval"],
        )
        parsed = self.stgcnpp_config_parser.build()
        self.stgcnpp_generator = StgcnppGenerator(**parsed)
        self.apply_stgcnpp_config()
        pose_shape = self.sgie_config_parser.meta["input_tensor_shape"]
        self.stgcnpp_preprocess_generator = StgcnppPreprocessGenerator(
            batch_size=self.stgcnpp_generator.batch_size,
            clip_len=self.stgcnpp_generator.clip_len,
            num_joints=self.stgcnpp_generator.num_joints,
            num_person=self.stgcnpp_generator.num_person,
            infer_width=int(pose_shape[3]),
            infer_height=int(pose_shape[2]),
            tensor_name=self.stgcnpp_generator.tensor_name,
            stream_count=self.stream_count(),
        )
        self.stgcnpp_preprocess_ini = self.stgcnpp_preprocess_generator.render()
        self.params_yml["stgcnpp"] = self.stgcnpp

    def apply_stgcnpp_config(self) -> None:
        self.stgcnpp_generator.config = StgcnppManager().config(
            self.stgcnpp_config_parser.meta["version"]
        )
        self.stgcnpp_generator.update_config()
        self.stgcnpp_yml = self.stgcnpp_generator.config

    def append_stgcnpp_nodes(self) -> None:
        self._append_node(
            "nvdspreprocess",
            "nvdspreprocess",
            self._add_nvdspreprocess(self.PREPROCESS_CONFIG_NAME),
        )
        self._append_node(
            "nvinfer",
            "sgie1",
            self._add_nvinfer(
                config_file_path=self.STGCNPP_CONFIG_NAME,
                batch_size=self.stgcnpp_generator.batch_size,
                gpu_id=self.stgcnpp_generator.gpu_id,
                input_tensor_meta=True,
            ),
        )

    def link_stgcnpp(self, edges: dict) -> None:
        edges["sgie0"] = "nvdspreprocess"
        edges["nvdspreprocess"] = "sgie1"
        edges["sgie1"] = "nvdsanalytics"

    def apply_save_paths(self, config_save_dir: Path) -> None:
        super().apply_save_paths(config_save_dir)
        for node in self.pipeline_yml["deepstream"]["nodes"]:
            name = node["name"]
            properties = node.setdefault("properties", {})
            if name == "nvdspreprocess":
                properties["config-file"] = str(
                    config_save_dir / self.PREPROCESS_CONFIG_NAME
                )
            if name == "sgie1":
                properties["config-file-path"] = str(
                    config_save_dir / self.STGCNPP_CONFIG_NAME
                )

    def write(self, config_save_dir: str | Path) -> None:
        super().write(config_save_dir)
        config_save_dir = Path(config_save_dir)
        shutil.copy2(
            self.stgcnpp_config_parser.meta_path,
            config_save_dir / self.STGCNPP_META_NAME,
        )
        with open(
            config_save_dir / self.STGCNPP_CONFIG_NAME, "w", encoding="utf-8"
        ) as handle:
            yaml.safe_dump(
                self.stgcnpp_yml, handle, sort_keys=False, default_flow_style=False
            )
        (
            config_save_dir / self.PREPROCESS_CONFIG_NAME
        ).write_text(self.stgcnpp_preprocess_ini, encoding="utf-8")
