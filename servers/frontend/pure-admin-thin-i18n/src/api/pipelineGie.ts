import { http } from "@/utils/http";
import { PATH_PIPELINE_GIE } from "@/constants/path";

export type GieClassAttrs = Record<string, Record<string, number>>;

export type GieDraft = {
  model_id: string;
  name: string;
  class_attrs: GieClassAttrs;
  interval: number;
};

export const createGie = (draft: GieDraft) => {
  return http.request<{ success: boolean; message: string }>("post", PATH_PIPELINE_GIE, {
    data: draft
  });
};
