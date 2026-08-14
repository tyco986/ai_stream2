from ..base_generator.base_rtsp_vis import BaseRTSPVisGenerator, VIS_RTSP_TOPOLOGY_DOC
from ..subelement_generator.utils.default_pgie.manager import PgieManager


class DetVisRTSPGenerator(BaseRTSPVisGenerator):
    GENERATOR = "DetVisRTSPGenerator"

    f"""Generate YOLO detection RTSP pipeline with OSD preview sink.

    Set ``analyzer=None`` to disable nvdsanalytics rules. Set ``tracker=None`` to skip nvtracker.
    {VIS_RTSP_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = PgieManager().config(
            self.pgie_config_parser.meta["version"]
        )
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
