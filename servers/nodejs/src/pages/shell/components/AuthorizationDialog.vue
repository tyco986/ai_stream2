<template>
  <UiDialog
    :model-value="modelValue"
    :title="t('shell.authorization')"
    width="480px"
    @update:model-value="onVisibleChange"
  >
    <div class="auth-row">
      <label class="auth-row__label">{{ t('shell.ticket') }}</label>
      <UiInput v-model="ticket" :placeholder="t('shell.ticketPlaceholder')" />
    </div>
    <template #footer>
      <UiButton :disabled="verifying" @click="onCancel">
        {{ t('shell.cancel') }}
      </UiButton>
      <UiButton
        type="primary"
        :loading="verifying"
        :disabled="!ticket.trim()"
        @click="onVerify"
      >
        {{ t('shell.verify') }}
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
  verifying?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  verify: [ticket: string]
  cancel: []
}>()

const { t } = useI18n()
const ticket = ref('')

watch(
  () => props.modelValue,
  (open) => {
    if (open) {
      ticket.value = ''
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

function onVerify() {
  emit('verify', ticket.value.trim())
}
</script>

<style scoped>
.auth-row {
  display: flex;
  align-items: center;
  gap: 16px;
}

.auth-row__label {
  width: 64px;
  flex-shrink: 0;
  font-size: 14px;
  color: #606266;
}
</style>
