import copy

from ..base_generator.base_sahi_pose_image import (
    BaseSahiPoseImageGenerator,
    SAHI_POSE_IMAGE_TOPOLOGY_DOC,
)
from ..subelement_generator.utils.default_gie import YoloPose


class PoseSahiImageGenerator(BaseSahiPoseImageGenerator):
    GENERATOR = "PoseSahiImageGenerator"

    f"""Generate YOLO SAHI pose image pipeline YAML.

    Reads ``input`` image via DeepStream, runs SAHI pose inference with OSD,
    and writes the annotated result to ``output``. Does not insert ``nvmsgconv`` / ``nvmsgbroker``.
    {SAHI_POSE_IMAGE_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloPose)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
