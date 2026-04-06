<template>
  <el-tag :type="tagType" size="small" effect="light">{{ label }}</el-tag>
</template>

<script setup lang="ts">
import { computed } from 'vue'

const props = defineProps<{ status: 'online' | 'offline' | 'connecting' | 'error' }>()

const tagType = computed(() => {
  const map: Record<string, string> = {
    online: 'success',
    offline: 'info',
    connecting: 'warning',
    error: 'danger',
  }
  return (map[props.status] || 'info') as any
})

const label = computed(() => {
  const map: Record<string, string> = {
    online: '在线',
    offline: '离线',
    connecting: '连接中',
    error: '错误',
  }
  return map[props.status] || props.status
})
</script>
