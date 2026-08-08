/**
 * Pipelines API client. Implementation selected by Vite mode (real → http, mock → mock).
 */

import * as api from './runtime'

export type {
  AnalyzerSourceResult,
  AnalyzerTemplate,
  AnalyzerTemplateListItem,
  Annotation,
  AnnotationShape,
  AnnotationType,
  BatchDeleteResult,
  BatchIdsBody,
  CaptureAnalyzerSourceStreamBody,
  ClassAttrRow,
  CreateAnalyzerTemplateBody,
  CreateGieTemplateBody,
  CreatePipelineBody,
  Debouncer,
  DebouncerMode,
  Drawer,
  GieTemplate,
  GieTemplateListItem,
  LineMode,
  ListAnalyzerTemplatesQuery,
  ListGieTemplatesQuery,
  ListPipelinesQuery,
  NameMap,
  PagedAnalyzerTemplates,
  PagedGieTemplates,
  PagedPipelines,
  Pipeline,
  PipelineAnalyzer,
  PipelineListItem,
  PipelineLogs,
  PipelineRef,
  PipelineSchemaBody,
  PipelineStatus,
  PipelineStatusResult,
  PipelineTypeItem,
  PipelineTypesResult,
  SchemaBlock,
  SchemaField,
  SahiConfig,
  SourceKind,
  Tracker,
  TypeSchema,
  UpdateAnalyzerTemplateBody,
  UpdateGieTemplateBody,
  UpdatePipelineBody,
} from '@/api/pipelines/types'

export {
  createAnalyzerPlaceholderDataUrl,
  previewAnalyzerSourceFile,
  previewAnalyzerSourceStream,
} from '@/api/pipelines/utils'

export const PIPELINES_API_BASE = api.PIPELINES_API_BASE
export const GIE_TEMPLATES_API_BASE = api.GIE_TEMPLATES_API_BASE
export const ANALYZER_TEMPLATES_API_BASE = api.ANALYZER_TEMPLATES_API_BASE
export const getPipelinesMap = api.getPipelinesMap
export const listPipelines = api.listPipelines
export const getPipelineTypes = api.getPipelineTypes
export const getPipelineSchema = api.getPipelineSchema
export const getPipeline = api.getPipeline
export const createPipeline = api.createPipeline
export const updatePipeline = api.updatePipeline
export const deletePipeline = api.deletePipeline
export const batchDeletePipelines = api.batchDeletePipelines
export const startPipeline = api.startPipeline
export const stopPipeline = api.stopPipeline
export const getPipelineStatus = api.getPipelineStatus
export const getPipelineLogs = api.getPipelineLogs
export const listGieTemplates = api.listGieTemplates
export const getGieTemplate = api.getGieTemplate
export const createGieTemplate = api.createGieTemplate
export const updateGieTemplate = api.updateGieTemplate
export const deleteGieTemplate = api.deleteGieTemplate
export const batchDeleteGieTemplates = api.batchDeleteGieTemplates
export const listAnalyzerTemplates = api.listAnalyzerTemplates
export const getAnalyzerTemplate = api.getAnalyzerTemplate
export const createAnalyzerTemplate = api.createAnalyzerTemplate
export const updateAnalyzerTemplate = api.updateAnalyzerTemplate
export const deleteAnalyzerTemplate = api.deleteAnalyzerTemplate
export const batchDeleteAnalyzerTemplates = api.batchDeleteAnalyzerTemplates
export const uploadAnalyzerSourceFile = api.uploadAnalyzerSourceFile
export const captureAnalyzerSourceStream = api.captureAnalyzerSourceStream
