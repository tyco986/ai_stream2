<template>
  <div class="users-table">
    <UiTable :data="rows" row-key="id" @selection-change="onSelectionChange">
      <UiTableColumn type="selection" width="48" />
      <UiTableColumn :label="t('users.name')" min-width="120">
        <template #default="{ row }">
          <span :class="{ 'is-inactive': !row.is_active }">{{ row.username }}</span>
        </template>
      </UiTableColumn>
      <UiTableColumn :label="t('users.groupsCol')" min-width="160">
        <template #default="{ row }">
          {{
            row.groups.length
              ? row.groups.map((group: { name: string }) => group.name).join(', ')
              : t('users.dash')
          }}
        </template>
      </UiTableColumn>
      <UiTableColumn :label="t('users.lastAction')" min-width="160">
        <template #default="{ row }">
          {{ formatLastAction(row.last_action_at, row.last_action_label) }}
        </template>
      </UiTableColumn>
      <UiTableColumn :label="t('users.operations')" width="120">
        <template #default="{ row }">
          <div class="ops">
            <button
              v-if="canChange"
              type="button"
              class="ops__btn"
              :title="t('users.edit')"
              @click="emit('edit', row)"
            >
              <UiIcon name="edit" :size="16" />
            </button>
            <button
              v-if="canDelete"
              type="button"
              class="ops__btn"
              :title="t('users.remove')"
              @click="emit('remove', row)"
            >
              <UiIcon name="delete" :size="16" />
            </button>
            <button
              type="button"
              class="ops__btn"
              :title="t('users.log')"
              @click="emit('log', row)"
            >
              <UiIcon name="document" :size="16" />
            </button>
          </div>
        </template>
      </UiTableColumn>
    </UiTable>
  </div>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'
import { formatLastAction, type UserItem } from '@/api/users'
import UiIcon from '@/shared/ui/Icon.vue'
import UiTable from '@/shared/ui/Table.vue'
import UiTableColumn from '@/shared/ui/TableColumn.vue'

defineProps<{
  rows: UserItem[]
  canChange: boolean
  canDelete: boolean
}>()

const emit = defineEmits<{
  'selection-change': [ids: string[]]
  edit: [row: UserItem]
  remove: [row: UserItem]
  log: [row: UserItem]
}>()

const { t } = useI18n()

function onSelectionChange(selected: UserItem[]) {
  emit(
    'selection-change',
    selected.map((row) => row.id),
  )
}
</script>

<style scoped>
.users-table {
  flex: 1;
  min-height: 0;
  overflow: auto;
  padding: 0 16px;
}

.is-inactive {
  color: #909399;
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

.ops__btn:hover {
  background: #f5f7fa;
}
</style>
