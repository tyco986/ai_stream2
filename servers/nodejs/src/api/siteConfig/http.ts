/**
 * Site Config Versions API — real HTTP (under shell prefix).
 */

import { ApiError } from '@/shared/http/client'
import { requestBlob, requestJson } from '@/shared/http/request'
import { BACKEND_PREFIX } from '@/shared/project'
import type { SiteConfigVersion } from '@/api/siteConfig/types'

const BASE = `${BACKEND_PREFIX}/shell/site-config`

export async function listVersions(): Promise<{ items: SiteConfigVersion[] }> {
  return requestJson<{ items: SiteConfigVersion[] }>(`${BASE}/versions`)
}

export async function createVersion(body: {
  version: string
  description?: string
}): Promise<SiteConfigVersion> {
  return requestJson<SiteConfigVersion>(`${BASE}/versions`, {
    method: 'POST',
    body,
  })
}

export async function importVersion(
  versionId: string,
  file: File,
  ticket: string,
): Promise<SiteConfigVersion> {
  const form = new FormData()
  form.append('file', file)
  return requestJson<SiteConfigVersion>(`${BASE}/versions/${versionId}/import`, {
    method: 'POST',
    formData: form,
    headers: { 'X-Auth-Ticket': ticket },
  })
}

export function exportVersionSync(versionId: string): Blob {
  void versionId
  throw new ApiError(400, 'exportVersionSync is mock-only; use exportVersion()')
}

export async function exportVersion(versionId: string): Promise<Blob> {
  return requestBlob(`${BASE}/versions/${versionId}/export`, {
    method: 'POST',
    body: {},
  })
}

export async function applyVersion(
  versionId: string,
  ticket: string,
): Promise<{ id: string; version: string; applied: boolean }> {
  return requestJson<{ id: string; version: string; applied: boolean }>(
    `${BASE}/versions/${versionId}/apply`,
    {
      method: 'POST',
      body: {},
      headers: { 'X-Auth-Ticket': ticket },
    },
  )
}

export function getCurrentVersionNumber(items: SiteConfigVersion[]): string {
  const current = items.find((item) => item.is_current)
  return current?.version ?? '0'
}
