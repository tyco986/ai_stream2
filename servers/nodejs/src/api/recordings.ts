/**
 * Recordings API client (contract-aligned mock).
 * Paths: /{PROJECT_NAME}/backend/recordings/...
 * Replace with OpenAPI-generated client when ready.
 */

import { ApiError } from '@/shared/http/client'
import { BACKEND_PREFIX, STORAGE_KEY_PREFIX } from '@/shared/project'

export type RecordingsLayout = '1x1' | '2x2' | '3x3' | '4x4'
export type ViewMode = 'grid' | 'focus'

export type LayoutPreset = {
  id: string
  name: string
  layout: RecordingsLayout
  view_mode: ViewMode
  slots: (string | null)[]
  stream_count: number
  shot_url: string | null
}

export type LayoutPresetSummary = {
  id: string
  name: string
  layout: RecordingsLayout
  stream_count: number
}

export type StreamAvailability = {
  stream_id: string
  dates: string[]
}

export type AvailabilityResult = {
  month: string
  streams: StreamAvailability[]
}

export type RecordingSegment = {
  id: string
  file: string
  start: string
  end: string
}

export type SegmentsResult = {
  stream_id: string
  date: string
  segments: RecordingSegment[]
}

export type PlaybackResult = {
  stream_id: string
  file: string
  url: string
  content_type: string
}

export const DEFAULT_LAYOUT_ID = '00000000-0000-4000-8000-000000000003'
export const DEFAULT_LAYOUT_NAME = 'Default'

export const LAYOUT_SLOT_COUNT: Record<RecordingsLayout, number> = {
  '1x1': 1,
  '2x2': 4,
  '3x3': 9,
  '4x4': 16,
}

export const RECORDINGS_API_BASE = `${BACKEND_PREFIX}/recordings`

const STORAGE_PRESETS = `${STORAGE_KEY_PREFIX}recordings.presets`
const STORAGE_ACTIVE = `${STORAGE_KEY_PREFIX}recordings.activeLayoutId`
const STORAGE_SHOT_PREFIX = `${STORAGE_KEY_PREFIX}recordings.shot.`

/** Stream ids aligned with streams seed (recording: true = cam-01). */
const CAM01 = 'c3000001-0000-4000-8000-000000000001'
const CAM02 = 'c3000002-0000-4000-8000-000000000002'

type SegmentSeed = {
  stream_id: string
  date: string
  segments: RecordingSegment[]
}

function emptySlots(layout: RecordingsLayout): (string | null)[] {
  return Array.from({ length: LAYOUT_SLOT_COUNT[layout] }, () => null)
}

function withStreamCount(preset: Omit<LayoutPreset, 'stream_count'>): LayoutPreset {
  return {
    ...preset,
    stream_count: preset.slots.filter((slot) => slot !== null).length,
  }
}

function defaultPreset(): LayoutPreset {
  return withStreamCount({
    id: DEFAULT_LAYOUT_ID,
    name: DEFAULT_LAYOUT_NAME,
    layout: '1x1',
    view_mode: 'grid',
    slots: emptySlots('1x1'),
    shot_url: null,
  })
}

function readPresets(): LayoutPreset[] {
  const raw = sessionStorage.getItem(STORAGE_PRESETS)
  if (!raw) {
    const seed = [defaultPreset()]
    sessionStorage.setItem(STORAGE_PRESETS, JSON.stringify(seed))
    return seed
  }
  return JSON.parse(raw) as LayoutPreset[]
}

function writePresets(presets: LayoutPreset[]) {
  sessionStorage.setItem(STORAGE_PRESETS, JSON.stringify(presets))
}

function readActiveId(): string {
  return sessionStorage.getItem(STORAGE_ACTIVE) || DEFAULT_LAYOUT_ID
}

function writeActiveId(presetId: string) {
  sessionStorage.setItem(STORAGE_ACTIVE, presetId)
}

function shotStorageKey(presetId: string) {
  return `${STORAGE_SHOT_PREFIX}${presetId}`
}

function toSummary(preset: LayoutPreset): LayoutPresetSummary {
  return {
    id: preset.id,
    name: preset.name,
    layout: preset.layout,
    stream_count: preset.stream_count,
  }
}

function plainSlots(slots: (string | null)[]): (string | null)[] {
  return slots.map((slot) => slot)
}

function pad2(n: number) {
  return String(n).padStart(2, '0')
}

function dayIso(year: number, month: number, day: number) {
  return `${year}-${pad2(month)}-${pad2(day)}`
}

function segmentWindow(
  streamId: string,
  date: string,
  startH: number,
  startM: number,
  endH: number,
  endM: number,
  index: number,
): RecordingSegment {
  const name = streamId === CAM01 ? 'cam-01' : streamId === CAM02 ? 'cam-02' : streamId.slice(0, 8)
  const start = `${date}T${pad2(startH)}:${pad2(startM)}:00`
  const end = `${date}T${pad2(endH)}:${pad2(endM)}:00`
  const file = `${name}/${date}_${pad2(startH)}-${pad2(startM)}-00-000000.mp4`
  return {
    id: `seg-${streamId.slice(0, 8)}-${date}-${index}`,
    file,
    start,
    end,
  }
}

function buildSeedSegments(): SegmentSeed[] {
  const now = new Date()
  const year = now.getFullYear()
  const month = now.getMonth() + 1
  const today = now.getDate()
  const dates = [1, 3, 9, 15, today].filter((d, i, arr) => {
    const last = new Date(year, month, 0).getDate()
    return d >= 1 && d <= last && arr.indexOf(d) === i
  })
  const seeds: SegmentSeed[] = []
  for (const day of dates) {
    const date = dayIso(year, month, day)
    seeds.push({
      stream_id: CAM01,
      date,
      segments: [
        segmentWindow(CAM01, date, 0, 0, 12, 0, 0),
        segmentWindow(CAM01, date, 18, 0, 23, 59, 1),
      ],
    })
    if (day === 1 || day === 3 || day === today) {
      seeds.push({
        stream_id: CAM02,
        date,
        segments: [segmentWindow(CAM02, date, 8, 0, 20, 0, 0)],
      })
    }
  }
  return seeds
}

const SEGMENT_SEEDS = buildSeedSegments()

function datesForStreamMonth(streamId: string, month: string): string[] {
  return [
    ...new Set(
      SEGMENT_SEEDS.filter(
        (item) => item.stream_id === streamId && item.date.startsWith(month),
      ).map((item) => item.date),
    ),
  ].sort()
}

function segmentsFor(streamId: string, date: string): RecordingSegment[] {
  const hit = SEGMENT_SEEDS.find(
    (item) => item.stream_id === streamId && item.date === date,
  )
  return hit ? structuredClone(hit.segments) : []
}

export async function getActiveLayout(): Promise<{ preset_id: string }> {
  return { preset_id: readActiveId() }
}

export async function putActiveLayout(body: {
  preset_id: string
}): Promise<{ preset_id: string }> {
  const presets = readPresets()
  if (!presets.some((item) => item.id === body.preset_id)) {
    throw new ApiError(404, 'Preset not found')
  }
  writeActiveId(body.preset_id)
  return { preset_id: body.preset_id }
}

export async function listLayouts(query: {
  search?: string
} = {}): Promise<{ items: LayoutPresetSummary[]; preset_id: string }> {
  let items = readPresets().map(toSummary)
  if (query.search?.trim()) {
    const q = query.search.trim().toLowerCase()
    items = items.filter((item) => item.name.toLowerCase().includes(q))
  }
  return { items, preset_id: readActiveId() }
}

export async function getLayoutsMap(): Promise<Record<string, string>> {
  return Object.fromEntries(readPresets().map((item) => [item.name, item.id]))
}

export async function getLayout(presetId: string): Promise<LayoutPreset> {
  const preset = readPresets().find((item) => item.id === presetId)
  if (!preset) {
    throw new ApiError(404, 'Preset not found')
  }
  return structuredClone(preset)
}

export async function patchLayout(
  presetId: string,
  body: Partial<Pick<LayoutPreset, 'name' | 'layout' | 'view_mode' | 'slots'>>,
): Promise<LayoutPreset> {
  const presets = readPresets()
  const index = presets.findIndex((item) => item.id === presetId)
  if (index < 0) {
    throw new ApiError(404, 'Preset not found')
  }
  const current = presets[index]
  if (body.name !== undefined) {
    if (presetId === DEFAULT_LAYOUT_ID) {
      throw new ApiError(403, 'Cannot modify Default name')
    }
    if (
      body.name === DEFAULT_LAYOUT_NAME ||
      presets.some((item) => item.name === body.name && item.id !== presetId)
    ) {
      throw new ApiError(409, 'Name already exists')
    }
    current.name = body.name
  }
  if (body.layout !== undefined) {
    current.layout = body.layout
  }
  if (body.view_mode !== undefined) {
    current.view_mode = body.view_mode
  }
  if (body.slots !== undefined) {
    const expected = LAYOUT_SLOT_COUNT[current.layout]
    if (body.slots.length !== expected) {
      throw new ApiError(400, 'Slots length mismatch')
    }
    current.slots = plainSlots(body.slots)
  }
  presets[index] = withStreamCount(current)
  writePresets(presets)
  return structuredClone(presets[index])
}

export async function postLayout(body: {
  name: string
  layout: RecordingsLayout
  view_mode: ViewMode
  slots: (string | null)[]
}): Promise<LayoutPreset> {
  if (body.name === DEFAULT_LAYOUT_NAME) {
    throw new ApiError(409, 'Name already exists')
  }
  const presets = readPresets()
  if (presets.some((item) => item.name === body.name)) {
    throw new ApiError(409, 'Name already exists')
  }
  const slots = plainSlots(body.slots)
  if (slots.length !== LAYOUT_SLOT_COUNT[body.layout]) {
    throw new ApiError(400, 'Slots length mismatch')
  }
  const created = withStreamCount({
    id: crypto.randomUUID(),
    name: body.name,
    layout: body.layout,
    view_mode: body.view_mode,
    slots,
    shot_url: null,
  })
  presets.push(created)
  writePresets(presets)
  return structuredClone(created)
}

export async function deleteLayout(
  presetId: string,
): Promise<{ deleted_id: string; was_active: boolean }> {
  if (presetId === DEFAULT_LAYOUT_ID) {
    throw new ApiError(403, 'Cannot delete Default')
  }
  const presets = readPresets()
  const next = presets.filter((item) => item.id !== presetId)
  if (next.length === presets.length) {
    throw new ApiError(404, 'Preset not found')
  }
  const wasActive = readActiveId() === presetId
  writePresets(next)
  sessionStorage.removeItem(shotStorageKey(presetId))
  if (wasActive) {
    writeActiveId(DEFAULT_LAYOUT_ID)
  }
  return { deleted_id: presetId, was_active: wasActive }
}

export async function batchDeleteLayouts(body: {
  ids: string[]
}): Promise<{ deleted_ids: string[]; active_deleted: boolean }> {
  if (body.ids.includes(DEFAULT_LAYOUT_ID)) {
    throw new ApiError(403, 'Cannot delete Default')
  }
  const remove = new Set(body.ids)
  const presets = readPresets()
  const deletedIds = presets
    .filter((item) => remove.has(item.id))
    .map((item) => item.id)
  const activeDeleted = deletedIds.includes(readActiveId())
  writePresets(presets.filter((item) => !remove.has(item.id)))
  for (const id of deletedIds) {
    sessionStorage.removeItem(shotStorageKey(id))
  }
  if (activeDeleted) {
    writeActiveId(DEFAULT_LAYOUT_ID)
  }
  return { deleted_ids: deletedIds, active_deleted: activeDeleted }
}

export async function putShot(
  presetId: string,
  dataUrl: string,
): Promise<LayoutPreset> {
  const presets = readPresets()
  const index = presets.findIndex((item) => item.id === presetId)
  if (index < 0) {
    throw new ApiError(404, 'Preset not found')
  }
  sessionStorage.setItem(shotStorageKey(presetId), dataUrl)
  presets[index].shot_url = dataUrl
  writePresets(presets)
  return structuredClone(presets[index])
}

export async function getShot(presetId: string): Promise<string> {
  const dataUrl = sessionStorage.getItem(shotStorageKey(presetId))
  if (!dataUrl) {
    const preset = readPresets().find((item) => item.id === presetId)
    if (!preset) {
      throw new ApiError(404, 'Preset not found')
    }
    if (preset.shot_url) {
      return preset.shot_url
    }
    throw new ApiError(404, 'Shot not found')
  }
  return dataUrl
}

export async function getAvailability(query: {
  stream_ids: string
  month: string
}): Promise<AvailabilityResult> {
  if (!query.stream_ids.trim()) {
    throw new ApiError(400, 'stream_ids required')
  }
  if (!/^\d{4}-\d{2}$/.test(query.month)) {
    throw new ApiError(400, 'Invalid month')
  }
  const ids = query.stream_ids.split(',').map((id) => id.trim()).filter(Boolean)
  return {
    month: query.month,
    streams: ids.map((streamId) => ({
      stream_id: streamId,
      dates: datesForStreamMonth(streamId, query.month),
    })),
  }
}

export async function getSegments(query: {
  stream_id: string
  date: string
}): Promise<SegmentsResult> {
  if (!/^\d{4}-\d{2}-\d{2}$/.test(query.date)) {
    throw new ApiError(400, 'Invalid date')
  }
  return {
    stream_id: query.stream_id,
    date: query.date,
    segments: segmentsFor(query.stream_id, query.date),
  }
}

export async function getPlayback(query: {
  stream_id: string
  file?: string
  segment_id?: string
}): Promise<PlaybackResult> {
  if (!query.file && !query.segment_id) {
    throw new ApiError(400, 'file or segment_id required')
  }
  let file = query.file ?? ''
  if (query.segment_id) {
    const hit = SEGMENT_SEEDS.flatMap((item) => item.segments).find(
      (seg) => seg.id === query.segment_id,
    )
    if (!hit) {
      throw new ApiError(404, 'Segment not found')
    }
    file = hit.file
  }
  return {
    stream_id: query.stream_id,
    file,
    url: `mock://recordings/${file}`,
    content_type: 'video/mp4',
  }
}

export function resizeSlots(
  slots: (string | null)[],
  layout: RecordingsLayout,
): (string | null)[] {
  const count = LAYOUT_SLOT_COUNT[layout]
  if (slots.length === count) {
    return [...slots]
  }
  if (slots.length < count) {
    return [...slots, ...Array.from({ length: count - slots.length }, () => null)]
  }
  return slots.slice(0, count)
}
