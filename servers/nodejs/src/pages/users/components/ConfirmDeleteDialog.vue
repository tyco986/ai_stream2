<template>
  <UiDialog
    :model-value="modelValue"
    :title="t('users.confirmRemove')"
    width="420px"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <p>{{ message }}</p>
    <template #footer>
      <UiButton @click="emit('update:modelValue', false)">{{ t('users.cancel') }}</UiButton>
      <UiButton type="danger" :loading="loading" @click="emit('confirm')">
        {{ t('users.remove') }}
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
