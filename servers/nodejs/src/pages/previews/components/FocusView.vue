<template>
  <div class="focus-view">
    <div class="focus-view__main">
      <SlotCell
        :stream="focusStream"
        :detail="focusDetail"
        :selected="selectedSlotIndex === focusIndex"
        @select="emit('select-slot', focusIndex)"
        @clear="emit('clear-slot', focusIndex)"
      />
    </div>
    <div class="focus-view__list">
      <div
        v-for="(slotId, index) in slots"
        :key="index"
        class="focus-view__item"
        :class="{ 'is-focus': index === focusIndex }"
      >
        <SlotCell
          :stream="slotId ? streamMap.get(slotId) ?? null : null"
          :detail="slotId ? streamDetailMap.get(slotId) ?? null : null"
          :selected="selectedSlotIndex === index"
          compact
          @select="emit('select-slot', index)"
          @clear="emit('clear-slot', index)"
        />
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type { Stream, TreeStreamNode } from '@/api/streams'
import SlotCell from './SlotCell.vue'

const props = defineProps<{
  slots: (string | null)[]
  streamMap: Map<string, TreeStreamNode>
  streamDetailMap: Map<string, Stream>
  focusIndex: number
  selectedSlotIndex: number | null
}>()

const emit = defineEmits<{
  'select-slot': [index: number]
  'clear-slot': [index: number]
}>()

const focusStream = computed(() => {
  const id = props.slots[props.focusIndex]
  if (!id) {
    return null
  }
  return props.streamMap.get(id) ?? null
})

const focusDetail = computed(() => {
  const id = props.slots[props.focusIndex]
  if (!id) {
    return null
  }
  return props.streamDetailMap.get(id) ?? null
})
</script>

<style scoped>
.focus-view {
  display: flex;
  height: 100%;
  min-height: 0;
  gap: 0;
}

.focus-view__main {
  flex: 1;
  min-width: 0;
}

.focus-view__list {
  width: 240px;
  flex-shrink: 0;
  overflow-y: auto;
  border-left: 1px solid #303133;
  background: #000;
  display: flex;
  flex-direction: column;
}

.focus-view__item {
  height: 120px;
  flex-shrink: 0;
}

.focus-view__item + .focus-view__item {
  margin-top: -1px;
}

.focus-view__item.is-focus {
  outline: 2px solid #409eff;
  outline-offset: -2px;
  z-index: 1;
}
</style>
