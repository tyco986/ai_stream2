import { computed, ref } from 'vue'
import {
  batchDeleteUsers,
  createUser,
  deleteUser,
  listGroups,
  listUserLogs,
  listUsers,
  patchUser,
  type CreateUserBody,
  type GroupItem,
  type PatchUserBody,
  type UserItem,
  type UserLogEntry,
} from '@/api/users'

const rows = ref<UserItem[]>([])
const total = ref(0)
const page = ref(1)
const pageSize = ref(20)
const search = ref('')
const selectedIds = ref<string[]>([])
const loaded = ref(false)
const listLoading = ref(false)
const groupOptions = ref<GroupItem[]>([])

export function useUsersPage() {
  const hasSelection = computed(() => selectedIds.value.length > 0)

  async function refreshList() {
    listLoading.value = true
    const result = await listUsers({
      search: search.value.trim() || undefined,
      page: page.value,
      page_size: pageSize.value,
    })
    rows.value = result.items
    total.value = result.total
    page.value = result.page
    const visible = new Set(result.items.map((item) => item.id))
    selectedIds.value = selectedIds.value.filter((id) => visible.has(id))
    listLoading.value = false
  }

  async function refreshGroupOptions() {
    const result = await listGroups()
    groupOptions.value = result.items
  }

  async function bootstrap() {
    loaded.value = false
    await Promise.all([refreshList(), refreshGroupOptions()])
    loaded.value = true
  }

  async function setPage(value: number) {
    page.value = value
    await refreshList()
  }

  async function setSearch(value: string) {
    search.value = value
    page.value = 1
    await refreshList()
  }

  async function saveUser(
    mode: 'add' | 'edit',
    userId: string | null,
    body: CreateUserBody | PatchUserBody,
  ) {
    if (mode === 'add') {
      await createUser(body as CreateUserBody)
    } else if (userId) {
      await patchUser(userId, body as PatchUserBody)
    }
    await refreshList()
  }

  async function removeOne(userId: string) {
    await deleteUser(userId)
    await refreshList()
  }

  async function removeSelected() {
    const ids = [...selectedIds.value]
    const result = await batchDeleteUsers({ user_ids: ids })
    selectedIds.value = []
    await refreshList()
    return result
  }

  async function loadLogs(userId: string): Promise<UserLogEntry[]> {
    const result = await listUserLogs(userId, { page: 1, page_size: 100 })
    return result.items
  }

  return {
    rows,
    total,
    page,
    pageSize,
    search,
    selectedIds,
    loaded,
    listLoading,
    groupOptions,
    hasSelection,
    bootstrap,
    refreshList,
    refreshGroupOptions,
    setPage,
    setSearch,
    saveUser,
    removeOne,
    removeSelected,
    loadLogs,
  }
}
