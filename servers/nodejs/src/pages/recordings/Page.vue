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
        <div class="recordings-page__toolbar">
          <UiButton
            type="primary"
            :disabled="!selectedIds.length || downloading"
            @click="onDownloadSelected"
          >
            {{ downloading ? t('recordings.zipping') : t('recordings.download') }}
          </UiButton>
        </div>
        <FilesTable
          :rows="files"
          :empty-text="emptyText"
          @selection-change="selectedIds = $event"
          @download="onDownloadRow"
          @play="onPlay"
        />
      </section>
    </div>

    <RecordingPlayDialog
      v-model="playOpen"
      :file="playFile"
      :video-url="playUrl"
      :window-start="playWindowStart"
      :window-duration="playWindowDuration"
      :loading="playLoading"
      :has-prev="playIndex > 0"
      :has-next="playIndex >= 0 && playIndex < files.length - 1"
      @prev="onPlayPrev"
      @next="onPlayNext"
      @seek="onPlaySeek"
    />
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import { useRoute } from 'vue-router'
import {
  downloadFile,
  downloadZip,
  getPlayback,
  type RecordingFile,
} from '@/api/recordings'
import UiButton from '@/shared/ui/Button.vue'
import { toast } from '@/shared/ui/toast'
import FilesTable from './components/FilesTable.vue'
import GroupsPanel from './components/GroupsPanel.vue'
import RecordingPlayDialog from './components/RecordingPlayDialog.vue'
import { useRecordingsSession } from './composables/useRecordingsSession'

const { t } = useI18n()
const route = useRoute()
const session = useRecordingsSession()
const {
  tree,
  loaded,
  treeLoading,
  filesLoading,
  searchQuery,
  filterBoundOnly,
  selectedGroupId,
  selectedStreamId,
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
} = session

const expandedIds = ref(new Set<string>())
const selectedIds = ref<string[]>([])
const downloading = ref(false)

const playOpen = ref(false)
const playIndex = ref(-1)
const playFile = ref<RecordingFile | null>(null)
const playUrl = ref<string | null>(null)
const playWindowStart = ref<string | null>(null)
const playWindowDuration = ref<number | null>(null)
const playLoading = ref(false)
let playGeneration = 0

const emptyText = computed(() => {
  let text = t('recordings.selectStreamDate')
  if (!selectedStreamId.value) {
    text = t('recordings.selectStream')
  } else if (!selectedDate.value) {
    text = t('recordings.selectDate')
  } else if (filesLoading.value) {
    text = t('recordings.loading')
  } else if (!files.value.length) {
    text = t('recordings.noFiles')
  }
  return text
})

function expandAll(node: { id: string; children?: unknown[]; streams?: unknown[] }) {
  expandedIds.value.add(node.id)
  const children = (node.children || []) as Array<{ id: string }>
  for (const child of children) {
    expandAll(child as { id: string; children?: unknown[] })
  }
}

async function onRefresh() {
  await refreshTree()
  if (tree.value?.root) {
    expandAll(tree.value.root)
  }
  await refreshAvailability()
}

function onClear() {
  clearSelection()
  selectedIds.value = []
  filterBoundOnly.value = false
  searchQuery.value = ''
}

function toggleExpand(groupId: string) {
  const next = new Set(expandedIds.value)
  if (next.has(groupId)) {
    next.delete(groupId)
  } else {
    next.add(groupId)
  }
  expandedIds.value = next
}

async function onSelectStream(streamId: string) {
  await selectStream(streamId)
  selectedIds.value = []
}

async function onPrevMonth() {
  await setCalendarMonth(shiftMonth(calendarMonth.value, -1))
}

async function onNextMonth() {
  await setCalendarMonth(shiftMonth(calendarMonth.value, 1))
}

async function onSelectDate(date: string) {
  await selectDate(date)
  selectedIds.value = []
}

async function onDownloadRow(row: RecordingFile) {
  toast.info(t('recordings.downloadStarted'))
  await downloadFile({
    stream_id: row.stream_id,
    file: row.file,
    filename: row.filename,
  })
}

async function onDownloadSelected() {
  const idSet = new Set(selectedIds.value)
  const items = files.value
    .filter((row) => idSet.has(row.id))
    .map((row) => ({ stream_id: row.stream_id, file: row.file }))
  if (!items.length) {
    return
  }
  downloading.value = true
  toast.info(
    items.length === 1
      ? t('recordings.downloadStarted')
      : t('recordings.zipStarted'),
  )
  await downloadZip(items).finally(() => {
    downloading.value = false
  })
}

async function openPlayAt(index: number, wallStart?: string) {
  if (index < 0 || index >= files.value.length) {
    return
  }
  const generation = playGeneration + 1
  playGeneration = generation
  playIndex.value = index
  playFile.value = files.value[index]
  playOpen.value = true
  playLoading.value = true
  const row = files.value[index]
  const start = wallStart || row.start
  try {
    const result = await getPlayback({
      stream_id: row.stream_id,
      file: row.file,
      start,
      end: row.end,
      duration: 10,
    })
    if (generation !== playGeneration) {
      return
    }
    playWindowStart.value = result.start
    playWindowDuration.value = result.duration
    playUrl.value = result.url
  } finally {
    if (generation === playGeneration) {
      playLoading.value = false
    }
  }
}

async function onPlay(row: RecordingFile) {
  const index = files.value.findIndex((item) => item.id === row.id)
  await openPlayAt(index)
}

function onPlayPrev() {
  void openPlayAt(playIndex.value - 1)
}

function onPlayNext() {
  void openPlayAt(playIndex.value + 1)
}

function onPlaySeek(wallStart: string) {
  void openPlayAt(playIndex.value, wallStart)
}

watch(playOpen, (open) => {
  if (open) {
    return
  }
  playGeneration += 1
  playUrl.value = null
  playWindowStart.value = null
  playWindowDuration.value = null
  playLoading.value = false
})

async function applyRouteQuery() {
  const streamId = typeof route.query.stream_id === 'string' ? route.query.stream_id : ''
  const date = typeof route.query.date === 'string' ? route.query.date : ''
  const clock = typeof route.query.t === 'string' ? route.query.t : ''
  if (!streamId) {
    return
  }
  if (date && date.length >= 7) {
    await setCalendarMonth(date.slice(0, 7))
  }
  await selectStream(streamId)
  if (date) {
    await selectDate(date)
  }
  if (!clock || !files.value.length) {
    return
  }
  const wall = Date.parse(`${date}T${clock}Z`)
  let index = files.value.findIndex((row) => {
    const start = Date.parse(row.start.endsWith('Z') ? row.start : `${row.start}Z`)
    const end = Date.parse(row.end.endsWith('Z') ? row.end : `${row.end}Z`)
    return Number.isFinite(wall) && start <= wall && wall < end
  })
  if (index < 0) {
    index = 0
  }
  await openPlayAt(index, Number.isFinite(wall) ? toSeekIso(wall) : undefined)
}

function toSeekIso(ms: number) {
  const date = new Date(ms)
  const pad = (n: number, width = 2) => String(n).padStart(width, '0')
  const micros = `${pad(date.getUTCMilliseconds(), 3)}000`
  return (
    `${date.getUTCFullYear()}-${pad(date.getUTCMonth() + 1)}-${pad(date.getUTCDate())}` +
    `T${pad(date.getUTCHours())}:${pad(date.getUTCMinutes())}:${pad(date.getUTCSeconds())}.${micros}`
  )
}

onMounted(async () => {
  await bootstrap()
  if (tree.value?.root) {
    expandAll(tree.value.root)
  }
  await applyRouteQuery()
})
</script>

<style scoped>
.recordings-page {
  display: flex;
  flex-direction: column;
  height: 100%;
  min-height: 0;
  background: #fff;
}

.recordings-page__title {
  flex: 0 0 auto;
  padding: 12px 16px;
  font-size: 18px;
  font-weight: 600;
  border-bottom: 1px solid #ebeef5;
}

.recordings-page__body {
  flex: 1;
  min-height: 0;
  display: flex;
}

.recordings-page__main {
  flex: 1;
  min-width: 0;
  min-height: 0;
  display: flex;
  flex-direction: column;
  border-left: 1px solid #ebeef5;
}

.recordings-page__toolbar {
  flex: 0 0 auto;
  display: flex;
  align-items: center;
  padding: 10px 16px;
  gap: 8px;
  border-bottom: 1px solid #ebeef5;
}
</style>
