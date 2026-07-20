<template>
  <div
    class="playback-grid"
    :style="{
      gridTemplateColumns: `repeat(${cols}, 1fr)`,
      gridTemplateRows: `repeat(${cols}, 1fr)`,
    }"
  >
    <SlotPlayer
      v-for="(slotId, index) in slots"
      :key="index"
      :stream="slotId ? streamMap.get(slotId) ?? null : null"
      :selected="selectedSlotIndex === index"
      :selected-date="selectedDate"
      :segments="slotId ? segmentsByStream.get(slotId) ?? [] : []"
      :wall-clock-ms="wallClockMs"
      :clock-label="clockLabel"
      :playing="playing"
      :loading="segmentsLoading"
      @select="emit('select-slot', index)"
      @clear="emit('clear-slot', index)"
    />
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type { RecordingSegment, RecordingsLayout } from '@/api/recordings'
import type { TreeStreamNode } from '@/api/streams'
import SlotPlayer from './SlotPlayer.vue'

const props = defineProps<{
  layout: RecordingsLayout
  slots: (string | null)[]
  streamMap: Map<string, TreeStreamNode>
  selectedSlotIndex: number | null
  selectedDate: string | null
  segmentsByStream: Map<string, RecordingSegment[]>
  wallClockMs: number
  clockLabel: string
  playing: boolean
  segmentsLoading: boolean
}>()

const emit = defineEmits<{
  'select-slot': [index: number]
  'clear-slot': [index: number]
}>()

const cols = computed(() => Number(props.layout.split('x')[0]))
</script>

<style scoped>
.playback-grid {
  display: grid;
  width: 100%;
  height: 100%;
  gap: 2px;
  padding: 2px;
  box-sizing: border-box;
  background: #f56c6c;
}
</style>
