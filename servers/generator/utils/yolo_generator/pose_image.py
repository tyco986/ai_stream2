import copy

from ..base_generator.base_image import BaseImageGenerator, IMAGE_TOPOLOGY_DOC
from ..subelement_generator.utils.default_gie import YoloPose


class PoseImageGenerator(BaseImageGenerator):
    GENERATOR = "PoseImageGenerator"

    f"""Generate YOLO pose image pipeline YAML.

    Reads ``input`` image via DeepStream, runs pose inference with OSD,
    and writes the annotated result to ``output``. Does not insert ``nvmsgconv`` / ``nvmsgbroker``.
    {IMAGE_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloPose)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config

    def osd_kwargs(self, gpu_id: int) -> dict:
        return {
            "gpu_id": gpu_id,
            "display_bbox": True,
            "display_text": True,
            "display_mask": False,
        }
