<template>
  <UiDialog
    :model-value="modelValue"
    :title="t('groups.title')"
    width="900px"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <div class="lib">
      <div class="lib__bar">
        <UiButton v-if="canAdd" type="primary" @click="openAdd">{{ t('groups.add') }}</UiButton>
        <UiButton
          v-if="canDelete"
          type="danger"
          :disabled="!selectedIds.length"
          @click="requestDeleteSelected"
        >
          {{ t('groups.remove') }}
        </UiButton>
        <UiInput
          class="lib__search"
          :model-value="search"
          :placeholder="t('groups.search')"
          clearable
          @update:model-value="search = $event"
        />
      </div>
      <UiTable :data="filteredRows" row-key="id" @selection-change="onSelectionChange">
        <UiTableColumn type="selection" width="48" />
        <UiTableColumn :label="t('groups.name')" min-width="160">
          <template #default="{ row }">
            {{ row.name }}
          </template>
        </UiTableColumn>
        <UiTableColumn :label="t('groups.members')" width="110">
          <template #default="{ row }">
            {{ row.member_count }}
          </template>
        </UiTableColumn>
        <UiTableColumn :label="t('groups.updated')" width="120">
          <template #default>
            {{ t('groups.dash') }}
          </template>
        </UiTableColumn>
        <UiTableColumn :label="t('groups.operations')" width="110">
          <template #default="{ row }">
            <div class="ops">
              <button
                v-if="canChange"
                type="button"
                class="ops__btn"
                :title="t('groups.edit')"
                @click="openEdit(row)"
              >
                <UiIcon name="edit" :size="16" />
              </button>
              <button
                v-if="canDelete"
                type="button"
                class="ops__btn is-danger"
                :title="t('groups.remove')"
                @click="requestDelete([row])"
              >
                <UiIcon name="delete" :size="16" />
              </button>
            </div>
          </template>
        </UiTableColumn>
      </UiTable>
    </div>
  </UiDialog>

  <GroupEditorDialog
    v-model="editorOpen"
    :mode="editorMode"
    :group="editingGroup"
    :catalog="catalog"
    :saving="editorSaving"
    @save="onSaveGroup"
  />

  <ConfirmDeleteDialog
    v-model="confirmOpen"
    :message="confirmMessage"
    :loading="confirmLoading"
    @confirm="onConfirmDelete"
  />
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import { getMe } from '@/api/shell'
import type { GroupItem } from '@/api/users'
import { ApiError } from '@/shared/http/client'
import UiButton from '@/shared/ui/Button.vue'
import UiDialog from '@/shared/ui/Dialog.vue'
import UiIcon from '@/shared/ui/Icon.vue'
import UiInput from '@/shared/ui/Input.vue'
import UiTable from '@/shared/ui/Table.vue'
import UiTableColumn from '@/shared/ui/TableColumn.vue'
import { toast } from '@/shared/ui/toast'
import { useGroupsPage } from '../composables/useGroupsPage'
import ConfirmDeleteDialog from './ConfirmDeleteDialog.vue'
import GroupEditorDialog from './GroupEditorDialog.vue'

const props = defineProps<{
  modelValue: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  changed: []
}>()

const { t } = useI18n()
const {
  rows,
  catalog,
  selectedIds,
  refreshList,
  refreshCatalog,
  saveGroup,
  removeOne,
  removeSelected,
} = useGroupsPage()

const isSuperuser = ref(false)
const permissions = ref<string[]>([])
const search = ref('')

const editorOpen = ref(false)
const editorMode = ref<'add' | 'edit'>('add')
const editingGroup = ref<GroupItem | null>(null)
const editorSaving = ref(false)

const confirmOpen = ref(false)
const confirmLoading = ref(false)
const confirmMessage = ref('')
const confirmIds = ref<string[]>([])

const canAdd = computed(() => hasPerm('users.add_group'))
const canChange = computed(() => hasPerm('users.change_group'))
const canDelete = computed(() => hasPerm('users.delete_group'))

const filteredRows = computed(() => {
  const keyword = search.value.trim().toLowerCase()
  if (!keyword) {
    return rows.value
  }
  return rows.value.filter((row) => row.name.toLowerCase().includes(keyword))
})

watch(
  () => props.modelValue,
  async (open) => {
    if (!open) {
      return
    }
    const me = await getMe()
    isSuperuser.value = me.is_superuser
    permissions.value = [...me.permissions]
    search.value = ''
    selectedIds.value = []
    await Promise.all([refreshList(), refreshCatalog()])
  },
)

function hasPerm(codename: string): boolean {
  if (isSuperuser.value || permissions.value.includes('*')) {
    return true
  }
  return permissions.value.includes(codename)
}

function onSelectionChange(selected: GroupItem[]) {
  selectedIds.value = selected.map((row) => row.id)
}

function openAdd() {
  editorMode.value = 'add'
  editingGroup.value = null
  editorOpen.value = true
}

function openEdit(row: GroupItem) {
  editorMode.value = 'edit'
  editingGroup.value = row
  editorOpen.value = true
}

async function onSaveGroup(payload: { name: string; permission_ids: number[] }) {
  editorSaving.value = true
  try {
    await saveGroup(editorMode.value, editingGroup.value?.id ?? null, payload)
    editorOpen.value = false
    emit('changed')
    toast.success(
      editorMode.value === 'add' ? t('groups.toastCreated') : t('groups.toastSaved'),
    )
  } catch (err) {
    toast.error(err instanceof ApiError ? err.message : String(err))
  } finally {
    editorSaving.value = false
  }
}

function requestDeleteSelected() {
  const selected = rows.value.filter((row) => selectedIds.value.includes(row.id))
  requestDelete(selected)
}

function requestDelete(groups: GroupItem[]) {
  if (!groups.length) {
    return
  }
  confirmIds.value = groups.map((row) => row.id)
  if (groups.length === 1) {
    confirmMessage.value = t('groups.confirmDeleteGroup', { name: groups[0].name })
  } else {
    confirmMessage.value = t('groups.confirmDeleteSelected', { n: groups.length })
  }
  confirmOpen.value = true
}

async function onConfirmDelete() {
  if (!confirmIds.value.length) {
    return
  }
  confirmLoading.value = true
  try {
    if (confirmIds.value.length === 1) {
      await removeOne(confirmIds.value[0])
    } else {
      selectedIds.value = [...confirmIds.value]
      await removeSelected()
    }
    confirmOpen.value = false
    confirmIds.value = []
    emit('changed')
    toast.success(t('groups.toastDeleted'))
  } catch (err) {
    toast.error(err instanceof ApiError ? err.message : String(err))
  } finally {
    confirmLoading.value = false
  }
}
</script>

<style scoped>
.lib {
  display: flex;
  flex-direction: column;
  gap: 12px;
  min-height: 360px;
}

.lib__bar {
  display: flex;
  align-items: center;
  gap: 12px;
}

.lib__search {
  margin-left: auto;
  width: 220px;
}

.ops {
  display: flex;
  align-items: center;
  gap: 2px;
}

.ops__btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 28px;
  height: 28px;
  border: none;
  border-radius: 4px;
  background: transparent;
  cursor: pointer;
  color: #606266;
}

.ops__btn.is-danger {
  color: #f56c6c;
}

.ops__btn:hover {
  background: #f5f7fa;
}
</style>
