<template>
  <div>
    <PageHeader :title="camera.name" subtitle="摄像头详情与配置" />

    <!-- Stale config banner -->
    <el-alert
      v-if="pipeline.analytics_config_stale"
      title="分析配置已过期：区域配置与当前管道不同步，请重新同步"
      type="warning"
      show-icon
      :closable="false"
      style="margin-bottom: 20px"
    />

    <!-- Basic info -->
    <el-card shadow="hover" style="border-radius: 8px; margin-bottom: 20px">
      <template #header><span style="font-weight: 600">基本信息</span></template>
      <el-descriptions :column="2" border>
        <el-descriptions-item label="名称">{{ camera.name }}</el-descriptions-item>
        <el-descriptions-item label="状态">
          <StatusTag :status="camera.status" />
        </el-descriptions-item>
        <el-descriptions-item label="RTSP 地址">{{ camera.rtsp_url }}</el-descriptions-item>
        <el-descriptions-item label="UID">{{ camera.uid }}</el-descriptions-item>
        <el-descriptions-item label="创建时间">{{ camera.created_at }}</el-descriptions-item>
        <el-descriptions-item label="更新时间">{{ camera.updated_at }}</el-descriptions-item>
      </el-descriptions>
    </el-card>

    <!-- Preview -->
    <el-card shadow="hover" style="border-radius: 8px; margin-bottom: 20px">
      <template #header><span style="font-weight: 600">实时预览</span></template>
      <div
        style="width: 100%; max-width: 720px; aspect-ratio: 16/9; background: #1a1a2e; border-radius: 8px; display: flex; align-items: center; justify-content: center; color: #909399; font-size: 14px"
      >
        实时预览（WebRTC WHEP）— 摄像头在线时自动播放
      </div>
    </el-card>

    <!-- Pipeline config -->
    <el-card shadow="hover" style="border-radius: 8px; margin-bottom: 20px">
      <template #header><span style="font-weight: 600">管道配置</span></template>
      <el-descriptions :column="1" border style="margin-bottom: 16px">
        <el-descriptions-item label="当前管道">{{ pipeline.name }}</el-descriptions-item>
        <el-descriptions-item label="检测器">{{ pipeline.detector.name }} ({{ pipeline.detector.framework }})</el-descriptions-item>
        <el-descriptions-item label="跟踪器">{{ pipeline.tracker ? pipeline.tracker.name : '—' }}</el-descriptions-item>
        <el-descriptions-item label="分析功能">
          <el-tag v-if="pipeline.analytics_enabled" type="success" size="small">已启用</el-tag>
          <el-tag v-else type="info" size="small">未启用</el-tag>
        </el-descriptions-item>
      </el-descriptions>
      <el-select v-model="selectedPipelineId" placeholder="切换管道配置" style="width: 260px" @change="handlePipelineChange">
        <el-option v-for="p in pipelineOptions" :key="p.id" :label="p.name" :value="p.id" />
      </el-select>
    </el-card>

    <!-- Analytics zones -->
    <el-card shadow="hover" style="border-radius: 8px; margin-bottom: 20px">
      <template #header>
        <div style="display: flex; justify-content: space-between; align-items: center">
          <span style="font-weight: 600">分析区域</span>
          <el-button type="primary" size="small" :icon="Plus" @click="handleAddZone">添加区域</el-button>
        </div>
      </template>
      <el-row :gutter="20">
        <el-col :span="14">
          <div
            style="width: 100%; aspect-ratio: 16/9; background: #1a1a2e; border-radius: 8px; display: flex; align-items: center; justify-content: center; color: #909399; font-size: 14px"
          >
            区域绘制画布（Canvas）
          </div>
        </el-col>
        <el-col :span="10">
          <el-table :data="zones" size="small" style="width: 100%">
            <el-table-column prop="name" label="区域名称" />
            <el-table-column prop="zone_type" label="类型" width="120">
              <template #default="{ row }">
                <el-tag size="small">{{ zoneTypeLabel(row.zone_type) }}</el-tag>
              </template>
            </el-table-column>
            <el-table-column label="状态" width="80">
              <template #default="{ row }">
                <el-tag :type="row.is_enabled ? 'success' : 'info'" size="small">
                  {{ row.is_enabled ? '启用' : '禁用' }}
                </el-tag>
              </template>
            </el-table-column>
          </el-table>
        </el-col>
      </el-row>
    </el-card>

    <!-- Recording & Screenshot -->
    <el-card shadow="hover" style="border-radius: 8px; margin-bottom: 20px">
      <template #header><span style="font-weight: 600">录制与截图</span></template>
      <div style="display: flex; align-items: center; gap: 12px; margin-bottom: 16px">
        <el-button type="success" :icon="VideoPlay" @click="handleStartRecording">开始录制</el-button>
        <el-button type="danger" :icon="VideoPause" @click="handleStopRecording">停止录制</el-button>
        <el-button :icon="Camera" @click="handleScreenshot">截图</el-button>
        <el-tag v-if="recordingStatus === 'recording'" type="danger" effect="dark" style="margin-left: 8px">
          ● 录制中
        </el-tag>
        <el-tag v-else type="info" style="margin-left: 8px">未在录制</el-tag>
      </div>
    </el-card>

    <!-- Recent detections -->
    <el-card shadow="hover" style="border-radius: 8px">
      <template #header><span style="font-weight: 600">最近检测</span></template>
      <el-table :data="recentDetections" size="small" style="width: 100%">
        <el-table-column prop="object_type" label="目标类型" width="100" />
        <el-table-column prop="confidence" label="置信度" width="100">
          <template #default="{ row }">{{ (row.confidence * 100).toFixed(1) }}%</template>
        </el-table-column>
        <el-table-column prop="tracker_id" label="跟踪ID" width="100" />
        <el-table-column prop="frame_number" label="帧号" width="100" />
        <el-table-column prop="detected_at" label="检测时间" min-width="180" />
      </el-table>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { useRoute } from 'vue-router'
import { Plus, VideoPlay, VideoPause, Camera } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import type { Camera as CameraType } from '@/types/camera'
import type { PipelineProfile } from '@/types/pipeline'
import type { AnalyticsZone, ZoneType } from '@/types/analytics'
import type { Detection } from '@/types/detection'
import StatusTag from '@/components/common/StatusTag.vue'
import PageHeader from '@/components/common/PageHeader.vue'

const route = useRoute()
const cameraId = route.params.id

/* @API:CAM_DETAIL — GET /api/v1/cameras/{id}/ */
const camera = ref<CameraType>({
  id: cameraId as string,
  uid: 'CAM-A001',
  name: '前门摄像头',
  rtsp_url: 'rtsp://192.168.1.10:554/stream1',
  organization: 'org-1',
  group: null,
  status: 'online',
  pipeline_profile: 'p1',
  config: {},
  created_at: '2025-12-01 08:00:00',
  updated_at: '2025-12-10 14:30:00',
})

/* @API:CAM_PIPELINE_GET — GET /api/v1/cameras/{id}/pipeline/ */
const pipeline = ref<PipelineProfile>({
  id: 'p1',
  name: '默认检测管道',
  description: 'YOLOv8 + NvDCF + Analytics',
  detector: { id: 'm1', name: 'YOLOv8s', model_type: 'detector', framework: 'engine', model_file: '/models/yolov8s.engine', label_file: '/models/labels.txt', config: {}, version: '8.0', description: '', is_active: true, created_at: '', updated_at: '' },
  tracker: { id: 'm2', name: 'NvDCF', model_type: 'tracker', framework: 'custom', model_file: '', label_file: null, config: {}, version: '1.0', description: '', is_active: true, created_at: '', updated_at: '' },
  analytics_enabled: true,
  analytics_config_stale: true,
  is_active: true,
  created_at: '2025-11-15 10:00:00',
  updated_at: '2025-12-01 08:00:00',
})

const selectedPipelineId = ref(pipeline.value.id)
const pipelineOptions = ref([
  { id: 'p1', name: '默认检测管道' },
  { id: 'p2', name: '高精度管道' },
  { id: 'p3', name: '轻量级管道' },
])

function handlePipelineChange(id: string) {
  /* @API:CAM_PIPELINE_SET — PUT /api/v1/cameras/{id}/pipeline/ */
  ElMessage.success(`管道已切换为: ${pipelineOptions.value.find(p => p.id === id)?.name}`)
}

/* @API:ZONE_LIST — GET /api/v1/cameras/{id}/analytics-zones/ */
const zones = ref<AnalyticsZone[]>([
  { id: 'z1', camera: cameraId as string, name: '入口区域', zone_type: 'roi', coordinates: [[0.1, 0.1], [0.5, 0.1], [0.5, 0.9], [0.1, 0.9]], config: {}, is_enabled: true, created_at: '2025-12-01', updated_at: '2025-12-01' },
  { id: 'z2', camera: cameraId as string, name: '越线检测线', zone_type: 'line_crossing', coordinates: [[0.0, 0.5], [1.0, 0.5]], config: { direction: 'both' }, is_enabled: true, created_at: '2025-12-02', updated_at: '2025-12-02' },
  { id: 'z3', camera: cameraId as string, name: '拥挤监测区', zone_type: 'overcrowding', coordinates: [[0.2, 0.2], [0.8, 0.2], [0.8, 0.8], [0.2, 0.8]], config: { max_count: 10 }, is_enabled: false, created_at: '2025-12-03', updated_at: '2025-12-03' },
])

const ZONE_TYPE_LABELS: Record<ZoneType, string> = {
  roi: 'ROI 区域',
  line_crossing: '越线检测',
  overcrowding: '拥挤监测',
  direction: '方向检测',
}

function zoneTypeLabel(type: ZoneType): string {
  return ZONE_TYPE_LABELS[type] || type
}

function handleAddZone() {
  /* @API:ZONE_CREATE — POST /api/v1/cameras/{id}/analytics-zones/ */
  ElMessage.info('添加区域（待实现）')
}

const recordingStatus = ref<'idle' | 'recording'>('idle')

function handleStartRecording() {
  /* @API:DS_START_REC — POST /api/v1/deepstream/start-recording/ */
  recordingStatus.value = 'recording'
  ElMessage.success('录制已开始')
}

function handleStopRecording() {
  /* @API:DS_STOP_REC — POST /api/v1/deepstream/stop-recording/ */
  recordingStatus.value = 'idle'
  ElMessage.info('录制已停止')
}

function handleScreenshot() {
  /* @API:DS_SCREENSHOT — POST /api/v1/deepstream/screenshot/ */
  ElMessage.success('截图已保存')
}

/* @API:DET_RECENT — GET /api/v1/detections/?camera_id={id} */
const recentDetections = ref<Detection[]>([
  { id: 'd1', camera: cameraId as string, camera_name: '前门摄像头', object_type: 'person', confidence: 0.952, bbox: { x: 120, y: 80, w: 60, h: 150 }, tracker_id: 1, analytics_data: null, frame_number: 14200, detected_at: '2025-12-10 14:30:12' },
  { id: 'd2', camera: cameraId as string, camera_name: '前门摄像头', object_type: 'person', confidence: 0.891, bbox: { x: 300, y: 100, w: 55, h: 140 }, tracker_id: 2, analytics_data: null, frame_number: 14201, detected_at: '2025-12-10 14:30:13' },
  { id: 'd3', camera: cameraId as string, camera_name: '前门摄像头', object_type: 'car', confidence: 0.873, bbox: { x: 400, y: 200, w: 120, h: 80 }, tracker_id: 5, analytics_data: null, frame_number: 14210, detected_at: '2025-12-10 14:30:15' },
  { id: 'd4', camera: cameraId as string, camera_name: '前门摄像头', object_type: 'person', confidence: 0.814, bbox: { x: 50, y: 90, w: 58, h: 145 }, tracker_id: 3, analytics_data: null, frame_number: 14220, detected_at: '2025-12-10 14:30:18' },
  { id: 'd5', camera: cameraId as string, camera_name: '前门摄像头', object_type: 'bicycle', confidence: 0.762, bbox: { x: 500, y: 250, w: 70, h: 60 }, tracker_id: 7, analytics_data: null, frame_number: 14230, detected_at: '2025-12-10 14:30:20' },
])
</script>
