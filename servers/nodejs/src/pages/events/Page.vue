<template>
  <div v-if="loaded" class="events-page">
    <header class="events-page__title">{{ t('events.title') }}</header>
    <section class="events-page__main">
      <EventsToolbar
        :date-from="dateFrom"
        :date-to="dateTo"
        :stream-id="streamId"
        :pipeline-id="pipelineId"
        :event-code="eventCode"
        :status="status"
        :search="search"
        :stream-options="streamOptions"
        :pipeline-options="pipelineOptions"
        :event-groups="eventGroups"
        :can-ack="hasNewSelection"
        :can-unack="hasAckedSelection"
        @update:date-from="onDateFrom"
        @update:date-to="onDateTo"
        @update:stream-id="setStreamId"
        @update:pipeline-id="setPipelineId"
        @update:event-code="setEventCode"
        @update:status="setStatus"
        @update:search="onSearchInput"
        @refresh="onRefresh"
        @ack="onBatchAck"
        @unack="onBatchUnack"
        @export="onExport"
        @collect="onCollect"
      />
      <EventsTable
        :rows="rows"
        @selection-change="selectedIds = $event"
        @preview="onPreview"
        @ack="onRowAck"
        @unack="onRowUnack"
      />
      <div class="events-page__pager">
        <UiPagination
          :page="page"
          :page-size="pageSize"
          :total="total"
          @update:page="setPage"
        />
      </div>
    </section>

    <EventPreviewDialog
      v-model="previewOpen"
      :detail="previewDetail"
      :loading="previewLoading"
      @prev="onPreviewPrev"
      @next="onPreviewNext"
      @ack="onPreviewAck"
      @unack="onPreviewUnack"
      @playback="onPlayback"
    />
  </div>
</template>

<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { useI18n } from 'vue-i18n'
import { useRouter } from 'vue-router'
import {
  eventDateFromOccurredAt,
  eventTimeQueryFromOccurredAt,
  type EventItem,
} from '@/api/events'
import { ApiError } from '@/shared/http/client'
import UiPagination from '@/shared/ui/Pagination.vue'
import { toast } from '@/shared/ui/toast'
import EventPreviewDialog from './components/EventPreviewDialog.vue'
import EventsTable from './components/EventsTable.vue'
import EventsToolbar from './components/EventsToolbar.vue'
import { useEventsPage } from './composables/useEventsPage'

const { t } = useI18n()
const router = useRouter()
const pageApi = useEventsPage()
const {
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
} = pageApi

const searchTimer = ref<ReturnType<typeof setTimeout> | null>(null)

onMounted(async () => {
  await bootstrap()
})

function onSearchInput(value: string) {
  if (searchTimer.value) {
    clearTimeout(searchTimer.value)
  }
  searchTimer.value = setTimeout(() => {
    void setSearch(value)
  }, 300)
}

async function onRefresh() {
  await applyFilters()
}

async function onDateFrom(value: string) {
  const ok = await setDateFrom(value)
  if (!ok) {
    toast.error(t('events.toastInvalidRange'))
  }
}

async function onDateTo(value: string) {
  const ok = await setDateTo(value)
  if (!ok) {
    toast.error(t('events.toastInvalidRange'))
  }
}

async function onBatchAck() {
  try {
    const result = await ackSelected('ack')
    toast.success(t('events.toastAckedBatch', { n: result.updated_count }))
  } catch (err) {
    toast.error(err instanceof ApiError ? err.message : String(err))
  }
}

async function onBatchUnack() {
  try {
    const result = await ackSelected('unack')
    toast.success(t('events.toastUnackedBatch', { n: result.updated_count }))
  } catch (err) {
    toast.error(err instanceof ApiError ? err.message : String(err))
  }
}

async function onRowAck(row: EventItem) {
  try {
    await ackOne(row.id, 'ack')
    toast.success(t('events.toastAcked'))
  } catch (err) {
    toast.error(err instanceof ApiError ? err.message : String(err))
  }
}

async function onRowUnack(row: EventItem) {
  try {
    await ackOne(row.id, 'unack')
    toast.success(t('events.toastUnacked'))
  } catch (err) {
    toast.error(err instanceof ApiError ? err.message : String(err))
  }
}

async function onPreview(row: EventItem) {
  try {
    await openPreview(row.id)
  } catch (err) {
    toast.error(err instanceof ApiError ? err.message : String(err))
  }
}

async function onPreviewPrev() {
  if (!previewDetail.value?.prev_id) {
    return
  }
  await loadPreviewNeighbor(previewDetail.value.prev_id)
}

async function onPreviewNext() {
  if (!previewDetail.value?.next_id) {
    return
  }
  await loadPreviewNeighbor(previewDetail.value.next_id)
}

async function onPreviewAck() {
  if (!previewDetail.value) {
    return
  }
  try {
    await ackOne(previewDetail.value.id, 'ack')
    toast.success(t('events.toastAcked'))
  } catch (err) {
    toast.error(err instanceof ApiError ? err.message : String(err))
  }
}

async function onPreviewUnack() {
  if (!previewDetail.value) {
    return
  }
  try {
    await ackOne(previewDetail.value.id, 'unack')
    toast.success(t('events.toastUnacked'))
  } catch (err) {
    toast.error(err instanceof ApiError ? err.message : String(err))
  }
}

async function onExport() {
  try {
    await runExport()
    toast.success(t('events.toastExportDone'))
  } catch (err) {
    toast.error(
      err instanceof ApiError ? err.message : t('events.toastNoExport'),
    )
  }
}

async function onCollect() {
  try {
    await runCollect()
    toast.success(t('events.toastCollectDone'))
  } catch (err) {
    toast.error(
      err instanceof ApiError ? err.message : t('events.toastNoExport'),
    )
  }
}

function onPlayback() {
  const detail = previewDetail.value
  if (!detail) {
    return
  }
  const day = eventDateFromOccurredAt(detail.occurred_at)
  const time = eventTimeQueryFromOccurredAt(detail.occurred_at)
  previewOpen.value = false
  void router.push({
    name: 'recordings',
    query: {
      mode: 'event',
      stream_id: detail.stream_id,
      date: day,
      t: time,
    },
  })
}
</script>

<style scoped>
.events-page {
  display: flex;
  flex-direction: column;
  height: 100%;
  min-height: 0;
  background: #f5f7fa;
}

.events-page__title {
  padding: 12px 16px;
  font-size: 18px;
  font-weight: 600;
  background: #fff;
  border-bottom: 1px solid #e4e7ed;
}

.events-page__main {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-width: 0;
  min-height: 0;
  background: #fff;
}

.events-page__pager {
  padding: 8px 16px 16px;
  display: flex;
  justify-content: flex-end;
}
</style>
