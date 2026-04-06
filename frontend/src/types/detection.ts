export interface Detection {
  id: string
  camera: string
  camera_name: string
  object_type: string
  confidence: number
  bbox: { x: number; y: number; w: number; h: number }
  tracker_id: number | null
  analytics_data: Record<string, any> | null
  frame_number: number
  detected_at: string
}
