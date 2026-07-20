<template>
  <div class="preset-bar">
    <UiSelect
      class="preset-bar__layout"
      :model-value="layout"
      :options="layoutOptions"
      @update:model-value="emit('update:layout', $event as RecordingsLayout)"
    />
    <UiSelect
      class="preset-bar__preset"
      :model-value="presetSelectValue"
      :options="presetOptions"
      @update:model-value="onPresetChange"
    />
    <button
      type="button"
      class="preset-bar__icon"
      :title="t('recordings.save')"
      :disabled="!dirty || saving"
      @click="emit('save')"
    >
      <UiIcon name="save" :size="16" />
    </button>
    <button
      type="button"
      class="preset-bar__icon"
      :title="t('recordings.saveAs')"
      :disabled="saving"
      @click="emit('save-as')"
    >
      <UiIcon name="saveAs" :size="16" />
    </button>
    <button
      type="button"
      class="preset-bar__icon"
      :title="t('recordings.clear')"
      :disabled="!canClear"
      @click="emit('clear')"
    >
      <UiIcon name="clear" :size="16" />
    </button>
    <div class="preset-bar__spacer" />
    <div class="preset-bar__mode" aria-hidden="true">
      <button type="button" class="preset-bar__icon is-active" tabindex="-1" disabled>
        <UiIcon name="grid" :size="16" />
      </button>
      <button type="button" class="preset-bar__icon" tabindex="-1" disabled>
        <UiIcon name="focus" :size="16" />
      </button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import type { LayoutPresetSummary, RecordingsLayout } from '@/api/recordings'
import UiIcon from '@/shared/ui/Icon.vue'
import UiSelect from '@/shared/ui/Select.vue'

const MANAGE_VALUE = '__manage__'

const props = defineProps<{
  layout: RecordingsLayout
  presetId: string
  dirty: boolean
  saving: boolean
  presets: LayoutPresetSummary[]
  canClear: boolean
}>()

const emit = defineEmits<{
  'update:layout': [value: RecordingsLayout]
  'change-preset': [presetId: string]
  save: []
  'save-as': []
  clear: []
  manage: []
}>()

const { t } = useI18n()
const presetSelectValue = ref(props.presetId)

watch(
  () => props.presetId,
  (value) => {
    presetSelectValue.value = value
  },
)

const layoutOptions = [
  { label: '1×1', value: '1x1' },
  { label: '2×2', value: '2x2' },
  { label: '3×3', value: '3x3' },
  { label: '4×4', value: '4x4' },
]

const presetOptions = computed(() => [
  ...props.presets.map((item) => ({
    label:
      item.id === props.presetId && props.dirty
        ? `${item.name} *`
        : item.name,
    value: item.id,
  })),
  { label: t('recordings.edit'), value: MANAGE_VALUE, color: '#409eff' },
])

function onPresetChange(value: string) {
  if (value === MANAGE_VALUE) {
    presetSelectValue.value = props.presetId
    emit('manage')
    return
  }
  presetSelectValue.value = value
  emit('change-preset', value)
}
</script>

<style scoped>
.preset-bar {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  border-bottom: 1px solid #e4e7ed;
  background: #fff;
}

.preset-bar__layout {
  width: 96px;
}

.preset-bar__preset {
  width: 180px;
}

.preset-bar__spacer {
  flex: 1;
}

.preset-bar__mode {
  display: flex;
  gap: 4px;
  visibility: hidden;
}

.preset-bar__icon {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 32px;
  height: 32px;
  border: 1px solid #dcdfe6;
  border-radius: 4px;
  background: #fff;
  cursor: pointer;
  color: #606266;
}

.preset-bar__icon:disabled {
  opacity: 0.45;
  cursor: not-allowed;
}

.preset-bar__icon.is-active,
.preset-bar__icon:hover:not(:disabled) {
  color: #409eff;
  border-color: #c6e2ff;
  background: #ecf5ff;
}
</style>
