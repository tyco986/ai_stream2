<template>
  <el-header style="background: #fff; display: flex; align-items: center; justify-content: space-between; padding: 0 20px; border-bottom: 1px solid #e6e6e6; height: 60px">
    <div style="display: flex; align-items: center">
      <el-icon :size="20" style="cursor: pointer" @click="$emit('toggle-sidebar')">
        <Fold v-if="!collapsed" />
        <Expand v-else />
      </el-icon>
    </div>
    <div style="display: flex; align-items: center; gap: 20px">
      <el-badge :value="mockAlertCount" :max="99">
        <el-icon :size="20" style="cursor: pointer"><Bell /></el-icon>
      </el-badge>
      <el-dropdown>
        <span style="display: flex; align-items: center; cursor: pointer; gap: 8px">
          <el-avatar :size="32" style="background: #409eff">A</el-avatar>
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
import { ref } from 'vue'

defineProps<{ collapsed: boolean }>()
defineEmits<{ 'toggle-sidebar': [] }>()

const mockAlertCount = ref(5)
</script>
