import { http } from "@/utils/http";
import {
  PATH_PIPELINE_GIE,
  PATH_PIPELINE_GIE_BATCH_REMOVE,
  PATH_PIPELINE_GIE_ITEM
} from "@/constants/path";

export type GieClassAttrs = Record<string, Record<string, number>>;

export type GieDraft = {
  model_id: string;
  name: string;
  class_attrs: GieClassAttrs;
  interval: number;
};

export type GieItem = GieDraft & {
  id: string;
  model_name: string | null;
  engine_path: string | null;
  updated_at: string | null;
};

type GieResult = { success: boolean; message: string; data: GieItem };

type GieListResult = {
  success: boolean;
  message: string;
  data: { items: GieItem[]; total: number; page: number; page_size: number };
};

const giePath = (id: string) => PATH_PIPELINE_GIE_ITEM.replace("{gie_id}", id);

export const getGies = (params?: object) => {
  return http.request<GieListResult>("get", PATH_PIPELINE_GIE, { params });
};

export const getGie = (id: string) => {
  return http.request<GieResult>("get", giePath(id));
};

export const createGie = (draft: GieDraft) => {
  return http.request<GieResult>("post", PATH_PIPELINE_GIE, { data: draft });
};

export const updateGie = (id: string, draft: GieDraft) => {
  return http.request<GieResult>("put", giePath(id), { data: draft });
};

export const removeGie = (id: string) => {
  return http.request<GieResult>("delete", giePath(id));
};

export const removeGies = (ids: string[]) => {
  return http.request<{ success: boolean; message: string }>("post", PATH_PIPELINE_GIE_BATCH_REMOVE, {
    data: { ids }
  });
};
