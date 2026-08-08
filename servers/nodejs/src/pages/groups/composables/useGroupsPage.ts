import { ref } from 'vue'
import {
  createGroup,
  deleteGroup,
  getPermissionCatalog,
  listGroups,
  patchGroup,
  type GroupItem,
  type PermissionCatalog,
} from '@/api/users'

const rows = ref<GroupItem[]>([])
const catalog = ref<PermissionCatalog>({ modules: [] })
const selectedIds = ref<string[]>([])
const loaded = ref(false)
const listLoading = ref(false)

export function useGroupsPage() {
  async function refreshList() {
    listLoading.value = true
    const result = await listGroups()
    rows.value = result.items
    const visible = new Set(result.items.map((item) => item.id))
    selectedIds.value = selectedIds.value.filter((id) => visible.has(id))
    listLoading.value = false
  }

  async function refreshCatalog() {
    catalog.value = await getPermissionCatalog()
  }

  async function bootstrap() {
    await Promise.all([refreshList(), refreshCatalog()])
    loaded.value = true
  }

  async function saveGroup(
    mode: 'add' | 'edit',
    groupId: string | null,
    body: { name: string; permission_ids: number[] },
  ) {
    if (mode === 'add') {
      await createGroup(body)
    } else if (groupId) {
      await patchGroup(groupId, body)
    }
    await refreshList()
  }

  async function removeOne(groupId: string) {
    await deleteGroup(groupId)
    await refreshList()
  }

  async function removeSelected() {
    const ids = [...selectedIds.value]
    for (const id of ids) {
      await deleteGroup(id)
    }
    selectedIds.value = []
    await refreshList()
    return ids.length
  }

  return {
    rows,
    catalog,
    selectedIds,
    loaded,
    listLoading,
    bootstrap,
    refreshList,
    refreshCatalog,
    saveGroup,
    removeOne,
    removeSelected,
  }
}
