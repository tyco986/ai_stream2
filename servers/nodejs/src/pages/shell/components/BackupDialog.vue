<template>
  <UiDialog
    :model-value="modelValue"
    :title="t('shell.backup')"
    width="480px"
    @update:model-value="onVisibleChange"
  >
    <div class="backup-form">
      <div class="backup-form__row">
        <label class="backup-form__label">{{ t('shell.version') }}</label>
        <UiInput
          v-model="version"
          :invalid="versionInvalid"
          :placeholder="t('shell.versionPlaceholder')"
        />
      </div>
      <div class="backup-form__row">
        <label class="backup-form__label">{{ t('shell.description') }}</label>
        <UiInput v-model="description" />
      </div>
    </div>
    <template #footer>
      <UiButton :disabled="saving" @click="onCancel">
        {{ t('shell.cancel') }}
      </UiButton>
      <UiButton type="primary" :loading="saving" @click="onSave">
        {{ t('shell.save') }}
      </UiButton>
    </template>
  </UiDialog>
</template>

<script setup lang="ts">
import { ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import UiButton from '@/shared/ui/Button.vue'
import UiDialog from '@/shared/ui/Dialog.vue'
import UiInput from '@/shared/ui/Input.vue'

const props = defineProps<{
  modelValue: boolean
  saving?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  save: [payload: { version: string; description: string }]
  cancel: []
}>()

const { t } = useI18n()
const version = ref('')
const description = ref('')
const versionInvalid = ref(false)

watch(
  () => props.modelValue,
  (open) => {
    if (open) {
      version.value = ''
      description.value = ''
      versionInvalid.value = false
    }
  },
)

function onVisibleChange(value: boolean) {
  if (!value) {
    emit('cancel')
  }
  emit('update:modelValue', value)
}

function onCancel() {
  emit('cancel')
  emit('update:modelValue', false)
}

function onSave() {
  const v = version.value.trim()
  versionInvalid.value = !/^\d+(\.\d+)?$/.test(v)
  if (versionInvalid.value) {
    return
  }
  emit('save', { version: v, description: description.value.trim() })
}
</script>

<style scoped>
.backup-form {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.backup-form__row {
  display: flex;
  align-items: center;
  gap: 16px;
}

.backup-form__label {
  width: 96px;
  flex-shrink: 0;
  font-size: 14px;
  color: #606266;
}
</style>
