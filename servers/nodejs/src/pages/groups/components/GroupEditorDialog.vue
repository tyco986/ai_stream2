<template>
  <UiDialog
    :model-value="modelValue"
    :title="mode === 'add' ? t('groups.addGroup') : t('groups.editGroup')"
    width="720px"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <div class="editor">
      <label class="editor__row">
        <span>{{ t('groups.name') }}</span>
        <UiInput
          :model-value="name"
          :invalid="nameInvalid"
          @update:model-value="onNameChange"
        />
      </label>
      <div class="editor__perms">
        <header class="editor__perms-title">{{ t('groups.permissions') }}</header>
        <table class="perm-table">
          <thead>
            <tr>
              <th class="perm-table__select" />
              <th class="perm-table__module">{{ t('groups.module') }}</th>
              <th v-for="col in actionColumns" :key="col">{{ actionLabel(col) }}</th>
            </tr>
          </thead>
          <tbody>
            <tr class="perm-table__all">
              <td class="perm-table__select">
                <UiCheckbox
                  :model-value="allChecked"
                  :indeterminate="allIndeterminate"
                  @update:model-value="setPermissionIds(allPermissionIds, $event)"
                />
              </td>
              <td class="perm-table__module">{{ t('groups.all') }}</td>
              <td v-for="col in actionColumns" :key="`all-${col}`">
                <UiCheckbox
                  v-if="permissionIdsForColumn(col).length"
                  :model-value="columnChecked(col)"
                  :indeterminate="columnIndeterminate(col)"
                  @update:model-value="setPermissionIds(permissionIdsForColumn(col), $event)"
                />
                <span v-else class="perm-na">{{ t('groups.dash') }}</span>
              </td>
            </tr>
            <tr v-for="mod in catalog.modules" :key="mod.key">
              <td class="perm-table__select">
                <UiCheckbox
                  :model-value="rowChecked(mod.key)"
                  :indeterminate="rowIndeterminate(mod.key)"
                  @update:model-value="
                    setPermissionIds(permissionIdsForRow(mod.key), $event)
                  "
                />
              </td>
              <td class="perm-table__module">{{ mod.label }}</td>
              <td v-for="col in actionColumns" :key="`${mod.key}-${col}`">
                <UiCheckbox
                  v-if="actionByModule(mod.key, col)"
                  :model-value="checkedIds.has(actionByModule(mod.key, col)!.permission_id)"
                  @update:model-value="
                    togglePerm(actionByModule(mod.key, col)!.permission_id, $event)
                  "
                />
                <span v-else class="perm-na">{{ t('groups.dash') }}</span>
              </td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>
    <template #footer>
      <UiButton @click="emit('update:modelValue', false)">{{ t('groups.cancel') }}</UiButton>
      <UiButton type="primary" :loading="saving" @click="onSave">
        {{ t('groups.save') }}
      </UiButton>
    </template>
  </UiDialog>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import type { GroupItem, PermissionCatalog } from '@/api/users'
import UiButton from '@/shared/ui/Button.vue'
import UiCheckbox from '@/shared/ui/Checkbox.vue'
import UiDialog from '@/shared/ui/Dialog.vue'
import UiInput from '@/shared/ui/Input.vue'

const props = defineProps<{
  modelValue: boolean
  mode: 'add' | 'edit'
  group: GroupItem | null
  catalog: PermissionCatalog
  saving: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  save: [payload: { name: string; permission_ids: number[] }]
}>()

const { t } = useI18n()
const name = ref('')
const nameInvalid = ref(false)
const checkedIds = ref<Set<number>>(new Set())

const actionColumns = computed(() => {
  const keys = new Set<string>()
  for (const mod of props.catalog.modules) {
    for (const action of mod.actions) {
      keys.add(action.key)
    }
  }
  const preferred = ['view', 'add', 'change', 'delete']
  return preferred.filter((key) => keys.has(key))
})

const allPermissionIds = computed(() =>
  props.catalog.modules.flatMap((mod) => mod.actions.map((action) => action.permission_id)),
)

const allChecked = computed(() => isAllChecked(allPermissionIds.value))
const allIndeterminate = computed(() => isIndeterminate(allPermissionIds.value))

watch(
  () => [props.modelValue, props.mode, props.group] as const,
  ([open]) => {
    if (!open) {
      return
    }
    nameInvalid.value = false
    if (props.mode === 'edit' && props.group) {
      name.value = props.group.name
      checkedIds.value = new Set(props.group.permission_ids)
    } else {
      name.value = ''
      checkedIds.value = new Set()
    }
  },
)

function onNameChange(value: string) {
  name.value = value
  nameInvalid.value = false
}

function actionByModule(moduleKey: string, actionKey: string) {
  const mod = props.catalog.modules.find((item) => item.key === moduleKey)
  return mod?.actions.find((action) => action.key === actionKey) ?? null
}

function actionLabel(actionKey: string): string {
  return actionKey.charAt(0).toUpperCase() + actionKey.slice(1)
}

function permissionIdsForColumn(actionKey: string): number[] {
  return props.catalog.modules.flatMap((mod) => {
    const action = mod.actions.find((item) => item.key === actionKey)
    return action ? [action.permission_id] : []
  })
}

function permissionIdsForRow(moduleKey: string): number[] {
  const mod = props.catalog.modules.find((item) => item.key === moduleKey)
  return mod ? mod.actions.map((action) => action.permission_id) : []
}

function isAllChecked(ids: number[]): boolean {
  return ids.length > 0 && ids.every((id) => checkedIds.value.has(id))
}

function isIndeterminate(ids: number[]): boolean {
  const checkedCount = ids.filter((id) => checkedIds.value.has(id)).length
  return checkedCount > 0 && checkedCount < ids.length
}

function columnChecked(actionKey: string): boolean {
  return isAllChecked(permissionIdsForColumn(actionKey))
}

function columnIndeterminate(actionKey: string): boolean {
  return isIndeterminate(permissionIdsForColumn(actionKey))
}

function rowChecked(moduleKey: string): boolean {
  return isAllChecked(permissionIdsForRow(moduleKey))
}

function rowIndeterminate(moduleKey: string): boolean {
  return isIndeterminate(permissionIdsForRow(moduleKey))
}

function setPermissionIds(ids: number[], checked: boolean) {
  const next = new Set(checkedIds.value)
  for (const id of ids) {
    if (checked) {
      next.add(id)
    } else {
      next.delete(id)
    }
  }
  checkedIds.value = next
}

function togglePerm(permissionId: number, checked: boolean) {
  setPermissionIds([permissionId], checked)
}

function onSave() {
  const trimmed = name.value.trim()
  nameInvalid.value = !trimmed
  if (nameInvalid.value) {
    return
  }
  emit('save', {
    name: trimmed,
    permission_ids: [...checkedIds.value],
  })
}
</script>

<style scoped>
.editor {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.editor__row {
  display: grid;
  grid-template-columns: 90px 1fr;
  align-items: center;
  gap: 8px;
}

.editor__perms-title {
  font-weight: 600;
  margin-bottom: 8px;
}

.perm-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 13px;
}

.perm-table th,
.perm-table td {
  border-bottom: 1px solid #ebeef5;
  padding: 8px 6px;
  text-align: center;
  vertical-align: middle;
}

.perm-table__select {
  width: 48px;
}

.perm-table__select :deep(.el-checkbox) {
  height: auto;
  margin: 0;
}

.perm-table th.perm-table__module,
.perm-table td.perm-table__module {
  text-align: left;
}

.perm-table__all {
  background: #fafafa;
}

.perm-na {
  color: #c0c4cc;
}
</style>
