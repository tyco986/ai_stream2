import { http } from "@/utils/http";
import {
  PATH_FFMPEG_RTSP_PUBLISH,
  PATH_MEDIA_BATCH_DOWNLOAD,
  PATH_MEDIA_BATCH_REMOVE,
  PATH_MEDIA_BATCH_UPLOAD,
  PATH_MEDIA_CONTENT,
  PATH_MEDIA_UPLOAD_FILE
} from "@/constants/path";

export type MediaItem = {
  id: string;
  type: "image" | "video";
  name: string;
  folder: string;
  width: number | null;
  height: number | null;
  duration: string | null;
  fps: number | null;
  size: number | null;
  updated_at: string | null;
};

export type MediaBatchResult = {
  success: boolean;
  message: string;
  data: {
    updated_rows: MediaItem[];
    failed_rows: Record<string, unknown>[];
    skipped_rows: MediaItem[];
    updated_count: number;
    failed_count: number;
    skipped_count: number;
  };
};

export type MediaListResult = {
  success: boolean;
  message: string;
  data: {
    items: MediaItem[];
    total: number;
    page: number;
    page_size: number;
  };
};

export const getMediaContent = (mediaId: string) => {
  return http.request<Blob>("get", `${PATH_MEDIA_CONTENT}/${mediaId}/content`, {
    responseType: "blob",
    timeout: 0
  });
};

export const getUploadFiles = (params?: object) => {
  return http.request<MediaListResult>("get", PATH_MEDIA_UPLOAD_FILE, {
    params
  });
};

export const downloadMedia = (mediaId: string) => {
  return http.request<Blob>("get", `${PATH_MEDIA_CONTENT}/${mediaId}/download`, {
    responseType: "blob",
    timeout: 0
  });
};

export const downloadMediaBatch = (ids: string[]) => {
  return http.request<Blob>("post", PATH_MEDIA_BATCH_DOWNLOAD, {
    data: { ids },
    responseType: "blob",
    timeout: 0
  });
};

export const uploadMedia = (userId: string, files: File[]) => {
  const form = new FormData();
  form.append("user_id", userId);
  for (const file of files) {
    form.append("file", file);
  }
  return http.request<MediaBatchResult>("post", PATH_MEDIA_BATCH_UPLOAD, {
    data: form,
    timeout: 0
  });
};

export const removeMedia = (ids: string[]) => {
  return http.request<MediaBatchResult>("post", PATH_MEDIA_BATCH_REMOVE, {
    data: { ids }
  });
};

export type RtspPublishItem = {
  path: string;
  url_docker: string;
  width: number | null;
  height: number | null;
  fps: number | null;
  status: string;
  probed_at: string;
};

export type RtspPublishResult = {
  success: boolean;
  message: string;
  data: Record<string, RtspPublishItem>;
};

export const publishRtsp = (paths: Record<string, string>) => {
  return http.request<RtspPublishResult>("post", PATH_FFMPEG_RTSP_PUBLISH, {
    data: { paths }
  });
};

export const getPublishedRtsp = () => {
  return http.request<RtspPublishResult>("get", PATH_FFMPEG_RTSP_PUBLISH);
};

export const unpublishRtsp = (names: string[]) => {
  const query = names.map(name => `names=${encodeURIComponent(name)}`).join("&");
  const url = query ? `${PATH_FFMPEG_RTSP_PUBLISH}?${query}` : PATH_FFMPEG_RTSP_PUBLISH;
  return http.request<RtspPublishResult>("delete", url);
};
