<template>
  <div>
    <PageHeader title="AI 模型管理" subtitle="管理检测器和跟踪器模型" />

    <el-row :gutter="16" align="middle" style="margin-bottom: 16px">
      <el-col :span="6">
        <el-select v-model="filterType" placeholder="按类型筛选" clearable style="width: 100%">
          <el-option label="全部" value="" />
          <el-option label="检测器" value="detector" />
          <el-option label="跟踪器" value="tracker" />
        </el-select>
      </el-col>
      <el-col :span="18" style="text-align: right">
        <el-button type="primary" :icon="Plus" @click="openCreate">注册模型</el-button>
      </el-col>
    </el-row>

    <el-table :data="filteredModels" stripe border style="width: 100%">
      <el-table-column prop="name" label="名称" min-width="140" />
      <el-table-column label="类型" width="100">
        <template #default="{ row }">
          <el-tag :type="row.model_type === 'detector' ? 'primary' : 'success'" size="small">
            {{ row.model_type === 'detector' ? '检测器' : '跟踪器' }}
          </el-tag>
        </template>
      </el-table-column>
      <el-table-column prop="framework" label="框架" width="100" />
      <el-table-column prop="version" label="版本" width="100" />
      <el-table-column label="状态" width="100">
        <template #default="{ row }">
          <el-tag :type="row.is_active ? 'success' : 'info'" size="small">
            {{ row.is_active ? '启用' : '禁用' }}
          </el-tag>
        </template>
      </el-table-column>
      <el-table-column label="操作" width="160" fixed="right">
        <template #default="{ row }">
          <el-button link type="primary" size="small" @click="openEdit(row)">编辑</el-button>
          <el-button link type="danger" size="small" @click="handleDelete(row)">删除</el-button>
        </template>
      </el-table-column>
    </el-table>

    <el-dialog
      v-model="dialogVisible"
      :title="isEditing ? '编辑模型' : '注册模型'"
      width="600px"
      destroy-on-close
    >
      <el-form :model="form" label-width="100px">
        <el-form-item label="名称" required>
          <el-input v-model="form.name" placeholder="模型名称" />
        </el-form-item>
        <el-form-item label="类型" required>
          <el-select v-model="form.model_type" placeholder="选择类型" style="width: 100%">
            <el-option label="检测器" value="detector" />
            <el-option label="跟踪器" value="tracker" />
          </el-select>
        </el-form-item>
        <el-form-item label="框架" required>
          <el-select v-model="form.framework" placeholder="选择框架" style="width: 100%">
            <el-option label="ONNX" value="onnx" />
            <el-option label="TensorRT Engine" value="engine" />
            <el-option label="Custom" value="custom" />
          </el-select>
        </el-form-item>
        <el-form-item label="模型文件" required>
          <el-input v-model="form.model_file" placeholder="/models/yolov8.engine" />
        </el-form-item>
        <el-form-item label="标签文件">
          <el-input v-model="form.label_file" placeholder="/models/labels.txt" />
        </el-form-item>
        <el-form-item label="版本">
          <el-input v-model="form.version" placeholder="v1.0" />
        </el-form-item>
        <el-form-item label="描述">
          <el-input v-model="form.description" type="textarea" :rows="2" />
        </el-form-item>

        <el-divider v-if="form.model_type">
          {{ form.model_type === 'detector' ? '检测器配置' : '跟踪器配置' }}
        </el-divider>

        <template v-if="form.model_type === 'detector'">
          <el-form-item label="类别数">
            <el-input-number v-model="form.config.num_classes" :min="1" />
          </el-form-item>
          <el-form-item label="缩放因子">
            <el-input-number v-model="form.config.scale_factor" :min="0" :step="0.001" :precision="4" />
          </el-form-item>
          <el-form-item label="聚类模式">
            <el-select v-model="form.config.cluster_mode" style="width: 100%">
              <el-option label="NMS" value="nms" />
              <el-option label="DBSCAN" value="dbscan" />
              <el-option label="Hybrid" value="hybrid" />
            </el-select>
          </el-form-item>
          <el-form-item label="精度">
            <el-select v-model="form.config.precision" style="width: 100%">
              <el-option label="FP32" value="fp32" />
              <el-option label="FP16" value="fp16" />
              <el-option label="INT8" value="int8" />
            </el-select>
          </el-form-item>
        </template>

        <template v-if="form.model_type === 'tracker'">
          <el-form-item label="跟踪算法">
            <el-select v-model="form.config.tracker_type" style="width: 100%">
              <el-option label="NvDCF_perf" value="NvDCF_perf" />
              <el-option label="IOU" value="IOU" />
              <el-option label="NvSORT" value="NvSORT" />
            </el-select>
          </el-form-item>
        </template>
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
import type { AIModel, AIModelCreate, ModelType } from '@/types/pipeline'

/* @API:MODEL_LIST — GET /api/v1/ai-models/ */
const models = ref<AIModel[]>([
  {
    id: '1',
    name: 'YOLOv8-L',
    model_type: 'detector',
    framework: 'engine',
    model_file: '/models/yolov8l.engine',
    label_file: '/models/coco_labels.txt',
    config: { num_classes: 80, scale_factor: 0.0039, cluster_mode: 'nms', precision: 'fp16' },
    version: 'v8.0',
    description: 'YOLOv8 Large 检测模型',
    is_active: true,
    created_at: '2026-03-01T08:00:00Z',
    updated_at: '2026-03-15T10:00:00Z',
  },
  {
    id: '2',
    name: 'YOLOv5-S',
    model_type: 'detector',
    framework: 'onnx',
    model_file: '/models/yolov5s.onnx',
    label_file: '/models/coco_labels.txt',
    config: { num_classes: 80, scale_factor: 0.0039, cluster_mode: 'nms', precision: 'fp32' },
    version: 'v5.0',
    description: 'YOLOv5 Small 轻量检测模型',
    is_active: true,
    created_at: '2026-02-20T08:00:00Z',
    updated_at: '2026-03-10T10:00:00Z',
  },
  {
    id: '3',
    name: 'NvDCF Tracker',
    model_type: 'tracker',
    framework: 'custom',
    model_file: '/models/tracker_nvdcf.yml',
    label_file: null,
    config: { tracker_type: 'NvDCF_perf' },
    version: 'v1.0',
    description: 'NVIDIA DCF 高性能跟踪器',
    is_active: true,
    created_at: '2026-03-05T08:00:00Z',
    updated_at: '2026-03-05T08:00:00Z',
  },
  {
    id: '4',
    name: 'IOU Tracker',
    model_type: 'tracker',
    framework: 'custom',
    model_file: '/models/tracker_iou.yml',
    label_file: null,
    config: { tracker_type: 'IOU' },
    version: 'v1.0',
    description: 'IOU 轻量跟踪器',
    is_active: false,
    created_at: '2026-03-06T08:00:00Z',
    updated_at: '2026-03-06T08:00:00Z',
  },
])

const filterType = ref<ModelType | ''>('')

const filteredModels = computed(() => {
  if (!filterType.value) return models.value
  return models.value.filter((m) => m.model_type === filterType.value)
})

const dialogVisible = ref(false)
const isEditing = ref(false)
const editingId = ref<string | null>(null)

const defaultForm = (): AIModelCreate => ({
  name: '',
  model_type: 'detector',
  framework: 'onnx',
  model_file: '',
  label_file: '',
  config: {},
  version: '',
  description: '',
})

const form = ref<AIModelCreate>(defaultForm())

function openCreate() {
  isEditing.value = false
  editingId.value = null
  form.value = defaultForm()
  dialogVisible.value = true
}

function openEdit(row: AIModel) {
  isEditing.value = true
  editingId.value = row.id
  form.value = {
    name: row.name,
    model_type: row.model_type,
    framework: row.framework,
    model_file: row.model_file,
    label_file: row.label_file ?? '',
    config: { ...row.config },
    version: row.version,
    description: row.description,
  }
  dialogVisible.value = true
}

function handleSubmit() {
  if (isEditing.value) {
    /* @API:MODEL_UPDATE — PATCH /api/v1/ai-models/{id}/ */
    const idx = models.value.findIndex((m) => m.id === editingId.value)
    if (idx !== -1) {
      models.value[idx] = { ...models.value[idx], ...form.value }
    }
    ElMessage.success('模型已更新')
  } else {
    /* @API:MODEL_CREATE — POST /api/v1/ai-models/ */
    const newModel: AIModel = {
      id: String(Date.now()),
      ...form.value,
      label_file: form.value.label_file || null,
      is_active: true,
      created_at: new Date().toISOString(),
      updated_at: new Date().toISOString(),
    }
    models.value.push(newModel)
    ElMessage.success('模型已创建')
  }
  dialogVisible.value = false
}

async function handleDelete(row: AIModel) {
  await ElMessageBox.confirm(`确定删除模型「${row.name}」？`, '确认删除', {
    type: 'warning',
    confirmButtonText: '删除',
    cancelButtonText: '取消',
  })
  /* @API:MODEL_DELETE — DELETE /api/v1/ai-models/{id}/ */
  models.value = models.value.filter((m) => m.id !== row.id)
  ElMessage.success('模型已删除')
}
</script>
