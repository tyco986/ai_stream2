<template>
  <div
    class="slot-player"
    :class="{ 'slot-player--selected': selected }"
    @click="emit('select')"
  >
    <button
      v-if="stream"
      type="button"
      class="slot-player__close"
      aria-label="clear"
      @click.stop="emit('clear')"
    >
      ×
    </button>
    <div v-if="!stream" class="slot-player__empty">
      {{ t('recordings.dropHere') }}
    </div>
    <div v-else-if="!selectedDate" class="slot-player__empty">
      {{ t('recordings.selectDate') }}
    </div>
    <div v-else-if="loading" class="slot-player__empty">
      {{ t('recordings.loading') }}
    </div>
    <div v-else-if="!hasSegment" class="slot-player__mask">
      <div>⊗</div>
      <div>{{ t('recordings.noRecording') }}</div>
    </div>
    <div v-else class="slot-player__video">
      <div class="slot-player__name">{{ stream.name }}</div>
      <div class="slot-player__clock">{{ clockLabel }}</div>
      <div class="slot-player__hint">
        {{ playing ? t('recordings.playing') : t('recordings.paused') }}
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { RecordingSegment } from '@/api/recordings'
import type { TreeStreamNode } from '@/api/streams'
import { findSegment } from '../utils/timeline'

const props = defineProps<{
  stream: TreeStreamNode | null
  selected: boolean
  selectedDate: string | null
  segments: RecordingSegment[]
  wallClockMs: number
  clockLabel: string
  playing: boolean
  loading: boolean
}>()

const emit = defineEmits<{
  select: []
  clear: []
}>()

const { t } = useI18n()

const hasSegment = computed(
  () => findSegment(props.segments, props.wallClockMs) !== null,
)
</script>

<style scoped>
.slot-player {
  position: relative;
  box-sizing: border-box;
  background: #1f1f1f;
  min-height: 0;
  min-width: 0;
  height: 100%;
  display: flex;
  flex-direction: column;
  cursor: pointer;
  color: #fff;
}

.slot-player--selected::after {
  content: '';
  position: absolute;
  inset: 0;
  box-shadow: inset 0 0 0 3px rgba(64, 158, 255, 0.55);
  pointer-events: none;
  z-index: 2;
}

.slot-player__close {
  position: absolute;
  top: 4px;
  right: 4px;
  z-index: 1;
  border: none;
  background: rgba(0, 0, 0, 0.55);
  color: #fff;
  width: 22px;
  height: 22px;
  border-radius: 4px;
  cursor: pointer;
  line-height: 1;
}

.slot-player__empty,
.slot-player__mask,
.slot-player__video {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 6px;
  color: #909399;
  font-size: 13px;
  background: linear-gradient(135deg, #303133 0%, #141414 100%);
}

.slot-player__mask {
  color: #c0c4cc;
  font-size: 16px;
}

.slot-player__video {
  color: #fff;
  background: linear-gradient(135deg, #1d3a5c 0%, #0f1c2c 100%);
}

.slot-player__name {
  font-size: 16px;
  font-weight: 600;
}

.slot-player__clock {
  font-family: monospace;
  font-size: 20px;
}

.slot-player__hint {
  font-size: 12px;
  color: #a0cfff;
}
</style>
