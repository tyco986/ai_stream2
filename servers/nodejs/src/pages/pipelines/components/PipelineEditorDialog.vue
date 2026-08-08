<template>
  <UiDialog
    :model-value="modelValue"
    :title="t('pipelines.pipelineEditorTitle')"
    width="920px"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <div class="form">
      <label class="form__row">
        <span class="form__label">{{ t('pipelines.type') }}</span>
        <UiSelect
          :model-value="form.type"
          :options="typeOptions"
          :placeholder="t('pipelines.selectType')"
          @update:model-value="onTypeChange"
        />
      </label>
      <label class="form__row">
        <span class="form__label">{{ t('pipelines.name') }}</span>
        <UiInput
          :model-value="form.name"
          :invalid="nameInvalid"
          @update:model-value="onNameChange"
        />
      </label>

      <fieldset class="form__section" :disabled="!hasType">
        <legend>{{ t('pipelines.sectionPipeline') }}</legend>
        <div class="form__block" :class="{ 'is-disabled': !blockAvailable('pipeline', 'drawer') }">
          <div class="form__block-title">{{ t('pipelines.drawer') }}</div>
          <div class="form__grid">
            <label>
              <span>{{ t('pipelines.showLabel') }}</span>
              <UiSelect
                :model-value="boolStr(form.drawer.show_label)"
                :options="boolOptions"
                :disabled="!fieldAvailable('pipeline', 'drawer', 'show_label')"
                @update:model-value="form.drawer.show_label = $event === 'true'"
              />
            </label>
            <label>
              <span>{{ t('pipelines.showConf') }}</span>
              <UiSelect
                :model-value="boolStr(form.drawer.show_conf)"
                :options="boolOptions"
                :disabled="!fieldAvailable('pipeline', 'drawer', 'show_conf')"
                @update:model-value="form.drawer.show_conf = $event === 'true'"
              />
            </label>
            <label>
              <span>{{ t('pipelines.showId') }}</span>
              <UiSelect
                :model-value="boolStr(form.drawer.show_id)"
                :options="boolOptions"
                :disabled="!fieldAvailable('pipeline', 'drawer', 'show_id')"
                @update:model-value="form.drawer.show_id = $event === 'true'"
              />
            </label>
            <label>
              <span>{{ t('pipelines.interval') }}</span>
              <UiInput
                :model-value="String(form.drawer.interval)"
                :disabled="!fieldAvailable('pipeline', 'drawer', 'interval')"
                @update:model-value="form.drawer.interval = Number($event) || 0"
              />
            </label>
            <label>
              <span>{{ t('pipelines.fadeTime') }}</span>
              <UiInput
                :model-value="String(form.drawer.fade_time)"
                :disabled="!fieldAvailable('pipeline', 'drawer', 'fade_time')"
                @update:model-value="form.drawer.fade_time = Number($event) || 0"
              />
            </label>
          </div>
        </div>

        <div class="form__block" :class="{ 'is-disabled': !blockAvailable('pipeline', 'logger') }">
          <div class="form__block-title">{{ t('pipelines.logger') }}</div>
          <label class="form__inline">
            <span>{{ t('pipelines.interval') }}</span>
            <UiInput
              :model-value="String(form.logger.interval)"
              :disabled="!fieldAvailable('pipeline', 'logger', 'interval')"
              @update:model-value="form.logger.interval = Number($event) || 0"
            />
          </label>
        </div>

        <div class="form__block" :class="{ 'is-disabled': !blockAvailable('pipeline', 'messager') }">
          <div class="form__block-title">{{ t('pipelines.messager') }}</div>
          <label class="form__inline">
            <span>{{ t('pipelines.interval') }}</span>
            <UiInput
              :model-value="String(form.messager.interval)"
              :disabled="!fieldAvailable('pipeline', 'messager', 'interval')"
              @update:model-value="form.messager.interval = Number($event) || 0"
            />
          </label>
        </div>

        <div class="form__block" :class="{ 'is-disabled': !blockAvailable('pipeline', 'debouncer') }">
          <div class="form__block-title">{{ t('pipelines.debouncer') }}</div>
          <div class="form__grid">
            <label>
              <span>{{ t('pipelines.length') }}</span>
              <UiInput
                :model-value="String(form.debouncer.length)"
                :disabled="!blockAvailable('pipeline', 'debouncer')"
                @update:model-value="form.debouncer.length = Number($event) || 1"
              />
            </label>
            <label>
              <span>{{ t('pipelines.threshold') }}</span>
              <UiInput
                :model-value="String(form.debouncer.threshold)"
                :disabled="!blockAvailable('pipeline', 'debouncer')"
                @update:model-value="form.debouncer.threshold = Number($event) || 0"
              />
            </label>
            <label>
              <span>{{ t('pipelines.class') }}</span>
              <div class="form__pick">
                <UiInput :model-value="classIdsText(form.debouncer.class_ids)" :clearable="false" />
                <UiButton
                  :disabled="!blockAvailable('pipeline', 'debouncer')"
                  @click="openClassPicker('debouncer')"
                >
                  +
                </UiButton>
              </div>
            </label>
            <label>
              <span>{{ t('pipelines.mode') }}</span>
              <UiSelect
                :model-value="form.debouncer.mode"
                :options="modeOptions"
                :disabled="!blockAvailable('pipeline', 'debouncer')"
                @update:model-value="form.debouncer.mode = $event as 'slide' | 'fold'"
              />
            </label>
          </div>
        </div>
      </fieldset>

      <fieldset class="form__section" :disabled="!hasType">
        <legend>{{ t('pipelines.sectionGenerator') }}</legend>
        <div class="form__block" :class="{ 'is-disabled': !blockAvailable('generator', 'streams') }">
          <div class="form__block-title">{{ t('pipelines.streams') }}</div>
          <div class="form__pick">
            <UiInput :model-value="streamNamesText" :clearable="false" :invalid="streamsInvalid" />
            <UiButton
              :disabled="!blockAvailable('generator', 'streams')"
              @click="streamsPickOpen = true"
            >
              +
            </UiButton>
          </div>
        </div>

        <div class="form__block" :class="{ 'is-disabled': !blockAvailable('generator', 'pgie') }">
          <div class="form__block-title">{{ t('pipelines.pgie') }}</div>
          <UiSelect
            :model-value="form.gie_id"
            :options="gieOptions"
            :placeholder="t('pipelines.selectPgie')"
            :disabled="!blockAvailable('generator', 'pgie')"
            @update:model-value="form.gie_id = $event"
          />
        </div>

        <div class="form__block" :class="{ 'is-disabled': !blockAvailable('generator', 'interval') }">
          <div class="form__block-title">{{ t('pipelines.interval') }}</div>
          <UiInput
            :model-value="String(form.interval)"
            :disabled="!blockAvailable('generator', 'interval')"
            @update:model-value="form.interval = Number($event) || 0"
          />
        </div>

        <div class="form__block" :class="{ 'is-disabled': !blockAvailable('generator', 'tracker') }">
          <div class="form__block-head">
            <span class="form__block-title">{{ t('pipelines.tracker') }}</span>
            <UiCheckbox
              :model-value="trackerEnabled"
              :disabled="!blockAvailable('generator', 'tracker')"
              @update:model-value="trackerEnabled = $event"
            />
          </div>
          <div class="form__pick" :class="{ 'is-disabled': !trackerEnabled }">
            <UiInput :model-value="classIdsText(trackerClassIds)" :clearable="false" />
            <UiButton :disabled="!trackerEnabled" @click="openClassPicker('tracker')">+</UiButton>
          </div>
        </div>

        <div class="form__block" :class="{ 'is-disabled': !blockAvailable('generator', 'analyzer') }">
          <div class="form__block-head">
            <span class="form__block-title">{{ t('pipelines.analyzer') }}</span>
            <UiCheckbox
              :model-value="analyzerEnabled"
              :disabled="!blockAvailable('generator', 'analyzer')"
              @update:model-value="analyzerEnabled = $event"
            />
          </div>
          <div class="form__analyzer" :class="{ 'is-disabled': !analyzerEnabled }">
            <label class="form__inline">
              <span>{{ t('pipelines.streams') }}</span>
              <UiSelect
                :model-value="analyzerStreamSelect"
                :options="analyzerStreamOptions"
                :disabled="!analyzerEnabled"
                @update:model-value="onAnalyzerStreamPick"
              />
            </label>
            <label class="form__inline">
              <span>{{ t('pipelines.selectTemplate') }}</span>
              <UiSelect
                :model-value="form.analyzer.template"
                :options="analyzerTemplateOptions"
                :placeholder="t('pipelines.selectTemplate')"
                :disabled="!analyzerEnabled"
                @update:model-value="form.analyzer.template = $event"
              />
            </label>
            <label
              v-for="rule in analyzerRules"
              :key="rule.key"
              class="form__inline"
            >
              <span>{{ rule.label }}</span>
              <div class="form__pick">
                <UiInput
                  :model-value="classIdsText(analyzerRuleIds(rule.key))"
                  :clearable="false"
                />
                <UiButton :disabled="!analyzerEnabled" @click="openClassPicker(rule.key)">+</UiButton>
              </div>
            </label>
          </div>
        </div>

        <div class="form__block" :class="{ 'is-disabled': !blockAvailable('generator', 'sahi') }">
          <div class="form__block-title">{{ t('pipelines.sahi') }}</div>
          <div class="form__grid">
            <label>
              <span>{{ t('pipelines.sliceWidth') }}</span>
              <UiInput
                :model-value="String(form.sahi.nvsahipreprocess.slice_width)"
                :disabled="!blockAvailable('generator', 'sahi')"
                @update:model-value="form.sahi.nvsahipreprocess.slice_width = Number($event) || 0"
              />
            </label>
            <label>
              <span>{{ t('pipelines.sliceHeight') }}</span>
              <UiInput
                :model-value="String(form.sahi.nvsahipreprocess.slice_height)"
                :disabled="!blockAvailable('generator', 'sahi')"
                @update:model-value="form.sahi.nvsahipreprocess.slice_height = Number($event) || 0"
              />
            </label>
            <label>
              <span>{{ t('pipelines.overlapWidthRatio') }}</span>
              <UiInput
                :model-value="String(form.sahi.nvsahipreprocess.overlap_width_ratio)"
                :disabled="!blockAvailable('generator', 'sahi')"
                @update:model-value="
                  form.sahi.nvsahipreprocess.overlap_width_ratio = Number($event) || 0
                "
              />
            </label>
            <label>
              <span>{{ t('pipelines.overlapHeightRatio') }}</span>
              <UiInput
                :model-value="String(form.sahi.nvsahipreprocess.overlap_height_ratio)"
                :disabled="!blockAvailable('generator', 'sahi')"
                @update:model-value="
                  form.sahi.nvsahipreprocess.overlap_height_ratio = Number($event) || 0
                "
              />
            </label>
            <label>
              <span>{{ t('pipelines.matchThreshold') }}</span>
              <UiInput
                :model-value="String(form.sahi.nvsahipostprocess.match_threshold)"
                :disabled="!blockAvailable('generator', 'sahi')"
                @update:model-value="
                  form.sahi.nvsahipostprocess.match_threshold = Number($event) || 0
                "
              />
            </label>
          </div>
        </div>

        <div class="form__block" :class="{ 'is-disabled': !blockAvailable('generator', 'input') }">
          <div class="form__block-title">{{ t('pipelines.input') }}</div>
          <UiInput
            :model-value="form.input"
            :disabled="!blockAvailable('generator', 'input')"
            @update:model-value="form.input = $event"
          />
        </div>

        <div class="form__block" :class="{ 'is-disabled': !blockAvailable('generator', 'output') }">
          <div class="form__block-title">{{ t('pipelines.output') }}</div>
          <UiInput
            :model-value="form.output"
            :disabled="!blockAvailable('generator', 'output')"
            @update:model-value="form.output = $event"
          />
        </div>
      </fieldset>

    </div>

    <template #footer>
      <UiButton @click="emit('update:modelValue', false)">{{ t('pipelines.cancel') }}</UiButton>
      <UiButton type="primary" :disabled="!hasType" :loading="saving" @click="onSave">
        {{ t('pipelines.save') }}
      </UiButton>
    </template>
  </UiDialog>

  <ClassSelectDialog
    v-model="classPickOpen"
    :class-options="classOptions"
    :value="classPickValue"
    @confirm="onClassConfirm"
  />
  <StreamsPickDialog
    v-model="streamsPickOpen"
    :rows="streamCandidates"
    :value="form.streams"
    @confirm="onStreamsConfirm"
  />
</template>

<script setup lang="ts">
import { computed, reactive, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import type { ClassItem } from '@/api/models'
import { getModel } from '@/api/models'
import {
  createPipeline,
  getPipeline,
  getPipelineSchema,
  getPipelineTypes,
  listAnalyzerTemplates,
  listGieTemplates,
  updatePipeline,
  type CreatePipelineBody,
  type Debouncer,
  type Drawer,
  type PipelineAnalyzer,
  type SahiConfig,
  type TypeSchema,
} from '@/api/pipelines'
import { listStreams, type Stream } from '@/api/streams'
import { ApiError } from '@/shared/http/client'
import UiButton from '@/shared/ui/Button.vue'
import UiCheckbox from '@/shared/ui/Checkbox.vue'
import UiDialog from '@/shared/ui/Dialog.vue'
import UiInput from '@/shared/ui/Input.vue'
import UiSelect from '@/shared/ui/Select.vue'
import { toast } from '@/shared/ui/toast'
import ClassSelectDialog from './ClassSelectDialog.vue'
import StreamsPickDialog from './StreamsPickDialog.vue'

const props = defineProps<{
  modelValue: boolean
  mode: 'add' | 'edit'
  pipelineId: string | null
  saving?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  saved: []
}>()

const { t } = useI18n()

const schema = ref<TypeSchema | null>(null)
const typeOptions = ref<{ label: string; value: string }[]>([])
const gieOptions = ref<{ label: string; value: string }[]>([])
const analyzerTemplateOptions = ref<{ label: string; value: string }[]>([])
const streamCandidates = ref<Stream[]>([])
const streamNameMap = ref<Record<string, string>>({})
const classOptions = ref<ClassItem[]>([])
const nameInvalid = ref(false)
const streamsInvalid = ref(false)
const localSaving = ref(false)

const trackerEnabled = ref(false)
const analyzerEnabled = ref(false)
const trackerClassIds = ref<number[]>([-1])

const classPickOpen = ref(false)
const classPickTarget = ref<string>('debouncer')
const classPickValue = ref<number[]>([])
const streamsPickOpen = ref(false)

const DEFAULT_SAHI: SahiConfig = {
  nvsahipreprocess: {
    slice_width: 640,
    slice_height: 640,
    overlap_width_ratio: 0.2,
    overlap_height_ratio: 0.2,
  },
  nvsahipostprocess: {
    match_threshold: 0.5,
  },
}

const form = reactive({
  name: '',
  type: '',
  drawer: {
    show_label: true,
    show_conf: true,
    show_id: false,
    interval: 50,
    fade_time: 1,
  } as Drawer,
  logger: { interval: 50 },
  messager: { interval: 0 },
  debouncer: {
    length: 10,
    threshold: 0.5,
    class_ids: [] as number[],
    mode: 'fold' as Debouncer['mode'],
  },
  streams: [] as string[],
  gie_id: '',
  interval: 50,
  analyzer: {
    streams: [] as string[],
    template: '',
    roi_filtering: null as { class_id: number | number[] } | null,
    overcrowding: null as { class_id: number | number[] } | null,
    line_crossing: null as { class_id: number | number[] } | null,
    direction_detection: null as { class_id: number | number[] } | null,
  },
  sahi: structuredClone(DEFAULT_SAHI),
  input: '',
  output: '',
})

const hasType = computed(() => Boolean(form.type))
const saving = computed(() => Boolean(props.saving) || localSaving.value)

const boolOptions = computed(() => [
  { label: t('pipelines.true'), value: 'true' },
  { label: t('pipelines.false'), value: 'false' },
])

const modeOptions = computed(() => [
  { label: t('pipelines.modeSlide'), value: 'slide' },
  { label: t('pipelines.modeFold'), value: 'fold' },
])

const streamNamesText = computed(() =>
  form.streams.map((id) => streamNameMap.value[id] ?? id).join(', '),
)

const analyzerStreamOptions = computed(() =>
  form.streams.map((id) => ({
    label: streamNameMap.value[id] ?? id,
    value: id,
  })),
)

const analyzerStreamSelect = computed(() => form.analyzer.streams[0] ?? '')

const analyzerRules = computed(() => [
  { key: 'roi_filtering', label: t('pipelines.roiFiltering') },
  { key: 'overcrowding', label: t('pipelines.overcrowding') },
  { key: 'line_crossing', label: t('pipelines.lineCrossing') },
  { key: 'direction_detection', label: t('pipelines.directionDetection') },
])

watch(
  () => props.modelValue,
  async (open) => {
    if (!open) {
      return
    }
    nameInvalid.value = false
    streamsInvalid.value = false
    await loadLookups()
    if (props.mode === 'edit' && props.pipelineId) {
      await loadExisting(props.pipelineId)
    } else {
      resetForm()
    }
  },
)

async function loadLookups() {
  const [types, gies, analyzers, streams] = await Promise.all([
    getPipelineTypes(),
    listGieTemplates({ page: 1, page_size: 100 }),
    listAnalyzerTemplates({ page: 1, page_size: 100 }),
    listStreams({ page: 1, page_size: 200 }),
  ])
  typeOptions.value = types.items.map((item) => ({
    label: item.type,
    value: item.type,
  }))
  gieOptions.value = gies.items.map((item) => ({
    label: item.name,
    value: item.id,
  }))
  analyzerTemplateOptions.value = analyzers.items.map((item) => ({
    label: item.name,
    value: item.id,
  }))
  streamCandidates.value = streams.items.filter(
    (item) => item.enabled && item.status === 'online',
  )
  streamNameMap.value = Object.fromEntries(
    streams.items.map((item) => [item.id, item.name]),
  )
}

function resetForm() {
  form.name = ''
  form.type = ''
  schema.value = null
  trackerEnabled.value = false
  analyzerEnabled.value = false
  trackerClassIds.value = [-1]
  form.drawer = {
    show_label: true,
    show_conf: true,
    show_id: false,
    interval: 50,
    fade_time: 1,
  }
  form.logger = { interval: 50 }
  form.messager = { interval: 0 }
  form.debouncer = { length: 10, threshold: 0.5, class_ids: [], mode: 'fold' }
  form.streams = []
  form.gie_id = gieOptions.value[0]?.value ?? ''
  form.interval = 50
  form.analyzer = {
    streams: [],
    template: analyzerTemplateOptions.value[0]?.value ?? '',
    roi_filtering: null,
    overcrowding: null,
    line_crossing: null,
    direction_detection: null,
  }
  form.sahi = structuredClone(DEFAULT_SAHI)
  form.input = ''
  form.output = ''
}

async function loadExisting(pipelineId: string) {
  const pipeline = await getPipeline(pipelineId)
  form.name = pipeline.name
  form.type = pipeline.type
  form.drawer = {
    show_label: true,
    show_conf: true,
    show_id: false,
    interval: 50,
    fade_time: 1,
    ...pipeline.drawer,
  }
  form.logger = { ...pipeline.logger }
  form.messager = { ...pipeline.messager }
  form.debouncer = pipeline.debouncer
    ? { ...pipeline.debouncer, class_ids: [...pipeline.debouncer.class_ids] }
    : { length: 10, threshold: 0.5, class_ids: [], mode: 'fold' }
  form.streams = [...pipeline.streams]
  form.gie_id = pipeline.gie_id
  form.interval = pipeline.interval
  trackerEnabled.value = Boolean(pipeline.tracker)
  trackerClassIds.value = normalizeClassIds(pipeline.tracker?.class_id ?? [-1])
  analyzerEnabled.value = Boolean(pipeline.analyzer)
  form.analyzer = pipeline.analyzer
    ? {
        streams: [...pipeline.analyzer.streams],
        template: pipeline.analyzer.template,
        roi_filtering: pipeline.analyzer.roi_filtering,
        overcrowding: pipeline.analyzer.overcrowding,
        line_crossing: pipeline.analyzer.line_crossing,
        direction_detection: pipeline.analyzer.direction_detection,
      }
    : {
        streams: [],
        template: analyzerTemplateOptions.value[0]?.value ?? '',
        roi_filtering: null,
        overcrowding: null,
        line_crossing: null,
        direction_detection: null,
      }
  form.sahi = pipeline.sahi
    ? {
        nvsahipreprocess: { ...pipeline.sahi.nvsahipreprocess },
        nvsahipostprocess: { ...pipeline.sahi.nvsahipostprocess },
      }
    : structuredClone(DEFAULT_SAHI)
  form.input = pipeline.input ?? ''
  form.output = pipeline.output ?? ''
  schema.value = await getPipelineSchema({ pipeline_type: pipeline.type })
  await refreshClassOptions()
}

async function onTypeChange(value: string) {
  form.type = value
  if (!value) {
    schema.value = null
    return
  }
  schema.value = await getPipelineSchema({ pipeline_type: value })
  applySchemaDefaults()
  await refreshClassOptions()
}

function applySchemaDefaults() {
  if (!schema.value) {
    return
  }
  const drawer = schema.value.pipeline.params.drawer
  if (drawer?.params) {
    form.drawer.show_label = Boolean(asFieldDefault(drawer.params.show_label, true))
    form.drawer.show_conf = Boolean(asFieldDefault(drawer.params.show_conf, true))
    form.drawer.show_id = Boolean(asFieldDefault(drawer.params.show_id, false))
    form.drawer.interval = Number(asFieldDefault(drawer.params.interval, 50))
    form.drawer.fade_time = Number(asFieldDefault(drawer.params.fade_time, 1))
  }
  const logger = schema.value.pipeline.params.logger
  form.logger.interval = Number(asFieldDefault(logger?.params?.interval, 50))
  const messager = schema.value.pipeline.params.messager
  form.messager.interval = Number(asFieldDefault(messager?.params?.interval, 0))
  const debouncer = schema.value.pipeline.params.debouncer
  form.debouncer.length = Number(asFieldDefault(debouncer?.params?.length, 10))
  form.debouncer.threshold = Number(asFieldDefault(debouncer?.params?.threshold, 0.5))
  form.debouncer.class_ids = normalizeClassIds(
    asFieldDefault(debouncer?.params?.class_ids, []) as number[],
  )
  form.debouncer.mode = String(asFieldDefault(debouncer?.params?.mode, 'fold')) as Debouncer['mode']
  form.interval = Number(asFieldDefault(schema.value.generator.params.interval, 50))
  const tracker = schema.value.generator.params.tracker
  trackerEnabled.value = Boolean(tracker?.available)
  trackerClassIds.value = normalizeClassIds(
    asFieldDefault(tracker?.params?.class_id, [-1]) as number | number[],
  )
  const analyzer = schema.value.generator.params.analyzer
  analyzerEnabled.value = Boolean(analyzer?.available)
  const sahi = schema.value.generator.params.sahi
  if (sahi?.available && sahi.params) {
    const preprocess = asFieldDefault(sahi.params.nvsahipreprocess, DEFAULT_SAHI.nvsahipreprocess) as
      | SahiConfig['nvsahipreprocess']
      | null
    const postprocess = asFieldDefault(
      sahi.params.nvsahipostprocess,
      DEFAULT_SAHI.nvsahipostprocess,
    ) as SahiConfig['nvsahipostprocess'] | null
    form.sahi = {
      nvsahipreprocess: { ...(preprocess ?? DEFAULT_SAHI.nvsahipreprocess) },
      nvsahipostprocess: { ...(postprocess ?? DEFAULT_SAHI.nvsahipostprocess) },
    }
  } else {
    form.sahi = structuredClone(DEFAULT_SAHI)
  }
  const inputDefault = asFieldDefault(schema.value.generator.params.input, null)
  form.input = inputDefault == null ? '' : String(inputDefault)
  const outputDefault = asFieldDefault(schema.value.generator.params.output, null)
  form.output = outputDefault == null ? '' : String(outputDefault)
}

function asFieldDefault(field: unknown, fallback: unknown) {
  if (field && typeof field === 'object' && 'default' in field) {
    return (field as { default: unknown }).default
  }
  return fallback
}

function blockAvailable(side: 'pipeline' | 'generator', key: string) {
  const params = schema.value?.[side].params as Record<string, { available?: boolean }> | undefined
  return Boolean(params?.[key]?.available)
}

function fieldAvailable(side: 'pipeline' | 'generator', blockKey: string, fieldKey: string) {
  const params = schema.value?.[side].params as
    | Record<string, { available?: boolean; params?: Record<string, { available?: boolean }> }>
    | undefined
  const block = params?.[blockKey]
  if (!block?.available) {
    return false
  }
  const field = block.params?.[fieldKey]
  if (!field) {
    return true
  }
  return Boolean(field.available)
}

function boolStr(value: boolean) {
  return value ? 'true' : 'false'
}

function classIdsText(ids: number[]) {
  return ids.join(',')
}

function normalizeClassIds(value: number | number[]) {
  return Array.isArray(value) ? [...value] : [value]
}

function analyzerRuleIds(key: string) {
  const rule = form.analyzer[key as keyof PipelineAnalyzer]
  if (!rule || typeof rule !== 'object' || !('class_id' in rule) || !rule.class_id) {
    return []
  }
  return normalizeClassIds(rule.class_id)
}

function onAnalyzerStreamPick(value: string) {
  form.analyzer.streams = value ? [value] : []
}

async function refreshClassOptions() {
  if (!form.gie_id) {
    classOptions.value = []
    return
  }
  const gies = await listGieTemplates({ page: 1, page_size: 100 })
  const gie = gies.items.find((item) => item.id === form.gie_id)
  if (!gie) {
    classOptions.value = []
    return
  }
  const model = await getModel(gie.model_id)
  classOptions.value = model.classes ?? []
}

function openClassPicker(target: string) {
  classPickTarget.value = target
  if (target === 'debouncer') {
    classPickValue.value = [...form.debouncer.class_ids]
  } else if (target === 'tracker') {
    classPickValue.value = [...trackerClassIds.value]
  } else {
    classPickValue.value = analyzerRuleIds(target)
  }
  classPickOpen.value = true
}

function onClassConfirm(value: number[]) {
  if (classPickTarget.value === 'debouncer') {
    form.debouncer.class_ids = value
  } else if (classPickTarget.value === 'tracker') {
    trackerClassIds.value = value
  } else {
    const key = classPickTarget.value as keyof PipelineAnalyzer
    if (value.length === 0) {
      form.analyzer[key] = null as never
    } else {
      ;(form.analyzer as Record<string, unknown>)[key] = { class_id: value.length === 1 ? value[0] : value }
    }
  }
  classPickOpen.value = false
}

function onStreamsConfirm(value: string[]) {
  form.streams = value
  form.analyzer.streams = form.analyzer.streams.filter((id) => value.includes(id))
  streamsInvalid.value = false
  streamsPickOpen.value = false
}

function onNameChange(value: string) {
  form.name = value
  nameInvalid.value = false
}

async function onSave() {
  nameInvalid.value = !form.name.trim()
  streamsInvalid.value =
    blockAvailable('generator', 'streams') && form.streams.length === 0
  if (nameInvalid.value || streamsInvalid.value) {
    return
  }
  localSaving.value = true
  const body: CreatePipelineBody = {
    name: form.name.trim(),
    type: form.type,
    drawer: { ...form.drawer },
    logger: { ...form.logger },
    messager: { ...form.messager },
    debouncer: blockAvailable('pipeline', 'debouncer')
      ? { ...form.debouncer, class_ids: [...form.debouncer.class_ids] }
      : null,
    streams: [...form.streams],
    gie_id: form.gie_id,
    interval: form.interval,
    tracker: trackerEnabled.value
      ? {
          class_id:
            trackerClassIds.value.length === 1
              ? trackerClassIds.value[0]
              : [...trackerClassIds.value],
        }
      : null,
    analyzer: analyzerEnabled.value
      ? {
          streams: [...form.analyzer.streams],
          template: form.analyzer.template,
          roi_filtering: form.analyzer.roi_filtering,
          overcrowding: form.analyzer.overcrowding,
          line_crossing: form.analyzer.line_crossing,
          direction_detection: form.analyzer.direction_detection,
        }
      : null,
    sahi: blockAvailable('generator', 'sahi')
      ? {
          nvsahipreprocess: { ...form.sahi.nvsahipreprocess },
          nvsahipostprocess: { ...form.sahi.nvsahipostprocess },
        }
      : null,
    input: blockAvailable('generator', 'input') ? form.input.trim() || null : null,
    output: blockAvailable('generator', 'output') ? form.output.trim() || null : null,
  }
  try {
    if (props.mode === 'edit' && props.pipelineId) {
      await updatePipeline(props.pipelineId, body)
    } else {
      await createPipeline(body)
    }
    emit('saved')
  } catch (err) {
    const message = err instanceof ApiError ? err.message : String(err)
    if (err instanceof ApiError && err.status === 409) {
      nameInvalid.value = true
    }
    toast.error(message)
  }
  localSaving.value = false
}

watch(
  () => form.gie_id,
  () => {
    refreshClassOptions()
  },
)
</script>

<style scoped>
.form {
  display: flex;
  flex-direction: column;
  gap: 12px;
  max-height: calc(100vh - 200px);
  overflow: auto;
}

.form__row,
.form__inline {
  display: grid;
  grid-template-columns: 110px 1fr;
  align-items: center;
  gap: 8px;
}

.form__section {
  border: 1px solid #e4e7ed;
  border-radius: 6px;
  padding: 12px;
  margin: 0;
}

.form__section:disabled {
  opacity: 0.55;
}

.form__block {
  margin-bottom: 12px;
}

.form__block.is-disabled,
.form__pick.is-disabled,
.form__analyzer.is-disabled {
  opacity: 0.55;
  pointer-events: none;
}

.form__block-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  margin-bottom: 8px;
}

.form__label,
.form__block-title {
  font-weight: 600;
}

.form__block > .form__block-title {
  margin-bottom: 8px;
}

.form__grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
}

.form__grid label,
.form__analyzer label {
  display: grid;
  gap: 4px;
}

.form__pick {
  display: flex;
  gap: 8px;
  align-items: center;
}

.form__pick :deep(.el-input) {
  flex: 1;
}

.form__analyzer {
  display: flex;
  flex-direction: column;
  gap: 8px;
  margin-top: 8px;
}

</style>
