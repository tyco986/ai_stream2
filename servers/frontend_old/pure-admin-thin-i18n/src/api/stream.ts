import { http } from "@/utils/http";
import {
  PATH_FFMPEG_RTSP_PROBE,
  PATH_STREAM,
  PATH_STREAM_BATCH_CREATE,
  PATH_STREAM_LIST,
  PATH_STREAM_PROBE
} from "@/constants/path";

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
  mount: boolean;
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
  mount?: boolean;
  record?: boolean;
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
  return http.request<{ success: boolean; message: string }>("post", PATH_STREAM_BATCH_CREATE, {
    data: { items }
  });
};

export const batchStreams = (path: string, ids: string[]) => {
  return http.request<{ success: boolean; message: string }>("post", path, {
    data: { ids }
  });
};

const streamPath = (id: string) => PATH_STREAM.replace("{stream_id}", id);

export const patchStream = (id: string, data: Partial<StreamDraft>) => {
  return http.request<{ success: boolean; message: string }>("patch", streamPath(id), { data });
};

export const probeStream = (id: string) => {
  return http.request<{ success: boolean; message: string }>(
    "post",
    PATH_STREAM_PROBE.replace("{stream_id}", id)
  );
};

export const removeStream = (id: string) => {
  return http.request<{ success: boolean; message: string }>("delete", streamPath(id));
};

export const probeRtsp = (url: string) => {
  return http.request<RtspProbeResult>("post", PATH_FFMPEG_RTSP_PROBE, {
    data: { urls: [url] }
  });
};
