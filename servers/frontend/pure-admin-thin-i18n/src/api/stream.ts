import { http } from "@/utils/http";
import { PATH_FFMPEG_RTSP_PROBE, PATH_STREAM_BATCH_CREATE, PATH_STREAM_LIST } from "@/constants/path";

export type StreamItem = {
  id: string;
  name: string;
  group_id: string | null;
  group_name: string | null;
  url: string;
  width: number | null;
  height: number | null;
  fps: number | null;
  status: "online" | "offline";
  enabled: boolean;
  record: boolean;
  probed_at: string | null;
};

export type StreamListResult = {
  success: boolean;
  data: {
    items: StreamItem[];
    total: number;
    page: number;
    page_size: number;
  };
};

export type StreamDraft = {
  name: string;
  group_id: string;
  url: string;
  width?: number | null;
  height?: number | null;
  fps?: number | null;
  status?: string | null;
  probed_at?: string | null;
};

export type RtspProbeResult = {
  success: boolean;
  data: Record<string, {
    status: string;
    width: number | null;
    height: number | null;
    fps: number | null;
    probed_at: string;
  }>;
};

export const getStreams = (params?: object) => {
  return http.request<StreamListResult>("get", PATH_STREAM_LIST, {
    params
  });
};

export const createStreams = (items: StreamDraft[]) => {
  return http.request<StreamListResult>("post", PATH_STREAM_BATCH_CREATE, {
    data: { items }
  });
};

export const probeRtsp = (url: string) => {
  return http.request<RtspProbeResult>("post", PATH_FFMPEG_RTSP_PROBE, {
    data: { urls: [url] }
  });
};
