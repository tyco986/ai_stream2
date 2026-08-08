<template>
  <div class="grid-view" :style="gridStyle">
    <SlotCell
      v-for="(slotId, index) in slots"
      :key="index"
      :index="index"
      :stream="slotId ? streamMap.get(slotId) ?? null : null"
      :detail="slotId ? streamDetailMap.get(slotId) ?? null : null"
      :selected="selectedSlotIndex === index"
      :paused="pausedBySlot[index] ?? false"
      @select="emit('select-slot', index)"
      @clear="emit('clear-slot', index)"
      @playback-change="emit('playback-change', index, $event)"
    />
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type { PreviewLayout } from '@/api/preview'
import type { Stream, TreeStreamNode } from '@/api/streams'
import SlotCell from './SlotCell.vue'

const props = defineProps<{
  layout: PreviewLayout
  slots: (string | null)[]
  streamMap: Map<string, TreeStreamNode>
  streamDetailMap: Map<string, Stream>
  selectedSlotIndex: number | null
  pausedBySlot: Record<number, boolean>
}>()

const emit = defineEmits<{
  'select-slot': [index: number]
  'clear-slot': [index: number]
  'playback-change': [index: number, paused: boolean]
}>()

const cols = computed(() => Number(props.layout.split('x')[0]) || 1)

const gridStyle = computed(() => ({
  gridTemplateColumns: `repeat(${cols.value}, 1fr)`,
  gridTemplateRows: `repeat(${cols.value}, 1fr)`,
}))
</script>

<style scoped>
.grid-view {
  width: 100%;
  height: 100%;
  display: grid;
  gap: 1px;
  box-sizing: border-box;
  background: #303133;
}
</style>
