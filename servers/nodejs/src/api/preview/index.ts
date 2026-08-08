/**
 * Preview API client. Implementation selected by Vite mode (real → http, mock → mock).
 */

import * as api from './runtime'

export type {
  LayoutPreset,
  LayoutPresetSummary,
  PreviewLayout,
  ViewMode,
} from '@/api/preview/types'

export {
  DEFAULT_LAYOUT_ID,
  DEFAULT_LAYOUT_NAME,
  LAYOUT_SLOT_COUNT,
} from '@/api/preview/types'

export { resizeSlots } from '@/api/preview/utils'

export const PREVIEW_API_BASE = api.PREVIEW_API_BASE
export const getActiveLayout = api.getActiveLayout
export const putActiveLayout = api.putActiveLayout
export const listLayouts = api.listLayouts
export const getLayoutsMap = api.getLayoutsMap
export const getLayout = api.getLayout
export const patchLayout = api.patchLayout
export const postLayout = api.postLayout
export const deleteLayout = api.deleteLayout
export const batchDeleteLayouts = api.batchDeleteLayouts
export const putShot = api.putShot
export const getShot = api.getShot
