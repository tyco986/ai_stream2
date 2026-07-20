<template>
  <div
    class="slot-cell"
    :class="{
      'slot-cell--selected': selected,
      'slot-cell--empty': !stream,
      'slot-cell--compact': compact,
    }"
    @click="emit('select')"
  >
    <button
      v-if="stream"
      type="button"
      class="slot-cell__close"
      aria-label="clear"
      @click.stop="emit('clear')"
    >
      ×
    </button>
    <template v-if="stream">
      <div class="slot-cell__head">
        <span class="slot-cell__name">{{ stream.name }}</span>
        <span
          class="slot-cell__badge"
          :class="stream.status === 'online' ? 'is-live' : 'is-offline'"
        >
          {{ stream.status === 'online' ? t('preview.live') : t('preview.offline') }}
        </span>
      </div>
      <div class="slot-cell__body">
        <div v-if="stream.status === 'offline'" class="slot-cell__mask">⊗</div>
      </div>
      <div v-if="!compact" class="slot-cell__foot">
        {{ footerText }}
      </div>
    </template>
    <div v-else class="slot-cell__empty">{{ t('preview.dropHere') }}</div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { Stream } from '@/api/streams'
import type { TreeStreamNode } from '@/api/streams'

const props = withDefaults(
  defineProps<{
    stream: TreeStreamNode | null
    detail?: Stream | null
    selected: boolean
    compact?: boolean
  }>(),
  {
    detail: null,
    compact: false,
  },
)

const emit = defineEmits<{
  select: []
  clear: []
}>()

const { t } = useI18n()

const footerText = computed(() => {
  const resolution = props.detail?.resolution || '—'
  const fps = props.detail?.fps ?? '—'
  return `${resolution} · ${fps}fps`
})
</script>

<style scoped>
.slot-cell {
  position: relative;
  box-sizing: border-box;
  border: 1px solid #f56c6c;
  background: #1f1f1f;
  min-height: 0;
  min-width: 0;
  height: 100%;
  display: flex;
  flex-direction: column;
  cursor: pointer;
  color: #fff;
}

.slot-cell--selected::after {
  content: '';
  position: absolute;
  inset: 0;
  box-shadow: inset 0 0 0 3px rgba(64, 158, 255, 0.55);
  pointer-events: none;
  z-index: 2;
}

.slot-cell--compact {
  min-height: 88px;
}

.slot-cell__head {
  display: flex;
  justify-content: space-between;
  gap: 8px;
  padding: 6px 32px 6px 8px;
  font-size: 12px;
  background: rgba(0, 0, 0, 0.45);
}

.slot-cell__name {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.slot-cell__badge.is-live {
  color: #67c23a;
}

.slot-cell__badge.is-offline {
  color: #e6a23c;
}

.slot-cell__body {
  flex: 1;
  position: relative;
  background: linear-gradient(135deg, #303133 0%, #141414 100%);
}

.slot-cell__mask {
  position: absolute;
  inset: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  background: rgba(0, 0, 0, 0.45);
  font-size: 28px;
  color: #c0c4cc;
}

.slot-cell__foot {
  padding: 4px 8px;
  font-size: 11px;
  color: #c0c4cc;
  background: rgba(0, 0, 0, 0.45);
}

.slot-cell__empty {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  color: #909399;
  font-size: 13px;
}

.slot-cell__close {
  position: absolute;
  top: 4px;
  right: 4px;
  z-index: 3;
  border: none;
  background: rgba(0, 0, 0, 0.55);
  color: #fff;
  width: 22px;
  height: 22px;
  border-radius: 4px;
  cursor: pointer;
  line-height: 1;
}
</style>
