import copy

from ..base_generator.base_sahi_pose_video import (
    BaseSahiPoseVideoGenerator,
    SAHI_POSE_VIDEO_TOPOLOGY_DOC,
)
from ..subelement_generator.utils.default_gie import YoloPose


class PoseSahiVideoGenerator(BaseSahiPoseVideoGenerator):
    GENERATOR = "PoseSahiVideoGenerator"

    f"""Generate YOLO SAHI pose video pipeline YAML (headless, ends at fakesink).

    {SAHI_POSE_VIDEO_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloPose)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
