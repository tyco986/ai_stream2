from ..base_generator.base_video import BaseVideoGenerator, VIDEO_TOPOLOGY_DOC
from ..subelement_generator.utils.default_pgie.manager import PgieManager


class DetVideoGenerator(BaseVideoGenerator):
    GENERATOR = "DetVideoGenerator"

    f"""Generate YOLO detection video pipeline YAML.

    Reads ``input`` video via DeepStream, runs inference with OSD, and writes the
    annotated result to ``output``. Does not insert ``nvmsgconv`` / ``nvmsgbroker``.
    {VIDEO_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = PgieManager().config(
            self.pgie_config_parser.meta["version"]
        )
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
