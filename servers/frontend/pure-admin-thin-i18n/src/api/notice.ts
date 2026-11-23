import { http } from "@/utils/http";
import { PATH_NOTICE_COUNT } from "@/constants/path";

export type NoticeCount = {
  total: number;
  event: number;
  system: number;
};

type NoticeCountResult = {
  success: boolean;
  message: string;
  data: NoticeCount;
};

export const getNoticeCount = () => {
  return http.request<NoticeCountResult>("get", PATH_NOTICE_COUNT);
};
