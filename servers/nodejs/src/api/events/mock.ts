/**
 * Events API — mock.
 */

import { ApiError } from '@/shared/http/client'
import { BACKEND_PREFIX, STORAGE_KEY_PREFIX } from '@/shared/project'
import type {
  EventStatus,
  EventItem,
  EventDetail,
  EventOptionItem,
  EventOptionGroup,
  EventListQuery,
  PagedEvents,
  BatchAckResult,
  FilterOption,
} from '@/api/events/types'

export const EVENTS_API_BASE = `${BACKEND_PREFIX}/events`

const STORAGE_EVENTS = `${STORAGE_KEY_PREFIX}events.items`

const CAM01 = 'c3000001-0000-4000-8000-000000000001'
const CAM02 = 'c3000002-0000-4000-8000-000000000002'
const PIPE_PRESENCE = 'd4000001-0000-4000-8000-000000000001'
const PIPE_DET = 'd4000002-0000-4000-8000-000000000002'

const PLACEHOLDER_RAW =
  'data:image/svg+xml,' +
  encodeURIComponent(
    '<svg xmlns="http://www.w3.org/2000/svg" width="640" height="360"><rect fill="#1a1a1a" width="100%" height="100%"/><text x="50%" y="50%" fill="#888" font-size="20" text-anchor="middle" dy=".3em">Raw</text></svg>',
  )
const PLACEHOLDER_VIZ =
  'data:image/svg+xml,' +
  encodeURIComponent(
    '<svg xmlns="http://www.w3.org/2000/svg" width="640" height="360"><rect fill="#0d2137" width="100%" height="100%"/><rect x="180" y="100" width="120" height="160" fill="none" stroke="#f56c6c" stroke-width="3"/><text x="50%" y="40" fill="#67c23a" font-size="18" text-anchor="middle">Visualization</text></svg>',
  )

type StoreRow = EventItem & {
  event_code: string
  raw_url: string | null
  visualization_url: string | null
  payload: Record<string, unknown> | null
}

function todayIso(): string {
  const d = new Date()
  const y = d.getFullYear()
  const m = String(d.getMonth() + 1).padStart(2, '0')
  const day = String(d.getDate()).padStart(2, '0')
  return `${y}-${m}-${day}`
}

function seedRows(): StoreRow[] {
  const day = todayIso()
  const yesterday = (() => {
    const d = new Date()
    d.setDate(d.getDate() - 1)
    const y = d.getFullYear()
    const m = String(d.getMonth() + 1).padStart(2, '0')
    const dd = String(d.getDate()).padStart(2, '0')
    return `${y}-${m}-${dd}`
  })()

  return [
    {
      id: 'e1000001-0000-4000-8000-000000000001',
      occurred_at: `${day}T19:40:12`,
      stream_id: CAM01,
      stream_name: 'cam-01',
      pipeline_id: PIPE_PRESENCE,
      pipeline_name: 'presence-a',
      event_code: '1',
      event: 'Intrusion',
      status: 'new',
      raw_url: PLACEHOLDER_RAW,
      visualization_url: PLACEHOLDER_VIZ,
      payload: { source: 'mock' },
    },
    {
      id: 'e1000002-0000-4000-8000-000000000002',
      occurred_at: `${day}T19:38:01`,
      stream_id: CAM02,
      stream_name: 'cam-02',
      pipeline_id: PIPE_PRESENCE,
      pipeline_name: 'presence-a',
      event_code: '2',
      event: 'Clear',
      status: 'acked',
      raw_url: PLACEHOLDER_RAW,
      visualization_url: PLACEHOLDER_VIZ,
      payload: null,
    },
    {
      id: 'e1000003-0000-4000-8000-000000000003',
      occurred_at: `${day}T14:12:45`,
      stream_id: CAM01,
      stream_name: 'cam-01',
      pipeline_id: PIPE_DET,
      pipeline_name: 'yolo26n-det-rtsp',
      event_code: 'person',
      event: 'Person',
      status: 'new',
      raw_url: PLACEHOLDER_RAW,
      visualization_url: PLACEHOLDER_VIZ,
      payload: null,
    },
    {
      id: 'e1000004-0000-4000-8000-000000000004',
      occurred_at: `${day}T09:05:00`,
      stream_id: CAM01,
      stream_name: 'cam-01',
      pipeline_id: PIPE_PRESENCE,
      pipeline_name: 'presence-a',
      event_code: '0',
      event: 'Idle',
      status: 'new',
      raw_url: PLACEHOLDER_RAW,
      visualization_url: null,
      payload: null,
    },
    {
      id: 'e1000005-0000-4000-8000-000000000005',
      occurred_at: `${yesterday}T18:20:33`,
      stream_id: CAM02,
      stream_name: 'cam-02',
      pipeline_id: PIPE_DET,
      pipeline_name: 'yolo26n-det-rtsp',
      event_code: 'person',
      event: 'Person',
      status: 'acked',
      raw_url: PLACEHOLDER_RAW,
      visualization_url: PLACEHOLDER_VIZ,
      payload: null,
    },
  ]
}

function readStore(): StoreRow[] {
  const raw = sessionStorage.getItem(STORAGE_EVENTS)
  if (!raw) {
    const seed = seedRows()
    writeStore(seed)
    return seed
  }
  return JSON.parse(raw) as StoreRow[]
}

function writeStore(rows: StoreRow[]) {
  sessionStorage.setItem(STORAGE_EVENTS, JSON.stringify(rows))
}

function toListItem(row: StoreRow): EventItem {
  return {
    id: row.id,
    occurred_at: row.occurred_at,
    stream_id: row.stream_id,
    stream_name: row.stream_name,
    pipeline_id: row.pipeline_id,
    pipeline_name: row.pipeline_name,
    event: row.event,
    status: row.status,
  }
}

function dateOf(occurredAt: string): string {
  return occurredAt.slice(0, 10)
}

function sortRows(rows: StoreRow[]): StoreRow[] {
  return [...rows].sort((a, b) => {
    if (a.occurred_at === b.occurred_at) {
      return a.id < b.id ? 1 : -1
    }
    return a.occurred_at < b.occurred_at ? 1 : -1
  })
}

function filterRows(rows: StoreRow[], query: EventListQuery): StoreRow[] {
  let result = rows
  if (query.from || query.to) {
    result = result.filter((row) => {
      const day = dateOf(row.occurred_at)
      if (query.from && day < query.from) {
        return false
      }
      if (query.to && day > query.to) {
        return false
      }
      return true
    })
  }
  if (query.stream_id) {
    result = result.filter((row) => row.stream_id === query.stream_id)
  }
  if (query.pipeline_id) {
    result = result.filter((row) => row.pipeline_id === query.pipeline_id)
  }
  if (query.event) {
    result = result.filter((row) => row.event_code === query.event)
  }
  if (query.status) {
    result = result.filter((row) => row.status === query.status)
  }
  const search = query.search?.trim().toLowerCase()
  if (search) {
    result = result.filter((row) => {
      const hay = `${row.stream_name} ${row.pipeline_name} ${row.event}`.toLowerCase()
      return hay.includes(search)
    })
  }
  return sortRows(result)
}

export async function listEvents(query: EventListQuery = {}): Promise<PagedEvents> {
  const page = query.page ?? 1
  const pageSize = query.page_size ?? 20
  const filtered = filterRows(readStore(), query)
  const start = (page - 1) * pageSize
  const items = filtered.slice(start, start + pageSize).map(toListItem)
  return {
    items,
    total: filtered.length,
    page,
    page_size: pageSize,
  }
}

export async function getEvent(
  eventId: string,
  query: EventListQuery = {},
): Promise<EventDetail> {
  const store = readStore()
  const row = store.find((item) => item.id === eventId)
  if (!row) {
    throw new ApiError(404, 'Event not found')
  }
  const filtered = filterRows(store, query)
  const index = filtered.findIndex((item) => item.id === eventId)
  const prevId = index > 0 ? filtered[index - 1].id : null
  const nextId =
    index >= 0 && index < filtered.length - 1 ? filtered[index + 1].id : null
  return {
    ...toListItem(row),
    raw_url: row.raw_url,
    visualization_url: row.visualization_url,
    prev_id: prevId,
    next_id: nextId,
    payload: row.payload,
  }
}

export async function listEventOptions(query: {
  pipeline_id?: string
} = {}): Promise<{ groups: EventOptionGroup[] }> {
  const store = readStore()
  const byPipeline = new Map<string, EventOptionGroup>()
  for (const row of store) {
    if (query.pipeline_id && row.pipeline_id !== query.pipeline_id) {
      continue
    }
    let group = byPipeline.get(row.pipeline_id)
    if (!group) {
      group = {
        pipeline_id: row.pipeline_id,
        pipeline_name: row.pipeline_name,
        items: [],
      }
      byPipeline.set(row.pipeline_id, group)
    }
    if (!group.items.some((item) => item.value === row.event_code)) {
      group.items.push({ value: row.event_code, label: row.event })
    }
  }
  return { groups: [...byPipeline.values()] }
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
  const prefix = `${query.year}-${String(query.month).padStart(2, '0')}`
  const filtered = filterRows(readStore(), {
    stream_id: query.stream_id,
    pipeline_id: query.pipeline_id,
    event: query.event,
    status: query.status,
    search: query.search,
  })
  const dates = [
    ...new Set(
      filtered
        .map((row) => dateOf(row.occurred_at))
        .filter((day) => day.startsWith(prefix)),
    ),
  ].sort()
  return { dates }
}

export async function listEventFilterStreams(): Promise<{ items: FilterOption[] }> {
  const map = new Map<string, string>()
  for (const row of readStore()) {
    map.set(row.stream_id, row.stream_name)
  }
  return {
    items: [...map.entries()].map(([id, name]) => ({ id, name })),
  }
}

export async function listEventFilterPipelines(): Promise<{ items: FilterOption[] }> {
  const map = new Map<string, string>()
  for (const row of readStore()) {
    map.set(row.pipeline_id, row.pipeline_name)
  }
  return {
    items: [...map.entries()].map(([id, name]) => ({ id, name })),
  }
}

export async function ackEvent(
  eventId: string,
  action: 'ack' | 'unack',
): Promise<EventItem> {
  const store = readStore()
  const row = store.find((item) => item.id === eventId)
  if (!row) {
    throw new ApiError(404, 'Event not found')
  }
  const target: EventStatus = action === 'ack' ? 'acked' : 'new'
  if (row.status === target) {
    return toListItem(row)
  }
  row.status = target
  writeStore(store)
  return toListItem(row)
}

export async function batchAckEvents(body: {
  event_ids: string[]
  action: 'ack' | 'unack'
}): Promise<BatchAckResult> {
  const store = readStore()
  let ackedCount = 0
  let unackedCount = 0
  const idSet = new Set(body.event_ids)
  const target: EventStatus = body.action === 'ack' ? 'acked' : 'new'
  for (const row of store) {
    if (!idSet.has(row.id) || row.status === target) {
      continue
    }
    row.status = target
    if (body.action === 'ack') {
      ackedCount += 1
    } else {
      unackedCount += 1
    }
  }
  writeStore(store)
  return {
    updated_count: ackedCount + unackedCount,
    acked_count: ackedCount,
    unacked_count: unackedCount,
  }
}

function buildExportText(query: EventListQuery, mode: 'export' | 'collect'): string {
  const rows = filterRows(readStore(), query)
  if (rows.length === 0) {
    throw new ApiError(400, 'No events to export')
  }
  const header = 'id,occurred_at,stream_name,pipeline_name,event,status'
  const lines = rows.map(
    (row) =>
      `${row.id},${row.occurred_at},${row.stream_name},${row.pipeline_name},${row.event},${row.status}`,
  )
  const extras =
    mode === 'collect'
      ? '\n\n# mock collect package\n# includes: events.csv, raw/, visualization/, labelme/, yolo/\n# encrypted: mock\n'
      : '\n\n# mock export package\n# includes: events.csv, raw/, visualization/\n'
  return `${header}\n${lines.join('\n')}${extras}`
}

function downloadBlob(filename: string, content: string, mime: string) {
  const blob = new Blob([content], { type: mime })
  const url = URL.createObjectURL(blob)
  const anchor = document.createElement('a')
  anchor.href = url
  anchor.download = filename
  anchor.click()
  URL.revokeObjectURL(url)
}

export async function exportEvents(query: EventListQuery = {}): Promise<void> {
  const text = buildExportText(query, 'export')
  downloadBlob(
    `events-export-${query.from ?? 'all'}_${query.to ?? 'all'}.zip.txt`,
    text,
    'text/plain',
  )
}

export async function collectEvents(query: EventListQuery = {}): Promise<void> {
  const text = buildExportText(query, 'collect')
  downloadBlob(
    `events-collect-${query.from ?? 'all'}_${query.to ?? 'all'}.zip.enc.txt`,
    text,
    'text/plain',
  )
}

export {
  eventDateFromOccurredAt,
  eventTimeQueryFromOccurredAt,
  formatEventTimestamp,
} from '@/api/events/utils'
