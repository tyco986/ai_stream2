<template>
  <UiSelect
    :model-value="modelValue"
    :options="options"
    :disabled="disabled"
    :invalid="invalid"
    :placeholder="placeholder"
    :clearable="clearable"
    popper-class="gie-class-select-popper"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <template #label>
      <GieClassOptionText
        v-if="selected"
        :class-id="selected.classId"
        :class-label="selected.classLabel"
      />
    </template>
    <template #option="{ option }">
      <GieClassOptionText
        :class-id="textOf(option).classId"
        :class-label="textOf(option).classLabel"
      />
    </template>
  </UiSelect>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type { ClassSelectOption } from '@/shared/gie/classAttrs'
import type { SelectOption } from '@/shared/ui/Select.vue'
import UiSelect from '@/shared/ui/Select.vue'
import GieClassOptionText from './GieClassOptionText.vue'

const props = withDefaults(
  defineProps<{
    modelValue: string
    options: ClassSelectOption[]
    disabled?: boolean
    invalid?: boolean
    placeholder?: string
    clearable?: boolean
  }>(),
  {
    disabled: false,
    invalid: false,
    clearable: false,
  },
)

const emit = defineEmits<{
  'update:modelValue': [value: string]
}>()

const selected = computed(
  () => props.options.find((item) => item.value === props.modelValue) ?? null,
)

function textOf(option: SelectOption) {
  const match = props.options.find((item) => item.value === option.value)
  return {
    classId: match?.classId ?? '',
    classLabel: match?.classLabel ?? '',
  }
}
</script>

<style>
.gie-class-select-popper.el-select__popper .el-select-dropdown__item {
  padding: 0 12px;
}

.gie-class-select-popper .el-select-dropdown__item.is-selected::after {
  display: none;
}
</style>
