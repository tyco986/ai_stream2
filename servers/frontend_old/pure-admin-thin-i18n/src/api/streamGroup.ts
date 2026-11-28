import { http } from "@/utils/http";
import {
  PATH_STREAM_GROUP,
  PATH_STREAM_GROUP_CANDIDATES,
  PATH_STREAM_GROUP_CREATE,
  PATH_STREAM_GROUP_MAP,
  PATH_STREAM_GROUP_MEMBERS,
  PATH_STREAM_GROUP_TREE
} from "@/constants/path";

export type StreamTreeNode = {
  id: string;
  name: string;
  type: "stream";
  mount: string;
  status: string;
  record: boolean;
};

export type GroupTreeNode = {
  id: string;
  name: string;
  type: "group";
  children: TreeNode[];
};

export type TreeNode = GroupTreeNode | StreamTreeNode;

export type GroupTreeResult = {
  success: boolean;
  message: string;
  data: GroupTreeNode;
};

export type GroupResult = {
  success: boolean;
  message: string;
  data: {
    id: string;
    name: string;
    parent_id: string | null;
  };
};

export const getGroupTree = () => {
  return http.request<GroupTreeResult>("get", PATH_STREAM_GROUP_TREE);
};

export const createGroup = (parentId: string, name: string) => {
  return http.request<GroupResult>("post", PATH_STREAM_GROUP_CREATE.replace("{parent_group_id}", parentId), {
    data: { name }
  });
};

export const parentGroupId = (id: string, nodes: TreeNode[], parentId = ""): string => {
  const groups = nodes.filter((node): node is GroupTreeNode => node.type === "group");
  const direct = groups.some(node => node.id === id);
  const nested = groups.map(node => parentGroupId(id, node.children, node.id)).find(found => found !== "");
  return direct ? parentId : nested || "";
};

export const updateGroup = (groupId: string, name: string, parentId: string) => {
  return http.request<GroupResult>("patch", PATH_STREAM_GROUP.replace("{group_id}", groupId), {
    data: { name, parent_id: parentId }
  });
};

export const removeGroup = (groupId: string) => {
  return http.request<GroupTreeResult>(
    "delete",
    PATH_STREAM_GROUP.replace("{group_id}", groupId)
  );
};

export type GroupStream = {
  id: string;
  name: string;
  group_id: string;
  group_name: string;
};

export type GroupStreamResult = {
  success: boolean;
  message: string;
  data: GroupStream[];
};

export type GroupMapResult = {
  success: boolean;
  message: string;
  data: Record<string, string>;
};

export const getGroupMap = () => {
  return http.request<GroupMapResult>("get", PATH_STREAM_GROUP_MAP);
};

export const getGroupMembers = (groupId: string) => {
  return http.request<GroupStreamResult>("get", PATH_STREAM_GROUP_MEMBERS.replace("{group_id}", groupId));
};

export const getGroupCandidates = (groupId: string) => {
  return http.request<GroupStreamResult>(
    "get",
    PATH_STREAM_GROUP_CANDIDATES.replace("{group_id}", groupId)
  );
};

export const setGroupMembers = (groupId: string, streamIds: string[]) => {
  return http.request<GroupTreeResult>("put", PATH_STREAM_GROUP_MEMBERS.replace("{group_id}", groupId), {
    data: { stream_ids: streamIds }
  });
};

export const groupById = (id: string, nodes: TreeNode[]): GroupTreeNode | null => {
  const groups = nodes.filter((node): node is GroupTreeNode => node.type === "group");
  const direct = groups.find(node => node.id === id);
  const nested = groups.map(node => groupById(id, node.children)).find(node => node);
  return direct || nested || null;
};

export const subtreeGroupIds = (node: GroupTreeNode): string[] => {
  const children = node.children.filter((child): child is GroupTreeNode => child.type === "group");
  return [node.id, ...children.flatMap(subtreeGroupIds)];
};
