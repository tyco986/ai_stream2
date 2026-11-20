import { http } from "@/utils/http";
import {
  PATH_GENERATOR_TYPES,
  PATH_PIPELINE,
  PATH_PIPELINE_BATCH_REMOVE,
  PATH_PIPELINE_START,
  PATH_PIPELINE_STOP,
  PATH_PIPELINE_PROBE,
  PATH_PIPELINE_LOGS_API,
  PATH_PIPELINE_LOGS_PAD,
  PATH_PIPELINE_LOGS_GSTREAMER,
  PATH_PIPELINE_LOGS,
  PATH_PIPELINE_LIST,
  PATH_PIPELINE_MAP
} from "@/constants/path";

export type PipelineScalar = string | number | boolean;

export type PipelineFields = Record<string, PipelineScalar>;

export type PipelineSpec = {
  generator: string;
  source: "image" | "video" | "rtsp";
  tracker: boolean;
  gie: Record<string, Record<string, boolean>>;
  logger?: PipelineFields;
  drawer?: PipelineFields;
  sahi?: Record<string, PipelineFields>;
  event_coder?: PipelineFields;
  capturer?: PipelineFields;
};

export type PipelineInput = Record<string, unknown>;

export type PipelineDraft = {
  name: string;
  type: string;
  input: PipelineInput;
};

export type PipelineItem = {
  id: string;
  name: string;
  type: string;
  status: string;
  gie_names: string[];
  analyzer_names: string[];
  start_time: string | null;
  end_time: string | null;
  duration: number | null;
};

type PipelineListResult = {
  success: boolean;
  message: string;
  data: { items: PipelineItem[]; total: number; page: number; page_size: number };
};

export type PipelineDetail = PipelineDraft & {
  id: string;
  status: string;
};

type PipelineResult = { success: boolean; message: string; data: PipelineDetail };

export const getPipelineTypes = () => {
  return http.request<{ success: boolean; message: string; data: Record<string, PipelineSpec> }>(
    "get",
    PATH_GENERATOR_TYPES
  );
};

export const getPipelineMap = () => {
  return http.request<{ success: boolean; message: string; data: Record<string, string> }>(
    "get",
    PATH_PIPELINE_MAP
  );
};

export const getPipelines = (params?: object) => {
  return http.request<PipelineListResult>("get", PATH_PIPELINE_LIST, { params });
};

const pipelinePath = (id: string) => PATH_PIPELINE.replace("{pipeline_id}", id);

export const getPipeline = (id: string) => {
  return http.request<PipelineResult>("get", pipelinePath(id));
};

export const createPipeline = (draft: PipelineDraft) => {
  return http.request<PipelineResult>("post", PATH_PIPELINE_LIST, { data: draft });
};

export const updatePipeline = (id: string, draft: PipelineDraft) => {
  return http.request<PipelineResult>("patch", pipelinePath(id), { data: draft });
};

export const startPipeline = (id: string) => {
  return http.request<PipelineResult>("post", PATH_PIPELINE_START.replace("{pipeline_id}", id), { timeout: 0 });
};

export const stopPipeline = (id: string) => {
  return http.request<PipelineResult>("post", PATH_PIPELINE_STOP.replace("{pipeline_id}", id), { timeout: 0 });
};

const pipelineLogPath = (path: string, id: string) => path.replace("{pipeline_id}", id);

export const getPipelineApiLogs = (id: string, date: string) => {
  return http.request<{ success: boolean; message: string; data: Record<string, { content: unknown[] }> }>(
    "get",
    pipelineLogPath(PATH_PIPELINE_LOGS_API, id),
    { params: { date } }
  );
};

export const getPipelinePadLogs = (id: string, pad: string, date: string) => {
  return http.request<{ success: boolean; message: string; data: { content: unknown[] } }>(
    "get",
    pipelineLogPath(PATH_PIPELINE_LOGS_PAD, id),
    { params: { pad, date } }
  );
};

export const removePipelineLogs = (id: string) => {
  return http.request<{ success: boolean; message: string; data: Record<string, never> }>(
    "delete",
    pipelineLogPath(PATH_PIPELINE_LOGS, id)
  );
};

export const getPipelineGstreamerLogs = (id: string, date: string) => {
  return http.request<{ success: boolean; message: string; data: { content: string } }>(
    "get",
    pipelineLogPath(PATH_PIPELINE_LOGS_GSTREAMER, id),
    { params: { date } }
  );
};

export const probePipeline = (id: string) => {
  return http.request<PipelineResult>("post", PATH_PIPELINE_PROBE.replace("{pipeline_id}", id), { timeout: 0 });
};

export const removePipeline = (id: string) => {
  return http.request<PipelineResult>("delete", pipelinePath(id), { timeout: 0 });
};

export const removePipelines = (ids: string[]) => {
  return http.request<{ success: boolean; message: string }>("post", PATH_PIPELINE_BATCH_REMOVE, {
    data: { ids },
    timeout: 0
  });
};
