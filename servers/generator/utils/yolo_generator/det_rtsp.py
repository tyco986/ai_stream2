from ..base_generator.base_rtsp import BaseRTSPGenerator, RTSP_TOPOLOGY_DOC
from ..subelement_generator.utils.default_gie.manager import PgieManager


class DetRTSPGenerator(BaseRTSPGenerator):
    GENERATOR = "DetRTSPGenerator"

    f"""Generate YOLO detection RTSP pipeline (headless, ends at fakesink).

    Set ``analyzer=None`` to disable nvdsanalytics rules. Set ``tracker=None`` to skip nvtracker.
    {RTSP_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = PgieManager().config(
            self.pgie_config_parser.meta["version"]
        )
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
