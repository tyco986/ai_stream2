/**
 * Pipelines API — real HTTP client.
 */

import { requestJson } from '@/shared/http/request'
import { BACKEND_PREFIX } from '@/shared/project'
import type {
  AnalyzerSourceResult,
  AnalyzerTemplate,
  BatchDeleteResult,
  BatchIdsBody,
  CaptureAnalyzerSourceStreamBody,
  CreateAnalyzerTemplateBody,
  CreateGieTemplateBody,
  CreatePipelineBody,
  GieTemplate,
  GieTemplateListItem,
  ListAnalyzerTemplatesQuery,
  ListGieTemplatesQuery,
  ListPipelinesQuery,
  NameMap,
  PagedAnalyzerTemplates,
  PagedGieTemplates,
  PagedPipelines,
  Pipeline,
  PipelineLogs,
  PipelineSchemaBody,
  PipelineStatusResult,
  PipelineTypesResult,
  TypeSchema,
  UpdateAnalyzerTemplateBody,
  UpdateGieTemplateBody,
  UpdatePipelineBody,
} from '@/api/pipelines/types'

export const PIPELINES_API_BASE = `${BACKEND_PREFIX}/pipelines`
export const GIE_TEMPLATES_API_BASE = `${BACKEND_PREFIX}/gie-templates`
export const ANALYZER_TEMPLATES_API_BASE = `${BACKEND_PREFIX}/analyzer-templates`

function pageQuery(query: {
  search?: string
  page?: number
  page_size?: number
}): string {
  const params = new URLSearchParams()
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
  return qs ? `?${qs}` : ''
}

export async function getPipelinesMap(): Promise<NameMap> {
  return requestJson<NameMap>(`${PIPELINES_API_BASE}/map`)
}

export async function listPipelines(
  query: ListPipelinesQuery = {},
): Promise<PagedPipelines> {
  return requestJson<PagedPipelines>(
    `${PIPELINES_API_BASE}/${pageQuery(query)}`,
  )
}

export async function getPipelineTypes(): Promise<PipelineTypesResult> {
  return requestJson<PipelineTypesResult>(`${PIPELINES_API_BASE}/types`)
}

export async function getPipelineSchema(body: PipelineSchemaBody): Promise<TypeSchema> {
  return requestJson<TypeSchema>(`${PIPELINES_API_BASE}/schema`, {
    method: 'POST',
    body,
  })
}

export async function getPipeline(pipelineId: string): Promise<Pipeline> {
  return requestJson<Pipeline>(`${PIPELINES_API_BASE}/${pipelineId}`)
}

export async function createPipeline(body: CreatePipelineBody): Promise<Pipeline> {
  return requestJson<Pipeline>(`${PIPELINES_API_BASE}/`, {
    method: 'POST',
    body,
  })
}

export async function updatePipeline(
  pipelineId: string,
  body: UpdatePipelineBody,
): Promise<Pipeline> {
  return requestJson<Pipeline>(`${PIPELINES_API_BASE}/${pipelineId}`, {
    method: 'PUT',
    body,
  })
}

export async function deletePipeline(pipelineId: string): Promise<null> {
  return requestJson<null>(`${PIPELINES_API_BASE}/${pipelineId}`, {
    method: 'DELETE',
  })
}

export async function batchDeletePipelines(
  body: BatchIdsBody,
): Promise<BatchDeleteResult> {
  return requestJson<BatchDeleteResult>(`${PIPELINES_API_BASE}/batch/delete`, {
    method: 'POST',
    body,
  })
}

export async function startPipeline(pipelineId: string): Promise<PipelineStatusResult> {
  return requestJson<PipelineStatusResult>(`${PIPELINES_API_BASE}/${pipelineId}/start`, {
    method: 'POST',
    body: {},
  })
}

export async function stopPipeline(pipelineId: string): Promise<PipelineStatusResult> {
  return requestJson<PipelineStatusResult>(`${PIPELINES_API_BASE}/${pipelineId}/stop`, {
    method: 'POST',
    body: {},
  })
}

export async function getPipelineStatus(pipelineId: string): Promise<PipelineStatusResult> {
  return requestJson<PipelineStatusResult>(`${PIPELINES_API_BASE}/${pipelineId}/status`)
}

export async function getPipelineLogs(
  pipelineId: string,
  query: { tail?: number; offset?: number } = {},
): Promise<PipelineLogs> {
  const params = new URLSearchParams()
  if (query.tail != null) {
    params.set('tail', String(query.tail))
  }
  if (query.offset != null) {
    params.set('offset', String(query.offset))
  }
  const qs = params.toString()
  const url = qs
    ? `${PIPELINES_API_BASE}/${pipelineId}/logs?${qs}`
    : `${PIPELINES_API_BASE}/${pipelineId}/logs`
  return requestJson<PipelineLogs>(url)
}

export async function listGieTemplates(
  query: ListGieTemplatesQuery = {},
): Promise<PagedGieTemplates> {
  return requestJson<PagedGieTemplates>(
    `${GIE_TEMPLATES_API_BASE}/${pageQuery(query)}`,
  )
}

export async function getGieTemplate(gieId: string): Promise<GieTemplateListItem> {
  return requestJson<GieTemplateListItem>(`${GIE_TEMPLATES_API_BASE}/${gieId}`)
}

export async function createGieTemplate(body: CreateGieTemplateBody): Promise<GieTemplate> {
  return requestJson<GieTemplate>(`${GIE_TEMPLATES_API_BASE}/`, {
    method: 'POST',
    body,
  })
}

export async function updateGieTemplate(
  gieId: string,
  body: UpdateGieTemplateBody,
): Promise<GieTemplate> {
  return requestJson<GieTemplate>(`${GIE_TEMPLATES_API_BASE}/${gieId}`, {
    method: 'PUT',
    body,
  })
}

export async function deleteGieTemplate(gieId: string): Promise<null> {
  return requestJson<null>(`${GIE_TEMPLATES_API_BASE}/${gieId}`, {
    method: 'DELETE',
  })
}

export async function batchDeleteGieTemplates(
  body: BatchIdsBody,
): Promise<BatchDeleteResult> {
  return requestJson<BatchDeleteResult>(`${GIE_TEMPLATES_API_BASE}/batch/delete`, {
    method: 'POST',
    body,
  })
}

export async function listAnalyzerTemplates(
  query: ListAnalyzerTemplatesQuery = {},
): Promise<PagedAnalyzerTemplates> {
  return requestJson<PagedAnalyzerTemplates>(
    `${ANALYZER_TEMPLATES_API_BASE}/${pageQuery(query)}`,
  )
}

export async function getAnalyzerTemplate(analyzerId: string): Promise<AnalyzerTemplate> {
  return requestJson<AnalyzerTemplate>(`${ANALYZER_TEMPLATES_API_BASE}/${analyzerId}`)
}

export async function createAnalyzerTemplate(
  body: CreateAnalyzerTemplateBody,
): Promise<AnalyzerTemplate> {
  return requestJson<AnalyzerTemplate>(`${ANALYZER_TEMPLATES_API_BASE}/`, {
    method: 'POST',
    body,
  })
}

export async function updateAnalyzerTemplate(
  analyzerId: string,
  body: UpdateAnalyzerTemplateBody,
): Promise<AnalyzerTemplate> {
  return requestJson<AnalyzerTemplate>(
    `${ANALYZER_TEMPLATES_API_BASE}/${analyzerId}`,
    {
      method: 'PUT',
      body,
    },
  )
}

export async function deleteAnalyzerTemplate(analyzerId: string): Promise<null> {
  return requestJson<null>(`${ANALYZER_TEMPLATES_API_BASE}/${analyzerId}`, {
    method: 'DELETE',
  })
}

export async function batchDeleteAnalyzerTemplates(
  body: BatchIdsBody,
): Promise<BatchDeleteResult> {
  return requestJson<BatchDeleteResult>(
    `${ANALYZER_TEMPLATES_API_BASE}/batch/delete`,
    {
      method: 'POST',
      body,
    },
  )
}

export async function uploadAnalyzerSourceFile(
  analyzerId: string,
  file: File,
): Promise<AnalyzerSourceResult> {
  const form = new FormData()
  form.append('file', file)
  return requestJson<AnalyzerSourceResult>(
    `${ANALYZER_TEMPLATES_API_BASE}/${analyzerId}/source/file`,
    {
      method: 'POST',
      formData: form,
    },
  )
}

export async function captureAnalyzerSourceStream(
  analyzerId: string,
  body: CaptureAnalyzerSourceStreamBody,
  streamName = body.stream_id,
): Promise<AnalyzerSourceResult> {
  void streamName
  return requestJson<AnalyzerSourceResult>(
    `${ANALYZER_TEMPLATES_API_BASE}/${analyzerId}/source/stream`,
    {
      method: 'POST',
      body,
    },
  )
}
