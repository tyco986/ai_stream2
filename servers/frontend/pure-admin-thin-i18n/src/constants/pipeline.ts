export const GIE_LIST_COLUMN_PERCENTS = {
  selection: 4,
  name: 16,
  model: 18,
  classAttrs: 22,
  interval: 10,
  updatedAt: 18,
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
  name: 22,
  annotations: 20,
  updatedAt: 38,
  ops: 15
};

export const PIPELINE_LIST_COLUMN_PERCENTS = {
  selection: 4,
  name: 16,
  type: 12,
  status: 12,
  gie: 22,
  analyzer: 22,
  ops: 12
};
