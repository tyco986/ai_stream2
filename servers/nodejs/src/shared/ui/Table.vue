<template>
  <el-table
    ref="tableRef"
    :data="data"
    :row-key="rowKey"
    :highlight-current-row="highlightCurrentRow"
    :current-row-key="currentRowKey"
    style="width: 100%"
    @selection-change="onSelectionChange"
    @row-click="onRowClick"
  >
    <slot />
  </el-table>
</template>

<script setup lang="ts" generic="T extends object">
import { ref } from 'vue'

defineProps<{
  data: T[]
  rowKey?: string
  highlightCurrentRow?: boolean
  currentRowKey?: string
}>()

const emit = defineEmits<{
  'selection-change': [rows: T[]]
  'row-click': [row: T]
}>()

const tableRef = ref<{
  toggleRowSelection: (row: T, selected?: boolean) => void
  clearSelection: () => void
} | null>(null)

function onSelectionChange(rows: T[]) {
  emit('selection-change', rows)
}

function onRowClick(row: T) {
  emit('row-click', row)
}

function toggleRowSelection(row: T, selected?: boolean) {
  tableRef.value?.toggleRowSelection(row, selected)
}

function clearSelection() {
  tableRef.value?.clearSelection()
}

defineExpose({
  toggleRowSelection,
  clearSelection,
})
</script>
