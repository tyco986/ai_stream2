export type ModelType = 'detector' | 'tracker'

export interface AIModel {
  id: string
  name: string
  model_type: ModelType
  framework: 'onnx' | 'engine' | 'custom'
  model_file: string
  label_file: string | null
  config: Record<string, any>
  version: string
  description: string
  is_active: boolean
  created_at: string
  updated_at: string
}

export interface AIModelCreate {
  name: string
  model_type: ModelType
  framework: 'onnx' | 'engine' | 'custom'
  model_file: string
  label_file?: string
  config: Record<string, any>
  version: string
  description?: string
}

export interface PipelineProfile {
  id: string
  name: string
  description: string
  detector: AIModel
  tracker: AIModel | null
  analytics_enabled: boolean
  analytics_config_stale: boolean
  is_active: boolean
  created_at: string
  updated_at: string
}

export interface PipelineProfileCreate {
  name: string
  description?: string
  detector_id: string
  tracker_id?: string
  analytics_enabled?: boolean
}
