import { assertColumnPercents } from "@/utils/columnWidth";

export const PIPELINE_CREATE_DIALOG_WIDTH = "50%";
export const PIPELINE_LIST_PAGE_SIZE = 20;
export const PIPELINE_OPTION_PAGE_SIZE = 200;
export const PIPELINE_SOURCE_PAGE_SIZE = 20;
export const PIPELINE_OUTPUT_DIR = "/root/outputs/deepstream";

export const PIPELINE_SOURCE_COLUMN_PERCENTS = {
  selection: 8,
  name: 52,
  width: 20,
  height: 20
};

export const PIPELINE_TIMED_SOURCE_COLUMN_PERCENTS = {
  selection: 8,
  name: 40,
  width: 16,
  height: 16,
  fps: 20
};

export const GIE_INTERVAL_MIN = 1;
export const GIE_DIALOG_WIDTH = "60%";
export const GIE_ATTR_DIALOG_WIDTH = "40%";
export const GIE_FORM_DIALOG_WIDTH = "70%";
export const ANALYZER_DIALOG_WIDTH = "50%";
export const ANALYZER_ANNOTATION_DIALOG_WIDTH = "40%";
export const ANALYZER_FORM_DIALOG_WIDTH = "75%";
export const ANALYZER_IMAGE_DIALOG_WIDTH = "50%";

export const GIE_LIST_COLUMN_PERCENTS = {
  selection: 5,
  name: 15,
  model: 18,
  classAttrs: 15,
  interval: 10,
  updatedAt: 25,
  ops: 12
};

export const GIE_ATTR_UNSUPPORTED_TYPES = ["RTMPOSE"];

export const GIE_ATTR_COLUMN_PERCENTS = {
  classKey: 14,
  conf: 12,
  maxDet: 12,
  minW: 13,
  minH: 13,
  maxW: 13,
  maxH: 13,
  ops: 10
};

export const ANALYZER_CANVAS_WIDTH = 800;
export const ANALYZER_CANVAS_HEIGHT = 450;
export const ANALYZER_SHAPE_TYPE_WIDTH = 72;
export const ANALYZER_SHAPE_OPS_WIDTH = 88;
export const ANALYZER_CROSSHAIR_STROKE = "#409eff";
export const ANALYZER_EDGE_STROKE_WIDTH = 1;
export const ANALYZER_CROSSHAIR_DASH = [6, 4];
export const ANALYZER_SNAP_THRESHOLD = 6;
export const ANALYZER_PAN_THRESHOLD = 8;
export const ANALYZER_RECT_RULES = ["roi", "overcrowding"];
export const ANALYZER_RECT_COLORS: Record<string, string> = {
  roi: "#ffff00",
  overcrowding: "#ff8000"
};
export const ANALYZER_RECT_BORDER = 4;
export const ANALYZER_RECT_FONT = "Serif";
export const ANALYZER_RECT_FONT_SIZE = 12;
export const ANALYZER_RECT_LABEL_BG = "#000000";
export const ANALYZER_LINE_RULE = "line";
export const ANALYZER_LINE_STROKE = "#00ff00";
export const ANALYZER_DIRECTION_RULE = "direction";
export const ANALYZER_DIRECTION_STROKE = "#ff0000";
export const ANALYZER_SHAPE_COLORS: Record<string, string> = {
  ...ANALYZER_RECT_COLORS,
  [ANALYZER_LINE_RULE]: ANALYZER_LINE_STROKE,
  [ANALYZER_DIRECTION_RULE]: ANALYZER_DIRECTION_STROKE
};
export const ANALYZER_LINE_WIDTH = 2;
export const ANALYZER_ANCHOR_RADIUS = 5;
export const ANALYZER_ANCHOR_FILL = "#ffffff";
export const ANALYZER_ANGLE_RADIUS = 28;
export const ANALYZER_ANGLE_GAP = 6;
export const ANALYZER_ANGLE_LABEL_WIDTH = 36;
export const ANALYZER_ARROW_HEAD = 0.15;
export const ANALYZER_ARROW_RADIAN = (15 * Math.PI) / 180;
export const ANALYZER_CLASS_ID = -1;
export const ANALYZER_ENABLE = 1;
export const ANALYZER_OSD_MODE = 2;
export const ANALYZER_DISPLAY_FONT_SIZE = 12;
export const ANALYZER_LINE_MODE = "balanced";
export const ANALYZER_DIRECTION_MODE = "loose";
export const ANALYZER_OBJECT_THRESHOLD = "5";
export const ANALYZER_TIME_THRESHOLD = "2000";
export const ANALYZER_TIP_OFFSET = 8;
export const ANALYZER_TIP_WIDTH = 200;
export const ANALYZER_TIP_HEIGHT = 32;
export const ANALYZER_TIP_EXTEND_HEIGHT = 32;
export const ANALYZER_IMAGE_PAGE_SIZE = 20;
export const ANALYZER_IMAGE_ACCEPT = ".jpg,.jpeg,.png";
export const ANALYZER_IMAGE_SUFFIXES = [".jpg", ".jpeg", ".png"];

export const ANALYZER_RULES = [
  { key: "roi", label: "pipeline.pureRoiFiltering" },
  { key: "overcrowding", label: "pipeline.pureOvercrowding" },
  { key: "line", label: "pipeline.pureLineCrossing" },
  { key: "direction", label: "pipeline.pureDirection" }
];

export const ANALYZER_LIST_COLUMN_PERCENTS = {
  selection: 5,
  name: 14,
  image: 18,
  width: 8,
  height: 8,
  annotations: 12,
  updatedAt: 20,
  ops: 15
};

export const PIPELINE_LIST_COLUMN_PERCENTS = {
  selection: 4,
  name: 12,
  type: 8,
  status: 8,
  gie: 14,
  startTime: 16,
  endTime: 16,
  duration: 10,
  ops: 12
};
assertColumnPercents(PIPELINE_SOURCE_COLUMN_PERCENTS);
assertColumnPercents(PIPELINE_TIMED_SOURCE_COLUMN_PERCENTS);
assertColumnPercents(GIE_LIST_COLUMN_PERCENTS);
assertColumnPercents(GIE_ATTR_COLUMN_PERCENTS);
assertColumnPercents(ANALYZER_LIST_COLUMN_PERCENTS);
assertColumnPercents(PIPELINE_LIST_COLUMN_PERCENTS);

