<script setup lang="ts">
import { onMounted, ref } from "vue";
import { createStreams, getStreams, probeRtsp, type StreamDraft, type StreamItem } from "@/api/stream";
import { getGroupMap } from "@/api/streamGroup";
import StreamGroupTree from "@/components/StreamGroupTree/index.vue";
import { ALL_GROUP_ID } from "@/constants/streamGroup";
import { $t, transformI18n } from "@/plugins/i18n";

defineOptions({
  name: "StreamIndex"
});

type DraftRow = StreamDraft & { info: string };

const blankRow = (): DraftRow => ({
  name: "",
  url: "",
  group_id: ALL_GROUP_ID,
  info: ""
});

const items = ref<StreamItem[]>([]);
const visible = ref(false);
const rows = ref<DraftRow[]>([blankRow()]);
const groups = ref<Record<string, string>>({});

const loadStreams = () => {
  getStreams({ page: 1, page_size: 20 }).then(res => {
    items.value = res.data.items;
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

const onProbe = (row: DraftRow) => {
  probeRtsp(row.url).then(res => {
    const probed = res.data[row.url];
    row.width = probed?.width ?? null;
    row.height = probed?.height ?? null;
    row.fps = probed?.fps ?? null;
    row.status = probed?.status ?? "offline";
    row.probed_at = probed?.probed_at ?? null;
    row.info = probed ? `${probed.width}×${probed.height} ${probed.fps} ${probed.status}` : "";
  });
};

const onCancel = () => {
  visible.value = false;
};

const onConfirm = () => {
  const drafts = rows.value
    .filter(row => row.name && row.url)
    .map(({ info, ...row }) => row);
  createStreams(drafts).then(() => {
    visible.value = false;
    loadStreams();
  });
};

onMounted(loadStreams);
</script>

<template>
  <div class="stream-page">
    <aside class="stream-tree">
      <StreamGroupTree />
    </aside>
    <div class="stream-main">
    <div class="stream-toolbar">
      <el-button type="primary" @click="onCreate">
        {{ transformI18n($t("stream.pureCreate")) }}
      </el-button>
    </div>
    <el-dialog
      v-model="visible"
      :title="transformI18n($t('stream.pureCreate'))"
      width="92%"
    >
      <el-table class="create-table" :data="rows" table-layout="fixed" border>
        <el-table-column :label="transformI18n($t('stream.pureName'))" min-width="120">
          <template #default="{ row }">
            <el-input v-model="row.name" />
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('stream.pureUrl'))" min-width="180">
          <template #default="{ row }">
            <el-input v-model="row.url" />
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('stream.pureGroup'))" width="140">
          <template #default="{ row }">
            <el-select v-model="row.group_id">
              <el-option v-for="(id, name) in groups" :key="id" :label="name" :value="id" />
            </el-select>
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('stream.pureProbe'))" width="108">
          <template #default="{ row }">
            <el-button type="primary" @click="onProbe(row)">
              {{ transformI18n($t("stream.pureProbe")) }}
            </el-button>
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('stream.pureInfo'))" min-width="120">
          <template #default="{ row }">{{ row.info }}</template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('stream.pureOps'))" width="72">
          <template #default="{ $index }">
            <el-button @click="onRemove($index)">-</el-button>
          </template>
        </el-table-column>
      </el-table>
      <el-button class="add-row" @click="onAdd">+</el-button>
      <template #footer>
        <el-button @click="onCancel">
          {{ transformI18n($t("stream.pureCancel")) }}
        </el-button>
        <el-button type="primary" @click="onConfirm">
          {{ transformI18n($t("stream.pureConfirm")) }}
        </el-button>
      </template>
    </el-dialog>
    <el-table class="stream-list" :data="items" table-layout="fixed">
      <el-table-column type="selection" width="48" />
      <el-table-column prop="name" :label="transformI18n($t('stream.pureName'))" />
      <el-table-column prop="url" :label="transformI18n($t('stream.pureUrl'))" show-overflow-tooltip />
      <el-table-column prop="group_name" :label="transformI18n($t('stream.pureGroup'))" width="140" />
      <el-table-column prop="width" :label="transformI18n($t('stream.pureWidth'))" width="90" />
      <el-table-column prop="height" :label="transformI18n($t('stream.pureHeight'))" width="90" />
      <el-table-column prop="fps" :label="transformI18n($t('stream.pureFps'))" width="80" />
      <el-table-column prop="status" :label="transformI18n($t('stream.pureStatus'))" width="100" />
      <el-table-column :label="transformI18n($t('stream.pureOps'))" width="100" />
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
  background: var(--el-bg-color);
}

.stream-list {
  flex: 1;
  min-height: 0;
}

.stream-toolbar {
  display: flex;
  flex: none;
  align-items: center;
  padding: 8px;
  background: var(--el-bg-color);
  border-bottom: 1px solid var(--el-border-color);
}

.create-table :deep(.cell) {
  padding: 0 4px;
  overflow: visible;
  text-overflow: clip;
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

.add-row {
  width: 100%;
  margin-top: -1px;
  border-radius: 0;
}

:deep(.el-dialog__footer) {
  padding-top: 8px;
}
</style>
