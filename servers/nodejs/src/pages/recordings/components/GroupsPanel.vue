<template>
  <aside class="groups-panel">
    <div class="groups-panel__header">
      <span class="groups-panel__title">{{ t('recordings.groups') }}</span>
      <div class="groups-panel__actions">
        <button
          type="button"
          class="groups-panel__icon"
          :class="{ 'is-active': filterBoundOnly }"
          :title="t('recordings.filterBound')"
          @click="emit('update:filterBoundOnly', !filterBoundOnly)"
        >
          <UiIcon name="grid" :size="14" />
        </button>
        <button
          type="button"
          class="groups-panel__icon"
          :title="t('recordings.clear')"
          :disabled="!canClear"
          @click="emit('clear')"
        >
          <UiIcon name="clear" :size="14" />
        </button>
        <button
          type="button"
          class="groups-panel__icon"
          :title="t('recordings.refresh')"
          :disabled="treeLoading"
          @click="emit('refresh')"
        >
          <UiIcon name="refresh" :size="14" />
        </button>
      </div>
    </div>
    <div class="groups-panel__search">
      <UiInput
        :model-value="searchQuery"
        :placeholder="t('recordings.search')"
        @update:model-value="emit('update:searchQuery', $event)"
      />
    </div>
    <GroupTree
      v-if="root"
      :root="root"
      :search-query="searchQuery"
      :filter-bound-only="filterBoundOnly"
      :bound-ids="boundIds"
      :selected-group-id="selectedGroupId"
      :expanded-ids="expandedIds"
      @toggle="emit('toggle', $event)"
      @select-group="emit('select-group', $event)"
      @select-stream="emit('select-stream', $event)"
    />
    <DateCalendar
      :month="calendarMonth"
      :selected-date="selectedDate"
      :available-dates="availableDates"
      @prev-month="emit('prev-month')"
      @next-month="emit('next-month')"
      @select-date="emit('select-date', $event)"
    />
  </aside>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'
import type { TreeGroupNode } from '@/api/streams'
import UiIcon from '@/shared/ui/Icon.vue'
import UiInput from '@/shared/ui/Input.vue'
import DateCalendar from './DateCalendar.vue'
import GroupTree from './GroupTree.vue'

defineProps<{
  root: TreeGroupNode | null
  searchQuery: string
  filterBoundOnly: boolean
  boundIds: Set<string>
  selectedGroupId: string | null
  expandedIds: Set<string>
  treeLoading: boolean
  canClear: boolean
  calendarMonth: string
  selectedDate: string | null
  availableDates: Set<string>
}>()

const emit = defineEmits<{
  'update:searchQuery': [value: string]
  'update:filterBoundOnly': [value: boolean]
  clear: []
  refresh: []
  toggle: [id: string]
  'select-group': [id: string]
  'select-stream': [id: string]
  'prev-month': []
  'next-month': []
  'select-date': [date: string]
}>()

const { t } = useI18n()
</script>

<style scoped>
.groups-panel {
  width: 240px;
  flex-shrink: 0;
  display: flex;
  flex-direction: column;
  border-right: 1px solid #e4e7ed;
  background: #fff;
  min-height: 0;
}

.groups-panel__header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 10px 12px 6px;
}

.groups-panel__title {
  font-weight: 600;
  font-size: 14px;
}

.groups-panel__actions {
  display: flex;
  gap: 4px;
}

.groups-panel__icon {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 26px;
  height: 26px;
  border: none;
  border-radius: 4px;
  background: transparent;
  cursor: pointer;
  color: #606266;
}

.groups-panel__icon:hover:not(:disabled),
.groups-panel__icon.is-active {
  background: #ecf5ff;
  color: #409eff;
}

.groups-panel__icon:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}

.groups-panel__search {
  padding: 0 12px 8px;
}
</style>
