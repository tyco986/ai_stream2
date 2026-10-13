from pathlib import Path

DEFAULT_DETECTOR_ENGINE = Path("/root/models/trt/yolo26n/yolo26n.engine")
PERSON_CLASS = 0
DET_THRESHOLD = 0.25
BBOX_PADDING = 1.25
YOLO_SCALE = 0.0039215697906911373
POSE_SCALE = 0.017124753831663668
POSE_MEAN = (123.675, 116.28, 103.53)
COLOR_GREEN_BGR = (0, 255, 0)
COLOR_RED_BGR = (0, 0, 255)
COLOR_ORANGE_BGR = (0, 128, 255)
BOX_WIDTH = 2
KPT_RADIUS = 4
SKELETON_WIDTH = 2
PERSON_LABEL = "person"
LABEL_SEP = "|"
FONT_SCALE = 0.5
FONT_THICKNESS = 1
LABEL_Y_OFFSET = 4
OUTPUT_DIR = Path("/root/outputs/export_trt")
DEFAULT_POSE_THRESHOLD = 0.5
DEFAULT_FPS = 25
BGR_PIX_FMT = "bgr24"
ENCODE_PIX_FMT = "yuv420p"
COCO17_EDGES = (
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
