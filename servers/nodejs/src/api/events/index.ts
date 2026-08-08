/**
 * Events API client. Implementation selected by Vite mode (real → http, mock → mock).
 */

import * as api from './runtime'

export type {
  BatchAckResult,
  EventDetail,
  EventItem,
  EventListQuery,
  EventOptionGroup,
  EventOptionItem,
  EventStatus,
  FilterOption,
  PagedEvents,
} from '@/api/events/types'

export {
  eventDateFromOccurredAt,
  eventTimeQueryFromOccurredAt,
  formatEventTimestamp,
} from '@/api/events/utils'

export const EVENTS_API_BASE = api.EVENTS_API_BASE
export const listEvents = api.listEvents
export const getEvent = api.getEvent
export const listEventOptions = api.listEventOptions
export const listCalendarDates = api.listCalendarDates
export const listEventFilterStreams = api.listEventFilterStreams
export const listEventFilterPipelines = api.listEventFilterPipelines
export const ackEvent = api.ackEvent
export const batchAckEvents = api.batchAckEvents
export const exportEvents = api.exportEvents
export const collectEvents = api.collectEvents
