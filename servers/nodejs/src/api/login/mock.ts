/**
 * Login API — mock (local users store + shell sessionStorage).
 */

import { establishSession, type SessionUser } from '@/api/shell'
import {
  authenticateUser,
  findUserByUsername,
  isJwtLikePassword,
  setUserMustChangePassword,
  setUserPassword,
} from '@/api/users/mock'
import { ApiError } from '@/shared/http/client'
import { BACKEND_PREFIX } from '@/shared/project'
import type { LoginBody, LoginResult } from '@/api/login/types'

export type { LoginBody, LoginResult } from '@/api/login/types'

export const LOGIN_API_BASE = `${BACKEND_PREFIX}/login`

function toSessionUser(user: {
  id: string
  username: string
  is_superuser: boolean
}): SessionUser {
  const session: SessionUser = {
    id: user.id,
    username: user.username,
    is_superuser: user.is_superuser,
    permissions: user.is_superuser ? ['*'] : [],
  }
  if (!user.is_superuser) {
    session.permissions = [
      'users.view_user',
      'users.view_group',
      'streams.view_stream',
      'events.view_event',
    ]
  }
  return session
}

export async function login(body: LoginBody): Promise<LoginResult> {
  const username = body.username?.trim() ?? ''
  const password = body.password ?? ''
  const newPassword = body.new_password
  if (!username) {
    throw new ApiError(400, 'Username is required')
  }
  if (!password) {
    throw new ApiError(400, 'Password is required')
  }
  let result: LoginResult = {}
  if (isJwtLikePassword(password)) {
    const user = findUserByUsername(username)
    if (!user || !user.is_active || !user.is_superuser) {
      throw new ApiError(401, 'Invalid username or password')
    }
    if (newPassword) {
      const cleaned = newPassword.trim()
      if (!cleaned) {
        throw new ApiError(400, 'new_password is required')
      }
      setUserPassword(username, cleaned)
      setUserMustChangePassword(username, false)
    }
    establishSession(toSessionUser(user))
  } else {
    const user = authenticateUser(username, password)
    if (user.must_change_password && !newPassword) {
      result = { must_change_password: true }
    } else {
      if (user.must_change_password) {
        const cleaned = (newPassword ?? '').trim()
        if (!cleaned) {
          throw new ApiError(400, 'new_password is required')
        }
        if (cleaned === password) {
          throw new ApiError(400, 'new_password must differ from password')
        }
        setUserPassword(username, cleaned)
        setUserMustChangePassword(username, false)
      }
      establishSession(toSessionUser(user))
    }
  }
  return result
}
