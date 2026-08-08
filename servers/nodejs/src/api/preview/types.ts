export type PreviewLayout = '1x1' | '2x2' | '3x3' | '4x4'
export type ViewMode = 'grid' | 'focus'

export type LayoutPreset = {
  id: string
  name: string
  layout: PreviewLayout
  view_mode: ViewMode
  slots: (string | null)[]
  stream_count: number
  shot_url: string | null
}

export type LayoutPresetSummary = {
  id: string
  name: string
  layout: PreviewLayout
  stream_count: number
}

export const DEFAULT_LAYOUT_ID = '00000000-0000-4000-8000-000000000002'
export const DEFAULT_LAYOUT_NAME = 'Default'

export const LAYOUT_SLOT_COUNT: Record<PreviewLayout, number> = {
  '1x1': 1,
  '2x2': 4,
  '3x3': 9,
  '4x4': 16,
}

