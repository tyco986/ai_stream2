<template>
  <div class="preset-bar">
    <UiSelect
      class="preset-bar__layout"
      :model-value="layout"
      :options="layoutOptions"
      @update:model-value="emit('update:layout', $event as PreviewLayout)"
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
      :title="t('preview.save')"
      :disabled="!dirty || saving"
      @click="emit('save')"
    >
      <UiIcon name="save" :size="16" />
    </button>
    <button
      type="button"
      class="preset-bar__icon"
      :title="t('preview.saveAs')"
      :disabled="saving"
      @click="emit('save-as')"
    >
      <UiIcon name="saveAs" :size="16" />
    </button>
    <button
      type="button"
      class="preset-bar__icon"
      :title="t('preview.clear')"
      :disabled="!canClear"
      @click="emit('clear')"
    >
      <UiIcon name="clear" :size="16" />
    </button>
    <div class="preset-bar__group">
      <button
        type="button"
        class="preset-bar__icon"
        :class="{ 'is-active': viewMode === 'grid' }"
        :title="t('preview.grid')"
        @click="emit('update:viewMode', 'grid')"
      >
        <UiIcon name="grid" :size="16" />
      </button>
      <button
        type="button"
        class="preset-bar__icon"
        :class="{ 'is-active': viewMode === 'focus' }"
        :title="t('preview.focus')"
        @click="emit('update:viewMode', 'focus')"
      >
        <UiIcon name="focus" :size="16" />
      </button>
    </div>
    <div class="preset-bar__group">
      <button
        type="button"
        class="preset-bar__icon"
        :title="allPaused ? t('preview.playAll') : t('preview.pauseAll')"
        @click="emit('toggle-playback')"
      >
        <UiIcon :name="allPaused ? 'videoPlay' : 'videoPause'" :size="16" />
      </button>
      <button
        type="button"
        class="preset-bar__icon"
        :title="t('preview.fullScreen')"
        @click="emit('fullscreen')"
      >
        <UiIcon name="fullScreen" :size="16" />
      </button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import type { LayoutPresetSummary, PreviewLayout, ViewMode } from '@/api/preview'
import UiIcon from '@/shared/ui/Icon.vue'
import UiSelect from '@/shared/ui/Select.vue'

const MANAGE_VALUE = '__manage__'

const props = defineProps<{
  layout: PreviewLayout
  viewMode: ViewMode
  presetId: string
  dirty: boolean
  saving: boolean
  presets: LayoutPresetSummary[]
  canClear: boolean
  allPaused: boolean
}>()

const emit = defineEmits<{
  'update:layout': [value: PreviewLayout]
  'update:viewMode': [value: ViewMode]
  'change-preset': [presetId: string]
  save: []
  'save-as': []
  clear: []
  manage: []
  'toggle-playback': []
  fullscreen: []
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
  { label: t('preview.edit'), value: MANAGE_VALUE, color: '#409eff' },
])

function onPresetChange(value: string) {
  if (value === MANAGE_VALUE) {
    presetSelectValue.value = props.presetId
    emit('manage')
    return
  }
  // Keep current selection until parent applies presetId (e.g. after discard confirm).
  presetSelectValue.value = props.presetId
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

.preset-bar__group {
  display: flex;
  gap: 4px;
  padding-left: 8px;
  border-left: 1px solid #dcdfe6;
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
