import copy

from .det_image import DetImageGenerator, IMAGE_TOPOLOGY_DOC
from .utils import YoloPose


class PoseImageGenerator(DetImageGenerator):
    GENERATOR = "PoseImageGenerator"

    f"""Generate YOLO pose image pipeline YAML.

    Reads ``input`` image via DeepStream, runs inference with OSD, and writes the
    annotated result to ``output``. Does not insert ``nvmsgconv`` / ``nvmsgbroker``.
    {IMAGE_TOPOLOGY_DOC}
    """

    def init_pgie(self) -> None:
        super().init_pgie()
        self.pgie_generator.config = copy.deepcopy(YoloPose)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
