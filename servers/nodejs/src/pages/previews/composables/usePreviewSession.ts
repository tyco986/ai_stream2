import { computed, ref, watch } from 'vue'
import {
  batchDeleteLayouts,
  DEFAULT_LAYOUT_ID,
  deleteLayout,
  getActiveLayout,
  getLayout,
  getShot,
  LAYOUT_SLOT_COUNT,
  listLayouts,
  patchLayout,
  postLayout,
  putActiveLayout,
  putShot,
  resizeSlots,
  type LayoutPreset,
  type LayoutPresetSummary,
  type PreviewLayout,
  type ViewMode,
} from '@/api/preview'
import {
  collectStreams,
  getGroupsTree,
  getStream,
  type GroupsTree,
  type Stream,
  type TreeStreamNode,
} from '@/api/streams'
import { capturePreviewShot } from '../utils/capturePreviewShot'
import { reconcileSlotMedia } from '../utils/slotMediaHub'

const layout = ref<PreviewLayout>('2x2')
const slots = ref<(string | null)[]>(Array.from({ length: 4 }, () => null))
const viewMode = ref<ViewMode>('grid')
const presetId = ref('')
const presetName = ref('Default')
const dirty = ref(false)
const selectedSlotIndex = ref<number | null>(null)
const focusIndex = ref(0)
const tree = ref<GroupsTree | null>(null)
const presets = ref<LayoutPresetSummary[]>([])
const loaded = ref(false)
const treeLoading = ref(false)
const saving = ref(false)
const searchQuery = ref('')
const filterBoundOnly = ref(false)
const selectedGroupId = ref<string | null>(null)
const streamDetailMap = ref(new Map<string, Stream>())

watch(
  slots,
  (next) => {
    reconcileSlotMedia(next)
  },
  { deep: true },
)

export function usePreviewSession() {
  const streamMap = computed(() => {
    const map = new Map<string, TreeStreamNode>()
    if (!tree.value) {
      return map
    }
    for (const stream of collectStreams(tree.value.root)) {
      map.set(stream.id, stream)
    }
    return map
  })

  const boundStreamIds = computed(
    () => new Set(slots.value.filter((id): id is string => id !== null)),
  )

  function markDirty() {
    dirty.value = true
  }

  function applyPreset(preset: LayoutPreset) {
    layout.value = preset.layout
    slots.value = [...preset.slots]
    viewMode.value = preset.view_mode
    presetId.value = preset.id
    presetName.value = preset.name
    dirty.value = false
    selectedSlotIndex.value = null
    const firstBound = preset.slots.findIndex((id) => id !== null)
    focusIndex.value = firstBound >= 0 ? firstBound : 0
  }

  async function refreshPresets(search?: string) {
    const result = await listLayouts(search !== undefined ? { search } : {})
    presets.value = result.items
  }

  async function syncStreamDetails(ids: (string | null)[]) {
    const next = new Map(streamDetailMap.value)
    for (const id of ids) {
      if (!id || next.has(id)) {
        continue
      }
      next.set(id, await getStream(id))
    }
    streamDetailMap.value = next
  }

  function buildShotDataUrl() {
    return capturePreviewShot()
  }

  async function bootstrap() {
    // Keep in-memory layout/slots/view_mode across SPA navigations until full reload.
    if (loaded.value) {
      const [treeData, layoutList] = await Promise.all([
        getGroupsTree(),
        listLayouts(),
      ])
      tree.value = treeData
      presets.value = layoutList.items
      await syncStreamDetails(slots.value)
      return
    }
    const [active, treeData, layoutList] = await Promise.all([
      getActiveLayout(),
      getGroupsTree(),
      listLayouts(),
    ])
    tree.value = treeData
    presets.value = layoutList.items
    const preset = await getLayout(active.preset_id)
    applyPreset(preset)
    await syncStreamDetails(preset.slots)
    loaded.value = true
  }

  async function refreshTree() {
    treeLoading.value = true
    const treeData = await getGroupsTree()
    tree.value = treeData
    const known = new Set(collectStreams(treeData.root).map((s) => s.id))
    let removed = 0
    slots.value = slots.value.map((id) => {
      if (id && !known.has(id)) {
        removed += 1
        return null
      }
      return id
    })
    if (removed > 0) {
      markDirty()
    }
    treeLoading.value = false
    return removed
  }

  function setLayout(next: PreviewLayout) {
    if (next === layout.value) {
      return
    }
    slots.value = resizeSlots(slots.value, next)
    layout.value = next
    if (
      selectedSlotIndex.value !== null &&
      selectedSlotIndex.value >= LAYOUT_SLOT_COUNT[next]
    ) {
      selectedSlotIndex.value = null
    }
    if (focusIndex.value >= LAYOUT_SLOT_COUNT[next]) {
      focusIndex.value = 0
    }
    markDirty()
  }

  function setViewMode(next: ViewMode) {
    if (next === viewMode.value) {
      return
    }
    viewMode.value = next
    if (next === 'focus') {
      const firstBound = slots.value.findIndex((id) => id !== null)
      if (firstBound < 0) {
        focusIndex.value = 0
      } else if (
        selectedSlotIndex.value !== null &&
        slots.value[selectedSlotIndex.value]
      ) {
        focusIndex.value = selectedSlotIndex.value
      } else {
        focusIndex.value = firstBound
      }
    }
    markDirty()
  }

  function clearSlots() {
    slots.value = Array.from({ length: LAYOUT_SLOT_COUNT[layout.value] }, () => null)
    selectedSlotIndex.value = null
    focusIndex.value = 0
    markDirty()
  }

  function selectSlot(index: number) {
    const same = selectedSlotIndex.value === index
    selectedSlotIndex.value = same ? null : index
    if (slots.value.every((id) => id === null)) {
      focusIndex.value = 0
    } else if (!same) {
      focusIndex.value = index
    }
  }

  function clearSlot(index: number) {
    const next = [...slots.value]
    next[index] = null
    slots.value = next
    markDirty()
    if (viewMode.value === 'focus' && index === focusIndex.value) {
      const nextBound = next.findIndex((id) => id !== null)
      focusIndex.value = nextBound >= 0 ? nextBound : 0
    }
  }

  function bindStream(streamId: string) {
    const next = [...slots.value]
    const existing = next.indexOf(streamId)
    const hadSelection = selectedSlotIndex.value !== null
    let target = selectedSlotIndex.value

    if (existing >= 0) {
      if (target === null || target === existing) {
        clearSlot(existing)
        return
      }
      next[existing] = null
      next[target] = streamId
    } else if (target === null) {
      target = next.findIndex((slot) => slot === null)
      if (target < 0) {
        target = next.length - 1
      }
      next[target] = streamId
    } else {
      next[target] = streamId
    }

    slots.value = next
    selectedSlotIndex.value = hadSelection ? target : null
    focusIndex.value = target
    markDirty()
    syncStreamDetails([streamId])
  }

  async function save() {
    saving.value = true
    const saved = await patchLayout(presetId.value, {
      layout: layout.value,
      slots: [...slots.value],
      view_mode: viewMode.value,
    })
    const shot = buildShotDataUrl()
    if (shot) {
      await putShot(saved.id, shot)
    }
    applyPreset(saved)
    await refreshPresets()
    saving.value = false
  }

  async function saveAs(name: string) {
    saving.value = true
    const created = await postLayout({
      name,
      layout: layout.value,
      view_mode: viewMode.value,
      slots: [...slots.value],
    })
    const shot = buildShotDataUrl()
    if (shot) {
      await putShot(created.id, shot)
    }
    await putActiveLayout({ preset_id: created.id })
    applyPreset(created)
    await refreshPresets()
    saving.value = false
    return created
  }

  async function renamePreset(targetId: string, name: string) {
    const updated = await patchLayout(targetId, { name })
    if (targetId === presetId.value) {
      presetName.value = updated.name
    }
    await refreshPresets()
    return updated
  }

  async function removePreset(targetId: string) {
    const result = await deleteLayout(targetId)
    if (result.was_active) {
      const preset = await getLayout(DEFAULT_LAYOUT_ID)
      await putActiveLayout({ preset_id: DEFAULT_LAYOUT_ID })
      applyPreset(preset)
      await syncStreamDetails(preset.slots)
    }
    await refreshPresets()
    return result
  }

  async function removePresets(ids: string[]) {
    const result = await batchDeleteLayouts({ ids })
    if (result.active_deleted) {
      const preset = await getLayout(DEFAULT_LAYOUT_ID)
      await putActiveLayout({ preset_id: DEFAULT_LAYOUT_ID })
      applyPreset(preset)
      await syncStreamDetails(preset.slots)
    }
    await refreshPresets()
    return result
  }

  async function loadPreset(nextPresetId: string) {
    const preset = await getLayout(nextPresetId)
    await putActiveLayout({ preset_id: nextPresetId })
    applyPreset(preset)
    await syncStreamDetails(preset.slots)
  }

  async function loadShot(targetId: string) {
    return getShot(targetId)
  }

  return {
    layout,
    slots,
    viewMode,
    presetId,
    presetName,
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
    refreshPresets,
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
  }
}
