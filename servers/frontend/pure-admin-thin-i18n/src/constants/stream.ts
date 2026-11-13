export const STREAM_STATUS_LABEL = {
  online: "Online",
  offline: "Offline"
} as const;

export const STREAM_DIALOG_WIDTH = "80%";

export const STREAM_ROW_HEIGHT = 40;

export const STREAM_LIST_COLUMN_RESIZABLE = false;

export const STREAM_CREATE_COLUMN_RESIZABLE = false;

export const STREAM_EDIT_COLUMN_RESIZABLE = false;

type ColumnAlign = "left" | "center" | "right";

export const STREAM_COLUMN_ALIGNS = {
  selection: { header: "center", content: "center" },
  name: { header: "center", content: "left" },
  url: { header: "center", content: "left" },
  group: { header: "center", content: "center" },
  width: { header: "center", content: "center" },
  height: { header: "center", content: "center" },
  fps: { header: "center", content: "center" },
  status: { header: "center", content: "center" },
  probedAt: { header: "center", content: "center" },
  ops: { header: "center", content: "center" }
} as const satisfies Record<string, { header: ColumnAlign; content: ColumnAlign }>;

export const STREAM_CREATE_COLUMN_ALIGNS = {
  selection: { header: "center", content: "center" },
  name: { header: "center", content: "left" },
  url: { header: "center", content: "left" },
  group: { header: "center", content: "center" },
  width: { header: "center", content: "center" },
  height: { header: "center", content: "center" },
  fps: { header: "center", content: "center" },
  status: { header: "center", content: "center" },
  probedAt: { header: "center", content: "center" },
  ops: { header: "center", content: "center" }
} as const satisfies Record<string, { header: ColumnAlign; content: ColumnAlign }>;

export const STREAM_LIST_COLUMN_PERCENTS = {
  selection: 4,
  name: 10,
  url: 23,
  group: 8,
  width: 6,
  height: 7,
  fps: 5,
  status: 8,
  probedAt: 16,
  ops: 13
};

export const STREAM_CREATE_COLUMN_PERCENTS = {
  selection: 4,
  name: 12,
  url: 26,
  group: 10,
  width: 8,
  height: 8,
  fps: 6,
  status: 8,
  probedAt: 10,
  ops: 8
};

