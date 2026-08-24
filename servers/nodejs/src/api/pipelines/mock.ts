/**
 * Pipelines API — mock.
 */

import { buildClassAttrsSummary as formatClassAttrsSummary } from '@/shared/gie/classAttrs'
import { ApiError } from '@/shared/http/client'
import { BACKEND_PREFIX, STORAGE_KEY_PREFIX } from '@/shared/project'
import {
  previewAnalyzerSourceFile,
  previewAnalyzerSourceStream,
} from '@/api/pipelines/utils'
import type {
  PipelineStatus,
  DebouncerMode,
  AnnotationType,
  AnnotationShape,
  SourceKind,
  LineMode,
  Drawer,
  Debouncer,
  Tracker,
  PipelineAnalyzer,
  Pipeline,
  PipelineListItem,
  SchemaField,
  SchemaBlock,
  TypeSchema,
  ClassAttrRow,
  PipelineRef,
  GieTemplate,
  GieTemplateListItem,
  Annotation,
  AnalyzerTemplate,
  AnalyzerTemplateListItem,
  ListPipelinesQuery,
  PagedPipelines,
  PipelineTypeItem,
  PipelineTypesResult,
  PipelineSchemaBody,
  NameMap,
  CreatePipelineBody,
  UpdatePipelineBody,
  BatchIdsBody,
  BatchDeleteResult,
  PipelineStatusResult,
  PipelineLogs,
  ListGieTemplatesQuery,
  PagedGieTemplates,
  CreateGieTemplateBody,
  UpdateGieTemplateBody,
  ListAnalyzerTemplatesQuery,
  PagedAnalyzerTemplates,
  CreateAnalyzerTemplateBody,
  UpdateAnalyzerTemplateBody,
  AnalyzerSourceResult,
  CaptureAnalyzerSourceStreamBody,
} from '@/api/pipelines/types'

export const PIPELINES_API_BASE = `${BACKEND_PREFIX}/pipelines`
export const GIE_TEMPLATES_API_BASE = `${BACKEND_PREFIX}/gie-templates`
export const ANALYZER_TEMPLATES_API_BASE = `${BACKEND_PREFIX}/analyzer-templates`

const STORAGE_PIPELINES = `${STORAGE_KEY_PREFIX}pipelines.items`
const STORAGE_GIE = `${STORAGE_KEY_PREFIX}pipelines.gie`
const STORAGE_ANALYZER = `${STORAGE_KEY_PREFIX}pipelines.analyzer`
const STORAGE_LOGS = `${STORAGE_KEY_PREFIX}pipelines.logs`

const SEED_GIE_ID = '9a1b2c3d-4567-89ab-cdef-0123456789ab'
const SEED_ANALYZER_ID = 'a1b2c3d4-e5f6-4789-a012-3456789abcde'
const SEED_PIPELINE_DET_ID = '550e8400-e29b-41d4-a716-446655440000'
const SEED_PIPELINE_PRESENCE_ID = '8f3a2b1c-1234-4cde-9abc-def012345678'
const BUILT_MODEL_ID = '550e8400-e29b-41d4-a716-446655440000'
const CAM01 = 'c3000001-0000-4000-8000-000000000001'
const CAM02 = 'c3000002-0000-4000-8000-000000000002'

const PIPELINE_TYPES = [
  'DetVisRTSPPipeline',
  'SegVisRTSPPipeline',
  'DetImagePipeline',
  'SegImagePipeline',
  'DetVideoPipeline',
  'SegVideoPipeline',
  'DetSahiVisRTSPPipeline',
  'DetSahiImagePipeline',
  'DetSahiVideoPipeline',
  'SegSahiVisRTSPPipeline',
  'SegSahiImagePipeline',
  'SegSahiVideoPipeline',
  'PresenceRTSPPipeline',
  'PresenceVideoPipeline',
] as const

type Store = {
  pipelines: Pipeline[]
  gieTemplates: GieTemplate[]
  analyzerTemplates: AnalyzerTemplate[]
  logs: Record<string, string[]>
}

const startTimers = new Map<string, ReturnType<typeof setTimeout>>()
const stopTimers = new Map<string, ReturnType<typeof setTimeout>>()

function newId(): string {
  return crypto.randomUUID()
}

function nowIso(): string {
  return new Date().toISOString()
}

function schemaField(available: boolean, defaultValue: unknown): SchemaField {
  return { available, default: defaultValue }
}

function parserBlock(available: boolean): SchemaBlock {
  return {
    available,
    params: {},
  }
}

function drawerBlock(available: boolean): SchemaBlock {
  return {
    available,
    params: {
      show_label: schemaField(available, true),
      show_conf: schemaField(available, true),
      show_id: schemaField(available, false),
      interval: schemaField(available, 50),
      fade_time: schemaField(available, 1),
    },
  }
}

function usesParser(pipelineType: string): boolean {
  const parserType =
    !pipelineType.includes('Vis') &&
    !pipelineType.includes('Image') &&
    !pipelineType.startsWith('Presence')
  return parserType
}

function toolParams(
  pipelineType: string,
  logger: SchemaBlock,
  messager: SchemaBlock,
  debouncer: SchemaBlock,
) {
  const parser = usesParser(pipelineType)
  const params = {
    parser: parserBlock(parser),
    drawer: drawerBlock(!parser),
    logger,
    messager,
    debouncer,
  }
  return params
}

function loggerBlock(available: boolean, interval = 50): SchemaBlock {
  return {
    available,
    params: {
      interval: schemaField(available, interval),
    },
  }
}

function messagerBlock(available: boolean, interval = 0): SchemaBlock {
  return {
    available,
    params: {
      interval: schemaField(available, interval),
    },
  }
}

function debouncerBlock(available: boolean): SchemaBlock {
  return {
    available,
    params: {
      length: schemaField(available, 10),
      threshold: schemaField(available, 0.5),
      class_ids: schemaField(available, [0, 1]),
      mode: schemaField(available, 'fold'),
    },
  }
}

function streamsBlock(available: boolean): SchemaBlock {
  return {
    available,
    params: {},
  }
}

function pgieBlock(available: boolean): SchemaBlock {
  return {
    available,
    params: {},
  }
}

function trackerBlock(available: boolean): SchemaBlock {
  return {
    available,
    params: {
      class_id: schemaField(available, [-1]),
    },
  }
}

function analyzerBlock(available: boolean): SchemaBlock {
  return {
    available,
    params: {
      streams: schemaField(available, []),
      template: schemaField(available, null),
      roi_filtering: schemaField(available, { class_id: [-1] }),
      overcrowding: schemaField(available, null),
      line_crossing: schemaField(available, null),
      direction_detection: schemaField(available, null),
    },
  }
}

function sahiBlock(available: boolean): SchemaBlock {
  if (!available) {
    return { available: false }
  }
  return {
    available: true,
    params: {
      nvsahipreprocess: schemaField(true, {
        slice_width: 640,
        slice_height: 640,
        overlap_width_ratio: 0.2,
        overlap_height_ratio: 0.2,
      }),
      nvsahipostprocess: schemaField(true, {
        match_threshold: 0.5,
      }),
    },
  }
}

function ioField(available: boolean): SchemaBlock {
  if (!available) {
    return { available: false }
  }
  return { available: true, default: null }
}

function buildGeneratorParams(options: {
  streams: boolean
  interval: number
  tracker: boolean
  analyzer: boolean
  sahi: boolean
  io: boolean
}): TypeSchema['generator']['params'] {
  return {
    streams: streamsBlock(options.streams),
    pgie: pgieBlock(true),
    interval: { available: true, default: options.interval },
    tracker: trackerBlock(options.tracker),
    analyzer: analyzerBlock(options.analyzer),
    sahi: sahiBlock(options.sahi),
    input: ioField(options.io),
    output: ioField(options.io),
  }
}

function buildRtspTypeSchema(pipelineType: string, generatorType: string): TypeSchema {
  const sahi = pipelineType.includes('Sahi')
  return {
    pipeline: {
      type: pipelineType,
      params: toolParams(
        pipelineType,
        loggerBlock(true),
        messagerBlock(true),
        debouncerBlock(false),
      ),
    },
    generator: {
      type: generatorType,
      params: buildGeneratorParams({
        streams: true,
        interval: 50,
        tracker: true,
        analyzer: true,
        sahi,
        io: false,
      }),
    },
  }
}

function buildFileTypeSchema(
  pipelineType: string,
  generatorType: string,
  options: { interval?: number; tracker?: boolean; analyzer?: boolean; sahi?: boolean } = {},
): TypeSchema {
  const sahi = options.sahi ?? pipelineType.includes('Sahi')
  const tracker = options.tracker ?? !pipelineType.includes('Image')
  const analyzer = options.analyzer ?? true
  return {
    pipeline: {
      type: pipelineType,
      params: toolParams(
        pipelineType,
        loggerBlock(true, options.interval ?? 50),
        messagerBlock(true, 0),
        debouncerBlock(false),
      ),
    },
    generator: {
      type: generatorType,
      params: buildGeneratorParams({
        streams: false,
        interval: options.interval ?? 50,
        tracker,
        analyzer,
        sahi,
        io: true,
      }),
    },
  }
}

function buildPresenceVideoTypeSchema(): TypeSchema {
  return {
    pipeline: {
      type: 'PresenceVideoPipeline',
      params: toolParams(
        'PresenceVideoPipeline',
        loggerBlock(true, 0),
        messagerBlock(true, 0),
        debouncerBlock(true),
      ),
    },
    generator: {
      type: 'DetVideoPresenceGenerator',
      params: buildGeneratorParams({
        streams: false,
        interval: 0,
        tracker: false,
        analyzer: false,
        sahi: false,
        io: true,
      }),
    },
  }
}

function buildTypeSchema(pipelineType: string): TypeSchema {
  let schema: TypeSchema | null = null
  if (pipelineType === 'DetVisRTSPPipeline') {
    schema = buildRtspTypeSchema('DetVisRTSPPipeline', 'DetVisRTSPGenerator')
  } else if (pipelineType === 'SegVisRTSPPipeline') {
    schema = buildRtspTypeSchema('SegVisRTSPPipeline', 'SegVisRTSPGenerator')
  } else if (pipelineType === 'DetSahiVisRTSPPipeline') {
    schema = buildRtspTypeSchema('DetSahiVisRTSPPipeline', 'DetSahiVisRTSPGenerator')
  } else if (pipelineType === 'SegSahiVisRTSPPipeline') {
    schema = buildRtspTypeSchema('SegSahiVisRTSPPipeline', 'SegSahiVisRTSPGenerator')
  } else if (pipelineType === 'DetImagePipeline') {
    schema = buildFileTypeSchema('DetImagePipeline', 'DetImageGenerator', { tracker: false })
  } else if (pipelineType === 'SegImagePipeline') {
    schema = buildFileTypeSchema('SegImagePipeline', 'SegImageGenerator', { tracker: false })
  } else if (pipelineType === 'DetSahiImagePipeline') {
    schema = buildFileTypeSchema('DetSahiImagePipeline', 'DetSahiImageGenerator', {
      tracker: false,
      sahi: true,
    })
  } else if (pipelineType === 'SegSahiImagePipeline') {
    schema = buildFileTypeSchema('SegSahiImagePipeline', 'SegSahiImageGenerator', {
      tracker: false,
      sahi: true,
    })
  } else if (pipelineType === 'DetVideoPipeline') {
    schema = buildFileTypeSchema('DetVideoPipeline', 'DetVideoGenerator')
  } else if (pipelineType === 'SegVideoPipeline') {
    schema = buildFileTypeSchema('SegVideoPipeline', 'SegVideoGenerator')
  } else if (pipelineType === 'DetSahiVideoPipeline') {
    schema = buildFileTypeSchema('DetSahiVideoPipeline', 'DetSahiVideoGenerator', { sahi: true })
  } else if (pipelineType === 'SegSahiVideoPipeline') {
    schema = buildFileTypeSchema('SegSahiVideoPipeline', 'SegSahiVideoGenerator', { sahi: true })
  } else if (pipelineType === 'PresenceVideoPipeline') {
    schema = buildPresenceVideoTypeSchema()
  } else if (pipelineType === 'PresenceRTSPPipeline') {
    schema = {
      pipeline: {
        type: 'PresenceRTSPPipeline',
        params: toolParams(
          'PresenceRTSPPipeline',
          loggerBlock(true, 0),
          messagerBlock(true, 0),
          debouncerBlock(true),
        ),
      },
      generator: {
        type: 'DetVisRTSPPresenceGenerator',
        params: buildGeneratorParams({
          streams: true,
          interval: 0,
          tracker: false,
          analyzer: false,
          sahi: false,
          io: false,
        }),
      },
    }
  }
  if (!schema) {
    throw new ApiError(404, `Unknown pipeline type: ${pipelineType}`)
  }
  return schema
}

function buildClassAttrsSummary(rows: ClassAttrRow[]): string {
  return formatClassAttrsSummary(rows)
}

function defaultClassAttrRow(): ClassAttrRow {
  return {
    class: 'all',
    conf: 0.25,
    topk: 300,
    detected_min_w: -1,
    detected_min_h: -1,
    detected_max_w: -1,
    detected_max_h: -1,
  }
}

function seedGieTemplate(): GieTemplate {
  const classAttrs = [defaultClassAttrRow()]
  const createdAt = '2026-07-01T10:00:00Z'
  return {
    id: SEED_GIE_ID,
    name: 'yolo26n-default',
    model_id: BUILT_MODEL_ID,
    model_name: 'yolo26n',
    class_attrs: classAttrs,
    class_attrs_summary: buildClassAttrsSummary(classAttrs),
    created_at: createdAt,
    updated_at: '2026-07-18T02:00:00Z',
  }
}

function seedAnalyzerTemplate(): AnalyzerTemplate {
  return {
    id: SEED_ANALYZER_ID,
    name: 'entrance_default',
    source_kind: 'stream',
    source_stream_id: CAM01,
    source_file_name: null,
    source_image_url: '/media/analyzer-templates/entrance_default/snapshot.jpg',
    config_width: 1920,
    config_height: 1080,
    captured_at: '2026-07-02T10:15:00Z',
    annotations: [],
  }
}

function seedDetPipeline(): Pipeline {
  const createdAt = '2026-07-01T10:00:00Z'
  return {
    id: SEED_PIPELINE_DET_ID,
    name: 'yolo26n-det-rtsp',
    type: 'DetVisRTSPPipeline',
    status: 'running',
    parser: {},
    drawer: {
      show_label: true,
      show_conf: true,
      show_id: false,
      interval: 50,
      fade_time: 1,
    },
    logger: { interval: 50 },
    messager: { interval: 0 },
    debouncer: null,
    streams: [CAM01, CAM02],
    gie_id: SEED_GIE_ID,
    interval: 50,
    tracker: { class_id: [0] },
    analyzer: {
      streams: [CAM01],
      template: SEED_ANALYZER_ID,
      roi_filtering: { class_id: [-1] },
      overcrowding: { class_id: [0] },
      line_crossing: { class_id: [0, 2] },
      direction_detection: null,
    },
    sahi: null,
    input: null,
    output: null,
    last_refresh_at: '2026-07-18T02:00:00Z',
    created_at: createdAt,
    updated_at: '2026-07-18T02:00:00Z',
  }
}

function seedPresencePipeline(): Pipeline {
  const createdAt = '2026-07-05T08:00:00Z'
  return {
    id: SEED_PIPELINE_PRESENCE_ID,
    name: 'smoke-fire',
    type: 'PresenceVideoPipeline',
    status: 'stopped',
    parser: {},
    drawer: {
      show_label: true,
      show_conf: true,
      show_id: false,
      interval: 0,
      fade_time: 0,
    },
    logger: { interval: 0 },
    messager: { interval: 0 },
    debouncer: {
      length: 10,
      threshold: 0.5,
      class_ids: [0, 1],
      mode: 'fold',
    },
    streams: [],
    gie_id: SEED_GIE_ID,
    interval: 0,
    tracker: null,
    analyzer: null,
    sahi: null,
    input: '/data/smoke.mp4',
    output: '/data/out/smoke',
    last_refresh_at: null,
    created_at: createdAt,
    updated_at: '2026-07-17T12:00:00Z',
  }
}

function seedStore(): Store {
  const detPipeline = seedDetPipeline()
  const presencePipeline = seedPresencePipeline()
  return {
    pipelines: [detPipeline, presencePipeline],
    gieTemplates: [seedGieTemplate()],
    analyzerTemplates: [seedAnalyzerTemplate()],
    logs: {
      [detPipeline.id]: [
        '2026-07-18 02:00:00 INFO pipeline started',
        '2026-07-18 02:00:01 INFO deepstream running',
        '2026-07-18 02:00:02 INFO stream cam-01 online',
      ],
      [presencePipeline.id]: [
        '2026-07-17 12:00:00 INFO pipeline stopped',
      ],
    },
  }
}

function readStore(): Store {
  const raw = sessionStorage.getItem(STORAGE_PIPELINES)
  const gieRaw = sessionStorage.getItem(STORAGE_GIE)
  const analyzerRaw = sessionStorage.getItem(STORAGE_ANALYZER)
  const logsRaw = sessionStorage.getItem(STORAGE_LOGS)
  if (!raw) {
    const seed = seedStore()
    writeStore(seed)
    return seed
  }
  return {
    pipelines: JSON.parse(raw) as Pipeline[],
    gieTemplates: gieRaw ? (JSON.parse(gieRaw) as GieTemplate[]) : [],
    analyzerTemplates: analyzerRaw ? (JSON.parse(analyzerRaw) as AnalyzerTemplate[]) : [],
    logs: logsRaw ? (JSON.parse(logsRaw) as Record<string, string[]>) : {},
  }
}

function writeStore(store: Store) {
  sessionStorage.setItem(STORAGE_PIPELINES, JSON.stringify(store.pipelines))
  sessionStorage.setItem(STORAGE_GIE, JSON.stringify(store.gieTemplates))
  sessionStorage.setItem(STORAGE_ANALYZER, JSON.stringify(store.analyzerTemplates))
  sessionStorage.setItem(STORAGE_LOGS, JSON.stringify(store.logs))
}

function findPipeline(store: Store, pipelineId: string): Pipeline {
  const pipeline = store.pipelines.find((item) => item.id === pipelineId)
  if (!pipeline) {
    throw new ApiError(404, 'Pipeline not found')
  }
  return pipeline
}

function findGieTemplate(store: Store, gieId: string): GieTemplate {
  const template = store.gieTemplates.find((item) => item.id === gieId)
  if (!template) {
    throw new ApiError(404, 'GIE template not found')
  }
  return template
}

function findAnalyzerTemplate(store: Store, analyzerId: string): AnalyzerTemplate {
  const template = store.analyzerTemplates.find((item) => item.id === analyzerId)
  if (!template) {
    throw new ApiError(404, 'Analyzer template not found')
  }
  return template
}

function toListItem(pipeline: Pipeline): PipelineListItem {
  return {
    id: pipeline.id,
    name: pipeline.name,
    type: pipeline.type,
    status: pipeline.status,
    last_refresh_at: pipeline.last_refresh_at,
    updated_at: pipeline.updated_at,
  }
}

function validatePipelineName(store: Store, name: string, excludeId?: string) {
  const trimmed = name.trim()
  if (!trimmed) {
    throw new ApiError(400, 'name is required')
  }
  const duplicate = store.pipelines.some(
    (item) => item.name === trimmed && item.id !== excludeId,
  )
  if (duplicate) {
    throw new ApiError(409, 'Pipeline name already exists')
  }
}

function validateGieName(store: Store, name: string, excludeId?: string) {
  const trimmed = name.trim()
  if (!trimmed) {
    throw new ApiError(400, 'name is required')
  }
  const duplicate = store.gieTemplates.some(
    (item) => item.name === trimmed && item.id !== excludeId,
  )
  if (duplicate) {
    throw new ApiError(409, 'GIE template name already exists')
  }
}

function validateAnalyzerName(store: Store, name: string, excludeId?: string) {
  const trimmed = name.trim()
  if (!trimmed) {
    throw new ApiError(400, 'name is required')
  }
  const duplicate = store.analyzerTemplates.some(
    (item) => item.name === trimmed && item.id !== excludeId,
  )
  if (duplicate) {
    throw new ApiError(409, 'Analyzer template name already exists')
  }
}

function validateClassAttrs(rows: ClassAttrRow[]) {
  if (!rows.length) {
    throw new ApiError(400, 'class_attrs must not be empty')
  }
  const classes = rows.map((row) => String(row.class))
  const unique = new Set(classes)
  if (unique.size !== classes.length) {
    throw new ApiError(400, 'class_attrs class values must be unique')
  }
}

function pipelineRefs(
  store: Store,
  match: (pipeline: Pipeline) => boolean,
): PipelineRef[] {
  return store.pipelines
    .filter(match)
    .map((item) => ({ id: item.id, name: item.name }))
}

function isPipelineReferenced(store: Store, gieId: string): boolean {
  return pipelineRefs(store, (item) => item.gie_id === gieId).length > 0
}

function toGieListItem(store: Store, template: GieTemplate): GieTemplateListItem {
  return {
    ...template,
    pipelines: pipelineRefs(store, (item) => item.gie_id === template.id),
  }
}

function isAnalyzerReferenced(store: Store, analyzerId: string): boolean {
  return pipelineRefs(store, (item) => item.analyzer?.template === analyzerId).length > 0
}

function assertNotRunning(pipeline: Pipeline, action: string) {
  if (
    pipeline.status === 'online' ||
    pipeline.status === 'running' ||
    pipeline.status === 'starting'
  ) {
    throw new ApiError(409, `Cannot ${action} while pipeline is started`)
  }
}

function clearPipelineTimer(map: Map<string, ReturnType<typeof setTimeout>>, pipelineId: string) {
  const timer = map.get(pipelineId)
  if (timer) {
    clearTimeout(timer)
    map.delete(pipelineId)
  }
}

function finishStart(pipelineId: string) {
  const store = readStore()
  const pipeline = findPipeline(store, pipelineId)
  pipeline.status = 'running'
  pipeline.updated_at = nowIso()
  const lines = store.logs[pipelineId] ?? []
  lines.push(`${nowIso()} INFO pipeline running`)
  store.logs[pipelineId] = lines
  writeStore(store)
  startTimers.delete(pipelineId)
}

function finishStop(pipelineId: string) {
  const store = readStore()
  const pipeline = findPipeline(store, pipelineId)
  pipeline.status = 'stopped'
  pipeline.updated_at = nowIso()
  const lines = store.logs[pipelineId] ?? []
  lines.push(`${nowIso()} INFO pipeline stopped`)
  store.logs[pipelineId] = lines
  writeStore(store)
  stopTimers.delete(pipelineId)
}

function toAnalyzerListItem(
  store: Store,
  template: AnalyzerTemplate,
): AnalyzerTemplateListItem {
  const counts = {
    roi_filtering_count: 0,
    overcrowding_count: 0,
    line_crossing_count: 0,
    direction_detection_count: 0,
  }
  for (const annotation of template.annotations) {
    if (annotation.type === 'roi_filtering') {
      counts.roi_filtering_count += 1
    } else if (annotation.type === 'overcrowding') {
      counts.overcrowding_count += 1
    } else if (annotation.type === 'line_crossing') {
      counts.line_crossing_count += 1
    } else if (annotation.type === 'direction_detection') {
      counts.direction_detection_count += 1
    }
  }
  return {
    id: template.id,
    name: template.name,
    roi_filtering_count: counts.roi_filtering_count,
    overcrowding_count: counts.overcrowding_count,
    line_crossing_count: counts.line_crossing_count,
    direction_detection_count: counts.direction_detection_count,
    pipelines: pipelineRefs(store, (item) => item.analyzer?.template === template.id),
    updated_at: template.captured_at,
  }
}

function applyGieTemplateBody(
  template: GieTemplate,
  body: CreateGieTemplateBody,
  modelName: string | null,
) {
  template.name = body.name.trim()
  template.model_id = body.model_id
  template.model_name = modelName
  template.class_attrs = body.class_attrs
  template.class_attrs_summary = buildClassAttrsSummary(body.class_attrs)
  template.updated_at = nowIso()
}

function paginate<T>(items: T[], page: number, pageSize: number) {
  const total = items.length
  const start = (page - 1) * pageSize
  return {
    items: items.slice(start, start + pageSize),
    total,
    page,
    page_size: pageSize,
  }
}

export async function getPipelinesMap(): Promise<NameMap> {
  const store = readStore()
  return Object.fromEntries(store.pipelines.map((item) => [item.id, item.name]))
}

export async function listPipelines(
  query: ListPipelinesQuery = {},
): Promise<PagedPipelines> {
  const store = readStore()
  const page = query.page ?? 1
  const pageSize = query.page_size ?? 20
  const search = query.search?.trim().toLowerCase() ?? ''
  let items = store.pipelines.map(toListItem)
  if (search) {
    items = items.filter((item) => item.name.toLowerCase().includes(search))
  }
  return paginate(items, page, pageSize)
}

export async function getPipelineTypes(): Promise<PipelineTypesResult> {
  return {
    items: PIPELINE_TYPES.map((type) => ({ type })),
  }
}

export async function getPipelineSchema(body: PipelineSchemaBody): Promise<TypeSchema> {
  const pipelineType = body.pipeline_type?.trim()
  if (!pipelineType) {
    throw new ApiError(400, 'pipeline_type is required')
  }
  return buildTypeSchema(pipelineType)
}

export async function getPipeline(pipelineId: string): Promise<Pipeline> {
  return findPipeline(readStore(), pipelineId)
}

export async function createPipeline(body: CreatePipelineBody): Promise<Pipeline> {
  const store = readStore()
  validatePipelineName(store, body.name)
  if (!PIPELINE_TYPES.includes(body.type as (typeof PIPELINE_TYPES)[number])) {
    throw new ApiError(400, 'Invalid pipeline type')
  }
  findGieTemplate(store, body.gie_id)
  const createdAt = nowIso()
  const pipeline: Pipeline = {
    id: newId(),
    name: body.name.trim(),
    type: body.type,
    status: 'stopped',
    parser: body.parser,
    drawer: body.drawer,
    logger: body.logger,
    messager: body.messager,
    debouncer: body.debouncer,
    streams: body.streams,
    gie_id: body.gie_id,
    interval: body.interval,
    tracker: body.tracker,
    analyzer: body.analyzer,
    sahi: body.sahi,
    input: body.input,
    output: body.output,
    last_refresh_at: null,
    created_at: createdAt,
    updated_at: createdAt,
  }
  store.pipelines.unshift(pipeline)
  store.logs[pipeline.id] = [`${nowIso()} created`]
  writeStore(store)
  return pipeline
}

export async function updatePipeline(
  pipelineId: string,
  body: UpdatePipelineBody,
): Promise<Pipeline> {
  const store = readStore()
  const pipeline = findPipeline(store, pipelineId)
  if (pipeline.status === 'online' || pipeline.status === 'running' || pipeline.status === 'starting') {
    throw new ApiError(409, 'Stop pipeline before editing')
  }
  validatePipelineName(store, body.name, pipelineId)
  if (!PIPELINE_TYPES.includes(body.type as (typeof PIPELINE_TYPES)[number])) {
    throw new ApiError(400, 'Invalid pipeline type')
  }
  findGieTemplate(store, body.gie_id)
  pipeline.name = body.name.trim()
  pipeline.type = body.type
  pipeline.parser = body.parser
  pipeline.drawer = body.drawer
  pipeline.logger = body.logger
  pipeline.messager = body.messager
  pipeline.debouncer = body.debouncer
  pipeline.streams = body.streams
  pipeline.gie_id = body.gie_id
  pipeline.interval = body.interval
  pipeline.tracker = body.tracker
  pipeline.analyzer = body.analyzer
  pipeline.sahi = body.sahi
  pipeline.input = body.input
  pipeline.output = body.output
  pipeline.updated_at = nowIso()
  const lines = store.logs[pipelineId] ?? []
  lines.push(`${nowIso()} updated`)
  store.logs[pipelineId] = lines
  writeStore(store)
  return pipeline
}

export async function deletePipeline(pipelineId: string): Promise<null> {
  const store = readStore()
  const pipeline = findPipeline(store, pipelineId)
  assertNotRunning(pipeline, 'delete')
  clearPipelineTimer(startTimers, pipelineId)
  clearPipelineTimer(stopTimers, pipelineId)
  const lines = store.logs[pipelineId] ?? []
  lines.push(`${nowIso()} deleted`)
  store.logs[pipelineId] = lines
  store.pipelines = store.pipelines.filter((item) => item.id !== pipelineId)
  writeStore(store)
  return null
}

export async function batchDeletePipelines(
  body: BatchIdsBody,
): Promise<BatchDeleteResult> {
  const store = readStore()
  const failedIds: string[] = []
  let deletedCount = 0
  for (const id of body.ids) {
    const pipeline = store.pipelines.find((item) => item.id === id)
    if (!pipeline) {
      failedIds.push(id)
      continue
    }
    if (pipeline.status === 'online' || pipeline.status === 'running' || pipeline.status === 'starting') {
      failedIds.push(id)
      continue
    }
    clearPipelineTimer(startTimers, id)
    clearPipelineTimer(stopTimers, id)
    const lines = store.logs[id] ?? []
    lines.push(`${nowIso()} deleted`)
    store.logs[id] = lines
    store.pipelines = store.pipelines.filter((item) => item.id !== id)
    deletedCount += 1
  }
  writeStore(store)
  return { deleted_count: deletedCount, failed_ids: failedIds }
}

export async function startPipeline(pipelineId: string): Promise<PipelineStatusResult> {
  const store = readStore()
  const pipeline = findPipeline(store, pipelineId)
  if (pipeline.status === 'running' || pipeline.status === 'starting') {
    throw new ApiError(409, 'Pipeline is already started')
  }
  clearPipelineTimer(stopTimers, pipelineId)
  pipeline.status = 'starting'
  pipeline.updated_at = nowIso()
  const lines = store.logs[pipelineId] ?? []
  lines.push(`${nowIso()} INFO pipeline starting (generator invoked)`)
  store.logs[pipelineId] = lines
  writeStore(store)
  clearPipelineTimer(startTimers, pipelineId)
  const timer = setTimeout(() => {
    finishStart(pipelineId)
  }, 1500)
  startTimers.set(pipelineId, timer)
  return { id: pipelineId, status: 'starting', message: '' }
}

export async function stopPipeline(pipelineId: string): Promise<PipelineStatusResult> {
  const store = readStore()
  const pipeline = findPipeline(store, pipelineId)
  clearPipelineTimer(startTimers, pipelineId)
  pipeline.status = 'stopping'
  pipeline.updated_at = nowIso()
  const lines = store.logs[pipelineId] ?? []
  lines.push(`${nowIso()} INFO pipeline stopping`)
  store.logs[pipelineId] = lines
  writeStore(store)
  clearPipelineTimer(stopTimers, pipelineId)
  const timer = setTimeout(() => {
    finishStop(pipelineId)
  }, 800)
  stopTimers.set(pipelineId, timer)
  return { id: pipelineId, status: 'stopping', message: '' }
}

export async function getPipelineStatus(pipelineId: string): Promise<PipelineStatusResult> {
  const store = readStore()
  const pipeline = findPipeline(store, pipelineId)
  const lastRefreshAt = nowIso()
  pipeline.last_refresh_at = lastRefreshAt
  pipeline.updated_at = lastRefreshAt
  writeStore(store)
  return {
    id: pipeline.id,
    status: pipeline.status,
    message: pipeline.status === 'error' ? 'Pipeline failed' : '',
    last_refresh_at: lastRefreshAt,
  }
}

export async function getPipelineLogs(
  pipelineId: string,
  query: { tail?: number; offset?: number } = {},
): Promise<PipelineLogs> {
  const store = readStore()
  findPipeline(store, pipelineId)
  let lines = [...(store.logs[pipelineId] ?? [])]
  const offset = query.offset ?? 0
  if (offset > 0) {
    lines = lines.slice(offset)
  }
  if (query.tail !== undefined && query.tail > 0) {
    lines = lines.slice(-query.tail)
  }
  return {
    lines,
    updated_at: nowIso(),
  }
}

export async function listGieTemplates(
  query: ListGieTemplatesQuery = {},
): Promise<PagedGieTemplates> {
  const store = readStore()
  const page = query.page ?? 1
  const pageSize = query.page_size ?? 20
  const search = query.search?.trim().toLowerCase() ?? ''
  let items = store.gieTemplates.map((item) => toGieListItem(store, item))
  if (search) {
    items = items.filter(
      (item) =>
        item.name.toLowerCase().includes(search) ||
        (item.model_name ?? '').toLowerCase().includes(search),
    )
  }
  return paginate(items, page, pageSize)
}

export async function getGieTemplate(gieId: string): Promise<GieTemplateListItem> {
  const store = readStore()
  return toGieListItem(store, findGieTemplate(store, gieId))
}

export async function createGieTemplate(body: CreateGieTemplateBody): Promise<GieTemplate> {
  const store = readStore()
  validateGieName(store, body.name)
  validateClassAttrs(body.class_attrs)
  const createdAt = nowIso()
  const template: GieTemplate = {
    id: newId(),
    name: body.name.trim(),
    model_id: body.model_id,
    model_name: 'yolo26n',
    class_attrs: body.class_attrs,
    class_attrs_summary: buildClassAttrsSummary(body.class_attrs),
    created_at: createdAt,
    updated_at: createdAt,
  }
  store.gieTemplates.unshift(template)
  writeStore(store)
  return template
}

export async function updateGieTemplate(
  gieId: string,
  body: UpdateGieTemplateBody,
): Promise<GieTemplate> {
  const store = readStore()
  const template = findGieTemplate(store, gieId)
  validateGieName(store, body.name, gieId)
  validateClassAttrs(body.class_attrs)
  applyGieTemplateBody(template, body, template.model_name)
  writeStore(store)
  return template
}

export async function deleteGieTemplate(gieId: string): Promise<null> {
  const store = readStore()
  findGieTemplate(store, gieId)
  if (isPipelineReferenced(store, gieId)) {
    throw new ApiError(409, 'GIE template is referenced by a pipeline')
  }
  store.gieTemplates = store.gieTemplates.filter((item) => item.id !== gieId)
  writeStore(store)
  return null
}

export async function batchDeleteGieTemplates(
  body: BatchIdsBody,
): Promise<BatchDeleteResult> {
  const store = readStore()
  const failedIds: string[] = []
  let deletedCount = 0
  for (const id of body.ids) {
    const template = store.gieTemplates.find((item) => item.id === id)
    if (!template) {
      failedIds.push(id)
      continue
    }
    if (isPipelineReferenced(store, id)) {
      failedIds.push(id)
      continue
    }
    store.gieTemplates = store.gieTemplates.filter((item) => item.id !== id)
    deletedCount += 1
  }
  writeStore(store)
  return { deleted_count: deletedCount, failed_ids: failedIds }
}

export async function listAnalyzerTemplates(
  query: ListAnalyzerTemplatesQuery = {},
): Promise<PagedAnalyzerTemplates> {
  const store = readStore()
  const page = query.page ?? 1
  const pageSize = query.page_size ?? 20
  const search = query.search?.trim().toLowerCase() ?? ''
  let items = store.analyzerTemplates.map((item) => toAnalyzerListItem(store, item))
  if (search) {
    items = items.filter((item) => item.name.toLowerCase().includes(search))
  }
  return paginate(items, page, pageSize)
}

export async function getAnalyzerTemplate(analyzerId: string): Promise<AnalyzerTemplate> {
  return findAnalyzerTemplate(readStore(), analyzerId)
}

export async function createAnalyzerTemplate(
  body: CreateAnalyzerTemplateBody,
): Promise<AnalyzerTemplate> {
  const store = readStore()
  validateAnalyzerName(store, body.name)
  const template: AnalyzerTemplate = {
    id: newId(),
    name: body.name.trim(),
    source_kind: body.source_kind,
    source_stream_id: body.source_stream_id,
    source_file_name: body.source_file_name,
    source_image_url: body.source_image_url,
    config_width: body.config_width,
    config_height: body.config_height,
    captured_at: body.captured_at,
    annotations: body.annotations,
  }
  store.analyzerTemplates.unshift(template)
  writeStore(store)
  return template
}

export async function updateAnalyzerTemplate(
  analyzerId: string,
  body: UpdateAnalyzerTemplateBody,
): Promise<AnalyzerTemplate> {
  const store = readStore()
  const template = findAnalyzerTemplate(store, analyzerId)
  validateAnalyzerName(store, body.name, analyzerId)
  template.name = body.name.trim()
  template.source_kind = body.source_kind
  template.source_stream_id = body.source_stream_id
  template.source_file_name = body.source_file_name
  template.source_image_url = body.source_image_url
  template.config_width = body.config_width
  template.config_height = body.config_height
  template.captured_at = body.captured_at
  template.annotations = body.annotations
  writeStore(store)
  return template
}

export async function deleteAnalyzerTemplate(analyzerId: string): Promise<null> {
  const store = readStore()
  findAnalyzerTemplate(store, analyzerId)
  if (isAnalyzerReferenced(store, analyzerId)) {
    throw new ApiError(409, 'Analyzer template is referenced by a pipeline')
  }
  store.analyzerTemplates = store.analyzerTemplates.filter((item) => item.id !== analyzerId)
  writeStore(store)
  return null
}

export async function batchDeleteAnalyzerTemplates(
  body: BatchIdsBody,
): Promise<BatchDeleteResult> {
  const store = readStore()
  const failedIds: string[] = []
  let deletedCount = 0
  for (const id of body.ids) {
    const template = store.analyzerTemplates.find((item) => item.id === id)
    if (!template) {
      failedIds.push(id)
      continue
    }
    if (isAnalyzerReferenced(store, id)) {
      failedIds.push(id)
      continue
    }
    store.analyzerTemplates = store.analyzerTemplates.filter((item) => item.id !== id)
    deletedCount += 1
  }
  writeStore(store)
  return { deleted_count: deletedCount, failed_ids: failedIds }
}

export {
  createAnalyzerPlaceholderDataUrl,
  previewAnalyzerSourceFile,
  previewAnalyzerSourceStream,
} from '@/api/pipelines/utils'

export async function uploadAnalyzerSourceFile(
  analyzerId: string,
  file: File,
): Promise<AnalyzerSourceResult> {
  const store = readStore()
  const template = findAnalyzerTemplate(store, analyzerId)
  const result = await previewAnalyzerSourceFile(file)
  template.source_kind = result.source_kind
  template.source_stream_id = null
  template.source_file_name = result.source_file_name ?? null
  template.source_image_url = result.source_image_url
  template.config_width = result.config_width
  template.config_height = result.config_height
  template.captured_at = result.captured_at
  writeStore(store)
  return result
}

export async function captureAnalyzerSourceStream(
  analyzerId: string,
  body: CaptureAnalyzerSourceStreamBody,
  streamName = body.stream_id,
): Promise<AnalyzerSourceResult> {
  const store = readStore()
  const template = findAnalyzerTemplate(store, analyzerId)
  const result = await previewAnalyzerSourceStream(body.stream_id, streamName)
  template.source_kind = result.source_kind
  template.source_stream_id = body.stream_id
  template.source_file_name = null
  template.source_image_url = result.source_image_url
  template.config_width = result.config_width
  template.config_height = result.config_height
  template.captured_at = result.captured_at
  writeStore(store)
  return result
}
