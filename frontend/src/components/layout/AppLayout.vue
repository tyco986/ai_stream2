<template>
  <el-container style="height: 100%; flex-direction: column">
    <Navbar />
    <el-container style="flex: 1; overflow: hidden">
      <Sidebar />
      <el-main style="background: #f5f7fa; padding: 20px; overflow-y: auto">
        <router-view />
      </el-main>
    </el-container>
  </el-container>
</template>

<script setup lang="ts">
import Sidebar from './Sidebar.vue'
import Navbar from './Navbar.vue'
</script>
