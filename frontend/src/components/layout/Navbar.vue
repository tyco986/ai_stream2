<template>
  <el-header class="top-nav">
    <div class="top-nav__left">
      <el-icon :size="22" color="#fff"><Monitor /></el-icon>
      <span class="top-nav__logo-text">AI Stream</span>
    </div>
    <el-menu
      :default-active="activeGroup"
      mode="horizontal"
      background-color="#001529"
      text-color="#ffffffa6"
      active-text-color="#fff"
      class="top-nav__menu"
      @select="handleGroupSelect"
    >
      <el-menu-item v-for="group in navGroups" :key="group.key" :index="group.key">
        <el-icon><component :is="group.icon" /></el-icon>
        <span>{{ group.label }}</span>
      </el-menu-item>
    </el-menu>
    <div class="top-nav__right">
      <el-badge :value="mockAlertCount" :max="99">
        <el-icon :size="18" color="#fff" style="cursor: pointer"><Bell /></el-icon>
      </el-badge>
      <el-dropdown>
        <span class="top-nav__user">
          <el-avatar :size="28" style="background: #409eff">A</el-avatar>
          <span>admin</span>
          <el-icon><ArrowDown /></el-icon>
        </span>
        <template #dropdown>
          <el-dropdown-menu>
            <el-dropdown-item>个人设置</el-dropdown-item>
            <el-dropdown-item divided>退出登录</el-dropdown-item>
          </el-dropdown-menu>
        </template>
      </el-dropdown>
    </div>
  </el-header>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { navGroups } from '@/router/nav-config'

const route = useRoute()
const router = useRouter()
const mockAlertCount = ref(5)

const activeGroup = computed(() => (route.meta?.group as string) || 'monitor')

function handleGroupSelect(key: string) {
  const group = navGroups.find(g => g.key === key)
  if (group) {
    router.push(group.defaultPath)
  }
}
</script>

<style scoped>
.top-nav {
  display: flex;
  align-items: center;
  background: #001529;
  padding: 0 20px;
  height: 56px;
  border-bottom: none;
}

.top-nav__left {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-right: 32px;
  flex-shrink: 0;
}

.top-nav__logo-text {
  color: #fff;
  font-size: 17px;
  font-weight: 700;
  white-space: nowrap;
}

.top-nav__menu {
  flex: 1;
  border-bottom: none !important;
  height: 56px;
}

.top-nav__menu .el-menu-item {
  height: 56px;
  line-height: 56px;
  border-bottom: 2px solid transparent;
}

.top-nav__menu .el-menu-item.is-active {
  border-bottom-color: #409eff;
  background-color: rgba(255, 255, 255, 0.08) !important;
}

.top-nav__right {
  display: flex;
  align-items: center;
  gap: 20px;
  margin-left: 24px;
  flex-shrink: 0;
}

.top-nav__user {
  display: flex;
  align-items: center;
  gap: 8px;
  cursor: pointer;
  color: #ffffffd9;
}
</style>
