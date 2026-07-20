<template>
  <div
    class="grid-view"
    :style="{
      gridTemplateColumns: `repeat(${cols}, 1fr)`,
      gridTemplateRows: `repeat(${cols}, 1fr)`,
    }"
  >
    <SlotCell
      v-for="(slotId, index) in slots"
      :key="index"
      :stream="slotId ? streamMap.get(slotId) ?? null : null"
      :detail="slotId ? streamDetailMap.get(slotId) ?? null : null"
      :selected="selectedSlotIndex === index"
      @select="emit('select-slot', index)"
      @clear="emit('clear-slot', index)"
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
}>()

const emit = defineEmits<{
  'select-slot': [index: number]
  'clear-slot': [index: number]
}>()

const cols = computed(() => Number(props.layout.split('x')[0]))
</script>

<style scoped>
.grid-view {
  display: grid;
  width: 100%;
  height: 100%;
  gap: 1px;
  padding: 1px;
  box-sizing: border-box;
  background: #f56c6c;
}

.grid-view :deep(.slot-cell) {
  border: none;
}
</style>
