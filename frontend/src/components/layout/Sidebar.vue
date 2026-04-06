<template>
  <el-aside :width="collapsed ? '64px' : '220px'" style="transition: width 0.3s; background: #001529">
    <div style="height: 60px; display: flex; align-items: center; justify-content: center; color: #fff; font-size: 18px; font-weight: bold; white-space: nowrap; overflow: hidden">
      <el-icon :size="24" style="margin-right: 8px"><Monitor /></el-icon>
      <span v-show="!collapsed">AI Stream</span>
    </div>
    <el-menu
      :default-active="activeMenu"
      :collapse="collapsed"
      background-color="#001529"
      text-color="#ffffffa6"
      active-text-color="#409eff"
      router
      :collapse-transition="false"
    >
      <el-menu-item index="/dashboard">
        <el-icon><DataAnalysis /></el-icon>
        <template #title>仪表盘</template>
      </el-menu-item>

      <el-sub-menu index="cameras-group">
        <template #title>
          <el-icon><VideoCamera /></el-icon>
          <span>摄像头</span>
        </template>
        <el-menu-item index="/cameras">摄像头列表</el-menu-item>
        <el-menu-item index="/cameras/preview">实时预览</el-menu-item>
      </el-sub-menu>

      <el-sub-menu index="pipelines-group">
        <template #title>
          <el-icon><Cpu /></el-icon>
          <span>AI 管道</span>
        </template>
        <el-menu-item index="/pipelines/models">模型管理</el-menu-item>
        <el-menu-item index="/pipelines/profiles">管道配置</el-menu-item>
      </el-sub-menu>

      <el-menu-item index="/detections">
        <el-icon><Search /></el-icon>
        <template #title>检测记录</template>
      </el-menu-item>

      <el-menu-item index="/recordings">
        <el-icon><Film /></el-icon>
        <template #title>录像回放</template>
      </el-menu-item>

      <el-menu-item index="/screenshots">
        <el-icon><Camera /></el-icon>
        <template #title>截图管理</template>
      </el-menu-item>

      <el-menu-item index="/alert-rules">
        <el-icon><Setting /></el-icon>
        <template #title>报警规则</template>
      </el-menu-item>

      <el-menu-item index="/alerts">
        <el-icon><Bell /></el-icon>
        <template #title>报警记录</template>
      </el-menu-item>

      <el-menu-item index="/system/users">
        <el-icon><UserFilled /></el-icon>
        <template #title>系统管理</template>
      </el-menu-item>
    </el-menu>
  </el-aside>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useRoute } from 'vue-router'

defineProps<{ collapsed: boolean }>()

const route = useRoute()
const activeMenu = computed(() => route.path)
</script>
