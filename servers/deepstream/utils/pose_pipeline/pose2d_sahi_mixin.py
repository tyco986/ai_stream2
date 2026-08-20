import yaml

from utils.base_pipeline.base_video import PIPELINE_YML


class Pose2DSahiMixin:
    def cache_target(self):
        return "nvsahipostprocess"

    def rect_expand_target(self):
        pipeline = yaml.safe_load(
            (self.config_dir / PIPELINE_YML).read_text(encoding="utf-8")
        )
        names = {node["name"] for node in pipeline["deepstream"]["nodes"]}
        target = "nvsahipostprocess"
        if "nvtracker" in names:
            target = "nvtracker"
        return target
