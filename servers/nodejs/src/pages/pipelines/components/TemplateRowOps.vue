<template>
  <div class="ops">
    <button type="button" class="ops__btn" @click="emit('edit')">
      <UiIcon name="edit" :size="16" />
    </button>
    <button type="button" class="ops__btn is-danger" @click="emit('remove')">
      <UiIcon name="delete" :size="16" />
    </button>
  </div>
</template>

<script setup lang="ts">
import UiIcon from '@/shared/ui/Icon.vue'

const emit = defineEmits<{
  edit: []
  remove: []
}>()
</script>

<style scoped>
.ops {
  display: flex;
  gap: 2px;
}

.ops__btn {
  width: 28px;
  height: 28px;
  border: none;
  background: transparent;
  cursor: pointer;
  color: #606266;
}

.ops__btn.is-danger {
  color: #f56c6c;
}
</style>
