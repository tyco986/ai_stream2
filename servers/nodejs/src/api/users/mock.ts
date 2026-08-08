/**
 * Users API — mock (sessionStorage).
 */

import { getSessionUserId } from '@/api/shell'
import { ApiError } from '@/shared/http/client'
import { BACKEND_PREFIX, STORAGE_KEY_PREFIX } from '@/shared/project'
import type {
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
  PermissionCatalog,
  UserItem,
  UserLogEntry,
} from '@/api/users/types'

export const USERS_API_BASE = `${BACKEND_PREFIX}/users`

const STORAGE_USERS = `${STORAGE_KEY_PREFIX}users.items`
const STORAGE_GROUPS = `${STORAGE_KEY_PREFIX}users.groups`
const STORAGE_LOGS = `${STORAGE_KEY_PREFIX}users.logs`
const STORAGE_PASSWORDS = `${STORAGE_KEY_PREFIX}users.passwords`

type Store = {
  users: UserItem[]
  groups: GroupItem[]
  logs: Record<string, UserLogEntry[]>
  passwords: Record<string, string>
}

function newId(): string {
  return crypto.randomUUID()
}

function buildCatalog(): PermissionCatalog {
  const modules: { key: string; label: string; actions: { key: string; codename: string }[] }[] =
    [
      {
        key: 'streams',
        label: 'Streams',
        actions: [
          { key: 'view', codename: 'streams.view_stream' },
          { key: 'add', codename: 'streams.add_stream' },
          { key: 'change', codename: 'streams.change_stream' },
          { key: 'delete', codename: 'streams.delete_stream' },
        ],
      },
      {
        key: 'previews',
        label: 'Previews',
        actions: [
          { key: 'view', codename: 'previews.view_preview' },
          { key: 'add', codename: 'previews.add_preview' },
          { key: 'change', codename: 'previews.change_preview' },
          { key: 'delete', codename: 'previews.delete_preview' },
        ],
      },
      {
        key: 'recordings',
        label: 'Recordings',
        actions: [
          { key: 'view', codename: 'recordings.view_recording' },
          { key: 'add', codename: 'recordings.add_recording' },
          { key: 'change', codename: 'recordings.change_recording' },
          { key: 'delete', codename: 'recordings.delete_recording' },
        ],
      },
      {
        key: 'models',
        label: 'Models',
        actions: [
          { key: 'view', codename: 'models.view_model' },
          { key: 'add', codename: 'models.add_model' },
          { key: 'change', codename: 'models.change_model' },
          { key: 'delete', codename: 'models.delete_model' },
        ],
      },
      {
        key: 'pipelines',
        label: 'Pipelines',
        actions: [
          { key: 'view', codename: 'pipelines.view_pipeline' },
          { key: 'add', codename: 'pipelines.add_pipeline' },
          { key: 'change', codename: 'pipelines.change_pipeline' },
          { key: 'delete', codename: 'pipelines.delete_pipeline' },
        ],
      },
      {
        key: 'servers',
        label: 'Servers',
        actions: [
          { key: 'view', codename: 'servers.view_server' },
          { key: 'change', codename: 'servers.change_server' },
        ],
      },
      {
        key: 'events',
        label: 'Events',
        actions: [
          { key: 'view', codename: 'events.view_event' },
          { key: 'change', codename: 'events.change_event' },
        ],
      },
      {
        key: 'users',
        label: 'Users',
        actions: [
          { key: 'view', codename: 'users.view_user' },
          { key: 'add', codename: 'users.add_user' },
          { key: 'change', codename: 'users.change_user' },
          { key: 'delete', codename: 'users.delete_user' },
        ],
      },
      {
        key: 'groups',
        label: 'Groups',
        actions: [
          { key: 'view', codename: 'users.view_group' },
          { key: 'add', codename: 'users.add_group' },
          { key: 'change', codename: 'users.change_group' },
          { key: 'delete', codename: 'users.delete_group' },
        ],
      },
    ]
  let id = 11
  return {
    modules: modules.map((mod) => ({
      key: mod.key,
      label: mod.label,
      actions: mod.actions.map((action) => {
        const permissionId = id
        id += 1
        return {
          key: action.key,
          label: action.key,
          permission_id: permissionId,
          codename: action.codename,
        }
      }),
    })),
  }
}

const CATALOG = buildCatalog()

function allPermissionIds(): number[] {
  return CATALOG.modules.flatMap((mod) => mod.actions.map((action) => action.permission_id))
}

function seedStore(): Store {
  const adminId = 'a1b2c3d4-e5f6-4789-a012-3456789abcde'
  const operatorId = 'b2c3d4e5-f6a7-4890-b123-456789abcdef'
  const viewerId = 'c3d4e5f6-a7b8-4901-c234-56789abcdef0'
  const aliceId = '550e8400-e29b-41d4-a716-446655440000'
  const bobId = '7c9e6679-7425-40de-944b-e07fc1f90ae7'
  const carolId = '6ba7b810-9dad-11d1-80b4-00c04fd430c8'
  const groups: GroupItem[] = [
    { id: adminId, name: 'admin', member_count: 0, permission_ids: allPermissionIds() },
    {
      id: operatorId,
      name: 'operator',
      member_count: 0,
      permission_ids: allPermissionIds().filter((_, index) => index % 2 === 0),
    },
    {
      id: viewerId,
      name: 'viewer',
      member_count: 0,
      permission_ids: CATALOG.modules.flatMap((mod) =>
        mod.actions.filter((action) => action.key === 'view').map((action) => action.permission_id),
      ),
    },
  ]
  const users: UserItem[] = [
    {
      id: aliceId,
      username: 'alice',
      is_active: true,
      is_superuser: true,
      must_change_password: true,
      groups: [{ id: adminId, name: 'admin' }],
      last_action_at: '2026-07-22T10:15:00Z',
      last_action_label: 'Edit',
    },
    {
      id: bobId,
      username: 'bob',
      is_active: true,
      is_superuser: false,
      must_change_password: false,
      groups: [{ id: viewerId, name: 'viewer' }],
      last_action_at: '2026-07-21T18:02:00Z',
      last_action_label: 'Login',
    },
    {
      id: carolId,
      username: 'carol',
      is_active: false,
      is_superuser: false,
      must_change_password: false,
      groups: [],
      last_action_at: null,
      last_action_label: null,
    },
  ]
  const logs: Record<string, UserLogEntry[]> = {
    [aliceId]: [
      {
        id: newId(),
        at: '2026-07-22T10:15:00Z',
        label: 'Edit',
        detail: 'Edit user',
      },
      {
        id: newId(),
        at: '2026-07-22T09:01:00Z',
        label: 'Login',
        detail: 'Login',
      },
      {
        id: newId(),
        at: '2026-07-21T18:02:00Z',
        label: 'Export',
        detail: 'Export site config',
      },
    ],
    [bobId]: [
      {
        id: newId(),
        at: '2026-07-21T18:02:00Z',
        label: 'Login',
        detail: 'Login',
      },
    ],
    [carolId]: [],
  }
  const passwords: Record<string, string> = {
    alice: 'alice',
    bob: 'bob',
    carol: 'carol',
  }
  return { users, groups, logs, passwords }
}

function readStore(): Store {
  const rawUsers = sessionStorage.getItem(STORAGE_USERS)
  const rawGroups = sessionStorage.getItem(STORAGE_GROUPS)
  const rawLogs = sessionStorage.getItem(STORAGE_LOGS)
  const rawPasswords = sessionStorage.getItem(STORAGE_PASSWORDS)
  if (!rawUsers || !rawGroups || !rawLogs) {
    const seeded = seedStore()
    writeStore(seeded)
    return seeded
  }
  const passwords = rawPasswords
    ? (JSON.parse(rawPasswords) as Record<string, string>)
    : { alice: 'alice', bob: 'bob', carol: 'carol' }
  const store: Store = {
    users: JSON.parse(rawUsers) as UserItem[],
    groups: JSON.parse(rawGroups) as GroupItem[],
    logs: JSON.parse(rawLogs) as Record<string, UserLogEntry[]>,
    passwords,
  }
  if (!rawPasswords) {
    writeStore(store)
  }
  return store
}

function writeStore(store: Store) {
  sessionStorage.setItem(STORAGE_USERS, JSON.stringify(store.users))
  sessionStorage.setItem(STORAGE_GROUPS, JSON.stringify(store.groups))
  sessionStorage.setItem(STORAGE_LOGS, JSON.stringify(store.logs))
  sessionStorage.setItem(STORAGE_PASSWORDS, JSON.stringify(store.passwords))
}

export function authenticateUser(username: string, password: string): UserItem {
  const store = readStore()
  const user = store.users.find((item) => item.username === username)
  if (!user || store.passwords[username] !== password) {
    throw new ApiError(401, 'Invalid username or password')
  }
  if (!user.is_active) {
    throw new ApiError(401, 'Account is inactive')
  }
  return {
    ...user,
    groups: user.groups.map((group) => ({ ...group })),
  }
}

export function findUserByUsername(username: string): UserItem | null {
  const store = readStore()
  const user = store.users.find((item) => item.username === username)
  let result: UserItem | null = null
  if (user) {
    result = {
      ...user,
      groups: user.groups.map((group) => ({ ...group })),
    }
  }
  return result
}

export function isJwtLikePassword(value: string): boolean {
  const parts = value.split('.')
  return parts.length === 3 && Boolean(parts[0] && parts[1] && parts[2])
}

export function setUserPassword(username: string, password: string) {
  const store = readStore()
  store.passwords[username] = password
  writeStore(store)
}

export function setUserMustChangePassword(username: string, value: boolean) {
  const store = readStore()
  const user = store.users.find((item) => item.username === username)
  if (user) {
    user.must_change_password = value
    writeStore(store)
  }
}

function resolveGroups(groupIds: string[], groups: GroupItem[]): GroupRef[] {
  return groupIds
    .map((id) => groups.find((group) => group.id === id))
    .filter((group): group is GroupItem => Boolean(group))
    .map((group) => ({ id: group.id, name: group.name }))
}

function appendLog(store: Store, userId: string, label: string, detail: string) {
  const at = new Date().toISOString()
  const entry: UserLogEntry = { id: newId(), at, label, detail }
  const list = store.logs[userId] ?? []
  store.logs[userId] = [entry, ...list]
  const user = store.users.find((item) => item.id === userId)
  if (user) {
    user.last_action_at = at
    user.last_action_label = label
  }
}

function memberCount(store: Store, groupId: string): number {
  return store.users.filter((user) => user.groups.some((group) => group.id === groupId)).length
}

function serializeGroup(store: Store, group: GroupItem): GroupItem {
  return {
    ...group,
    member_count: memberCount(store, group.id),
    permission_ids: [...group.permission_ids],
  }
}

export async function getPermissionCatalog(): Promise<PermissionCatalog> {
  return CATALOG
}

export async function listGroups(): Promise<{ items: GroupItem[] }> {
  const store = readStore()
  return { items: store.groups.map((group) => serializeGroup(store, group)) }
}

export async function getGroup(groupId: string): Promise<GroupItem> {
  const store = readStore()
  const group = store.groups.find((item) => item.id === groupId)
  if (!group) {
    throw new ApiError(404, 'Group not found')
  }
  return serializeGroup(store, group)
}

export async function createGroup(body: CreateGroupBody): Promise<GroupItem> {
  const name = body.name.trim()
  if (!name) {
    throw new ApiError(400, 'name required')
  }
  const store = readStore()
  if (store.groups.some((group) => group.name === name)) {
    throw new ApiError(400, 'Group name already exists')
  }
  const group: GroupItem = {
    id: newId(),
    name,
    member_count: 0,
    permission_ids: body.permission_ids ? [...body.permission_ids] : [],
  }
  store.groups.push(group)
  writeStore(store)
  return serializeGroup(store, group)
}

export async function patchGroup(groupId: string, body: PatchGroupBody): Promise<GroupItem> {
  const store = readStore()
  const group = store.groups.find((item) => item.id === groupId)
  if (!group) {
    throw new ApiError(404, 'Group not found')
  }
  if (body.name !== undefined) {
    const name = body.name.trim()
    if (!name) {
      throw new ApiError(400, 'name required')
    }
    if (store.groups.some((item) => item.id !== groupId && item.name === name)) {
      throw new ApiError(400, 'Group name already exists')
    }
    group.name = name
    for (const user of store.users) {
      for (const ref of user.groups) {
        if (ref.id === groupId) {
          ref.name = name
        }
      }
    }
  }
  if (body.permission_ids !== undefined) {
    group.permission_ids = [...body.permission_ids]
  }
  writeStore(store)
  return serializeGroup(store, group)
}

export async function deleteGroup(groupId: string): Promise<void> {
  const store = readStore()
  const index = store.groups.findIndex((item) => item.id === groupId)
  if (index < 0) {
    throw new ApiError(404, 'Group not found')
  }
  store.groups.splice(index, 1)
  for (const user of store.users) {
    user.groups = user.groups.filter((ref) => ref.id !== groupId)
  }
  writeStore(store)
}

export async function listUsers(query: ListUsersQuery = {}): Promise<PagedUsers> {
  const page = query.page ?? 1
  const pageSize = query.page_size ?? 20
  const search = (query.search ?? '').trim().toLowerCase()
  let items = [...readStore().users]
  if (search) {
    items = items.filter((user) => user.username.toLowerCase().includes(search))
  }
  items.sort((a, b) => a.username.localeCompare(b.username))
  const total = items.length
  const start = (page - 1) * pageSize
  return {
    items: items.slice(start, start + pageSize).map((user) => ({
      ...user,
      groups: user.groups.map((group) => ({ ...group })),
    })),
    total,
    page,
    page_size: pageSize,
  }
}

export async function getUser(userId: string): Promise<UserItem> {
  const user = readStore().users.find((item) => item.id === userId)
  if (!user) {
    throw new ApiError(404, 'User not found')
  }
  return {
    ...user,
    groups: user.groups.map((group) => ({ ...group })),
  }
}

export async function createUser(body: CreateUserBody): Promise<UserItem> {
  const username = body.username.trim()
  if (!username) {
    throw new ApiError(400, 'username required')
  }
  if (!body.password) {
    throw new ApiError(400, 'password required')
  }
  if (!body.group_ids?.length) {
    throw new ApiError(400, 'group_ids is required')
  }
  const store = readStore()
  if (store.users.some((user) => user.username === username)) {
    throw new ApiError(400, 'Username already exists')
  }
  const groupIds = body.group_ids
  const user: UserItem = {
    id: newId(),
    username,
    is_active: body.is_active ?? true,
    is_superuser: false,
    must_change_password: false,
    groups: resolveGroups(groupIds, store.groups),
    last_action_at: null,
    last_action_label: null,
  }
  store.users.push(user)
  store.passwords[username] = body.password
  store.logs[user.id] = []
  appendLog(store, user.id, 'Create', 'Create user')
  writeStore(store)
  return getUser(user.id)
}

export async function patchUser(userId: string, body: PatchUserBody): Promise<UserItem> {
  const store = readStore()
  const user = store.users.find((item) => item.id === userId)
  if (!user) {
    throw new ApiError(404, 'User not found')
  }
  const oldUsername = user.username
  if (body.username !== undefined) {
    const username = body.username.trim()
    if (!username) {
      throw new ApiError(400, 'username required')
    }
    if (store.users.some((item) => item.id !== userId && item.username === username)) {
      throw new ApiError(400, 'Username already exists')
    }
    user.username = username
    if (username !== oldUsername) {
      store.passwords[username] = store.passwords[oldUsername] ?? ''
      delete store.passwords[oldUsername]
    }
  }
  if (body.password !== undefined && body.password) {
    store.passwords[user.username] = body.password
  }
  if (body.is_active !== undefined) {
    user.is_active = body.is_active
  }
  if (body.group_ids !== undefined) {
    if (!body.group_ids.length) {
      throw new ApiError(400, 'group_ids is required')
    }
    user.groups = resolveGroups(body.group_ids, store.groups)
  }
  appendLog(store, userId, 'Edit', 'Edit user')
  writeStore(store)
  return getUser(userId)
}

export async function deleteUser(userId: string): Promise<void> {
  const meId = getSessionUserId()
  if (userId === meId) {
    throw new ApiError(400, 'Cannot delete the current user')
  }
  const store = readStore()
  const index = store.users.findIndex((item) => item.id === userId)
  if (index < 0) {
    throw new ApiError(404, 'User not found')
  }
  const target = store.users[index]
  if (
    target.is_superuser &&
    store.users.filter((item) => item.is_superuser).length <= 1
  ) {
    throw new ApiError(400, 'Cannot delete the last superuser')
  }
  const username = target.username
  store.users.splice(index, 1)
  delete store.logs[userId]
  delete store.passwords[username]
  writeStore(store)
}

export async function batchDeleteUsers(body: {
  user_ids: string[]
}): Promise<BatchDeleteUsersResult> {
  const meId = getSessionUserId()
  const store = readStore()
  let deletedCount = 0
  let skippedCount = 0
  const keep = new Set(body.user_ids)
  for (const id of body.user_ids) {
    if (id === meId) {
      skippedCount += 1
      keep.delete(id)
    }
  }
  store.users = store.users.filter((user) => {
    if (!keep.has(user.id)) {
      return true
    }
    if (
      user.is_superuser &&
      store.users.filter((item) => item.is_superuser).length <= 1
    ) {
      skippedCount += 1
      return true
    }
    deletedCount += 1
    delete store.logs[user.id]
    delete store.passwords[user.username]
    return false
  })
  writeStore(store)
  return { deleted_count: deletedCount, skipped_count: skippedCount }
}

export async function listUserLogs(
  userId: string,
  query: { page?: number; page_size?: number } = {},
): Promise<PagedUserLogs> {
  const store = readStore()
  if (!store.users.some((user) => user.id === userId)) {
    throw new ApiError(404, 'User not found')
  }
  const page = query.page ?? 1
  const pageSize = query.page_size ?? 50
  const items = store.logs[userId] ?? []
  const start = (page - 1) * pageSize
  return {
    items: items.slice(start, start + pageSize).map((entry) => ({ ...entry })),
    total: items.length,
    page,
    page_size: pageSize,
  }
}

export { formatLastAction, formatLogAt } from '@/api/users/utils'
