import { http } from "@/utils/http";
import {
  PATH_EVENT_BATCH_READ,
  PATH_EVENT_BATCH_REMOVE,
  PATH_EVENT_BATCH_UNREAD,
  PATH_EVENT_ITEM,
  PATH_EVENT_LIST,
  PATH_EVENT_READ,
  PATH_EVENT_UNREAD
} from "@/constants/path";

export type EventAnnotations = {
  num?: string | number;
  object?: string | number;
} | null;

export type EventItem = {
  id: string;
  occurred_at: string;
  stream_id: string | null;
  stream_name: string | null;
  media_id: string | null;
  media_name: string | null;
  pipeline_id: string;
  pipeline_name: string | null;
  read_at: string | null;
  event_code: string;
  event_label: string;
  annotations: EventAnnotations;
};

type EventListResult = {
  success: boolean;
  message: string;
  data: { items: EventItem[]; total: number; page: number; page_size: number };
};

type EventResult = {
  success: boolean;
  message: string;
  data: EventItem;
};

type EventBatchResult = {
  success: boolean;
  message: string;
  data: {
    updated_rows: EventItem[];
    failed_rows: { id: string }[];
    updated_count: number;
    failed_count: number;
  };
};

const eventPath = (path: string, id: string) => path.replace("{event_id}", id);

export const getEvents = (params?: object) => {
  return http.request<EventListResult>("get", PATH_EVENT_LIST, { params });
};

export const readEvent = (id: string) => {
  return http.request<EventResult>("post", eventPath(PATH_EVENT_READ, id));
};

export const unreadEvent = (id: string) => {
  return http.request<EventResult>("post", eventPath(PATH_EVENT_UNREAD, id));
};

export const removeEvent = (id: string) => {
  return http.request<EventResult>("delete", eventPath(PATH_EVENT_ITEM, id));
};

export const readEvents = (ids: string[]) => {
  return http.request<EventBatchResult>("post", PATH_EVENT_BATCH_READ, { data: { ids } });
};

export const unreadEvents = (ids: string[]) => {
  return http.request<EventBatchResult>("post", PATH_EVENT_BATCH_UNREAD, { data: { ids } });
};

export const removeEvents = (ids: string[]) => {
  return http.request<EventBatchResult>("post", PATH_EVENT_BATCH_REMOVE, { data: { ids } });
};
