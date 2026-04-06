<template>
  <div style="height: 100%; display: flex; align-items: center; justify-content: center; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%)">
    <el-card style="width: 400px; border-radius: 12px" shadow="always">
      <div style="text-align: center; margin-bottom: 24px">
        <h1 style="font-size: 24px; color: #303133; margin: 0">AI Stream</h1>
        <p style="color: #909399; margin-top: 4px">智能视频分析平台</p>
      </div>
      <el-form :model="form" @submit.prevent="handleLogin" label-position="top">
        <el-form-item label="用户名">
          <el-input v-model="form.username" placeholder="请输入用户名" prefix-icon="User" />
        </el-form-item>
        <el-form-item label="密码">
          <el-input v-model="form.password" type="password" placeholder="请输入密码" prefix-icon="Lock" show-password />
        </el-form-item>
        <el-form-item>
          <el-button type="primary" native-type="submit" :loading="loading" style="width: 100%">登录</el-button>
        </el-form-item>
      </el-form>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { useRouter } from 'vue-router'

const router = useRouter()
const loading = ref(false)
const form = ref({ username: '', password: '' })

async function handleLogin() {
  loading.value = true
  /* @API:AUTH_LOGIN — POST /api/v1/auth/login/ */
  await new Promise(r => setTimeout(r, 500))
  loading.value = false
  router.push('/dashboard')
}
</script>
