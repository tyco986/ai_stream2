<template>
  <div v-if="loaded" class="preview-page">
    <header class="preview-page__title">{{ t('preview.title') }}</header>
    <div class="preview-page__body">
      <GroupsPanel
        :root="tree?.root ?? null"
        :search-query="searchQuery"
        :filter-bound-only="filterBoundOnly"
        :bound-ids="boundStreamIds"
        :selected-group-id="selectedGroupId"
        :expanded-ids="expandedIds"
        :tree-loading="treeLoading"
        :can-clear="canClear"
        @update:search-query="searchQuery = $event"
        @update:filter-bound-only="filterBoundOnly = $event"
        @clear="clearSlots"
        @refresh="onRefresh"
        @toggle="toggleExpand"
        @select-group="selectedGroupId = $event"
        @select-stream="bindStream"
      />
      <section class="preview-page__main">
        <PresetBar
          :layout="layout"
          :view-mode="viewMode"
          :preset-id="presetId"
          :dirty="dirty"
          :saving="saving"
          :presets="presets"
          :can-clear="canClear"
          :all-paused="allPaused"
          @update:layout="setLayout"
          @update:view-mode="setViewMode"
          @change-preset="onChangePreset"
          @save="onSave"
          @save-as="openSaveAs"
          @clear="clearSlots"
          @manage="openManage"
          @toggle-playback="toggleAllPlayback"
          @fullscreen="enterFullscreen"
        />
        <div ref="stageSlotRef" class="preview-page__stage-slot">
          <div class="preview-page__stage" :style="stageStyle">
            <GridView
              v-if="viewMode === 'grid'"
              :layout="layout"
              :slots="slots"
              :stream-map="streamMap"
              :stream-detail-map="streamDetailMap"
              :selected-slot-index="selectedSlotIndex"
              :paused-by-slot="pausedBySlot"
              @select-slot="selectSlot"
              @clear-slot="clearSlot"
              @playback-change="onPlaybackChange"
            />
            <FocusView
              v-else
              :slots="slots"
              :stream-map="streamMap"
              :stream-detail-map="streamDetailMap"
              :focus-index="focusIndex"
              :selected-slot-index="selectedSlotIndex"
              :main-width="focusSplit.mainWidth"
              :main-height="focusSplit.mainHeight"
              :list-width="focusSplit.listWidth"
              :item-width="focusSplit.itemWidth"
              :item-height="focusSplit.itemHeight"
              :paused-by-slot="pausedBySlot"
              @select-slot="selectSlot"
              @clear-slot="clearSlot"
              @playback-change="onPlaybackChange"
            />
          </div>
        </div>
      </section>
    </div>

    <LayoutFormDialog
      v-model="formOpen"
      :mode="formMode"
      :name="formName"
      :layout="formLayout"
      :view-mode="formViewMode"
      :slots="formSlots"
      :stream-names="streamNames"
      :shot-url="formShotUrl"
      :saving="formSaving"
      :error="formError"
      @update:name="formName = $event"
      @submit="onFormSubmit"
      @preview-shot="shotOpen = true"
    />

    <ManageLayoutsDialog
      v-model="manageOpen"
      :items="manageItems"
      :search="manageSearch"
      @update:search="onManageSearch"
      @rename="openRename"
      @delete="openDeleteOne"
      @batch-delete="openDeleteBatch"
    />

    <ShotPreviewDialog v-model="shotOpen" :url="formShotUrl" />

    <ConfirmDialog
      v-model="confirmOpen"
      :title="confirmTitle"
      :message="confirmMessage"
      :hint="confirmHint"
      :confirm-label="confirmLabel"
      :confirm-type="confirmType"
      :loading="confirmLoading"
      @confirm="onConfirmAction"
    />
  </div>
</template>

<script setup lang="ts">
import { computed, nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import {
  getLayout,
  listLayouts,
  type LayoutPresetSummary,
  type PreviewLayout,
  type ViewMode,
} from '@/api/preview'
import { useShellSettings } from '@/pages/shell/composables/useShellSettings'
import { toast } from '@/shared/ui/toast'
import ConfirmDialog from './components/ConfirmDialog.vue'
import FocusView from './components/FocusView.vue'
import GridView from './components/GridView.vue'
import GroupsPanel from './components/GroupsPanel.vue'
import LayoutFormDialog from './components/LayoutFormDialog.vue'
import ManageLayoutsDialog from './components/ManageLayoutsDialog.vue'
import PresetBar from './components/PresetBar.vue'
import ShotPreviewDialog from './components/ShotPreviewDialog.vue'
import { usePreviewSession } from './composables/usePreviewSession'
import {
  fitPreviewChromeLayout,
  fitStageInHost,
  splitFocusStage,
} from './utils/gridLayout'

const { t } = useI18n()
const session = usePreviewSession()
const {
  layout,
  slots,
  viewMode,
  presetId,
  dirty,
  selectedSlotIndex,
  focusIndex,
  tree,
  presets,
  loaded,
  treeLoading,
  saving,
  searchQuery,
  filterBoundOnly,
  selectedGroupId,
  streamMap,
  streamDetailMap,
  boundStreamIds,
  bootstrap,
  refreshTree,
  setLayout,
  setViewMode,
  clearSlots,
  selectSlot,
  clearSlot,
  bindStream,
  save,
  saveAs,
  renamePreset,
  removePreset,
  removePresets,
  loadPreset,
  loadShot,
  buildShotDataUrl,
} = session

const { mode: shellMode } = useShellSettings()

const expandedIds = ref(new Set<string>())
const canClear = computed(
  () => boundStreamIds.value.size > 0 || selectedSlotIndex.value !== null,
)

const stageSlotRef = ref<HTMLElement | null>(null)
const shellWidth = ref(0)
const stageSlotWidth = ref(0)
const stageSlotHeight = ref(0)
const layoutStageHeight = ref(0)
const isFullscreen = ref(false)
const pausedBySlot = ref<Record<number, boolean>>({})
let shellObserver: ResizeObserver | null = null
let stageSlotObserver: ResizeObserver | null = null

const allPaused = computed(() => {
  const boundIndexes = slots.value.flatMap((id, index) => (id ? [index] : []))
  return (
    boundIndexes.length > 0 &&
    boundIndexes.every((index) => pausedBySlot.value[index] ?? false)
  )
})

function onPlaybackChange(index: number, paused: boolean) {
  pausedBySlot.value = { ...pausedBySlot.value, [index]: paused }
}

function toggleAllPlayback() {
  const next = !allPaused.value
  pausedBySlot.value = Object.fromEntries(
    slots.value.map((_, index) => [index, next]),
  )
}

const pageChrome = computed(() => {
  const cols = Number(layout.value.split('x')[0]) || 1
  return fitPreviewChromeLayout({
    shellWidth: shellWidth.value,
    stageHeight: layoutStageHeight.value,
    cols,
    sidebarMode: shellMode.value === 'sidebar',
  })
})

const stageChrome = computed(() => {
  const cols = Number(layout.value.split('x')[0]) || 1
  if (isFullscreen.value) {
    return fitStageInHost({
      hostWidth: stageSlotWidth.value,
      hostHeight: stageSlotHeight.value,
      cols,
    })
  }
  return {
    stageWidth: pageChrome.value.stageWidth,
    stageHeight: pageChrome.value.stageHeight,
  }
})

const focusSplit = computed(() =>
  splitFocusStage({
    stageWidth: stageChrome.value.stageWidth,
    stageHeight: stageChrome.value.stageHeight,
    showList: viewMode.value === 'focus' && slots.value.length > 1,
  }),
)

const stageStyle = computed(() => ({
  width: `${stageChrome.value.stageWidth}px`,
  height: `${stageChrome.value.stageHeight}px`,
}))

const streamNames = computed(() => {
  const names: Record<string, string> = {}
  for (const [id, node] of streamMap.value) {
    names[id] = node.name
  }
  return names
})

const formOpen = ref(false)
const formMode = ref<'create' | 'rename'>('create')
const formName = ref('')
const formLayout = ref<PreviewLayout>('2x2')
const formViewMode = ref<ViewMode>('grid')
const formSlots = ref<(string | null)[]>([])
const formShotUrl = ref('')
const formSaving = ref(false)
const formError = ref('')
const formTargetId = ref('')

const manageOpen = ref(false)
const manageSearch = ref('')
const manageItems = ref<LayoutPresetSummary[]>([])

const shotOpen = ref(false)

const confirmOpen = ref(false)
const confirmMessage = ref('')
const confirmLoading = ref(false)
const confirmIds = ref<string[]>([])
const confirmAction = ref<'delete' | 'discard' | null>(null)
const pendingPresetId = ref('')

const confirmTitle = computed(() =>
  confirmAction.value === 'discard' ? t('preview.confirmTitle') : t('preview.confirmDelete'),
)
const confirmHint = computed(() =>
  confirmAction.value === 'delete' ? t('preview.cannotUndo') : '',
)
const confirmLabel = computed(() =>
  confirmAction.value === 'discard' ? t('preview.confirm') : t('preview.delete'),
)
const confirmType = computed(() =>
  confirmAction.value === 'discard' ? 'primary' : 'danger',
)

onMounted(async () => {
  await bootstrap()
  if (tree.value) {
    expandedIds.value = new Set(collectGroupIds(tree.value.root))
  }
  await nextTick()
  shellObserver = new ResizeObserver((entries) => {
    const entry = entries[0]
    if (!entry) {
      return
    }
    shellWidth.value = entry.contentRect.width
  })
  stageSlotObserver = new ResizeObserver((entries) => {
    const entry = entries[0]
    if (!entry) {
      return
    }
    stageSlotWidth.value = entry.contentRect.width
    stageSlotHeight.value = entry.contentRect.height
    if (!isFullscreen.value) {
      layoutStageHeight.value = entry.contentRect.height
    }
  })
  const shellEl = document.querySelector('.shell')
  if (shellEl instanceof HTMLElement) {
    shellWidth.value = shellEl.clientWidth
    shellObserver.observe(shellEl)
  }
  if (stageSlotRef.value) {
    stageSlotWidth.value = stageSlotRef.value.clientWidth
    stageSlotHeight.value = stageSlotRef.value.clientHeight
    layoutStageHeight.value = stageSlotRef.value.clientHeight
    stageSlotObserver.observe(stageSlotRef.value)
  }
  document.addEventListener('fullscreenchange', onFullscreenChange)
})

onBeforeUnmount(() => {
  document.removeEventListener('fullscreenchange', onFullscreenChange)
  if (isFullscreen.value) {
    void document.exitFullscreen()
  }
  if (shellObserver) {
    shellObserver.disconnect()
    shellObserver = null
  }
  if (stageSlotObserver) {
    stageSlotObserver.disconnect()
    stageSlotObserver = null
  }
})

watch(manageOpen, async (open) => {
  if (open) {
    manageSearch.value = ''
    await refreshManageList()
  }
})

function onFullscreenChange() {
  isFullscreen.value = document.fullscreenElement === stageSlotRef.value
  if (!isFullscreen.value && stageSlotRef.value) {
    layoutStageHeight.value = stageSlotRef.value.clientHeight
  }
}

async function enterFullscreen() {
  if (!stageSlotRef.value || document.fullscreenElement) {
    return
  }
  await stageSlotRef.value.requestFullscreen()
}

function collectGroupIds(node: {
  type: string
  id: string
  children?: Array<{ type: string; id: string; children?: unknown[] }>
}): string[] {
  if (node.type !== 'group') {
    return []
  }
  const ids = [node.id]
  for (const child of node.children ?? []) {
    ids.push(
      ...collectGroupIds(
        child as {
          type: string
          id: string
          children?: Array<{ type: string; id: string; children?: unknown[] }>
        },
      ),
    )
  }
  return ids
}

function toggleExpand(id: string) {
  const next = new Set(expandedIds.value)
  if (next.has(id)) {
    next.delete(id)
  } else {
    next.add(id)
  }
  expandedIds.value = next
}

async function onRefresh() {
  const removed = await refreshTree()
  if (removed > 0) {
    toast.warning(t('preview.streamsMissing', { n: removed }))
  }
}

async function onSave() {
  await save()
  toast.success(t('preview.layoutSaved'))
}

async function onChangePreset(nextId: string) {
  if (nextId === presetId.value) {
    return
  }
  if (!dirty.value) {
    await loadPreset(nextId)
    return
  }
  pendingPresetId.value = nextId
  confirmAction.value = 'discard'
  confirmMessage.value = t('preview.discardConfirm')
  confirmOpen.value = true
}

function openSaveAs() {
  formMode.value = 'create'
  formName.value = ''
  formLayout.value = layout.value
  formViewMode.value = viewMode.value
  formSlots.value = [...slots.value]
  formShotUrl.value = buildShotDataUrl()
  formError.value = ''
  formTargetId.value = ''
  formOpen.value = true
}

async function openManage() {
  manageOpen.value = true
}

async function refreshManageList() {
  const result = await listLayouts({ search: manageSearch.value })
  manageItems.value = result.items
}

async function onManageSearch(value: string) {
  manageSearch.value = value
  await refreshManageList()
}

async function openRename(row: LayoutPresetSummary) {
  const preset = await getLayout(row.id)
  formMode.value = 'rename'
  formName.value = preset.name
  formLayout.value = preset.layout
  formViewMode.value = preset.view_mode
  formSlots.value = [...preset.slots]
  formTargetId.value = preset.id
  formError.value = ''
  formShotUrl.value = ''
  if (preset.shot_url) {
    formShotUrl.value = await loadShot(preset.id)
  }
  formOpen.value = true
}

async function onFormSubmit() {
  formSaving.value = true
  formError.value = ''
  const name = formName.value.trim()
  if (!name) {
    formError.value = 'Name required'
    formSaving.value = false
    return
  }
  if (formMode.value === 'create') {
    await saveAs(name)
    toast.success(t('preview.layoutCreated'))
  } else {
    await renamePreset(formTargetId.value, name)
    toast.success(t('preview.layoutRenamed'))
    await refreshManageList()
  }
  formSaving.value = false
  formOpen.value = false
}

function openDeleteOne(row: LayoutPresetSummary) {
  confirmAction.value = 'delete'
  confirmIds.value = [row.id]
  confirmMessage.value = t('preview.deletePresets', { n: 1 })
  confirmOpen.value = true
}

function openDeleteBatch(ids: string[]) {
  confirmAction.value = 'delete'
  confirmIds.value = [...ids]
  confirmMessage.value = t('preview.deletePresets', { n: ids.length })
  confirmOpen.value = true
}

async function onConfirmAction() {
  if (confirmAction.value === 'discard') {
    const nextId = pendingPresetId.value
    pendingPresetId.value = ''
    confirmOpen.value = false
    confirmAction.value = null
    await loadPreset(nextId)
  } else {
    confirmLoading.value = true
    if (confirmIds.value.length === 1) {
      await removePreset(confirmIds.value[0])
    } else {
      await removePresets(confirmIds.value)
    }
    confirmLoading.value = false
    confirmOpen.value = false
    confirmAction.value = null
    await refreshManageList()
  }
}
</script>

<style scoped>
.preview-page {
  display: flex;
  flex-direction: column;
  height: 100%;
  min-height: 0;
  background: #f5f7fa;
  overflow: hidden;
}

.preview-page__title {
  padding: 12px 16px;
  font-size: 18px;
  font-weight: 600;
  background: #fff;
  border-bottom: 1px solid #e4e7ed;
}

.preview-page__body {
  display: flex;
  flex: 1;
  min-height: 0;
}

.preview-page__main {
  display: flex;
  flex-direction: column;
  flex: 1;
  min-width: 0;
  height: 100%;
  min-height: 0;
  background: #fff;
}

.preview-page__stage-slot {
  flex: 1;
  min-height: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  background: #fff;
  position: relative;
}

.preview-page__stage-slot:fullscreen {
  width: 100%;
  height: 100%;
  background: #000;
}

.preview-page__stage {
  flex-shrink: 0;
  background: #000;
  overflow: hidden;
}
</style>
