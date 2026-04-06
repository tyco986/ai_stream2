export type RecordingType = 'rolling' | 'event' | 'manual'

export interface Recording {
  id: string
  camera: string
  camera_name: string
  recording_type: RecordingType
  file_path: string
  duration_seconds: number
  file_size_bytes: number
  started_at: string
  ended_at: string
  created_at: string
}
