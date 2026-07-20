<template>
  <UiDialog
    :model-value="modelValue"
    :title="t('servers.confirmRestart')"
    width="420px"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <p>{{ t('servers.restartMessage', { name }) }}</p>
    <p class="hint">{{ t('servers.restartHint') }}</p>
    <template #footer>
      <UiButton @click="emit('update:modelValue', false)">{{ t('servers.cancel') }}</UiButton>
      <UiButton type="primary" :loading="loading" @click="emit('confirm')">
        {{ t('servers.restart') }}
      </UiButton>
    </template>
  </UiDialog>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'
import UiButton from '@/shared/ui/Button.vue'
import UiDialog from '@/shared/ui/Dialog.vue'

defineProps<{
  modelValue: boolean
  name: string
  loading?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  confirm: []
}>()

const { t } = useI18n()
</script>

<style scoped>
.hint {
  color: #909399;
  margin-top: 8px;
}
</style>
