import copy

from .det_image import DetImageGenerator, IMAGE_TOPOLOGY_DOC
from .det_sahi_image import DetSahiImageGenerator, SAHI_IMAGE_TOPOLOGY_DOC
from .utils import YoloSegSahi


class SegSahiImageGenerator(DetSahiImageGenerator):
    GENERATOR = "SegSahiImageGenerator"

    f"""Generate YOLO SAHI segmentation image pipeline YAML.

    Reads ``input`` image via DeepStream, runs SAHI instance-segmentation inference with OSD,
    and writes the annotated result to ``output``. Does not insert ``nvmsgconv`` / ``nvmsgbroker``.
    {IMAGE_TOPOLOGY_DOC}
    {SAHI_IMAGE_TOPOLOGY_DOC}
    """

    def init_pgie(self) -> None:
        DetImageGenerator.init_pgie(self)
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
