/**
 * Streams API — mock.
 */

import { ApiError } from '@/shared/http/client'
import { BACKEND_PREFIX, STORAGE_KEY_PREFIX } from '@/shared/project'
import type {
  StreamStatus,
  TreeStreamNode,
  TreeGroupNode,
  TreeNode,
  GroupsTree,
  Group,
  Stream,
  StreamSummary,
  ProbeResult,
  PublishResult,
  ListStreamsQuery,
  PagedStreams,
} from '@/api/streams/types'
import {
  ALL_GROUP_ID,
} from '@/api/streams/types'

export const STREAMS_API_BASE = `${BACKEND_PREFIX}/streams`

const STORAGE_GROUPS = `${STORAGE_KEY_PREFIX}streams.groups`
const STORAGE_STREAMS = `${STORAGE_KEY_PREFIX}streams.streams`

type Store = {
  groups: Group[]
  streams: Stream[]
}

function seedStore(): Store {
  return {
    groups: [
      { id: ALL_GROUP_ID, name: 'All', parent_id: null },
      {
        id: 'b2000001-0000-4000-8000-000000000001',
        name: 'Site A',
        parent_id: ALL_GROUP_ID,
      },
      {
        id: 'b2000002-0000-4000-8000-000000000002',
        name: 'Entrance',
        parent_id: 'b2000001-0000-4000-8000-000000000001',
      },
      {
        id: 'b2000003-0000-4000-8000-000000000003',
        name: 'East Gate',
        parent_id: 'b2000002-0000-4000-8000-000000000002',
      },
      {
        id: 'b2000004-0000-4000-8000-000000000004',
        name: 'Garage',
        parent_id: 'b2000001-0000-4000-8000-000000000001',
      },
    ],
    streams: [
      {
        id: 'a1000001-0000-4000-8000-000000000001',
        name: 'cam-04',
        group_id: ALL_GROUP_ID,
        group_name: 'All',
        url: 'rtsp://user:pass@192.168.1.40/s1',
        resolution: '1920x1080',
        fps: 30,
        status: 'offline',
        enabled: true,
        recording: false,
        last_probe_at: '2026-06-27T18:22:00Z',
      },
      {
        id: 'c3000001-0000-4000-8000-000000000001',
        name: 'cam-01',
        group_id: 'b2000002-0000-4000-8000-000000000002',
        group_name: 'Entrance',
        url: 'rtsp://user:pass@192.168.1.10/s1',
        resolution: '1920x1080',
        fps: 25,
        status: 'online',
        enabled: true,
        recording: true,
        last_probe_at: '2026-06-28T10:15:00Z',
      },
      {
        id: 'c3000002-0000-4000-8000-000000000002',
        name: 'cam-02',
        group_id: 'b2000002-0000-4000-8000-000000000002',
        group_name: 'Entrance',
        url: 'rtsp://user:pass@192.168.1.11/s2',
        resolution: '1280x720',
        fps: 15,
        status: 'online',
        enabled: true,
        recording: false,
        last_probe_at: '2026-06-28T09:40:00Z',
      },
      {
        id: 'c3000006-0000-4000-8000-000000000006',
        name: 'cam-06',
        group_id: 'b2000003-0000-4000-8000-000000000003',
        group_name: 'East Gate',
        url: 'rtsp://user:pass@192.168.1.16/s1',
        resolution: null,
        fps: null,
        status: 'offline',
        enabled: false,
        recording: false,
        last_probe_at: null,
      },
    ],
  }
}

function readStore(): Store {
  const g = sessionStorage.getItem(STORAGE_GROUPS)
  const s = sessionStorage.getItem(STORAGE_STREAMS)
  if (!g || !s) {
    const seed = seedStore()
    writeStore(seed)
    return seed
  }
  return {
    groups: JSON.parse(g) as Group[],
    streams: JSON.parse(s) as Stream[],
  }
}

function writeStore(store: Store) {
  sessionStorage.setItem(STORAGE_GROUPS, JSON.stringify(store.groups))
  sessionStorage.setItem(STORAGE_STREAMS, JSON.stringify(store.streams))
}

function groupName(store: Store, groupId: string): string {
  return store.groups.find((item) => item.id === groupId)?.name ?? 'All'
}

function syncStreamGroupNames(store: Store) {
  for (const stream of store.streams) {
    stream.group_name = groupName(store, stream.group_id)
  }
}

function childGroupIds(store: Store, groupId: string): string[] {
  const ids = [groupId]
  for (const group of store.groups) {
    if (group.parent_id === groupId) {
      ids.push(...childGroupIds(store, group.id))
    }
  }
  return ids
}

function buildTree(store: Store): GroupsTree {
  const byParent = new Map<string | null, Group[]>()
  for (const group of store.groups) {
    const key = group.parent_id
    const list = byParent.get(key) ?? []
    list.push(group)
    byParent.set(key, list)
  }

  function buildGroup(group: Group): TreeGroupNode {
    const childGroups = (byParent.get(group.id) ?? []).map(buildGroup)
    const streams: TreeStreamNode[] = store.streams
      .filter((stream) => stream.group_id === group.id)
      .map((stream) => ({
        id: stream.id,
        name: stream.name,
        type: 'stream' as const,
        enabled: stream.enabled,
        status: stream.status,
        recording: stream.recording,
      }))
    return {
      id: group.id,
      name: group.name,
      type: 'group',
      children: [...streams, ...childGroups],
    }
  }

  const rootGroup = store.groups.find((item) => item.id === ALL_GROUP_ID)
  if (!rootGroup) {
    throw new ApiError(500, 'All group missing')
  }
  return { root: buildGroup(rootGroup) }
}

export { collectStreams } from '@/api/streams/utils'

export async function getGroupsTree(): Promise<GroupsTree> {
  return buildTree(readStore())
}

export async function getGroupsMap(): Promise<Record<string, string>> {
  const store = readStore()
  return Object.fromEntries(store.groups.map((item) => [item.id, item.name]))
}

export async function getGroupsList(): Promise<{ items: Group[] }> {
  return { items: structuredClone(readStore().groups) }
}

export async function createGroup(
  parentGroupId: string,
  body: { name: string },
): Promise<Group> {
  const store = readStore()
  if (!store.groups.some((item) => item.id === parentGroupId)) {
    throw new ApiError(404, 'Parent group not found')
  }
  const name = body.name.trim()
  if (!name) {
    throw new ApiError(400, 'Name required')
  }
  const created: Group = {
    id: crypto.randomUUID(),
    name,
    parent_id: parentGroupId,
  }
  store.groups.push(created)
  writeStore(store)
  return structuredClone(created)
}

export async function patchGroup(
  groupId: string,
  body: { name: string },
): Promise<Group> {
  if (groupId === ALL_GROUP_ID) {
    throw new ApiError(403, 'Cannot rename All')
  }
  const store = readStore()
  const group = store.groups.find((item) => item.id === groupId)
  if (!group) {
    throw new ApiError(404, 'Group not found')
  }
  const name = body.name.trim()
  if (!name) {
    throw new ApiError(400, 'Name required')
  }
  group.name = name
  syncStreamGroupNames(store)
  writeStore(store)
  return structuredClone(group)
}

export async function deleteGroup(groupId: string): Promise<void> {
  if (groupId === ALL_GROUP_ID) {
    throw new ApiError(403, 'Cannot delete All')
  }
  const store = readStore()
  const removeIds = new Set(childGroupIds(store, groupId))
  for (const stream of store.streams) {
    if (removeIds.has(stream.group_id)) {
      stream.group_id = ALL_GROUP_ID
      stream.group_name = 'All'
    }
  }
  store.groups = store.groups.filter((item) => !removeIds.has(item.id))
  writeStore(store)
}

export async function getGroupMembers(
  groupId: string,
): Promise<{ items: StreamSummary[] }> {
  const store = readStore()
  const items = store.streams
    .filter((stream) => stream.group_id === groupId)
    .map((stream) => ({
      id: stream.id,
      name: stream.name,
      group_id: stream.group_id,
      group_name: stream.group_name,
    }))
  return { items }
}

export async function getGroupCandidates(
  groupId: string,
): Promise<{ items: StreamSummary[] }> {
  const store = readStore()
  const items = store.streams
    .filter((stream) => stream.group_id !== groupId)
    .map((stream) => ({
      id: stream.id,
      name: stream.name,
      group_id: stream.group_id,
      group_name: stream.group_name,
    }))
  return { items }
}

export async function putGroupMembers(
  groupId: string,
  body: { stream_ids: string[] },
): Promise<void> {
  const store = readStore()
  if (!store.groups.some((item) => item.id === groupId)) {
    throw new ApiError(404, 'Group not found')
  }
  const selected = new Set(body.stream_ids)
  for (const stream of store.streams) {
    if (selected.has(stream.id)) {
      stream.group_id = groupId
      stream.group_name = groupName(store, groupId)
    } else if (stream.group_id === groupId) {
      stream.group_id = ALL_GROUP_ID
      stream.group_name = 'All'
    }
  }
  writeStore(store)
}

export async function listStreams(query: ListStreamsQuery = {}): Promise<PagedStreams> {
  const store = readStore()
  const page = query.page ?? 1
  const pageSize = query.page_size ?? 20
  let rows = [...store.streams]

  if (query.stream_id) {
    rows = rows.filter((item) => item.id === query.stream_id)
  } else if (query.group_id) {
    const ids = new Set(childGroupIds(store, query.group_id))
    rows = rows.filter((item) => ids.has(item.group_id))
  }

  if (query.search?.trim()) {
    const q = query.search.trim().toLowerCase()
    rows = rows.filter(
      (item) =>
        item.name.toLowerCase().includes(q) ||
        item.url.toLowerCase().includes(q) ||
        item.group_name.toLowerCase().includes(q),
    )
  }

  const total = rows.length
  const start = (page - 1) * pageSize
  return {
    items: structuredClone(rows.slice(start, start + pageSize)),
    total,
    page,
    page_size: pageSize,
  }
}

export async function getStreamsMap(): Promise<Record<string, string>> {
  const store = readStore()
  return Object.fromEntries(store.streams.map((item) => [item.id, item.name]))
}

export async function getStream(streamId: string): Promise<Stream> {
  const stream = readStore().streams.find((item) => item.id === streamId)
  if (!stream) {
    throw new ApiError(404, 'Stream not found')
  }
  return structuredClone(stream)
}

export async function createStream(body: {
  name: string
  group_id: string
  url: string
  enabled: boolean
  recording: boolean
  resolution?: string | null
  fps?: number | null
}): Promise<Stream> {
  const store = readStore()
  if (!store.groups.some((item) => item.id === body.group_id)) {
    throw new ApiError(400, 'Invalid group')
  }
  if (store.streams.some((item) => item.name === body.name.trim())) {
    throw new ApiError(409, 'Name already exists')
  }
  const hasProbe = body.resolution != null || body.fps != null
  const created: Stream = {
    id: crypto.randomUUID(),
    name: body.name.trim(),
    group_id: body.group_id,
    group_name: groupName(store, body.group_id),
    url: body.url.trim(),
    resolution: hasProbe ? (body.resolution ?? null) : null,
    fps: hasProbe ? (body.fps ?? null) : null,
    status: hasProbe ? 'online' : 'offline',
    enabled: body.enabled,
    recording: body.recording,
    last_probe_at: hasProbe ? new Date().toISOString() : null,
  }
  store.streams.push(created)
  writeStore(store)
  return structuredClone(created)
}

export async function patchStream(
  streamId: string,
  body: Partial<
    Pick<
      Stream,
      'name' | 'group_id' | 'url' | 'enabled' | 'recording' | 'resolution' | 'fps'
    >
  >,
): Promise<Stream> {
  const store = readStore()
  const stream = store.streams.find((item) => item.id === streamId)
  if (!stream) {
    throw new ApiError(404, 'Stream not found')
  }
  if (body.name !== undefined) {
    const name = body.name.trim()
    if (store.streams.some((item) => item.name === name && item.id !== streamId)) {
      throw new ApiError(409, 'Name already exists')
    }
    stream.name = name
  }
  if (body.group_id !== undefined) {
    if (!store.groups.some((item) => item.id === body.group_id)) {
      throw new ApiError(400, 'Invalid group')
    }
    stream.group_id = body.group_id
    stream.group_name = groupName(store, body.group_id)
  }
  if (body.url !== undefined) {
    stream.url = body.url.trim()
  }
  if (body.enabled !== undefined) {
    stream.enabled = body.enabled
  }
  if (body.recording !== undefined) {
    stream.recording = body.recording
  }
  if (body.resolution !== undefined || body.fps !== undefined) {
    if (body.resolution !== undefined) {
      stream.resolution = body.resolution
    }
    if (body.fps !== undefined) {
      stream.fps = body.fps
    }
    stream.status = 'online'
    stream.last_probe_at = new Date().toISOString()
  }
  writeStore(store)
  return structuredClone(stream)
}

export async function deleteStream(streamId: string): Promise<void> {
  const store = readStore()
  const next = store.streams.filter((item) => item.id !== streamId)
  if (next.length === store.streams.length) {
    throw new ApiError(404, 'Stream not found')
  }
  store.streams = next
  writeStore(store)
}

export async function probeStream(streamId: string): Promise<ProbeResult> {
  const store = readStore()
  const stream = store.streams.find((item) => item.id === streamId)
  if (!stream) {
    throw new ApiError(404, 'Stream not found')
  }
  const now = new Date().toISOString()
  stream.status = 'online'
  stream.resolution = stream.resolution || '1920x1080'
  stream.fps = stream.fps || 25
  stream.last_probe_at = now
  writeStore(store)
  return {
    id: stream.id,
    success: true,
    resolution: stream.resolution,
    fps: stream.fps,
    status: stream.status,
    last_probe_at: now,
    enabled: stream.enabled,
    recording: stream.recording,
  }
}

export async function probeStreamUrl(url: string): Promise<ProbeResult> {
  const cleaned = url.trim()
  if (!cleaned) {
    return { id: '', success: false, error: 'url is required' }
  }
  if (!cleaned.startsWith('rtsp://')) {
    return { id: '', success: false, error: 'rtsp must start with rtsp://' }
  }
  return {
    id: '',
    success: true,
    resolution: '1920x1080',
    fps: 25,
  }
}

export async function getStreamLogs(
  streamId: string,
): Promise<{ content: string }> {
  const stream = readStore().streams.find((item) => item.id === streamId)
  if (!stream) {
    throw new ApiError(404, 'Stream not found')
  }
  return {
    content: [
      `[mock] stream=${stream.name} id=${stream.id}`,
      `[mock] url=${stream.url}`,
      `[mock] status=${stream.status} enabled=${stream.enabled}`,
    ].join('\n'),
  }
}

export async function batchDelete(body: { ids: string[] }): Promise<void> {
  const store = readStore()
  const remove = new Set(body.ids)
  store.streams = store.streams.filter((item) => !remove.has(item.id))
  writeStore(store)
}

export async function batchEnable(body: { ids: string[] }): Promise<void> {
  const store = readStore()
  const ids = new Set(body.ids)
  for (const stream of store.streams) {
    if (ids.has(stream.id)) {
      stream.enabled = true
    }
  }
  writeStore(store)
}

export async function batchDisable(body: { ids: string[] }): Promise<void> {
  const store = readStore()
  const ids = new Set(body.ids)
  for (const stream of store.streams) {
    if (ids.has(stream.id)) {
      stream.enabled = false
    }
  }
  writeStore(store)
}

export async function batchRecord(body: { ids: string[] }): Promise<void> {
  const store = readStore()
  const ids = new Set(body.ids)
  for (const stream of store.streams) {
    if (ids.has(stream.id) && stream.enabled) {
      stream.recording = true
    }
  }
  writeStore(store)
}

export async function batchUnrecord(body: { ids: string[] }): Promise<void> {
  const store = readStore()
  const ids = new Set(body.ids)
  for (const stream of store.streams) {
    if (ids.has(stream.id)) {
      stream.recording = false
    }
  }
  writeStore(store)
}

export async function batchProbe(body: {
  ids: string[]
}): Promise<{ results: ProbeResult[] }> {
  const results: ProbeResult[] = []
  for (const id of body.ids) {
    results.push(await probeStream(id))
  }
  return { results }
}

export async function publishVideo(body: {
  file: File
  name?: string
}): Promise<PublishResult> {
  const stem = body.file.name.replace(/\.[^.]+$/, '') || 'video'
  const name = body.name?.trim() || stem
  return {
    name,
    url: `rtsp://ai_stream2_mediamtx:8554/${name}`,
  }
}
