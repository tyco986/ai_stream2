/**
 * Models API client (contract-aligned mock).
 * Paths: /{PROJECT_NAME}/backend/models/...
 * Replace with OpenAPI-generated client when ready.
 */

import { ApiError } from '@/shared/http/client'
import { BACKEND_PREFIX, STORAGE_KEY_PREFIX } from '@/shared/project'

export type ModelType = 'static' | 'dynamic'
export type ModelPrecision = 'fp16'
export type ModelStatus = 'built' | 'not_built' | 'building'

export type ClassItem = {
  id: number
  name: string
}

export type Model = {
  id: string
  name: string
  version: string | null
  batch_size: number
  type: ModelType
  precision: ModelPrecision
  task: string | null
  num_class: number | null
  classes: ClassItem[] | null
  source_file: string | null
  status: ModelStatus
  last_build_at: string | null
}

export type NameMap = Record<string, string>

export type BuildResult = {
  id: string
  success: boolean
  status: ModelStatus
  version: string | null
  task: string | null
  num_class: number | null
  classes: ClassItem[] | null
  last_build_at: string | null
  error: string | null
}

export type BuildStatus = {
  id: string
  status: ModelStatus
  last_build_at: string | null
}

export type ListModelsQuery = {
  search?: string
  page?: number
  page_size?: number
}

export type PagedModels = {
  items: Model[]
  total: number
  page: number
  page_size: number
}

export type CreateModelBody = {
  name: string
  type: ModelType
  batch_size: number
  precision?: ModelPrecision
  source_file?: File | null
}

export type BatchIdsBody = {
  ids: string[]
}

export type BatchDeleteResult = {
  deleted_count: number
  failed_ids: string[]
}

export type ModelLogs = {
  content: string
}

export const MODELS_API_BASE = `${BACKEND_PREFIX}/models`

const STORAGE_MODELS = `${STORAGE_KEY_PREFIX}models.items`
const STORAGE_LOGS = `${STORAGE_KEY_PREFIX}models.logs`

type Store = {
  models: Model[]
  logs: Record<string, string>
}

const buildTimers = new Map<string, ReturnType<typeof setTimeout>>()

function newId(): string {
  return crypto.randomUUID()
}

function seedStore(): Store {
  return {
    models: [
      {
        id: '550e8400-e29b-41d4-a716-446655440000',
        name: 'yolo11n',
        version: 'v1.0',
        batch_size: 1,
        type: 'static',
        precision: 'fp16',
        task: 'det',
        num_class: 80,
        classes: [
          { id: 0, name: 'person' },
          { id: 1, name: 'bicycle' },
          { id: 2, name: 'car' },
        ],
        source_file: '/root/models/pt/yolo11n.pt',
        status: 'built',
        last_build_at: '2026-06-28T10:15:00Z',
      },
      {
        id: '7c9e6679-7425-40de-944b-e07fc1f90ae7',
        name: 'yolo11s',
        version: null,
        batch_size: 4,
        type: 'dynamic',
        precision: 'fp16',
        task: null,
        num_class: null,
        classes: null,
        source_file: '/root/models/pt/yolo11s.pt',
        status: 'not_built',
        last_build_at: null,
      },
      {
        id: '9b1deb4d-3b7d-4bad-9bdd-2b0d7b3dcb6d',
        name: 'custom',
        version: null,
        batch_size: 1,
        type: 'static',
        precision: 'fp16',
        task: null,
        num_class: null,
        classes: null,
        source_file: null,
        status: 'not_built',
        last_build_at: null,
      },
    ],
    logs: {
      '550e8400-e29b-41d4-a716-446655440000':
        '2026-06-28 10:15:00 INFO export onnx ...\n2026-06-28 10:16:00 INFO trt build ...\n2026-06-28 10:16:30 INFO build success\n',
      '7c9e6679-7425-40de-944b-e07fc1f90ae7': '',
      '9b1deb4d-3b7d-4bad-9bdd-2b0d7b3dcb6d': '',
    },
  }
}

function readStore(): Store {
  const raw = sessionStorage.getItem(STORAGE_MODELS)
  const logsRaw = sessionStorage.getItem(STORAGE_LOGS)
  if (!raw) {
    const seed = seedStore()
    writeStore(seed)
    return seed
  }
  return {
    models: JSON.parse(raw) as Model[],
    logs: logsRaw ? (JSON.parse(logsRaw) as Record<string, string>) : {},
  }
}

function writeStore(store: Store) {
  sessionStorage.setItem(STORAGE_MODELS, JSON.stringify(store.models))
  sessionStorage.setItem(STORAGE_LOGS, JSON.stringify(store.logs))
}

function findModel(store: Store, modelId: string): Model {
  const model = store.models.find((item) => item.id === modelId)
  if (!model) {
    throw new ApiError(404, 'Model not found')
  }
  return model
}

function validateBatchSize(batchSize: number) {
  if (!Number.isInteger(batchSize) || batchSize <= 0 || batchSize > 128) {
    throw new ApiError(400, 'batch_size must be an integer >0 and <=128')
  }
}

function validatePrecision(precision: string | undefined) {
  if (precision !== undefined && precision !== 'fp16') {
    throw new ApiError(400, 'precision must be fp16')
  }
}

function appendLog(store: Store, modelId: string, line: string) {
  const prev = store.logs[modelId] ?? ''
  store.logs[modelId] = `${prev}${line}\n`
}

function finishBuild(modelId: string, success: boolean) {
  const store = readStore()
  const model = findModel(store, modelId)
  const endedAt = new Date().toISOString()
  model.last_build_at = endedAt
  if (success) {
    model.status = 'built'
    model.version = model.version ?? 'v1.0'
    model.task = model.task ?? 'det'
    model.num_class = model.num_class ?? 80
    model.classes = model.classes ?? [
      { id: 0, name: 'person' },
      { id: 1, name: 'bicycle' },
      { id: 2, name: 'car' },
    ]
    appendLog(store, modelId, `${endedAt} INFO build success`)
  } else {
    const hadBuilt = Boolean(model.version)
    model.status = hadBuilt ? 'built' : 'not_built'
    appendLog(store, modelId, `${endedAt} ERROR build failed`)
  }
  writeStore(store)
  buildTimers.delete(modelId)
}

export async function getModelsMap(): Promise<NameMap> {
  const store = readStore()
  return Object.fromEntries(store.models.map((item) => [item.name, item.id]))
}

export async function listModels(query: ListModelsQuery = {}): Promise<PagedModels> {
  const store = readStore()
  const page = query.page ?? 1
  const pageSize = query.page_size ?? 20
  const search = query.search?.trim().toLowerCase() ?? ''
  let items = [...store.models]
  if (search) {
    items = items.filter(
      (item) =>
        item.name.toLowerCase().includes(search) ||
        (item.source_file ?? '').toLowerCase().includes(search),
    )
  }
  const total = items.length
  const start = (page - 1) * pageSize
  return {
    items: items.slice(start, start + pageSize),
    total,
    page,
    page_size: pageSize,
  }
}

export async function createModel(body: CreateModelBody): Promise<Model> {
  const store = readStore()
  const name = body.name.trim()
  if (!name) {
    throw new ApiError(400, 'name is required')
  }
  if (store.models.some((item) => item.name === name)) {
    throw new ApiError(409, 'Model name already exists')
  }
  validateBatchSize(body.batch_size)
  validatePrecision(body.precision)
  if (body.type !== 'static' && body.type !== 'dynamic') {
    throw new ApiError(400, 'type must be static or dynamic')
  }
  let sourceFile: string | null = null
  if (body.source_file) {
    if (!body.source_file.name.endsWith('.pt')) {
      throw new ApiError(400, 'source_file must be a .pt file')
    }
    sourceFile = body.source_file.name
  }
  const model: Model = {
    id: newId(),
    name,
    version: null,
    batch_size: body.batch_size,
    type: body.type,
    precision: 'fp16',
    task: null,
    num_class: null,
    classes: null,
    source_file: sourceFile,
    status: 'not_built',
    last_build_at: null,
  }
  store.models.unshift(model)
  store.logs[model.id] = ''
  writeStore(store)
  return model
}

export async function getModel(modelId: string): Promise<Model> {
  return findModel(readStore(), modelId)
}

export async function deleteModel(modelId: string): Promise<null> {
  const store = readStore()
  findModel(store, modelId)
  if (buildTimers.has(modelId)) {
    clearTimeout(buildTimers.get(modelId))
    buildTimers.delete(modelId)
  }
  store.models = store.models.filter((item) => item.id !== modelId)
  delete store.logs[modelId]
  writeStore(store)
  return null
}

export async function uploadModelFile(modelId: string, file: File): Promise<Model> {
  const store = readStore()
  const model = findModel(store, modelId)
  if (model.status === 'building') {
    throw new ApiError(409, 'Cannot upload while building')
  }
  if (!file.name.endsWith('.pt')) {
    throw new ApiError(400, 'file must be a .pt file')
  }
  const hadSource = Boolean(model.source_file)
  model.source_file = file.name
  if (hadSource) {
    model.status = 'not_built'
    model.version = null
    model.task = null
    model.num_class = null
    model.classes = null
  }
  appendLog(store, modelId, `${new Date().toISOString()} INFO uploaded ${file.name}`)
  writeStore(store)
  return model
}

export async function buildModel(
  modelId: string,
): Promise<{ id: string; status: ModelStatus }> {
  const store = readStore()
  const model = findModel(store, modelId)
  if (!model.source_file) {
    throw new ApiError(400, 'source_file is required before build')
  }
  if (model.status === 'building') {
    throw new ApiError(409, 'Build already in progress')
  }
  model.status = 'building'
  appendLog(store, modelId, `${new Date().toISOString()} INFO build started`)
  writeStore(store)
  if (buildTimers.has(modelId)) {
    clearTimeout(buildTimers.get(modelId))
  }
  const timer = setTimeout(() => {
    finishBuild(modelId, true)
  }, 2500)
  buildTimers.set(modelId, timer)
  return { id: modelId, status: 'building' }
}

export async function getBuildStatus(modelId: string): Promise<BuildStatus | BuildResult> {
  const store = readStore()
  const model = findModel(store, modelId)
  if (model.status === 'building') {
    return {
      id: model.id,
      status: model.status,
      last_build_at: model.last_build_at,
    }
  }
  return {
    id: model.id,
    success: model.status === 'built',
    status: model.status,
    version: model.version,
    task: model.task,
    num_class: model.num_class,
    classes: model.classes,
    last_build_at: model.last_build_at,
    error: model.status === 'built' ? null : 'Build failed',
  }
}

export async function getModelLogs(
  modelId: string,
  query: { tail?: number } = {},
): Promise<ModelLogs> {
  const store = readStore()
  findModel(store, modelId)
  let content = store.logs[modelId] ?? ''
  if (query.tail !== undefined && query.tail > 0) {
    const lines = content.split('\n')
    content = lines.slice(-query.tail).join('\n')
  }
  return { content }
}

export async function batchDeleteModels(body: BatchIdsBody): Promise<BatchDeleteResult> {
  const store = readStore()
  const failedIds: string[] = []
  let deletedCount = 0
  for (const id of body.ids) {
    const model = store.models.find((item) => item.id === id)
    if (!model) {
      failedIds.push(id)
      continue
    }
    if (model.status === 'building') {
      failedIds.push(id)
      continue
    }
    store.models = store.models.filter((item) => item.id !== id)
    delete store.logs[id]
    deletedCount += 1
  }
  writeStore(store)
  return { deleted_count: deletedCount, failed_ids: failedIds }
}
