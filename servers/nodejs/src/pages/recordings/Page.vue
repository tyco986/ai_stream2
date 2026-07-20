<template>
  <div v-if="loaded" class="recordings-page">
    <header class="recordings-page__title">{{ t('recordings.title') }}</header>
    <div class="recordings-page__body">
      <GroupsPanel
        :root="tree?.root ?? null"
        :search-query="searchQuery"
        :filter-bound-only="filterBoundOnly"
        :bound-ids="boundStreamIds"
        :selected-group-id="selectedGroupId"
        :expanded-ids="expandedIds"
        :tree-loading="treeLoading"
        :can-clear="canClear"
        :calendar-month="calendarMonth"
        :selected-date="selectedDate"
        :available-dates="availableDates"
        @update:search-query="searchQuery = $event"
        @update:filter-bound-only="filterBoundOnly = $event"
        @clear="onClear"
        @refresh="onRefresh"
        @toggle="toggleExpand"
        @select-group="selectedGroupId = $event"
        @select-stream="onSelectStream"
        @prev-month="onPrevMonth"
        @next-month="onNextMonth"
        @select-date="onSelectDate"
      />
      <section class="recordings-page__main">
        <PresetBar
          :layout="layout"
          :preset-id="presetId"
          :dirty="dirty"
          :saving="saving"
          :presets="presets"
          :can-clear="canClear"
          @update:layout="onSetLayout"
          @change-preset="onChangePreset"
          @save="onSave"
          @save-as="openSaveAs"
          @clear="onClear"
          @manage="openManage"
        />
        <div ref="stageRef" class="recordings-page__stage">
          <PlaybackGrid
            :layout="layout"
            :slots="slots"
            :stream-map="streamMap"
            :selected-slot-index="selectedSlotIndex"
            :selected-date="selectedDate"
            :segments-by-stream="segmentsByStream"
            :wall-clock-ms="wallClockMs"
            :clock-label="clockLabel"
            :playing="playing"
            :segments-loading="segmentsLoading"
            @select-slot="selectSlot"
            @clear-slot="onClearSlot"
          />
          <PlaybackBar
            :disabled="barDisabled"
            :playing="playing"
            :clock-label="clockLabel"
            :wall-clock-ms="wallClockMs"
            :selected-date="selectedDate"
            :timeline-ranges="timelineRanges"
            :playback-rate="playbackRate"
            :muted="muted"
            :volume="volume"
            :audio-slot-index="audioSlotIndex"
            :audio-sources="audioSources"
            @toggle-play="togglePlay"
            @seek-by="seekBy"
            @seek="seekTo"
            @set-volume="setVolume"
            @set-audio-slot="setAudioSlot"
            @set-rate="setPlaybackRate"
            @fullscreen="onFullscreen"
          />
        </div>
      </section>
    </div>

    <LayoutFormDialog
      v-model="formOpen"
      :mode="formMode"
      :name="formName"
      :layout="formLayout"
      :view-mode="formViewMode"
      :slots="formSlots"
      :stream-names="streamNames"
      :shot-url="formShotUrl"
      :saving="formSaving"
      :error="formError"
      @update:name="formName = $event"
      @submit="onFormSubmit"
      @preview-shot="shotOpen = true"
    />

    <ManageLayoutsDialog
      v-model="manageOpen"
      :items="manageItems"
      :search="manageSearch"
      @update:search="onManageSearch"
      @rename="openRename"
      @delete="openDeleteOne"
      @batch-delete="openDeleteBatch"
    />

    <ShotPreviewDialog v-model="shotOpen" :url="formShotUrl" />

    <ConfirmDeleteDialog
      v-model="confirmOpen"
      :message="confirmMessage"
      :loading="confirmLoading"
      @confirm="onConfirmDelete"
    />
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import {
  getLayout,
  listLayouts,
  type LayoutPresetSummary,
  type RecordingsLayout,
  type ViewMode,
} from '@/api/recordings'
import { toast } from '@/shared/ui/toast'
import ConfirmDeleteDialog from './components/ConfirmDeleteDialog.vue'
import GroupsPanel from './components/GroupsPanel.vue'
import LayoutFormDialog from './components/LayoutFormDialog.vue'
import ManageLayoutsDialog from './components/ManageLayoutsDialog.vue'
import PlaybackBar from './components/PlaybackBar.vue'
import PlaybackGrid from './components/PlaybackGrid.vue'
import PresetBar from './components/PresetBar.vue'
import ShotPreviewDialog from './components/ShotPreviewDialog.vue'
import { useRecordingsSession } from './composables/useRecordingsSession'

const { t } = useI18n()
const session = useRecordingsSession()
const {
  layout,
  slots,
  viewMode,
  presetId,
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
  setPlaybackRate,
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
} = session

const expandedIds = ref(new Set<string>())
const canClear = computed(
  () => boundStreamIds.value.size > 0 || selectedSlotIndex.value !== null,
)
const stageRef = ref<HTMLElement | null>(null)

const streamNames = computed(() => {
  const names: Record<string, string> = {}
  for (const [id, node] of streamMap.value) {
    names[id] = node.name
  }
  return names
})

const audioSources = computed(() =>
  slots.value
    .map((id, index) => {
      if (!id) {
        return null
      }
      return {
        index,
        name: streamMap.value.get(id)?.name ?? id,
      }
    })
    .filter((item): item is { index: number; name: string } => item !== null),
)

const formOpen = ref(false)
const formMode = ref<'create' | 'rename'>('create')
const formName = ref('')
const formLayout = ref<RecordingsLayout>('1x1')
const formViewMode = ref<ViewMode>('grid')
const formSlots = ref<(string | null)[]>([])
const formShotUrl = ref('')
const formSaving = ref(false)
const formError = ref('')
const formTargetId = ref('')

const manageOpen = ref(false)
const manageSearch = ref('')
const manageItems = ref<LayoutPresetSummary[]>([])

const shotOpen = ref(false)

const confirmOpen = ref(false)
const confirmMessage = ref('')
const confirmLoading = ref(false)
const confirmIds = ref<string[]>([])

onMounted(async () => {
  await bootstrap()
  if (tree.value) {
    expandedIds.value = new Set(collectGroupIds(tree.value.root))
  }
})

watch(manageOpen, async (open) => {
  if (open) {
    manageSearch.value = ''
    await refreshManageList()
  }
})

function collectGroupIds(node: {
  type: string
  id: string
  children?: Array<{ type: string; id: string; children?: unknown[] }>
}): string[] {
  if (node.type !== 'group') {
    return []
  }
  const ids = [node.id]
  for (const child of node.children ?? []) {
    ids.push(
      ...collectGroupIds(
        child as {
          type: string
          id: string
          children?: Array<{ type: string; id: string; children?: unknown[] }>
        },
      ),
    )
  }
  return ids
}

function toggleExpand(id: string) {
  const next = new Set(expandedIds.value)
  if (next.has(id)) {
    next.delete(id)
  } else {
    next.add(id)
  }
  expandedIds.value = next
}

function onDateUnavailable() {
  toast.warning(t('recordings.dateUnavailable'))
}

async function onSelectStream(streamId: string) {
  const node = streamMap.value.get(streamId)
  if (!node) {
    return
  }
  if (!node.recording) {
    toast.warning(t('recordings.recordingDisabled', { name: node.name }))
    return
  }
  await bindStream(streamId, onDateUnavailable)
}

async function onClearSlot(index: number) {
  await clearSlot(index, onDateUnavailable)
}

function onClear() {
  clearSlots()
}

async function onSetLayout(next: RecordingsLayout) {
  setLayout(next)
  await refreshAvailability()
  if (selectedDate.value && !availableDates.value.has(selectedDate.value)) {
    onDateUnavailable()
  }
}

async function onPrevMonth() {
  await setCalendarMonth(shiftMonth(calendarMonth.value, -1))
}

async function onNextMonth() {
  await setCalendarMonth(shiftMonth(calendarMonth.value, 1))
}

async function onSelectDate(date: string) {
  await selectDate(date)
}

async function onRefresh() {
  const removed = await refreshTree()
  if (removed > 0) {
    toast.warning(t('recordings.streamsMissing', { n: removed }))
  }
  await refreshAvailability()
  if (selectedDate.value && !availableDates.value.has(selectedDate.value)) {
    onDateUnavailable()
  }
}

async function onSave() {
  await save()
  toast.success(t('recordings.layoutSaved'))
}

async function onChangePreset(nextId: string) {
  if (nextId === presetId.value) {
    return
  }
  if (dirty.value && !window.confirm(t('recordings.discardConfirm'))) {
    return
  }
  await loadPreset(nextId, onDateUnavailable)
}

function openSaveAs() {
  formMode.value = 'create'
  formName.value = ''
  formLayout.value = layout.value
  formViewMode.value = viewMode.value
  formSlots.value = [...slots.value]
  formShotUrl.value = buildShotDataUrl()
  formError.value = ''
  formTargetId.value = ''
  formOpen.value = true
}

async function openManage() {
  manageOpen.value = true
}

async function refreshManageList() {
  const result = await listLayouts({ search: manageSearch.value })
  manageItems.value = result.items
}

async function onManageSearch(value: string) {
  manageSearch.value = value
  await refreshManageList()
}

async function openRename(row: LayoutPresetSummary) {
  const preset = await getLayout(row.id)
  formMode.value = 'rename'
  formName.value = preset.name
  formLayout.value = preset.layout
  formViewMode.value = preset.view_mode
  formSlots.value = [...preset.slots]
  formTargetId.value = preset.id
  formError.value = ''
  formShotUrl.value = ''
  if (preset.shot_url) {
    formShotUrl.value = await loadShot(preset.id)
  }
  formOpen.value = true
}

async function onFormSubmit() {
  formSaving.value = true
  formError.value = ''
  const name = formName.value.trim()
  if (!name) {
    formError.value = 'Name required'
    formSaving.value = false
    return
  }
  if (formMode.value === 'create') {
    await saveAs(name)
    toast.success(t('recordings.layoutCreated'))
  } else {
    await renamePreset(formTargetId.value, name)
    toast.success(t('recordings.layoutRenamed'))
    await refreshManageList()
  }
  formSaving.value = false
  formOpen.value = false
}

function openDeleteOne(row: LayoutPresetSummary) {
  confirmIds.value = [row.id]
  confirmMessage.value = t('recordings.deletePresets', { n: 1 })
  confirmOpen.value = true
}

function openDeleteBatch(ids: string[]) {
  confirmIds.value = [...ids]
  confirmMessage.value = t('recordings.deletePresets', { n: ids.length })
  confirmOpen.value = true
}

async function onConfirmDelete() {
  confirmLoading.value = true
  if (confirmIds.value.length === 1) {
    await removePreset(confirmIds.value[0])
  } else {
    await removePresets(confirmIds.value)
  }
  confirmLoading.value = false
  confirmOpen.value = false
  await refreshManageList()
}

function onFullscreen() {
  const el = stageRef.value
  if (!el) {
    return
  }
  if (document.fullscreenElement) {
    document.exitFullscreen()
    return
  }
  el.requestFullscreen()
}
</script>

<style scoped>
.recordings-page {
  display: flex;
  flex-direction: column;
  height: 100%;
  min-height: 0;
  background: #f5f7fa;
}

.recordings-page__title {
  padding: 12px 16px;
  font-size: 18px;
  font-weight: 600;
  background: #fff;
  border-bottom: 1px solid #e4e7ed;
}

.recordings-page__body {
  display: flex;
  flex: 1;
  min-height: 0;
}

.recordings-page__main {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-width: 0;
  min-height: 0;
}

.recordings-page__stage {
  flex: 1;
  min-height: 0;
  display: flex;
  flex-direction: column;
  background: #000;
}
</style>
