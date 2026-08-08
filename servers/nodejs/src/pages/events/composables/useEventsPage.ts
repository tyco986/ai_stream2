import { computed, ref } from 'vue'
import {
  ackEvent,
  batchAckEvents,
  collectEvents,
  exportEvents,
  getEvent,
  listEventFilterPipelines,
  listEventFilterStreams,
  listEventOptions,
  listEvents,
  type EventDetail,
  type EventItem,
  type EventListQuery,
  type EventOptionGroup,
  type EventStatus,
  type FilterOption,
} from '@/api/events'

function todayIso(): string {
  const d = new Date()
  const y = d.getFullYear()
  const m = String(d.getMonth() + 1).padStart(2, '0')
  const day = String(d.getDate()).padStart(2, '0')
  return `${y}-${m}-${day}`
}

const rows = ref<EventItem[]>([])
const total = ref(0)
const page = ref(1)
const pageSize = ref(20)
const dateFrom = ref(todayIso())
const dateTo = ref(todayIso())
const streamId = ref('')
const pipelineId = ref('')
const eventCode = ref('')
const status = ref<EventStatus | ''>('')
const search = ref('')
const selectedIds = ref<string[]>([])
const loaded = ref(false)
const listLoading = ref(false)
const streamOptions = ref<FilterOption[]>([])
const pipelineOptions = ref<FilterOption[]>([])
const eventGroups = ref<EventOptionGroup[]>([])

const previewOpen = ref(false)
const previewDetail = ref<EventDetail | null>(null)
const previewLoading = ref(false)

export function useEventsPage() {
  const filterQuery = computed((): EventListQuery => {
    const query: EventListQuery = {
      from: dateFrom.value,
      to: dateTo.value,
      page: page.value,
      page_size: pageSize.value,
    }
    if (streamId.value) {
      query.stream_id = streamId.value
    }
    if (pipelineId.value) {
      query.pipeline_id = pipelineId.value
    }
    if (eventCode.value) {
      query.event = eventCode.value
    }
    if (status.value) {
      query.status = status.value
    }
    if (search.value.trim()) {
      query.search = search.value.trim()
    }
    return query
  })

  const hasNewSelection = computed(() =>
    selectedIds.value.some((id) =>
      rows.value.some((row) => row.id === id && row.status === 'new'),
    ),
  )

  const hasAckedSelection = computed(() =>
    selectedIds.value.some((id) =>
      rows.value.some((row) => row.id === id && row.status === 'acked'),
    ),
  )

  async function refreshOptions() {
    const [streams, pipelines, options] = await Promise.all([
      listEventFilterStreams(),
      listEventFilterPipelines(),
      listEventOptions(
        pipelineId.value ? { pipeline_id: pipelineId.value } : {},
      ),
    ])
    streamOptions.value = streams.items
    pipelineOptions.value = pipelines.items
    eventGroups.value = options.groups
  }

  async function refreshList() {
    listLoading.value = true
    const result = await listEvents(filterQuery.value)
    rows.value = result.items
    total.value = result.total
    page.value = result.page
    const visible = new Set(result.items.map((item) => item.id))
    selectedIds.value = selectedIds.value.filter((id) => visible.has(id))
    listLoading.value = false
  }

  async function bootstrap() {
    await refreshOptions()
    await refreshList()
    loaded.value = true
  }

  async function applyFilters() {
    page.value = 1
    await Promise.all([refreshList(), refreshOptions()])
  }

  async function setPage(value: number) {
    page.value = value
    await refreshList()
  }

  async function setDateFrom(value: string) {
    if (dateTo.value && value > dateTo.value) {
      return false
    }
    dateFrom.value = value
    await applyFilters()
    return true
  }

  async function setDateTo(value: string) {
    if (dateFrom.value && value < dateFrom.value) {
      return false
    }
    dateTo.value = value
    await applyFilters()
    return true
  }

  async function setStreamId(value: string) {
    streamId.value = value
    await applyFilters()
  }

  async function setPipelineId(value: string) {
    pipelineId.value = value
    eventCode.value = ''
    await applyFilters()
  }

  async function setEventCode(value: string) {
    eventCode.value = value
    await applyFilters()
  }

  async function setStatus(value: EventStatus | '') {
    status.value = value
    await applyFilters()
  }

  async function setSearch(value: string) {
    search.value = value
    await applyFilters()
  }

  async function openPreview(eventId: string) {
    previewLoading.value = true
    previewOpen.value = true
    previewDetail.value = await getEvent(eventId, filterQuery.value)
    previewLoading.value = false
  }

  async function loadPreviewNeighbor(eventId: string | null) {
    if (!eventId) {
      return
    }
    previewLoading.value = true
    previewDetail.value = await getEvent(eventId, filterQuery.value)
    previewLoading.value = false
  }

  async function ackOne(eventId: string, action: 'ack' | 'unack') {
    const updated = await ackEvent(eventId, action)
    rows.value = rows.value.map((row) => (row.id === eventId ? updated : row))
    if (previewDetail.value?.id === eventId) {
      previewDetail.value = {
        ...previewDetail.value,
        status: updated.status,
      }
    }
    return updated
  }

  async function ackSelected(action: 'ack' | 'unack') {
    const ids = [...selectedIds.value]
    if (ids.length === 0) {
      return { updated_count: 0, acked_count: 0, unacked_count: 0 }
    }
    const result = await batchAckEvents({ event_ids: ids, action })
    await refreshList()
    return result
  }

  async function runExport() {
    await exportEvents(filterQuery.value)
  }

  async function runCollect() {
    await collectEvents(filterQuery.value)
  }

  return {
    rows,
    total,
    page,
    pageSize,
    dateFrom,
    dateTo,
    streamId,
    pipelineId,
    eventCode,
    status,
    search,
    selectedIds,
    loaded,
    listLoading,
    streamOptions,
    pipelineOptions,
    eventGroups,
    hasNewSelection,
    hasAckedSelection,
    previewOpen,
    previewDetail,
    previewLoading,
    bootstrap,
    applyFilters,
    setPage,
    setDateFrom,
    setDateTo,
    setStreamId,
    setPipelineId,
    setEventCode,
    setStatus,
    setSearch,
    openPreview,
    loadPreviewNeighbor,
    ackOne,
    ackSelected,
    runExport,
    runCollect,
  }
}

export type { EventDetail, EventItem }
