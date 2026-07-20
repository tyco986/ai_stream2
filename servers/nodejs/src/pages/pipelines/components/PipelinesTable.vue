<template>
  <div class="pipelines-table">
    <UiTable
      ref="tableRef"
      :data="rows"
      row-key="id"
      @selection-change="onSelectionChange"
    >
      <UiTableColumn type="selection" />
      <UiTableColumn prop="name" :label="t('pipelines.name')" min-width="140" />
      <UiTableColumn prop="type" :label="t('pipelines.type')" min-width="160" />
      <UiTableColumn :label="t('pipelines.status')" width="110">
        <template #default="{ row }">
          <span :class="statusClass(row.status)">{{ statusLabel(row.status) }}</span>
        </template>
      </UiTableColumn>
      <UiTableColumn :label="t('pipelines.operations')" width="200">
        <template #default="{ row }">
          <div class="ops">
            <button
              v-if="showStart(row)"
              type="button"
              class="ops__btn is-play"
              :title="t('pipelines.start')"
              :disabled="pollingIds.has(row.id)"
              @click="emit('start', row)"
            >
              <UiIcon name="videoPlay" :size="16" />
            </button>
            <button
              v-else
              type="button"
              class="ops__btn is-pause"
              :title="t('pipelines.stop')"
              :disabled="pollingIds.has(row.id)"
              @click="emit('stop', row)"
            >
              <UiIcon name="videoPause" :size="16" />
            </button>
            <button
              type="button"
              class="ops__btn is-refresh"
              :class="{ 'is-spinning': pollingIds.has(row.id) }"
              :title="t('pipelines.refresh')"
              :disabled="pollingIds.has(row.id)"
              @click="emit('refresh-row', row)"
            >
              <UiIcon name="refresh" :size="16" />
            </button>
            <button
              type="button"
              class="ops__btn"
              :title="t('pipelines.edit')"
              @click="emit('edit', row)"
            >
              <UiIcon name="edit" :size="16" />
            </button>
            <button
              type="button"
              class="ops__btn is-danger"
              :title="t('pipelines.delete')"
              @click="emit('delete', row)"
            >
              <UiIcon name="delete" :size="16" />
            </button>
            <button
              type="button"
              class="ops__btn"
              :title="t('pipelines.log')"
              @click="emit('log', row)"
            >
              <UiIcon name="document" :size="16" />
            </button>
          </div>
        </template>
      </UiTableColumn>
    </UiTable>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { useI18n } from 'vue-i18n'
import type { PipelineListItem, PipelineStatus } from '@/api/pipelines'
import UiIcon from '@/shared/ui/Icon.vue'
import UiTable from '@/shared/ui/Table.vue'
import UiTableColumn from '@/shared/ui/TableColumn.vue'

defineProps<{
  rows: PipelineListItem[]
  pollingIds: Set<string>
  restoring?: boolean
}>()

const emit = defineEmits<{
  'selection-change': [ids: string[]]
  start: [row: PipelineListItem]
  stop: [row: PipelineListItem]
  'refresh-row': [row: PipelineListItem]
  edit: [row: PipelineListItem]
  delete: [row: PipelineListItem]
  log: [row: PipelineListItem]
}>()

const { t } = useI18n()
const tableRef = ref<{
  toggleRowSelection: (row: PipelineListItem, selected?: boolean) => void
  clearSelection: () => void
} | null>(null)

function showStart(row: PipelineListItem) {
  const status = row.status
  return status === 'stopped' || status === 'error'
}

function statusLabel(status: PipelineStatus) {
  let label = t('pipelines.statusStopped')
  if (status === 'running') {
    label = t('pipelines.statusRunning')
  } else if (status === 'starting') {
    label = t('pipelines.statusStarting')
  } else if (status === 'stopping') {
    label = t('pipelines.statusStopping')
  } else if (status === 'error') {
    label = t('pipelines.statusError')
  }
  return label
}

function statusClass(status: PipelineStatus) {
  let cls = 'status-stopped'
  if (status === 'running') {
    cls = 'status-running'
  } else if (status === 'error') {
    cls = 'status-error'
  } else if (status === 'starting' || status === 'stopping') {
    cls = 'status-transient'
  }
  return cls
}

function onSelectionChange(selected: PipelineListItem[]) {
  emit(
    'selection-change',
    selected.map((row) => row.id),
  )
}

function clearSelection() {
  tableRef.value?.clearSelection()
}

function toggleRowSelection(row: PipelineListItem, selected: boolean) {
  tableRef.value?.toggleRowSelection(row, selected)
}

defineExpose({
  clearSelection,
  toggleRowSelection,
})
</script>

<style scoped>
.pipelines-table {
  flex: 1;
  min-height: 0;
  overflow: auto;
  padding: 0 16px;
}

.status-running {
  color: #67c23a;
}

.status-error {
  color: #f56c6c;
}

.status-transient {
  color: #909399;
}

.status-stopped {
  color: #606266;
}

.ops {
  display: flex;
  align-items: center;
  gap: 2px;
}

.ops__btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 28px;
  height: 28px;
  border: none;
  border-radius: 4px;
  background: transparent;
  cursor: pointer;
  color: #606266;
}

.ops__btn:hover:not(:disabled) {
  background: #f5f7fa;
}

.ops__btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.ops__btn.is-play {
  color: #67c23a;
}

.ops__btn.is-pause {
  color: #f56c6c;
}

.ops__btn.is-refresh {
  color: #409eff;
}

.ops__btn.is-danger {
  color: #f56c6c;
}

.ops__btn.is-spinning :deep(.el-icon) {
  animation: spin 0.8s linear infinite;
}

@keyframes spin {
  to {
    transform: rotate(360deg);
  }
}
</style>
