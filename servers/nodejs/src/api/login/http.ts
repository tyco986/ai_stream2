/**
 * Login API — real HTTP (Django session).
 */

import { ApiError } from '@/shared/http/client'
import { requestJson } from '@/shared/http/request'
import { BACKEND_PREFIX } from '@/shared/project'
import type { LoginBody, LoginResult } from '@/api/login/types'

export const LOGIN_API_BASE = `${BACKEND_PREFIX}/login`

export async function login(body: LoginBody): Promise<LoginResult> {
  const username = body.username?.trim() ?? ''
  const password = body.password ?? ''
  if (!username) {
    throw new ApiError(400, 'Username is required')
  }
  if (!password) {
    throw new ApiError(400, 'Password is required')
  }
  const payload: Record<string, string> = { username, password }
  if (body.new_password) {
    payload.new_password = body.new_password
  }
  return requestJson<LoginResult>(`${LOGIN_API_BASE}/`, {
    method: 'POST',
    body: payload,
  })
}
