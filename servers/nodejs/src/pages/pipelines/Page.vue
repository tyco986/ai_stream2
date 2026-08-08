<template>
  <div v-if="loaded" class="pipelines-page">
    <header class="pipelines-page__title">{{ t('pipelines.title') }}</header>
    <section class="pipelines-page__main">
      <PipelinesToolbar
        :search="searchInput"
        :has-selection="selectedIds.length > 0"
        @add="openAdd"
        @remove="openBatchDelete"
        @refresh="onRefresh"
        @open-gie="gieLibOpen = true"
        @open-analyzer="analyzerLibOpen = true"
        @update:search="onSearchInput"
      />
      <div class="pipelines-page__meta">
        <span>
          {{ t('pipelines.target') }}: {{ selectedIds.length }} {{ t('pipelines.selected') }}
        </span>
      </div>
      <PipelinesTable
        ref="tableRef"
        :rows="rows"
        :polling-ids="pollingIds"
        @selection-change="onSelectionChange"
        @start="onStart"
        @stop="onStop"
        @refresh-row="onRefreshRow"
        @edit="openEdit"
        @delete="openRowDelete"
        @log="openLog"
      />
      <div class="pipelines-page__pager">
        <UiPagination
          :page="page"
          :page-size="pageSize"
          :total="total"
          @update:page="setPage"
        />
      </div>
    </section>

    <PipelineEditorDialog
      v-model="editorOpen"
      :mode="editorMode"
      :pipeline-id="editingId"
      :saving="editorSaving"
      @saved="onEditorSaved"
    />

    <GieLibraryDialog v-model="gieLibOpen" />
    <AnalyzerLibraryDialog v-model="analyzerLibOpen" />

    <ConfirmDeleteDialog
      v-model="confirmOpen"
      :message="confirmMessage"
      :hint="confirmHint"
      :loading="confirmLoading"
      @confirm="onConfirmDelete"
    />

    <PipelineLogDialog v-model="logOpen" :content="logContent" />
  </div>
</template>

<script setup lang="ts">
import { nextTick, onMounted, ref } from 'vue'
import { useI18n } from 'vue-i18n'
import type { PipelineListItem } from '@/api/pipelines'
import { ApiError } from '@/shared/http/client'
import UiPagination from '@/shared/ui/Pagination.vue'
import { toast } from '@/shared/ui/toast'
import AnalyzerLibraryDialog from './components/AnalyzerLibraryDialog.vue'
import ConfirmDeleteDialog from './components/ConfirmDeleteDialog.vue'
import GieLibraryDialog from './components/GieLibraryDialog.vue'
import PipelineEditorDialog from './components/PipelineEditorDialog.vue'
import PipelineLogDialog from './components/PipelineLogDialog.vue'
import PipelinesTable from './components/PipelinesTable.vue'
import PipelinesToolbar from './components/PipelinesToolbar.vue'
import { usePipelinesPage } from './composables/usePipelinesPage'

const { t } = useI18n()
const pageApi = usePipelinesPage()
const {
  rows,
  total,
  page,
  pageSize,
  selectedIds,
  loaded,
  listLoading,
  pollingIds,
  bootstrap,
  refreshList,
  setSearch,
  setPage,
  markPolling,
  waitForTerminalStatus,
  updateRowStatus,
  startPipeline,
  stopPipeline,
  getPipelineStatus,
  getPipelineLogs,
  batchDeletePipelines,
  deletePipeline,
} = pageApi

const searchInput = ref('')
const searchTimer = ref<ReturnType<typeof setTimeout> | null>(null)
const tableRef = ref<{
  clearSelection: () => void
  toggleRowSelection: (row: PipelineListItem, selected: boolean) => void
} | null>(null)
const restoring = ref(false)

const editorOpen = ref(false)
const editorMode = ref<'add' | 'edit'>('add')
const editingId = ref<string | null>(null)
const editorSaving = ref(false)

const gieLibOpen = ref(false)
const analyzerLibOpen = ref(false)

const confirmOpen = ref(false)
const confirmMessage = ref('')
const confirmHint = ref('')
const confirmLoading = ref(false)
const confirmIds = ref<string[]>([])

const logOpen = ref(false)
const logContent = ref('')

onMounted(async () => {
  await bootstrap()
  await restoreSelection()
})

async function restoreSelection() {
  restoring.value = true
  await nextTick()
  tableRef.value?.clearSelection()
  for (const row of rows.value) {
    if (selectedIds.value.includes(row.id)) {
      tableRef.value?.toggleRowSelection(row, true)
    }
  }
  await nextTick()
  restoring.value = false
}

function onSelectionChange(ids: string[]) {
  if (restoring.value) {
    return
  }
  selectedIds.value = ids
}

function onSearchInput(value: string) {
  searchInput.value = value
  if (searchTimer.value) {
    clearTimeout(searchTimer.value)
  }
  searchTimer.value = setTimeout(() => {
    setSearch(value).then(() => restoreSelection())
  }, 250)
}

async function onRefresh() {
  await refreshList()
  await restoreSelection()
}

function openAdd() {
  editorMode.value = 'add'
  editingId.value = null
  editorOpen.value = true
}

function openEdit(row: PipelineListItem) {
  editorMode.value = 'edit'
  editingId.value = row.id
  editorOpen.value = true
}

async function onEditorSaved() {
  editorOpen.value = false
  toast.success(t('pipelines.toastSaved'))
  await refreshList()
  await restoreSelection()
}

function openBatchDelete() {
  confirmIds.value = [...selectedIds.value]
  confirmMessage.value = t('pipelines.deletePipelines', { n: selectedIds.value.length })
  confirmHint.value = t('pipelines.cannotUndo')
  confirmOpen.value = true
}

function openRowDelete(row: PipelineListItem) {
  confirmIds.value = [row.id]
  confirmMessage.value = t('pipelines.deletePipelines', { n: 1 })
  confirmHint.value = t('pipelines.cannotUndo')
  confirmOpen.value = true
}

async function onConfirmDelete() {
  confirmLoading.value = true
  const ids = [...confirmIds.value]
  if (ids.length === 1) {
    await deletePipeline(ids[0])
    toast.success(t('pipelines.toastDeleted'))
  } else {
    const result = await batchDeletePipelines({ ids })
    toast.success(t('pipelines.toastDeletedN', { n: result.deleted_count }))
  }
  selectedIds.value = []
  confirmLoading.value = false
  confirmOpen.value = false
  await refreshList()
}

async function onStart(row: PipelineListItem) {
  markPolling(row.id, true)
  try {
    await startPipeline(row.id)
    updateRowStatus(row.id, 'starting')
    toast.success(t('pipelines.toastStarting'))
    const status = await waitForTerminalStatus(row.id, ['running', 'online', 'error', 'stopped'])
    if (status === 'running' || status === 'online') {
      toast.success(t('pipelines.toastStarted'))
    } else if (status === 'error') {
      toast.error(t('pipelines.toastFailed'))
    } else if (status === 'starting' || status === 'stopping') {
      toast.error(t('pipelines.toastStatusTimeout'))
    }
  } catch (err) {
    toast.error(err instanceof ApiError ? err.message : t('pipelines.toastFailed'))
  }
  markPolling(row.id, false)
}

async function onStop(row: PipelineListItem) {
  markPolling(row.id, true)
  try {
    await stopPipeline(row.id)
    updateRowStatus(row.id, 'stopping')
    const status = await waitForTerminalStatus(row.id, ['stopped', 'error'])
    if (status === 'stopped') {
      toast.success(t('pipelines.toastStopped'))
    } else if (status === 'stopping') {
      toast.error(t('pipelines.toastStatusTimeout'))
    }
  } catch (err) {
    toast.error(err instanceof ApiError ? err.message : t('pipelines.toastFailed'))
  }
  markPolling(row.id, false)
}

async function onRefreshRow(row: PipelineListItem) {
  markPolling(row.id, true)
  try {
    const result = await getPipelineStatus(row.id)
    updateRowStatus(row.id, result.status, result.last_refresh_at)
  } catch (err) {
    toast.error(err instanceof ApiError ? err.message : t('pipelines.toastFailed'))
  }
  markPolling(row.id, false)
}

async function openLog(row: PipelineListItem) {
  const result = await getPipelineLogs(row.id)
  logContent.value = result.lines.join('\n')
  logOpen.value = true
}
</script>

<style scoped>
.pipelines-page {
  display: flex;
  flex-direction: column;
  height: 100%;
  min-height: 0;
  background: #f5f7fa;
}

.pipelines-page__title {
  padding: 12px 16px;
  font-size: 18px;
  font-weight: 600;
  background: #fff;
  border-bottom: 1px solid #e4e7ed;
}

.pipelines-page__main {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-width: 0;
  min-height: 0;
  background: #fff;
}

.pipelines-page__meta {
  display: flex;
  gap: 24px;
  padding: 0 16px 8px;
  font-size: 13px;
  color: #606266;
}

.pipelines-page__pager {
  display: flex;
  justify-content: center;
  padding: 12px 16px;
}
</style>
