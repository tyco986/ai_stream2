<template>
  <div>
    <PageHeader title="截图管理" />

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
        <el-form-item>
          <el-button type="primary" @click="handleSearch">查询</el-button>
        </el-form-item>
      </el-form>
    </el-card>

    <!-- @API:SS_LIST — GET /api/v1/screenshots/ -->
    <el-row :gutter="16">
      <el-col v-for="item in pagedData" :key="item.id" :span="6" style="margin-bottom: 16px">
        <el-card shadow="hover" :body-style="{ padding: '0' }">
          <div
            style="aspect-ratio: 16/9; background: #e8eaed; display: flex; align-items: center; justify-content: center; cursor: pointer"
            @click="openPreview(item)"
          >
            <el-icon :size="36" color="#bbb"><Camera /></el-icon>
          </div>
          <div style="padding: 12px">
            <div style="font-weight: 500; font-size: 14px; color: #303133; margin-bottom: 4px">
              {{ item.camera_name }}
            </div>
            <div style="font-size: 12px; color: #909399; margin-bottom: 8px">
              {{ item.captured_at }}
            </div>
            <div style="display: flex; gap: 8px">
              <el-button type="primary" link size="small" @click.stop="openPreview(item)">查看</el-button>
              <el-button type="success" link size="small" @click.stop="handleDownload(item)">下载</el-button>
            </div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <div style="display: flex; justify-content: flex-end; margin-top: 8px">
      <el-pagination
        v-model:current-page="pagination.page"
        v-model:page-size="pagination.pageSize"
        :total="mockScreenshots.length"
        :page-sizes="[8, 16, 32]"
        layout="total, sizes, prev, pager, next"
        background
      />
    </div>

    <!-- Preview Dialog -->
    <el-dialog v-model="previewVisible" :title="previewRow?.camera_name ?? '截图预览'" width="640px" destroy-on-close>
      <div v-if="previewRow">
        <div style="aspect-ratio: 16/9; background: #e8eaed; display: flex; align-items: center; justify-content: center; border-radius: 8px; margin-bottom: 12px">
          <el-icon :size="64" color="#bbb"><Camera /></el-icon>
        </div>
        <el-descriptions :column="2" size="small" border>
          <el-descriptions-item label="摄像头">{{ previewRow.camera_name }}</el-descriptions-item>
          <el-descriptions-item label="拍摄时间">{{ previewRow.captured_at }}</el-descriptions-item>
          <el-descriptions-item label="文件大小">{{ formatFileSize(previewRow.file_size_bytes) }}</el-descriptions-item>
        </el-descriptions>
        <div style="margin-top: 16px; text-align: right">
          <!-- @API:SS_DOWNLOAD — GET /api/v1/screenshots/{id}/download/ -->
          <el-button type="primary" @click="handleDownload(previewRow)">下载</el-button>
        </div>
      </div>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { Camera } from '@element-plus/icons-vue'
import type { Screenshot } from '@/types/screenshot'
import PageHeader from '@/components/common/PageHeader.vue'

const filters = ref({
  camera: '',
  dateRange: null as [string, string] | null,
})

const cameraOptions = [
  { id: 'cam-001', name: '前门摄像头' },
  { id: 'cam-002', name: '后门摄像头' },
  { id: 'cam-003', name: '停车场A' },
  { id: 'cam-004', name: '大厅' },
]

/* @API:SS_LIST — GET /api/v1/screenshots/ */
const mockScreenshots: Screenshot[] = [
  { id: 's-001', camera: 'cam-001', camera_name: '前门摄像头', file_path: '/screenshots/cam-001/20260406_091233.jpg', file_size_bytes: 245760, captured_at: '2026-04-06 09:12:33', created_at: '2026-04-06 09:12:34' },
  { id: 's-002', camera: 'cam-002', camera_name: '后门摄像头', file_path: '/screenshots/cam-002/20260406_091018.jpg', file_size_bytes: 198656, captured_at: '2026-04-06 09:10:18', created_at: '2026-04-06 09:10:19' },
  { id: 's-003', camera: 'cam-003', camera_name: '停车场A', file_path: '/screenshots/cam-003/20260406_090845.jpg', file_size_bytes: 312320, captured_at: '2026-04-06 09:08:45', created_at: '2026-04-06 09:08:46' },
  { id: 's-004', camera: 'cam-004', camera_name: '大厅', file_path: '/screenshots/cam-004/20260406_090522.jpg', file_size_bytes: 276480, captured_at: '2026-04-06 09:05:22', created_at: '2026-04-06 09:05:23' },
  { id: 's-005', camera: 'cam-001', camera_name: '前门摄像头', file_path: '/screenshots/cam-001/20260406_085000.jpg', file_size_bytes: 231424, captured_at: '2026-04-06 08:50:00', created_at: '2026-04-06 08:50:01' },
  { id: 's-006', camera: 'cam-003', camera_name: '停车场A', file_path: '/screenshots/cam-003/20260406_083000.jpg', file_size_bytes: 340992, captured_at: '2026-04-06 08:30:00', created_at: '2026-04-06 08:30:01' },
]

const pagination = ref({ page: 1, pageSize: 8 })

const pagedData = computed(() => {
  const start = (pagination.value.page - 1) * pagination.value.pageSize
  return mockScreenshots.slice(start, start + pagination.value.pageSize)
})

const previewVisible = ref(false)
const previewRow = ref<Screenshot | null>(null)

function handleSearch() {
  pagination.value.page = 1
}

function openPreview(item: Screenshot) {
  previewRow.value = item
  previewVisible.value = true
}

function handleDownload(item: Screenshot) {
  /* @API:SS_DOWNLOAD — GET /api/v1/screenshots/{id}/download/ */
  ElMessage.info(`下载: ${item.file_path}`)
}

function formatFileSize(bytes: number): string {
  if (bytes < 1024) return `${bytes} B`
  if (bytes < 1048576) return `${(bytes / 1024).toFixed(1)} KB`
  return `${(bytes / 1048576).toFixed(1)} MB`
}
</script>
