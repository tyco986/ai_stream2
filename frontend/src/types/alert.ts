export type RuleType = 'object_count' | 'object_type' | 'zone_intrusion' | 'line_crossing' | 'overcrowding'
export type AlertStatus = 'pending' | 'acknowledged' | 'resolved'

export interface AlertRule {
  id: string
  name: string
  rule_type: RuleType
  cameras: string[]
  conditions: Record<string, any>
  is_enabled: boolean
  cooldown_seconds: number
  created_at: string
  updated_at: string
}

export interface AlertRuleCreate {
  name: string
  rule_type: RuleType
  cameras: string[]
  conditions: Record<string, any>
  is_enabled?: boolean
  cooldown_seconds?: number
}

export interface Alert {
  id: string
  rule: string
  rule_name: string
  camera: string
  camera_name: string
  status: AlertStatus
  detail: Record<string, any>
  recording_id: string | null
  acknowledged_by: string | null
  acknowledged_at: string | null
  resolved_by: string | null
  resolved_at: string | null
  triggered_at: string
}
