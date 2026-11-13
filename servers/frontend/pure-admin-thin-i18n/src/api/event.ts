import { http } from "@/utils/http";
import { PATH_EVENT_LIST } from "@/constants/path";

export type EventAnnotations = {
  num?: string | number;
  object?: string | number;
} | null;

export type EventItem = {
  id: string;
  occurred_at: string;
  event_code: string;
  event_label: string;
  annotations: EventAnnotations;
};

type EventListResult = {
  success: boolean;
  message: string;
  data: { items: EventItem[]; total: number; page: number; page_size: number };
};

export const getEvents = (params?: object) => {
  return http.request<EventListResult>("get", PATH_EVENT_LIST, { params });
};
