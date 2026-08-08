/**
 * Login API client. Implementation selected by Vite mode (real → http, mock → mock).
 */

import * as api from './runtime'

export type { LoginBody, LoginResult } from '@/api/login/types'

export const LOGIN_API_BASE = api.LOGIN_API_BASE
export const login = api.login
