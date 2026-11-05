import { http } from "@/utils/http";
import {
  PATH_PIPELINE_ANALYZER,
  PATH_PIPELINE_ANALYZER_BATCH_REMOVE,
  PATH_PIPELINE_ANALYZER_ITEM
} from "@/constants/path";

export type AnalyzerItem = {
  id: string;
  name: string;
  media_id: string | null;
  annotations: Record<string, unknown>;
  updated_at: string | null;
};

type AnalyzerDraft = {
  name: string;
  media_id: string;
  annotations: Record<string, unknown>;
};

type AnalyzerResult = { success: boolean; message: string; data: AnalyzerItem };

type AnalyzerListResult = {
  success: boolean;
  message: string;
  data: { items: AnalyzerItem[]; total: number; page: number; page_size: number };
};

export const getAnalyzers = (params?: object) => {
  return http.request<AnalyzerListResult>("get", PATH_PIPELINE_ANALYZER, { params });
};

const analyzerPath = (id: string) => PATH_PIPELINE_ANALYZER_ITEM.replace("{analyzer_id}", id);

export const getAnalyzer = (id: string) => {
  return http.request<AnalyzerResult>("get", analyzerPath(id));
};

export const createAnalyzer = (draft: AnalyzerDraft) => {
  return http.request<AnalyzerResult>("post", PATH_PIPELINE_ANALYZER, { data: draft });
};

export const updateAnalyzer = (id: string, draft: AnalyzerDraft) => {
  return http.request<AnalyzerResult>("put", analyzerPath(id), { data: draft });
};

export const removeAnalyzer = (id: string) => {
  return http.request<AnalyzerResult>("delete", analyzerPath(id));
};

export const removeAnalyzers = (ids: string[]) => {
  return http.request<{ success: boolean; message: string }>("post", PATH_PIPELINE_ANALYZER_BATCH_REMOVE, {
    data: { ids }
  });
};
