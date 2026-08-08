/**
 * Users API — real HTTP client.
 */

import { requestJson } from '@/shared/http/request'
import { BACKEND_PREFIX } from '@/shared/project'
import type {
  BatchDeleteUsersResult,
  CreateGroupBody,
  CreateUserBody,
  GroupItem,
  ListUsersQuery,
  PagedUserLogs,
  PagedUsers,
  PatchGroupBody,
  PatchUserBody,
  PermissionCatalog,
  UserItem,
} from '@/api/users/types'

export const USERS_API_BASE = `${BACKEND_PREFIX}/users`

export async function getPermissionCatalog(): Promise<PermissionCatalog> {
  return requestJson<PermissionCatalog>(`${USERS_API_BASE}/permissions/catalog`)
}

export async function listGroups(): Promise<{ items: GroupItem[] }> {
  return requestJson<{ items: GroupItem[] }>(`${USERS_API_BASE}/groups`)
}

export async function getGroup(groupId: string): Promise<GroupItem> {
  return requestJson<GroupItem>(`${USERS_API_BASE}/groups/${groupId}`)
}

export async function createGroup(body: CreateGroupBody): Promise<GroupItem> {
  return requestJson<GroupItem>(`${USERS_API_BASE}/groups`, {
    method: 'POST',
    body,
  })
}

export async function patchGroup(groupId: string, body: PatchGroupBody): Promise<GroupItem> {
  return requestJson<GroupItem>(`${USERS_API_BASE}/groups/${groupId}`, {
    method: 'PATCH',
    body,
  })
}

export async function deleteGroup(groupId: string): Promise<void> {
  await requestJson<Record<string, never>>(`${USERS_API_BASE}/groups/${groupId}`, {
    method: 'DELETE',
  })
}

export async function listUsers(query: ListUsersQuery = {}): Promise<PagedUsers> {
  const params = new URLSearchParams()
  if (query.search) {
    params.set('search', query.search)
  }
  if (query.page != null) {
    params.set('page', String(query.page))
  }
  if (query.page_size != null) {
    params.set('page_size', String(query.page_size))
  }
  const qs = params.toString()
  const url = qs ? `${USERS_API_BASE}/?${qs}` : `${USERS_API_BASE}/`
  return requestJson<PagedUsers>(url)
}

export async function getUser(userId: string): Promise<UserItem> {
  return requestJson<UserItem>(`${USERS_API_BASE}/${userId}`)
}

export async function createUser(body: CreateUserBody): Promise<UserItem> {
  return requestJson<UserItem>(`${USERS_API_BASE}/`, {
    method: 'POST',
    body,
  })
}

export async function patchUser(userId: string, body: PatchUserBody): Promise<UserItem> {
  return requestJson<UserItem>(`${USERS_API_BASE}/${userId}`, {
    method: 'PATCH',
    body,
  })
}

export async function deleteUser(userId: string): Promise<void> {
  await requestJson<Record<string, never>>(`${USERS_API_BASE}/${userId}`, {
    method: 'DELETE',
  })
}

export async function batchDeleteUsers(body: {
  user_ids: string[]
}): Promise<BatchDeleteUsersResult> {
  return requestJson<BatchDeleteUsersResult>(`${USERS_API_BASE}/batch/delete`, {
    method: 'POST',
    body,
  })
}

export async function listUserLogs(
  userId: string,
  query: { page?: number; page_size?: number } = {},
): Promise<PagedUserLogs> {
  const params = new URLSearchParams()
  if (query.page != null) {
    params.set('page', String(query.page))
  }
  if (query.page_size != null) {
    params.set('page_size', String(query.page_size))
  }
  const qs = params.toString()
  const url = qs
    ? `${USERS_API_BASE}/${userId}/logs?${qs}`
    : `${USERS_API_BASE}/${userId}/logs`
  return requestJson<PagedUserLogs>(url)
}
