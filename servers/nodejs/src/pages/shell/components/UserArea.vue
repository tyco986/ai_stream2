<template>
  <div class="user-area">
    <span class="user-area__name">{{ displayName }}</span>
    <button
      type="button"
      class="user-area__gear"
      :aria-label="t('shell.pageSettings')"
      @click="emit('open-settings')"
    >
      <UiIcon name="setting" :size="18" />
    </button>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import UiIcon from '@/shared/ui/Icon.vue'

const props = defineProps<{
  username: string | null
}>()

const emit = defineEmits<{
  'open-settings': []
}>()

const { t } = useI18n()

const displayName = computed(() => {
  if (props.username) {
    return props.username
  }
  return t('shell.guestUsername')
})
</script>

<style scoped>
.user-area {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  flex-shrink: 0;
}

.user-area__name {
  font-size: 14px;
  color: #303133;
  max-width: 120px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.user-area__gear {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border: none;
  background: transparent;
  cursor: pointer;
  padding: 4px;
  border-radius: 4px;
  color: #606266;
}

.user-area__gear:hover {
  background: #f2f3f5;
  color: #409eff;
}
</style>
