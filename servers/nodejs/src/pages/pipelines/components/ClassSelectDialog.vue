<template>
  <UiDialog
    :model-value="modelValue"
    :title="t('pipelines.selectClasses')"
    width="480px"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <PickTable :data="classOptions" row-key="id">
      <UiTableColumn width="48">
        <template #header>
          <UiCheckbox
            :model-value="allChecked"
            :disabled="!classOptions.length"
            @update:model-value="onAllChange"
          />
        </template>
        <template #default="{ row }">
          <UiCheckbox
            :model-value="isRowChecked(row.id)"
            @update:model-value="onRowToggle(row.id, Boolean($event))"
          />
        </template>
      </UiTableColumn>
      <UiTableColumn prop="id" :label="t('pipelines.classId')" min-width="140" />
      <UiTableColumn prop="name" :label="t('pipelines.classLabel')" min-width="180" />
    </PickTable>
    <template #footer>
      <UiButton @click="emit('update:modelValue', false)">{{ t('pipelines.cancel') }}</UiButton>
      <UiButton type="primary" @click="onConfirm">{{ t('pipelines.save') }}</UiButton>
    </template>
  </UiDialog>
</template>

<script setup lang="ts">
import { ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import type { ClassItem } from '@/api/models'
import UiButton from '@/shared/ui/Button.vue'
import UiCheckbox from '@/shared/ui/Checkbox.vue'
import UiDialog from '@/shared/ui/Dialog.vue'
import UiTableColumn from '@/shared/ui/TableColumn.vue'
import PickTable from './PickTable.vue'

const props = defineProps<{
  modelValue: boolean
  classOptions: ClassItem[]
  value: number[]
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  confirm: [value: number[]]
}>()

const { t } = useI18n()
const selectedIds = ref<number[]>([])
const allChecked = ref(false)

watch(
  () => props.modelValue,
  (open) => {
    if (!open) {
      return
    }
    allChecked.value = props.value.includes(-1)
    selectedIds.value = allChecked.value ? [] : [...props.value]
  },
)

function allClassIds() {
  return props.classOptions.map((item) => item.id)
}

function isRowChecked(classId: number) {
  return allChecked.value || selectedIds.value.includes(classId)
}

function onAllChange(checked: boolean) {
  allChecked.value = checked
  selectedIds.value = []
}

function onRowToggle(classId: number, checked: boolean) {
  const base = allChecked.value ? allClassIds() : selectedIds.value
  allChecked.value = false
  selectedIds.value = checked
    ? [...new Set([...base, classId])]
    : base.filter((id) => id !== classId)
}

function onConfirm() {
  emit('confirm', allChecked.value ? [-1] : [...selectedIds.value])
  emit('update:modelValue', false)
}
</script>
