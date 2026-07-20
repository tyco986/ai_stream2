<template>
  <el-pagination
    background
    layout="prev, pager, next"
    :current-page="page"
    :page-size="pageSize"
    :total="total"
    @current-change="emit('update:page', $event)"
  />
</template>

<script setup lang="ts">
defineProps<{
  page: number
  pageSize: number
  total: number
}>()

const emit = defineEmits<{
  'update:page': [page: number]
}>()
</script>
