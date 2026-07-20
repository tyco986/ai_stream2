<template>
  <button
    type="button"
    class="icon-btn is-ok"
    :disabled="confirmDisabled"
    @click="emit('confirm')"
  >
    <UiIcon name="check" :size="14" />
  </button>
  <button type="button" class="icon-btn is-cancel" @click="emit('cancel')">
    <UiIcon name="close" :size="14" />
  </button>
</template>

<script setup lang="ts">
import UiIcon from '@/shared/ui/Icon.vue'

withDefaults(
  defineProps<{
    confirmDisabled?: boolean
  }>(),
  {
    confirmDisabled: false,
  },
)

const emit = defineEmits<{
  confirm: []
  cancel: []
}>()
</script>

<style scoped>
.icon-btn {
  width: 28px;
  height: 28px;
  border: 1px solid #dcdfe6;
  border-radius: 4px;
  background: #fff;
  cursor: pointer;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.icon-btn.is-ok {
  color: #67c23a;
}

.icon-btn.is-cancel {
  color: #f56c6c;
}

.icon-btn:disabled {
  opacity: 0.45;
  cursor: not-allowed;
}
</style>
