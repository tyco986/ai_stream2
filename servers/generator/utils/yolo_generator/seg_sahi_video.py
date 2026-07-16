import copy

from .det_image import DetImageGenerator
from .det_sahi_video import DetSahiVideoGenerator, SAHI_VIDEO_TOPOLOGY_DOC
from .det_video import VIDEO_TOPOLOGY_DOC
from .utils import YoloSegSahi


class SegSahiVideoGenerator(DetSahiVideoGenerator):
    GENERATOR = "SegSahiVideoGenerator"

    f"""Generate YOLO SAHI segmentation video pipeline YAML.

    Reads ``input`` video via DeepStream, runs SAHI instance-segmentation inference with OSD,
    and writes the annotated result to ``output``. Does not insert ``nvmsgconv`` / ``nvmsgbroker``.
    {VIDEO_TOPOLOGY_DOC}
    {SAHI_VIDEO_TOPOLOGY_DOC}
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
