<template>
  <UiDialog
    :model-value="modelValue"
    :title="dialogTitle"
    width="720px"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <div class="transfer">
      <div class="transfer__pane">
        <div class="transfer__title">{{ t('streams.available') }}</div>
        <div class="transfer__list">
          <label
            v-for="item in leftItems"
            :key="item.id"
            class="transfer__row"
          >
            <input
              type="checkbox"
              :checked="leftSelected.has(item.id)"
              @change="toggleLeft(item.id)"
            />
            <span>{{ item.name }}</span>
            <span class="transfer__group">{{ item.group_name }}</span>
          </label>
        </div>
      </div>
      <div class="transfer__mid">
        <UiButton
          class="transfer__move"
          type="primary"
          :disabled="leftSelected.size === 0"
          @click="moveRight"
        >
          <UiIcon name="arrowRight" :size="14" />
        </UiButton>
        <UiButton
          class="transfer__move"
          type="primary"
          :disabled="rightSelected.size === 0"
          @click="moveLeft"
        >
          <UiIcon name="arrowLeft" :size="14" />
        </UiButton>
      </div>
      <div class="transfer__pane">
        <div class="transfer__title">{{ t('streams.targetGroup') }}</div>
        <div class="transfer__list">
          <label
            v-for="item in rightItems"
            :key="item.id"
            class="transfer__row"
          >
            <input
              type="checkbox"
              :checked="rightSelected.has(item.id)"
              @change="toggleRight(item.id)"
            />
            <span>{{ item.name }}</span>
            <span class="transfer__group">{{ item.group_name }}</span>
          </label>
        </div>
      </div>
    </div>
    <template #footer>
      <UiButton @click="emit('update:modelValue', false)">{{ t('streams.cancel') }}</UiButton>
      <UiButton type="primary" :loading="saving" @click="onSave">
        {{ t('streams.save') }}
      </UiButton>
    </template>
  </UiDialog>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import type { StreamSummary } from '@/api/streams'
import UiButton from '@/shared/ui/Button.vue'
import UiDialog from '@/shared/ui/Dialog.vue'
import UiIcon from '@/shared/ui/Icon.vue'

const props = defineProps<{
  modelValue: boolean
  groupName: string
  candidates: StreamSummary[]
  members: StreamSummary[]
  saving: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  save: [streamIds: string[]]
}>()

const { t } = useI18n()

const leftItems = ref<StreamSummary[]>([])
const rightItems = ref<StreamSummary[]>([])
const leftSelected = ref(new Set<string>())
const rightSelected = ref(new Set<string>())

const dialogTitle = computed(
  () => `${t('streams.transferTitle')}: ${props.groupName}`,
)

watch(
  () => [props.modelValue, props.candidates, props.members] as const,
  () => {
    if (!props.modelValue) {
      return
    }
    leftItems.value = [...props.candidates]
    rightItems.value = [...props.members]
    leftSelected.value = new Set()
    rightSelected.value = new Set()
  },
)

function toggleLeft(id: string) {
  const next = new Set(leftSelected.value)
  if (next.has(id)) {
    next.delete(id)
  } else {
    next.add(id)
  }
  leftSelected.value = next
}

function toggleRight(id: string) {
  const next = new Set(rightSelected.value)
  if (next.has(id)) {
    next.delete(id)
  } else {
    next.add(id)
  }
  rightSelected.value = next
}

function moveRight() {
  const moving = leftItems.value.filter((item) => leftSelected.value.has(item.id))
  leftItems.value = leftItems.value.filter((item) => !leftSelected.value.has(item.id))
  rightItems.value = [...rightItems.value, ...moving]
  leftSelected.value = new Set()
}

function moveLeft() {
  const moving = rightItems.value.filter((item) => rightSelected.value.has(item.id))
  rightItems.value = rightItems.value.filter((item) => !rightSelected.value.has(item.id))
  leftItems.value = [...leftItems.value, ...moving]
  rightSelected.value = new Set()
}

function onSave() {
  emit(
    'save',
    rightItems.value.map((item) => item.id),
  )
}
</script>

<style scoped>
.transfer {
  display: grid;
  grid-template-columns: 1fr auto 1fr;
  gap: 12px;
  min-height: 280px;
}

.transfer__pane {
  border: 1px solid #e4e7ed;
  border-radius: 4px;
  display: flex;
  flex-direction: column;
  min-height: 0;
}

.transfer__title {
  padding: 8px 10px;
  font-weight: 600;
  border-bottom: 1px solid #e4e7ed;
}

.transfer__list {
  flex: 1;
  overflow: auto;
  padding: 6px 0;
}

.transfer__row {
  display: grid;
  grid-template-columns: 20px 1fr auto;
  gap: 8px;
  align-items: center;
  padding: 6px 10px;
  cursor: pointer;
}

.transfer__row:hover {
  background: #f5f7fa;
}

.transfer__group {
  color: #909399;
  font-size: 12px;
}

.transfer__mid {
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  gap: 8px;
}

.transfer__mid :deep(.transfer__move + .transfer__move) {
  margin-left: 0;
}
</style>
