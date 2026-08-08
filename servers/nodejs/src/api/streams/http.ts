/**
 * Streams API — real HTTP client.
 */

import { requestJson } from '@/shared/http/request'
import { BACKEND_PREFIX } from '@/shared/project'
import type {
  Group,
  GroupsTree,
  ListStreamsQuery,
  PagedStreams,
  PublishResult,
  Stream,
  StreamSummary,
  ProbeResult,
} from '@/api/streams/types'

export const STREAMS_API_BASE = `${BACKEND_PREFIX}/streams`

export async function getGroupsTree(): Promise<GroupsTree> {
  return requestJson<GroupsTree>(`${STREAMS_API_BASE}/groups/tree`)
}

export async function getGroupsMap(): Promise<Record<string, string>> {
  return requestJson<Record<string, string>>(`${STREAMS_API_BASE}/groups/map`)
}

export async function getGroupsList(): Promise<{ items: Group[] }> {
  return requestJson<{ items: Group[] }>(`${STREAMS_API_BASE}/groups/list`)
}

export async function createGroup(
  parentGroupId: string,
  body: { name: string },
): Promise<Group> {
  return requestJson<Group>(`${STREAMS_API_BASE}/groups/${parentGroupId}`, {
    method: 'POST',
    body,
  })
}

export async function patchGroup(
  groupId: string,
  body: { name: string },
): Promise<Group> {
  return requestJson<Group>(`${STREAMS_API_BASE}/groups/${groupId}`, {
    method: 'PATCH',
    body,
  })
}

export async function deleteGroup(groupId: string): Promise<void> {
  await requestJson<Record<string, never>>(`${STREAMS_API_BASE}/groups/${groupId}`, {
    method: 'DELETE',
  })
}

export async function getGroupMembers(
  groupId: string,
): Promise<{ items: StreamSummary[] }> {
  return requestJson<{ items: StreamSummary[] }>(
    `${STREAMS_API_BASE}/groups/${groupId}/members`,
  )
}

export async function getGroupCandidates(
  groupId: string,
): Promise<{ items: StreamSummary[] }> {
  return requestJson<{ items: StreamSummary[] }>(
    `${STREAMS_API_BASE}/groups/${groupId}/candidates`,
  )
}

export async function putGroupMembers(
  groupId: string,
  body: { stream_ids: string[] },
): Promise<void> {
  await requestJson<Record<string, never>>(
    `${STREAMS_API_BASE}/groups/${groupId}/members`,
    {
      method: 'PUT',
      body,
    },
  )
}

export async function listStreams(query: ListStreamsQuery = {}): Promise<PagedStreams> {
  const params = new URLSearchParams()
  if (query.group_id) {
    params.set('group_id', query.group_id)
  }
  if (query.stream_id) {
    params.set('stream_id', query.stream_id)
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
  const qs = params.toString()
  const url = qs ? `${STREAMS_API_BASE}/?${qs}` : `${STREAMS_API_BASE}/`
  return requestJson<PagedStreams>(url)
}

export async function getStreamsMap(): Promise<Record<string, string>> {
  return requestJson<Record<string, string>>(`${STREAMS_API_BASE}/map`)
}

export async function getStream(streamId: string): Promise<Stream> {
  return requestJson<Stream>(`${STREAMS_API_BASE}/${streamId}`)
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
  return requestJson<Stream>(`${STREAMS_API_BASE}/`, {
    method: 'POST',
    body,
  })
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
  return requestJson<Stream>(`${STREAMS_API_BASE}/${streamId}`, {
    method: 'PATCH',
    body,
  })
}

export async function deleteStream(streamId: string): Promise<void> {
  await requestJson<Record<string, never>>(`${STREAMS_API_BASE}/${streamId}`, {
    method: 'DELETE',
  })
}

export async function probeStream(streamId: string): Promise<ProbeResult> {
  return requestJson<ProbeResult>(`${STREAMS_API_BASE}/${streamId}/probe`, {
    method: 'POST',
    body: {},
  })
}

export async function probeStreamUrl(url: string): Promise<ProbeResult> {
  return requestJson<ProbeResult>(`${STREAMS_API_BASE}/probe`, {
    method: 'POST',
    body: { url },
  })
}

export async function getStreamLogs(
  streamId: string,
): Promise<{ content: string }> {
  return requestJson<{ content: string }>(`${STREAMS_API_BASE}/${streamId}/logs`)
}

export async function batchDelete(body: { ids: string[] }): Promise<void> {
  await requestJson<Record<string, never>>(`${STREAMS_API_BASE}/batch/delete`, {
    method: 'POST',
    body,
  })
}

export async function batchEnable(body: { ids: string[] }): Promise<void> {
  await requestJson<Record<string, never>>(`${STREAMS_API_BASE}/batch/enable`, {
    method: 'POST',
    body,
  })
}

export async function batchDisable(body: { ids: string[] }): Promise<void> {
  await requestJson<Record<string, never>>(`${STREAMS_API_BASE}/batch/disable`, {
    method: 'POST',
    body,
  })
}

export async function batchRecord(body: { ids: string[] }): Promise<void> {
  await requestJson<Record<string, never>>(`${STREAMS_API_BASE}/batch/record`, {
    method: 'POST',
    body,
  })
}

export async function batchUnrecord(body: { ids: string[] }): Promise<void> {
  await requestJson<Record<string, never>>(`${STREAMS_API_BASE}/batch/unrecord`, {
    method: 'POST',
    body,
  })
}

export async function batchProbe(body: {
  ids: string[]
}): Promise<{ results: ProbeResult[] }> {
  return requestJson<{ results: ProbeResult[] }>(`${STREAMS_API_BASE}/batch/probe`, {
    method: 'POST',
    body,
  })
}

export async function publishVideo(body: {
  file: File
  name?: string
}): Promise<PublishResult> {
  const form = new FormData()
  form.append('input', body.file)
  if (body.name?.trim()) {
    form.append('name', body.name.trim())
  }
  return requestJson<PublishResult>(`${STREAMS_API_BASE}/publishers`, {
    method: 'POST',
    formData: form,
  })
}
