import { computed, ref } from 'vue'
import {
  getAvailability,
  listFiles,
  type RecordingFile,
} from '@/api/recordings'
import {
  collectStreams,
  getGroupsTree,
  type GroupsTree,
  type TreeStreamNode,
} from '@/api/streams'

function pad2(value: number) {
  return String(value).padStart(2, '0')
}

export function monthIso(date = new Date()) {
  return `${date.getFullYear()}-${pad2(date.getMonth() + 1)}`
}

export function todayIso(date = new Date()) {
  return `${date.getFullYear()}-${pad2(date.getMonth() + 1)}-${pad2(date.getDate())}`
}

export function shiftMonth(month: string, delta: number) {
  const [yearText, monthText] = month.split('-')
  const year = Number(yearText)
  const monthIndex = Number(monthText) - 1 + delta
  const next = new Date(year, monthIndex, 1)
  return monthIso(next)
}

const tree = ref<GroupsTree | null>(null)
const loaded = ref(false)
const treeLoading = ref(false)
const filesLoading = ref(false)
const searchQuery = ref('')
const filterBoundOnly = ref(false)
const selectedGroupId = ref<string | null>(null)
const selectedStreamId = ref<string | null>(null)

const calendarMonth = ref(monthIso())
const selectedDate = ref<string | null>(null)
const availableDates = ref<Set<string>>(new Set())
const files = ref<RecordingFile[]>([])

export function useRecordingsSession() {
  const streamMap = computed(() => {
    const map = new Map<string, TreeStreamNode>()
    if (!tree.value) {
      return map
    }
    for (const stream of collectStreams(tree.value.root)) {
      map.set(stream.id, stream)
    }
    return map
  })

  const boundStreamIds = computed(() => {
    const ids = new Set<string>()
    if (selectedStreamId.value) {
      ids.add(selectedStreamId.value)
    }
    return ids
  })

  const canClear = computed(() => selectedStreamId.value !== null)

  async function refreshTree() {
    treeLoading.value = true
    tree.value = await getGroupsTree()
    treeLoading.value = false
  }

  async function refreshAvailability() {
    const streamId = selectedStreamId.value
    if (!streamId) {
      availableDates.value = new Set()
      return
    }
    const result = await getAvailability({
      stream_ids: streamId,
      month: calendarMonth.value,
    })
    const dates = new Set<string>()
    for (const row of result.streams) {
      for (const day of row.dates) {
        dates.add(day)
      }
    }
    availableDates.value = dates
    if (selectedDate.value && !dates.has(selectedDate.value)) {
      selectedDate.value = null
      files.value = []
    }
  }

  async function loadFilesForDate(date: string) {
    const streamId = selectedStreamId.value
    if (!streamId) {
      files.value = []
      return
    }
    filesLoading.value = true
    const result = await listFiles({ stream_id: streamId, date })
    files.value = result.files
    filesLoading.value = false
  }

  async function maybeSelectToday() {
    const today = todayIso()
    if (availableDates.value.has(today)) {
      selectedDate.value = today
      await loadFilesForDate(today)
    }
  }

  async function bootstrap() {
    await refreshTree()
    await refreshAvailability()
    await maybeSelectToday()
    loaded.value = true
  }

  async function selectStream(streamId: string) {
    selectedStreamId.value = streamId
    await refreshAvailability()
    if (selectedDate.value && availableDates.value.has(selectedDate.value)) {
      await loadFilesForDate(selectedDate.value)
      return
    }
    await maybeSelectToday()
  }

  function clearSelection() {
    selectedStreamId.value = null
    selectedDate.value = null
    availableDates.value = new Set()
    files.value = []
  }

  async function setCalendarMonth(month: string) {
    calendarMonth.value = month
    await refreshAvailability()
  }

  async function selectDate(date: string) {
    selectedDate.value = date
    await loadFilesForDate(date)
  }

  return {
    tree,
    loaded,
    treeLoading,
    filesLoading,
    searchQuery,
    filterBoundOnly,
    selectedGroupId,
    selectedStreamId,
    streamMap,
    boundStreamIds,
    canClear,
    calendarMonth,
    selectedDate,
    availableDates,
    files,
    bootstrap,
    refreshTree,
    refreshAvailability,
    selectStream,
    clearSelection,
    setCalendarMonth,
    selectDate,
    shiftMonth,
  }
}
