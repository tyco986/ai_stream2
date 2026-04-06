<template>
  <div>
    <div style="display: flex; justify-content: space-between; align-items: flex-start; margin-bottom: 20px">
      <PageHeader title="用户管理" />
      <el-button type="primary" :icon="Plus" disabled>新建用户</el-button>
    </div>

    <el-card shadow="hover" style="border-radius: 8px">
      <el-table :data="users" stripe style="width: 100%">
        <el-table-column prop="username" label="用户名" min-width="120" />
        <el-table-column prop="email" label="邮箱" min-width="180" />
        <el-table-column label="角色" width="110">
          <template #default="{ row }">
            <el-tag :type="roleTagType[row.role]" size="small">
              {{ roleLabel[row.role] }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="organization" label="组织" min-width="130" />
        <el-table-column label="状态" width="90" align="center">
          <template #default="{ row }">
            <el-tag :type="row.is_active ? 'success' : 'info'" size="small">
              {{ row.is_active ? '启用' : '禁用' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="创建时间" width="170">
          <template #default="{ row }">
            {{ formatTime(row.created_at) }}
          </template>
        </el-table-column>
        <el-table-column label="操作" width="120" fixed="right">
          <template #default="{ row }">
            <el-button link type="primary" @click="openRoleDialog(row)">编辑角色</el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <!-- Role edit dialog -->
    <el-dialog v-model="dialogVisible" title="编辑角色" width="400px" destroy-on-close>
      <el-form :model="roleForm" label-width="80px">
        <el-form-item label="用户">
          <el-input :model-value="roleForm.username" disabled />
        </el-form-item>
        <el-form-item label="角色">
          <el-select v-model="roleForm.role" style="width: 100%">
            <el-option v-for="(label, key) in roleLabel" :key="key" :label="label" :value="key" />
          </el-select>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" @click="handleSaveRole">确定</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive } from 'vue'
import { Plus } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import PageHeader from '@/components/common/PageHeader.vue'

type UserRole = 'admin' | 'operator' | 'viewer'

interface User {
  id: string
  username: string
  email: string
  role: UserRole
  organization: string
  is_active: boolean
  created_at: string
}

const roleLabel: Record<UserRole, string> = {
  admin: '管理员',
  operator: '操作员',
  viewer: '观察者',
}

const roleTagType: Record<UserRole, 'danger' | 'warning' | ''> = {
  admin: 'danger',
  operator: 'warning',
  viewer: '',
}

/* @API:USER_LIST — GET /api/v1/users/ */
const users = ref<User[]>([
  {
    id: 'user-1',
    username: 'admin',
    email: 'admin@example.com',
    role: 'admin',
    organization: '总部',
    is_active: true,
    created_at: '2025-01-15T08:00:00Z',
  },
  {
    id: 'user-2',
    username: 'zhangsan',
    email: 'zhangsan@example.com',
    role: 'operator',
    organization: '分部A',
    is_active: true,
    created_at: '2025-02-20T10:00:00Z',
  },
  {
    id: 'user-3',
    username: 'lisi',
    email: 'lisi@example.com',
    role: 'viewer',
    organization: '分部A',
    is_active: true,
    created_at: '2025-03-10T14:00:00Z',
  },
  {
    id: 'user-4',
    username: 'wangwu',
    email: 'wangwu@example.com',
    role: 'operator',
    organization: '分部B',
    is_active: false,
    created_at: '2025-04-05T09:00:00Z',
  },
  {
    id: 'user-5',
    username: 'zhaoliu',
    email: 'zhaoliu@example.com',
    role: 'viewer',
    organization: '总部',
    is_active: true,
    created_at: '2025-05-18T16:00:00Z',
  },
])

const dialogVisible = ref(false)

const roleForm = reactive({
  userId: '',
  username: '',
  role: '' as UserRole,
})

function formatTime(iso: string): string {
  return new Date(iso).toLocaleString('zh-CN', {
    year: 'numeric',
    month: '2-digit',
    day: '2-digit',
    hour: '2-digit',
    minute: '2-digit',
  })
}

function openRoleDialog(row: User) {
  roleForm.userId = row.id
  roleForm.username = row.username
  roleForm.role = row.role
  dialogVisible.value = true
}

function handleSaveRole() {
  /* @API:USER_UPDATE — PATCH /api/v1/users/{id}/ */
  const user = users.value.find((u) => u.id === roleForm.userId)
  if (user) {
    user.role = roleForm.role
    ElMessage.success('角色已更新')
  }
  dialogVisible.value = false
}
</script>
