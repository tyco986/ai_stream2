/**
 * Servers API client (contract-aligned mock).
 * Paths: /{PROJECT_NAME}/backend/servers/...
 * Replace with OpenAPI-generated client when ready.
 */

import { ApiError } from '@/shared/http/client'
import { BACKEND_PREFIX, STORAGE_KEY_PREFIX } from '@/shared/project'

export type ServerStatus = 'online' | 'offline'

export type Server = {
  id: string
  name: string
  status: ServerStatus
  last_refresh_at: string | null
}

export type RefreshResult = {
  id: string
  status: ServerStatus
  last_refresh_at: string
  latency_ms: number | null
  detail: string
}

export type RestartResult = {
  id: string
  container_name: string
  restarted_at: string
  refresh?: RefreshResult
}

export type ServerLogs = {
  content: string
}

export type ServerList = {
  items: Server[]
}

export type RefreshResponse = {
  results: RefreshResult[]
}

type RegistryEntry = {
  id: string
  container_name: string
  sort_order: number
  default_online: boolean
}

export const SERVERS_API_BASE = `${BACKEND_PREFIX}/servers`

const STORAGE_SERVERS = `${STORAGE_KEY_PREFIX}servers.items`
const STORAGE_LOGS = `${STORAGE_KEY_PREFIX}servers.logs`

const REGISTRY: RegistryEntry[] = [
  { id: 'deepstream', container_name: 'ai_stream2_deepstream', sort_order: 1, default_online: true },
  { id: 'dsyolo', container_name: 'DeepStream-Yolo', sort_order: 2, default_online: true },
  { id: 'export_trt', container_name: 'ai_stream2_export_trt', sort_order: 3, default_online: true },
  { id: 'ffmpeg', container_name: 'ai_stream2_ffmpeg', sort_order: 4, default_online: true },
  { id: 'generator', container_name: 'ai_stream2_generator', sort_order: 5, default_online: false },
  { id: 'kafka', container_name: 'ai_stream2_kafka', sort_order: 6, default_online: true },
  { id: 'mediamtx', container_name: 'ai_stream2_mediamtx', sort_order: 7, default_online: true },
  { id: 'nodejs', container_name: 'ai_stream2_nodejs', sort_order: 8, default_online: false },
]

type Store = {
  servers: Server[]
  logs: Record<string, string>
}

function seedStore(): Store {
  const servers = REGISTRY.map((entry) => ({
    id: entry.id,
    name: entry.id,
    status: (entry.default_online ? 'online' : 'offline') as ServerStatus,
    last_refresh_at: entry.default_online ? '2026-06-30T14:22:00Z' : null,
  }))
  const logs = Object.fromEntries(
    REGISTRY.map((entry) => [
      entry.id,
      `2026-06-30 14:22:00 INFO ${entry.id} service started\n2026-06-30 14:22:01 INFO listening\n`,
    ]),
  )
  return { servers, logs }
}

function readStore(): Store {
  const raw = sessionStorage.getItem(STORAGE_SERVERS)
  const logsRaw = sessionStorage.getItem(STORAGE_LOGS)
  if (!raw) {
    const seed = seedStore()
    writeStore(seed)
    return seed
  }
  return {
    servers: JSON.parse(raw) as Server[],
    logs: logsRaw ? (JSON.parse(logsRaw) as Record<string, string>) : {},
  }
}

function writeStore(store: Store) {
  sessionStorage.setItem(STORAGE_SERVERS, JSON.stringify(store.servers))
  sessionStorage.setItem(STORAGE_LOGS, JSON.stringify(store.logs))
}

function findRegistry(serverId: string): RegistryEntry {
  const entry = REGISTRY.find((item) => item.id === serverId)
  if (!entry) {
    throw new ApiError(404, 'Server not found')
  }
  return entry
}

function findServer(store: Store, serverId: string): Server {
  findRegistry(serverId)
  const server = store.servers.find((item) => item.id === serverId)
  if (!server) {
    throw new ApiError(404, 'Server not found')
  }
  return server
}

function sortedServers(servers: Server[]): Server[] {
  const order = new Map(REGISTRY.map((entry) => [entry.id, entry.sort_order]))
  return [...servers].sort(
    (a, b) => (order.get(a.id) ?? 999) - (order.get(b.id) ?? 999),
  )
}

function probeOne(store: Store, serverId: string): RefreshResult {
  const entry = findRegistry(serverId)
  const server = findServer(store, serverId)
  const now = new Date().toISOString()
  const online = entry.default_online
  server.status = online ? 'online' : 'offline'
  server.last_refresh_at = now
  return {
    id: serverId,
    status: server.status,
    last_refresh_at: now,
    latency_ms: online ? 20 + Math.floor(Math.random() * 80) : null,
    detail: online ? '' : 'connection refused',
  }
}

export async function listServers(): Promise<ServerList> {
  const store = readStore()
  return { items: sortedServers(store.servers) }
}

export async function helloWorld(
  serverId: string,
): Promise<{ success: boolean; message: string }> {
  const store = readStore()
  const result = probeOne(store, serverId)
  writeStore(store)
  if (result.status !== 'online') {
    throw new ApiError(502, result.detail || 'Upstream unreachable')
  }
  return { success: true, message: '' }
}

export async function refreshServers(): Promise<RefreshResponse> {
  const store = readStore()
  await new Promise((resolve) => setTimeout(resolve, 400))
  const results = REGISTRY.map((entry) => probeOne(store, entry.id))
  writeStore(store)
  return { results }
}

export async function restartServer(serverId: string): Promise<RestartResult> {
  const store = readStore()
  const entry = findRegistry(serverId)
  findServer(store, serverId)
  await new Promise((resolve) => setTimeout(resolve, 600))
  const restartedAt = new Date().toISOString()
  const log = store.logs[serverId] ?? ''
  store.logs[serverId] = `${log}${restartedAt} INFO container restarted\n`
  const refresh = probeOne(store, serverId)
  writeStore(store)
  return {
    id: serverId,
    container_name: entry.container_name,
    restarted_at: restartedAt,
    refresh,
  }
}

export async function getServerLogs(
  serverId: string,
  query: { tail?: number } = {},
): Promise<ServerLogs> {
  const store = readStore()
  findServer(store, serverId)
  let content = store.logs[serverId] ?? ''
  const tail = query.tail ?? 500
  if (tail > 0) {
    const lines = content.split('\n')
    content = lines.slice(-tail).join('\n')
  }
  return { content }
}
