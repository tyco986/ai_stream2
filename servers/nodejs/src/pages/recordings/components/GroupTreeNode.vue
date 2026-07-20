<template>
  <div class="tree-node">
    <div
      class="tree-node__row"
      :class="{
        'is-group-selected': node.type === 'group' && node.id === selectedGroupId,
      }"
      :style="{ paddingLeft: `${12 + depth * 14}px` }"
      @click="onRowClick"
    >
      <button
        v-if="node.type === 'group'"
        type="button"
        class="tree-node__twist"
        @click.stop="emit('toggle', node.id)"
      >
        {{ expandedIds.has(node.id) ? 'v' : '>' }}
      </button>
      <span v-else class="tree-node__twist-spacer" />
      <span class="tree-node__label" :class="labelClass">
        {{ node.name }}{{ boundMark }}
      </span>
    </div>
    <template v-if="node.type === 'group' && expandedIds.has(node.id)">
      <GroupTreeNode
        v-for="child in node.children"
        :key="child.id"
        :node="child"
        :depth="depth + 1"
        :bound-ids="boundIds"
        :selected-group-id="selectedGroupId"
        :expanded-ids="expandedIds"
        @toggle="emit('toggle', $event)"
        @select-group="emit('select-group', $event)"
        @select-stream="emit('select-stream', $event)"
      />
    </template>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type { TreeNode } from '@/api/streams'

const props = defineProps<{
  node: TreeNode
  depth: number
  boundIds: Set<string>
  selectedGroupId: string | null
  expandedIds: Set<string>
}>()

const emit = defineEmits<{
  toggle: [id: string]
  'select-group': [id: string]
  'select-stream': [id: string]
}>()

const boundMark = computed(() => {
  if (props.node.type === 'stream' && props.boundIds.has(props.node.id)) {
    return ' *'
  }
  return ''
})

const labelClass = computed(() => {
  if (props.node.type !== 'stream') {
    return ''
  }
  if (!props.node.recording) {
    return 'is-no-recording'
  }
  return 'is-stream'
})

function onRowClick() {
  if (props.node.type === 'group') {
    emit('select-group', props.node.id)
    return
  }
  emit('select-stream', props.node.id)
}
</script>

<script lang="ts">
export default {
  name: 'GroupTreeNode',
}
</script>

<style scoped>
.tree-node__row {
  display: flex;
  align-items: center;
  gap: 4px;
  min-height: 28px;
  cursor: pointer;
}

.tree-node__row:hover {
  background: #f5f7fa;
}

.tree-node__row.is-group-selected {
  background: #ecf5ff;
}

.tree-node__twist,
.tree-node__twist-spacer {
  width: 16px;
  flex-shrink: 0;
  border: none;
  background: transparent;
  cursor: pointer;
  color: #909399;
  padding: 0;
  text-align: center;
}

.tree-node__label {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.tree-node__label.is-stream {
  color: #303133;
}

.tree-node__label.is-no-recording {
  color: #c0c4cc;
}
</style>
