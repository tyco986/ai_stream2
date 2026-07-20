<template>
  <UiDialog
    :model-value="modelValue"
    :title="t('pipelines.gieTemplatesTitle')"
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
        <UiTableColumn prop="name" :label="t('pipelines.name')" min-width="140" />
        <UiTableColumn :label="t('pipelines.model')" min-width="120">
          <template #default="{ row }">
            {{ row.model_name || '—' }}
          </template>
        </UiTableColumn>
        <UiTableColumn :label="t('pipelines.classAttrs')" min-width="360">
          <template #default="{ row }">
            <div class="class-attrs">
              <div
                v-for="(line, index) in classAttrLines(row)"
                :key="`${row.id}-${index}`"
                class="class-attrs__line"
              >
                {{ line }}
              </div>
            </div>
          </template>
        </UiTableColumn>
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
      <div class="lib__pager">
        <UiPagination :page="page" :page-size="pageSize" :total="total" @update:page="setPage" />
      </div>
    </div>
  </UiDialog>

  <GieEditorDialog
    v-model="editorOpen"
    :mode="editorMode"
    :gie-id="editingId"
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
import { listModels, type ClassItem } from '@/api/models'
import {
  batchDeleteGieTemplates,
  deleteGieTemplate,
  listGieTemplates,
  type GieTemplateListItem,
} from '@/api/pipelines'
import { GieClassAttrsFormat } from '@/shared/gie/classAttrs'
import UiButton from '@/shared/ui/Button.vue'
import UiDialog from '@/shared/ui/Dialog.vue'
import UiPagination from '@/shared/ui/Pagination.vue'
import UiTable from '@/shared/ui/Table.vue'
import UiTableColumn from '@/shared/ui/TableColumn.vue'
import { toast } from '@/shared/ui/toast'
import { createLibraryDeleteConfirm } from '../composables/useLibraryDeleteConfirm'
import ConfirmDeleteDialog from './ConfirmDeleteDialog.vue'
import GieEditorDialog from './GieEditorDialog.vue'
import PipelineRefsCell from './PipelineRefsCell.vue'
import TemplateRowOps from './TemplateRowOps.vue'

const props = defineProps<{
  modelValue: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
}>()

const { t } = useI18n()
const rows = ref<GieTemplateListItem[]>([])
const total = ref(0)
const page = ref(1)
const pageSize = ref(20)
const selectedIds = ref<string[]>([])
const editorOpen = ref(false)
const editorMode = ref<'add' | 'edit'>('add')
const editingId = ref<string | null>(null)
const modelClassesById = ref<Record<string, ClassItem[]>>({})

watch(
  () => props.modelValue,
  async (open) => {
    if (open) {
      await refresh()
    }
  },
)

async function refresh() {
  const [gieResult, modelResult] = await Promise.all([
    listGieTemplates({ page: page.value, page_size: pageSize.value }),
    listModels({ page: 1, page_size: 100 }),
  ])
  rows.value = gieResult.items
  total.value = gieResult.total
  modelClassesById.value = Object.fromEntries(
    modelResult.items.map((item) => [item.id, item.classes ?? []]),
  )
}

async function setPage(value: number) {
  page.value = value
  await refresh()
}

function onSelectionChange(selected: GieTemplateListItem[]) {
  selectedIds.value = selected.map((row) => row.id)
}

function classAttrLines(row: GieTemplateListItem): string[] {
  if (!row.class_attrs?.length) {
    return ['—']
  }
  return new GieClassAttrsFormat(modelClassesById.value[row.model_id] ?? []).summaryLines(
    row.class_attrs,
  )
}

function openAdd() {
  editorMode.value = 'add'
  editingId.value = null
  editorOpen.value = true
}

function openEdit(row: GieTemplateListItem) {
  editorMode.value = 'edit'
  editingId.value = row.id
  editorOpen.value = true
}

async function onSaved() {
  editorOpen.value = false
  toast.success(t('pipelines.toastGieSaved'))
  await refresh()
}

async function deleteIds(ids: string[]) {
  if (ids.length === 1) {
    await deleteGieTemplate(ids[0])
  } else {
    await batchDeleteGieTemplates({ ids })
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
  messageForCount: (n) => t('pipelines.deleteGieTemplates', { n }),
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

.lib__pager {
  display: flex;
  justify-content: center;
  padding-top: 12px;
}

.class-attrs {
  display: flex;
  flex-direction: column;
  gap: 2px;
  line-height: 1.4;
  white-space: normal;
}

.class-attrs__line {
  word-break: break-word;
}
</style>
