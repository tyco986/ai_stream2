import { http } from "@/utils/http";
import {
  PATH_MEDIA_PUBLISH,
  PATH_MEDIA_PUBLISH_LIST,
  PATH_MEDIA_BATCH_DOWNLOAD,
  PATH_MEDIA_BATCH_REMOVE,
  PATH_MEDIA_BATCH_REMOVE_FOLDER,
  PATH_MEDIA_BATCH_UPLOAD,
  PATH_MEDIA,
  PATH_MEDIA_CONTENT,
  PATH_MEDIA_DOWNLOAD,
  PATH_MEDIA_UPLOAD_FILE,
  PATH_MEDIA_PIPELINE_FILE,
  PATH_MEDIA_PIPELINE_FOLDER,
  PATH_MEDIA_RECORDING_FILE,
  PATH_MEDIA_RECORDING_FOLDER
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

export type MediaFolderResult = {
  success: boolean;
  message: string;
  data: Record<string, string>;
};

const mediaPath = (path: string, mediaId: string) => path.replace("{media_id}", mediaId);

export const getMedia = (mediaId: string) => {
  return http.request<{ success: boolean; message: string; data: MediaItem }>("get", mediaPath(PATH_MEDIA, mediaId));
};

export const getMediaContent = (mediaId: string) => {
  return http.request<Blob>("get", mediaPath(PATH_MEDIA_CONTENT, mediaId), {
    responseType: "blob",
    timeout: 0
  });
};

export const getUploadFiles = (params?: object) => {
  return http.request<MediaListResult>("get", PATH_MEDIA_UPLOAD_FILE, {
    params
  });
};

export const getPipelineFiles = (params?: object) => {
  return http.request<MediaListResult>("get", PATH_MEDIA_PIPELINE_FILE, {
    params
  });
};

export const getRecordingFiles = (params?: object) => {
  return http.request<MediaListResult>("get", PATH_MEDIA_RECORDING_FILE, {
    params
  });
};

export const getPipelineFolders = () => {
  return http.request<MediaFolderResult>("get", PATH_MEDIA_PIPELINE_FOLDER);
};

export const getRecordingFolders = () => {
  return http.request<MediaFolderResult>("get", PATH_MEDIA_RECORDING_FOLDER);
};

export const downloadMedia = (mediaId: string) => {
  return http.request<Blob>("get", mediaPath(PATH_MEDIA_DOWNLOAD, mediaId), {
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

export const removeMediaFolder = (folders: string[]) => {
  return http.request<MediaBatchResult>("post", PATH_MEDIA_BATCH_REMOVE_FOLDER, {
    data: { folders }
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

export type RtspPublishResponse = {
  success: boolean;
  message: string;
  data: RtspPublishItem;
};

export const publishMedia = (mediaId: string) => {
  return http.request<RtspPublishResponse>("post", PATH_MEDIA_PUBLISH.replace("{media_id}", mediaId));
};

export const getPublishedMedia = () => {
  return http.request<RtspPublishResult>("get", PATH_MEDIA_PUBLISH_LIST);
};

export const unpublishMedia = (mediaId: string) => {
  return http.request<RtspPublishResponse>("delete", PATH_MEDIA_PUBLISH.replace("{media_id}", mediaId));
};
