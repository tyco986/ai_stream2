<template>
  <div>
    <PageHeader title="管道配置" subtitle="管理推理管道配置方案" />

    <div style="margin-bottom: 16px; text-align: right">
      <el-button type="primary" :icon="Plus" @click="openCreate">新建管道</el-button>
    </div>

    <el-table :data="profiles" stripe border style="width: 100%">
      <el-table-column prop="name" label="名称" min-width="140" />
      <el-table-column label="检测器" min-width="120">
        <template #default="{ row }">{{ row.detector.name }}</template>
      </el-table-column>
      <el-table-column label="跟踪器" min-width="120">
        <template #default="{ row }">
          {{ row.tracker?.name ?? '—' }}
        </template>
      </el-table-column>
      <el-table-column label="视频分析" width="110">
        <template #default="{ row }">
          <el-tag :type="row.analytics_enabled ? 'success' : 'info'" size="small">
            {{ row.analytics_enabled ? '启用' : '禁用' }}
          </el-tag>
        </template>
      </el-table-column>
      <el-table-column label="操作" width="200" fixed="right">
        <template #default="{ row }">
          <el-button link type="primary" size="small" @click="openEdit(row)">编辑</el-button>
          <el-button link type="success" size="small" @click="handleDeploy(row)">部署</el-button>
          <el-button link type="danger" size="small" @click="handleDelete(row)">删除</el-button>
        </template>
      </el-table-column>
    </el-table>

    <el-dialog
      v-model="dialogVisible"
      :title="isEditing ? '编辑管道' : '新建管道'"
      width="640px"
      destroy-on-close
    >
      <el-form :model="form" label-width="110px">
        <el-form-item label="名称" required>
          <el-input v-model="form.name" placeholder="管道名称" />
        </el-form-item>
        <el-form-item label="描述">
          <el-input v-model="form.description" type="textarea" :rows="2" />
        </el-form-item>
        <el-form-item label="检测器" required>
          <el-select v-model="form.detector_id" placeholder="选择检测器" style="width: 100%">
            <el-option
              v-for="d in detectorOptions"
              :key="d.id"
              :label="d.name"
              :value="d.id"
            />
          </el-select>
        </el-form-item>
        <el-form-item label="跟踪器">
          <el-select
            v-model="form.tracker_id"
            placeholder="选择跟踪器（可选）"
            clearable
            style="width: 100%"
          >
            <el-option
              v-for="t in trackerOptions"
              :key="t.id"
              :label="t.name"
              :value="t.id"
            />
          </el-select>
        </el-form-item>
        <el-form-item label="视频分析">
          <el-checkbox v-model="form.analytics_enabled">启用 nvdsanalytics</el-checkbox>
        </el-form-item>

        <el-divider>推理链路预览</el-divider>
        <div style="padding: 12px 16px; background: #f5f7fa; border-radius: 6px; font-size: 14px; color: #606266; text-align: center;">
          <span v-for="(step, i) in chainPreview" :key="i">
            <el-tag effect="dark" size="default" style="font-size: 13px">{{ step }}</el-tag>
            <span v-if="i < chainPreview.length - 1" style="margin: 0 8px; color: #c0c4cc">→</span>
          </span>
          <span v-if="chainPreview.length === 0" style="color: #c0c4cc">请选择检测器</span>
        </div>
      </el-form>

      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" @click="handleSubmit">
          {{ isEditing ? '保存' : '创建' }}
        </el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { Plus } from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import PageHeader from '@/components/common/PageHeader.vue'
import type { AIModel, PipelineProfile, PipelineProfileCreate } from '@/types/pipeline'

const mockModels: AIModel[] = [
  {
    id: '1', name: 'YOLOv8-L', model_type: 'detector', framework: 'engine',
    model_file: '/models/yolov8l.engine', label_file: '/models/coco_labels.txt',
    config: {}, version: 'v8.0', description: '', is_active: true,
    created_at: '2026-03-01T08:00:00Z', updated_at: '2026-03-15T10:00:00Z',
  },
  {
    id: '2', name: 'YOLOv5-S', model_type: 'detector', framework: 'onnx',
    model_file: '/models/yolov5s.onnx', label_file: '/models/coco_labels.txt',
    config: {}, version: 'v5.0', description: '', is_active: true,
    created_at: '2026-02-20T08:00:00Z', updated_at: '2026-03-10T10:00:00Z',
  },
  {
    id: '3', name: 'NvDCF Tracker', model_type: 'tracker', framework: 'custom',
    model_file: '/models/tracker_nvdcf.yml', label_file: null,
    config: {}, version: 'v1.0', description: '', is_active: true,
    created_at: '2026-03-05T08:00:00Z', updated_at: '2026-03-05T08:00:00Z',
  },
  {
    id: '4', name: 'IOU Tracker', model_type: 'tracker', framework: 'custom',
    model_file: '/models/tracker_iou.yml', label_file: null,
    config: {}, version: 'v1.0', description: '', is_active: false,
    created_at: '2026-03-06T08:00:00Z', updated_at: '2026-03-06T08:00:00Z',
  },
]

const detectorOptions = computed(() => mockModels.filter((m) => m.model_type === 'detector'))
const trackerOptions = computed(() => mockModels.filter((m) => m.model_type === 'tracker'))

function findModel(id: string | undefined): AIModel | undefined {
  return mockModels.find((m) => m.id === id)
}

/* @API:PIPE_LIST — GET /api/v1/pipeline-profiles/ */
const profiles = ref<PipelineProfile[]>([
  {
    id: 'p1', name: '默认检测管道', description: 'YOLOv8 + NvDCF + analytics',
    detector: mockModels[0], tracker: mockModels[2], analytics_enabled: true,
    analytics_config_stale: false, is_active: true,
    created_at: '2026-03-10T08:00:00Z', updated_at: '2026-03-20T08:00:00Z',
  },
  {
    id: 'p2', name: '轻量检测管道', description: 'YOLOv5 仅检测',
    detector: mockModels[1], tracker: null, analytics_enabled: false,
    analytics_config_stale: false, is_active: true,
    created_at: '2026-03-12T08:00:00Z', updated_at: '2026-03-18T08:00:00Z',
  },
  {
    id: 'p3', name: '全功能管道', description: 'YOLOv8 + IOU + analytics',
    detector: mockModels[0], tracker: mockModels[3], analytics_enabled: true,
    analytics_config_stale: true, is_active: false,
    created_at: '2026-03-15T08:00:00Z', updated_at: '2026-03-25T08:00:00Z',
  },
])

const dialogVisible = ref(false)
const isEditing = ref(false)
const editingId = ref<string | null>(null)

const defaultForm = (): PipelineProfileCreate => ({
  name: '',
  description: '',
  detector_id: '',
  tracker_id: undefined,
  analytics_enabled: false,
})

const form = ref<PipelineProfileCreate>(defaultForm())

const chainPreview = computed(() => {
  const steps: string[] = []
  const det = findModel(form.value.detector_id)
  if (det) steps.push(det.name)
  const trk = findModel(form.value.tracker_id)
  if (trk) steps.push(trk.name)
  if (form.value.analytics_enabled) steps.push('nvdsanalytics')
  return steps
})

function openCreate() {
  isEditing.value = false
  editingId.value = null
  form.value = defaultForm()
  dialogVisible.value = true
}

function openEdit(row: PipelineProfile) {
  isEditing.value = true
  editingId.value = row.id
  form.value = {
    name: row.name,
    description: row.description,
    detector_id: row.detector.id,
    tracker_id: row.tracker?.id,
    analytics_enabled: row.analytics_enabled,
  }
  dialogVisible.value = true
}

function handleSubmit() {
  if (isEditing.value) {
    /* @API:PIPE_UPDATE — PATCH /api/v1/pipeline-profiles/{id}/ */
    const idx = profiles.value.findIndex((p) => p.id === editingId.value)
    if (idx !== -1) {
      const det = findModel(form.value.detector_id)!
      const trk = form.value.tracker_id ? findModel(form.value.tracker_id) ?? null : null
      profiles.value[idx] = {
        ...profiles.value[idx],
        name: form.value.name,
        description: form.value.description ?? '',
        detector: det,
        tracker: trk,
        analytics_enabled: form.value.analytics_enabled ?? false,
        updated_at: new Date().toISOString(),
      }
    }
    ElMessage.success('管道配置已更新')
  } else {
    /* @API:PIPE_CREATE — POST /api/v1/pipeline-profiles/ */
    const det = findModel(form.value.detector_id)!
    const trk = form.value.tracker_id ? findModel(form.value.tracker_id) ?? null : null
    const newProfile: PipelineProfile = {
      id: String(Date.now()),
      name: form.value.name,
      description: form.value.description ?? '',
      detector: det,
      tracker: trk,
      analytics_enabled: form.value.analytics_enabled ?? false,
      analytics_config_stale: false,
      is_active: true,
      created_at: new Date().toISOString(),
      updated_at: new Date().toISOString(),
    }
    profiles.value.push(newProfile)
    ElMessage.success('管道配置已创建')
  }
  dialogVisible.value = false
}

async function handleDeploy(row: PipelineProfile) {
  await ElMessageBox.confirm(
    `部署管道「${row.name}」将重启 DeepStream 推理引擎，所有摄像头会短暂中断，是否继续？`,
    '确认部署',
    { type: 'warning', confirmButtonText: '部署', cancelButtonText: '取消' },
  )
  /* @API:PIPE_DEPLOY — POST /api/v1/pipeline-profiles/{id}/deploy/ */
  ElMessage.success(`管道「${row.name}」已提交部署`)
}

async function handleDelete(row: PipelineProfile) {
  await ElMessageBox.confirm(`确定删除管道配置「${row.name}」？`, '确认删除', {
    type: 'warning',
    confirmButtonText: '删除',
    cancelButtonText: '取消',
  })
  /* @API:PIPE_DELETE — DELETE /api/v1/pipeline-profiles/{id}/ */
  profiles.value = profiles.value.filter((p) => p.id !== row.id)
  ElMessage.success('管道配置已删除')
}
</script>
