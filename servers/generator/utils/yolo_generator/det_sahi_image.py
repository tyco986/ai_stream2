import copy

from ..base_generator.base_sahi_image import BaseSahiImageGenerator, SAHI_IMAGE_TOPOLOGY_DOC
from ..subelement_generator.utils.default_gie import YoloDet


class DetSahiImageGenerator(BaseSahiImageGenerator):
    GENERATOR = "DetSahiImageGenerator"

    f"""Generate YOLO SAHI detection image pipeline YAML.

    Reads ``input`` image via DeepStream, runs SAHI inference with OSD, and writes the
    annotated result to ``output``. Does not insert ``nvmsgconv`` / ``nvmsgbroker``.
    {SAHI_IMAGE_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloDet)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
