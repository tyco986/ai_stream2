<template>
  <div>
    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px">
      <PageHeader title="实时预览" subtitle="4×4 多路摄像头实时监控" style="margin-bottom: 0" />
      <el-button @click="handleBack">返回总览</el-button>
    </div>

    <el-row :gutter="8">
      <el-col v-for="(cell, idx) in gridCells" :key="idx" :span="6" style="margin-bottom: 8px">
        <div
          :style="cellStyle"
          @click="handleCellClick(cell)"
        >
          <span style="color: #fff; font-size: 13px; text-shadow: 0 1px 3px rgba(0,0,0,0.6)">
            {{ cell.name }}
          </span>
          <el-tag
            v-if="cell.camera"
            type="success"
            size="small"
            effect="dark"
            style="position: absolute; top: 6px; left: 6px"
          >
            在线
          </el-tag>
        </div>
      </el-col>
    </el-row>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { ElMessage } from 'element-plus'
import { useRouter } from 'vue-router'
import PageHeader from '@/components/common/PageHeader.vue'
import type { Camera } from '@/types/camera'

const router = useRouter()

const GRID_SIZE = 16

/* @API:DS_PREVIEW_URL — GET /api/v1/deepstream/preview-url/ */

const mockCameras: Camera[] = [
  { id: 'c1', uid: 'cam_001', name: '大门入口', rtsp_url: 'rtsp://192.168.1.101/stream', organization: 'default', group: null, status: 'online', pipeline_profile: 'p1', config: {}, created_at: '2026-03-01T08:00:00Z', updated_at: '2026-03-20T08:00:00Z' },
  { id: 'c2', uid: 'cam_002', name: '停车场A', rtsp_url: 'rtsp://192.168.1.102/stream', organization: 'default', group: null, status: 'online', pipeline_profile: 'p1', config: {}, created_at: '2026-03-01T08:00:00Z', updated_at: '2026-03-20T08:00:00Z' },
  { id: 'c3', uid: 'cam_003', name: '仓库东侧', rtsp_url: 'rtsp://192.168.1.103/stream', organization: 'default', group: null, status: 'online', pipeline_profile: 'p2', config: {}, created_at: '2026-03-02T08:00:00Z', updated_at: '2026-03-20T08:00:00Z' },
  { id: 'c4', uid: 'cam_004', name: '办公楼大厅', rtsp_url: 'rtsp://192.168.1.104/stream', organization: 'default', group: null, status: 'online', pipeline_profile: 'p1', config: {}, created_at: '2026-03-03T08:00:00Z', updated_at: '2026-03-20T08:00:00Z' },
  { id: 'c5', uid: 'cam_005', name: '园区周界北', rtsp_url: 'rtsp://192.168.1.105/stream', organization: 'default', group: null, status: 'online', pipeline_profile: 'p1', config: {}, created_at: '2026-03-04T08:00:00Z', updated_at: '2026-03-20T08:00:00Z' },
  { id: 'c6', uid: 'cam_006', name: '生产车间', rtsp_url: 'rtsp://192.168.1.106/stream', organization: 'default', group: null, status: 'online', pipeline_profile: 'p2', config: {}, created_at: '2026-03-05T08:00:00Z', updated_at: '2026-03-20T08:00:00Z' },
  { id: 'c7', uid: 'cam_007', name: '食堂入口', rtsp_url: 'rtsp://192.168.1.107/stream', organization: 'default', group: null, status: 'online', pipeline_profile: 'p1', config: {}, created_at: '2026-03-06T08:00:00Z', updated_at: '2026-03-20T08:00:00Z' },
  { id: 'c8', uid: 'cam_008', name: '地下车库B1', rtsp_url: 'rtsp://192.168.1.108/stream', organization: 'default', group: null, status: 'online', pipeline_profile: 'p1', config: {}, created_at: '2026-03-07T08:00:00Z', updated_at: '2026-03-20T08:00:00Z' },
]

interface GridCell {
  name: string
  camera: Camera | null
}

const gridCells = computed<GridCell[]>(() => {
  const cells: GridCell[] = mockCameras.map((cam) => ({
    name: cam.name,
    camera: cam,
  }))
  while (cells.length < GRID_SIZE) {
    cells.push({ name: '空闲', camera: null })
  }
  return cells
})

const cellStyle = {
  position: 'relative' as const,
  aspectRatio: '16 / 9',
  background: 'linear-gradient(135deg, #1a1a2e 0%, #16213e 100%)',
  borderRadius: '6px',
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'center',
  cursor: 'pointer',
  border: '1px solid #2a2a3e',
  transition: 'border-color 0.2s, box-shadow 0.2s',
  overflow: 'hidden',
}

function handleCellClick(cell: GridCell) {
  if (!cell.camera) return
  /* @API:DS_SWITCH_PREVIEW — POST /api/v1/deepstream/switch-preview/ */
  ElMessage.info(`切换到单路预览: ${cell.camera.uid}`)
}

function handleBack() {
  router.back()
}
</script>

<style scoped>
div[style*="aspect-ratio"]:hover {
  border-color: #409eff !important;
  box-shadow: 0 0 8px rgba(64, 158, 255, 0.3);
}
</style>
