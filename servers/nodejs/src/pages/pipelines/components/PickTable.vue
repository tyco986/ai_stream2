<template>
  <div class="pick-table">
    <UiTable :data="data" :row-key="rowKey">
      <slot />
    </UiTable>
  </div>
</template>

<script setup lang="ts" generic="T extends object">
import UiTable from '@/shared/ui/Table.vue'

defineProps<{
  data: T[]
  rowKey?: string
}>()
</script>

<style scoped>
.pick-table {
  width: 100%;
}

.pick-table :deep(.el-table) {
  width: 100%;
}

.pick-table :deep(.el-table__header th),
.pick-table :deep(.el-table__body td) {
  text-align: left;
}

.pick-table :deep(.el-table .cell) {
  padding-left: 12px;
  padding-right: 12px;
}
</style>
