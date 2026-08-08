<template>
  <div class="events-toolbar">
    <div class="events-toolbar__filters">
      <label class="events-toolbar__field">
        <span>{{ t('events.from') }}</span>
        <input
          class="events-toolbar__date"
          type="date"
          :value="dateFrom"
          @change="onFromChange"
        />
      </label>
      <label class="events-toolbar__field">
        <span>{{ t('events.to') }}</span>
        <input
          class="events-toolbar__date"
          type="date"
          :value="dateTo"
          @change="onToChange"
        />
      </label>
      <label class="events-toolbar__field">
        <span>{{ t('events.stream') }}</span>
        <UiSelect
          :model-value="streamId"
          :options="streamSelectOptions"
          clearable
          :placeholder="t('events.all')"
          @update:model-value="emit('update:streamId', $event || '')"
        />
      </label>
      <label class="events-toolbar__field">
        <span>{{ t('events.pipeline') }}</span>
        <UiSelect
          :model-value="pipelineId"
          :options="pipelineSelectOptions"
          clearable
          :placeholder="t('events.all')"
          @update:model-value="emit('update:pipelineId', $event || '')"
        />
      </label>
      <label class="events-toolbar__field">
        <span>{{ t('events.event') }}</span>
        <UiSelect
          :model-value="eventCode"
          :groups="eventSelectGroups"
          clearable
          :placeholder="t('events.all')"
          @update:model-value="emit('update:eventCode', $event || '')"
        />
      </label>
      <label class="events-toolbar__field">
        <span>{{ t('events.status') }}</span>
        <UiSelect
          :model-value="status"
          :options="statusOptions"
          clearable
          :placeholder="t('events.statusAll')"
          @update:model-value="emit('update:status', ($event || '') as '' | 'new' | 'acked')"
        />
      </label>
    </div>
    <div class="events-toolbar__right">
      <UiInput
        class="events-toolbar__search"
        :model-value="search"
        :placeholder="t('events.search')"
        @update:model-value="emit('update:search', $event)"
      />
      <UiButton @click="emit('export')">{{ t('events.export') }}</UiButton>
      <UiButton type="warning" @click="emit('collect')">{{ t('events.collect') }}</UiButton>
    </div>
  </div>
  <div class="events-toolbar__actions">
    <UiButton type="primary" @click="emit('refresh')">{{ t('events.refresh') }}</UiButton>
    <UiButton type="primary" :disabled="!canAck" @click="emit('ack')">
      {{ t('events.ack') }}
    </UiButton>
    <UiButton type="primary" :disabled="!canUnack" @click="emit('unack')">
      {{ t('events.unack') }}
    </UiButton>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { EventOptionGroup, EventStatus, FilterOption } from '@/api/events'
import UiButton from '@/shared/ui/Button.vue'
import UiInput from '@/shared/ui/Input.vue'
import UiSelect, { type SelectOptionGroup } from '@/shared/ui/Select.vue'

const props = defineProps<{
  dateFrom: string
  dateTo: string
  streamId: string
  pipelineId: string
  eventCode: string
  status: EventStatus | ''
  search: string
  streamOptions: FilterOption[]
  pipelineOptions: FilterOption[]
  eventGroups: EventOptionGroup[]
  canAck: boolean
  canUnack: boolean
}>()

const emit = defineEmits<{
  'update:dateFrom': [value: string]
  'update:dateTo': [value: string]
  'update:streamId': [value: string]
  'update:pipelineId': [value: string]
  'update:eventCode': [value: string]
  'update:status': [value: EventStatus | '']
  'update:search': [value: string]
  refresh: []
  ack: []
  unack: []
  export: []
  collect: []
}>()

const { t } = useI18n()

const streamSelectOptions = computed(() =>
  props.streamOptions.map((item) => ({ label: item.name, value: item.id })),
)

const pipelineSelectOptions = computed(() =>
  props.pipelineOptions.map((item) => ({ label: item.name, value: item.id })),
)

const eventSelectGroups = computed((): SelectOptionGroup[] =>
  props.eventGroups.map((group) => ({
    label: group.pipeline_name,
    options: group.items.map((item) => ({
      label: item.label,
      value: item.value,
    })),
  })),
)

const statusOptions = computed(() => [
  { label: t('events.statusNew'), value: 'new' },
  { label: t('events.statusAcked'), value: 'acked' },
])

function onFromChange(event: Event) {
  const target = event.target as HTMLInputElement
  if (target.value) {
    emit('update:dateFrom', target.value)
  }
}

function onToChange(event: Event) {
  const target = event.target as HTMLInputElement
  if (target.value) {
    emit('update:dateTo', target.value)
  }
}
</script>

<style scoped>
.events-toolbar {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12px;
  padding: 12px 16px 8px;
  flex-wrap: wrap;
}

.events-toolbar__filters {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  align-items: flex-end;
}

.events-toolbar__field {
  display: flex;
  flex-direction: column;
  gap: 4px;
  font-size: 12px;
  color: #606266;
  min-width: 120px;
}

.events-toolbar__field :deep(.ui-select) {
  width: 140px;
}

.events-toolbar__date {
  height: 32px;
  padding: 0 8px;
  border: 1px solid #dcdfe6;
  border-radius: 4px;
  color: #606266;
  font: inherit;
  font-size: 13px;
  line-height: 32px;
}

.events-toolbar__date::-webkit-datetime-edit,
.events-toolbar__date::-webkit-datetime-edit-fields-wrapper,
.events-toolbar__date::-webkit-datetime-edit-text,
.events-toolbar__date::-webkit-datetime-edit-year-field,
.events-toolbar__date::-webkit-datetime-edit-month-field,
.events-toolbar__date::-webkit-datetime-edit-day-field {
  font: inherit;
}

.events-toolbar__right {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-left: auto;
}

.events-toolbar__search {
  width: 180px;
}

.events-toolbar__actions {
  display: flex;
  gap: 8px;
  padding: 0 16px 8px;
}
</style>
