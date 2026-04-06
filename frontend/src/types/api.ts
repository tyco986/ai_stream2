export interface ApiResponse<T> {
  code: string
  message: string
  data: T
}

export interface PaginatedResponse<T> {
  count: number
  results: T[]
  next: string | null
  previous: string | null
}
