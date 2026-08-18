import copy

from ..base_generator.base_sahi_video import BaseSahiVideoGenerator, SAHI_VIDEO_TOPOLOGY_DOC
from ..subelement_generator.utils.default_gie import YoloDet


class DetSahiVideoGenerator(BaseSahiVideoGenerator):
    GENERATOR = "DetSahiVideoGenerator"

    f"""Generate YOLO SAHI detection video pipeline YAML.

    Reads ``input`` video via DeepStream, runs SAHI inference with OSD, and writes the
    annotated result to ``output``. Does not insert ``nvmsgconv`` / ``nvmsgbroker``.
    {SAHI_VIDEO_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloDet)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
