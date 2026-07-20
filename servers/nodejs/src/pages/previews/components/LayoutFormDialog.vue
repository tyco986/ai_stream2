<template>
  <UiDialog
    :model-value="modelValue"
    :title="title"
    width="560px"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <div class="form">
      <div class="form__name">
        <span>{{ t('preview.name') }}</span>
        <UiInput :model-value="name" @update:model-value="emit('update:name', $event)" />
        <button
          type="button"
          class="form__shot"
          :title="t('preview.shotPreview')"
          :disabled="!shotUrl"
          @click="emit('preview-shot')"
        >
          <UiIcon name="picture" :size="16" />
        </button>
      </div>
      <div class="form__meta">
        <div>
          {{ t('preview.gridLayout') }}
          {{ layoutLabel }} ({{ t('preview.slotsCount', { n: slotCount }) }})
        </div>
        <div>{{ t('preview.view') }} {{ viewLabel }}</div>
        <div>{{ t('preview.streams') }} {{ t('preview.bound', { n: boundCount }) }}</div>
      </div>
      <table class="form__slots">
        <thead>
          <tr>
            <th>{{ t('preview.slot') }}</th>
            <th>{{ t('preview.stream') }}</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="(slot, index) in slotRows" :key="index">
            <td>{{ index }}</td>
            <td>{{ slot }}</td>
          </tr>
        </tbody>
      </table>
      <div v-if="error" class="form__error">{{ error }}</div>
    </div>
    <template #footer>
      <UiButton @click="emit('update:modelValue', false)">{{ t('preview.cancel') }}</UiButton>
      <UiButton type="primary" :loading="saving" @click="emit('submit')">
        {{ mode === 'create' ? t('preview.create') : t('preview.save') }}
      </UiButton>
    </template>
  </UiDialog>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import { LAYOUT_SLOT_COUNT, type PreviewLayout, type ViewMode } from '@/api/preview'
import UiButton from '@/shared/ui/Button.vue'
import UiDialog from '@/shared/ui/Dialog.vue'
import UiIcon from '@/shared/ui/Icon.vue'
import UiInput from '@/shared/ui/Input.vue'

const props = defineProps<{
  modelValue: boolean
  mode: 'create' | 'rename'
  name: string
  layout: PreviewLayout
  viewMode: ViewMode
  slots: (string | null)[]
  streamNames: Record<string, string>
  shotUrl: string
  saving: boolean
  error: string
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  'update:name': [value: string]
  submit: []
  'preview-shot': []
}>()

const { t } = useI18n()

const title = computed(() =>
  props.mode === 'create' ? t('preview.saveAsTitle') : t('preview.renameTitle'),
)

const layoutLabel = computed(() => props.layout.replace('x', '×'))
const viewLabel = computed(() =>
  props.viewMode === 'grid' ? t('preview.grid') : t('preview.focus'),
)
const slotCount = computed(() => LAYOUT_SLOT_COUNT[props.layout])
const boundCount = computed(
  () => props.slots.filter((slot) => slot !== null).length,
)
const slotRows = computed(() =>
  props.slots.map((id) => {
    if (!id) {
      return t('preview.empty')
    }
    return props.streamNames[id] ?? id
  }),
)
</script>

<style scoped>
.form {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.form__name {
  display: grid;
  grid-template-columns: 64px 1fr 36px;
  gap: 8px;
  align-items: center;
}

.form__shot {
  width: 32px;
  height: 32px;
  border: 1px solid #dcdfe6;
  border-radius: 4px;
  background: #fff;
  cursor: pointer;
  color: #606266;
}

.form__shot:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}

.form__meta {
  font-size: 13px;
  color: #606266;
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.form__slots {
  width: 100%;
  border-collapse: collapse;
  font-size: 13px;
}

.form__slots th,
.form__slots td {
  border: 1px solid #ebeef5;
  padding: 6px 8px;
  text-align: left;
}

.form__error {
  color: #f56c6c;
  font-size: 13px;
}
</style>
