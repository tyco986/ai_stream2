<template>
  <div class="ui-input" :class="{ 'ui-input--invalid': invalid }">
    <el-input
      :model-value="modelValue"
      :placeholder="placeholder"
      :clearable="clearable"
      @update:model-value="emit('update:modelValue', String($event))"
      @blur="emit('blur')"
    />
  </div>
</template>

<script setup lang="ts">
withDefaults(
  defineProps<{
    modelValue: string
    placeholder?: string
    clearable?: boolean
    invalid?: boolean
  }>(),
  {
    placeholder: '',
    clearable: true,
    invalid: false,
  },
)

const emit = defineEmits<{
  'update:modelValue': [value: string]
  blur: []
}>()
</script>

<style scoped>
.ui-input {
  width: 100%;
}

.ui-input--invalid :deep(.el-input__wrapper) {
  box-shadow: 0 0 0 1px var(--el-color-danger) inset;
}
</style>
