<template>
  <UiDialog
    :model-value="modelValue"
    :title="t('pipelines.log')"
    width="560px"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <pre class="log">{{ content }}</pre>
    <template #footer>
      <UiButton type="primary" @click="emit('update:modelValue', false)">
        {{ t('pipelines.close') }}
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
  content: string
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
}>()

const { t } = useI18n()
</script>

<style scoped>
.log {
  margin: 0;
  padding: 12px;
  background: #f5f7fa;
  border-radius: 4px;
  font-size: 12px;
  white-space: pre-wrap;
  max-height: 360px;
  overflow: auto;
}
</style>
