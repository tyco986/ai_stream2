import { http } from "@/utils/http";
import { PATH_PIPELINE_ANALYZER } from "@/constants/path";

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

export const createAnalyzer = (draft: AnalyzerDraft) => {
  return http.request<AnalyzerResult>("post", PATH_PIPELINE_ANALYZER, { data: draft });
};
