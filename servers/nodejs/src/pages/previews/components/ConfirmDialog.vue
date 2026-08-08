<template>
  <UiDialog
    :model-value="modelValue"
    :title="title"
    width="420px"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <p>{{ message }}</p>
    <p v-if="hint" class="hint">{{ hint }}</p>
    <template #footer>
      <UiButton @click="emit('update:modelValue', false)">{{ t('preview.cancel') }}</UiButton>
      <UiButton :type="confirmType" :loading="loading" @click="emit('confirm')">
        {{ confirmLabel }}
      </UiButton>
    </template>
  </UiDialog>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'
import UiButton from '@/shared/ui/Button.vue'
import UiDialog from '@/shared/ui/Dialog.vue'

withDefaults(
  defineProps<{
    modelValue: boolean
    title: string
    message: string
    hint?: string
    confirmLabel: string
    confirmType?: 'primary' | 'danger' | 'default'
    loading?: boolean
  }>(),
  {
    hint: '',
    confirmType: 'danger',
    loading: false,
  },
)

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
