/**
 * Servers API — real HTTP client.
 */

import { requestJson } from '@/shared/http/request'
import { BACKEND_PREFIX } from '@/shared/project'
import type {
  RefreshResponse,
  RestartResult,
  ServerList,
  ServerLogs,
} from '@/api/servers/types'

export const SERVERS_API_BASE = `${BACKEND_PREFIX}/servers`

export async function listServers(): Promise<ServerList> {
  return requestJson<ServerList>(`${SERVERS_API_BASE}/`)
}

export async function health(
  serverId: string,
): Promise<{ success: boolean; message: string }> {
  return requestJson<{ success: boolean; message: string }>(
    `${SERVERS_API_BASE}/${encodeURIComponent(serverId)}/health`,
  )
}

export async function refreshServers(): Promise<RefreshResponse> {
  return requestJson<RefreshResponse>(`${SERVERS_API_BASE}/refresh`, {
    method: 'POST',
    body: {},
  })
}

export async function restartServer(serverId: string): Promise<RestartResult> {
  return requestJson<RestartResult>(
    `${SERVERS_API_BASE}/${encodeURIComponent(serverId)}/restart`,
    {
      method: 'POST',
      body: {},
    },
  )
}

export async function getServerLogs(
  serverId: string,
  query: { tail?: number } = {},
): Promise<ServerLogs> {
  const params = new URLSearchParams()
  if (query.tail != null) {
    params.set('tail', String(query.tail))
  }
  const qs = params.toString()
  const base = `${SERVERS_API_BASE}/${encodeURIComponent(serverId)}/logs`
  const url = qs ? `${base}?${qs}` : base
  return requestJson<ServerLogs>(url)
}
