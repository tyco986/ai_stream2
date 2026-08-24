from utils.tool.drawer.rtmpose_fade_drawer import RtmposeFadeDrawer
from utils.tool.drawer.stgcnpp_drawer import (
    FADE_INTERVAL,
    FADE_TIME,
    INFER_HEIGHT,
    INFER_WIDTH,
    KPT_THRESHOLD,
    MISSING_ACTION,
    SGIE_UNIQUE_ID,
    STGCNPP_UNIQUE_ID,
    StgcnppDrawer,
)


class StgcnppFadeDrawer(RtmposeFadeDrawer, StgcnppDrawer):
    def __init__(self, show_pose=False):
        self.show_pose = bool(show_pose)
        self.show_label = True
        self.show_conf = True
        self.show_id = True
        self.kpt_threshold = float(KPT_THRESHOLD)
        self.infer_height = int(INFER_HEIGHT)
        self.infer_width = int(INFER_WIDTH)
        self.sgie_unique_id = int(SGIE_UNIQUE_ID)
        self.stgcnpp_unique_id = int(STGCNPP_UNIQUE_ID)
        self.interval = int(FADE_INTERVAL)
        self.fade_time = int(FADE_TIME)
        self.frame_width = 1
        self.frame_height = 1
        self.fade_alpha = 1.0
        self.min_alpha = 0.2
        self.shadow_box_color = (0.6, 0.0, 1.0, 1.0)
        self.frame_count = {}
        self.phase = {}
        self.object_cache = {}
        self.conf_cache = {}
        self.pose_cache = {}
        self.alpha_lut = self.build_alpha_lut(self.interval, self.fade_time)
        self.runtime_interval = len(self.alpha_lut)
        self.init_osd_colors()

    def cache_item(self, object_meta) -> dict:
        return self.parse_object(object_meta, [], list(MISSING_ACTION))

    def apply_pose_cache(self, pad_index, item) -> None:
        if self.show_pose:
            RtmposeFadeDrawer.apply_pose_cache(self, pad_index, item)

    def prune_pose_cache(self, pad_index, items) -> None:
        if self.show_pose:
            RtmposeFadeDrawer.prune_pose_cache(self, pad_index, items)
