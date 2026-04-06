export interface Camera {
  id: string
  uid: string
  name: string
  rtsp_url: string
  organization: string
  group: string | null
  status: 'offline' | 'connecting' | 'online' | 'error'
  pipeline_profile: string | null
  config: Record<string, any>
  created_at: string
  updated_at: string
}

export interface CameraCreate {
  name: string
  rtsp_url: string
  group?: string
}

export interface CameraGroup {
  id: string
  name: string
  created_at: string
}
