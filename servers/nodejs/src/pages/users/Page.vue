<template>
  <div v-if="loaded" class="users-page">
    <header class="users-page__title">{{ t('users.title') }}</header>
    <section class="users-page__main">
      <UsersToolbar
        :has-selection="hasSelection"
        @add="openAdd"
        @remove="openBatchDelete"
        @refresh="onRefresh"
        @groups="groupsOpen = true"
      />
      <UsersTable
        :rows="rows"
        :can-change="canChange"
        :can-delete="canDelete"
        @selection-change="selectedIds = $event"
        @edit="openEdit"
        @remove="openRowDelete"
        @log="openLog"
      />
      <div class="users-page__pager">
        <UiPagination
          :page="page"
          :page-size="pageSize"
          :total="total"
          @update:page="setPage"
        />
      </div>
    </section>

    <UserFormDialog
      v-model="formOpen"
      :mode="formMode"
      :user="formUser"
      :groups="groupOptions"
      :saving="formSaving"
      :error="formError"
      @update:form="formDraft = $event"
      @save="onSaveUser"
    />

    <ConfirmDeleteDialog
      v-model="confirmOpen"
      :message="confirmMessage"
      :loading="confirmLoading"
      @confirm="onConfirmDelete"
    />

    <UserLogDialog v-model="logOpen" :name="logName" :entries="logEntries" />

    <GroupsLibraryDialog v-model="groupsOpen" @changed="onGroupsChanged" />
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { useI18n } from 'vue-i18n'
import { getMe } from '@/api/shell'
import type { UserItem, UserLogEntry } from '@/api/users'
import { ApiError } from '@/shared/http/client'
import UiPagination from '@/shared/ui/Pagination.vue'
import { toast } from '@/shared/ui/toast'
import GroupsLibraryDialog from '@/pages/groups/components/GroupsLibraryDialog.vue'
import ConfirmDeleteDialog from './components/ConfirmDeleteDialog.vue'
import UserFormDialog, { type UserFormDraft } from './components/UserFormDialog.vue'
import UserLogDialog from './components/UserLogDialog.vue'
import UsersTable from './components/UsersTable.vue'
import UsersToolbar from './components/UsersToolbar.vue'
import { useUsersPage } from './composables/useUsersPage'

const { t } = useI18n()
const pageApi = useUsersPage()
const {
  rows,
  total,
  page,
  pageSize,
  selectedIds,
  loaded,
  groupOptions,
  hasSelection,
  bootstrap,
  refreshList,
  refreshGroupOptions,
  setPage,
  saveUser,
  removeOne,
  removeSelected,
  loadLogs,
} = pageApi

const currentUserId = ref<string | null>(null)
const isSuperuser = ref(false)
const permissions = ref<string[]>([])
const groupsOpen = ref(false)

function hasPerm(codename: string): boolean {
  if (isSuperuser.value || permissions.value.includes('*')) {
    return true
  }
  return permissions.value.includes(codename)
}

const canChange = computed(() => hasPerm('users.change_user'))
const canDelete = computed(() => hasPerm('users.delete_user'))

const formOpen = ref(false)
const formMode = ref<'add' | 'edit'>('add')
const formUser = ref<UserItem | null>(null)
const formSaving = ref(false)
const formError = ref('')
const formDraft = ref<UserFormDraft>({
  username: '',
  password: '',
  is_active: true,
  group_ids: [],
})

const confirmOpen = ref(false)
const confirmLoading = ref(false)
const confirmMessage = ref('')
const confirmMode = ref<'batch' | 'one'>('batch')
const confirmUserId = ref<string | null>(null)

const logOpen = ref(false)
const logName = ref('')
const logEntries = ref<UserLogEntry[]>([])

onMounted(async () => {
  const me = await getMe()
  currentUserId.value = me.id
  isSuperuser.value = me.is_superuser
  permissions.value = [...me.permissions]
  await bootstrap()
})

async function onRefresh() {
  await refreshList()
}

async function onGroupsChanged() {
  await refreshGroupOptions()
  await refreshList()
}

function openAdd() {
  formMode.value = 'add'
  formUser.value = null
  formError.value = ''
  formOpen.value = true
}

function openEdit(row: UserItem) {
  formMode.value = 'edit'
  formUser.value = row
  formError.value = ''
  formOpen.value = true
}

async function onSaveUser() {
  formSaving.value = true
  formError.value = ''
  try {
    const draft = formDraft.value
    if (!draft.username.trim()) {
      formError.value = t('users.usernameRequired')
      return
    }
    if (formMode.value === 'add') {
      if (!draft.password) {
        formError.value = t('users.passwordRequired')
        return
      }
      if (!draft.group_ids.length) {
        formError.value = t('users.groupsRequired')
        return
      }
      await saveUser('add', null, {
        username: draft.username.trim(),
        password: draft.password,
        is_active: draft.is_active,
        group_ids: draft.group_ids,
      })
    } else {
      if (!draft.group_ids.length) {
        formError.value = t('users.groupsRequired')
        return
      }
      const body: {
        username: string
        is_active: boolean
        group_ids: string[]
        password?: string
      } = {
        username: draft.username.trim(),
        is_active: draft.is_active,
        group_ids: draft.group_ids,
      }
      if (draft.password) {
        body.password = draft.password
      }
      await saveUser('edit', formUser.value?.id ?? null, body)
    }
    formOpen.value = false
    toast.success(t('users.toastSaved'))
  } catch (err) {
    formError.value = err instanceof ApiError ? err.message : String(err)
  } finally {
    formSaving.value = false
  }
}

function openBatchDelete() {
  confirmMode.value = 'batch'
  confirmUserId.value = null
  confirmMessage.value = t('users.confirmRemoveMessage', {
    n: selectedIds.value.length,
  })
  confirmOpen.value = true
}

function openRowDelete(row: UserItem) {
  if (row.id === currentUserId.value) {
    toast.error(t('users.toastCannotDeleteSelf'))
    return
  }
  confirmMode.value = 'one'
  confirmUserId.value = row.id
  confirmMessage.value = t('users.confirmRemoveOne', { name: row.username })
  confirmOpen.value = true
}

async function onConfirmDelete() {
  confirmLoading.value = true
  try {
    if (confirmMode.value === 'one' && confirmUserId.value) {
      await removeOne(confirmUserId.value)
      toast.success(t('users.toastDeleted'))
    } else {
      const result = await removeSelected()
      if (result.skipped_count > 0) {
        toast.error(t('users.toastCannotDeleteSelf'))
      }
      if (result.deleted_count > 0) {
        toast.success(t('users.toastDeletedBatch', { n: result.deleted_count }))
      }
    }
    confirmOpen.value = false
  } catch (err) {
    toast.error(err instanceof ApiError ? err.message : String(err))
  } finally {
    confirmLoading.value = false
  }
}

async function openLog(row: UserItem) {
  logName.value = row.username
  logEntries.value = await loadLogs(row.id)
  logOpen.value = true
}
</script>

<style scoped>
.users-page {
  display: flex;
  flex-direction: column;
  height: 100%;
  min-height: 0;
  background: #f5f7fa;
}

.users-page__title {
  padding: 12px 16px;
  font-size: 18px;
  font-weight: 600;
  background: #fff;
  border-bottom: 1px solid #e4e7ed;
}

.users-page__main {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-width: 0;
  min-height: 0;
  background: #fff;
}

.users-page__pager {
  padding: 8px 16px 16px;
  display: flex;
  justify-content: flex-end;
}
</style>
