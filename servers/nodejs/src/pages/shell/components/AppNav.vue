<template>
  <nav :class="['app-nav', `app-nav--${layout}`]">
    <RouterLink
      v-for="item in NAV_ITEMS"
      :key="item.path"
      :to="item.path"
      class="app-nav__link"
      active-class="app-nav__link--active"
    >
      {{ item.label }}
    </RouterLink>
  </nav>
</template>

<script setup lang="ts">
import { RouterLink } from 'vue-router'
import { NAV_ITEMS } from '../nav'

defineProps<{
  layout: 'sidebar' | 'topbar'
}>()
</script>

<style scoped>
.app-nav--sidebar {
  display: flex;
  flex-direction: column;
  gap: 4px;
  padding: 8px;
  flex: 1;
  overflow-y: auto;
}

.app-nav--topbar {
  display: flex;
  flex-direction: row;
  flex-wrap: nowrap;
  gap: 4px;
  align-items: center;
  overflow-x: auto;
  flex: 1;
  min-width: 0;
}

.app-nav__link {
  color: #303133;
  text-decoration: none;
  padding: 8px 12px;
  border-radius: 4px;
  white-space: nowrap;
  font-size: 14px;
}

.app-nav__link:hover {
  background: #f2f3f5;
}

.app-nav__link--active {
  background: #ecf5ff;
  color: #409eff;
  font-weight: 600;
}
</style>
