<template>
  <div class="tree-node">
    <InlineGroupEdit
      v-if="isRenaming"
      :model-value="draftName"
      :depth="depth"
      @update:model-value="emit('update:draft-name', $event)"
      @confirm="emit('confirm-draft')"
      @cancel="emit('cancel-draft')"
    />
    <div
      v-else
      class="tree-node__row"
      :class="{
        'is-selected': isSelected,
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
        {{ node.name }}
      </span>
      <GroupMenu
        v-if="node.type === 'group'"
        class="tree-node__menu"
        :can-delete="node.id !== allGroupId"
        :can-rename="node.id !== allGroupId"
        @command="onMenuCommand"
      />
    </div>

    <template v-if="node.type === 'group' && expandedIds.has(node.id)">
      <InlineGroupEdit
        v-if="isAddingChild"
        :model-value="draftName"
        :depth="depth + 1"
        @update:model-value="emit('update:draft-name', $event)"
        @confirm="emit('confirm-draft')"
        @cancel="emit('cancel-draft')"
      />
      <StreamGroupTreeNode
        v-for="child in node.children"
        :key="child.id"
        :node="child"
        :depth="depth + 1"
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
    </template>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type { TreeNode } from '@/api/streams'
import GroupMenu from './GroupMenu.vue'
import InlineGroupEdit from './InlineGroupEdit.vue'

export type GroupDraft =
  | { mode: 'add'; parentId: string }
  | { mode: 'rename'; groupId: string }
  | null

const props = defineProps<{
  node: TreeNode
  depth: number
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

const isRenaming = computed(
  () =>
    props.node.type === 'group' &&
    props.draft?.mode === 'rename' &&
    props.draft.groupId === props.node.id,
)

const isAddingChild = computed(
  () =>
    props.node.type === 'group' &&
    props.draft?.mode === 'add' &&
    props.draft.parentId === props.node.id,
)

const isSelected = computed(() => {
  if (props.node.type === 'group') {
    return props.node.id === props.selectedGroupId
  }
  return props.node.id === props.selectedStreamId
})

const labelClass = computed(() => {
  if (props.node.type !== 'stream') {
    return ''
  }
  if (!props.node.enabled) {
    return 'is-disabled'
  }
  return 'is-stream'
})

function onRowClick() {
  if (props.node.type === 'group') {
    emit('select-group', { id: props.node.id, name: props.node.name })
    return
  }
  emit('select-stream', { id: props.node.id, name: props.node.name })
}

function onMenuCommand(command: string) {
  if (props.node.type !== 'group') {
    return
  }
  emit('menu', {
    command,
    groupId: props.node.id,
    name: props.node.name,
  })
}
</script>

<script lang="ts">
export default {
  name: 'StreamGroupTreeNode',
}
</script>

<style scoped>
.tree-node__row {
  display: flex;
  align-items: center;
  gap: 4px;
  min-height: 28px;
  cursor: pointer;
  padding-right: 4px;
}

.tree-node__row:hover {
  background: #f5f7fa;
}

.tree-node__row.is-selected {
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
  flex: 1;
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.tree-node__label.is-stream {
  color: #303133;
}

.tree-node__label.is-disabled {
  color: #c0c4cc;
}

.tree-node__menu {
  margin-left: auto;
  flex-shrink: 0;
}
</style>
