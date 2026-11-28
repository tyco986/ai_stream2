<script setup lang="ts">
defineOptions({
  name: "BarButton"
});

defineProps<{
  loading?: boolean;
  disabled?: boolean;
}>();
</script>

<template>
  <span class="bar-button">
    <el-button type="primary" :disabled="disabled || loading">
      <slot />
    </el-button>
    <span v-if="loading" class="bar" />
  </span>
</template>

<style scoped>
.bar-button {
  position: relative;
  display: inline-block;
  overflow: hidden;
  border-radius: var(--el-border-radius-base);
}

.bar {
  position: absolute;
  bottom: 0;
  left: 0;
  z-index: 1;
  width: 40%;
  height: 3px;
  background: #fff;
  animation: bar-slide 1s linear infinite;
}

@keyframes bar-slide {
  from {
    transform: translateX(-100%);
  }
  to {
    transform: translateX(350%);
  }
}
</style>
