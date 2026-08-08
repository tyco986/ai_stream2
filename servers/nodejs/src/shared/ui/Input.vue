<template>
  <div class="ui-input" :class="{ 'ui-input--invalid': invalid }">
    <el-input
      :model-value="modelValue"
      :type="type"
      :show-password="type === 'password'"
      :placeholder="placeholder"
      :clearable="clearable"
      :disabled="disabled"
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
    disabled?: boolean
    type?: 'text' | 'password'
  }>(),
  {
    placeholder: '',
    clearable: true,
    invalid: false,
    disabled: false,
    type: 'text',
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
