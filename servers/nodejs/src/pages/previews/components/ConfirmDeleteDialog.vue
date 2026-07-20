<template>
  <UiDialog
    :model-value="modelValue"
    :title="t('preview.confirmDelete')"
    width="420px"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <p>{{ message }}</p>
    <p class="hint">{{ t('preview.cannotUndo') }}</p>
    <template #footer>
      <UiButton @click="emit('update:modelValue', false)">{{ t('preview.cancel') }}</UiButton>
      <UiButton type="danger" :loading="loading" @click="emit('confirm')">
        {{ t('preview.delete') }}
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
  message: string
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
