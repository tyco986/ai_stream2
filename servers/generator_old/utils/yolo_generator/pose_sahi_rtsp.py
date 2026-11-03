import copy

from ..base_generator.base_sahi_pose_rtsp import (
    BaseSahiPoseRTSPGenerator,
    SAHI_POSE_RTSP_TOPOLOGY_DOC,
)
from ..subelement_generator.utils.default_gie import YoloPose


class PoseSahiRTSPGenerator(BaseSahiPoseRTSPGenerator):
    GENERATOR = "PoseSahiRTSPGenerator"

    f"""Generate YOLO SAHI pose RTSP pipeline (headless, ends at fakesink).

    Set ``analyzer=None`` to skip nvdsanalytics. Set ``tracker=None`` to skip nvtracker.
    {SAHI_POSE_RTSP_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloPose)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
