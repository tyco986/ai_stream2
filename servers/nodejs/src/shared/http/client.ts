/**
 * Shell HTTP helpers.
 * Replace call sites with OpenAPI-generated clients when available.
 */

export type ApiEnvelope<T> = {
  success: boolean
  message: string
  data: T
}

export class ApiError extends Error {
  status: number

  constructor(status: number, message: string) {
    super(message)
    this.status = status
    this.name = 'ApiError'
  }
}

export function unwrapData<T>(envelope: ApiEnvelope<T>): T {
  if (!envelope.success) {
    throw new ApiError(400, envelope.message || 'Request failed')
  }
  return envelope.data
}
