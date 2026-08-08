<template>
  <div class="users-toolbar">
    <div class="users-toolbar__actions">
      <UiButton type="primary" @click="emit('add')">{{ t('users.add') }}</UiButton>
      <UiButton type="danger" :disabled="!hasSelection" @click="emit('remove')">
        {{ t('users.remove') }}
      </UiButton>
      <UiButton type="primary" :disabled="!hasSelection" @click="emit('refresh')">
        {{ t('users.refresh') }}
      </UiButton>
      <span class="users-toolbar__divider" />
      <UiButton type="primary" @click="emit('groups')">{{ t('users.groups') }}</UiButton>
    </div>
  </div>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'
import UiButton from '@/shared/ui/Button.vue'

defineProps<{
  hasSelection: boolean
}>()

const emit = defineEmits<{
  add: []
  remove: []
  refresh: []
  groups: []
}>()

const { t } = useI18n()
</script>

<style scoped>
.users-toolbar {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 12px 16px 8px;
}

.users-toolbar__actions {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 8px;
}

.users-toolbar__divider {
  width: 1px;
  height: 20px;
  margin: 0 4px;
  background: #dcdfe6;
}
</style>
