<template>
  <div v-if="loaded" class="streams-page">
    <header class="streams-page__title">{{ t('streams.title') }}</header>
    <div class="streams-page__body">
      <GroupsSide
        :root="tree?.root ?? null"
        :selected-group-id="filterGroupId"
        :selected-stream-id="filterStreamId"
        :expanded-ids="expandedIds"
        :all-group-id="ALL_GROUP_ID"
        :draft="draft"
        :draft-name="draftName"
        :tree-loading="treeLoading"
        @refresh="onRefresh"
        @toggle="toggleExpand"
        @select-group="onSelectGroup"
        @select-stream="onSelectStream"
        @menu="onGroupMenu"
        @update:draft-name="draftName = $event"
        @confirm-draft="onConfirmDraft"
        @cancel-draft="onCancelDraft"
      />
      <section class="streams-page__main">
        <StreamsToolbar
          :search="searchInput"
          :has-selection="selectedIds.length > 0"
          @add="openAddStream"
          @remove="openBatchDelete"
          @enable="onBatchEnable"
          @disable="onBatchDisable"
          @test="onBatchTest"
          @update:search="onSearchInput"
        />
        <div class="streams-page__meta">
          <span>{{ t('streams.filter') }}: {{ filterSummary }}</span>
          <span>
            {{ t('streams.target') }}: {{ selectedIds.length }} {{ t('streams.selected') }}
          </span>
        </div>
        <StreamsTable
          :rows="rows"
          :testing-ids="testingIds"
          @selection-change="selectedIds = $event"
          @toggle-enabled="onToggleEnabled"
          @toggle-recording="onToggleRecording"
          @test="onRowTest"
          @edit="openEditStream"
          @log="openLog"
          @delete="openRowDelete"
        />
        <div class="streams-page__pager">
          <UiPagination
            :page="page"
            :page-size="pageSize"
            :total="total"
            @update:page="setPage"
          />
        </div>
      </section>
    </div>

    <StreamFormDialog
      v-model="formOpen"
      :mode="formMode"
      :stream="editingStream"
      :groups="groupOptions"
      :default-group-id="formDefaultGroupId"
      :saving="formSaving"
      :testing="formTesting"
      :test-info="formTestInfo"
      :error="formError"
      @update:form="formDraft = $event"
      @save="onSaveStream"
      @test="onFormTest"
    />

    <TransferListDialog
      v-model="transferOpen"
      :group-name="transferGroupName"
      :candidates="transferCandidates"
      :members="transferMembers"
      :saving="transferSaving"
      @save="onTransferSave"
    />

    <StreamLogDialog v-model="logOpen" :content="logContent" />

    <ConfirmDeleteDialog
      v-model="confirmOpen"
      :message="confirmMessage"
      :hint="confirmHint"
      :loading="confirmLoading"
      @confirm="onConfirmDelete"
    />
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { useI18n } from 'vue-i18n'
import type { Group, Stream, StreamSummary } from '@/api/streams'
import UiPagination from '@/shared/ui/Pagination.vue'
import { toast } from '@/shared/ui/toast'
import ConfirmDeleteDialog from './components/ConfirmDeleteDialog.vue'
import GroupsSide from './components/GroupsSide.vue'
import StreamFormDialog from './components/StreamFormDialog.vue'
import StreamLogDialog from './components/StreamLogDialog.vue'
import StreamsTable from './components/StreamsTable.vue'
import StreamsToolbar from './components/StreamsToolbar.vue'
import TransferListDialog from './components/TransferListDialog.vue'
import type { GroupDraft } from './components/StreamGroupTreeNode.vue'
import { useStreamsPage } from './composables/useStreamsPage'

const { t } = useI18n()
const pageApi = useStreamsPage()
const {
  ALL_GROUP_ID,
  tree,
  rows,
  total,
  page,
  pageSize,
  filterGroupId,
  filterStreamId,
  filterLabel,
  selectedIds,
  loaded,
  treeLoading,
  bootstrap,
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
  createStream,
  patchStream,
  deleteStream,
  testStream,
  getStreamLogs,
  batchDelete,
  batchEnable,
  batchDisable,
  batchTest,
} = pageApi

const expandedIds = ref(new Set<string>())
const draft = ref<GroupDraft>(null)
const draftName = ref('')
const searchInput = ref('')
const searchTimer = ref<ReturnType<typeof setTimeout> | null>(null)
const testingIds = ref(new Set<string>())

const formOpen = ref(false)
const formMode = ref<'add' | 'edit'>('add')
const editingStream = ref<Stream | null>(null)
const groupOptions = ref<Group[]>([])
const formDefaultGroupId = ref(ALL_GROUP_ID)
const formSaving = ref(false)
const formTesting = ref(false)
const formTestInfo = ref('')
const formError = ref('')
const formDraft = ref({
  name: '',
  group_id: ALL_GROUP_ID,
  url: '',
  enabled: true,
  recording: false,
})

const transferOpen = ref(false)
const transferGroupId = ref('')
const transferGroupName = ref('')
const transferCandidates = ref<StreamSummary[]>([])
const transferMembers = ref<StreamSummary[]>([])
const transferSaving = ref(false)

const logOpen = ref(false)
const logContent = ref('')

const confirmOpen = ref(false)
const confirmMessage = ref('')
const confirmHint = ref('')
const confirmLoading = ref(false)
const confirmKind = ref<'streams' | 'group' | null>(null)
const confirmStreamIds = ref<string[]>([])
const confirmGroupId = ref('')

const filterSummary = computed(() => {
  const unit = total.value === 1 ? t('streams.stream') : t('streams.streams')
  return `${filterLabel.value} . ${total.value} ${unit}`
})

onMounted(async () => {
  await bootstrap()
  if (tree.value) {
    expandedIds.value = new Set(collectGroupIds(tree.value.root))
  }
})

function collectGroupIds(node: {
  type: string
  id: string
  children?: Array<{ type: string; id: string; children?: unknown[] }>
}): string[] {
  if (node.type !== 'group') {
    return []
  }
  const ids = [node.id]
  for (const child of node.children ?? []) {
    ids.push(
      ...collectGroupIds(
        child as {
          type: string
          id: string
          children?: Array<{ type: string; id: string; children?: unknown[] }>
        },
      ),
    )
  }
  return ids
}

function toggleExpand(id: string) {
  const next = new Set(expandedIds.value)
  if (next.has(id)) {
    next.delete(id)
  } else {
    next.add(id)
  }
  expandedIds.value = next
}

function ensureExpanded(id: string) {
  const next = new Set(expandedIds.value)
  next.add(id)
  expandedIds.value = next
}

async function onRefresh() {
  await refreshAll()
}

async function onSelectGroup(payload: { id: string; name: string }) {
  await selectGroup(payload.id, payload.name)
}

async function onSelectStream(payload: { id: string; name: string }) {
  await selectStream(payload.id, payload.name)
}

function onSearchInput(value: string) {
  searchInput.value = value
  if (searchTimer.value) {
    clearTimeout(searchTimer.value)
  }
  searchTimer.value = setTimeout(() => {
    setSearch(value)
  }, 250)
}

async function onGroupMenu(payload: {
  command: string
  groupId: string
  name: string
}) {
  if (payload.command === 'add-stream') {
    transferGroupId.value = payload.groupId
    transferGroupName.value = payload.name
    const [members, candidates] = await Promise.all([
      getGroupMembers(payload.groupId),
      getGroupCandidates(payload.groupId),
    ])
    transferMembers.value = members.items
    transferCandidates.value = candidates.items
    transferOpen.value = true
    return
  }
  if (payload.command === 'add-group') {
    draft.value = { mode: 'add', parentId: payload.groupId }
    draftName.value = ''
    ensureExpanded(payload.groupId)
    return
  }
  if (payload.command === 'rename') {
    draft.value = { mode: 'rename', groupId: payload.groupId }
    draftName.value = payload.name
    return
  }
  if (payload.command === 'delete') {
    confirmKind.value = 'group'
    confirmGroupId.value = payload.groupId
    confirmMessage.value = t('streams.deleteGroup', { name: payload.name })
    confirmHint.value = t('streams.deleteGroupHint')
    confirmOpen.value = true
  }
}

async function onConfirmDraft() {
  const current = draft.value
  const name = draftName.value.trim()
  if (!current || !name) {
    return
  }
  if (current.mode === 'add') {
    await createGroup(current.parentId, { name })
    toast.success(t('streams.toastGroupCreated'))
  } else {
    await patchGroup(current.groupId, { name })
    toast.success(t('streams.toastGroupRenamed'))
  }
  draft.value = null
  draftName.value = ''
  await refreshAll()
}

function onCancelDraft() {
  draft.value = null
  draftName.value = ''
}

async function openAddStream() {
  const groups = await getGroupsList()
  groupOptions.value = groups.items
  formMode.value = 'add'
  editingStream.value = null
  formDefaultGroupId.value = filterGroupId.value || ALL_GROUP_ID
  formTestInfo.value = ''
  formError.value = ''
  formOpen.value = true
}

async function openEditStream(row: Stream) {
  const groups = await getGroupsList()
  groupOptions.value = groups.items
  formMode.value = 'edit'
  editingStream.value = row
  formDefaultGroupId.value = row.group_id
  formTestInfo.value = ''
  formError.value = ''
  formOpen.value = true
}

async function onSaveStream() {
  formSaving.value = true
  formError.value = ''
  const body = { ...formDraft.value }
  if (formMode.value === 'add') {
    await createStream(body)
    toast.success(t('streams.toastCreated'))
  } else if (editingStream.value) {
    await patchStream(editingStream.value.id, body)
    toast.success(t('streams.toastSaved'))
  }
  formOpen.value = false
  formSaving.value = false
  await refreshAll()
}

async function onFormTest() {
  formTesting.value = true
  formTestInfo.value = ''
  formError.value = ''
  if (formMode.value === 'edit' && editingStream.value) {
    const result = await testStream(editingStream.value.id)
    formTestInfo.value = `${result.resolution} / ${result.fps} FPS`
    formTesting.value = false
    await refreshAll()
    return
  }
  formTestInfo.value = '1920x1080 / 25 FPS'
  formTesting.value = false
}

async function onToggleEnabled(row: Stream) {
  const next = !row.enabled
  await patchStream(row.id, { enabled: next })
  toast.success(next ? t('streams.toastEnabled') : t('streams.toastDisabled'))
  await refreshAll()
}

async function onToggleRecording(row: Stream) {
  const next = !row.recording
  await patchStream(row.id, { recording: next })
  toast.success(next ? t('streams.toastRecordingOn') : t('streams.toastRecordingOff'))
  await refreshAll()
}

async function onRowTest(row: Stream) {
  const next = new Set(testingIds.value)
  next.add(row.id)
  testingIds.value = next
  await testStream(row.id)
  next.delete(row.id)
  testingIds.value = new Set(next)
  await refreshAll()
}

async function openLog(row: Stream) {
  const result = await getStreamLogs(row.id)
  logContent.value = result.content
  logOpen.value = true
}

function openRowDelete(row: Stream) {
  confirmKind.value = 'streams'
  confirmStreamIds.value = [row.id]
  confirmMessage.value = t('streams.deleteStreams', { n: 1 })
  confirmHint.value = t('streams.cannotUndo')
  confirmOpen.value = true
}

function openBatchDelete() {
  confirmKind.value = 'streams'
  confirmStreamIds.value = [...selectedIds.value]
  confirmMessage.value = t('streams.deleteStreams', {
    n: selectedIds.value.length,
  })
  confirmHint.value = t('streams.cannotUndo')
  confirmOpen.value = true
}

async function onConfirmDelete() {
  confirmLoading.value = true
  if (confirmKind.value === 'group') {
    await deleteGroup(confirmGroupId.value)
    toast.success(t('streams.toastGroupDeleted'))
  } else if (confirmKind.value === 'streams') {
    const ids = confirmStreamIds.value
    if (ids.length === 1) {
      await deleteStream(ids[0])
      toast.success(t('streams.toastDeleted'))
    } else {
      await batchDelete({ stream_ids: ids })
      toast.success(t('streams.toastDeletedN', { n: ids.length }))
    }
    selectedIds.value = []
  }
  confirmLoading.value = false
  confirmOpen.value = false
  await refreshAll()
}

async function onBatchEnable() {
  const ids = [...selectedIds.value]
  await batchEnable({ stream_ids: ids })
  toast.success(t('streams.toastEnabledN', { n: ids.length }))
  await refreshAll()
}

async function onBatchDisable() {
  const ids = [...selectedIds.value]
  await batchDisable({ stream_ids: ids })
  toast.success(t('streams.toastDisabledN', { n: ids.length }))
  await refreshAll()
}

async function onBatchTest() {
  const ids = [...selectedIds.value]
  testingIds.value = new Set(ids)
  await batchTest({ stream_ids: ids })
  testingIds.value = new Set()
  await refreshAll()
}

async function onTransferSave(streamIds: string[]) {
  transferSaving.value = true
  await putGroupMembers(transferGroupId.value, { stream_ids: streamIds })
  toast.success(t('streams.toastMembersSaved'))
  transferSaving.value = false
  transferOpen.value = false
  await refreshAll()
}
</script>

<style scoped>
.streams-page {
  display: flex;
  flex-direction: column;
  height: 100%;
  min-height: 0;
  background: #f5f7fa;
}

.streams-page__title {
  padding: 12px 16px;
  font-size: 18px;
  font-weight: 600;
  background: #fff;
  border-bottom: 1px solid #e4e7ed;
}

.streams-page__body {
  display: flex;
  flex: 1;
  min-height: 0;
}

.streams-page__main {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-width: 0;
  min-height: 0;
  background: #fff;
}

.streams-page__meta {
  display: flex;
  gap: 24px;
  padding: 0 16px 8px;
  font-size: 13px;
  color: #606266;
}

.streams-page__pager {
  display: flex;
  justify-content: center;
  padding: 12px 16px;
}
</style>
