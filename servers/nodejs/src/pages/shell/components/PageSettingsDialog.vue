<template>
  <UiDialog
    :model-value="modelValue"
    :title="t('shell.pageSettings')"
    @update:model-value="onVisibleChange"
  >
    <div class="settings-row">
      <label class="settings-row__label">{{ t('shell.mode') }}</label>
      <UiSelect v-model="draftMode" :options="modeOptions" />
    </div>
    <template #footer>
      <UiButton :disabled="applying" @click="onCancel">
        {{ t('shell.cancel') }}
      </UiButton>
      <UiButton type="primary" :loading="applying" @click="onApply">
        {{ t('shell.apply') }}
      </UiButton>
    </template>
  </UiDialog>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import type { ShellMode } from '@/api/shell'
import UiButton from '@/shared/ui/Button.vue'
import UiDialog from '@/shared/ui/Dialog.vue'
import UiSelect from '@/shared/ui/Select.vue'

const props = defineProps<{
  modelValue: boolean
  appliedMode: ShellMode
  applying?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  apply: [mode: ShellMode]
  cancel: []
}>()

const { t } = useI18n()
const draftMode = ref<ShellMode>(props.appliedMode)

const modeOptions = computed(() => [
  { label: t('shell.modeSidebar'), value: 'sidebar' },
  { label: t('shell.modeTopbar'), value: 'topbar' },
])

watch(
  () => props.modelValue,
  (open) => {
    if (open) {
      draftMode.value = props.appliedMode
    }
  },
)

function onVisibleChange(value: boolean) {
  if (!value) {
    draftMode.value = props.appliedMode
    emit('cancel')
  }
  emit('update:modelValue', value)
}

function onCancel() {
  draftMode.value = props.appliedMode
  emit('cancel')
  emit('update:modelValue', false)
}

function onApply() {
  emit('apply', draftMode.value)
}
</script>

<style scoped>
.settings-row {
  display: flex;
  align-items: center;
  gap: 16px;
}

.settings-row__label {
  width: 64px;
  flex-shrink: 0;
  font-size: 14px;
  color: #606266;
}
</style>
