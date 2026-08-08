<template>
  <div class="focus-view">
    <div class="focus-view__main" :style="mainStyle">
      <SlotCell
        :index="focusIndex"
        :stream="focusStream"
        :detail="focusDetail"
        :selected="selectedSlotIndex === focusIndex"
        :paused="pausedBySlot[focusIndex] ?? false"
        :use-mirror="showList"
        :mirror-src-object="focusMirrorStream"
        fit="contain"
        @select="emit('select-slot', focusIndex)"
        @clear="emit('clear-slot', focusIndex)"
        @playback-change="emit('playback-change', focusIndex, $event)"
      />
    </div>
    <div
      v-if="showList"
      class="focus-view__list"
      :class="{ 'is-scrollbar-visible': scrollbarVisible }"
      :style="listStyle"
      @mouseenter="onListEnter"
      @mouseleave="onListLeave"
      @wheel="onListScrollActivity"
      @scroll="onListScrollActivity"
    >
      <div
        v-for="(slotId, index) in slots"
        :key="index"
        class="focus-view__item"
        :style="itemStyle"
      >
        <SlotCell
          :index="index"
          :stream="slotId ? streamMap.get(slotId) ?? null : null"
          :detail="slotId ? streamDetailMap.get(slotId) ?? null : null"
          :selected="selectedSlotIndex === index"
          :paused="pausedBySlot[index] ?? false"
          compact
          @select="emit('select-slot', index)"
          @clear="emit('clear-slot', index)"
          @playback-change="emit('playback-change', index, $event)"
        />
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, onBeforeUnmount, ref } from 'vue'
import type { Stream, TreeStreamNode } from '@/api/streams'
import { slotMediaMap } from '../utils/slotMediaHub'
import SlotCell from './SlotCell.vue'

const SCROLLBAR_HIDE_MS = 800

const props = defineProps<{
  slots: (string | null)[]
  streamMap: Map<string, TreeStreamNode>
  streamDetailMap: Map<string, Stream>
  focusIndex: number
  selectedSlotIndex: number | null
  mainWidth: number
  mainHeight: number
  listWidth: number
  itemWidth: number
  itemHeight: number
  pausedBySlot: Record<number, boolean>
}>()

const emit = defineEmits<{
  'select-slot': [index: number]
  'clear-slot': [index: number]
  'playback-change': [index: number, paused: boolean]
}>()

const showList = computed(() => props.slots.length > 1 && props.listWidth > 0)

const scrollbarVisible = ref(false)
const listHovered = ref(false)
let hideTimer: ReturnType<typeof setTimeout> | null = null

const mainStyle = computed(() => ({
  width: `${props.mainWidth}px`,
  height: `${props.mainHeight}px`,
}))

const listStyle = computed(() => ({
  width: `${props.listWidth}px`,
}))

const itemStyle = computed(() => ({
  width: `${props.itemWidth}px`,
  height: `${props.itemHeight}px`,
}))

const focusStream = computed(() => {
  const id = props.slots[props.focusIndex]
  return id ? (props.streamMap.get(id) ?? null) : null
})

const focusDetail = computed(() => {
  const id = props.slots[props.focusIndex]
  return id ? (props.streamDetailMap.get(id) ?? null) : null
})

const focusMirrorStream = computed(
  () => slotMediaMap.value[props.focusIndex] ?? null,
)

function clearHideTimer() {
  if (hideTimer) {
    clearTimeout(hideTimer)
    hideTimer = null
  }
}

function onListEnter() {
  listHovered.value = true
  clearHideTimer()
  scrollbarVisible.value = true
}

function onListLeave() {
  listHovered.value = false
  clearHideTimer()
  scrollbarVisible.value = false
}

function onListScrollActivity() {
  scrollbarVisible.value = true
  clearHideTimer()
  hideTimer = setTimeout(() => {
    hideTimer = null
    scrollbarVisible.value = listHovered.value
  }, SCROLLBAR_HIDE_MS)
}

onBeforeUnmount(() => {
  clearHideTimer()
})
</script>

<style scoped>
.focus-view {
  display: flex;
  width: 100%;
  height: 100%;
  min-height: 0;
  overflow: hidden;
  gap: 1px;
  background: #303133;
}

.focus-view__main {
  flex-shrink: 0;
  height: 100%;
  background: #000;
}

.focus-view__list {
  flex-shrink: 0;
  height: 100%;
  display: flex;
  flex-direction: column;
  gap: 1px;
  background: #303133;
  overflow-x: hidden;
  overflow-y: auto;
  scrollbar-width: none;
}

.focus-view__list.is-scrollbar-visible {
  scrollbar-width: thin;
  scrollbar-color: rgba(255, 255, 255, 0.35) transparent;
}

.focus-view__list::-webkit-scrollbar {
  width: 0;
}

.focus-view__list.is-scrollbar-visible::-webkit-scrollbar {
  width: 6px;
}

.focus-view__list::-webkit-scrollbar-thumb {
  background: transparent;
  border-radius: 3px;
}

.focus-view__list.is-scrollbar-visible::-webkit-scrollbar-thumb {
  background: rgba(255, 255, 255, 0.35);
}

.focus-view__list::-webkit-scrollbar-track {
  background: transparent;
}

.focus-view__item {
  flex-shrink: 0;
  min-height: 0;
}
</style>
