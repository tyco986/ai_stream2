export type GroupRef = {
  id: string
  name: string
}

export type UserItem = {
  id: string
  username: string
  is_active: boolean
  is_superuser: boolean
  must_change_password?: boolean
  groups: GroupRef[]
  last_action_at: string | null
  last_action_label: string | null
}

export type GroupItem = {
  id: string
  name: string
  member_count: number
  permission_ids: number[]
}

export type PermissionAction = {
  key: string
  label: string
  permission_id: number
  codename: string
}

export type PermissionModule = {
  key: string
  label: string
  actions: PermissionAction[]
}

export type PermissionCatalog = {
  modules: PermissionModule[]
}

export type UserLogEntry = {
  id: string
  at: string
  label: string
  detail: string
}

export type ListUsersQuery = {
  search?: string
  page?: number
  page_size?: number
}

export type PagedUsers = {
  items: UserItem[]
  total: number
  page: number
  page_size: number
}

export type PagedUserLogs = {
  items: UserLogEntry[]
  total: number
  page: number
  page_size: number
}

export type CreateUserBody = {
  username: string
  password: string
  is_active?: boolean
  group_ids: string[]
}

export type PatchUserBody = {
  username?: string
  password?: string
  is_active?: boolean
  group_ids?: string[]
}

export type CreateGroupBody = {
  name: string
  permission_ids?: number[]
}

export type PatchGroupBody = {
  name?: string
  permission_ids?: number[]
}

export type BatchDeleteUsersResult = {
  deleted_count: number
  skipped_count: number
}
