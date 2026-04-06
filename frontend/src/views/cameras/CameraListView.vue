<template>
  <div>
    <PageHeader title="摄像头列表" subtitle="管理所有摄像头及其流状态" />

    <el-alert
      title="部分摄像头分析配置已过期，请前往详情页重新同步管道配置"
      type="warning"
      show-icon
      :closable="false"
      style="margin-bottom: 20px"
    />

    <!-- Toolbar -->
    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 16px">
      <el-select v-model="statusFilter" placeholder="状态筛选" clearable style="width: 180px" @change="filterCameras">
        <el-option label="在线" value="online" />
        <el-option label="离线" value="offline" />
        <el-option label="连接中" value="connecting" />
        <el-option label="错误" value="error" />
      </el-select>
      <el-button type="primary" :icon="Plus" @click="handleCreate">新增摄像头</el-button>
    </div>

    <!-- Table -->
    <el-table :data="filteredCameras" stripe style="width: 100%">
      <el-table-column prop="name" label="名称" min-width="140" />
      <el-table-column prop="uid" label="UID" min-width="120" />
      <el-table-column prop="rtsp_url" label="RTSP地址" min-width="260" show-overflow-tooltip />
      <el-table-column label="状态" width="100">
        <template #default="{ row }">
          <StatusTag :status="row.status" />
        </template>
      </el-table-column>
      <el-table-column prop="created_at" label="创建时间" width="180" />
      <el-table-column label="操作" width="220" fixed="right">
        <template #default="{ row }">
          <el-button
            v-if="row.status === 'offline'"
            size="small"
            type="success"
            text
            @click="handleStart(row)"
          >启动</el-button>
          <el-button
            v-if="row.status === 'online' || row.status === 'connecting'"
            size="small"
            type="warning"
            text
            @click="handleStop(row)"
          >停止</el-button>
          <el-button size="small" type="primary" text @click="handleDetail(row)">详情</el-button>
        </template>
      </el-table-column>
    </el-table>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { useRouter } from 'vue-router'
import { Plus } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import type { Camera } from '@/types/camera'
import StatusTag from '@/components/common/StatusTag.vue'
import PageHeader from '@/components/common/PageHeader.vue'

const router = useRouter()
const statusFilter = ref('')

/* @API:CAM_LIST — GET /api/v1/cameras/ */
const cameras = ref<Camera[]>([
  { id: '1', uid: 'CAM-A001', name: '前门摄像头', rtsp_url: 'rtsp://192.168.1.10:554/stream1', organization: 'org-1', group: null, status: 'online', pipeline_profile: 'p1', config: {}, created_at: '2025-12-01 08:00:00', updated_at: '2025-12-01 08:00:00' },
  { id: '2', uid: 'CAM-A002', name: '后门摄像头', rtsp_url: 'rtsp://192.168.1.11:554/stream1', organization: 'org-1', group: null, status: 'online', pipeline_profile: 'p1', config: {}, created_at: '2025-12-01 09:30:00', updated_at: '2025-12-01 09:30:00' },
  { id: '3', uid: 'CAM-B001', name: '停车场入口', rtsp_url: 'rtsp://192.168.1.20:554/stream1', organization: 'org-1', group: 'parking', status: 'offline', pipeline_profile: null, config: {}, created_at: '2025-12-02 10:00:00', updated_at: '2025-12-02 10:00:00' },
  { id: '4', uid: 'CAM-B002', name: '停车场出口', rtsp_url: 'rtsp://192.168.1.21:554/stream1', organization: 'org-1', group: 'parking', status: 'connecting', pipeline_profile: 'p1', config: {}, created_at: '2025-12-02 10:15:00', updated_at: '2025-12-02 10:15:00' },
  { id: '5', uid: 'CAM-C001', name: '仓库内部', rtsp_url: 'rtsp://192.168.1.30:554/stream1', organization: 'org-1', group: 'warehouse', status: 'error', pipeline_profile: 'p2', config: {}, created_at: '2025-12-03 14:00:00', updated_at: '2025-12-03 14:00:00' },
  { id: '6', uid: 'CAM-C002', name: '大厅监控', rtsp_url: 'rtsp://192.168.1.31:554/stream1', organization: 'org-1', group: null, status: 'online', pipeline_profile: 'p1', config: {}, created_at: '2025-12-04 16:20:00', updated_at: '2025-12-04 16:20:00' },
])

const filteredCameras = computed(() => {
  if (!statusFilter.value) return cameras.value
  return cameras.value.filter(c => c.status === statusFilter.value)
})

function filterCameras() { /* triggered by el-select @change */ }

function handleStart(camera: Camera) {
  /* @API:CAM_START — POST /api/v1/cameras/{id}/start-stream/ */
  ElMessage.success(`正在启动 ${camera.name}`)
  camera.status = 'connecting'
}

function handleStop(camera: Camera) {
  /* @API:CAM_STOP — POST /api/v1/cameras/{id}/stop-stream/ */
  ElMessage.info(`正在停止 ${camera.name}`)
  camera.status = 'offline'
}

function handleCreate() {
  /* @API:CAM_CREATE — POST /api/v1/cameras/ */
  ElMessage.info('新增摄像头（待实现）')
}

function handleDetail(camera: Camera) {
  router.push(`/cameras/${camera.id}`)
}
</script>
