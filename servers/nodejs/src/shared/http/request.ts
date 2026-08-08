import { ApiError, type ApiEnvelope } from '@/shared/http/client'

type RequestOptions = {
  method?: string
  body?: unknown
  headers?: Record<string, string>
  formData?: FormData
}

function buildInit(options: RequestOptions): RequestInit {
  const method = options.method ?? 'GET'
  const headers: Record<string, string> = { ...(options.headers ?? {}) }
  let body: BodyInit | undefined
  if (options.formData) {
    body = options.formData
  } else if (options.body !== undefined) {
    headers['Content-Type'] = headers['Content-Type'] ?? 'application/json'
    body = JSON.stringify(options.body)
  }
  return {
    method,
    credentials: 'include',
    headers,
    body,
  }
}

function readEnvelopeMessage(text: string): string | null {
  let message: string | null = null
  if (text.trim().startsWith('{')) {
    const envelope = JSON.parse(text) as Partial<ApiEnvelope<unknown>>
    if (typeof envelope.message === 'string' && envelope.message) {
      message = envelope.message
    }
  }
  return message
}

export async function requestJson<T>(
  url: string,
  options: RequestOptions = {},
): Promise<T> {
  const response = await fetch(url, buildInit(options))
  const text = await response.text()
  let envelope: ApiEnvelope<T> | null = null
  if (text.trim().startsWith('{')) {
    envelope = JSON.parse(text) as ApiEnvelope<T>
  }
  if (!response.ok) {
    const message =
      envelope?.message ||
      response.statusText ||
      `HTTP ${response.status}`
    throw new ApiError(response.status, message)
  }
  if (!envelope) {
    throw new ApiError(response.status, 'Empty response')
  }
  if (!envelope.success) {
    throw new ApiError(response.status || 400, envelope.message || 'Request failed')
  }
  return envelope.data
}

export async function requestBlob(
  url: string,
  options: RequestOptions = {},
): Promise<Blob> {
  const response = await fetch(url, buildInit(options))
  if (!response.ok) {
    const text = await response.text()
    const message =
      readEnvelopeMessage(text) ||
      text ||
      response.statusText ||
      'Request failed'
    throw new ApiError(response.status, message)
  }
  return response.blob()
}
