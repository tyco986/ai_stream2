import { http } from "@/utils/http";
import {
  PATH_MODEL,
  PATH_MODEL_BATCH_CREATE,
  PATH_MODEL_BATCH_REMOVE,
  PATH_MODEL_BUILD,
  PATH_MODEL_LIST,
  PATH_MODEL_MAP,
  PATH_MODEL_STATUS,
  PATH_MODEL_UPLOAD,
  PATH_MODEL_TYPES,
  PATH_DEVICE
} from "@/constants/path";

export type ModelItem = {
  id: string;
  name: string;
  type: string;
  version: string | null;
  batch_size: number;
  batch_mode: string;
  input_tensors: unknown;
  output_tensors: unknown;
  precision: string;
  opt_level: number | null;
  device: number;
  workspace: number;
  static_plugin: string | null;
  max_det: number | null;
  conf: number | null;
  iou: number | null;
  num_class: number | null;
  classes: unknown;
  source_path: string | null;
  source_name: string | null;
  engine_name: string | null;
  status: "not_built" | "building" | "built";
  built_at: string | null;
  trt_version: string | null;
  ever_built: boolean;
};

export type ModelDraft = {
  name: string;
  type: string;
  batch_size: number;
  batch_mode: string;
  precision: string;
  max_det: number | null;
  conf: number | null;
  iou: number | null;
  device: number;
};

export type ModelListResult = {
  success: boolean;
  message: string;
  data: {
    items: ModelItem[];
    total: number;
    page: number;
    page_size: number;
  };
};

export type ModelStatus = {
  id: string;
  status: ModelItem["status"];
  built_at: string | null;
  version: string | null;
  num_class: number | null;
  classes: unknown;
};

export type ModelResult = {
  success: boolean;
  message: string;
  data: ModelItem;
};

export type ModelBatchResult = {
  success: boolean;
  message: string;
};

const modelForm = (draft: ModelDraft) => {
  const form = new FormData();
  const fields: Record<string, string | number | null> = {
    name: draft.name,
    type: draft.type,
    batch_size: draft.batch_size,
    batch_mode: draft.batch_mode,
    precision: draft.precision,
    max_det: draft.max_det,
    conf: draft.conf,
    iou: draft.iou,
    device: draft.device
  };
  for (const [key, value] of Object.entries(fields)) {
    value !== null && value !== "" && form.append(key, String(value));
  }
  return form;
};

export type ModelTypeDefaults = {
  batch_size: number;
  batch_mode: string;
  precision: string;
  device: number;
  opt_level: number | null;
  max_det: number | null;
  conf: number | null;
  iou: number | null;
  source_suffixes: string[];
};

export const getDevices = () => {
  return http.request<{ success: boolean; data: Record<string, { name: string }> }>("get", PATH_DEVICE);
};

export const getModelTypes = () => {
  return http.request<{ success: boolean; data: Record<string, ModelTypeDefaults> }>(
    "get",
    PATH_MODEL_TYPES
  );
};

export const getModelMap = () => {
  return http.request<{ success: boolean; message: string; data: Record<string, string> }>(
    "get",
    PATH_MODEL_MAP
  );
};

export const getModels = (params?: object) => {
  return http.request<ModelListResult>("get", PATH_MODEL_LIST, { params });
};

export const createModels = (items: ModelDraft[]) => {
  return http.request<ModelBatchResult>("post", PATH_MODEL_BATCH_CREATE, {
    data: { items }
  });
};

export const removeModels = (ids: string[]) => {
  return http.request<ModelBatchResult>("post", PATH_MODEL_BATCH_REMOVE, {
    data: { ids }
  });
};

const modelPath = (id: string) => PATH_MODEL.replace("{model_id}", id);

export const getModel = (id: string) => {
  return http.request<ModelResult>("get", modelPath(id));
};

export const patchModel = (id: string, draft: ModelDraft) => {
  return http.request<ModelResult>("patch", modelPath(id), {
    data: modelForm(draft)
  });
};

export const removeModel = (id: string) => {
  return http.request<ModelResult>("delete", modelPath(id));
};

export const uploadModel = (id: string, file: File) => {
  const form = new FormData();
  form.append("input", file);
  return http.request<ModelResult>(
    "post",
    PATH_MODEL_UPLOAD.replace("{model_id}", id),
    { data: form, timeout: 0 }
  );
};

export const buildModel = (id: string) => {
  return http.request<ModelResult>(
    "post",
    PATH_MODEL_BUILD.replace("{model_id}", id),
    { timeout: 0 }
  );
};

export const getModelStatus = (id: string) => {
  return http.request<{ success: boolean; data: ModelStatus }>(
    "get",
    PATH_MODEL_STATUS.replace("{model_id}", id)
  );
};
