export const MODEL_STATUS_FIRST_MS = 500;
export const MODEL_STATUS_POLL_MS = 2000;

export const MODEL_BATCH_MODES = ["static", "dynamic"];

export const MODEL_PRECISIONS = ["fp16"];

export const MODEL_LIST_COLUMN_PERCENTS = {
  selection: 5,
  name: 12,
  source: 8,
  type: 6,
  batchSize: 8,
  batchMode: 9,
  precision: 9,
  device: 8,
  classes: 8,
  status: 8,
  builtAt: 8,
  ops: 11
};

export const MODEL_EDIT_COLUMN_PERCENTS = {
  name: 17,
  source: 9,
  type: 11,
  batchSize: 10,
  batchMode: 11,
  precision: 9,
  device: 7,
  maxDet: 8,
  conf: 6,
  iou: 5,
  ops: 7
};

export const MODEL_CREATE_COLUMN_PERCENTS = {
  selection: 5,
  name: 21,
  type: 11,
  batchSize: 10,
  batchMode: 11,
  precision: 9,
  device: 7,
  maxDet: 8,
  conf: 6,
  iou: 5,
  ops: 7
};
