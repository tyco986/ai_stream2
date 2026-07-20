<template>
  <UiDialog
    :model-value="modelValue"
    :title="t('pipelines.analyzerEditorTitle')"
    width="1240px"
    :close-on-click-modal="false"
    :before-close="onBeforeClose"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <div class="editor">
      <div class="editor__top">
        <label class="editor__field">
          <span>{{ t('pipelines.name') }}</span>
          <UiInput
            :model-value="name"
            :invalid="nameInvalid"
            @update:model-value="onNameChange"
          />
        </label>
        <label class="editor__field editor__field--source">
          <span>{{ t('pipelines.source') }}</span>
          <UiSelect
            :model-value="sourceSelectValue"
            :groups="sourceGroups"
            :placeholder="t('pipelines.selectSource')"
            :invalid="sourceInvalid"
            @change="onSourceChange"
          />
          <button
            type="button"
            class="icon-btn"
            :title="t('pipelines.refreshSource')"
            :disabled="!hasSource || sourceLoading"
            @click="onRefreshSource"
          >
            <UiIcon name="refresh" :size="16" />
          </button>
          <input
            ref="fileInputRef"
            type="file"
            accept=".jpg,.jpeg,.png,image/jpeg,image/png"
            class="file-input"
            @change="onFilePicked"
          />
        </label>
      </div>

      <div class="editor__body">
        <div class="editor__main">
          <div class="editor__toolbar">
            <label class="editor__field editor__field--mode">
              <span>{{ t('pipelines.mode') }}</span>
              <UiSelect
                :model-value="annotationMode"
                :options="modeOptions"
                @update:model-value="onModeChange"
              />
            </label>
            <div class="tools">
              <button
                v-for="item in toolButtons"
                :key="item.tool"
                type="button"
                class="tools__btn"
                :class="{ 'is-active': tool === item.tool }"
                :disabled="isToolDisabled(item.tool)"
                :title="item.title"
                @click="onToolChange(item.tool)"
              >
                <UiIcon :name="item.icon" :size="16" />
              </button>
            </div>
          </div>

          <div class="editor__canvas">
            <AnalyzerCanvas
              :image-url="displayImageUrl"
              :image-width="configWidth"
              :image-height="configHeight"
              :annotations="annotations"
              :selected-id="selectedId"
              :edit-mode="tool === 'edit'"
              :transform="transform"
              :crosshair-visible="crosshairVisible"
              :crosshair-lines="crosshairLines"
              :draft-color="draftStrokeColor"
              :draft-preview="draftPreview"
              :draft-handle-points="draftHandlePoints"
              :polygon-close-highlight="polygonCloseHighlight"
              :pending-draft="pendingDraft"
              @mousemove="onMouseMove"
              @mouseleave="onMouseLeave"
              @mousedown="onMouseDown"
              @select="onSelectAnnotation"
              @move-selected="onMoveSelected"
              @move-anchor="onMoveAnchor"
              @resize="onStageResize"
            />
            <AnnotationPropertyPopover
              v-model="propertyOpen"
              :draft="propertyDraft"
              :existing-names="propertyExistingNames"
              :anchor-x="propertyPopoverX"
              :anchor-y="propertyPopoverY"
              :container-width="stageWidth"
              :container-height="stageHeight"
              @confirm="onPropertyConfirm"
              @cancel="onPropertyCancel"
            />
          </div>
        </div>

        <div class="editor__list">
          <UiTable
            :data="annotations"
            row-key="id"
            highlight-current-row
            :current-row-key="selectedId ?? undefined"
            @row-click="onRowClick"
          >
            <UiTableColumn prop="name" :label="t('pipelines.annotationName')" min-width="120" />
            <UiTableColumn :label="t('pipelines.annotationType')" min-width="140">
              <template #default="{ row }">
                {{ typeLabel(row.type) }}
              </template>
            </UiTableColumn>
            <UiTableColumn :label="t('pipelines.operations')" width="100">
              <template #default="{ row }">
                <div class="ops">
                  <button type="button" class="ops__btn" @click.stop="openEditProps(row)">
                    <UiIcon name="edit" :size="16" />
                  </button>
                  <button
                    type="button"
                    class="ops__btn is-danger"
                    @click.stop="askDeleteAnnotation(row)"
                  >
                    <UiIcon name="delete" :size="16" />
                  </button>
                </div>
              </template>
            </UiTableColumn>
          </UiTable>
        </div>
      </div>

    </div>

    <template #footer>
      <UiButton @click="requestClose()">{{ t('pipelines.cancel') }}</UiButton>
      <UiButton type="primary" :loading="saving" @click="onSave">{{ t('pipelines.save') }}</UiButton>
    </template>
  </UiDialog>

  <UiDialog
    :model-value="confirmOpen"
    :title="t('pipelines.confirmTitle')"
    width="420px"
    :close-on-click-modal="false"
    @update:model-value="onConfirmVisible"
  >
    <p>{{ confirmMessage }}</p>
    <template #footer>
      <UiButton @click="onConfirmVisible(false)">{{ t('pipelines.cancel') }}</UiButton>
      <UiButton type="danger" @click="onConfirmAction">{{ t('pipelines.confirm') }}</UiButton>
    </template>
  </UiDialog>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import {
  createAnalyzerPlaceholderDataUrl,
  createAnalyzerTemplate,
  getAnalyzerTemplate,
  previewAnalyzerSourceFile,
  previewAnalyzerSourceStream,
  updateAnalyzerTemplate,
  type Annotation,
} from '@/api/pipelines'
import { listStreams } from '@/api/streams'
import { ApiError } from '@/shared/http/client'
import UiButton from '@/shared/ui/Button.vue'
import UiDialog from '@/shared/ui/Dialog.vue'
import UiIcon from '@/shared/ui/Icon.vue'
import type { UiIconName } from '@/shared/ui/Icon.vue'
import UiInput from '@/shared/ui/Input.vue'
import UiSelect from '@/shared/ui/Select.vue'
import UiTable from '@/shared/ui/Table.vue'
import UiTableColumn from '@/shared/ui/TableColumn.vue'
import { toast } from '@/shared/ui/toast'
import {
  getClickPoints,
  normalizeAnnotation,
} from '../composables/annotationGeometry'
import { useAnalyzerCanvas } from '../composables/useAnalyzerCanvas'
import {
  colorForAnnotationType,
  defaultToolForMode,
  isToolAllowed,
  pixelToStage,
  type AnnotationModeType,
  type DrawTool,
} from '../composables/useAnalyzerGeometry'
import { useAnnotationEdit } from '../composables/useAnnotationEdit'
import AnalyzerCanvas from './analyzer/AnalyzerCanvas.vue'
import AnnotationPropertyPopover, {
  type AnnotationDraftInput,
} from './analyzer/AnnotationPropertyPopover.vue'

const CHOOSE_FILE = '__choose_file__'
const NO_STREAMS = '__no_streams__'

const props = defineProps<{
  modelValue: boolean
  mode: 'add' | 'edit'
  analyzerId: string | null
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  saved: []
}>()

const { t } = useI18n()

const name = ref('')
const sourceKind = ref<'file' | 'stream' | null>(null)
const sourceFileName = ref<string | null>(null)
const sourceStreamId = ref<string | null>(null)
const sourceImageUrl = ref('')
const displayImageUrl = ref('')
const configWidth = ref(1920)
const configHeight = ref(1080)
const annotations = ref<Annotation[]>([])
const selectedId = ref<string | null>(null)
const annotationMode = ref<AnnotationModeType>('roi_filtering')
const tool = ref<DrawTool>('rectangle')
const stageWidth = ref(640)
const stageHeight = ref(360)
const onlineStreams = ref<{ id: string; name: string }[]>([])
const fileInputRef = ref<HTMLInputElement | null>(null)
const sourceLoading = ref(false)
const saving = ref(false)
const nameInvalid = ref(false)
const sourceInvalid = ref(false)
const dirty = ref(false)
const retainedFile = ref<File | null>(null)

const propertyOpen = ref(false)
const propertyDraft = ref<AnnotationDraftInput | null>(null)
const propertyPopoverX = ref(0)
const propertyPopoverY = ref(0)
const editingAnnotationId = ref<string | null>(null)

const confirmOpen = ref(false)
const confirmMessage = ref('')
const confirmAction = ref<'discard' | 'delete' | 'resolution' | null>(null)
const pendingDeleteId = ref<string | null>(null)
const pendingSourceApply = ref<null | (() => void)>(null)
const pendingCloseDone = ref<(() => void) | null>(null)

const hasSource = computed(() => Boolean(displayImageUrl.value))

const modeOptions = computed(() => [
  { label: t('pipelines.modeRoiFiltering'), value: 'roi_filtering' },
  { label: t('pipelines.modeOvercrowding'), value: 'overcrowding' },
  { label: t('pipelines.modeLineCrossing'), value: 'line_crossing' },
  { label: t('pipelines.modeDirectionDetection'), value: 'direction_detection' },
])

const toolButtons = computed(() => {
  const items: { tool: DrawTool; icon: UiIconName; title: string }[] = [
    { tool: 'rectangle', icon: 'crop', title: t('pipelines.toolRectangle') },
    { tool: 'polygon', icon: 'share', title: t('pipelines.toolPolygon') },
    { tool: 'line_direction', icon: 'fullScreen', title: t('pipelines.toolLineDirection') },
    { tool: 'direction', icon: 'arrowRight', title: t('pipelines.toolDirection') },
    { tool: 'edit', icon: 'editPen', title: t('pipelines.toolEdit') },
  ]
  return items
})

const sourceSelectValue = computed(() => {
  if (sourceKind.value === 'file' && sourceFileName.value) {
    return `file:${sourceFileName.value}`
  }
  if (sourceKind.value === 'stream' && sourceStreamId.value) {
    return sourceStreamId.value
  }
  return ''
})

const sourceGroups = computed(() => {
  const fileOptions = [
    { label: t('pipelines.chooseFile'), value: CHOOSE_FILE },
  ]
  if (sourceKind.value === 'file' && sourceFileName.value) {
    fileOptions.unshift({
      label: sourceFileName.value,
      value: `file:${sourceFileName.value}`,
    })
  }
  const streamOptions =
    onlineStreams.value.length > 0
      ? onlineStreams.value.map((item) => ({ label: item.name, value: item.id }))
      : [{ label: t('pipelines.noOnlineStreams'), value: NO_STREAMS, disabled: true }]
  return [
    { label: t('pipelines.sourceFile'), options: fileOptions },
    { label: t('pipelines.sourceStream'), options: streamOptions },
  ]
})

const propertyExistingNames = computed(() =>
  annotations.value
    .filter((item) => item.id !== editingAnnotationId.value)
    .map((item) => item.name),
)

const draftStrokeColor = computed(() => colorForAnnotationType(annotationMode.value))
const canvasEnabled = computed(() => !propertyOpen.value)

const {
  transform,
  crosshairVisible,
  crosshairLines,
  draftPreview,
  draftHandlePoints,
  polygonCloseHighlight,
  pendingDraft,
  resetDraft,
  clearPending,
  onMouseMove,
  onMouseLeave,
  onMouseDown,
} = useAnalyzerCanvas({
  stageWidth,
  stageHeight,
  imageWidth: configWidth,
  imageHeight: configHeight,
  hasImage: hasSource,
  enabled: canvasEnabled,
  tool,
  annotationType: annotationMode,
  onDraftComplete: (draft) => {
    editingAnnotationId.value = null
    propertyDraft.value = {
      type: draft.type,
      shape: draft.shape,
      points: draft.points,
      click_points: draft.clickPoints,
    }
    propertyPopoverX.value = draft.popoverX
    propertyPopoverY.value = draft.popoverY
    propertyOpen.value = true
  },
})

const { selectAnnotation, applyDeltaToSelected, moveAnchor } = useAnnotationEdit({
  annotations,
  selectedId,
  transform,
  imageWidth: configWidth,
  imageHeight: configHeight,
})

watch(
  () => props.modelValue,
  async (open) => {
    if (!open) {
      return
    }
    await bootstrap()
  },
)

async function bootstrap() {
  nameInvalid.value = false
  sourceInvalid.value = false
  dirty.value = false
  selectedId.value = null
  resetDraft()
  clearPending()
  propertyOpen.value = false
  propertyDraft.value = null
  annotationMode.value = 'roi_filtering'
  tool.value = 'rectangle'
  retainedFile.value = null
  const streams = await listStreams({ page: 1, page_size: 200 })
  onlineStreams.value = streams.items
    .filter((item) => item.enabled && item.status === 'online')
    .map((item) => ({ id: item.id, name: item.name }))
  if (props.mode === 'edit' && props.analyzerId) {
    const item = await getAnalyzerTemplate(props.analyzerId)
    name.value = item.name
    sourceKind.value = item.source_kind
    sourceFileName.value = item.source_file_name
    sourceStreamId.value = item.source_stream_id
    sourceImageUrl.value = item.source_image_url
    configWidth.value = item.config_width
    configHeight.value = item.config_height
    annotations.value = item.annotations.map((ann) => normalizeAnnotation(ann))
    displayImageUrl.value = await resolveDisplayImage(item.source_image_url, item.name)
  } else {
    name.value = ''
    sourceKind.value = null
    sourceFileName.value = null
    sourceStreamId.value = null
    sourceImageUrl.value = ''
    displayImageUrl.value = ''
    configWidth.value = 1920
    configHeight.value = 1080
    annotations.value = []
  }
}

async function resolveDisplayImage(url: string, label: string) {
  if (url.startsWith('data:') || url.startsWith('blob:')) {
    return url
  }
  const ok = await probeImage(url)
  if (ok) {
    return url
  }
  return createAnalyzerPlaceholderDataUrl(configWidth.value, configHeight.value, label)
}

function probeImage(url: string): Promise<boolean> {
  return new Promise((resolve) => {
    const img = new Image()
    img.onload = () => resolve(true)
    img.onerror = () => resolve(false)
    img.src = url
  })
}

function markDirty() {
  dirty.value = true
}

function onNameChange(value: string) {
  name.value = value
  nameInvalid.value = false
  markDirty()
}

function typeLabel(type: Annotation['type']) {
  const map: Record<Annotation['type'], string> = {
    roi_filtering: t('pipelines.modeRoiFiltering'),
    overcrowding: t('pipelines.modeOvercrowding'),
    line_crossing: t('pipelines.modeLineCrossing'),
    direction_detection: t('pipelines.modeDirectionDetection'),
  }
  return map[type]
}

function isToolDisabled(next: DrawTool) {
  if (next === 'edit') {
    return annotations.value.length === 0
  }
  if (!hasSource.value) {
    return true
  }
  return !isToolAllowed(annotationMode.value, next)
}

function onModeChange(value: string) {
  annotationMode.value = value as AnnotationModeType
  resetDraft()
  if (!isToolAllowed(annotationMode.value, tool.value) || tool.value === 'edit') {
    tool.value = defaultToolForMode(annotationMode.value)
  }
  markDirty()
}

function onToolChange(next: DrawTool) {
  if (isToolDisabled(next)) {
    return
  }
  resetDraft()
  tool.value = next
  if (next !== 'edit') {
    selectedId.value = null
  }
}

function onStageResize(width: number, height: number) {
  stageWidth.value = width
  stageHeight.value = height
}

function onSelectAnnotation(id: string | null) {
  selectAnnotation(id)
}

function onMoveSelected(dx: number, dy: number) {
  applyDeltaToSelected(dx, dy)
  markDirty()
}

function onMoveAnchor(pointIndex: number, stageX: number, stageY: number) {
  moveAnchor(pointIndex, stageX, stageY)
  markDirty()
}

function onRowClick(row: Annotation) {
  selectedId.value = row.id
}

async function onSourceChange(value: string) {
  if (value === CHOOSE_FILE) {
    fileInputRef.value?.click()
    return
  }
  if (value === NO_STREAMS || value.startsWith('file:')) {
    return
  }
  const stream = onlineStreams.value.find((item) => item.id === value)
  if (!stream) {
    return
  }
  resetDraft()
  sourceLoading.value = true
  sourceInvalid.value = false
  try {
    const result = await previewAnalyzerSourceStream(stream.id, stream.name)
    await commitSourceChange({
      width: result.config_width,
      height: result.config_height,
      apply: () => {
        sourceKind.value = 'stream'
        sourceStreamId.value = stream.id
        sourceFileName.value = null
        retainedFile.value = null
        sourceImageUrl.value = result.source_image_url
        displayImageUrl.value = result.source_image_url
        configWidth.value = result.config_width
        configHeight.value = result.config_height
        markDirty()
      },
    })
  } catch (err) {
    toast.error(
      err instanceof ApiError
        ? err.message
        : t('pipelines.snapshotFailed', { name: stream.name }),
    )
  }
  sourceLoading.value = false
}

function commitSourceChange(prepared: {
  width: number
  height: number
  apply: () => void
}) {
  if (
    annotations.value.length > 0 &&
    (prepared.width !== configWidth.value || prepared.height !== configHeight.value)
  ) {
    pendingSourceApply.value = () => {
      annotations.value = []
      selectedId.value = null
      prepared.apply()
    }
    confirmMessage.value = t('pipelines.resolutionChangedConfirm')
    confirmAction.value = 'resolution'
    confirmOpen.value = true
    return
  }
  prepared.apply()
}

async function onFilePicked(event: Event) {
  const input = event.target as HTMLInputElement
  const file = input.files?.[0]
  input.value = ''
  if (!file) {
    return
  }
  resetDraft()
  sourceLoading.value = true
  sourceInvalid.value = false
  try {
    const result = await previewAnalyzerSourceFile(file)
    commitSourceChange({
      width: result.config_width,
      height: result.config_height,
      apply: () => {
        sourceKind.value = 'file'
        sourceFileName.value = result.source_file_name ?? file.name
        sourceStreamId.value = null
        retainedFile.value = file
        sourceImageUrl.value = result.source_image_url
        displayImageUrl.value = result.source_image_url
        configWidth.value = result.config_width
        configHeight.value = result.config_height
        markDirty()
      },
    })
  } catch (err) {
    toast.error(err instanceof ApiError ? err.message : String(err))
  }
  sourceLoading.value = false
}

async function onRefreshSource() {
  if (!hasSource.value || sourceLoading.value) {
    return
  }
  resetDraft()
  sourceLoading.value = true
  try {
    if (sourceKind.value === 'file' && retainedFile.value) {
      const result = await previewAnalyzerSourceFile(retainedFile.value)
      await applyRefreshResult(result)
    } else if (sourceKind.value === 'file' && sourceImageUrl.value) {
      displayImageUrl.value = sourceImageUrl.value
    } else if (sourceKind.value === 'stream' && sourceStreamId.value) {
      const stream = onlineStreams.value.find((item) => item.id === sourceStreamId.value)
      const result = await previewAnalyzerSourceStream(
        sourceStreamId.value,
        stream?.name ?? sourceStreamId.value,
      )
      await applyRefreshResult(result)
    }
  } catch (err) {
    const stream = onlineStreams.value.find((item) => item.id === sourceStreamId.value)
    const message =
      err instanceof ApiError
        ? err.message
        : t('pipelines.snapshotFailed', { name: stream?.name ?? '' })
    toast.error(message)
  }
  sourceLoading.value = false
}

async function applyRefreshResult(result: {
  source_image_url: string
  config_width: number
  config_height: number
}) {
  if (
    annotations.value.length > 0 &&
    (result.config_width !== configWidth.value ||
      result.config_height !== configHeight.value)
  ) {
    pendingSourceApply.value = () => {
      annotations.value = []
      selectedId.value = null
      sourceImageUrl.value = result.source_image_url
      displayImageUrl.value = result.source_image_url
      configWidth.value = result.config_width
      configHeight.value = result.config_height
      markDirty()
    }
    confirmMessage.value = t('pipelines.resolutionChangedConfirm')
    confirmAction.value = 'resolution'
    confirmOpen.value = true
    return
  }
  sourceImageUrl.value = result.source_image_url
  displayImageUrl.value = result.source_image_url
  configWidth.value = result.config_width
  configHeight.value = result.config_height
  markDirty()
}

function openEditProps(row: Annotation) {
  editingAnnotationId.value = row.id
  propertyDraft.value = {
    id: row.id,
    name: row.name,
    type: row.type,
    shape: row.shape,
    points: [...row.points],
    click_points: [...getClickPoints(row)],
    object_threshold: row.object_threshold,
    time_threshold: row.time_threshold,
    extend: row.extend,
    mode: row.mode,
  }
  const clicks = getClickPoints(row)
  const px = clicks.length >= 2 ? clicks[clicks.length - 2] : 0
  const py = clicks.length >= 2 ? clicks[clicks.length - 1] : 0
  const anchor = pixelToStage(px, py, transform.value)
  propertyPopoverX.value = anchor.x
  propertyPopoverY.value = anchor.y
  propertyOpen.value = true
}

function onPropertyCancel() {
  propertyDraft.value = null
  editingAnnotationId.value = null
  clearPending()
}

function askDeleteAnnotation(row: Annotation) {
  pendingDeleteId.value = row.id
  confirmMessage.value = t('pipelines.confirmDeleteAnnotation', { name: row.name })
  confirmAction.value = 'delete'
  confirmOpen.value = true
}

function onPropertyConfirm(annotation: Annotation) {
  const index = annotations.value.findIndex((item) => item.id === annotation.id)
  if (index >= 0) {
    annotations.value[index] = annotation
  } else {
    annotations.value = [...annotations.value, annotation]
  }
  selectedId.value = annotation.id
  propertyDraft.value = null
  editingAnnotationId.value = null
  clearPending()
  markDirty()
}

/** Cancel and Dialog [x] share one close path; `done` is Element Plus before-close callback. */
function requestClose(done?: () => void) {
  if (!dirty.value) {
    if (done) {
      done()
    } else {
      emit('update:modelValue', false)
    }
    return
  }
  pendingCloseDone.value = done ?? null
  confirmMessage.value = t('pipelines.discardChangesConfirm')
  confirmAction.value = 'discard'
  confirmOpen.value = true
}

function onBeforeClose(done: () => void) {
  requestClose(done)
}

function onConfirmVisible(value: boolean) {
  confirmOpen.value = value
  if (!value) {
    pendingCloseDone.value = null
  }
}

function finishDiscardClose() {
  dirty.value = false
  confirmOpen.value = false
  const done = pendingCloseDone.value
  pendingCloseDone.value = null
  if (done) {
    done()
    return
  }
  emit('update:modelValue', false)
}

function onConfirmAction() {
  if (confirmAction.value === 'discard') {
    finishDiscardClose()
    return
  }
  if (confirmAction.value === 'delete' && pendingDeleteId.value) {
    annotations.value = annotations.value.filter((item) => item.id !== pendingDeleteId.value)
    if (selectedId.value === pendingDeleteId.value) {
      selectedId.value = null
    }
    pendingDeleteId.value = null
    markDirty()
  }
  if (confirmAction.value === 'resolution' && pendingSourceApply.value) {
    pendingSourceApply.value()
    pendingSourceApply.value = null
  }
  confirmOpen.value = false
}

async function onSave() {
  const kind = sourceKind.value
  nameInvalid.value = !name.value.trim()
  sourceInvalid.value = !hasSource.value || !kind
  if (nameInvalid.value || sourceInvalid.value || !kind) {
    return
  }
  saving.value = true
  const body = {
    name: name.value.trim(),
    source_kind: kind,
    source_stream_id: kind === 'stream' ? sourceStreamId.value : null,
    source_file_name: kind === 'file' ? sourceFileName.value : null,
    source_image_url: sourceImageUrl.value,
    config_width: configWidth.value,
    config_height: configHeight.value,
    captured_at: new Date().toISOString(),
    annotations: annotations.value.map((item) => normalizeAnnotation(item)),
  }
  try {
    if (props.mode === 'edit' && props.analyzerId) {
      await updateAnalyzerTemplate(props.analyzerId, body)
    } else {
      await createAnalyzerTemplate(body)
    }
    dirty.value = false
    emit('saved')
  } catch (err) {
    const message = err instanceof ApiError ? err.message : String(err)
    if (err instanceof ApiError && err.status === 409) {
      nameInvalid.value = true
    }
    toast.error(message)
  }
  saving.value = false
}
</script>

<style scoped>
.editor {
  display: flex;
  flex-direction: column;
  gap: 12px;
  height: min(640px, calc(100vh - 200px));
  max-height: calc(100vh - 200px);
  overflow: hidden;
}

.editor__top {
  display: flex;
  gap: 16px;
  flex-wrap: wrap;
  flex-shrink: 0;
}

.editor__field {
  display: grid;
  grid-template-columns: 64px 240px;
  gap: 8px;
  align-items: center;
}

.editor__field--source {
  grid-template-columns: 64px 260px 32px;
}

.editor__field--mode {
  grid-template-columns: 56px 180px;
}

.editor__body {
  display: grid;
  grid-template-columns: 1fr 400px;
  gap: 12px;
  flex: 1;
  min-height: 0;
  overflow: hidden;
}

.editor__main {
  display: flex;
  flex-direction: column;
  gap: 8px;
  min-width: 0;
  min-height: 0;
  overflow: hidden;
}

.editor__toolbar {
  display: flex;
  align-items: center;
  gap: 12px;
  flex-wrap: wrap;
  flex-shrink: 0;
}

.tools {
  display: flex;
  gap: 4px;
}

.tools__btn,
.icon-btn {
  width: 32px;
  height: 32px;
  border: 1px solid #dcdfe6;
  border-radius: 4px;
  background: #fff;
  color: #606266;
  cursor: pointer;
  display: inline-flex;
  align-items: center;
  justify-content: center;
}

.tools__btn.is-active {
  border-color: #409eff;
  color: #409eff;
  background: #ecf5ff;
}

.tools__btn:disabled,
.icon-btn:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}

.editor__canvas {
  position: relative;
  flex: 1;
  min-height: 0;
  overflow: hidden;
}

.editor__list {
  min-width: 0;
  min-height: 0;
  overflow: auto;
}

.ops {
  display: flex;
  gap: 2px;
}

.ops__btn {
  width: 28px;
  height: 28px;
  border: none;
  background: transparent;
  cursor: pointer;
  color: #606266;
}

.ops__btn.is-danger {
  color: #f56c6c;
}

.file-input {
  display: none;
}
</style>
