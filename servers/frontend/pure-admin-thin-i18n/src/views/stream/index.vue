<script setup lang="ts">
import { computed, nextTick, onMounted, onUnmounted, ref, watch } from "vue";
import HardDrive from "~icons/ri/hard-drive-2-line";
import RecordCircle from "~icons/ri/record-circle-line";
import Search from "~icons/ri/search-line";
import Edit from "~icons/ri/edit-line";
import DeleteBin from "~icons/ri/delete-bin-line";
import {
  batchStreams,
  createStreams,
  getStreams,
  patchStream,
  probeRtsp,
  probeStream,
  removeStream,
  type StreamDraft,
  type StreamItem
} from "@/api/stream";
import { getGroupMap } from "@/api/streamGroup";
import BarButton from "@/components/BarButton/index.vue";
import SearchHighlight from "@/components/SearchHighlight/index.vue";
import StreamGroupTree from "@/components/StreamGroupTree/index.vue";
import {
  PATH_STREAM_BATCH_MOUNT,
  PATH_STREAM_BATCH_PROBE,
  PATH_STREAM_BATCH_RECORD,
  PATH_STREAM_BATCH_REMOVE,
  PATH_STREAM_BATCH_UNMOUNT,
  PATH_STREAM_BATCH_UNRECORD
} from "@/constants/path";
import { STREAM_COLUMN_ALIGNS, STREAM_CREATE_COLUMN_ALIGNS, STREAM_CREATE_COLUMN_PERCENTS, STREAM_CREATE_COLUMN_RESIZABLE, STREAM_DIALOG_WIDTH, STREAM_EDIT_COLUMN_RESIZABLE, STREAM_LIST_COLUMN_PERCENTS, STREAM_LIST_COLUMN_RESIZABLE, STREAM_ROW_HEIGHT, STREAM_STATUS_LABEL } from "@/constants/stream";
import { ALL_GROUP_ID } from "@/constants/streamGroup";
import { $t, transformI18n } from "@/plugins/i18n";
import { message } from "@/utils/message";
import { columnPixels } from "@/utils/columnWidth";
import { formatLocalTime } from "@/utils/time";

defineOptions({
  name: "StreamIndex"
});

type DraftRow = StreamDraft;

const statusLabel = (status?: string) =>
  STREAM_STATUS_LABEL[status as keyof typeof STREAM_STATUS_LABEL] ?? status ?? "";

const blankRow = (): DraftRow => ({
  name: "",
  url: "",
  group_id: ALL_GROUP_ID
});

const treeRef = ref<{ load: () => Promise<unknown> }>();
const listRef = ref();
const listWidth = ref(0);
const listObserver = new ResizeObserver(entries => {
  listWidth.value = Math.floor(entries[0].contentRect.width);
});
const createRef = ref();
const createWidth = ref(0);
const createObserver = new ResizeObserver(entries => {
  createWidth.value = Math.floor(entries[0].contentRect.width);
});

const listWidths = computed(() => columnPixels(listWidth.value, STREAM_LIST_COLUMN_PERCENTS));

const createWidths = computed(() => columnPixels(createWidth.value, STREAM_CREATE_COLUMN_PERCENTS));

const observeCreate = () => {
  const el = createRef.value?.$el as HTMLElement | undefined;
  el && createObserver.observe(el);
};

const keyword = ref("");
const searched = ref("");
const items = ref<StreamItem[]>([]);
const selected = ref<StreamItem[]>([]);
const visible = ref(false);
const editing = ref(false);
const editId = ref("");
const editRow = ref<DraftRow>(blankRow());
const rows = ref<DraftRow[]>([blankRow()]);
const namesReady = computed(() => rows.value.every(row => row.name.trim()));
const picked = ref<DraftRow[]>([]);
const groups = ref<Record<string, string>>({});
const probing = ref(false);
const probingIds = ref<string[]>([]);
const probingRows = ref<DraftRow[]>([]);

const loadStreams = () => {
  searched.value = keyword.value;
  getStreams({ page: 1, page_size: 20, search: keyword.value }).then(res => {
    items.value = res.data.items;
  });
};

const reloadTree = () => {
  treeRef.value?.load();
};

const onBatch = (path: string) => {
  batchStreams(path, selected.value.map(row => row.id)).then(res => {
    message(res.message, { type: "success" });
    loadStreams();
    path === PATH_STREAM_BATCH_REMOVE && reloadTree();
  });
};

const onSelectStreams = (value: StreamItem[]) => {
  selected.value = value;
};

const done = (messageText: string) => {
  message(messageText, { type: "success" });
  loadStreams();
};

const onMount = (row: StreamItem) => {
  const path = row.mount ? PATH_STREAM_BATCH_UNMOUNT : PATH_STREAM_BATCH_MOUNT;
  batchStreams(path, [row.id]).then(res => done(res.message));
};

const onRecord = (row: StreamItem) => {
  const path = row.record ? PATH_STREAM_BATCH_UNRECORD : PATH_STREAM_BATCH_RECORD;
  batchStreams(path, [row.id]).then(res => done(res.message));
};

const onProbeBatch = () => {
  probing.value = true;
  batchStreams(PATH_STREAM_BATCH_PROBE, selected.value.map(row => row.id))
    .then(res => {
      message(res.message, { type: "success" });
      loadStreams();
    })
    .finally(() => {
      probing.value = false;
    });
};

const onProbeItem = (row: StreamItem) => {
  probingIds.value = [...probingIds.value, row.id];
  probeStream(row.id)
    .then(res => done(res.message))
    .finally(() => {
      probingIds.value = probingIds.value.filter(id => id !== row.id);
    });
};

const onEdit = (row: StreamItem) => {
  editId.value = row.id;
  editRow.value = {
    name: row.name,
    url: row.url,
    group_id: row.group_id ?? ALL_GROUP_ID,
    width: row.width,
    height: row.height,
    fps: row.fps,
    status: row.status,
    probed_at: row.probed_at
  };
  editing.value = true;
  getGroupMap().then(res => {
    groups.value = res.data;
  });
};

const onEditCancel = () => {
  editing.value = false;
};

const onEditConfirm = () => {
  patchStream(editId.value, {
    name: editRow.value.name,
    url: editRow.value.url,
    group_id: editRow.value.group_id
  }).then(res => {
    editing.value = false;
    done(res.message);
    reloadTree();
  });
};

const onRemoveItem = (row: StreamItem) => {
  removeStream(row.id).then(res => {
    done(res.message);
    reloadTree();
  });
};

const onCreate = () => {
  rows.value = [blankRow()];
  visible.value = true;
  getGroupMap().then(res => {
    groups.value = res.data;
  });
};

const onAdd = () => {
  rows.value = [...rows.value, blankRow()];
};

const onRemove = (index: number) => {
  const next = rows.value.filter((_, itemIndex) => itemIndex !== index);
  rows.value = next.length ? next : [blankRow()];
};

const onSelect = (value: DraftRow[]) => {
  picked.value = value;
};

const onProbeSelected = () => {
  picked.value.forEach(onProbe);
};

const onRemoveSelected = () => {
  const next = rows.value.filter(row => !picked.value.includes(row));
  rows.value = next.length ? next : [blankRow()];
  picked.value = [];
};

const onProbe = (row: DraftRow) => {
  probingRows.value = [...probingRows.value, row];
  probeRtsp(row.url)
    .then(res => {
      const probed = res.data[row.url];
      row.width = probed?.width ?? null;
      row.height = probed?.height ?? null;
      row.fps = probed?.fps ?? null;
      row.status = probed?.status ?? "offline";
      row.probed_at = probed?.probed_at ?? null;
    })
    .finally(() => {
      probingRows.value = probingRows.value.filter(item => item !== row);
    });
};

const onCancel = () => {
  visible.value = false;
};

const onConfirm = () => {
  const drafts = rows.value.filter(row => row.name && row.url);
  createStreams(drafts).then(res => {
    message(res.message, { type: res.success ? "success" : "warning" });
    visible.value = false;
    loadStreams();
    reloadTree();
  });
};

watch(visible, value => {
  value && nextTick(observeCreate);
});

onMounted(() => {
  loadStreams();
  const el = listRef.value?.$el as HTMLElement | undefined;
  el && listObserver.observe(el);
});

onUnmounted(() => {
  listObserver.disconnect();
  createObserver.disconnect();
});
</script>

<template>
  <div class="stream-page">
    <aside class="stream-tree">
      <StreamGroupTree ref="treeRef" />
    </aside>
    <div class="stream-main">
    <div class="stream-toolbar">
      <el-button type="primary" @click="onCreate">
        {{ transformI18n($t("stream.pureCreate")) }}
      </el-button>
      <el-button type="primary" :disabled="!selected.length" @click="onBatch(PATH_STREAM_BATCH_REMOVE)">
        {{ transformI18n($t("stream.pureRemove")) }}
      </el-button>
      <BarButton :loading="probing" :disabled="!selected.length" @click="onProbeBatch">
        {{ transformI18n($t("stream.pureProbe")) }}
      </BarButton>
      <el-button type="primary" :disabled="!selected.length" @click="onBatch(PATH_STREAM_BATCH_RECORD)">
        {{ transformI18n($t("stream.pureRecord")) }}
      </el-button>
      <el-button type="primary" :disabled="!selected.length" @click="onBatch(PATH_STREAM_BATCH_UNRECORD)">
        {{ transformI18n($t("stream.pureUnrecord")) }}
      </el-button>
      <el-button type="primary" :disabled="!selected.length" @click="onBatch(PATH_STREAM_BATCH_MOUNT)">
        {{ transformI18n($t("stream.pureMount")) }}
      </el-button>
      <el-button type="primary" :disabled="!selected.length" @click="onBatch(PATH_STREAM_BATCH_UNMOUNT)">
        {{ transformI18n($t("stream.pureUnmount")) }}
      </el-button>
      <el-input
        v-model="keyword"
        class="stream-search"
        clearable
        :placeholder="transformI18n($t('stream.pureSearch'))"
        @keyup.enter="loadStreams"
        @clear="loadStreams"
      >
        <template #prefix>
          <IconifyIconOffline :icon="Search" />
        </template>
      </el-input>
    </div>
    <el-dialog
      v-model="visible"
      class="create-dialog"
      :title="transformI18n($t('stream.pureCreate'))"
      :width="STREAM_DIALOG_WIDTH"
    >
      <div class="stream-toolbar">
        <BarButton @click="onProbeSelected">
          {{ transformI18n($t("stream.pureProbe")) }}
        </BarButton>
        <el-button type="primary" @click="onRemoveSelected">
          {{ transformI18n($t("stream.pureRemove")) }}
        </el-button>
      </div>
      <el-table ref="createRef" class="create-table" :data="rows" table-layout="fixed" border @selection-change="onSelect">
        <el-table-column type="selection" :resizable="STREAM_CREATE_COLUMN_RESIZABLE" :width="createWidths.selection" :header-align="STREAM_CREATE_COLUMN_ALIGNS.selection.header" :align="STREAM_CREATE_COLUMN_ALIGNS.selection.content" />
        <el-table-column :label="transformI18n($t('stream.pureName'))" :resizable="STREAM_CREATE_COLUMN_RESIZABLE" :width="createWidths.name" :header-align="STREAM_CREATE_COLUMN_ALIGNS.name.header" :align="STREAM_CREATE_COLUMN_ALIGNS.name.content">
          <template #default="{ row }">
            <el-input v-model="row.name" />
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('stream.pureUrl'))" :resizable="STREAM_CREATE_COLUMN_RESIZABLE" :width="createWidths.url" :header-align="STREAM_CREATE_COLUMN_ALIGNS.url.header" :align="STREAM_CREATE_COLUMN_ALIGNS.url.content">
          <template #default="{ row }">
            <el-input v-model="row.url" />
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('stream.pureGroup'))" :resizable="STREAM_CREATE_COLUMN_RESIZABLE" :width="createWidths.group" :header-align="STREAM_CREATE_COLUMN_ALIGNS.group.header" :align="STREAM_CREATE_COLUMN_ALIGNS.group.content">
          <template #default="{ row }">
            <el-select v-model="row.group_id">
              <el-option v-for="(id, name) in groups" :key="id" :label="name" :value="id" />
            </el-select>
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('stream.pureWidth'))" :resizable="STREAM_CREATE_COLUMN_RESIZABLE" :width="createWidths.width" :header-align="STREAM_CREATE_COLUMN_ALIGNS.width.header" :align="STREAM_CREATE_COLUMN_ALIGNS.width.content">
          <template #default="{ row }">{{ row.width ?? "" }}</template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('stream.pureHeight'))" :resizable="STREAM_CREATE_COLUMN_RESIZABLE" :width="createWidths.height" :header-align="STREAM_CREATE_COLUMN_ALIGNS.height.header" :align="STREAM_CREATE_COLUMN_ALIGNS.height.content">
          <template #default="{ row }">{{ row.height ?? "" }}</template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('stream.pureFps'))" :resizable="STREAM_CREATE_COLUMN_RESIZABLE" :width="createWidths.fps" :header-align="STREAM_CREATE_COLUMN_ALIGNS.fps.header" :align="STREAM_CREATE_COLUMN_ALIGNS.fps.content">
          <template #default="{ row }">{{ row.fps ?? "" }}</template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('stream.pureStatus'))" :resizable="STREAM_CREATE_COLUMN_RESIZABLE" :width="createWidths.status" :header-align="STREAM_CREATE_COLUMN_ALIGNS.status.header" :align="STREAM_CREATE_COLUMN_ALIGNS.status.content">
          <template #default="{ row }"><span :class="row.status">{{ statusLabel(row.status) }}</span></template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('stream.pureProbedAt'))" :resizable="STREAM_CREATE_COLUMN_RESIZABLE" :width="createWidths.probedAt" :header-align="STREAM_CREATE_COLUMN_ALIGNS.probedAt.header" :align="STREAM_CREATE_COLUMN_ALIGNS.probedAt.content">
          <template #default="{ row }">{{ formatLocalTime(row.probed_at) }}</template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('stream.pureOps'))" :resizable="STREAM_CREATE_COLUMN_RESIZABLE" :width="createWidths.ops" :show-overflow-tooltip="false" :header-align="STREAM_CREATE_COLUMN_ALIGNS.ops.header" :align="STREAM_CREATE_COLUMN_ALIGNS.ops.content">
          <template #default="{ row, $index }">
            <el-button link type="primary" :disabled="probingRows.includes(row)" :title="transformI18n($t('stream.pureProbe'))" @click="onProbe(row)">
              <IconifyIconOffline :icon="Search" />
            </el-button>
            <el-button link type="primary" :title="transformI18n($t('stream.pureRemove'))" @click="onRemove($index)">
              <IconifyIconOffline :icon="DeleteBin" />
            </el-button>
          </template>
        </el-table-column>
      </el-table>
      <el-button class="add-row" @click="onAdd">+</el-button>
      <template #footer>
        <el-button @click="onCancel">
          {{ transformI18n($t("stream.pureCancel")) }}
        </el-button>
        <el-button type="primary" :disabled="!namesReady" @click="onConfirm">
          {{ transformI18n($t("stream.pureConfirm")) }}
        </el-button>
      </template>
    </el-dialog>
    <el-dialog
      v-model="editing"
      class="create-dialog"
      :title="transformI18n($t('stream.pureEdit'))"
      :width="STREAM_DIALOG_WIDTH"
    >
      <el-table class="create-table" :data="[editRow]" table-layout="fixed" border>
        <el-table-column :resizable="STREAM_EDIT_COLUMN_RESIZABLE" :label="transformI18n($t('stream.pureName'))" :header-align="STREAM_COLUMN_ALIGNS.name.header" :align="STREAM_COLUMN_ALIGNS.name.content">
          <template #default="{ row }">
            <el-input v-model="row.name" />
          </template>
        </el-table-column>
        <el-table-column :resizable="STREAM_EDIT_COLUMN_RESIZABLE" :label="transformI18n($t('stream.pureUrl'))" :header-align="STREAM_COLUMN_ALIGNS.url.header" :align="STREAM_COLUMN_ALIGNS.url.content">
          <template #default="{ row }">
            <el-input v-model="row.url" />
          </template>
        </el-table-column>
        <el-table-column :resizable="STREAM_EDIT_COLUMN_RESIZABLE" :label="transformI18n($t('stream.pureGroup'))" width="100" :header-align="STREAM_COLUMN_ALIGNS.group.header" :align="STREAM_COLUMN_ALIGNS.group.content">
          <template #default="{ row }">
            <el-select v-model="row.group_id">
              <el-option v-for="(id, name) in groups" :key="id" :label="name" :value="id" />
            </el-select>
          </template>
        </el-table-column>
        <el-table-column :resizable="STREAM_EDIT_COLUMN_RESIZABLE" :label="transformI18n($t('stream.pureWidth'))" width="72" :header-align="STREAM_COLUMN_ALIGNS.width.header" :align="STREAM_COLUMN_ALIGNS.width.content">
          <template #default="{ row }">{{ row.width ?? "" }}</template>
        </el-table-column>
        <el-table-column :resizable="STREAM_EDIT_COLUMN_RESIZABLE" :label="transformI18n($t('stream.pureHeight'))" width="72" :header-align="STREAM_COLUMN_ALIGNS.height.header" :align="STREAM_COLUMN_ALIGNS.height.content">
          <template #default="{ row }">{{ row.height ?? "" }}</template>
        </el-table-column>
        <el-table-column :resizable="STREAM_EDIT_COLUMN_RESIZABLE" :label="transformI18n($t('stream.pureFps'))" width="72" :header-align="STREAM_COLUMN_ALIGNS.fps.header" :align="STREAM_COLUMN_ALIGNS.fps.content">
          <template #default="{ row }">{{ row.fps ?? "" }}</template>
        </el-table-column>
        <el-table-column :resizable="STREAM_EDIT_COLUMN_RESIZABLE" :label="transformI18n($t('stream.pureStatus'))" width="88" :header-align="STREAM_COLUMN_ALIGNS.status.header" :align="STREAM_COLUMN_ALIGNS.status.content">
          <template #default="{ row }"><span :class="row.status">{{ statusLabel(row.status) }}</span></template>
        </el-table-column>
        <el-table-column :resizable="STREAM_EDIT_COLUMN_RESIZABLE" :label="transformI18n($t('stream.pureProbedAt'))" min-width="160" :header-align="STREAM_COLUMN_ALIGNS.probedAt.header" :align="STREAM_COLUMN_ALIGNS.probedAt.content">
          <template #default="{ row }">{{ formatLocalTime(row.probed_at) }}</template>
        </el-table-column>
      </el-table>
      <template #footer>
        <el-button @click="onEditCancel">
          {{ transformI18n($t("stream.pureCancel")) }}
        </el-button>
        <el-button type="primary" @click="onEditConfirm">
          {{ transformI18n($t("stream.pureConfirm")) }}
        </el-button>
      </template>
    </el-dialog>
    <el-table ref="listRef" class="stream-list" :data="items" table-layout="fixed" border show-overflow-tooltip @selection-change="onSelectStreams">
      <el-table-column type="selection" :resizable="STREAM_LIST_COLUMN_RESIZABLE" :width="listWidths.selection" :header-align="STREAM_COLUMN_ALIGNS.selection.header" :align="STREAM_COLUMN_ALIGNS.selection.content" />
      <el-table-column :label="transformI18n($t('stream.pureName'))" :resizable="STREAM_LIST_COLUMN_RESIZABLE" :width="listWidths.name" :header-align="STREAM_COLUMN_ALIGNS.name.header" :align="STREAM_COLUMN_ALIGNS.name.content">
        <template #default="{ row }"><SearchHighlight :text="row.name" :query="searched" /></template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('stream.pureUrl'))" :resizable="STREAM_LIST_COLUMN_RESIZABLE" :width="listWidths.url" :header-align="STREAM_COLUMN_ALIGNS.url.header" :align="STREAM_COLUMN_ALIGNS.url.content">
        <template #default="{ row }"><SearchHighlight :text="row.url" :query="searched" /></template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('stream.pureGroup'))" :resizable="STREAM_LIST_COLUMN_RESIZABLE" :width="listWidths.group" :header-align="STREAM_COLUMN_ALIGNS.group.header" :align="STREAM_COLUMN_ALIGNS.group.content">
        <template #default="{ row }"><SearchHighlight :text="row.group_name" :query="searched" /></template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('stream.pureWidth'))" :resizable="STREAM_LIST_COLUMN_RESIZABLE" :width="listWidths.width" :header-align="STREAM_COLUMN_ALIGNS.width.header" :align="STREAM_COLUMN_ALIGNS.width.content">
        <template #default="{ row }"><SearchHighlight :text="String(row.width ?? '')" :query="searched" /></template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('stream.pureHeight'))" :resizable="STREAM_LIST_COLUMN_RESIZABLE" :width="listWidths.height" :header-align="STREAM_COLUMN_ALIGNS.height.header" :align="STREAM_COLUMN_ALIGNS.height.content">
        <template #default="{ row }"><SearchHighlight :text="String(row.height ?? '')" :query="searched" /></template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('stream.pureFps'))" :resizable="STREAM_LIST_COLUMN_RESIZABLE" :width="listWidths.fps" :header-align="STREAM_COLUMN_ALIGNS.fps.header" :align="STREAM_COLUMN_ALIGNS.fps.content">
        <template #default="{ row }"><SearchHighlight :text="String(row.fps ?? '')" :query="searched" /></template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('stream.pureStatus'))" :resizable="STREAM_LIST_COLUMN_RESIZABLE" :width="listWidths.status" :header-align="STREAM_COLUMN_ALIGNS.status.header" :align="STREAM_COLUMN_ALIGNS.status.content">
        <template #default="{ row }"><span :class="row.status"><SearchHighlight :text="statusLabel(row.status)" :query="searched" /></span></template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('stream.pureProbedAt'))" :resizable="STREAM_LIST_COLUMN_RESIZABLE" :width="listWidths.probedAt" :header-align="STREAM_COLUMN_ALIGNS.probedAt.header" :align="STREAM_COLUMN_ALIGNS.probedAt.content">
        <template #default="{ row }"><SearchHighlight :text="formatLocalTime(row.probed_at)" :query="searched" /></template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('stream.pureOps'))" :resizable="STREAM_LIST_COLUMN_RESIZABLE" :width="listWidths.ops" :show-overflow-tooltip="false" :header-align="STREAM_COLUMN_ALIGNS.ops.header" :align="STREAM_COLUMN_ALIGNS.ops.content">
        <template #default="{ row }">
          <el-button
            link
            :type="row.mount ? 'success' : 'primary'"
            :title="transformI18n($t(row.mount ? 'stream.pureUnmount' : 'stream.pureMount'))"
            @click="onMount(row)"
          >
            <IconifyIconOffline :icon="HardDrive" />
          </el-button>
          <el-button
            link
            :type="row.record ? 'success' : 'primary'"
            :title="transformI18n($t(row.record ? 'stream.pureUnrecord' : 'stream.pureRecord'))"
            @click="onRecord(row)"
          >
            <IconifyIconOffline :icon="RecordCircle" />
          </el-button>
          <el-button link type="primary" :disabled="probingIds.includes(row.id)" :title="transformI18n($t('stream.pureProbe'))" @click="onProbeItem(row)">
            <IconifyIconOffline :icon="Search" />
          </el-button>
          <el-button link type="primary" :title="transformI18n($t('stream.pureEdit'))" @click="onEdit(row)">
            <IconifyIconOffline :icon="Edit" />
          </el-button>
          <el-button link type="primary" :title="transformI18n($t('stream.pureRemove'))" @click="onRemoveItem(row)">
            <IconifyIconOffline :icon="DeleteBin" />
          </el-button>
        </template>
      </el-table-column>
    </el-table>
    </div>
  </div>
</template>

<style scoped>
.stream-page.main-content {
  display: flex;
  flex: 1;
  min-width: 0;
  min-height: 0;
  margin: 0;
}

.stream-tree {
  flex: 0 0 240px;
  align-self: stretch;
  overflow: auto;
  background: var(--el-bg-color);
  border-right: 1px solid var(--el-border-color);
}

.stream-main {
  display: flex;
  flex: 1;
  flex-direction: column;
  min-width: 0;
  min-height: 0;
  overflow: hidden;
  background: var(--el-bg-color);
}

.stream-list {
  flex: 1;
  width: 100%;
  min-height: 0;
}

.stream-list :deep(.cell) {
  word-break: normal;
  white-space: nowrap;
}

.stream-toolbar {
  display: flex;
  flex: none;
  gap: 8px;
  align-items: center;
  padding: 8px;
  background: var(--el-bg-color);
}

.stream-toolbar :deep(.el-button) {
  margin: 0;
}

.stream-search {
  width: 220px;
  margin-left: auto;
}

.stream-list :deep(.el-button) {
  margin: 0;
  font-size: 16px;
}

.create-table :deep(.el-table__cell) {
  height: v-bind('STREAM_ROW_HEIGHT + "px"');
  padding-top: 0;
  padding-bottom: 0;
}

.create-table :deep(.cell) {
  padding: 0 4px;
  overflow: visible;
  text-overflow: clip;
  white-space: nowrap;
}

.create-table :deep(.el-button) {
  margin: 0;
  font-size: 16px;
}

.create-table :deep(.el-input__wrapper),
.create-table :deep(.el-input__wrapper:hover),
.create-table :deep(.el-input__wrapper.is-focus),
.create-table :deep(.el-select__wrapper),
.create-table :deep(.el-select__wrapper:hover),
.create-table :deep(.el-select__wrapper.is-focused) {
  padding: 0;
  background: transparent;
  box-shadow: none;
}

.online {
  color: var(--el-color-success);
}

.offline {
  color: var(--el-color-danger);
}

.add-row {
  width: 100%;
  margin-top: -1px;
  border-radius: 0;
}

</style>

<style>
.create-dialog.el-dialog {
  padding: 12px;
}

.create-dialog .el-dialog__header {
  padding-bottom: 8px;
}

.create-dialog .el-dialog__body {
  padding: 0;
}

.create-dialog .el-dialog__footer {
  padding-top: 8px;
}
</style>
