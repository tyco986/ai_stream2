from utils.tool.drawer.rtmpose_drawer import RtmposeDrawer
from utils.tool.drawer.yolo_pose_fade_drawer import YoloPoseFadeDrawer


class RtmposeFadeDrawer(YoloPoseFadeDrawer, RtmposeDrawer):
    def __init__(
        self,
        show_label=False,
        show_conf=False,
        show_id=False,
        kpt_threshold=0.0,
        infer_height=256,
        infer_width=192,
        sgie_unique_id=2,
        interval=0,
        fade_time=0,
    ):
        self.show_label = show_label
        self.show_conf = show_conf
        self.show_id = show_id
        self.kpt_threshold = float(kpt_threshold)
        self.infer_height = int(infer_height)
        self.infer_width = int(infer_width)
        self.sgie_unique_id = int(sgie_unique_id)
        self.interval = int(interval)
        self.fade_time = int(fade_time)
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

    def pose_item(self, object_meta, frame_meta, object_index) -> dict:
        item = self.restore_object(
            object_meta,
            int(frame_meta.source_id),
            int(frame_meta.frame_number),
            object_index,
        )
        return item
