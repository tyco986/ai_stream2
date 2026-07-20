<template>
  <el-checkbox
    :model-value="modelValue"
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
    disabled?: boolean
  }>(),
  {
    disabled: false,
  },
)

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
}>()
</script>
