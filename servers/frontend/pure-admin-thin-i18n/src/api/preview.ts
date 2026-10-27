import { http } from "@/utils/http";
import { PATH_PREVIEW_ACTIVE } from "@/constants/preview";
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

export const getActivePreset = () => {
  return http.request<PresetResult>("get", PATH_PREVIEW_ACTIVE);
};
