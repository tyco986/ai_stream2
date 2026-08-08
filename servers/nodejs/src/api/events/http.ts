/**
 * Events API — real HTTP client.
 */

import { requestBlob, requestJson } from '@/shared/http/request'
import { BACKEND_PREFIX } from '@/shared/project'
import type {
  BatchAckResult,
  EventDetail,
  EventItem,
  EventListQuery,
  EventOptionGroup,
  EventStatus,
  FilterOption,
  PagedEvents,
} from '@/api/events/types'

export const EVENTS_API_BASE = `${BACKEND_PREFIX}/events`

function appendListQuery(params: URLSearchParams, query: EventListQuery) {
  if (query.from) {
    params.set('from', query.from)
  }
  if (query.to) {
    params.set('to', query.to)
  }
  if (query.stream_id) {
    params.set('stream_id', query.stream_id)
  }
  if (query.pipeline_id) {
    params.set('pipeline_id', query.pipeline_id)
  }
  if (query.event) {
    params.set('event', query.event)
  }
  if (query.status) {
    params.set('status', query.status)
  }
  if (query.search) {
    params.set('search', query.search)
  }
  if (query.page != null) {
    params.set('page', String(query.page))
  }
  if (query.page_size != null) {
    params.set('page_size', String(query.page_size))
  }
}

function downloadBlob(filename: string, blob: Blob) {
  const url = URL.createObjectURL(blob)
  const anchor = document.createElement('a')
  anchor.href = url
  anchor.download = filename
  anchor.click()
  URL.revokeObjectURL(url)
}

export async function listEvents(query: EventListQuery = {}): Promise<PagedEvents> {
  const params = new URLSearchParams()
  appendListQuery(params, query)
  const qs = params.toString()
  const url = qs ? `${EVENTS_API_BASE}/?${qs}` : `${EVENTS_API_BASE}/`
  return requestJson<PagedEvents>(url)
}

export async function getEvent(
  eventId: string,
  query: EventListQuery = {},
): Promise<EventDetail> {
  const params = new URLSearchParams()
  appendListQuery(params, query)
  const qs = params.toString()
  const url = qs
    ? `${EVENTS_API_BASE}/${eventId}?${qs}`
    : `${EVENTS_API_BASE}/${eventId}`
  return requestJson<EventDetail>(url)
}

export async function listEventOptions(query: {
  pipeline_id?: string
} = {}): Promise<{ groups: EventOptionGroup[] }> {
  const params = new URLSearchParams()
  if (query.pipeline_id) {
    params.set('pipeline_id', query.pipeline_id)
  }
  const qs = params.toString()
  const url = qs
    ? `${EVENTS_API_BASE}/options/events?${qs}`
    : `${EVENTS_API_BASE}/options/events`
  return requestJson<{ groups: EventOptionGroup[] }>(url)
}

export async function listCalendarDates(query: {
  year: number
  month: number
  stream_id?: string
  pipeline_id?: string
  event?: string
  status?: EventStatus | ''
  search?: string
}): Promise<{ dates: string[] }> {
  const params = new URLSearchParams()
  params.set('year', String(query.year))
  params.set('month', String(query.month))
  if (query.stream_id) {
    params.set('stream_id', query.stream_id)
  }
  if (query.pipeline_id) {
    params.set('pipeline_id', query.pipeline_id)
  }
  if (query.event) {
    params.set('event', query.event)
  }
  if (query.status) {
    params.set('status', query.status)
  }
  if (query.search) {
    params.set('search', query.search)
  }
  return requestJson<{ dates: string[] }>(
    `${EVENTS_API_BASE}/calendar?${params.toString()}`,
  )
}

export async function listEventFilterStreams(): Promise<{ items: FilterOption[] }> {
  // streams /map is name → id (stream_api.md); mock getStreamsMap is id → name
  const map = await requestJson<Record<string, string>>(
    `${BACKEND_PREFIX}/streams/map`,
  )
  return {
    items: Object.entries(map).map(([name, id]) => ({ id, name })),
  }
}

export async function listEventFilterPipelines(): Promise<{ items: FilterOption[] }> {
  // pipelines /map is id → name (pipeline_api.md)
  const map = await requestJson<Record<string, string>>(
    `${BACKEND_PREFIX}/pipelines/map`,
  )
  return {
    items: Object.entries(map).map(([id, name]) => ({ id, name })),
  }
}

export async function ackEvent(
  eventId: string,
  action: 'ack' | 'unack',
): Promise<EventItem> {
  return requestJson<EventItem>(`${EVENTS_API_BASE}/${eventId}/ack`, {
    method: 'POST',
    body: { action },
  })
}

export async function batchAckEvents(body: {
  event_ids: string[]
  action: 'ack' | 'unack'
}): Promise<BatchAckResult> {
  return requestJson<BatchAckResult>(`${EVENTS_API_BASE}/batch/ack`, {
    method: 'POST',
    body,
  })
}

export async function exportEvents(query: EventListQuery = {}): Promise<void> {
  const params = new URLSearchParams()
  appendListQuery(params, query)
  const qs = params.toString()
  const url = qs ? `${EVENTS_API_BASE}/export?${qs}` : `${EVENTS_API_BASE}/export`
  const blob = await requestBlob(url, {
    method: 'POST',
    body: {},
  })
  downloadBlob(
    `events-export-${query.from ?? 'all'}_${query.to ?? 'all'}.zip`,
    blob,
  )
}

export async function collectEvents(query: EventListQuery = {}): Promise<void> {
  const params = new URLSearchParams()
  appendListQuery(params, query)
  const qs = params.toString()
  const url = qs ? `${EVENTS_API_BASE}/collect?${qs}` : `${EVENTS_API_BASE}/collect`
  const blob = await requestBlob(url, {
    method: 'POST',
    body: {},
  })
  downloadBlob(
    `events-collect-${query.from ?? 'all'}_${query.to ?? 'all'}.zip.enc`,
    blob,
  )
}
