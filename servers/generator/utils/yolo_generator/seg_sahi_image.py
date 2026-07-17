import copy

from ..base_generator import BaseSahiImageGenerator, SAHI_IMAGE_TOPOLOGY_DOC
from ..subelement_generator import YoloSegSahi


class SegSahiImageGenerator(BaseSahiImageGenerator):
    GENERATOR = "SegSahiImageGenerator"

    f"""Generate YOLO SAHI segmentation image pipeline YAML.

    Reads ``input`` image via DeepStream, runs SAHI instance-segmentation inference with OSD,
    and writes the annotated result to ``output``. Does not insert ``nvmsgconv`` / ``nvmsgbroker``.
    {SAHI_IMAGE_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloSegSahi)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config

    def osd_kwargs(self, gpu_id: int) -> dict:
        return {
            "gpu_id": gpu_id,
            "display_bbox": True,
            "display_text": True,
            "display_mask": True,
        }
