<template>
  <aside class="groups-side">
    <div class="groups-side__header">
      <span class="groups-side__title">{{ t('streams.groups') }}</span>
      <button
        type="button"
        class="groups-side__icon"
        :title="t('streams.refresh')"
        :disabled="treeLoading"
        @click="emit('refresh')"
      >
        <UiIcon name="refresh" :size="14" />
      </button>
    </div>
    <StreamGroupTree
      :root="root"
      :selected-group-id="selectedGroupId"
      :selected-stream-id="selectedStreamId"
      :expanded-ids="expandedIds"
      :all-group-id="allGroupId"
      :draft="draft"
      :draft-name="draftName"
      @toggle="emit('toggle', $event)"
      @select-group="emit('select-group', $event)"
      @select-stream="emit('select-stream', $event)"
      @menu="emit('menu', $event)"
      @update:draft-name="emit('update:draft-name', $event)"
      @confirm-draft="emit('confirm-draft')"
      @cancel-draft="emit('cancel-draft')"
    />
  </aside>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'
import type { TreeGroupNode } from '@/api/streams'
import UiIcon from '@/shared/ui/Icon.vue'
import StreamGroupTree from './StreamGroupTree.vue'
import type { GroupDraft } from './StreamGroupTreeNode.vue'

defineProps<{
  root: TreeGroupNode | null
  selectedGroupId: string | null
  selectedStreamId: string | null
  expandedIds: Set<string>
  allGroupId: string
  draft: GroupDraft
  draftName: string
  treeLoading: boolean
}>()

const emit = defineEmits<{
  refresh: []
  toggle: [id: string]
  'select-group': [payload: { id: string; name: string }]
  'select-stream': [payload: { id: string; name: string }]
  menu: [payload: { command: string; groupId: string; name: string }]
  'update:draft-name': [value: string]
  'confirm-draft': []
  'cancel-draft': []
}>()

const { t } = useI18n()
</script>

<style scoped>
.groups-side {
  width: 260px;
  flex-shrink: 0;
  display: flex;
  flex-direction: column;
  border-right: 1px solid #e4e7ed;
  background: #fff;
  min-height: 0;
}

.groups-side__header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 10px 12px 8px;
}

.groups-side__title {
  font-weight: 600;
  font-size: 14px;
}

.groups-side__icon {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 26px;
  height: 26px;
  border: none;
  border-radius: 4px;
  background: transparent;
  cursor: pointer;
  color: #606266;
}

.groups-side__icon:hover:not(:disabled) {
  background: #ecf5ff;
  color: #409eff;
}

.groups-side__icon:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}
</style>
