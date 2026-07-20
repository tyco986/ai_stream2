<template>
  <div class="servers-table">
    <UiTable :data="rows" row-key="id">
      <UiTableColumn prop="name" :label="t('servers.name')" min-width="140" />
      <UiTableColumn :label="t('servers.status')" width="120">
        <template #default="{ row }">
          <span :class="row.status === 'online' ? 'status-online' : 'status-offline'">
            {{ statusLabel(row.status) }}
          </span>
        </template>
      </UiTableColumn>
      <UiTableColumn :label="t('servers.lastRefresh')" width="160">
        <template #default="{ row }">
          {{ formatLastRefresh(row.last_refresh_at) }}
        </template>
      </UiTableColumn>
      <UiTableColumn :label="t('servers.operations')" width="120">
        <template #default="{ row }">
          <div class="ops">
            <button
              type="button"
              class="ops__btn is-restart"
              :class="{ 'is-spinning': restartingIds.has(row.id) }"
              :title="t('servers.restart')"
              :disabled="restartingIds.has(row.id)"
              @click="emit('restart', row)"
            >
              <UiIcon name="refresh" :size="16" />
            </button>
            <button
              type="button"
              class="ops__btn"
              :title="t('servers.log')"
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
import { useI18n } from 'vue-i18n'
import type { Server, ServerStatus } from '@/api/servers'
import UiIcon from '@/shared/ui/Icon.vue'
import UiTable from '@/shared/ui/Table.vue'
import UiTableColumn from '@/shared/ui/TableColumn.vue'

defineProps<{
  rows: Server[]
  restartingIds: Set<string>
}>()

const emit = defineEmits<{
  restart: [row: Server]
  log: [row: Server]
}>()

const { t } = useI18n()

function statusLabel(status: ServerStatus) {
  return status === 'online' ? t('servers.online') : t('servers.offline')
}

function formatLastRefresh(value: string | null) {
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
</script>

<style scoped>
.servers-table {
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

.ops__btn.is-restart {
  color: #409eff;
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
