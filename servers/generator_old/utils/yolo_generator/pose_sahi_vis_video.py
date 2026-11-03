import copy

from ..base_generator.base_sahi_pose_vis_video import (
    BaseSahiPoseVisVideoGenerator,
    SAHI_POSE_VIS_VIDEO_TOPOLOGY_DOC,
)
from ..subelement_generator.utils.default_gie import YoloPose


class PoseSahiVisVideoGenerator(BaseSahiPoseVisVideoGenerator):
    GENERATOR = "PoseSahiVisVideoGenerator"

    f"""Generate YOLO SAHI pose video pipeline YAML with OSD and mp4 filesink.

    {SAHI_POSE_VIS_VIDEO_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloPose)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
