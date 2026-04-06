<template>
  <div style="margin-bottom: 20px">
    <h2 style="font-size: 20px; font-weight: 600; color: #303133; margin: 0">{{ title }}</h2>
    <p v-if="subtitle" style="font-size: 14px; color: #909399; margin-top: 4px">{{ subtitle }}</p>
  </div>
</template>

<script setup lang="ts">
defineProps<{ title: string; subtitle?: string }>()
</script>
