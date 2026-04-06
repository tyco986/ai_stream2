export type ZoneType = 'roi' | 'line_crossing' | 'overcrowding' | 'direction'

export interface AnalyticsZone {
  id: string
  camera: string
  name: string
  zone_type: ZoneType
  coordinates: number[][]
  config: Record<string, any>
  is_enabled: boolean
  created_at: string
  updated_at: string
}

export interface AnalyticsZoneCreate {
  name: string
  zone_type: ZoneType
  coordinates: number[][]
  config: Record<string, any>
  is_enabled?: boolean
}
