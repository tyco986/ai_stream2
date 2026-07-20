<template>
  <div v-if="pipelines.length" class="pipeline-refs">
    <div v-for="item in pipelines" :key="item.id" class="pipeline-refs__line">
      {{ item.name }}
    </div>
  </div>
  <span v-else>—</span>
</template>

<script setup lang="ts">
import type { PipelineRef } from '@/api/pipelines'

withDefaults(
  defineProps<{
    pipelines?: PipelineRef[]
  }>(),
  {
    pipelines: () => [],
  },
)
</script>

<style scoped>
.pipeline-refs {
  display: flex;
  flex-direction: column;
  gap: 2px;
  line-height: 1.4;
  white-space: normal;
}

.pipeline-refs__line {
  word-break: break-word;
}
</style>
