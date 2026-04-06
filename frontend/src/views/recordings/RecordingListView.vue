<template>
  <div>
    <PageHeader title="录像回放" />

    <el-card shadow="never" style="margin-bottom: 16px">
      <el-form :inline="true" :model="filters">
        <el-form-item label="摄像头">
          <el-select v-model="filters.camera" placeholder="全部" clearable style="width: 180px">
            <el-option v-for="c in cameraOptions" :key="c.id" :label="c.name" :value="c.id" />
          </el-select>
        </el-form-item>
        <el-form-item label="录像类型">
          <el-select v-model="filters.recordingType" placeholder="全部" clearable style="width: 140px">
            <el-option label="全部" value="" />
            <el-option label="滚动" value="rolling" />
            <el-option label="事件" value="event" />
            <el-option label="手动" value="manual" />
          </el-select>
        </el-form-item>
        <el-form-item label="时间范围">
          <el-date-picker
            v-model="filters.dateRange"
            type="daterange"
            range-separator="至"
            start-placeholder="开始日期"
            end-placeholder="结束日期"
            value-format="YYYY-MM-DD"
            style="width: 260px"
          />
        </el-form-item>
        <el-form-item>
          <el-button type="primary" @click="handleSearch">查询</el-button>
        </el-form-item>
      </el-form>
    </el-card>

    <el-card shadow="never">
      <!-- @API:REC_LIST — GET /api/v1/recordings/ -->
      <el-table :data="pagedData" stripe style="width: 100%">
        <el-table-column label="摄像头" prop="camera_name" width="150" />
        <el-table-column label="类型" width="100">
          <template #default="{ row }">
            <el-tag :type="typeTagMap[row.recording_type].type" size="small">
              {{ typeTagMap[row.recording_type].label }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="开始时间" prop="started_at" width="180" />
        <el-table-column label="时长" width="100">
          <template #default="{ row }">
            {{ formatDuration(row.duration_seconds) }}
          </template>
        </el-table-column>
        <el-table-column label="文件大小" width="120">
          <template #default="{ row }">
            {{ formatFileSize(row.file_size_bytes) }}
          </template>
        </el-table-column>
        <el-table-column label="操作" width="160" fixed="right">
          <template #default="{ row }">
            <el-button type="primary" link size="small" @click="openPlayback(row)">回放</el-button>
            <el-button type="success" link size="small" @click="handleDownload(row)">下载</el-button>
          </template>
        </el-table-column>
      </el-table>

      <div style="display: flex; justify-content: flex-end; margin-top: 16px">
        <el-pagination
          v-model:current-page="pagination.page"
          v-model:page-size="pagination.pageSize"
          :total="mockRecordings.length"
          :page-sizes="[10, 20, 50]"
          layout="total, sizes, prev, pager, next"
          background
        />
      </div>
    </el-card>

    <!-- Playback Dialog -->
    <el-dialog
      v-model="playbackVisible"
      :title="`录像回放 — ${playbackRow?.camera_name ?? ''}`"
      width="720px"
      destroy-on-close
      :style="{ '--el-dialog-bg-color': '#1a1a2e' }"
    >
      <div v-if="playbackRow" style="color: #e0e0e0">
        <!-- Mock video player area -->
        <div style="background: #0f0f23; border-radius: 8px; overflow: hidden; margin-bottom: 16px">
          <div style="aspect-ratio: 16/9; display: flex; align-items: center; justify-content: center; position: relative">
            <el-icon :size="48" color="#555"><VideoPlay /></el-icon>
            <div style="position: absolute; bottom: 0; left: 0; right: 0; background: rgba(0,0,0,0.6); padding: 8px 12px">
              <div style="display: flex; align-items: center; gap: 8px">
                <el-icon color="#fff"><VideoPlay /></el-icon>
                <div style="flex: 1; height: 4px; background: #333; border-radius: 2px; position: relative">
                  <div style="width: 35%; height: 100%; background: #409eff; border-radius: 2px" />
                </div>
                <span style="color: #aaa; font-size: 12px">{{ formatDuration(playbackRow.duration_seconds) }}</span>
              </div>
            </div>
          </div>
        </div>

        <el-descriptions :column="2" size="small" border>
          <el-descriptions-item label="摄像头">{{ playbackRow.camera_name }}</el-descriptions-item>
          <el-descriptions-item label="类型">
            <el-tag :type="typeTagMap[playbackRow.recording_type].type" size="small">
              {{ typeTagMap[playbackRow.recording_type].label }}
            </el-tag>
          </el-descriptions-item>
          <el-descriptions-item label="开始时间">{{ playbackRow.started_at }}</el-descriptions-item>
          <el-descriptions-item label="结束时间">{{ playbackRow.ended_at }}</el-descriptions-item>
          <el-descriptions-item label="时长">{{ formatDuration(playbackRow.duration_seconds) }}</el-descriptions-item>
          <el-descriptions-item label="文件大小">{{ formatFileSize(playbackRow.file_size_bytes) }}</el-descriptions-item>
        </el-descriptions>

        <div style="margin-top: 16px; text-align: right">
          <!-- @API:REC_DOWNLOAD — GET /api/v1/recordings/{id}/download/ -->
          <el-button type="primary" @click="handleDownload(playbackRow)">下载</el-button>
        </div>
      </div>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { VideoPlay } from '@element-plus/icons-vue'
import type { Recording, RecordingType } from '@/types/recording'
import PageHeader from '@/components/common/PageHeader.vue'

const filters = ref({
  camera: '',
  recordingType: '',
  dateRange: null as [string, string] | null,
})

const cameraOptions = [
  { id: 'cam-001', name: '前门摄像头' },
  { id: 'cam-002', name: '后门摄像头' },
  { id: 'cam-003', name: '停车场A' },
  { id: 'cam-004', name: '大厅' },
]

const typeTagMap: Record<RecordingType, { label: string; type: 'info' | 'danger' | 'warning' }> = {
  rolling: { label: '滚动', type: 'info' },
  event: { label: '事件', type: 'danger' },
  manual: { label: '手动', type: 'warning' },
}

/* @API:REC_LIST — GET /api/v1/recordings/ */
const mockRecordings: Recording[] = [
  { id: 'r-001', camera: 'cam-001', camera_name: '前门摄像头', recording_type: 'rolling', file_path: '/recordings/cam-001/2026-04-06_08-00.mp4', duration_seconds: 3600, file_size_bytes: 524288000, started_at: '2026-04-06 08:00:00', ended_at: '2026-04-06 09:00:00', created_at: '2026-04-06 09:00:05' },
  { id: 'r-002', camera: 'cam-001', camera_name: '前门摄像头', recording_type: 'event', file_path: '/recordings/cam-001/evt_2026-04-06_09-12.mp4', duration_seconds: 45, file_size_bytes: 6553600, started_at: '2026-04-06 09:12:10', ended_at: '2026-04-06 09:12:55', created_at: '2026-04-06 09:13:00' },
  { id: 'r-003', camera: 'cam-002', camera_name: '后门摄像头', recording_type: 'rolling', file_path: '/recordings/cam-002/2026-04-06_08-00.mp4', duration_seconds: 3600, file_size_bytes: 498073600, started_at: '2026-04-06 08:00:00', ended_at: '2026-04-06 09:00:00', created_at: '2026-04-06 09:00:03' },
  { id: 'r-004', camera: 'cam-003', camera_name: '停车场A', recording_type: 'manual', file_path: '/recordings/cam-003/manual_2026-04-06_08-30.mp4', duration_seconds: 120, file_size_bytes: 15728640, started_at: '2026-04-06 08:30:00', ended_at: '2026-04-06 08:32:00', created_at: '2026-04-06 08:32:05' },
  { id: 'r-005', camera: 'cam-004', camera_name: '大厅', recording_type: 'event', file_path: '/recordings/cam-004/evt_2026-04-06_09-05.mp4', duration_seconds: 30, file_size_bytes: 4194304, started_at: '2026-04-06 09:05:00', ended_at: '2026-04-06 09:05:30', created_at: '2026-04-06 09:05:35' },
  { id: 'r-006', camera: 'cam-002', camera_name: '后门摄像头', recording_type: 'event', file_path: '/recordings/cam-002/evt_2026-04-06_09-01.mp4', duration_seconds: 62, file_size_bytes: 8912896, started_at: '2026-04-06 09:01:00', ended_at: '2026-04-06 09:02:02', created_at: '2026-04-06 09:02:07' },
  { id: 'r-007', camera: 'cam-003', camera_name: '停车场A', recording_type: 'rolling', file_path: '/recordings/cam-003/2026-04-06_08-00.mp4', duration_seconds: 3600, file_size_bytes: 536870912, started_at: '2026-04-06 08:00:00', ended_at: '2026-04-06 09:00:00', created_at: '2026-04-06 09:00:04' },
  { id: 'r-008', camera: 'cam-004', camera_name: '大厅', recording_type: 'manual', file_path: '/recordings/cam-004/manual_2026-04-06_09-00.mp4', duration_seconds: 180, file_size_bytes: 23068672, started_at: '2026-04-06 09:00:00', ended_at: '2026-04-06 09:03:00', created_at: '2026-04-06 09:03:03' },
]

const pagination = ref({ page: 1, pageSize: 10 })

const pagedData = computed(() => {
  const start = (pagination.value.page - 1) * pagination.value.pageSize
  return mockRecordings.slice(start, start + pagination.value.pageSize)
})

const playbackVisible = ref(false)
const playbackRow = ref<Recording | null>(null)

function handleSearch() {
  pagination.value.page = 1
}

function openPlayback(row: Recording) {
  /* @API:REC_STREAM — GET /api/v1/recordings/{id}/stream/ */
  playbackRow.value = row
  playbackVisible.value = true
}

function handleDownload(row: Recording) {
  /* @API:REC_DOWNLOAD — GET /api/v1/recordings/{id}/download/ */
  ElMessage.info(`下载: ${row.file_path}`)
}

function formatDuration(seconds: number): string {
  const m = Math.floor(seconds / 60)
  const s = seconds % 60
  return `${String(m).padStart(2, '0')}:${String(s).padStart(2, '0')}`
}

function formatFileSize(bytes: number): string {
  if (bytes < 1024) return `${bytes} B`
  if (bytes < 1048576) return `${(bytes / 1024).toFixed(1)} KB`
  if (bytes < 1073741824) return `${(bytes / 1048576).toFixed(1)} MB`
  return `${(bytes / 1073741824).toFixed(2)} GB`
}
</script>
