<template>
  <div
    class="inline-edit"
    :style="{ paddingLeft: `${12 + depth * 14}px` }"
  >
    <UiInput
      :model-value="modelValue"
      class="inline-edit__input"
      @update:model-value="emit('update:modelValue', $event)"
    />
    <ConfirmCancelButtons @confirm="emit('confirm')" @cancel="emit('cancel')" />
  </div>
</template>

<script setup lang="ts">
import ConfirmCancelButtons from '@/shared/ui/ConfirmCancelButtons.vue'
import UiInput from '@/shared/ui/Input.vue'

defineProps<{
  modelValue: string
  depth: number
}>()

const emit = defineEmits<{
  'update:modelValue': [value: string]
  confirm: []
  cancel: []
}>()
</script>

<style scoped>
.inline-edit {
  display: flex;
  align-items: center;
  gap: 6px;
  min-height: 32px;
  padding-right: 8px;
}

.inline-edit__input {
  flex: 1;
  min-width: 0;
}
</style>
