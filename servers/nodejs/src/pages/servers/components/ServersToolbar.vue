<template>
  <div class="servers-toolbar">
    <UiButton type="success" :loading="refreshing" @click="emit('refresh')">
      {{ t('servers.refresh') }}
    </UiButton>
  </div>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'
import UiButton from '@/shared/ui/Button.vue'

defineProps<{
  refreshing: boolean
}>()

const emit = defineEmits<{
  refresh: []
}>()

const { t } = useI18n()
</script>

<style scoped>
.servers-toolbar {
  display: flex;
  align-items: center;
  padding: 12px 16px 8px;
}
</style>
