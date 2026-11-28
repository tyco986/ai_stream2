import { http } from "@/utils/http";
import { PATH_USER_LOGIN, PATH_USER_ME } from "@/constants/path";

export type LoginResult = {
  success: boolean;
  message: string;
  data: UserMe & {
    token: string;
  };
};

export type UserMe = {
  id: string;
  username: string;
  active: boolean;
  group_id: string | null;
  group_name: string | null;
  last_login: string | null;
};

export type UserMeResult = {
  success: boolean;
  message: string;
  data: UserMe;
};

export const getLogin = (data: { username: string; passwd: string }) => {
  return http.request<LoginResult>("post", PATH_USER_LOGIN, { data });
};

export const getMe = () => {
  return http.request<UserMeResult>("get", PATH_USER_ME);
};
