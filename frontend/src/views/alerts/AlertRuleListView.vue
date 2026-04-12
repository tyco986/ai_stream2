<template>
  <div>
    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px">
      <PageHeader title="报警规则" style="margin-bottom: 0" />
      <el-button type="primary" :icon="Plus" @click="openCreateDialog">新建规则</el-button>
    </div>

    <el-card shadow="hover" style="border-radius: 8px">
      <el-table :data="rules" stripe style="width: 100%">
        <el-table-column prop="name" label="名称" min-width="140" />
        <el-table-column label="规则类型" width="130">
          <template #default="{ row }">
            <el-tag size="small">{{ ruleTypeLabel[row.rule_type] }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column label="关联摄像头" width="120" align="center">
          <template #default="{ row }">
            {{ row.cameras.length }}
          </template>
        </el-table-column>
        <el-table-column label="启用状态" width="100" align="center">
          <template #default="{ row }">
            <el-switch
              v-model="row.is_enabled"
              @change="(val: boolean) => handleToggle(row, val)"
            />
          </template>
        </el-table-column>
        <el-table-column prop="cooldown_seconds" label="冷却时间" width="110" align="center">
          <template #default="{ row }">
            {{ row.cooldown_seconds }}s
          </template>
        </el-table-column>
        <el-table-column label="操作" width="160" fixed="right">
          <template #default="{ row }">
            <el-button link type="primary" @click="openEditDialog(row)">编辑</el-button>
            <el-button link type="danger" @click="handleDelete(row)">删除</el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <!-- Create / Edit dialog -->
    <el-dialog
      v-model="dialogVisible"
      :title="isEdit ? '编辑规则' : '新建规则'"
      width="600px"
      destroy-on-close
    >
      <el-form :model="form" label-width="100px">
        <el-form-item label="名称">
          <el-input v-model="form.name" placeholder="输入规则名称" />
        </el-form-item>
        <el-form-item label="规则类型">
          <el-select v-model="form.rule_type" placeholder="选择规则类型" style="width: 100%" @change="handleRuleTypeChange">
            <el-option v-for="(label, key) in ruleTypeLabel" :key="key" :label="label" :value="key" />
          </el-select>
        </el-form-item>
        <el-form-item label="关联摄像头">
          <el-select v-model="form.cameras" multiple placeholder="选择摄像头" style="width: 100%">
            <el-option v-for="c in cameraOptions" :key="c.id" :label="c.name" :value="c.id" />
          </el-select>
        </el-form-item>
        <el-form-item label="冷却时间">
          <el-input-number v-model="form.cooldown_seconds" :min="0" :step="10" />
          <span style="margin-left: 8px; color: #909399">秒</span>
        </el-form-item>
        <el-form-item label="启用">
          <el-switch v-model="form.is_enabled" />
        </el-form-item>

        <!-- Dynamic conditions -->
        <el-divider content-position="left">触发条件</el-divider>

        <template v-if="form.rule_type === 'object_count'">
          <el-form-item label="阈值">
            <el-input-number v-model="form.conditions.threshold" :min="1" />
          </el-form-item>
        </template>

        <template v-else-if="form.rule_type === 'object_type'">
          <el-form-item label="目标类型">
            <el-select v-model="form.conditions.object_type" placeholder="选择目标类型" style="width: 100%">
              <el-option label="人" value="person" />
              <el-option label="车辆" value="vehicle" />
              <el-option label="自行车" value="bicycle" />
            </el-select>
          </el-form-item>
          <el-form-item label="阈值">
            <el-input-number v-model="form.conditions.threshold" :min="1" />
          </el-form-item>
        </template>

        <template v-else-if="form.rule_type === 'zone_intrusion'">
          <el-form-item label="区域">
            <el-select v-model="form.conditions.zone_name" placeholder="选择区域" style="width: 100%">
              <el-option v-for="z in zoneOptions" :key="z" :label="z" :value="z" />
            </el-select>
          </el-form-item>
          <el-form-item label="目标类型">
            <el-select v-model="form.conditions.target_type" placeholder="选择目标类型" style="width: 100%">
              <el-option label="人" value="person" />
              <el-option label="车辆" value="vehicle" />
              <el-option label="全部" value="all" />
            </el-select>
          </el-form-item>
        </template>

        <template v-else-if="form.rule_type === 'line_crossing'">
          <el-form-item label="检测线">
            <el-select v-model="form.conditions.line_name" placeholder="选择检测线" style="width: 100%">
              <el-option v-for="l in lineOptions" :key="l" :label="l" :value="l" />
            </el-select>
          </el-form-item>
          <el-form-item label="计数阈值">
            <el-input-number v-model="form.conditions.count_threshold" :min="1" />
          </el-form-item>
        </template>

        <template v-else-if="form.rule_type === 'overcrowding'">
          <el-form-item label="区域">
            <el-select v-model="form.conditions.zone_name" placeholder="选择区域" style="width: 100%">
              <el-option v-for="z in zoneOptions" :key="z" :label="z" :value="z" />
            </el-select>
          </el-form-item>
        </template>
      </el-form>

      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" @click="handleSubmit">确定</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive } from 'vue'
import { Plus } from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import type { AlertRule, RuleType } from '@/types/alert'
import PageHeader from '@/components/common/PageHeader.vue'

const ruleTypeLabel: Record<RuleType, string> = {
  object_count: '目标计数',
  object_type: '目标类型',
  zone_intrusion: '区域入侵',
  line_crossing: '越线检测',
  overcrowding: '拥挤检测',
}

const cameraOptions = ref([
  { id: 'cam-1', name: '前门摄像头' },
  { id: 'cam-2', name: '后门摄像头' },
  { id: 'cam-3', name: '停车场A' },
  { id: 'cam-4', name: '大厅摄像头' },
  { id: 'cam-5', name: '仓库通道' },
])

const zoneOptions = ['禁区A', '禁区B', '仓库入口', '大厅中央']
const lineOptions = ['入口线1', '入口线2', '出口线1', '通道检测线']

/* @API:RULE_LIST — GET /api/v1/alert-rules/ */
const rules = ref<AlertRule[]>([
  {
    id: 'rule-1',
    name: '大厅人数超限',
    rule_type: 'object_count',
    cameras: ['cam-4'],
    conditions: { threshold: 50 },
    is_enabled: true,
    cooldown_seconds: 60,
    created_at: '2025-06-01T08:00:00Z',
    updated_at: '2025-06-01T08:00:00Z',
  },
  {
    id: 'rule-2',
    name: '仓库车辆检测',
    rule_type: 'object_type',
    cameras: ['cam-5', 'cam-3'],
    conditions: { object_type: 'vehicle', threshold: 1 },
    is_enabled: true,
    cooldown_seconds: 120,
    created_at: '2025-06-02T10:00:00Z',
    updated_at: '2025-06-02T10:00:00Z',
  },
  {
    id: 'rule-3',
    name: '禁区入侵告警',
    rule_type: 'zone_intrusion',
    cameras: ['cam-1', 'cam-2'],
    conditions: { zone_name: '禁区A', target_type: 'person' },
    is_enabled: true,
    cooldown_seconds: 30,
    created_at: '2025-06-03T14:00:00Z',
    updated_at: '2025-06-03T14:00:00Z',
  },
  {
    id: 'rule-4',
    name: '入口越线计数',
    rule_type: 'line_crossing',
    cameras: ['cam-1'],
    conditions: { line_name: '入口线1', count_threshold: 100 },
    is_enabled: false,
    cooldown_seconds: 300,
    created_at: '2025-06-04T09:00:00Z',
    updated_at: '2025-06-04T09:00:00Z',
  },
  {
    id: 'rule-5',
    name: '大厅拥挤检测',
    rule_type: 'overcrowding',
    cameras: ['cam-4'],
    conditions: { zone_name: '大厅中央' },
    is_enabled: true,
    cooldown_seconds: 60,
    created_at: '2025-06-05T11:00:00Z',
    updated_at: '2025-06-05T11:00:00Z',
  },
])

const dialogVisible = ref(false)
const isEdit = ref(false)
const editingId = ref<string | null>(null)

interface RuleForm {
  name: string
  rule_type: RuleType | ''
  cameras: string[]
  conditions: Record<string, any>
  is_enabled: boolean
  cooldown_seconds: number
}

const defaultForm = (): RuleForm => ({
  name: '',
  rule_type: '',
  cameras: [],
  conditions: {},
  is_enabled: true,
  cooldown_seconds: 60,
})

const form = reactive<RuleForm>(defaultForm())

function resetForm(data?: RuleForm) {
  const src = data ?? defaultForm()
  form.name = src.name
  form.rule_type = src.rule_type
  form.cameras = [...src.cameras]
  form.conditions = { ...src.conditions }
  form.is_enabled = src.is_enabled
  form.cooldown_seconds = src.cooldown_seconds
}

function openCreateDialog() {
  isEdit.value = false
  editingId.value = null
  resetForm()
  dialogVisible.value = true
}

function openEditDialog(row: AlertRule) {
  isEdit.value = true
  editingId.value = row.id
  resetForm({
    name: row.name,
    rule_type: row.rule_type,
    cameras: [...row.cameras],
    conditions: { ...row.conditions },
    is_enabled: row.is_enabled,
    cooldown_seconds: row.cooldown_seconds,
  })
  dialogVisible.value = true
}

function handleRuleTypeChange() {
  form.conditions = {}
}

function handleSubmit() {
  if (isEdit.value && editingId.value) {
    /* @API:RULE_UPDATE — PATCH /api/v1/alert-rules/{id}/ */
    const idx = rules.value.findIndex((r) => r.id === editingId.value)
    if (idx !== -1) {
      rules.value[idx] = {
        ...rules.value[idx],
        name: form.name,
        rule_type: form.rule_type as RuleType,
        cameras: [...form.cameras],
        conditions: { ...form.conditions },
        is_enabled: form.is_enabled,
        cooldown_seconds: form.cooldown_seconds,
        updated_at: new Date().toISOString(),
      }
    }
    ElMessage.success('规则已更新')
  } else {
    /* @API:RULE_CREATE — POST /api/v1/alert-rules/ */
    const now = new Date().toISOString()
    rules.value.push({
      id: `rule-${Date.now()}`,
      name: form.name,
      rule_type: form.rule_type as RuleType,
      cameras: [...form.cameras],
      conditions: { ...form.conditions },
      is_enabled: form.is_enabled,
      cooldown_seconds: form.cooldown_seconds,
      created_at: now,
      updated_at: now,
    })
    ElMessage.success('规则已创建')
  }
  dialogVisible.value = false
}

function handleToggle(row: AlertRule, val: boolean) {
  /* @API:RULE_TOGGLE — PATCH /api/v1/alert-rules/{id}/ */
  row.is_enabled = val
  ElMessage.success(val ? '已启用' : '已禁用')
}

function handleDelete(row: AlertRule) {
  ElMessageBox.confirm(`确定删除规则「${row.name}」？`, '确认删除', {
    type: 'warning',
    confirmButtonText: '删除',
    cancelButtonText: '取消',
  }).then(() => {
    /* @API:RULE_DELETE — DELETE /api/v1/alert-rules/{id}/ */
    rules.value = rules.value.filter((r) => r.id !== row.id)
    ElMessage.success('已删除')
  }).catch(() => {})
}
</script>
