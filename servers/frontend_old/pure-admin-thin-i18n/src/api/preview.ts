import { http } from "@/utils/http";
import { PATH_PREVIEW, PATH_PREVIEW_ACTIVE, PATH_PREVIEW_ITEM, PATH_PREVIEW_MAP } from "@/constants/preview";
import type { StreamItem } from "@/api/stream";

export type Preset = {
  id: string;
  user_id: string;
  name: string;
  layout: string;
  mode: string;
  slots: Record<string, StreamItem | null>;
  active: boolean;
};

export type PresetResult = {
  success: boolean;
  message: string;
  data: Preset;
};

export type PresetDraft = {
  name: string;
  layout: string;
  mode: string;
  slots: Record<string, string | null>;
  active: boolean;
};

export type PresetMapResult = {
  success: boolean;
  message: string;
  data: Record<string, string>;
};

export const getPresetMap = () => {
  return http.request<PresetMapResult>("get", PATH_PREVIEW_MAP);
};

export const getActivePreset = () => {
  return http.request<PresetResult>("get", PATH_PREVIEW_ACTIVE);
};

export const getPreset = (presetId: string) => {
  return http.request<PresetResult>("get", PATH_PREVIEW_ITEM.replace("{preset_id}", presetId));
};

export const createPreset = (data: PresetDraft) => {
  return http.request<PresetResult>("post", PATH_PREVIEW, { data });
};

export const updatePreset = (presetId: string, data: PresetDraft) => {
  return http.request<PresetResult>("patch", PATH_PREVIEW_ITEM.replace("{preset_id}", presetId), { data });
};
