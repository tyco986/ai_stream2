<template>
  <div
    class="slot-cell"
    :class="{
      'is-selected': selected,
      'slot-cell--empty': !stream,
      'slot-cell--compact': compact,
      'slot-cell--contain': fit === 'contain',
    }"
    @click="emit('select')"
  >
    <template v-if="stream">
      <div class="slot-cell__body">
        <video
          ref="videoRef"
          class="slot-cell__video"
          autoplay
          muted
          playsinline
        />
        <div v-if="playState === 'loading'" class="slot-cell__mask">
          {{ t('preview.loading') }}
        </div>
        <div v-else-if="stream.status === 'offline'" class="slot-cell__mask">⊗</div>
        <div v-else-if="playState === 'failed'" class="slot-cell__mask">
          {{ t('preview.connectionFailed') }}
        </div>
      </div>
      <div class="slot-cell__chrome">
        <div class="slot-cell__toolbar">
          <button
            type="button"
            class="slot-cell__btn"
            :aria-label="effectivePaused ? 'play' : 'pause'"
            :disabled="playState !== 'playing' && !effectivePaused"
            @click.stop="togglePlayback"
          >
            <UiIcon
              :name="effectivePaused ? 'videoPlay' : 'videoPause'"
              :size="compact ? 14 : 16"
            />
          </button>
          <div class="slot-cell__sep" aria-hidden="true" />
          <div class="slot-cell__head">
            <span class="slot-cell__name">{{ stream.name }}</span>
            <span class="slot-cell__meta">{{ metaText }}</span>
            <span
              class="slot-cell__badge"
              :class="stream.status === 'online' ? 'is-live' : 'is-offline'"
            >
              {{ stream.status === 'online' ? t('preview.live') : t('preview.offline') }}
            </span>
          </div>
          <div class="slot-cell__sep" aria-hidden="true" />
          <button
            type="button"
            class="slot-cell__btn"
            aria-label="clear"
            @click.stop="emit('clear')"
          >
            ×
          </button>
        </div>
      </div>
    </template>
    <div v-else class="slot-cell__empty">{{ index + 1 }}</div>
    <div v-if="selected" class="slot-cell__selection" aria-hidden="true" />
  </div>
</template>

<script setup lang="ts">
import { computed, nextTick, onBeforeUnmount, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import type { Stream } from '@/api/streams'
import type { TreeStreamNode } from '@/api/streams'
import UiIcon from '@/shared/ui/Icon.vue'
import {
  type WhepPlayState,
} from '../utils/mediamtxWhep'
import {
  bindSlotMedia,
  peekSlotMedia,
  slotMediaMap,
  unbindSlotMedia,
} from '../utils/slotMediaHub'

const props = withDefaults(
  defineProps<{
    index: number
    stream: TreeStreamNode | null
    detail?: Stream | null
    selected: boolean
    compact?: boolean
    /** null = uncontrolled local pause; boolean = synced from parent */
    paused?: boolean | null
    /** Reuse an existing MediaStream (focus main mirrors list thumb). */
    mirrorSrcObject?: MediaStream | null
    useMirror?: boolean
    fit?: 'cover' | 'contain'
  }>(),
  {
    detail: null,
    compact: false,
    paused: null,
    mirrorSrcObject: null,
    useMirror: false,
    fit: 'cover',
  },
)

const emit = defineEmits<{
  select: []
  clear: []
  'playback-change': [paused: boolean]
}>()

const { t } = useI18n()
const videoRef = ref<HTMLVideoElement | null>(null)
const playState = ref<WhepPlayState>('idle')
const localPaused = ref(false)

const effectivePaused = computed(() =>
  props.paused === null ? localPaused.value : props.paused,
)

const metaText = computed(() => {
  const resolution = props.detail?.resolution || '—'
  const fps = props.detail?.fps ?? '—'
  return `${resolution} · ${fps} FPS`
})

async function applyPaused(next: boolean) {
  const videoEl = videoRef.value
  if (!videoEl || playState.value === 'failed' || playState.value === 'loading') {
    return
  }
  if (next) {
    videoEl.pause()
  } else if (videoEl.paused) {
    await videoEl.play()
    playState.value = 'playing'
  }
}

function detachVideo() {
  if (videoRef.value) {
    videoRef.value.srcObject = null
  }
  localPaused.value = false
}

async function startPlay() {
  const stream = props.stream
  const videoEl = videoRef.value
  if (!stream || !videoEl) {
    return
  }
  if (stream.status === 'offline' || !stream.enabled) {
    playState.value = 'idle'
    detachVideo()
    return
  }
  const pathName = props.detail?.name || stream.name
  const existing = peekSlotMedia(props.index, stream.id)
  if (existing) {
    videoEl.srcObject = existing
    localPaused.value = false
    if (effectivePaused.value) {
      videoEl.pause()
    } else {
      await videoEl.play()
    }
    playState.value = 'playing'
    return
  }
  playState.value = 'loading'
  localPaused.value = false
  const state = await bindSlotMedia({
    slotIndex: props.index,
    streamId: stream.id,
    pathName,
    videoEl,
  })
  if (state === 'playing') {
    if (effectivePaused.value) {
      videoEl.pause()
    } else {
      await videoEl.play()
    }
  }
  playState.value = state
}

async function applyMirror() {
  localPaused.value = false
  const stream = props.stream
  const videoEl = videoRef.value
  if (!stream || !videoEl) {
    detachVideo()
    playState.value = 'idle'
    return
  }
  if (stream.status === 'offline' || !stream.enabled) {
    detachVideo()
    playState.value = 'idle'
    return
  }
  const media = props.mirrorSrcObject
  if (!media) {
    videoEl.srcObject = null
    playState.value = 'loading'
    return
  }
  if (videoEl.srcObject !== media) {
    videoEl.srcObject = media
  }
  if (effectivePaused.value) {
    videoEl.pause()
  } else {
    await videoEl.play()
  }
  playState.value = 'playing'
}

async function togglePlayback() {
  const next = !effectivePaused.value
  if (props.paused === null) {
    await applyPaused(next)
    localPaused.value = next
  }
  emit('playback-change', next)
}

watch(
  () => props.paused,
  async (value) => {
    if (value === null) {
      return
    }
    await applyPaused(value)
  },
)

watch(
  () => [
    props.useMirror,
    props.mirrorSrcObject,
    props.stream?.id,
    props.stream?.status,
    props.stream?.enabled,
    props.detail?.name,
    videoRef.value,
    // Only mirrors react to hub updates; owners write the map and must not re-enter.
    props.useMirror ? slotMediaMap.value[props.index] : null,
  ],
  async () => {
    await nextTick()
    if (props.useMirror) {
      await applyMirror()
      return
    }
    if (!props.stream) {
      unbindSlotMedia(props.index)
      detachVideo()
      playState.value = 'idle'
      return
    }
    await startPlay()
  },
  { immediate: true },
)

onBeforeUnmount(() => {
  // Keep WHEP alive across grid/focus remounts; only detach this video element.
  detachVideo()
})
</script>

<style scoped>
.slot-cell {
  position: relative;
  box-sizing: border-box;
  border: none;
  background: #000;
  min-height: 0;
  min-width: 0;
  height: 100%;
  width: 100%;
  display: flex;
  flex-direction: column;
  cursor: pointer;
  color: #fff;
  overflow: visible;
  --chrome-bar-size: 26px;
}

.slot-cell.is-selected {
  z-index: 2;
}

.slot-cell__selection {
  position: absolute;
  inset: 0;
  box-sizing: border-box;
  border: none;
  box-shadow: inset 0 0 0 3px #409eff;
  pointer-events: none;
  z-index: 5;
}

.slot-cell--compact {
  min-height: 88px;
  --chrome-bar-size: 22px;
}

.slot-cell__body {
  flex: 1;
  position: relative;
  background: #000;
  min-height: 0;
  overflow: hidden;
}

.slot-cell__video {
  position: absolute;
  inset: 0;
  width: 100%;
  height: 100%;
  object-fit: cover;
  background: #000;
}

.slot-cell--contain .slot-cell__video {
  object-fit: contain;
}

.slot-cell__mask {
  position: absolute;
  inset: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  background: rgba(0, 0, 0, 0.45);
  font-size: 16px;
  color: #c0c4cc;
  z-index: 1;
}

.slot-cell__chrome {
  position: absolute;
  inset: 0;
  z-index: 3;
  pointer-events: none;
  opacity: 0;
  transition: opacity 0.15s ease;
}

.slot-cell:hover .slot-cell__chrome {
  opacity: 1;
}

.slot-cell__toolbar {
  position: absolute;
  top: 0;
  left: 0;
  display: flex;
  align-items: stretch;
  max-width: 100%;
  height: var(--chrome-bar-size);
  pointer-events: none;
}

.slot-cell__btn {
  box-sizing: border-box;
  flex-shrink: 0;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: var(--chrome-bar-size);
  height: var(--chrome-bar-size);
  padding: 0;
  border: none;
  border-radius: 0;
  background: rgba(0, 0, 0, 0.55);
  color: #fff;
  font-size: 16px;
  line-height: 1;
  cursor: pointer;
  pointer-events: auto;
}

.slot-cell--compact .slot-cell__btn {
  font-size: 14px;
}

.slot-cell__btn:disabled {
  opacity: 0.45;
  cursor: not-allowed;
}

.slot-cell__sep {
  flex-shrink: 0;
  width: 1px;
  height: 100%;
  background: rgba(255, 255, 255, 0.28);
}

.slot-cell__head {
  display: flex;
  align-items: center;
  justify-content: flex-start;
  gap: 8px;
  min-width: 0;
  flex: 0 1 auto;
  height: var(--chrome-bar-size);
  padding: 0 8px;
  box-sizing: border-box;
  font-size: 12px;
  line-height: 1;
  background: rgba(0, 0, 0, 0.55);
  overflow: hidden;
}

.slot-cell__name {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  min-width: 0;
  line-height: 1;
}

.slot-cell__meta {
  flex-shrink: 0;
  color: #c0c4cc;
  line-height: 1;
  white-space: nowrap;
}

.slot-cell__badge {
  flex-shrink: 0;
  line-height: 1;
}

.slot-cell__badge.is-live {
  color: #67c23a;
}

.slot-cell__badge.is-offline {
  color: #e6a23c;
}

.slot-cell--compact .slot-cell__head {
  padding: 0 6px;
  font-size: 11px;
  gap: 6px;
}

.slot-cell__empty {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  color: #606266;
  font-size: 64px;
  font-weight: 600;
  line-height: 1;
  user-select: none;
}
</style>
