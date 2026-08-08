/**
 * Shell API — mock implementation (sessionStorage).
 */

import { ApiError, type ApiEnvelope } from '@/shared/http/client'
import { BACKEND_PREFIX, STORAGE_KEY_PREFIX } from '@/shared/project'
import type { PageSettings, SessionUser, ShellMode } from '@/api/shell/types'

const BASE = `${BACKEND_PREFIX}/shell`
const STORAGE_LOGGED_IN = `${STORAGE_KEY_PREFIX}shell.mockLoggedIn`
const STORAGE_SESSION_USER = `${STORAGE_KEY_PREFIX}shell.sessionUser`
const STORAGE_MODE = `${STORAGE_KEY_PREFIX}shell.mode`
const DEFAULT_MODE: ShellMode = 'sidebar'

export const SHELL_API_BASE = BASE

const FALLBACK_SESSION_USER: SessionUser = {
  id: '550e8400-e29b-41d4-a716-446655440000',
  username: 'alice',
  is_superuser: true,
  permissions: ['*'],
}

function ok<T>(data: T): ApiEnvelope<T> {
  return { success: true, message: '', data }
}

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

export function isLoggedIn(): boolean {
  return sessionStorage.getItem(STORAGE_LOGGED_IN) === '1'
}

export function getSessionUserId(): string | null {
  let result: string | null = null
  if (isLoggedIn()) {
    const raw = sessionStorage.getItem(STORAGE_SESSION_USER)
    if (raw) {
      const user = JSON.parse(raw) as SessionUser
      result = user.id
    }
  }
  return result
}

export function establishSession(user: SessionUser) {
  sessionStorage.setItem(STORAGE_LOGGED_IN, '1')
  sessionStorage.setItem(STORAGE_SESSION_USER, JSON.stringify(user))
}

export function clearSession() {
  sessionStorage.setItem(STORAGE_LOGGED_IN, '0')
  sessionStorage.removeItem(STORAGE_SESSION_USER)
}

export function setMockLoggedIn(loggedIn: boolean) {
  if (loggedIn) {
    establishSession(FALLBACK_SESSION_USER)
    return
  }
  clearSession()
}

export function getMockLoggedIn(): boolean {
  return isLoggedIn()
}

function readSessionUser(): SessionUser | null {
  let result: SessionUser | null = null
  if (isLoggedIn()) {
    const raw = sessionStorage.getItem(STORAGE_SESSION_USER)
    if (raw) {
      result = JSON.parse(raw) as SessionUser
    }
  }
  return result
}

export async function getMe(): Promise<SessionUser> {
  const user = readSessionUser()
  if (!user) {
    throw new ApiError(401, 'Unauthorized')
  }
  return ok(user).data
}

export async function logout(): Promise<Record<string, never>> {
  clearSession()
  return ok({}).data
}

export async function getSettings(): Promise<PageSettings> {
  await getMe()
  return ok({ mode: readStoredMode() }).data
}

export async function putSettings(body: PageSettings): Promise<PageSettings> {
  await getMe()
  if (body.mode !== 'sidebar' && body.mode !== 'topbar') {
    throw new ApiError(400, 'Invalid mode')
  }
  writeStoredMode(body.mode)
  return ok({ mode: body.mode }).data
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
