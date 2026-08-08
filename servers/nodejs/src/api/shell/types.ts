export type ShellMode = 'sidebar' | 'topbar'

export type SessionUser = {
  id: string
  username: string
  is_superuser: boolean
  permissions: string[]
}

/** @deprecated Prefer SessionUser */
export type CurrentUser = SessionUser

export type PageSettings = {
  mode: ShellMode
}
