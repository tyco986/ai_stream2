from utils.probe.utils.drawer.pose2d_fade_drawer import Pose2DFadeDrawer
from utils.probe.utils.drawer.stgcnpp_drawer import (
    CLASSIFIER_THRESHOLD,
    EMPTY_ACTION,
    FADE_INTERVAL,
    FADE_TIME,
    INFER_HEIGHT,
    INFER_WIDTH,
    KPT_THRESHOLD,
    SGIE_UNIQUE_ID,
    STGCNPP_UNIQUE_ID,
    StgcnppDrawer,
)


class StgcnppFadeDrawer(Pose2DFadeDrawer, StgcnppDrawer):
    def __init__(self, show_pose=True):
        self.show_pose = show_pose
        self.show_label = True
        self.show_conf = False
        self.show_id = False
        self.kpt_threshold = float(KPT_THRESHOLD)
        self.infer_height = int(INFER_HEIGHT)
        self.infer_width = int(INFER_WIDTH)
        self.sgie_unique_id = int(SGIE_UNIQUE_ID)
        self.stgcnpp_unique_id = int(STGCNPP_UNIQUE_ID)
        self.classifier_threshold = float(CLASSIFIER_THRESHOLD)
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

    def cache_item(self, object_meta) -> dict:
        return self.parse_object(object_meta, [], list(EMPTY_ACTION))

