/**
 * Servers API client. Implementation selected by Vite mode (real → http, mock → mock).
 */

import * as api from './runtime'

export type {
  RefreshResponse,
  RefreshResult,
  RestartResult,
  Server,
  ServerCategory,
  ServerList,
  ServerLogs,
  ServerStatus,
} from '@/api/servers/types'

export const SERVERS_API_BASE = api.SERVERS_API_BASE
export const listServers = api.listServers
export const health = api.health
export const refreshServers = api.refreshServers
export const restartServer = api.restartServer
export const getServerLogs = api.getServerLogs
