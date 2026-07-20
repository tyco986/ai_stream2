import { ref } from 'vue'
import {
  getServerLogs,
  listServers,
  refreshServers,
  restartServer,
  type Server,
} from '@/api/servers'

const rows = ref<Server[]>([])
const loaded = ref(false)
const refreshing = ref(false)
const restartingIds = ref(new Set<string>())

export function useServersPage() {
  function applyRefreshResults(
    results: Array<{
      id: string
      status: Server['status']
      last_refresh_at: string
    }>,
  ) {
    const byId = new Map(results.map((item) => [item.id, item]))
    rows.value = rows.value.map((row) => {
      const next = byId.get(row.id)
      if (!next) {
        return row
      }
      return {
        ...row,
        status: next.status,
        last_refresh_at: next.last_refresh_at,
      }
    })
  }

  async function loadList() {
    const result = await listServers()
    rows.value = result.items
  }

  async function bootstrap() {
    await loadList()
    loaded.value = true
  }

  async function refreshAll() {
    refreshing.value = true
    const result = await refreshServers()
    applyRefreshResults(result.results)
    refreshing.value = false
  }

  function markRestarting(serverId: string, active: boolean) {
    const next = new Set(restartingIds.value)
    if (active) {
      next.add(serverId)
    } else {
      next.delete(serverId)
    }
    restartingIds.value = next
  }

  return {
    rows,
    loaded,
    refreshing,
    restartingIds,
    bootstrap,
    loadList,
    refreshAll,
    markRestarting,
    applyRefreshResults,
    restartServer,
    getServerLogs,
  }
}

export type { Server }
