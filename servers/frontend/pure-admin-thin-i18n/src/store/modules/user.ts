import { defineStore } from "pinia";
import {
  type userType,
  store,
  router,
  resetRouter,
  routerArrays,
  storageLocal
} from "../utils";
import { type LoginResult, getLogin, getMe } from "@/api/user";
import { useMultiTagsStoreHook } from "./multiTags";
import { type DataInfo, setToken, removeToken, userKey } from "@/utils/auth";

export const useUserStore = defineStore("pure-user", {
  state: (): userType => ({
    // 头像
    avatar: storageLocal().getItem<DataInfo<number>>(userKey)?.avatar ?? "",
    // 用户名
    username: storageLocal().getItem<DataInfo<number>>(userKey)?.username ?? "",
    userId: storageLocal().getItem<DataInfo<number>>(userKey)?.userId ?? "",
    // 昵称
    nickname: storageLocal().getItem<DataInfo<number>>(userKey)?.nickname ?? "",
    // 页面级别权限
    roles: storageLocal().getItem<DataInfo<number>>(userKey)?.roles ?? [],
    // 按钮级别权限
    permissions:
      storageLocal().getItem<DataInfo<number>>(userKey)?.permissions ?? [],
    // 是否勾选了登录页的免登录
    isRemembered: false,
    // 登录页的免登录存储几天，默认7天
    loginDay: 7
  }),
  actions: {
    /** 存储头像 */
    SET_AVATAR(avatar: string) {
      this.avatar = avatar;
    },
    /** 存储用户名 */
    SET_USERNAME(username: string) {
      this.username = username;
    },
    /** 存储昵称 */
    SET_NICKNAME(nickname: string) {
      this.nickname = nickname;
    },
    /** 存储角色 */
    SET_ROLES(roles: Array<string>) {
      this.roles = roles;
    },
    /** 存储按钮级别权限 */
    SET_PERMS(permissions: Array<string>) {
      this.permissions = permissions;
    },
    /** 存储是否勾选了登录页的免登录 */
    SET_ISREMEMBERED(bool: boolean) {
      this.isRemembered = bool;
    },
    /** 设置登录页的免登录存储几天 */
    SET_LOGINDAY(value: number) {
      this.loginDay = Number(value);
    },
    /** 登入 */
    async loginByUsername(data: { username: string; passwd: string }) {
      return new Promise<LoginResult>((resolve, reject) => {
        getLogin(data)
          .then(result => {
            if (result?.success) {
              setToken(result.data.token);
              this.applyUser(result.data.id, result.data.username);
            }
            return result;
          })
          .then(result => {
            resolve(result);
          })
          .catch(error => {
            reject(error);
          });
      });
    },
    applyUser(id: string, username: string) {
      const stored = storageLocal().getItem<DataInfo<number>>(userKey) ?? {};
      this.userId = id;
      this.SET_USERNAME(username);
      storageLocal().setItem(userKey, {
        ...stored,
        userId: id,
        username
      });
    },
    loadMe() {
      return getMe().then(result => {
        this.applyUser(result.data.id, result.data.username);
        return result;
      });
    },
    /** 清本地登录态，不跳转 */
    clearSession() {
      this.username = "";
      this.userId = "";
      this.roles = [];
      this.permissions = [];
      removeToken();
      useMultiTagsStoreHook().handleTags("equal", [...routerArrays]);
      resetRouter();
    },
    /** 前端登出（不调用接口） */
    logOut() {
      this.clearSession();
      router.push("/login");
    }
  }
});

export function useUserStoreHook() {
  return useUserStore(store);
}
