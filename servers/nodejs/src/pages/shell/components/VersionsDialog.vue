<template>
  <UiDialog
    :model-value="modelValue"
    :title="t('shell.versions')"
    width="840px"
    @update:model-value="onVisibleChange"
  >
    <div class="versions">
      <div class="versions__toolbar">
        <UiButton @click="backupOpen = true">{{ t('shell.backup') }}</UiButton>
        <UiButton :disabled="!selectedId" @click="onImportClick">
          {{ t('shell.import') }}
        </UiButton>
        <input
          ref="fileInput"
          type="file"
          class="versions__file"
          @change="onFilePicked"
        />
      </div>

      <UiTable
        :data="items"
        row-key="id"
        class="versions__table"
        :row-class-name="tableRowClass"
        @row-click="onRowClick"
      >
        <UiTableColumn :label="t('shell.version')" prop="version" width="100">
          <template #default="{ row }">
            <span
              class="versions__cell"
              :class="rowClass(row)"
            >
              {{ row.version }}
              <span v-if="row.is_current" class="versions__current-dot" />
            </span>
          </template>
        </UiTableColumn>
        <UiTableColumn
          :label="t('shell.description')"
          prop="description"
          min-width="160"
        >
          <template #default="{ row }">
            <span :class="rowClass(row)">
              {{ row.description || '—' }}
            </span>
          </template>
        </UiTableColumn>
        <UiTableColumn
          :label="t('shell.createTime')"
          prop="created_at"
          min-width="180"
        >
          <template #default="{ row }">
            <span :class="rowClass(row)">
              {{ formatTime(row.created_at) }}
            </span>
          </template>
        </UiTableColumn>
        <UiTableColumn :label="t('shell.operations')" width="120">
          <template #default="{ row }">
            <button
              type="button"
              class="versions__export"
              @click.stop="onExport(row)"
            >
              {{ t('shell.export') }}
            </button>
          </template>
        </UiTableColumn>
      </UiTable>
    </div>

    <template #footer>
      <UiButton @click="onCancel">{{ t('shell.cancel') }}</UiButton>
      <UiButton
        type="primary"
        :disabled="!canApply"
        :loading="busy"
        @click="onApplyClick"
      >
        {{ t('shell.apply') }}
      </UiButton>
    </template>
  </UiDialog>

  <BackupDialog
    v-model="backupOpen"
    :saving="busy"
    @save="onBackupSave"
  />

  <AuthorizationDialog
    v-model="authOpen"
    :verifying="busy"
    @verify="onAuthVerify"
    @cancel="pendingAction = null"
  />
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import {
  applyVersion,
  createVersion,
  exportVersion,
  importVersion,
  listVersions,
  type SiteConfigVersion,
} from '@/api/siteConfig'
import { ApiError } from '@/shared/http/client'
import UiButton from '@/shared/ui/Button.vue'
import UiDialog from '@/shared/ui/Dialog.vue'
import UiTable from '@/shared/ui/Table.vue'
import UiTableColumn from '@/shared/ui/TableColumn.vue'
import { toast } from '@/shared/ui/toast'
import AuthorizationDialog from './AuthorizationDialog.vue'
import BackupDialog from './BackupDialog.vue'

const props = defineProps<{
  modelValue: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  changed: []
}>()

const { t } = useI18n()
const items = ref<SiteConfigVersion[]>([])
const selectedId = ref<string | null>(null)
const backupOpen = ref(false)
const authOpen = ref(false)
const busy = ref(false)
const fileInput = ref<HTMLInputElement | null>(null)
const pendingFile = ref<File | null>(null)
const pendingAction = ref<'import' | 'apply' | null>(null)

const selected = computed(() =>
  items.value.find((item) => item.id === selectedId.value) ?? null,
)

const canApply = computed(() => {
  const row = selected.value
  return Boolean(row && !row.is_current)
})

watch(
  () => props.modelValue,
  async (open) => {
    if (open) {
      selectedId.value = null
      await refresh()
    }
  },
)

async function refresh() {
  const data = await listVersions()
  items.value = data.items
}

function rowClass(row: SiteConfigVersion): string {
  const classes: string[] = []
  if (row.is_current) {
    classes.push('versions__row--current')
  }
  if (row.id === selectedId.value) {
    classes.push('versions__row--selected')
  }
  return classes.join(' ')
}

function tableRowClass(data: { row: SiteConfigVersion }): string {
  return rowClass(data.row)
}

function formatTime(iso: string): string {
  const date = new Date(iso)
  if (Number.isNaN(date.getTime())) {
    return iso
  }
  const pad = (n: number) => String(n).padStart(2, '0')
  return `${date.getFullYear()}-${pad(date.getMonth() + 1)}-${pad(date.getDate())} ${pad(date.getHours())}:${pad(date.getMinutes())}`
}

function onVisibleChange(value: boolean) {
  emit('update:modelValue', value)
}

function onCancel() {
  emit('update:modelValue', false)
}

function onRowClick(row: SiteConfigVersion) {
  selectedId.value = row.id
}

async function onBackupSave(payload: {
  version: string
  description: string
}) {
  busy.value = true
  try {
    await createVersion(payload)
    toast.success(t('shell.toastVersionSaved'))
    backupOpen.value = false
    await refresh()
    emit('changed')
  } catch (err) {
    const message =
      err instanceof ApiError ? err.message : t('shell.operationFailed')
    toast.error(message)
  }
  busy.value = false
}

function onImportClick() {
  if (!selectedId.value) {
    return
  }
  fileInput.value?.click()
}

function onFilePicked(event: Event) {
  const input = event.target as HTMLInputElement
  const file = input.files?.[0] ?? null
  input.value = ''
  if (!file || !selectedId.value) {
    return
  }
  pendingFile.value = file
  pendingAction.value = 'import'
  authOpen.value = true
}

function onApplyClick() {
  if (!canApply.value) {
    return
  }
  pendingFile.value = null
  pendingAction.value = 'apply'
  authOpen.value = true
}

async function onAuthVerify(ticket: string) {
  const action = pendingAction.value
  if (!action || !selectedId.value) {
    return
  }
  busy.value = true
  try {
    if (action === 'import') {
      const file = pendingFile.value
      if (!file) {
        throw new ApiError(400, 'Invalid package')
      }
      await importVersion(selectedId.value, file, ticket)
      toast.success(t('shell.toastVersionImported'))
    }
    if (action === 'apply') {
      await applyVersion(selectedId.value, ticket)
      toast.success(t('shell.toastVersionApplied'))
    }
    authOpen.value = false
    pendingAction.value = null
    pendingFile.value = null
    await refresh()
    emit('changed')
  } catch (err) {
    const message =
      err instanceof ApiError ? err.message : t('shell.operationFailed')
    toast.error(message)
  }
  busy.value = false
}

async function onExport(row: SiteConfigVersion) {
  const filename = `site-config-v${row.version}.bin`
  busy.value = true
  try {
    const blob = await exportVersion(row.id)
    const url = URL.createObjectURL(blob)
    const anchor = document.createElement('a')
    anchor.href = url
    anchor.download = filename
    anchor.rel = 'noopener'
    anchor.style.display = 'none'
    document.body.appendChild(anchor)
    anchor.click()
    anchor.remove()
    window.setTimeout(() => URL.revokeObjectURL(url), 2000)
    toast.success(t('shell.toastSiteConfigExported'))
  } catch (err) {
    const message =
      err instanceof ApiError ? err.message : t('shell.operationFailed')
    toast.error(message)
  }
  busy.value = false
}
</script>

<style scoped>
.versions__toolbar {
  display: flex;
  gap: 8px;
  margin-bottom: 12px;
}

.versions__file {
  display: none;
}

.versions__table :deep(.el-table__row) {
  cursor: pointer;
}

.versions__table :deep(.versions__row--current) {
  --el-table-tr-bg-color: #f0f9eb;
}

.versions__table :deep(.versions__row--selected) {
  --el-table-tr-bg-color: #ecf5ff;
}

.versions__table :deep(.versions__row--current.versions__row--selected) {
  --el-table-tr-bg-color: #ecf5ff;
}

.versions__current-dot {
  display: inline-block;
  width: 8px;
  height: 8px;
  margin-left: 6px;
  border-radius: 50%;
  background: #67c23a;
  vertical-align: middle;
}

.versions__row--current {
  color: #67c23a;
  font-weight: 600;
}

.versions__row--selected {
  color: #409eff;
  font-weight: 600;
}

.versions__row--current.versions__row--selected {
  color: #409eff;
}

.versions__export {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  height: 32px;
  padding: 0 15px;
  border: 1px solid #dcdfe6;
  border-radius: 4px;
  background: #fff;
  color: #606266;
  font-size: 14px;
  cursor: pointer;
  box-sizing: border-box;
}

.versions__export:hover {
  color: #409eff;
  border-color: #c6e2ff;
  background: #ecf5ff;
}
</style>
