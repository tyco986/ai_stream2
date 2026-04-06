<template>
  <div>
    <PageHeader title="检测记录" />

    <el-card shadow="never" style="margin-bottom: 16px">
      <el-form :inline="true" :model="filters">
        <el-form-item label="摄像头">
          <el-select v-model="filters.camera" placeholder="全部" clearable style="width: 180px">
            <el-option v-for="c in cameraOptions" :key="c.id" :label="c.name" :value="c.id" />
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
        <el-form-item label="目标类型">
          <el-input v-model="filters.objectType" placeholder="如: person" clearable style="width: 140px" />
        </el-form-item>
        <el-form-item>
          <el-button type="primary" @click="handleSearch">查询</el-button>
        </el-form-item>
      </el-form>
    </el-card>

    <el-card shadow="never">
      <!-- @API:DET_LIST — GET /api/v1/detections/ -->
      <el-table :data="pagedData" stripe style="width: 100%">
        <el-table-column label="时间" prop="detected_at" width="180" />
        <el-table-column label="摄像头" prop="camera_name" width="140" />
        <el-table-column label="目标类型" prop="object_type" width="120">
          <template #default="{ row }">
            <el-tag size="small">{{ row.object_type }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column label="置信度" width="100">
          <template #default="{ row }">
            {{ (row.confidence * 100).toFixed(1) }}%
          </template>
        </el-table-column>
        <el-table-column label="追踪ID" prop="tracker_id" width="100">
          <template #default="{ row }">
            {{ row.tracker_id ?? '—' }}
          </template>
        </el-table-column>
        <el-table-column label="分析结果" min-width="200">
          <template #default="{ row }">
            <template v-if="row.analytics_data">
              <el-tag
                v-for="(val, key) in row.analytics_data"
                :key="String(key)"
                size="small"
                type="info"
                style="margin-right: 4px; margin-bottom: 2px"
              >
                {{ key }}: {{ val }}
              </el-tag>
            </template>
            <span v-else style="color: #c0c4cc">—</span>
          </template>
        </el-table-column>
      </el-table>

      <div style="display: flex; justify-content: flex-end; margin-top: 16px">
        <el-pagination
          v-model:current-page="pagination.page"
          v-model:page-size="pagination.pageSize"
          :total="mockDetections.length"
          :page-sizes="[10, 20, 50]"
          layout="total, sizes, prev, pager, next"
          background
        />
      </div>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import type { Detection } from '@/types/detection'
import PageHeader from '@/components/common/PageHeader.vue'

const filters = ref({
  camera: '',
  dateRange: null as [string, string] | null,
  objectType: '',
})

const cameraOptions = [
  { id: 'cam-001', name: '前门摄像头' },
  { id: 'cam-002', name: '后门摄像头' },
  { id: 'cam-003', name: '停车场A' },
  { id: 'cam-004', name: '大厅' },
]

/* @API:DET_LIST — GET /api/v1/detections/ */
const mockDetections: Detection[] = [
  { id: 'd-001', camera: 'cam-001', camera_name: '前门摄像头', object_type: 'person', confidence: 0.952, bbox: { x: 120, y: 80, w: 60, h: 150 }, tracker_id: 1001, analytics_data: { zone: '入口A', direction: 'in' }, frame_number: 14320, detected_at: '2026-04-06 09:12:33' },
  { id: 'd-002', camera: 'cam-001', camera_name: '前门摄像头', object_type: 'person', confidence: 0.891, bbox: { x: 340, y: 90, w: 55, h: 140 }, tracker_id: 1002, analytics_data: { zone: '入口A', direction: 'out' }, frame_number: 14325, detected_at: '2026-04-06 09:12:35' },
  { id: 'd-003', camera: 'cam-002', camera_name: '后门摄像头', object_type: 'car', confidence: 0.874, bbox: { x: 200, y: 300, w: 180, h: 100 }, tracker_id: 2001, analytics_data: { zone: '停车区', event: 'entry' }, frame_number: 8760, detected_at: '2026-04-06 09:10:18' },
  { id: 'd-004', camera: 'cam-003', camera_name: '停车场A', object_type: 'car', confidence: 0.931, bbox: { x: 50, y: 200, w: 200, h: 120 }, tracker_id: 3001, analytics_data: { zone: 'B区', occupancy: 12 }, frame_number: 22100, detected_at: '2026-04-06 09:08:45' },
  { id: 'd-005', camera: 'cam-004', camera_name: '大厅', object_type: 'person', confidence: 0.967, bbox: { x: 400, y: 150, w: 50, h: 130 }, tracker_id: 4001, analytics_data: { zone: '大厅', crowd_count: 5 }, frame_number: 30200, detected_at: '2026-04-06 09:05:22' },
  { id: 'd-006', camera: 'cam-004', camera_name: '大厅', object_type: 'person', confidence: 0.812, bbox: { x: 500, y: 160, w: 48, h: 125 }, tracker_id: 4002, analytics_data: null, frame_number: 30205, detected_at: '2026-04-06 09:05:24' },
  { id: 'd-007', camera: 'cam-001', camera_name: '前门摄像头', object_type: 'bicycle', confidence: 0.783, bbox: { x: 100, y: 280, w: 80, h: 60 }, tracker_id: 1003, analytics_data: { zone: '入口A', direction: 'in' }, frame_number: 14500, detected_at: '2026-04-06 09:03:11' },
  { id: 'd-008', camera: 'cam-002', camera_name: '后门摄像头', object_type: 'person', confidence: 0.905, bbox: { x: 300, y: 100, w: 55, h: 145 }, tracker_id: 2002, analytics_data: { zone: '通道', line_cross: true }, frame_number: 8900, detected_at: '2026-04-06 09:01:07' },
  { id: 'd-009', camera: 'cam-003', camera_name: '停车场A', object_type: 'truck', confidence: 0.846, bbox: { x: 10, y: 180, w: 250, h: 150 }, tracker_id: 3002, analytics_data: { zone: 'A区', occupancy: 8 }, frame_number: 22300, detected_at: '2026-04-06 08:58:50' },
  { id: 'd-010', camera: 'cam-004', camera_name: '大厅', object_type: 'bag', confidence: 0.724, bbox: { x: 420, y: 310, w: 35, h: 30 }, tracker_id: null, analytics_data: { zone: '大厅', unattended: true }, frame_number: 30400, detected_at: '2026-04-06 08:55:16' },
]

const pagination = ref({ page: 1, pageSize: 10 })

const pagedData = computed(() => {
  const start = (pagination.value.page - 1) * pagination.value.pageSize
  return mockDetections.slice(start, start + pagination.value.pageSize)
})

function handleSearch() {
  pagination.value.page = 1
}
</script>
