/**
 * Shell API — real HTTP client (Django session cookie).
 */

import { ApiError } from '@/shared/http/client'
import { requestJson } from '@/shared/http/request'
import { BACKEND_PREFIX, STORAGE_KEY_PREFIX } from '@/shared/project'
import type { PageSettings, SessionUser, ShellMode } from '@/api/shell/types'

const BASE = `${BACKEND_PREFIX}/shell`
const STORAGE_MODE = `${STORAGE_KEY_PREFIX}shell.mode`
const DEFAULT_MODE: ShellMode = 'sidebar'

export const SHELL_API_BASE = BASE

function readStoredMode(): ShellMode {
  const value = sessionStorage.getItem(STORAGE_MODE)
  if (value === 'sidebar' || value === 'topbar') {
    return value
  }
  return DEFAULT_MODE
}

function writeStoredMode(mode: ShellMode) {
  sessionStorage.setItem(STORAGE_MODE, mode)
}

/** Real mode: session is cookie-backed; local flag is best-effort cache. */
export function isLoggedIn(): boolean {
  return sessionStorage.getItem(`${STORAGE_KEY_PREFIX}shell.httpLoggedIn`) === '1'
}

export function getSessionUserId(): string | null {
  return sessionStorage.getItem(`${STORAGE_KEY_PREFIX}shell.httpUserId`)
}

export function establishSession(user: SessionUser) {
  sessionStorage.setItem(`${STORAGE_KEY_PREFIX}shell.httpLoggedIn`, '1')
  sessionStorage.setItem(`${STORAGE_KEY_PREFIX}shell.httpUserId`, user.id)
}

export function clearSession() {
  sessionStorage.removeItem(`${STORAGE_KEY_PREFIX}shell.httpLoggedIn`)
  sessionStorage.removeItem(`${STORAGE_KEY_PREFIX}shell.httpUserId`)
}

export function setMockLoggedIn(loggedIn: boolean) {
  void loggedIn
  throw new ApiError(400, 'setMockLoggedIn is mock-only')
}

export function getMockLoggedIn(): boolean {
  return isLoggedIn()
}

export async function getMe(): Promise<SessionUser> {
  const user = await requestJson<SessionUser>(`${BASE}/me`)
  establishSession(user)
  return user
}

export async function logout(): Promise<Record<string, never>> {
  const data = await requestJson<Record<string, never>>(`${BASE}/logout`, {
    method: 'POST',
    body: {},
  })
  clearSession()
  return data
}

export async function getSettings(): Promise<PageSettings> {
  return requestJson<PageSettings>(`${BASE}/settings`)
}

export async function putSettings(body: PageSettings): Promise<PageSettings> {
  if (body.mode !== 'sidebar' && body.mode !== 'topbar') {
    throw new ApiError(400, 'Invalid mode')
  }
  return requestJson<PageSettings>(`${BASE}/settings`, {
    method: 'PUT',
    body,
  })
}

export function applyGuestMode(mode: ShellMode): PageSettings {
  if (mode !== 'sidebar' && mode !== 'topbar') {
    throw new ApiError(400, 'Invalid mode')
  }
  writeStoredMode(mode)
  return { mode }
}

export function readGuestMode(): ShellMode {
  return readStoredMode()
}
