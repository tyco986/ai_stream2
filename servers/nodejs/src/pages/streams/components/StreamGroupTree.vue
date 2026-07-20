<template>
  <div class="group-tree">
    <StreamGroupTreeNode
      v-if="root"
      :node="root"
      :depth="0"
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
  </div>
</template>

<script setup lang="ts">
import type { TreeGroupNode } from '@/api/streams'
import StreamGroupTreeNode, { type GroupDraft } from './StreamGroupTreeNode.vue'

defineProps<{
  root: TreeGroupNode | null
  selectedGroupId: string | null
  selectedStreamId: string | null
  expandedIds: Set<string>
  allGroupId: string
  draft: GroupDraft
  draftName: string
}>()

const emit = defineEmits<{
  toggle: [id: string]
  'select-group': [payload: { id: string; name: string }]
  'select-stream': [payload: { id: string; name: string }]
  menu: [payload: { command: string; groupId: string; name: string }]
  'update:draft-name': [value: string]
  'confirm-draft': []
  'cancel-draft': []
}>()
</script>

<style scoped>
.group-tree {
  font-size: 13px;
  overflow: auto;
  flex: 1;
  min-height: 0;
}
</style>
