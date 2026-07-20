<template>
  <span class="gie-class-text">
    <span class="gie-class-text__id">{{ classId }}</span>
    <span class="gie-class-text__rest"> - {{ classLabel }}</span>
  </span>
</template>

<script setup lang="ts">
defineProps<{
  classId: string
  classLabel: string
}>()
</script>

<style scoped>
.gie-class-text {
  display: inline-flex;
  align-items: baseline;
  max-width: 100%;
  overflow: hidden;
}

.gie-class-text__id {
  width: 2ch;
  flex-shrink: 0;
  text-align: center;
  font-variant-numeric: tabular-nums;
}

.gie-class-text__rest {
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
</style>
