<template>
  <el-container style="height: 100%">
    <Sidebar :collapsed="isCollapsed" />
    <el-container>
      <Navbar :collapsed="isCollapsed" @toggle-sidebar="isCollapsed = !isCollapsed" />
      <el-main style="background: #f5f7fa; padding: 20px; overflow-y: auto">
        <router-view />
      </el-main>
    </el-container>
  </el-container>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import Sidebar from './Sidebar.vue'
import Navbar from './Navbar.vue'

const isCollapsed = ref(false)
</script>
