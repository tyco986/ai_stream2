/**
 * Preview API — mock.
 */

import { ApiError } from '@/shared/http/client'
import { BACKEND_PREFIX, STORAGE_KEY_PREFIX } from '@/shared/project'
import type {
  PreviewLayout,
  ViewMode,
  LayoutPreset,
  LayoutPresetSummary,
} from '@/api/preview/types'
import {
  DEFAULT_LAYOUT_ID,
  DEFAULT_LAYOUT_NAME,
  LAYOUT_SLOT_COUNT,
} from '@/api/preview/types'

/** Reserved for real HTTP client. */
export const PREVIEW_API_BASE = `${BACKEND_PREFIX}/preview`

const STORAGE_PRESETS = `${STORAGE_KEY_PREFIX}preview.presets`
const STORAGE_ACTIVE = `${STORAGE_KEY_PREFIX}preview.activeLayoutId`
const STORAGE_SHOT_PREFIX = `${STORAGE_KEY_PREFIX}preview.shot.`

function emptySlots(layout: PreviewLayout): (string | null)[] {
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
    layout: '2x2',
    view_mode: 'grid',
    slots: emptySlots('2x2'),
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
  layout: PreviewLayout
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

export { resizeSlots } from '@/api/preview/utils'
