/**
 * Shell API client. Implementation selected by Vite mode (real → http, mock → mock).
 */

import * as api from './runtime'

export type {
  CurrentUser,
  PageSettings,
  SessionUser,
  ShellMode,
} from '@/api/shell/types'

export const SHELL_API_BASE = api.SHELL_API_BASE
export const isLoggedIn = api.isLoggedIn
export const getSessionUserId = api.getSessionUserId
export const establishSession = api.establishSession
export const clearSession = api.clearSession
export const setMockLoggedIn = api.setMockLoggedIn
export const getMockLoggedIn = api.getMockLoggedIn
export const getMe = api.getMe
export const logout = api.logout
export const getSettings = api.getSettings
export const putSettings = api.putSettings
export const applyGuestMode = api.applyGuestMode
export const readGuestMode = api.readGuestMode
