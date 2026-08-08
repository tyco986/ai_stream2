export type ServerStatus = 'online' | 'offline'

export type ServerCategory = 'infrastructure' | 'pipeline'

export type Server = {
  id: string
  name: string
  status: ServerStatus
  last_refresh_at: string | null
  category: ServerCategory
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
