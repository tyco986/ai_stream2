from utils.tool.drawer.yolo_pose_drawer import YoloPoseDrawer
from utils.tool.parser.result import new_result
from utils.tool.parser.rtmpose_parser import PADDING, RtmposeParser


class RtmposeDrawer(YoloPoseDrawer, RtmposeParser):
    def __init__(
        self,
        show_label=False,
        show_conf=False,
        show_id=False,
        kpt_threshold=0.0,
        infer_height=256,
        infer_width=192,
        sgie_unique_id=2,
    ):
        self.show_label = show_label
        self.show_conf = show_conf
        self.show_id = show_id
        self.kpt_threshold = float(kpt_threshold)
        self.infer_height = int(infer_height)
        self.infer_width = int(infer_width)
        self.sgie_unique_id = int(sgie_unique_id)
        self.frame_width = 1
        self.frame_height = 1
        self.init_osd_colors()

    def process_frame(
        self,
        batch_meta,
        frame_meta,
        box_color=(0.0, 1.0, 0.0, 1.0),
        box_width=2,
        text_color=(1.0, 1.0, 1.0, 1.0),
        text_bg_color=(0.0, 0.0, 0.0, 0.6),
    ) -> dict:
        result = new_result()
        self.fill_frame_meta(result, frame_meta)
        self.frame_width = max(1, int(result["pipeline_width"]))
        self.frame_height = max(1, int(result["pipeline_height"]))
        source_id = int(frame_meta.source_id)
        frame_number = int(frame_meta.frame_number)
        for object_index, object_meta in enumerate(frame_meta.object_items):
            item = self.restore_object(object_meta, source_id, frame_number, object_index)
            self.draw_inplace(
                object_meta,
                item,
                box_color,
                box_width,
                text_color,
                text_bg_color,
            )
            self.draw_pose(batch_meta, frame_meta, item)
            result["objects"].append(item)
        result["num_objects"] = len(result["objects"])
        return result
