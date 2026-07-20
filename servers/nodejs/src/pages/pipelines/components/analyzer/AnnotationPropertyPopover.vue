<template>
  <div
    v-if="modelValue"
    ref="popoverRef"
    class="popover"
    :style="popoverStyle"
    @mousedown.stop
    @click.stop
    @keydown.enter="onConfirm"
    @keydown.escape="onCancel"
  >
    <div class="popover__row">
      <UiInput
        :model-value="name"
        :placeholder="t('pipelines.annotationName')"
        :invalid="nameInvalid"
        @update:model-value="onNameChange"
      />
      <ConfirmCancelButtons
        :confirm-disabled="!hasName"
        @confirm="onConfirm"
        @cancel="onCancel"
      />
    </div>

    <template v-if="showThresholds">
      <div class="popover__row popover__row--field">
        <span>{{ t('pipelines.objectThreshold') }}</span>
        <UiInput
          :model-value="objectThreshold"
          :invalid="thresholdInvalid"
          @update:model-value="onObjectThresholdChange"
        />
      </div>
      <div class="popover__row popover__row--field">
        <span>{{ t('pipelines.timeThreshold') }}</span>
        <UiInput
          :model-value="timeThreshold"
          :invalid="thresholdInvalid"
          @update:model-value="onTimeThresholdChange"
        />
        <span class="popover__hint">ms</span>
      </div>
    </template>

    <div v-if="showExtend" class="popover__row popover__row--field">
      <span>{{ t('pipelines.extend') }}</span>
      <UiCheckbox :model-value="extend" @update:model-value="extend = $event">
        {{ extend ? t('pipelines.extendOn') : t('pipelines.extendOff') }}
      </UiCheckbox>
    </div>

    <div v-if="showMode" class="popover__row popover__row--field popover__row--top">
      <span>{{ t('pipelines.mode') }}</span>
      <UiRadioGroup
        :model-value="mode"
        :options="modeOptions"
        @update:model-value="mode = String($event)"
      />
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, nextTick, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import type { Annotation } from '@/api/pipelines'
import { pointsFromClicks } from '../../composables/annotationGeometry'
import ConfirmCancelButtons from '@/shared/ui/ConfirmCancelButtons.vue'
import UiCheckbox from '@/shared/ui/Checkbox.vue'
import UiInput from '@/shared/ui/Input.vue'
import UiRadioGroup from '@/shared/ui/RadioGroup.vue'
import { toast } from '@/shared/ui/toast'

export type AnnotationDraftInput = {
  id?: string
  name?: string
  type: Annotation['type']
  shape: Annotation['shape']
  points: number[]
  click_points: number[]
  object_threshold?: number | null
  time_threshold?: number | null
  extend?: boolean | null
  mode?: string | null
}

const POPOVER_OFFSET = 12
const POPOVER_WIDTH = 280

const props = defineProps<{
  modelValue: boolean
  draft: AnnotationDraftInput | null
  existingNames: string[]
  anchorX: number
  anchorY: number
  containerWidth: number
  containerHeight: number
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  confirm: [annotation: Annotation]
  cancel: []
}>()

const { t } = useI18n()
const popoverRef = ref<HTMLElement | null>(null)
const name = ref('')
const objectThreshold = ref('5')
const timeThreshold = ref('3000')
const extend = ref(false)
const mode = ref('balanced')
const popoverHeight = ref(48)
const nameInvalid = ref(false)
const thresholdInvalid = ref(false)

const modeOptions = [
  { label: 'strict', value: 'strict' },
  { label: 'balanced', value: 'balanced' },
  { label: 'loose', value: 'loose' },
]

const showThresholds = computed(
  () => props.draft?.type === 'overcrowding' && props.draft.shape === 'polygon',
)
const showExtend = computed(() => props.draft?.type === 'line_crossing')
const showMode = computed(
  () =>
    props.draft?.type === 'line_crossing' || props.draft?.type === 'direction_detection',
)

const trimmedName = computed(() => name.value.trim())
const hasName = computed(() => trimmedName.value.length > 0)

const popoverStyle = computed(() => {
  const maxLeft = Math.max(0, props.containerWidth - POPOVER_WIDTH - 8)
  const maxTop = Math.max(0, props.containerHeight - popoverHeight.value - 8)
  const left = Math.min(Math.max(props.anchorX + POPOVER_OFFSET, 8), maxLeft)
  const top = Math.min(Math.max(props.anchorY + POPOVER_OFFSET, 8), maxTop)
  return {
    left: `${left}px`,
    top: `${top}px`,
    width: `${POPOVER_WIDTH}px`,
  }
})

watch(
  () => props.modelValue,
  async (open) => {
    if (!open || !props.draft) {
      return
    }
    name.value = props.draft.name ?? ''
    objectThreshold.value = String(props.draft.object_threshold ?? 5)
    timeThreshold.value = String(props.draft.time_threshold ?? 3000)
    extend.value = props.draft.extend ?? false
    mode.value = props.draft.mode ?? 'balanced'
    nameInvalid.value = false
    thresholdInvalid.value = false
    await nextTick()
    popoverHeight.value = popoverRef.value?.offsetHeight ?? 48
    const input = popoverRef.value?.querySelector('input')
    input?.focus()
  },
)

function onNameChange(value: string) {
  name.value = value
  nameInvalid.value = false
}

function onObjectThresholdChange(value: string) {
  objectThreshold.value = value
  thresholdInvalid.value = false
}

function onTimeThresholdChange(value: string) {
  timeThreshold.value = value
  thresholdInvalid.value = false
}

function isDuplicateName() {
  return props.existingNames.some(
    (item) => item.toLowerCase() === trimmedName.value.toLowerCase(),
  )
}

function isThresholdValid() {
  if (!showThresholds.value) {
    return true
  }
  const objectValue = Number(objectThreshold.value)
  const timeValue = Number(timeThreshold.value)
  return (
    Number.isInteger(objectValue) &&
    objectValue >= 1 &&
    Number.isInteger(timeValue) &&
    timeValue >= 1
  )
}

function onConfirm() {
  if (!props.draft || !hasName.value) {
    return
  }
  if (isDuplicateName()) {
    nameInvalid.value = true
    toast.error(t('pipelines.annotationNameDuplicate'))
    return
  }
  if (!isThresholdValid()) {
    thresholdInvalid.value = true
    toast.error(t('pipelines.thresholdInvalid'))
    return
  }
  const clicks = [...props.draft.click_points]
  const annotation: Annotation = {
    id: props.draft.id ?? crypto.randomUUID(),
    name: trimmedName.value,
    type: props.draft.type,
    shape: props.draft.shape,
    points: pointsFromClicks(props.draft.shape, clicks),
    click_points: clicks,
    object_threshold: showThresholds.value ? Number(objectThreshold.value) : null,
    time_threshold: showThresholds.value ? Number(timeThreshold.value) : null,
    extend: showExtend.value ? extend.value : null,
    mode: showMode.value ? (mode.value as Annotation['mode']) : null,
  }
  emit('confirm', annotation)
  emit('update:modelValue', false)
}

function onCancel() {
  emit('cancel')
  emit('update:modelValue', false)
}
</script>

<style scoped>
.popover {
  position: absolute;
  z-index: 20;
  padding: 8px;
  border: 1px solid #dcdfe6;
  border-radius: 6px;
  background: #fff;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.12);
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.popover__row {
  display: grid;
  grid-template-columns: 1fr 28px 28px;
  gap: 6px;
  align-items: center;
}

.popover__row--field {
  grid-template-columns: 88px 1fr auto;
}

.popover__row--top {
  align-items: start;
}

.popover__hint {
  color: #909399;
  font-size: 12px;
}
</style>
