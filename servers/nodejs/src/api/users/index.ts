/**
 * Users API client. Implementation selected by Vite mode (real → http, mock → mock).
 */

import * as api from './runtime'

export type {
  BatchDeleteUsersResult,
  CreateGroupBody,
  CreateUserBody,
  GroupItem,
  GroupRef,
  ListUsersQuery,
  PagedUserLogs,
  PagedUsers,
  PatchGroupBody,
  PatchUserBody,
  PermissionAction,
  PermissionCatalog,
  PermissionModule,
  UserItem,
  UserLogEntry,
} from '@/api/users/types'

export { formatLastAction, formatLogAt } from '@/api/users/utils'

export const USERS_API_BASE = api.USERS_API_BASE
export const getPermissionCatalog = api.getPermissionCatalog
export const listGroups = api.listGroups
export const getGroup = api.getGroup
export const createGroup = api.createGroup
export const patchGroup = api.patchGroup
export const deleteGroup = api.deleteGroup
export const listUsers = api.listUsers
export const getUser = api.getUser
export const createUser = api.createUser
export const patchUser = api.patchUser
export const deleteUser = api.deleteUser
export const batchDeleteUsers = api.batchDeleteUsers
export const listUserLogs = api.listUserLogs
