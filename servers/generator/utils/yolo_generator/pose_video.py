import copy

from .det_video import DetVideoGenerator, VIDEO_TOPOLOGY_DOC
from .utils import YoloPose


class PoseVideoGenerator(DetVideoGenerator):
    GENERATOR = "PoseVideoGenerator"

    f"""Generate YOLO pose video pipeline YAML.

    Reads ``input`` video via DeepStream, runs inference with OSD, and writes the
    annotated result to ``output``. Does not insert ``nvmsgconv`` / ``nvmsgbroker``.
    {VIDEO_TOPOLOGY_DOC}
    """

    def init_pgie(self) -> None:
        super().init_pgie()
        self.pgie_generator.config = copy.deepcopy(YoloPose)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
