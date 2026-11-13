export const ALL_GROUP_ID = "00000000-0000-4000-8000-000000000001";

export const STREAM_GROUP_DIALOG_WIDTH = "50%";

export const STREAM_GROUP_ROW_HEIGHT = 20;

export const STREAM_GROUP_TREE_INDENT = 10;

export const STREAM_GROUP_COLUMN_RESIZABLE = false;

type ColumnAlign = "left" | "center" | "right";

export const STREAM_GROUP_COLUMN_ALIGNS = {
  selection: { header: "center", content: "center" },
  name: { header: "center", content: "left" },
  group: { header: "center", content: "center" }
} as const satisfies Record<string, { header: ColumnAlign; content: ColumnAlign }>;

export const STREAM_GROUP_COLUMN_PERCENTS = {
  selection: 12,
  name: 44,
  group: 44
};

