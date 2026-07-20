<template>
  <div class="streams-toolbar">
    <div class="streams-toolbar__actions">
      <UiButton type="primary" @click="emit('add')">{{ t('streams.add') }}</UiButton>
      <UiButton type="danger" :disabled="!hasSelection" @click="emit('remove')">
        {{ t('streams.remove') }}
      </UiButton>
      <UiButton type="primary" :disabled="!hasSelection" @click="emit('enable')">
        {{ t('streams.enable') }}
      </UiButton>
      <UiButton type="primary" :disabled="!hasSelection" @click="emit('disable')">
        {{ t('streams.disable') }}
      </UiButton>
      <UiButton type="primary" :disabled="!hasSelection" @click="emit('test')">
        {{ t('streams.test') }}
      </UiButton>
    </div>
    <div class="streams-toolbar__search">
      <UiInput
        :model-value="search"
        :placeholder="t('streams.search')"
        @update:model-value="emit('update:search', $event)"
      />
    </div>
  </div>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'
import UiButton from '@/shared/ui/Button.vue'
import UiInput from '@/shared/ui/Input.vue'

defineProps<{
  search: string
  hasSelection: boolean
}>()

const emit = defineEmits<{
  add: []
  remove: []
  enable: []
  disable: []
  test: []
  'update:search': [value: string]
}>()

const { t } = useI18n()
</script>

<style scoped>
.streams-toolbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  padding: 12px 16px 8px;
}

.streams-toolbar__actions {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.streams-toolbar__search {
  width: 220px;
  flex-shrink: 0;
}
</style>
