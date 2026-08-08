<template>
  <UiDialog
    :model-value="modelValue"
    :title="t('events.preview')"
    width="780px"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <div
      v-if="detail"
      class="preview"
      tabindex="0"
      @wheel.prevent="onWheel"
    >
      <div class="preview__meta">
        <div>{{ t('events.timestamp') }}: {{ formatEventTimestamp(detail.occurred_at) }}</div>
        <div>{{ t('events.stream') }}: {{ detail.stream_name }}</div>
        <div>{{ t('events.pipeline') }}: {{ detail.pipeline_name }}</div>
        <div>{{ t('events.event') }}: {{ detail.event }}</div>
        <div>
          {{ t('events.status') }}:
          {{
            detail.status === 'new' ? t('events.statusNew') : t('events.statusAcked')
          }}
        </div>
      </div>

      <div class="preview__modes">
        <UiButton
          :type="imageMode === 'raw' ? 'primary' : 'default'"
          @click="imageMode = 'raw'"
        >
          {{ t('events.raw') }}
        </UiButton>
        <UiButton
          :type="imageMode === 'visualization' ? 'primary' : 'default'"
          :disabled="!detail.visualization_url"
          @click="imageMode = 'visualization'"
        >
          {{ t('events.visualization') }}
        </UiButton>
      </div>

      <div class="preview__image-wrap">
        <img
          v-if="currentImageUrl"
          class="preview__image"
          :src="currentImageUrl"
          :alt="imageMode"
        />
        <div v-else class="preview__empty">—</div>
        <button
          type="button"
          class="preview__nav preview__nav--prev"
          :disabled="!detail.prev_id || loading"
          :title="t('events.prev')"
          @click="emit('prev')"
        >
          ‹
        </button>
        <button
          type="button"
          class="preview__nav preview__nav--next"
          :disabled="!detail.next_id || loading"
          :title="t('events.next')"
          @click="emit('next')"
        >
          ›
        </button>
      </div>

      <div class="preview__footer">
        <UiButton @click="emit('playback')">{{ t('events.playback') }}</UiButton>
        <UiButton
          type="primary"
          :disabled="loading || detail.status !== 'new'"
          @click="emit('ack')"
        >
          {{ t('events.ack') }}
        </UiButton>
        <UiButton
          type="warning"
          :disabled="loading || detail.status !== 'acked'"
          @click="emit('unack')"
        >
          {{ t('events.unack') }}
        </UiButton>
      </div>
    </div>
  </UiDialog>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import { formatEventTimestamp, type EventDetail } from '@/api/events'
import UiButton from '@/shared/ui/Button.vue'
import UiDialog from '@/shared/ui/Dialog.vue'

const props = defineProps<{
  modelValue: boolean
  detail: EventDetail | null
  loading?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  prev: []
  next: []
  ack: []
  unack: []
  playback: []
}>()

const { t } = useI18n()
const imageMode = ref<'raw' | 'visualization'>('visualization')

watch(
  () => props.detail?.id,
  () => {
    if (props.detail?.visualization_url) {
      imageMode.value = 'visualization'
      return
    }
    imageMode.value = 'raw'
  },
)

const currentImageUrl = computed(() => {
  if (!props.detail) {
    return null
  }
  if (imageMode.value === 'visualization') {
    return props.detail.visualization_url ?? props.detail.raw_url
  }
  return props.detail.raw_url
})

function onWheel(event: WheelEvent) {
  if (props.loading) {
    return
  }
  if (event.deltaY < 0) {
    emit('prev')
    return
  }
  if (event.deltaY > 0) {
    emit('next')
  }
}
</script>

<style scoped>
.preview {
  display: flex;
  flex-direction: column;
  gap: 12px;
  outline: none;
}

.preview__meta {
  display: grid;
  gap: 4px;
  font-size: 13px;
  color: #606266;
}

.preview__modes {
  display: flex;
  gap: 8px;
}

.preview__image-wrap {
  position: relative;
  width: 100%;
  min-height: 320px;
  background: #111;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 4px;
  overflow: hidden;
}

.preview__image {
  max-width: 100%;
  max-height: 420px;
  object-fit: contain;
}

.preview__empty {
  color: #909399;
}

.preview__nav {
  position: absolute;
  top: 50%;
  transform: translateY(-50%);
  z-index: 2;
  width: 36px;
  height: 56px;
  padding: 0;
  border: none;
  border-radius: 4px;
  background: rgba(0, 0, 0, 0.45);
  color: rgba(255, 255, 255, 0.9);
  font-size: 28px;
  line-height: 1;
  cursor: pointer;
  display: inline-flex;
  align-items: center;
  justify-content: center;
}

.preview__nav:hover:not(:disabled) {
  background: rgba(0, 0, 0, 0.65);
}

.preview__nav:disabled {
  opacity: 0.25;
  cursor: not-allowed;
}

.preview__nav--prev {
  left: 8px;
}

.preview__nav--next {
  right: 8px;
}

.preview__footer {
  display: flex;
  justify-content: flex-end;
  gap: 8px;
}
</style>
