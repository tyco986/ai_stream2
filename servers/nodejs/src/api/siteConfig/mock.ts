/**
 * Site Config Versions API — mock.
 */

import { getMe } from '@/api/shell'
import { ApiError, type ApiEnvelope } from '@/shared/http/client'
import { STORAGE_KEY_PREFIX } from '@/shared/project'
import type { SiteConfigVersion } from '@/api/siteConfig/types'

type StoredVersion = SiteConfigVersion & {
  payload_marker: string
}

const STORAGE_VERSIONS = `${STORAGE_KEY_PREFIX}shell.siteConfigVersions`
const STORAGE_USED_JTIS = `${STORAGE_KEY_PREFIX}shell.usedAuthTickets`

const SEED_ID = '00000000-0000-4000-8000-000000000000'

function ok<T>(data: T): ApiEnvelope<T> {
  return { success: true, message: '', data }
}

function newId(): string {
  return crypto.randomUUID()
}

function seedVersions(): StoredVersion[] {
  return [
    {
      id: SEED_ID,
      version: '0',
      description: 'image default',
      created_at: '2026-07-01T00:00:00.000Z',
      is_current: true,
      payload_marker: 'seed-v0',
    },
  ]
}

function readVersions(): StoredVersion[] {
  const raw = sessionStorage.getItem(STORAGE_VERSIONS)
  if (!raw) {
    const seeded = seedVersions()
    writeVersions(seeded)
    return seeded
  }
  return JSON.parse(raw) as StoredVersion[]
}

function writeVersions(items: StoredVersion[]) {
  sessionStorage.setItem(STORAGE_VERSIONS, JSON.stringify(items))
}

function readUsedJtis(): string[] {
  const raw = sessionStorage.getItem(STORAGE_USED_JTIS)
  if (!raw) {
    return []
  }
  return JSON.parse(raw) as string[]
}

function markJtiUsed(jti: string) {
  const used = readUsedJtis()
  used.push(jti)
  sessionStorage.setItem(STORAGE_USED_JTIS, JSON.stringify(used))
}

function toPublic(item: StoredVersion): SiteConfigVersion {
  return {
    id: item.id,
    version: item.version,
    description: item.description,
    created_at: item.created_at,
    is_current: item.is_current,
  }
}

function decodeJwtPayload(ticket: string): Record<string, unknown> | null {
  const parts = ticket.trim().split('.')
  if (parts.length !== 3) {
    return null
  }
  const json = atob(parts[1].replace(/-/g, '+').replace(/_/g, '/'))
  return JSON.parse(json) as Record<string, unknown>
}

function parseAuthTicket(
  ticket: string,
  expectedAction: 'import' | 'apply',
): { jti: string } {
  const trimmed = ticket.trim()
  if (!trimmed) {
    throw new ApiError(401, 'Invalid auth ticket')
  }
  const payload = decodeJwtPayload(trimmed)
  let action = ''
  let jti = trimmed
  if (payload) {
    action = String(payload.action ?? '')
    jti = String(payload.jti ?? trimmed)
  }
  if (!action && (trimmed === 'import' || trimmed.startsWith('import:'))) {
    action = 'import'
    jti = trimmed
  }
  if (!action && (trimmed === 'apply' || trimmed.startsWith('apply:'))) {
    action = 'apply'
    jti = trimmed
  }
  if (action !== expectedAction) {
    throw new ApiError(401, 'Invalid auth ticket')
  }
  if (readUsedJtis().includes(jti)) {
    throw new ApiError(401, 'Invalid auth ticket')
  }
  return { jti }
}

function requireExportPerm() {
  return getMe().then((user) => {
    if (user.is_superuser || user.permissions.includes('*')) {
      return
    }
    const okPerm =
      user.permissions.includes('users.export_site_config') ||
      user.permissions.includes('users.import_site_config')
    if (!okPerm) {
      throw new ApiError(403, 'Forbidden')
    }
  })
}

function requireImportPerm() {
  return getMe().then((user) => {
    if (user.is_superuser || user.permissions.includes('*')) {
      return
    }
    if (!user.permissions.includes('users.import_site_config')) {
      throw new ApiError(403, 'Forbidden')
    }
  })
}

export async function listVersions(): Promise<{ items: SiteConfigVersion[] }> {
  await requireExportPerm()
  const items = readVersions()
    .map(toPublic)
    .sort((a, b) => parseFloat(b.version) - parseFloat(a.version))
  return ok({ items }).data
}

export async function createVersion(body: {
  version: string
  description?: string
}): Promise<SiteConfigVersion> {
  await requireExportPerm()
  const version = body.version?.trim() ?? ''
  if (!/^\d+(\.\d+)?$/.test(version)) {
    throw new ApiError(400, 'Invalid version')
  }
  const items = readVersions()
  if (items.some((item) => item.version === version)) {
    throw new ApiError(400, 'Version already exists')
  }
  const created: StoredVersion = {
    id: newId(),
    version,
    description: body.description?.trim() || null,
    created_at: new Date().toISOString(),
    is_current: false,
    payload_marker: `backup-${version}-${Date.now()}`,
  }
  items.push(created)
  writeVersions(items)
  return ok(toPublic(created)).data
}

export async function importVersion(
  versionId: string,
  file: File,
  ticket: string,
): Promise<SiteConfigVersion> {
  await requireImportPerm()
  const { jti } = parseAuthTicket(ticket, 'import')
  const items = readVersions()
  const target = items.find((item) => item.id === versionId)
  if (!target) {
    throw new ApiError(404, 'Version not found')
  }
  if (!file || file.size === 0) {
    throw new ApiError(400, 'Invalid package')
  }
  target.payload_marker = `import-${file.name}-${Date.now()}`
  writeVersions(items)
  markJtiUsed(jti)
  return ok(toPublic(target)).data
}

function buildExportBlob(versionId: string): Blob {
  const items = readVersions()
  const target = items.find((item) => item.id === versionId)
  if (!target) {
    throw new ApiError(404, 'Version not found')
  }
  const body = `mock-age-package\nversion=${target.version}\npayload=${target.payload_marker}\n`
  return new Blob([body], { type: 'application/octet-stream' })
}

/** Sync mock export so browser download keeps the user-gesture (no await before a.click). */
export function exportVersionSync(versionId: string): Blob {
  const raw = sessionStorage.getItem(`${STORAGE_KEY_PREFIX}shell.mockLoggedIn`)
  if (raw !== '1') {
    throw new ApiError(401, 'Unauthorized')
  }
  return buildExportBlob(versionId)
}

export async function exportVersion(versionId: string): Promise<Blob> {
  await requireExportPerm()
  return buildExportBlob(versionId)
}

export async function applyVersion(
  versionId: string,
  ticket: string,
): Promise<{ id: string; version: string; applied: boolean }> {
  await requireImportPerm()
  const { jti } = parseAuthTicket(ticket, 'apply')
  const items = readVersions()
  const target = items.find((item) => item.id === versionId)
  if (!target) {
    throw new ApiError(404, 'Version not found')
  }
  if (target.is_current) {
    throw new ApiError(400, 'Already current version')
  }
  for (const item of items) {
    item.is_current = item.id === target.id
  }
  writeVersions(items)
  markJtiUsed(jti)
  return ok({ id: target.id, version: target.version, applied: true }).data
}

export function getCurrentVersionNumber(items: SiteConfigVersion[]): string {
  const current = items.find((item) => item.is_current)
  return current?.version ?? '0'
}
