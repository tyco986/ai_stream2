import copy

from ..base_generator.base_image import BaseImageGenerator, IMAGE_TOPOLOGY_DOC
from ..subelement_generator.utils.default_pgie import YoloDet


class DetImageGenerator(BaseImageGenerator):
    GENERATOR = "DetImageGenerator"

    f"""Generate YOLO detection image pipeline YAML.

    Reads ``input`` image via DeepStream, runs inference with OSD, and writes the
    annotated result to ``output``. Does not insert ``nvmsgconv`` / ``nvmsgbroker``.
    {IMAGE_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloDet)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
