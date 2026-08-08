<template>
  <div class="pipelines-toolbar">
    <div class="pipelines-toolbar__actions">
      <UiButton type="primary" @click="emit('add')">{{ t('pipelines.add') }}</UiButton>
      <UiButton type="danger" :disabled="!hasSelection" @click="emit('remove')">
        {{ t('pipelines.remove') }}
      </UiButton>
      <UiButton
        type="primary"
        :disabled="!hasSelection"
        @click="emit('refresh')"
      >
        {{ t('pipelines.refresh') }}
      </UiButton>
      <span class="pipelines-toolbar__divider" />
      <UiButton type="primary" @click="emit('open-gie')">{{ t('pipelines.gie') }}</UiButton>
      <UiButton type="primary" @click="emit('open-analyzer')">{{ t('pipelines.analyzer') }}</UiButton>
    </div>
    <div class="pipelines-toolbar__search">
      <UiInput
        :model-value="search"
        :placeholder="t('pipelines.search')"
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
  refresh: []
  'open-gie': []
  'open-analyzer': []
  'update:search': [value: string]
}>()

const { t } = useI18n()
</script>

<style scoped>
.pipelines-toolbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  padding: 12px 16px 8px;
}

.pipelines-toolbar__actions {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 8px;
}

.pipelines-toolbar__divider {
  width: 1px;
  height: 20px;
  margin: 0 4px;
  background: #dcdfe6;
}

.pipelines-toolbar__search {
  width: 220px;
  flex-shrink: 0;
}
</style>
