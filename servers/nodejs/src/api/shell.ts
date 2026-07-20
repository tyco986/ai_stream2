/**
 * Shell API client (contract-aligned mock).
 * Paths match docs/shell_api.md: /{PROJECT_NAME}/backend/shell/...
 * Replace with OpenAPI-generated client when Django Shell API is ready.
 */

import { ApiError, type ApiEnvelope } from '@/shared/http/client'
import { BACKEND_PREFIX, STORAGE_KEY_PREFIX } from '@/shared/project'

export type ShellMode = 'sidebar' | 'topbar'

export type CurrentUser = {
  id: string
  username: string
}

export type PageSettings = {
  mode: ShellMode
}

const BASE = `${BACKEND_PREFIX}/shell`
const STORAGE_LOGGED_IN = `${STORAGE_KEY_PREFIX}shell.mockLoggedIn`
const STORAGE_MODE = `${STORAGE_KEY_PREFIX}shell.mode`
const DEFAULT_MODE: ShellMode = 'sidebar'

/** Reserved for real HTTP client when Django is ready. */
export const SHELL_API_BASE = BASE

const MOCK_USER: CurrentUser = {
  id: '550e8400-e29b-41d4-a716-446655440000',
  username: 'alice',
}

function isLoggedIn(): boolean {
  return sessionStorage.getItem(STORAGE_LOGGED_IN) !== '0'
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

function ok<T>(data: T): ApiEnvelope<T> {
  return { success: true, message: '', data }
}

/** Debug: set mock login. Default is logged in as alice. */
export function setMockLoggedIn(loggedIn: boolean) {
  sessionStorage.setItem(STORAGE_LOGGED_IN, loggedIn ? '1' : '0')
}

export function getMockLoggedIn(): boolean {
  return isLoggedIn()
}

export async function getMe(): Promise<CurrentUser> {
  if (!isLoggedIn()) {
    throw new ApiError(401, 'Unauthorized')
  }
  return ok(MOCK_USER).data
}

export async function getSettings(): Promise<PageSettings> {
  if (!isLoggedIn()) {
    throw new ApiError(401, 'Unauthorized')
  }
  return ok({ mode: readStoredMode() }).data
}

export async function putSettings(body: PageSettings): Promise<PageSettings> {
  if (!isLoggedIn()) {
    throw new ApiError(401, 'Unauthorized')
  }
  if (body.mode !== 'sidebar' && body.mode !== 'topbar') {
    throw new ApiError(400, 'Invalid mode')
  }
  writeStoredMode(body.mode)
  return ok({ mode: body.mode }).data
}

/** Apply mode in session when unauthenticated (no PUT). */
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
