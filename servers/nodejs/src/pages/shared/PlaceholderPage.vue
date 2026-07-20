<template>
  <div class="placeholder">
    <h1 class="placeholder__title">{{ title }}</h1>
  </div>
</template>

<script setup lang="ts">
defineProps<{
  title: string
}>()
</script>

<style scoped>
.placeholder {
  padding: 24px;
}

.placeholder__title {
  margin: 0;
  font-size: 20px;
  font-weight: 600;
  color: #303133;
}
</style>
