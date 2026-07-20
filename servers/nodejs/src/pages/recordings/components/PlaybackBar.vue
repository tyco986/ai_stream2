<template>
  <div class="playback-bar" :class="{ 'is-disabled': disabled }">
    <div class="playback-bar__controls">
      <button
        type="button"
        class="playback-bar__btn"
        :disabled="disabled"
        :title="playing ? t('recordings.pause') : t('recordings.play')"
        @click="emit('toggle-play')"
      >
        <UiIcon :name="playing ? 'videoPause' : 'videoPlay'" :size="16" />
      </button>
      <button
        type="button"
        class="playback-bar__btn"
        :disabled="disabled"
        :title="t('recordings.rewind')"
        @click="emit('seek-by', -5000)"
      >
        <UiIcon name="rewind" :size="16" />
      </button>
      <button
        type="button"
        class="playback-bar__btn"
        :disabled="disabled"
        :title="t('recordings.forward')"
        @click="emit('seek-by', 5000)"
      >
        <UiIcon name="forward" :size="16" />
      </button>
      <div class="playback-bar__audio">
        <button
          type="button"
          class="playback-bar__btn"
          :disabled="disabled"
          :title="t('recordings.audio')"
          @click="menuOpen = !menuOpen"
        >
          <UiIcon :name="muted ? 'mute' : 'volume'" :size="16" />
          ▼
        </button>
        <div v-if="menuOpen && !disabled" class="playback-bar__menu">
          <label
            v-for="item in audioSources"
            :key="item.index"
            class="playback-bar__menu-item"
          >
            <input
              type="radio"
              name="audio-source"
              :checked="audioSlotIndex === item.index"
              @change="onPickAudio(item.index)"
            />
            {{ item.name }}
          </label>
          <div class="playback-bar__volume">
            <span>{{ t('recordings.volume') }}</span>
            <input
              type="range"
              min="0"
              max="1"
              step="0.05"
              :value="volume"
              @input="onVolume"
            />
          </div>
        </div>
      </div>
    </div>

    <div
      class="playback-bar__timeline"
      :class="{ 'is-dragging': dragging }"
      @pointerdown="onPointerDown"
      @pointermove="onPointerMove"
      @pointerup="onPointerUp"
      @pointercancel="onPointerUp"
    >
      <div class="playback-bar__track">
        <div
          v-for="(range, index) in trackFills"
          :key="index"
          class="playback-bar__fill"
          :style="{ left: range.left, width: range.width }"
        />
        <div class="playback-bar__head" :style="{ left: headPercent }" />
      </div>
      <div class="playback-bar__ticks">
        <span>00:00</span>
        <span>12:00</span>
        <span>23:59</span>
      </div>
    </div>

    <div class="playback-bar__meta">
      <span class="playback-bar__clock">{{ clockLabel }}</span>
      <select
        class="playback-bar__rate"
        :disabled="disabled"
        :value="playbackRate"
        @change="onRate"
      >
        <option v-for="rate in rates" :key="rate" :value="rate">{{ rate }}x</option>
      </select>
      <button
        type="button"
        class="playback-bar__btn"
        :disabled="disabled"
        :title="t('recordings.fullscreen')"
        @click="emit('fullscreen')"
      >
        <UiIcon name="fullScreen" :size="16" />
      </button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'
import { useI18n } from 'vue-i18n'
import UiIcon from '@/shared/ui/Icon.vue'
import { dateStartMs, type TimeRange } from '../utils/timeline'

const props = defineProps<{
  disabled: boolean
  playing: boolean
  clockLabel: string
  wallClockMs: number
  selectedDate: string | null
  timelineRanges: TimeRange[]
  playbackRate: number
  muted: boolean
  volume: number
  audioSlotIndex: number | null
  audioSources: { index: number; name: string }[]
}>()

const emit = defineEmits<{
  'toggle-play': []
  'seek-by': [deltaMs: number]
  seek: [ms: number]
  'set-volume': [value: number]
  'set-audio-slot': [index: number]
  'set-rate': [rate: number]
  fullscreen: []
}>()

const { t } = useI18n()
const menuOpen = ref(false)
const dragging = ref(false)
const rates = [0.5, 1, 1.5, 2]

const daySpanMs = 24 * 60 * 60 * 1000 - 1000

const headPercent = computed(() => {
  if (!props.selectedDate) {
    return '0%'
  }
  const start = dateStartMs(props.selectedDate)
  const ratio = (props.wallClockMs - start) / daySpanMs
  return `${Math.min(100, Math.max(0, ratio * 100))}%`
})

const trackFills = computed(() => {
  if (!props.selectedDate) {
    return []
  }
  const start = dateStartMs(props.selectedDate)
  return props.timelineRanges.map((range) => {
    const left = ((range.startMs - start) / daySpanMs) * 100
    const width = ((range.endMs - range.startMs) / daySpanMs) * 100
    return {
      left: `${Math.max(0, left)}%`,
      width: `${Math.max(0, width)}%`,
    }
  })
})

function seekFromClientX(el: HTMLElement, clientX: number) {
  if (!props.selectedDate) {
    return
  }
  const rect = el.getBoundingClientRect()
  const ratio = Math.min(1, Math.max(0, (clientX - rect.left) / rect.width))
  const ms = dateStartMs(props.selectedDate) + ratio * daySpanMs
  emit('seek', ms)
}

function onPointerDown(event: PointerEvent) {
  if (props.disabled || !props.selectedDate || event.button !== 0) {
    return
  }
  const el = event.currentTarget as HTMLElement
  dragging.value = true
  el.setPointerCapture(event.pointerId)
  seekFromClientX(el, event.clientX)
}

function onPointerMove(event: PointerEvent) {
  if (!dragging.value || props.disabled || !props.selectedDate) {
    return
  }
  seekFromClientX(event.currentTarget as HTMLElement, event.clientX)
}

function onPointerUp(event: PointerEvent) {
  if (!dragging.value) {
    return
  }
  dragging.value = false
  if (props.disabled || !props.selectedDate) {
    return
  }
  seekFromClientX(event.currentTarget as HTMLElement, event.clientX)
}

function onPickAudio(index: number) {
  emit('set-audio-slot', index)
}

function onVolume(event: Event) {
  const value = Number((event.target as HTMLInputElement).value)
  emit('set-volume', value)
}

function onRate(event: Event) {
  const value = Number((event.target as HTMLSelectElement).value)
  emit('set-rate', value)
}
</script>

<style scoped>
.playback-bar {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 8px 10px;
  background: #1f1f1f;
  color: #fff;
  border-top: 1px solid #303133;
  flex-shrink: 0;
}

.playback-bar.is-disabled {
  opacity: 0.55;
}

.playback-bar__controls {
  display: flex;
  align-items: center;
  gap: 4px;
  flex-shrink: 0;
}

.playback-bar__btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 2px;
  min-width: 28px;
  height: 28px;
  border: none;
  border-radius: 4px;
  background: transparent;
  color: #fff;
  cursor: pointer;
}

.playback-bar__btn:disabled {
  cursor: not-allowed;
}

.playback-bar__btn:hover:not(:disabled) {
  background: #303133;
}

.playback-bar__audio {
  position: relative;
}

.playback-bar__menu {
  position: absolute;
  bottom: 34px;
  left: 0;
  z-index: 5;
  min-width: 160px;
  padding: 8px;
  background: #fff;
  color: #303133;
  border-radius: 4px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.2);
}

.playback-bar__menu-item {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 4px 0;
  font-size: 13px;
  cursor: pointer;
}

.playback-bar__volume {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-top: 8px;
  padding-top: 8px;
  border-top: 1px solid #ebeef5;
  font-size: 12px;
}

.playback-bar__timeline {
  flex: 1;
  min-width: 0;
  cursor: pointer;
  user-select: none;
  touch-action: none;
}

.playback-bar__timeline.is-dragging {
  cursor: grabbing;
}

.playback-bar__track {
  position: relative;
  height: 10px;
  border-radius: 5px;
  background: #4a4a4a;
  overflow: hidden;
}

.playback-bar__fill {
  position: absolute;
  top: 0;
  bottom: 0;
  background: #67c23a;
}

.playback-bar__head {
  position: absolute;
  top: -2px;
  width: 3px;
  height: 14px;
  background: #fff;
  transform: translateX(-50%);
}

.playback-bar__ticks {
  display: flex;
  justify-content: space-between;
  margin-top: 4px;
  font-size: 11px;
  color: #a8abb2;
}

.playback-bar__meta {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-shrink: 0;
}

.playback-bar__clock {
  font-family: monospace;
  font-size: 13px;
  min-width: 70px;
}

.playback-bar__rate {
  background: #303133;
  color: #fff;
  border: 1px solid #4a4a4a;
  border-radius: 4px;
  height: 28px;
}
</style>
