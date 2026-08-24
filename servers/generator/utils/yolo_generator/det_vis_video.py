from ..base_generator.base_vis_video import BaseVisVideoGenerator, VIS_VIDEO_TOPOLOGY_DOC
from ..subelement_generator.utils.default_gie.manager import PgieManager


class DetVisVideoGenerator(BaseVisVideoGenerator):
    GENERATOR = "DetVisVideoGenerator"

    f"""Generate YOLO detection video pipeline YAML with OSD and mp4 filesink.

    {VIS_VIDEO_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = PgieManager().config(
            self.pgie_config_parser.meta["version"]
        )
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
