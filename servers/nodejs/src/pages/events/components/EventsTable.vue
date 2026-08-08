<template>
  <div class="events-table">
    <UiTable :data="rows" row-key="id" @selection-change="onSelectionChange">
      <UiTableColumn type="selection" width="48" />
      <UiTableColumn :label="t('events.timestamp')" min-width="170">
        <template #default="{ row }">
          {{ formatEventTimestamp(row.occurred_at) }}
        </template>
      </UiTableColumn>
      <UiTableColumn prop="stream_name" :label="t('events.stream')" min-width="100" />
      <UiTableColumn prop="pipeline_name" :label="t('events.pipeline')" min-width="140" />
      <UiTableColumn prop="event" :label="t('events.event')" min-width="120" />
      <UiTableColumn :label="t('events.status')" width="100">
        <template #default="{ row }">
          {{
            row.status === 'new' ? t('events.statusNew') : t('events.statusAcked')
          }}
        </template>
      </UiTableColumn>
      <UiTableColumn :label="t('events.operations')" width="120">
        <template #default="{ row }">
          <div class="ops">
            <button
              type="button"
              class="ops__btn"
              :title="t('events.preview')"
              @click="emit('preview', row)"
            >
              <UiIcon name="picture" :size="16" />
            </button>
            <button
              type="button"
              class="ops__btn is-ack"
              :disabled="row.status !== 'new'"
              :title="t('events.ack')"
              @click="emit('ack', row)"
            >
              <UiIcon name="check" :size="16" />
            </button>
            <button
              type="button"
              class="ops__btn is-unack"
              :disabled="row.status !== 'acked'"
              :title="t('events.unack')"
              @click="emit('unack', row)"
            >
              <UiIcon name="close" :size="16" />
            </button>
          </div>
        </template>
      </UiTableColumn>
    </UiTable>
  </div>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'
import { formatEventTimestamp, type EventItem } from '@/api/events'
import UiIcon from '@/shared/ui/Icon.vue'
import UiTable from '@/shared/ui/Table.vue'
import UiTableColumn from '@/shared/ui/TableColumn.vue'

defineProps<{
  rows: EventItem[]
}>()

const emit = defineEmits<{
  'selection-change': [ids: string[]]
  preview: [row: EventItem]
  ack: [row: EventItem]
  unack: [row: EventItem]
}>()

const { t } = useI18n()

function onSelectionChange(rows: EventItem[]) {
  emit(
    'selection-change',
    rows.map((row) => row.id),
  )
}
</script>

<style scoped>
.events-table {
  flex: 1;
  min-height: 0;
  overflow: auto;
  padding: 0 16px;
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

.ops__btn:hover {
  background: #f5f7fa;
}

.ops__btn.is-ack {
  color: #67c23a;
}

.ops__btn.is-unack {
  color: #e6a23c;
}

.ops__btn:disabled {
  opacity: 0.35;
  cursor: not-allowed;
}

.ops__btn:disabled:hover {
  background: transparent;
}
</style>
