import copy

from ..base_generator.base_sahi_pose_rtsp_vis import (
    BaseSahiPoseVisRTSPGenerator,
    SAHI_POSE_VIS_RTSP_TOPOLOGY_DOC,
)
from ..subelement_generator.utils.default_gie import YoloPose


class PoseSahiVisRTSPGenerator(BaseSahiPoseVisRTSPGenerator):
    GENERATOR = "PoseSahiVisRTSPGenerator"

    f"""Generate YOLO SAHI pose RTSP pipeline with OSD preview sink.

    Set ``analyzer=None`` to skip nvdsanalytics. Set ``tracker=None`` to skip nvtracker.
    {SAHI_POSE_VIS_RTSP_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloPose)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
