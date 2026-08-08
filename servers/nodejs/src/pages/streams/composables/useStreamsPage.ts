import { computed, ref } from 'vue'
import {
  ALL_GROUP_ID,
  batchDelete,
  batchDisable,
  batchEnable,
  batchRecord,
  batchProbe,
  batchUnrecord,
  createGroup,
  createStream,
  deleteGroup,
  deleteStream,
  getGroupCandidates,
  getGroupMembers,
  getGroupsList,
  getGroupsTree,
  getStream,
  getStreamLogs,
  listStreams,
  patchGroup,
  patchStream,
  publishVideo,
  putGroupMembers,
  probeStream,
  probeStreamUrl,
  type Group,
  type GroupsTree,
  type Stream,
  type StreamSummary,
} from '@/api/streams'

const tree = ref<GroupsTree | null>(null)
const rows = ref<Stream[]>([])
const total = ref(0)
const page = ref(1)
const pageSize = ref(20)
const search = ref('')
const filterGroupId = ref<string | null>(ALL_GROUP_ID)
const filterStreamId = ref<string | null>(null)
const filterLabel = ref('All')
const selectedIds = ref<string[]>([])
const loaded = ref(false)
const treeLoading = ref(false)
const listLoading = ref(false)

export function useStreamsPage() {
  const filterText = computed(() => {
    const count = total.value
    const unit = count === 1 ? 'stream' : 'streams'
    return `${filterLabel.value} . ${count} ${unit}`
  })

  async function refreshTree() {
    treeLoading.value = true
    tree.value = await getGroupsTree()
    treeLoading.value = false
  }

  async function refreshList() {
    listLoading.value = true
    const query: {
      group_id?: string
      stream_id?: string
      search?: string
      page: number
      page_size: number
    } = {
      page: page.value,
      page_size: pageSize.value,
    }
    if (filterStreamId.value) {
      query.stream_id = filterStreamId.value
    } else if (filterGroupId.value) {
      query.group_id = filterGroupId.value
    }
    if (search.value.trim()) {
      query.search = search.value.trim()
    }
    const result = await listStreams(query)
    rows.value = result.items
    total.value = result.total
    listLoading.value = false
  }

  async function bootstrap() {
    await Promise.all([refreshTree(), refreshList()])
    loaded.value = true
  }

  async function selectGroup(groupId: string, name: string) {
    filterGroupId.value = groupId
    filterStreamId.value = null
    filterLabel.value = name
    page.value = 1
    selectedIds.value = []
    await refreshList()
  }

  async function selectStream(streamId: string, name: string) {
    filterStreamId.value = streamId
    filterGroupId.value = null
    filterLabel.value = name
    page.value = 1
    selectedIds.value = []
    await refreshList()
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

  async function refreshAll() {
    await Promise.all([refreshTree(), refreshList()])
  }

  return {
    ALL_GROUP_ID,
    tree,
    rows,
    total,
    page,
    pageSize,
    search,
    filterGroupId,
    filterStreamId,
    filterLabel,
    filterText,
    selectedIds,
    loaded,
    treeLoading,
    listLoading,
    bootstrap,
    refreshTree,
    refreshList,
    refreshAll,
    selectGroup,
    selectStream,
    setSearch,
    setPage,
    createGroup,
    patchGroup,
    deleteGroup,
    getGroupMembers,
    getGroupCandidates,
    putGroupMembers,
    getGroupsList,
    getStream,
    createStream,
    patchStream,
    deleteStream,
    probeStream,
    probeStreamUrl,
    getStreamLogs,
    batchDelete,
    batchEnable,
    batchDisable,
    batchRecord,
    batchUnrecord,
    batchProbe,
    publishVideo,
  }
}

export type { Group, Stream, StreamSummary }
