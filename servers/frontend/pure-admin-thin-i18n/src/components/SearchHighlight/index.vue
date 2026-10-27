<script setup lang="ts">
import { computed } from "vue";

defineOptions({
  name: "SearchHighlight"
});

const props = defineProps<{
  text: unknown;
  query: string;
}>();

const parts = computed(() => {
  const text = props.text === null || props.text === undefined ? "" : String(props.text);
  const query = props.query;
  const escaped = query.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
  const pieces = query ? text.split(new RegExp(`(${escaped})`, "ig")) : [text];
  return pieces.filter(part => part !== "").map(part => ({
    text: part,
    hit: query !== "" && part.toLowerCase() === query.toLowerCase()
  }));
});
</script>

<template>
  <span v-for="(part, index) in parts" :key="index" :class="{ hit: part.hit }">{{ part.text }}</span>
</template>

<style scoped>
.hit {
  background: var(--el-color-warning-light-7);
}
</style>
