/**
 * Preview API — real HTTP client.
 */

import { requestBlob, requestJson } from '@/shared/http/request'
import { BACKEND_PREFIX } from '@/shared/project'
import type {
  LayoutPreset,
  LayoutPresetSummary,
  PreviewLayout,
  ViewMode,
} from '@/api/preview/types'

export const PREVIEW_API_BASE = `${BACKEND_PREFIX}/preview`

async function dataUrlToFile(dataUrl: string, filename: string): Promise<File> {
  const response = await fetch(dataUrl)
  const blob = await response.blob()
  return new File([blob], filename, { type: blob.type || 'image/jpeg' })
}

async function blobToDataUrl(blob: Blob): Promise<string> {
  return new Promise((resolve, reject) => {
    const reader = new FileReader()
    reader.onload = () => resolve(String(reader.result))
    reader.onerror = () => reject(reader.error ?? new Error('Failed to read blob'))
    reader.readAsDataURL(blob)
  })
}

export async function getActiveLayout(): Promise<{ preset_id: string }> {
  return requestJson<{ preset_id: string }>(`${PREVIEW_API_BASE}/active_layout`)
}

export async function putActiveLayout(body: {
  preset_id: string
}): Promise<{ preset_id: string }> {
  return requestJson<{ preset_id: string }>(`${PREVIEW_API_BASE}/active_layout`, {
    method: 'PUT',
    body,
  })
}

export async function listLayouts(query: {
  search?: string
} = {}): Promise<{ items: LayoutPresetSummary[]; preset_id: string }> {
  const params = new URLSearchParams()
  if (query.search) {
    params.set('search', query.search)
  }
  const qs = params.toString()
  const url = qs
    ? `${PREVIEW_API_BASE}/layouts?${qs}`
    : `${PREVIEW_API_BASE}/layouts`
  return requestJson<{ items: LayoutPresetSummary[]; preset_id: string }>(url)
}

export async function getLayoutsMap(): Promise<Record<string, string>> {
  return requestJson<Record<string, string>>(`${PREVIEW_API_BASE}/layouts/map`)
}

export async function getLayout(presetId: string): Promise<LayoutPreset> {
  return requestJson<LayoutPreset>(`${PREVIEW_API_BASE}/layouts/${presetId}`)
}

export async function patchLayout(
  presetId: string,
  body: Partial<Pick<LayoutPreset, 'name' | 'layout' | 'view_mode' | 'slots'>>,
): Promise<LayoutPreset> {
  return requestJson<LayoutPreset>(`${PREVIEW_API_BASE}/layouts/${presetId}`, {
    method: 'PATCH',
    body,
  })
}

export async function postLayout(body: {
  name: string
  layout: PreviewLayout
  view_mode: ViewMode
  slots: (string | null)[]
}): Promise<LayoutPreset> {
  return requestJson<LayoutPreset>(`${PREVIEW_API_BASE}/layouts`, {
    method: 'POST',
    body,
  })
}

export async function deleteLayout(
  presetId: string,
): Promise<{ deleted_id: string; was_active: boolean }> {
  return requestJson<{ deleted_id: string; was_active: boolean }>(
    `${PREVIEW_API_BASE}/layouts/${presetId}`,
    { method: 'DELETE' },
  )
}

export async function batchDeleteLayouts(body: {
  ids: string[]
}): Promise<{ deleted_ids: string[]; active_deleted: boolean }> {
  return requestJson<{ deleted_ids: string[]; active_deleted: boolean }>(
    `${PREVIEW_API_BASE}/layouts/batch/delete`,
    {
      method: 'POST',
      body,
    },
  )
}

export async function putShot(
  presetId: string,
  dataUrl: string,
): Promise<LayoutPreset> {
  const file = await dataUrlToFile(dataUrl, 'shot.jpg')
  const form = new FormData()
  form.append('file', file)
  return requestJson<LayoutPreset>(`${PREVIEW_API_BASE}/layouts/${presetId}/shot`, {
    method: 'PUT',
    formData: form,
  })
}

export async function getShot(presetId: string): Promise<string> {
  const blob = await requestBlob(`${PREVIEW_API_BASE}/layouts/${presetId}/shot`)
  return blobToDataUrl(blob)
}
