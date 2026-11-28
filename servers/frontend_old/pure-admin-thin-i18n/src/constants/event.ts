export const EVENT_LIST_PAGE_SIZE = 20;
export const EVENT_BANNER_BATCH_MS = 8000;

export const EVENT_ACTION_INSERT = "insert";
export const EVENT_ACTION_READ = "read";
export const EVENT_ACTION_UNREAD = "unread";
export const EVENT_ACTION_REMOVE = "remove";
export const EVENT_ACTION_BATCH_READ = "batch_read";
export const EVENT_ACTION_BATCH_UNREAD = "batch_unread";
export const EVENT_ACTION_BATCH_REMOVE = "batch_remove";

export const EVENT_LEVEL_ALL = "all";
export const EVENT_LEVEL_ALERT = "alert";
export const EVENT_LEVEL_TRANSIT = "transit";

export const EVENT_LEVELS = [EVENT_LEVEL_ALL, EVENT_LEVEL_ALERT, EVENT_LEVEL_TRANSIT] as const;

export const EVENT_LEVEL_LABELS: Record<(typeof EVENT_LEVELS)[number], string> = {
  [EVENT_LEVEL_ALL]: "event.pureLevelAll",
  [EVENT_LEVEL_ALERT]: "event.pureLevelAlert",
  [EVENT_LEVEL_TRANSIT]: "event.pureLevelTransit"
};

export const EVENT_ANNOTATION_DIALOG_WIDTH = "40%";

export const EVENT_LIST_COLUMN_PERCENTS = {
  selection: 4,
  occurredAt: 16,
  eventCode: 10,
  eventLabel: 14,
  pipeline: 16,
  source: 16,
  annotations: 14,
  ops: 10
};
