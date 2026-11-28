import { http } from "@/utils/http";
import {
  PATH_LOGS,
  PATH_LOGS_DISABLE,
  PATH_LOGS_ENABLE,
  PATH_LOGS_STATUS
} from "@/constants/path";

export type LogStatusResult = {
  success: boolean;
  message: string;
  data: {
    enabled: boolean;
  };
};

export type LogContent = {
  content: string;
};

export type LogGetResult = {
  success: boolean;
  message: string;
  data: Record<string, Record<string, LogContent>>;
};

export type LogRemoveResult = {
  success: boolean;
  message: string;
  data: Record<string, number>;
};

export const getLogStatus = () => {
  return http.request<LogStatusResult>("get", PATH_LOGS_STATUS);
};

export const getLogs = (page: string, date: string) => {
  return http.request<LogGetResult>("get", PATH_LOGS, {
    params: { page, date }
  });
};

export const removeLogs = (page: string) => {
  return http.request<LogRemoveResult>("delete", PATH_LOGS, {
    params: { page }
  });
};

export const enableLogs = () => {
  return http.request<LogStatusResult>("post", PATH_LOGS_ENABLE);
};

export const disableLogs = () => {
  return http.request<LogStatusResult>("post", PATH_LOGS_DISABLE);
};
