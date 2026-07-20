import { computed, onUnmounted, ref } from 'vue'
import {
  batchDeleteLayouts,
  DEFAULT_LAYOUT_ID,
  deleteLayout,
  getActiveLayout,
  getAvailability,
  getLayout,
  getSegments,
  getShot,
  LAYOUT_SLOT_COUNT,
  listLayouts,
  patchLayout,
  postLayout,
  putActiveLayout,
  putShot,
  resizeSlots,
  type LayoutPreset,
  type LayoutPresetSummary,
  type RecordingSegment,
  type RecordingsLayout,
  type ViewMode,
} from '@/api/recordings'
import {
  collectStreams,
  getGroupsTree,
  getStream,
  type GroupsTree,
  type Stream,
  type TreeStreamNode,
} from '@/api/streams'
import { composeShot } from '../utils/composeShot'
import {
  clampToDay,
  dateStartMs,
  firstIntersectionStart,
  formatClock,
  intersectSegmentRanges,
  monthIso,
  shiftMonth,
  todayIso,
  type TimeRange,
} from '../utils/timeline'

const layout = ref<RecordingsLayout>('1x1')
const slots = ref<(string | null)[]>([null])
const viewMode = ref<ViewMode>('grid')
const presetId = ref('')
const presetName = ref('Default')
const dirty = ref(false)
const selectedSlotIndex = ref<number | null>(null)
const tree = ref<GroupsTree | null>(null)
const presets = ref<LayoutPresetSummary[]>([])
const loaded = ref(false)
const treeLoading = ref(false)
const saving = ref(false)
const searchQuery = ref('')
const filterBoundOnly = ref(false)
const selectedGroupId = ref<string | null>(null)
const streamDetailMap = ref(new Map<string, Stream>())

const calendarMonth = ref(monthIso())
const selectedDate = ref<string | null>(null)
const availableDates = ref<Set<string>>(new Set())
const segmentsByStream = ref(new Map<string, RecordingSegment[]>())
const wallClockMs = ref(0)
const playing = ref(false)
const playbackRate = ref(1)
const muted = ref(false)
const volume = ref(0.8)
const audioSlotIndex = ref<number | null>(null)
const segmentsLoading = ref(false)

let playTimer: number | null = null
let lastTick = 0

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

  const boundStreamIds = computed(
    () => new Set(slots.value.filter((id): id is string => id !== null)),
  )

  const boundSlotIndexes = computed(() =>
    slots.value
      .map((id, index) => (id !== null ? index : -1))
      .filter((index) => index >= 0),
  )

  const timelineRanges = computed((): TimeRange[] => {
    const lists = [...boundStreamIds.value].map(
      (id) => segmentsByStream.value.get(id) ?? [],
    )
    return intersectSegmentRanges(lists)
  })

  const barDisabled = computed(
    () => !selectedDate.value || boundStreamIds.value.size === 0,
  )

  const clockLabel = computed(() => formatClock(wallClockMs.value))

  function markDirty() {
    dirty.value = true
  }

  function stopPlayback() {
    playing.value = false
    if (playTimer !== null) {
      cancelAnimationFrame(playTimer)
      playTimer = null
    }
  }

  function clearPlaybackState() {
    stopPlayback()
    selectedDate.value = null
    segmentsByStream.value = new Map()
    wallClockMs.value = 0
  }

  function ensureAudioSource() {
    const bound = boundSlotIndexes.value
    if (bound.length === 0) {
      audioSlotIndex.value = null
      return
    }
    if (
      audioSlotIndex.value === null ||
      slots.value[audioSlotIndex.value] === null
    ) {
      audioSlotIndex.value = bound[0]
    }
  }

  async function refreshAvailability() {
    const ids = [...boundStreamIds.value]
    if (ids.length === 0) {
      availableDates.value = new Set()
      return
    }
    const result = await getAvailability({
      stream_ids: ids.join(','),
      month: calendarMonth.value,
    })
    let intersection: string[] | null = null
    for (const item of result.streams) {
      const dates = item.dates
      if (intersection === null) {
        intersection = [...dates]
      } else {
        const set = new Set(dates)
        intersection = intersection.filter((date) => set.has(date))
      }
    }
    availableDates.value = new Set(intersection ?? [])
  }

  async function loadSegmentsForDate(date: string) {
    segmentsLoading.value = true
    const next = new Map<string, RecordingSegment[]>()
    const ids = [...boundStreamIds.value]
    await Promise.all(
      ids.map(async (id) => {
        const result = await getSegments({ stream_id: id, date })
        next.set(id, result.segments)
      }),
    )
    segmentsByStream.value = next
    const lists = ids.map((id) => next.get(id) ?? [])
    wallClockMs.value = firstIntersectionStart(lists, date)
    segmentsLoading.value = false
  }

  async function validateSelectedDate(showToastHint: {
    onUnavailable: () => void
  }) {
    if (!selectedDate.value) {
      return
    }
    if (boundStreamIds.value.size === 0) {
      clearPlaybackState()
      return
    }
    await refreshAvailability()
    if (!availableDates.value.has(selectedDate.value)) {
      clearPlaybackState()
      showToastHint.onUnavailable()
      return
    }
    await loadSegmentsForDate(selectedDate.value)
  }

  async function maybeSelectToday() {
    if (selectedDate.value || boundStreamIds.value.size === 0) {
      return
    }
    const today = todayIso()
    await refreshAvailability()
    if (availableDates.value.has(today)) {
      selectedDate.value = today
      await loadSegmentsForDate(today)
    }
  }

  function applyPreset(preset: LayoutPreset) {
    layout.value = preset.layout
    slots.value = [...preset.slots]
    viewMode.value = preset.view_mode
    presetId.value = preset.id
    presetName.value = preset.name
    dirty.value = false
    selectedSlotIndex.value = null
    ensureAudioSource()
  }

  async function refreshPresets(search?: string) {
    const result = await listLayouts(search !== undefined ? { search } : {})
    presets.value = result.items
  }

  async function syncStreamDetails(ids: (string | null)[]) {
    const next = new Map(streamDetailMap.value)
    for (const id of ids) {
      if (!id || next.has(id)) {
        continue
      }
      next.set(id, await getStream(id))
    }
    streamDetailMap.value = next
  }

  function buildShotDataUrl() {
    const infos = slots.value.map((id) => {
      if (!id) {
        return { id: null, name: null }
      }
      return {
        id,
        name: streamMap.value.get(id)?.name ?? id,
      }
    })
    return composeShot(layout.value, infos)
  }

  async function bootstrap() {
    const [active, treeData, layoutList] = await Promise.all([
      getActiveLayout(),
      getGroupsTree(),
      listLayouts(),
    ])
    tree.value = treeData
    presets.value = layoutList.items
    const preset = await getLayout(active.preset_id)
    applyPreset(preset)
    await syncStreamDetails(preset.slots)
    calendarMonth.value = monthIso()
    await refreshAvailability()
    await maybeSelectToday()
    loaded.value = true
  }

  async function refreshTree() {
    treeLoading.value = true
    const treeData = await getGroupsTree()
    tree.value = treeData
    const known = new Set(collectStreams(treeData.root).map((s) => s.id))
    let removed = 0
    slots.value = slots.value.map((id) => {
      if (id && !known.has(id)) {
        removed += 1
        return null
      }
      return id
    })
    if (removed > 0) {
      markDirty()
    }
    ensureAudioSource()
    treeLoading.value = false
    return removed
  }

  function setLayout(next: RecordingsLayout) {
    if (next === layout.value) {
      return
    }
    slots.value = resizeSlots(slots.value, next)
    layout.value = next
    if (
      selectedSlotIndex.value !== null &&
      selectedSlotIndex.value >= LAYOUT_SLOT_COUNT[next]
    ) {
      selectedSlotIndex.value = null
    }
    ensureAudioSource()
    markDirty()
  }

  function clearSlots() {
    slots.value = Array.from(
      { length: LAYOUT_SLOT_COUNT[layout.value] },
      () => null,
    )
    selectedSlotIndex.value = null
    audioSlotIndex.value = null
    clearPlaybackState()
    availableDates.value = new Set()
    markDirty()
  }

  function selectSlot(index: number) {
    const same = selectedSlotIndex.value === index
    selectedSlotIndex.value = same ? null : index
    if (!same && slots.value[index]) {
      audioSlotIndex.value = index
    }
  }

  async function clearSlot(
    index: number,
    onDateUnavailable: () => void,
  ) {
    const next = [...slots.value]
    next[index] = null
    slots.value = next
    markDirty()
    ensureAudioSource()
    await refreshAvailability()
    await validateSelectedDate({ onUnavailable: onDateUnavailable })
  }

  async function bindStream(
    streamId: string,
    onDateUnavailable: () => void,
  ) {
    const next = [...slots.value]
    const existing = next.indexOf(streamId)
    const hadSelection = selectedSlotIndex.value !== null
    let target = selectedSlotIndex.value

    if (existing >= 0) {
      if (target === null || target === existing) {
        await clearSlot(existing, onDateUnavailable)
        return
      }
      next[existing] = null
      next[target] = streamId
    } else if (target === null) {
      target = next.findIndex((slot) => slot === null)
      if (target < 0) {
        target = next.length - 1
      }
      next[target] = streamId
    } else {
      next[target] = streamId
    }

    slots.value = next
    selectedSlotIndex.value = hadSelection ? target : null
    markDirty()
    await syncStreamDetails([streamId])
    ensureAudioSource()
    await refreshAvailability()
    if (!selectedDate.value) {
      await maybeSelectToday()
    } else {
      await validateSelectedDate({ onUnavailable: onDateUnavailable })
    }
  }

  async function setCalendarMonth(next: string) {
    calendarMonth.value = next
    if (selectedDate.value && !selectedDate.value.startsWith(next)) {
      clearPlaybackState()
    }
    await refreshAvailability()
  }

  async function selectDate(date: string) {
    if (!availableDates.value.has(date)) {
      return
    }
    stopPlayback()
    selectedDate.value = date
    await loadSegmentsForDate(date)
  }

  function seekTo(ms: number) {
    if (!selectedDate.value) {
      return
    }
    wallClockMs.value = clampToDay(ms, selectedDate.value)
  }

  function seekBy(deltaMs: number) {
    seekTo(wallClockMs.value + deltaMs)
  }

  function tick(now: number) {
    if (!playing.value || !selectedDate.value) {
      playTimer = null
      return
    }
    const elapsed = now - lastTick
    lastTick = now
    const next = wallClockMs.value + elapsed * playbackRate.value
    const end = dateStartMs(selectedDate.value) + 24 * 60 * 60 * 1000 - 1000
    if (next >= end) {
      wallClockMs.value = end
      stopPlayback()
      return
    }
    wallClockMs.value = next
    playTimer = requestAnimationFrame(tick)
  }

  function togglePlay() {
    if (barDisabled.value) {
      return
    }
    if (playing.value) {
      stopPlayback()
      return
    }
    playing.value = true
    lastTick = performance.now()
    playTimer = requestAnimationFrame(tick)
  }

  function setPlaybackRate(rate: number) {
    playbackRate.value = rate
  }

  function setMuted(next: boolean) {
    muted.value = next
  }

  function setVolume(next: number) {
    volume.value = next
  }

  function setAudioSlot(index: number | null) {
    audioSlotIndex.value = index
    if (index !== null) {
      muted.value = false
    }
  }

  async function save() {
    saving.value = true
    const saved = await patchLayout(presetId.value, {
      layout: layout.value,
      slots: [...slots.value],
      view_mode: viewMode.value,
    })
    const shot = buildShotDataUrl()
    if (shot) {
      await putShot(saved.id, shot)
    }
    applyPreset(saved)
    await refreshPresets()
    saving.value = false
  }

  async function saveAs(name: string) {
    saving.value = true
    const created = await postLayout({
      name,
      layout: layout.value,
      view_mode: viewMode.value,
      slots: [...slots.value],
    })
    const shot = buildShotDataUrl()
    if (shot) {
      await putShot(created.id, shot)
    }
    await putActiveLayout({ preset_id: created.id })
    applyPreset(created)
    await refreshPresets()
    saving.value = false
    return created
  }

  async function renamePreset(targetId: string, name: string) {
    const updated = await patchLayout(targetId, { name })
    if (targetId === presetId.value) {
      presetName.value = updated.name
    }
    await refreshPresets()
    return updated
  }

  async function removePreset(targetId: string) {
    const result = await deleteLayout(targetId)
    if (result.was_active) {
      const preset = await getLayout(DEFAULT_LAYOUT_ID)
      await putActiveLayout({ preset_id: DEFAULT_LAYOUT_ID })
      applyPreset(preset)
      await syncStreamDetails(preset.slots)
      clearPlaybackState()
      await refreshAvailability()
    }
    await refreshPresets()
    return result
  }

  async function removePresets(ids: string[]) {
    const result = await batchDeleteLayouts({ ids })
    if (result.active_deleted) {
      const preset = await getLayout(DEFAULT_LAYOUT_ID)
      await putActiveLayout({ preset_id: DEFAULT_LAYOUT_ID })
      applyPreset(preset)
      await syncStreamDetails(preset.slots)
      clearPlaybackState()
      await refreshAvailability()
    }
    await refreshPresets()
    return result
  }

  async function loadPreset(
    nextPresetId: string,
    onDateUnavailable: () => void,
  ) {
    const preset = await getLayout(nextPresetId)
    await putActiveLayout({ preset_id: nextPresetId })
    applyPreset(preset)
    await syncStreamDetails(preset.slots)
    await refreshAvailability()
    await validateSelectedDate({ onUnavailable: onDateUnavailable })
    if (!selectedDate.value) {
      await maybeSelectToday()
    }
  }

  async function loadShot(targetId: string) {
    return getShot(targetId)
  }

  onUnmounted(() => {
    stopPlayback()
  })

  return {
    layout,
    slots,
    viewMode,
    presetId,
    presetName,
    dirty,
    selectedSlotIndex,
    tree,
    presets,
    loaded,
    treeLoading,
    saving,
    searchQuery,
    filterBoundOnly,
    selectedGroupId,
    streamMap,
    streamDetailMap,
    boundStreamIds,
    calendarMonth,
    selectedDate,
    availableDates,
    segmentsByStream,
    wallClockMs,
    playing,
    playbackRate,
    muted,
    volume,
    audioSlotIndex,
    segmentsLoading,
    timelineRanges,
    barDisabled,
    clockLabel,
    bootstrap,
    refreshTree,
    refreshPresets,
    refreshAvailability,
    setLayout,
    clearSlots,
    selectSlot,
    clearSlot,
    bindStream,
    setCalendarMonth,
    selectDate,
    seekTo,
    seekBy,
    togglePlay,
    stopPlayback,
    setPlaybackRate,
    setMuted,
    setVolume,
    setAudioSlot,
    save,
    saveAs,
    renamePreset,
    removePreset,
    removePresets,
    loadPreset,
    loadShot,
    buildShotDataUrl,
    shiftMonth,
  }
}
