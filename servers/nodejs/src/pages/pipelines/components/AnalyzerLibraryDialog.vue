<template>
  <UiDialog
    :model-value="modelValue"
    :title="t('pipelines.analyzerTemplatesTitle')"
    width="1200px"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <div class="lib">
      <div class="lib__actions">
        <UiButton type="primary" @click="openAdd">{{ t('pipelines.add') }}</UiButton>
        <UiButton
          type="danger"
          :disabled="!selectedIds.length"
          @click="requestDelete([...selectedIds])"
        >
          {{ t('pipelines.remove') }}
        </UiButton>
      </div>
      <UiTable :data="rows" row-key="id" @selection-change="onSelectionChange">
        <UiTableColumn type="selection" />
        <UiTableColumn prop="name" :label="t('pipelines.name')" min-width="120" />
        <UiTableColumn
          prop="roi_filtering_count"
          :label="t('pipelines.roiFilteringCount')"
          width="110"
        />
        <UiTableColumn
          prop="overcrowding_count"
          :label="t('pipelines.overcrowdingCount')"
          width="110"
        />
        <UiTableColumn
          prop="line_crossing_count"
          :label="t('pipelines.lineCrossingCount')"
          width="110"
        />
        <UiTableColumn
          prop="direction_detection_count"
          :label="t('pipelines.directionDetectCount')"
          width="120"
        />
        <UiTableColumn :label="t('pipelines.pipelines')" min-width="160">
          <template #default="{ row }">
            <PipelineRefsCell :pipelines="row.pipelines" />
          </template>
        </UiTableColumn>
        <UiTableColumn :label="t('pipelines.operations')" width="110">
          <template #default="{ row }">
            <TemplateRowOps @edit="openEdit(row)" @remove="requestDelete([row.id])" />
          </template>
        </UiTableColumn>
      </UiTable>
    </div>
  </UiDialog>

  <AnalyzerEditorDialog
    v-model="editorOpen"
    :mode="editorMode"
    :analyzer-id="editingId"
    @saved="onSaved"
  />

  <ConfirmDeleteDialog
    v-model="confirmOpen"
    :message="confirmMessage"
    :hint="confirmHint"
    :loading="confirmLoading"
    @confirm="confirmDelete"
  />
</template>

<script setup lang="ts">
import { ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import {
  batchDeleteAnalyzerTemplates,
  deleteAnalyzerTemplate,
  listAnalyzerTemplates,
  type AnalyzerTemplateListItem,
} from '@/api/pipelines'
import UiButton from '@/shared/ui/Button.vue'
import UiDialog from '@/shared/ui/Dialog.vue'
import UiTable from '@/shared/ui/Table.vue'
import UiTableColumn from '@/shared/ui/TableColumn.vue'
import { toast } from '@/shared/ui/toast'
import { createLibraryDeleteConfirm } from '../composables/useLibraryDeleteConfirm'
import AnalyzerEditorDialog from './AnalyzerEditorDialog.vue'
import ConfirmDeleteDialog from './ConfirmDeleteDialog.vue'
import PipelineRefsCell from './PipelineRefsCell.vue'
import TemplateRowOps from './TemplateRowOps.vue'

const props = defineProps<{
  modelValue: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
}>()

const { t } = useI18n()
const rows = ref<AnalyzerTemplateListItem[]>([])
const selectedIds = ref<string[]>([])
const editorOpen = ref(false)
const editorMode = ref<'add' | 'edit'>('add')
const editingId = ref<string | null>(null)

watch(
  () => props.modelValue,
  async (open) => {
    if (open) {
      await refresh()
    }
  },
)

async function refresh() {
  const result = await listAnalyzerTemplates({ page: 1, page_size: 100 })
  rows.value = result.items
}

function onSelectionChange(selected: AnalyzerTemplateListItem[]) {
  selectedIds.value = selected.map((row) => row.id)
}

function openAdd() {
  editorMode.value = 'add'
  editingId.value = null
  editorOpen.value = true
}

function openEdit(row: AnalyzerTemplateListItem) {
  editorMode.value = 'edit'
  editingId.value = row.id
  editorOpen.value = true
}

async function onSaved() {
  editorOpen.value = false
  toast.success(t('pipelines.toastAnalyzerSaved'))
  await refresh()
}

async function deleteIds(ids: string[]) {
  if (ids.length === 1) {
    await deleteAnalyzerTemplate(ids[0])
  } else {
    await batchDeleteAnalyzerTemplates({ ids })
  }
  selectedIds.value = []
  await refresh()
}

const {
  confirmOpen,
  confirmLoading,
  confirmMessage,
  confirmHint,
  requestDelete,
  confirmDelete,
} = createLibraryDeleteConfirm({
  messageForCount: (n) => t('pipelines.deleteAnalyzerTemplates', { n }),
  hint: t('pipelines.cannotUndo'),
  deleteIds,
})
</script>

<style scoped>
.lib__actions {
  display: flex;
  gap: 8px;
  margin-bottom: 12px;
}
</style>
