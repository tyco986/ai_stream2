from pyservicemaker import osd

from utils.tool.drawer.det_drawer import DetDrawer
from utils.tool.parser.result import new_result
from utils.tool.parser.yolo_pose_parser import YoloPoseParser

MAX_DISPLAY_ELEMENTS = 16
KPT_RADIUS = 2
KPT_WIDTH = 1
SKELETON_WIDTH = 2
KPT_LEGEL_COLOR = (1.0, 0.5, 0.0, 1.0)
KPT_ILLEGEL_COLOR = (1.0, 0.0, 0.0, 1.0)
COCO17_SKELETON = (
    (0, 1),
    (0, 2),
    (1, 3),
    (2, 4),
    (5, 6),
    (5, 7),
    (7, 9),
    (6, 8),
    (8, 10),
    (5, 11),
    (6, 12),
    (11, 12),
    (11, 13),
    (13, 15),
    (12, 14),
    (14, 16),
)


class YoloPoseDrawer(DetDrawer, YoloPoseParser):
    def __init__(
        self,
        show_label=False,
        show_conf=False,
        show_id=False,
        kpt_threshold=0.0,
    ):
        self.show_label = show_label
        self.show_conf = show_conf
        self.show_id = show_id
        self.kpt_threshold = float(kpt_threshold)
        self.frame_width = 1
        self.frame_height = 1
        self.init_osd_colors()

    def osd_color(self, color) -> osd.Color:
        return self.fill_osd_color(self.kpt_osd_color, color)

    def clamp_x(self, value) -> int:
        return max(0, min(int(round(value)), self.frame_width - 1))

    def clamp_y(self, value) -> int:
        return max(0, min(int(round(value)), self.frame_height - 1))

    def make_circle(self, x, y, color) -> osd.Circle:
        circle = osd.Circle()
        circle.xc = self.clamp_x(x)
        circle.yc = self.clamp_y(y)
        circle.radius = KPT_RADIUS
        circle.color = color
        circle.width = KPT_WIDTH
        circle.has_bg_color = 1
        circle.bg_color = color
        return circle

    def make_line(self, x1, y1, x2, y2, color) -> osd.Line:
        line = osd.Line()
        line.x1 = self.clamp_x(x1)
        line.y1 = self.clamp_y(y1)
        line.x2 = self.clamp_x(x2)
        line.y2 = self.clamp_y(y2)
        line.width = SKELETON_WIDTH
        line.color = color
        return line

    def kpt_draw_color(self, score):
        color = KPT_LEGEL_COLOR
        if score < self.kpt_threshold:
            color = KPT_ILLEGEL_COLOR
        return color

    def build_circles(self, keypoints) -> list:
        circles = [
            self.make_circle(x, y, self.osd_color(self.kpt_draw_color(score)))
            for x, y, score in keypoints
        ]
        return circles

    def build_lines(self, keypoints) -> list:
        color = self.osd_color(KPT_LEGEL_COLOR)
        lines = [
            self.make_line(
                keypoints[i][0],
                keypoints[i][1],
                keypoints[j][0],
                keypoints[j][1],
                color,
            )
            for i, j in COCO17_SKELETON
            if i < len(keypoints) and j < len(keypoints)
        ]
        return lines

    def append_display_elements(self, batch_meta, frame_meta, circles, lines) -> None:
        display_meta = None
        circle_index = 0
        line_index = 0
        while circle_index < len(circles) or line_index < len(lines):
            if display_meta is None:
                display_meta = batch_meta.acquire_display_meta()
            added = False
            if circle_index < len(circles) and display_meta.n_circles < MAX_DISPLAY_ELEMENTS:
                display_meta.add_circle(circles[circle_index])
                circle_index += 1
                added = True
            if line_index < len(lines) and display_meta.n_lines < MAX_DISPLAY_ELEMENTS:
                display_meta.add_line(lines[line_index])
                line_index += 1
                added = True
            if not added:
                frame_meta.append(display_meta)
                display_meta = None
        if display_meta is not None:
            frame_meta.append(display_meta)

    def draw_pose(self, batch_meta, frame_meta, item) -> None:
        keypoints = item["keypoints"]
        self.append_display_elements(
            batch_meta,
            frame_meta,
            self.build_circles(keypoints),
            self.build_lines(keypoints),
        )

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
        for object_meta in frame_meta.object_items:
            item = self.parse_pose_object(object_meta)
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
