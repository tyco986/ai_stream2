<template>
  <el-radio-group
    :model-value="modelValue"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <el-radio
      v-for="opt in options"
      :key="String(opt.value)"
      :value="opt.value"
    >
      {{ opt.label }}
    </el-radio>
  </el-radio-group>
</template>

<script setup lang="ts">
defineProps<{
  modelValue: string | boolean | number
  options: { label: string; value: string | boolean | number }[]
}>()

const emit = defineEmits<{
  'update:modelValue': [value: string | boolean | number]
}>()
</script>
