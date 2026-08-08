<template>
  <div class="streams-table">
    <UiTable :data="rows" row-key="id" @selection-change="onSelectionChange">
      <UiTableColumn type="selection" />
      <UiTableColumn prop="name" :label="t('streams.name')" min-width="100" />
      <UiTableColumn prop="group_name" :label="t('streams.group')" min-width="100" />
      <UiTableColumn prop="url" :label="t('streams.url')" min-width="180" />
      <UiTableColumn prop="resolution" :label="t('streams.resolution')" width="110">
        <template #default="{ row }">
          {{ row.resolution || '—' }}
        </template>
      </UiTableColumn>
      <UiTableColumn prop="fps" :label="t('streams.fps')" width="70">
        <template #default="{ row }">
          {{ row.fps ?? '—' }}
        </template>
      </UiTableColumn>
      <UiTableColumn :label="t('streams.status')" width="90">
        <template #default="{ row }">
          <span :class="statusClass(row)">{{ statusLabel(row) }}</span>
        </template>
      </UiTableColumn>
      <UiTableColumn :label="t('streams.lastProbe')" width="140">
        <template #default="{ row }">
          {{ formatLastProbe(row.last_probe_at) }}
        </template>
      </UiTableColumn>
      <UiTableColumn :label="t('streams.operations')" width="200">
        <template #default="{ row }">
          <div class="ops">
            <button
              type="button"
              class="ops__btn"
              :class="row.enabled ? 'is-play' : 'is-pause'"
              :title="row.enabled ? t('streams.disable') : t('streams.enable')"
              @click="emit('toggle-enabled', row)"
            >
              <UiIcon :name="row.enabled ? 'videoPlay' : 'videoPause'" :size="16" />
            </button>
            <button
              type="button"
              class="ops__btn"
              :class="row.recording ? 'is-rec-on' : 'is-rec-off'"
              :title="t('streams.recording')"
              :disabled="!row.enabled"
              @click="emit('toggle-recording', row)"
            >
              <UiIcon name="videoCamera" :size="16" />
            </button>
            <button
              type="button"
              class="ops__btn is-probe"
              :class="{ 'is-spinning': probingIds.has(row.id) }"
              :title="t('streams.probe')"
              :disabled="probingIds.has(row.id)"
              @click="emit('probe', row)"
            >
              <UiIcon name="refresh" :size="16" />
            </button>
            <button
              type="button"
              class="ops__btn"
              :title="t('streams.editStream')"
              @click="emit('edit', row)"
            >
              <UiIcon name="edit" :size="16" />
            </button>
            <button
              type="button"
              class="ops__btn"
              :title="t('streams.log')"
              @click="emit('log', row)"
            >
              <UiIcon name="document" :size="16" />
            </button>
            <button
              type="button"
              class="ops__btn is-danger"
              :title="t('streams.delete')"
              @click="emit('delete', row)"
            >
              <UiIcon name="delete" :size="16" />
            </button>
          </div>
        </template>
      </UiTableColumn>
    </UiTable>
  </div>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'
import type { Stream } from '@/api/streams'
import UiIcon from '@/shared/ui/Icon.vue'
import UiTable from '@/shared/ui/Table.vue'
import UiTableColumn from '@/shared/ui/TableColumn.vue'

defineProps<{
  rows: Stream[]
  probingIds: Set<string>
}>()

const emit = defineEmits<{
  'selection-change': [ids: string[]]
  'toggle-enabled': [row: Stream]
  'toggle-recording': [row: Stream]
  probe: [row: Stream]
  edit: [row: Stream]
  log: [row: Stream]
  delete: [row: Stream]
}>()

const { t } = useI18n()

function statusLabel(row: Stream) {
  let label = t('streams.offline')
  if (row.status === 'online') {
    label = t('streams.online')
  }
  return label
}

function statusClass(row: Stream) {
  let cls = 'status-offline'
  if (row.status === 'online') {
    cls = 'status-online'
  }
  return cls
}

function formatLastProbe(value: string | null) {
  if (!value) {
    return '—'
  }
  const date = new Date(value)
  const mm = String(date.getMonth() + 1).padStart(2, '0')
  const dd = String(date.getDate()).padStart(2, '0')
  const hh = String(date.getHours()).padStart(2, '0')
  const mi = String(date.getMinutes()).padStart(2, '0')
  return `${mm}-${dd} ${hh}:${mi}`
}

function onSelectionChange(rows: Stream[]) {
  emit(
    'selection-change',
    rows.map((row) => row.id),
  )
}
</script>

<style scoped>
.streams-table {
  flex: 1;
  min-height: 0;
  overflow: auto;
  padding: 0 16px;
}

.status-online {
  color: #67c23a;
}

.status-offline {
  color: #f56c6c;
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

.ops__btn.is-rec-on {
  color: #67c23a;
}

.ops__btn.is-rec-off {
  color: #c0c4cc;
}

.ops__btn.is-probe {
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
