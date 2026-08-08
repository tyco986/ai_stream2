<template>
  <el-checkbox
    :model-value="modelValue"
    :indeterminate="indeterminate"
    :disabled="disabled"
    @update:model-value="emit('update:modelValue', Boolean($event))"
  >
    <slot />
  </el-checkbox>
</template>

<script setup lang="ts">
withDefaults(
  defineProps<{
    modelValue: boolean
    indeterminate?: boolean
    disabled?: boolean
  }>(),
  {
    indeterminate: false,
    disabled: false,
  },
)

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
}>()
</script>
