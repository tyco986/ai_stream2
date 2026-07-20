<template>
  <UiDialog
    :model-value="modelValue"
    :title="t('pipelines.selectStreams')"
    width="720px"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <PickTable :data="rows" row-key="id">
      <UiTableColumn width="48">
        <template #header>
          <UiCheckbox
            :model-value="allChecked"
            :disabled="!rows.length"
            @update:model-value="onAllChange"
          />
        </template>
        <template #default="{ row }">
          <UiCheckbox
            :model-value="selectedIds.includes(row.id)"
            @update:model-value="onRowToggle(row.id, Boolean($event))"
          />
        </template>
      </UiTableColumn>
      <UiTableColumn prop="name" :label="t('pipelines.name')" min-width="120" />
      <UiTableColumn prop="resolution" :label="t('pipelines.resolution')" width="110">
        <template #default="{ row }">
          {{ row.resolution || '—' }}
        </template>
      </UiTableColumn>
      <UiTableColumn prop="fps" :label="t('pipelines.fps')" width="70">
        <template #default="{ row }">
          {{ row.fps ?? '—' }}
        </template>
      </UiTableColumn>
      <UiTableColumn :label="t('pipelines.status')" width="90">
        <template #default="{ row }">
          {{ row.status === 'online' ? t('pipelines.streamOnline') : t('pipelines.streamOffline') }}
        </template>
      </UiTableColumn>
      <UiTableColumn prop="group_name" :label="t('pipelines.group')" min-width="100" />
    </PickTable>
    <template #footer>
      <UiButton @click="emit('update:modelValue', false)">{{ t('pipelines.cancel') }}</UiButton>
      <UiButton type="primary" @click="onConfirm">{{ t('pipelines.save') }}</UiButton>
    </template>
  </UiDialog>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import type { Stream } from '@/api/streams'
import UiButton from '@/shared/ui/Button.vue'
import UiCheckbox from '@/shared/ui/Checkbox.vue'
import UiDialog from '@/shared/ui/Dialog.vue'
import UiTableColumn from '@/shared/ui/TableColumn.vue'
import PickTable from './PickTable.vue'

const props = defineProps<{
  modelValue: boolean
  rows: Stream[]
  value: string[]
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  confirm: [value: string[]]
}>()

const { t } = useI18n()
const selectedIds = ref<string[]>([])

const allChecked = computed(
  () => props.rows.length > 0 && props.rows.every((row) => selectedIds.value.includes(row.id)),
)

watch(
  () => props.modelValue,
  (open) => {
    if (!open) {
      return
    }
    selectedIds.value = [...props.value]
  },
)

function onAllChange(checked: boolean) {
  selectedIds.value = checked ? props.rows.map((row) => row.id) : []
}

function onRowToggle(streamId: string, checked: boolean) {
  selectedIds.value = checked
    ? [...new Set([...selectedIds.value, streamId])]
    : selectedIds.value.filter((id) => id !== streamId)
}

function onConfirm() {
  emit('confirm', [...selectedIds.value])
  emit('update:modelValue', false)
}
</script>
