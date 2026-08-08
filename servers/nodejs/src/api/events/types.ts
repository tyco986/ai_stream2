export type EventStatus = 'new' | 'acked'

export type EventItem = {
  id: string
  occurred_at: string
  stream_id: string
  stream_name: string
  pipeline_id: string
  pipeline_name: string
  event: string
  status: EventStatus
}

export type EventDetail = EventItem & {
  raw_url: string | null
  visualization_url: string | null
  prev_id: string | null
  next_id: string | null
  payload: Record<string, unknown> | null
}

export type EventOptionItem = {
  value: string
  label: string
}

export type EventOptionGroup = {
  pipeline_id: string
  pipeline_name: string
  items: EventOptionItem[]
}

export type EventListQuery = {
  from?: string
  to?: string
  stream_id?: string
  pipeline_id?: string
  event?: string
  status?: EventStatus | ''
  search?: string
  page?: number
  page_size?: number
}

export type PagedEvents = {
  items: EventItem[]
  total: number
  page: number
  page_size: number
}

export type BatchAckResult = {
  updated_count: number
  acked_count: number
  unacked_count: number
}

export type FilterOption = {
  id: string
  name: string
}

