<template>
  <div>
    <PageHeader title="报警记录" />

    <!-- Filters -->
    <el-card shadow="hover" style="border-radius: 8px; margin-bottom: 16px">
      <el-row :gutter="16" align="middle">
        <el-col :span="5">
          <el-select v-model="filters.status" placeholder="状态" clearable style="width: 100%">
            <el-option label="全部" value="" />
            <el-option label="待处理" value="pending" />
            <el-option label="已确认" value="acknowledged" />
            <el-option label="已解决" value="resolved" />
          </el-select>
        </el-col>
        <el-col :span="5">
          <el-select v-model="filters.camera" placeholder="摄像头" clearable style="width: 100%">
            <el-option v-for="c in cameraOptions" :key="c.id" :label="c.name" :value="c.id" />
          </el-select>
        </el-col>
        <el-col :span="9">
          <el-date-picker
            v-model="filters.dateRange"
            type="daterange"
            range-separator="至"
            start-placeholder="开始日期"
            end-placeholder="结束日期"
            style="width: 100%"
          />
        </el-col>
        <el-col :span="3">
          <el-button type="primary" @click="handleQuery">查询</el-button>
        </el-col>
      </el-row>
    </el-card>

    <!-- Table -->
    <el-card shadow="hover" style="border-radius: 8px">
      <el-table :data="filteredAlerts" stripe style="width: 100%">
        <el-table-column label="触发时间" width="180">
          <template #default="{ row }">
            {{ formatTime(row.triggered_at) }}
          </template>
        </el-table-column>
        <el-table-column prop="rule_name" label="规则名称" min-width="140" />
        <el-table-column prop="camera_name" label="摄像头" min-width="130" />
        <el-table-column label="状态" width="110">
          <template #default="{ row }">
            <el-tag :type="statusTagType[row.status]" size="small">
              {{ statusLabel[row.status] }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="操作" width="220" fixed="right">
          <template #default="{ row }">
            <el-button
              v-if="row.status === 'pending'"
              link
              type="warning"
              @click="handleAcknowledge(row)"
            >确认</el-button>
            <el-button
              v-if="row.status === 'pending' || row.status === 'acknowledged'"
              link
              type="success"
              @click="handleResolve(row)"
            >解决</el-button>
            <el-button
              v-if="row.recording_id"
              link
              type="primary"
              @click="handleViewRecording(row)"
            >查看录像</el-button>
          </template>
        </el-table-column>
      </el-table>

      <div style="display: flex; justify-content: flex-end; margin-top: 16px">
        <el-pagination
          v-model:current-page="currentPage"
          :page-size="pageSize"
          :total="totalCount"
          layout="total, prev, pager, next"
        />
      </div>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, computed } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import type { Alert, AlertStatus } from '@/types/alert'
import PageHeader from '@/components/common/PageHeader.vue'

const router = useRouter()

const statusLabel: Record<AlertStatus, string> = {
  pending: '待处理',
  acknowledged: '已确认',
  resolved: '已解决',
}

const statusTagType: Record<AlertStatus, 'danger' | 'warning' | 'success'> = {
  pending: 'danger',
  acknowledged: 'warning',
  resolved: 'success',
}

const cameraOptions = ref([
  { id: 'cam-1', name: '前门摄像头' },
  { id: 'cam-2', name: '后门摄像头' },
  { id: 'cam-3', name: '停车场A' },
  { id: 'cam-4', name: '大厅摄像头' },
  { id: 'cam-5', name: '仓库通道' },
])

const filters = reactive({
  status: '' as AlertStatus | '',
  camera: '',
  dateRange: null as [Date, Date] | null,
})

const currentPage = ref(1)
const pageSize = 10
const totalCount = ref(8)

/* @API:ALERT_LIST — GET /api/v1/alerts/ */
const alerts = ref<Alert[]>([
  {
    id: 'alert-1',
    rule: 'rule-1',
    rule_name: '大厅人数超限',
    camera: 'cam-4',
    camera_name: '大厅摄像头',
    status: 'pending',
    detail: { count: 58, threshold: 50 },
    recording_id: 'rec-101',
    acknowledged_by: null,
    acknowledged_at: null,
    resolved_by: null,
    resolved_at: null,
    triggered_at: '2025-06-10T14:32:00Z',
  },
  {
    id: 'alert-2',
    rule: 'rule-3',
    rule_name: '禁区入侵告警',
    camera: 'cam-1',
    camera_name: '前门摄像头',
    status: 'pending',
    detail: { zone: '禁区A', object: 'person' },
    recording_id: 'rec-102',
    acknowledged_by: null,
    acknowledged_at: null,
    resolved_by: null,
    resolved_at: null,
    triggered_at: '2025-06-10T13:45:00Z',
  },
  {
    id: 'alert-3',
    rule: 'rule-2',
    rule_name: '仓库车辆检测',
    camera: 'cam-5',
    camera_name: '仓库通道',
    status: 'acknowledged',
    detail: { object_type: 'vehicle', count: 2 },
    recording_id: null,
    acknowledged_by: 'user-1',
    acknowledged_at: '2025-06-10T12:10:00Z',
    resolved_by: null,
    resolved_at: null,
    triggered_at: '2025-06-10T12:00:00Z',
  },
  {
    id: 'alert-4',
    rule: 'rule-5',
    rule_name: '大厅拥挤检测',
    camera: 'cam-4',
    camera_name: '大厅摄像头',
    status: 'resolved',
    detail: { zone: '大厅中央' },
    recording_id: 'rec-103',
    acknowledged_by: 'user-1',
    acknowledged_at: '2025-06-10T10:05:00Z',
    resolved_by: 'user-1',
    resolved_at: '2025-06-10T10:30:00Z',
    triggered_at: '2025-06-10T10:00:00Z',
  },
  {
    id: 'alert-5',
    rule: 'rule-4',
    rule_name: '入口越线计数',
    camera: 'cam-1',
    camera_name: '前门摄像头',
    status: 'pending',
    detail: { line: '入口线1', count: 115 },
    recording_id: null,
    acknowledged_by: null,
    acknowledged_at: null,
    resolved_by: null,
    resolved_at: null,
    triggered_at: '2025-06-10T09:20:00Z',
  },
  {
    id: 'alert-6',
    rule: 'rule-3',
    rule_name: '禁区入侵告警',
    camera: 'cam-2',
    camera_name: '后门摄像头',
    status: 'acknowledged',
    detail: { zone: '禁区A', object: 'person' },
    recording_id: 'rec-104',
    acknowledged_by: 'user-2',
    acknowledged_at: '2025-06-09T18:00:00Z',
    resolved_by: null,
    resolved_at: null,
    triggered_at: '2025-06-09T17:45:00Z',
  },
  {
    id: 'alert-7',
    rule: 'rule-1',
    rule_name: '大厅人数超限',
    camera: 'cam-4',
    camera_name: '大厅摄像头',
    status: 'resolved',
    detail: { count: 52, threshold: 50 },
    recording_id: 'rec-105',
    acknowledged_by: 'user-1',
    acknowledged_at: '2025-06-09T15:10:00Z',
    resolved_by: 'user-1',
    resolved_at: '2025-06-09T15:30:00Z',
    triggered_at: '2025-06-09T15:00:00Z',
  },
  {
    id: 'alert-8',
    rule: 'rule-2',
    rule_name: '仓库车辆检测',
    camera: 'cam-3',
    camera_name: '停车场A',
    status: 'resolved',
    detail: { object_type: 'vehicle', count: 3 },
    recording_id: null,
    acknowledged_by: 'user-2',
    acknowledged_at: '2025-06-09T11:00:00Z',
    resolved_by: 'user-2',
    resolved_at: '2025-06-09T12:00:00Z',
    triggered_at: '2025-06-09T10:30:00Z',
  },
])

const filteredAlerts = computed(() => {
  return alerts.value.filter((a) => {
    if (filters.status && a.status !== filters.status) return false
    if (filters.camera && a.camera !== filters.camera) return false
    if (filters.dateRange) {
      const t = new Date(a.triggered_at).getTime()
      if (t < filters.dateRange[0].getTime() || t > filters.dateRange[1].getTime()) return false
    }
    return true
  })
})

function formatTime(iso: string): string {
  return new Date(iso).toLocaleString('zh-CN', {
    year: 'numeric',
    month: '2-digit',
    day: '2-digit',
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
  })
}

function handleQuery() {
  /* @API:ALERT_LIST — GET /api/v1/alerts/ */
  currentPage.value = 1
  ElMessage.info('已刷新查询')
}

function handleAcknowledge(row: Alert) {
  /* @API:ALERT_ACK — POST /api/v1/alerts/{id}/acknowledge/ */
  row.status = 'acknowledged'
  row.acknowledged_by = 'current-user'
  row.acknowledged_at = new Date().toISOString()
  ElMessage.success('已确认')
}

function handleResolve(row: Alert) {
  /* @API:ALERT_RESOLVE — POST /api/v1/alerts/{id}/resolve/ */
  row.status = 'resolved'
  row.resolved_by = 'current-user'
  row.resolved_at = new Date().toISOString()
  ElMessage.success('已解决')
}

function handleViewRecording(row: Alert) {
  router.push({ path: '/recordings', query: { id: row.recording_id! } })
}
</script>
