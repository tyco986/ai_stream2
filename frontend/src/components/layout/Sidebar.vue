<template>
  <el-aside v-if="menuItems.length > 0" width="200px" class="side-menu">
    <el-menu
      :default-active="activePath"
      router
      class="side-menu__nav"
    >
      <el-menu-item v-for="item in menuItems" :key="item.path" :index="item.path">
        <el-icon><component :is="item.icon" /></el-icon>
        <span>{{ item.label }}</span>
      </el-menu-item>
    </el-menu>
  </el-aside>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useRoute } from 'vue-router'
import { navGroups } from '@/router/nav-config'

const route = useRoute()

const activePath = computed(() => route.path)

const menuItems = computed(() => {
  const group = (route.meta?.group as string) || 'monitor'
  const found = navGroups.find(g => g.key === group)
  return found?.children ?? []
})
</script>

<style scoped>
.side-menu {
  background: #fff;
  border-right: 1px solid #f0f0f0;
  overflow-y: auto;
}

.side-menu__nav {
  border-right: none;
  padding-top: 8px;
}

.side-menu__nav .el-menu-item {
  height: 44px;
  line-height: 44px;
  margin: 2px 8px;
  border-radius: 6px;
}

.side-menu__nav .el-menu-item.is-active {
  background-color: #ecf5ff;
  color: #409eff;
}
</style>
