<template>
  <button
    v-if="visible"
    type="button"
    class="logo-version"
    :class="`logo-version--${layout}`"
    @click="emit('open')"
  >
    <span class="logo-version__logo">{{ projectName }}</span>
    <span class="logo-version__ver">v{{ version }}</span>
  </button>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { PROJECT_NAME } from '@/shared/project'

const props = defineProps<{
  layout: 'sidebar' | 'topbar'
  version: string
  visible: boolean
}>()

const emit = defineEmits<{
  open: []
}>()

const projectName = PROJECT_NAME
const version = computed(() => props.version || '0')
</script>

<style scoped>
.logo-version {
  display: inline-flex;
  align-items: baseline;
  gap: 8px;
  border: none;
  background: transparent;
  cursor: pointer;
  color: #303133;
  font: inherit;
  padding: 16px 12px;
  text-align: left;
}

.logo-version:hover {
  background: #f5f7fa;
}

.logo-version--topbar {
  padding: 8px 12px;
  flex-shrink: 0;
  border-radius: 4px;
}

.logo-version__logo {
  font-weight: 700;
  font-size: 16px;
}

.logo-version__ver {
  font-size: 13px;
  color: #909399;
  font-weight: 600;
}
</style>
