<template>
  <div class="group-tree">
    <GroupTreeNode
      v-if="visibleRoot"
      :node="visibleRoot"
      :depth="0"
      :bound-ids="boundIds"
      :selected-group-id="selectedGroupId"
      :expanded-ids="expandedIds"
      @toggle="emit('toggle', $event)"
      @select-group="emit('select-group', $event)"
      @select-stream="emit('select-stream', $event)"
    />
    <div v-else class="group-tree__empty">
      {{ t('preview.emptyTree') }}
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { TreeGroupNode, TreeNode } from '@/api/streams'
import GroupTreeNode from './GroupTreeNode.vue'

const props = defineProps<{
  root: TreeGroupNode
  searchQuery: string
  filterBoundOnly: boolean
  boundIds: Set<string>
  selectedGroupId: string | null
  expandedIds: Set<string>
}>()

const emit = defineEmits<{
  toggle: [id: string]
  'select-group': [id: string]
  'select-stream': [id: string]
}>()

const { t } = useI18n()

function nodeMatches(node: TreeNode, query: string, boundOnly: boolean): boolean {
  if (node.type === 'stream') {
    if (boundOnly && !props.boundIds.has(node.id)) {
      return false
    }
    if (!query) {
      return true
    }
    return node.name.toLowerCase().includes(query)
  }
  return filterChildren(node.children, query, boundOnly).length > 0
}

function filterChildren(
  children: TreeNode[],
  query: string,
  boundOnly: boolean,
): TreeNode[] {
  return children
    .map((child) => {
      if (child.type === 'stream') {
        return nodeMatches(child, query, boundOnly) ? child : null
      }
      const nextChildren = filterChildren(child.children, query, boundOnly)
      if (!query && !boundOnly) {
        return child
      }
      if (nextChildren.length === 0 && !child.name.toLowerCase().includes(query)) {
        return null
      }
      if (!query || child.name.toLowerCase().includes(query) || nextChildren.length > 0) {
        return { ...child, children: nextChildren }
      }
      return null
    })
    .filter((child): child is TreeNode => child !== null)
}

const visibleRoot = computed(() => {
  const query = props.searchQuery.trim().toLowerCase()
  if (!query && !props.filterBoundOnly) {
    return props.root
  }
  const children = filterChildren(props.root.children, query, props.filterBoundOnly)
  if (
    children.length === 0 &&
    query &&
    !props.root.name.toLowerCase().includes(query)
  ) {
    return null
  }
  return { ...props.root, children }
})
</script>

<script lang="ts">
export default {
  name: 'GroupTree',
}
</script>

<style scoped>
.group-tree {
  font-size: 13px;
  overflow: auto;
  flex: 1;
  min-height: 0;
}

.group-tree__empty {
  padding: 12px;
  color: #909399;
}
</style>
