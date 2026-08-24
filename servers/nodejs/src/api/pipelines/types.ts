export type PipelineStatus =
  | 'stopped'
  | 'starting'
  | 'online'
  | 'running'
  | 'stopping'
  | 'error'
  | 'offline'

export type DebouncerMode = 'slide' | 'fold'
export type AnnotationType =
  | 'roi_filtering'
  | 'overcrowding'
  | 'line_crossing'
  | 'direction_detection'
export type AnnotationShape = 'rectangle' | 'polygon' | 'line_direction' | 'direction'
export type SourceKind = 'file' | 'stream'
export type LineMode = 'strict' | 'balanced' | 'loose'

export type Parser = Record<string, unknown>

export type Drawer = {
  show_label: boolean
  show_conf: boolean
  show_id: boolean
  interval: number
  fade_time: number
}

export type Debouncer = {
  length: number
  threshold: number
  class_ids: number[]
  mode: DebouncerMode
}

export type Tracker = {
  class_id: number | number[]
}

export type SahiConfig = {
  nvsahipreprocess: {
    slice_width: number
    slice_height: number
    overlap_width_ratio: number
    overlap_height_ratio: number
  }
  nvsahipostprocess: {
    match_threshold: number
  }
}

export type PipelineAnalyzer = {
  streams: string[]
  template: string
  roi_filtering: { class_id: number | number[] } | null
  overcrowding: { class_id: number | number[] } | null
  line_crossing: { class_id: number | number[] } | null
  direction_detection: { class_id: number | number[] } | null
}

export type Pipeline = {
  id: string
  name: string
  type: string
  status: PipelineStatus
  drawer: Drawer
  parser: Parser
  logger: { interval: number }
  messager: { interval: number }
  debouncer: Debouncer | null
  streams: string[]
  gie_id: string
  interval: number
  tracker: Tracker | null
  analyzer: PipelineAnalyzer | null
  sahi: SahiConfig | null
  input: string | null
  output: string | null
  last_refresh_at: string | null
  created_at: string | null
  updated_at: string | null
}

export type PipelineListItem = Pick<
  Pipeline,
  'id' | 'name' | 'type' | 'status' | 'last_refresh_at' | 'updated_at'
>

export type SchemaField = {
  available: boolean
  default: unknown
}

export type SchemaBlock = {
  available: boolean
  params?: Record<string, SchemaField | SchemaBlock>
  default?: unknown
}

export type TypeSchema = {
  pipeline: {
    type: string
    params: {
      parser: SchemaBlock
      drawer: SchemaBlock
      logger: SchemaBlock
      messager: SchemaBlock
      debouncer: SchemaBlock
    }
  }
  generator: {
    type: string
    params: {
      streams: SchemaBlock
      pgie: SchemaBlock
      interval: SchemaBlock
      tracker: SchemaBlock
      analyzer: SchemaBlock
      sahi: SchemaBlock
      input: SchemaBlock
      output: SchemaBlock
    }
  }
}

export type ClassAttrRow = {
  class: string | number
  conf: number
  topk: number
  detected_min_w: number
  detected_min_h: number
  detected_max_w: number
  detected_max_h: number
}

export type PipelineRef = {
  id: string
  name: string
}

export type GieTemplate = {
  id: string
  name: string
  model_id: string
  model_name: string | null
  class_attrs: ClassAttrRow[]
  class_attrs_summary: string | null
  created_at: string | null
  updated_at: string | null
}

/** List/detail API shape: persisted template + pipelines that reference it. */
export type GieTemplateListItem = GieTemplate & {
  pipelines: PipelineRef[]
}

export type Annotation = {
  id: string
  name: string
  type: AnnotationType
  shape: AnnotationShape
  points: number[]
  click_points: number[] | null
  object_threshold: number | null
  time_threshold: number | null
  extend: boolean | null
  mode: LineMode | null
}

export type AnalyzerTemplate = {
  id: string
  name: string
  source_kind: SourceKind
  source_stream_id: string | null
  source_file_name: string | null
  source_image_url: string
  config_width: number
  config_height: number
  captured_at: string | null
  annotations: Annotation[]
}

export type AnalyzerTemplateListItem = {
  id: string
  name: string
  roi_filtering_count: number
  overcrowding_count: number
  line_crossing_count: number
  direction_detection_count: number
  pipelines: PipelineRef[]
  updated_at: string | null
}

export type ListPipelinesQuery = {
  search?: string
  page?: number
  page_size?: number
}

export type PagedPipelines = {
  items: PipelineListItem[]
  total: number
  page: number
  page_size: number
}

export type PipelineTypeItem = {
  type: string
}

export type PipelineTypesResult = {
  items: PipelineTypeItem[]
}

export type PipelineSchemaBody = {
  pipeline_type: string
}

export type NameMap = Record<string, string>

export type CreatePipelineBody = Omit<
  Pipeline,
  'id' | 'status' | 'created_at' | 'updated_at'
>

export type UpdatePipelineBody = CreatePipelineBody

export type BatchIdsBody = {
  ids: string[]
}

export type BatchDeleteResult = {
  deleted_count: number
  failed_ids: string[]
}

export type PipelineStatusResult = {
  id: string
  status: PipelineStatus
  message: string
  last_refresh_at?: string | null
}

export type PipelineLogs = {
  lines: string[]
  updated_at: string
}

export type ListGieTemplatesQuery = {
  search?: string
  page?: number
  page_size?: number
}

export type PagedGieTemplates = {
  items: GieTemplateListItem[]
  total: number
  page: number
  page_size: number
}

export type CreateGieTemplateBody = {
  name: string
  model_id: string
  class_attrs: ClassAttrRow[]
}

export type UpdateGieTemplateBody = CreateGieTemplateBody

export type ListAnalyzerTemplatesQuery = {
  search?: string
  page?: number
  page_size?: number
}

export type PagedAnalyzerTemplates = {
  items: AnalyzerTemplateListItem[]
  total: number
  page: number
  page_size: number
}

export type CreateAnalyzerTemplateBody = Omit<AnalyzerTemplate, 'id'>

export type UpdateAnalyzerTemplateBody = CreateAnalyzerTemplateBody

export type AnalyzerSourceResult = {
  source_kind: SourceKind
  source_stream_id?: string | null
  source_file_name?: string | null
  source_image_url: string
  config_width: number
  config_height: number
  captured_at: string
}

export type CaptureAnalyzerSourceStreamBody = {
  stream_id: string
}
