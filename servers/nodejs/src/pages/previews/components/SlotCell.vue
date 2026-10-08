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
        <div ref="videoHost" class="slot-cell__video-host" />
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
import { computed, onBeforeUnmount, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import type { Stream, TreeStreamNode } from '@/api/streams'
import UiIcon from '@/shared/ui/Icon.vue'
import {
  type WhepPlayState,
} from '../utils/mediamtxWhep'
import {
  ensureSlotMedia,
  mountSlotVideo,
  setSlotPaused,
  slotMediaStatus,
  unbindSlotMedia,
  unmountSlotVideo,
} from '../utils/slotMediaHub'

const props = withDefaults(
  defineProps<{
    index: number
    stream: TreeStreamNode | null
    detail?: Stream | null
    selected: boolean
    compact?: boolean
    paused?: boolean
    /** Focus main shows the hub mirror element; the list keeps the primary. */
    useMirror?: boolean
    fit?: 'cover' | 'contain'
  }>(),
  {
    detail: null,
    compact: false,
    paused: false,
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
const videoHost = ref<HTMLElement | null>(null)
const playState = ref<WhepPlayState>('idle')
let syncToken = 0

const effectivePaused = computed(() => props.paused)

const metaText = computed(() => {
  const resolution = props.detail?.resolution || '—'
  const fps = props.detail?.fps ?? '—'
  return `${resolution} · ${fps} FPS`
})

function parkHost() {
  const host = videoHost.value
  if (host) {
    unmountSlotVideo(host)
  }
}

function mountHost(token: number, mirror: boolean) {
  const host = videoHost.value
  if (host && token === syncToken) {
    mountSlotVideo(props.index, host, mirror)
  }
}

async function syncMirror(token: number) {
  const host = videoHost.value
  const stream = props.stream
  let state: WhepPlayState = 'idle'
  if (host && stream && stream.status !== 'offline' && stream.enabled) {
    mountHost(token, true)
    state = slotMediaStatus.value[props.index] ?? 'loading'
    await setSlotPaused(props.index, props.paused)
  }
  if (token === syncToken) {
    playState.value = state
  }
}

async function syncOwner(token: number) {
  const host = videoHost.value
  const stream = props.stream
  let state: WhepPlayState = 'idle'
  if (!stream) {
    unbindSlotMedia(props.index)
  } else if (host && stream.status !== 'offline' && stream.enabled) {
    const pathName = props.detail?.name || stream.name
    const pending = ensureSlotMedia({
      slotIndex: props.index,
      streamId: stream.id,
      pathName,
    })
    mountHost(token, false)
    await setSlotPaused(props.index, props.paused)
    state = await pending
    if (token === syncToken) {
      mountHost(token, false)
      await setSlotPaused(props.index, props.paused)
    }
  }
  if (token === syncToken) {
    playState.value = state
  }
}

async function syncPlayback() {
  syncToken += 1
  const token = syncToken
  if (props.useMirror) {
    await syncMirror(token)
  } else {
    await syncOwner(token)
  }
}

function togglePlayback() {
  emit('playback-change', !props.paused)
}

watch(
  () => props.paused,
  (paused) => {
    void setSlotPaused(props.index, paused)
  },
)

watch(
  () => ({
    mirror: props.useMirror,
    index: props.index,
    streamId: props.stream?.id ?? '',
    status: props.stream?.status ?? '',
    enabled: props.stream?.enabled ?? false,
    pathName: props.detail?.name || props.stream?.name || '',
    host: videoHost.value,
    mediaStatus: props.useMirror ? (slotMediaStatus.value[props.index] ?? 'idle') : '',
  }),
  () => {
    void syncPlayback()
  },
  { immediate: true },
)

onBeforeUnmount(() => {
  syncToken += 1
  parkHost()
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

.slot-cell__video-host {
  position: absolute;
  inset: 0;
}

.slot-cell__video-host :deep(.slot-cell__video) {
  position: absolute;
  inset: 0;
  width: 100%;
  height: 100%;
  object-fit: cover;
  background: #000;
}

.slot-cell--contain .slot-cell__video-host :deep(.slot-cell__video) {
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
