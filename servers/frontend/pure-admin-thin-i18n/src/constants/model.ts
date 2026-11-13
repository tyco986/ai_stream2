export const MODEL_DIALOG_WIDTH = "60%";

export const MODEL_CLASSES_DIALOG_WIDTH = "25%";

export const MODEL_CLASSES_TABLE_MAX_HEIGHT = 420;

export const MODEL_CLASSES_ROW_HEIGHT = 30;

export const MODEL_CLASSES_COLUMN_RESIZABLE = false;

export const MODEL_CLASSES_ID_COLUMN_WIDTH = 120;

export const MODEL_ROW_HEIGHT = 40;

export const MODEL_STATUS_FIRST_MS = 500;

export const MODEL_STATUS_POLL_MS = 2000;

export const MODEL_BATCH_MODES = ["static", "dynamic"];

export const MODEL_PRECISIONS = ["fp16"];

export const MODEL_STATUS_LABEL = {
  built: "Built",
  not_built: "Not Built",
  building: "Building"
} as const;

export const MODEL_LIST_COLUMN_RESIZABLE = false;

export const MODEL_CREATE_COLUMN_RESIZABLE = false;

export const MODEL_EDIT_COLUMN_RESIZABLE = false;

type ColumnAlign = "left" | "center" | "right";

export const MODEL_CLASSES_COLUMN_ALIGNS = {
  id: { header: "center", content: "center" },
  name: { header: "center", content: "left" }
} as const satisfies Record<string, { header: ColumnAlign; content: ColumnAlign }>;

export const MODEL_COLUMN_ALIGNS = {
  selection: { header: "center", content: "center" },
  name: { header: "center", content: "left" },
  source: { header: "center", content: "left" },
  type: { header: "center", content: "center" },
  batchSize: { header: "center", content: "center" },
  batchMode: { header: "center", content: "center" },
  precision: { header: "center", content: "center" },
  classes: { header: "center", content: "center" },
  status: { header: "center", content: "center" },
  builtAt: { header: "center", content: "center" },
  ops: { header: "center", content: "center" }
} as const satisfies Record<string, { header: ColumnAlign; content: ColumnAlign }>;

export const MODEL_CREATE_COLUMN_ALIGNS = {
  selection: { header: "center", content: "center" },
  name: { header: "center", content: "left" },
  type: { header: "center", content: "center" },
  batchSize: { header: "center", content: "center" },
  batchMode: { header: "center", content: "center" },
  precision: { header: "center", content: "center" },
  maxDet: { header: "center", content: "center" },
  conf: { header: "center", content: "center" },
  iou: { header: "center", content: "center" },
  ops: { header: "center", content: "center" }
} as const satisfies Record<string, { header: ColumnAlign; content: ColumnAlign }>;

export const MODEL_EDIT_COLUMN_ALIGNS = {
  name: { header: "center", content: "left" },
  source: { header: "center", content: "left" },
  type: { header: "center", content: "center" },
  batchSize: { header: "center", content: "center" },
  batchMode: { header: "center", content: "center" },
  precision: { header: "center", content: "center" },
  maxDet: { header: "center", content: "center" },
  conf: { header: "center", content: "center" },
  iou: { header: "center", content: "center" },
  ops: { header: "center", content: "center" }
} as const satisfies Record<string, { header: ColumnAlign; content: ColumnAlign }>;

export const MODEL_LIST_COLUMN_PERCENTS = {
  selection: 3,
  name: 12,
  source: 18,
  type: 10,
  batchSize: 6,
  batchMode: 8,
  precision: 6,
  classes: 5,
  status: 8,
  builtAt: 15,
  ops: 15
};

export const MODEL_CREATE_COLUMN_PERCENTS = {
  selection: 5,
  name: 27,
  type: 11,
  batchSize: 10,
  batchMode: 11,
  precision: 9,
  maxDet: 8,
  conf: 6,
  iou: 6,
  ops: 9
};

export const MODEL_EDIT_COLUMN_PERCENTS = {
  name: 17,
  source: 9,
  type: 11,
  batchSize: 10,
  batchMode: 11,
  precision: 9,
  maxDet: 15,
  conf: 6,
  iou: 5,
  ops: 7
};
