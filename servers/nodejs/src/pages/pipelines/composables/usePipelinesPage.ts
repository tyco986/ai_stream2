import { ref } from 'vue'
import {
  batchDeletePipelines,
  createPipeline,
  deletePipeline,
  getPipeline,
  getPipelineLogs,
  getPipelineSchema,
  getPipelineStatus,
  getPipelineTypes,
  listPipelines,
  startPipeline,
  stopPipeline,
  updatePipeline,
  type PipelineListItem,
  type PipelineStatus,
} from '@/api/pipelines'

const rows = ref<PipelineListItem[]>([])
const total = ref(0)
const page = ref(1)
const pageSize = ref(20)
const search = ref('')
const selectedIds = ref<string[]>([])
const loaded = ref(false)
const listLoading = ref(false)
const pollingIds = ref(new Set<string>())

export function usePipelinesPage() {
  function updateRowStatus(pipelineId: string, status: PipelineStatus) {
    rows.value = rows.value.map((row) => {
      if (row.id === pipelineId) {
        return { ...row, status }
      }
      return row
    })
  }

  function markPolling(pipelineId: string, active: boolean) {
    const next = new Set(pollingIds.value)
    if (active) {
      next.add(pipelineId)
    } else {
      next.delete(pipelineId)
    }
    pollingIds.value = next
  }

  async function refreshList() {
    listLoading.value = true
    const query: { search?: string; page: number; page_size: number } = {
      page: page.value,
      page_size: pageSize.value,
    }
    if (search.value.trim()) {
      query.search = search.value.trim()
    }
    const result = await listPipelines(query)
    rows.value = result.items
    total.value = result.total
    listLoading.value = false
  }

  async function bootstrap() {
    await refreshList()
    loaded.value = true
  }

  async function setSearch(value: string) {
    search.value = value
    page.value = 1
    await refreshList()
  }

  async function setPage(value: number) {
    page.value = value
    await refreshList()
  }

  async function waitForTerminalStatus(
    pipelineId: string,
    terminalStatuses: PipelineStatus[],
    options: { intervalMs?: number; timeoutMs?: number } = {},
  ): Promise<PipelineStatus> {
    const intervalMs = options.intervalMs ?? 1000
    const timeoutMs = options.timeoutMs ?? 60000
    const start = Date.now()
    let lastStatus: PipelineStatus = 'starting'
    while (Date.now() - start < timeoutMs) {
      await new Promise((resolve) => setTimeout(resolve, intervalMs))
      const result = await getPipelineStatus(pipelineId)
      lastStatus = result.status
      updateRowStatus(pipelineId, result.status)
      if (terminalStatuses.includes(result.status)) {
        return result.status
      }
    }
    return lastStatus
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
    pollingIds,
    bootstrap,
    refreshList,
    setSearch,
    setPage,
    updateRowStatus,
    markPolling,
    waitForTerminalStatus,
    listPipelines,
    getPipeline,
    createPipeline,
    updatePipeline,
    deletePipeline,
    batchDeletePipelines,
    startPipeline,
    stopPipeline,
    getPipelineStatus,
    getPipelineLogs,
    getPipelineTypes,
    getPipelineSchema,
  }
}

export type { PipelineListItem, PipelineStatus }
