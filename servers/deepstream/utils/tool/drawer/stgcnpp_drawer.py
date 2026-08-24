from utils.tool.drawer.rtmpose_drawer import RtmposeDrawer
from utils.tool.parser.stgcnpp_parser import (
    INFER_HEIGHT,
    INFER_WIDTH,
    MISSING_ACTION,
    SGIE_UNIQUE_ID,
    STGCNPP_UNIQUE_ID,
    StgcnppParser,
)

FADE_INTERVAL = 50
FADE_TIME = 1
KPT_THRESHOLD = 0.0


class StgcnppDrawer(RtmposeDrawer, StgcnppParser):
    def __init__(self, show_pose=False):
        self.show_pose = bool(show_pose)
        self.stgcnpp_unique_id = STGCNPP_UNIQUE_ID
        super().__init__(
            show_label=True,
            show_conf=True,
            show_id=True,
            kpt_threshold=KPT_THRESHOLD,
            infer_height=INFER_HEIGHT,
            infer_width=INFER_WIDTH,
            sgie_unique_id=SGIE_UNIQUE_ID,
        )

    def build_display_text(self, item) -> str:
        name, conf = item["action"][0], item["action"][1]
        display_text = f"{name}|{conf:.2f}|{item['object'][7]}"
        return display_text

    def draw_pose(self, batch_meta, frame_meta, item) -> None:
        if self.show_pose:
            RtmposeDrawer.draw_pose(self, batch_meta, frame_meta, item)
