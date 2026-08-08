export type StreamStatus = 'online' | 'offline'

export type TreeStreamNode = {
  id: string
  name: string
  type: 'stream'
  enabled: boolean
  status: StreamStatus
  recording: boolean
}

export type TreeGroupNode = {
  id: string
  name: string
  type: 'group'
  children: TreeNode[]
}

export type TreeNode = TreeGroupNode | TreeStreamNode

export type GroupsTree = {
  root: TreeGroupNode
}

export type Group = {
  id: string
  name: string
  parent_id: string | null
}

export type Stream = {
  id: string
  name: string
  group_id: string
  group_name: string
  url: string
  resolution: string | null
  fps: number | null
  status: StreamStatus
  enabled: boolean
  recording: boolean
  last_probe_at: string | null
}

export type StreamSummary = {
  id: string
  name: string
  group_id: string
  group_name: string
}

export type ProbeResult = {
  id: string
  success: boolean
  resolution?: string | null
  fps?: number | null
  status?: StreamStatus
  last_probe_at?: string | null
  enabled?: boolean
  recording?: boolean
  error?: string
}

export type PublishResult = {
  name: string
  url: string
}

export type ListStreamsQuery = {
  group_id?: string
  stream_id?: string
  search?: string
  page?: number
  page_size?: number
}

export type PagedStreams = {
  items: Stream[]
  total: number
  page: number
  page_size: number
}

export const ALL_GROUP_ID = '00000000-0000-4000-8000-000000000001'

