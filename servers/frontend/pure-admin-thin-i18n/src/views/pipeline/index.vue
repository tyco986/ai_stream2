<script setup lang="ts">
import { onMounted, ref } from "vue";
import Play from "~icons/ri/play-circle-line";
import Stop from "~icons/ri/stop-circle-line";
import Search from "~icons/ri/search-line";
import Edit from "~icons/ri/edit-line";
import DeleteBin from "~icons/ri/delete-bin-line";
import { getPipelines, probePipeline, removePipeline, removePipelines, startPipeline, stopPipeline, type PipelineItem } from "@/api/pipeline";
import { PIPELINE_LIST_COLUMN_PERCENTS, PIPELINE_LIST_PAGE_SIZE } from "@/constants/pipeline";
import { $t, transformI18n } from "@/plugins/i18n";
import { message } from "@/utils/message";
import { formatDuration, formatLocalTime } from "@/utils/time";
import AnalyzerDialog from "@/views/pipeline/analyzer/index.vue";
import { useColumnWidths } from "@/views/pipeline/columnWidth";
import PipelineCreate from "@/views/pipeline/create.vue";
import GieDialog from "@/views/pipeline/gie/index.vue";
import "@/views/pipeline/dialog.css";

defineOptions({
  name: "PipelineIndex"
});

const { target: listRef, widths: listWidths, observe } = useColumnWidths(PIPELINE_LIST_COLUMN_PERCENTS);
const items = ref<PipelineItem[]>([]);
const selected = ref<PipelineItem[]>([]);
const createOpen = ref(false);
const editId = ref("");
const gieOpen = ref(false);
const analyzerOpen = ref(false);
const startingIds = ref<string[]>([]);
const stoppingIds = ref<string[]>([]);
const probingIds = ref<string[]>([]);
const removingIds = ref<string[]>([]);

const joined = (names: string[]) => names.join(", ");

const loadPipelines = () => {
  getPipelines({ page: 1, page_size: PIPELINE_LIST_PAGE_SIZE }).then(res => {
    items.value = res.data.items;
  });
};

const onCreated = () => {
  createOpen.value = false;
  loadPipelines();
};

const onCreate = () => {
  editId.value = "";
  createOpen.value = true;
};

const onStart = (row: PipelineItem) => {
  startingIds.value = [...startingIds.value, row.id];
  startPipeline(row.id)
    .then(res => {
      message(res.message, { type: "success" });
      loadPipelines();
    })
    .finally(() => {
      startingIds.value = startingIds.value.filter(id => id !== row.id);
    });
};

const onStop = (row: PipelineItem) => {
  stoppingIds.value = [...stoppingIds.value, row.id];
  stopPipeline(row.id)
    .then(res => {
      message(res.message, { type: "success" });
      loadPipelines();
    })
    .finally(() => {
      stoppingIds.value = stoppingIds.value.filter(id => id !== row.id);
    });
};

const onProbe = (row: PipelineItem) => {
  probingIds.value = [...probingIds.value, row.id];
  probePipeline(row.id)
    .then(res => {
      message(res.message, { type: "success" });
      loadPipelines();
    })
    .finally(() => {
      probingIds.value = probingIds.value.filter(id => id !== row.id);
    });
};

const onEdit = (row: PipelineItem) => {
  editId.value = row.id;
  createOpen.value = true;
};

const onRemove = (row: PipelineItem) => {
  removingIds.value = [...removingIds.value, row.id];
  removePipeline(row.id)
    .then(res => {
      message(res.message, { type: "success" });
    })
    .finally(() => {
      removingIds.value = removingIds.value.filter(id => id !== row.id);
      loadPipelines();
    });
};

const onRemoveBatch = () => {
  const ids = selected.value.map(row => row.id);
  removingIds.value = [...removingIds.value, ...ids];
  removePipelines(ids)
    .then(res => {
      message(res.message, { type: "success" });
    })
    .finally(() => {
      removingIds.value = removingIds.value.filter(id => !ids.includes(id));
      loadPipelines();
    });
};

onMounted(() => {
  observe();
  loadPipelines();
});
</script>

<template>
  <div class="pipeline-page">
    <div class="pipeline-toolbar">
      <el-button type="primary" @click="onCreate">
        {{ transformI18n($t("pipeline.pureCreate")) }}
      </el-button>
      <el-button type="primary" :disabled="!selected.length || selected.some(row => removingIds.includes(row.id))" @click="onRemoveBatch">
        {{ transformI18n($t("pipeline.pureRemove")) }}
      </el-button>
      <el-button type="primary" @click="gieOpen = true">
        {{ transformI18n($t("pipeline.pureGie")) }}
      </el-button>
      <el-button type="primary" @click="analyzerOpen = true">
        {{ transformI18n($t("pipeline.pureAnalyzer")) }}
      </el-button>
    </div>
    <PipelineCreate v-if="createOpen" :pipeline-id="editId" @close="createOpen = false" @created="onCreated" />
    <GieDialog v-if="gieOpen" @close="gieOpen = false" />
    <AnalyzerDialog v-if="analyzerOpen" @close="analyzerOpen = false" />
    <el-table ref="listRef" class="pipeline-list" :data="items" table-layout="fixed" border show-overflow-tooltip @selection-change="selected = $event">
      <el-table-column type="selection" :width="listWidths.selection" />
      <el-table-column prop="name" :label="transformI18n($t('pipeline.pureName'))" :width="listWidths.name" />
      <el-table-column prop="type" :label="transformI18n($t('pipeline.pureType'))" :width="listWidths.type" />
      <el-table-column prop="status" :label="transformI18n($t('pipeline.pureStatus'))" :width="listWidths.status" />
      <el-table-column :label="transformI18n($t('pipeline.pureGie'))" :width="listWidths.gie">
        <template #default="{ row }">{{ joined(row.gie_names) }}</template>
        </el-table-column>
      <el-table-column :label="transformI18n($t('pipeline.pureStartTime'))" :width="listWidths.startTime">
        <template #default="{ row }">{{ formatLocalTime(row.start_time) }}</template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('pipeline.pureEndTime'))" :width="listWidths.endTime">
        <template #default="{ row }">{{ formatLocalTime(row.end_time) }}</template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('pipeline.pureDuration'))" :width="listWidths.duration">
        <template #default="{ row }">{{ formatDuration(row.duration) }}</template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('pipeline.pureOps'))" :width="listWidths.ops" :show-overflow-tooltip="false">
        <template #default="{ row }">
          <el-button v-if="row.status === 'running' || row.status === 'degraded'" link type="primary" :disabled="stoppingIds.includes(row.id)" :title="transformI18n($t('pipeline.pureStop'))" @click="onStop(row)">
            <IconifyIconOffline :icon="Stop" />
          </el-button>
          <el-button v-else link type="primary" :disabled="row.status === 'starting' || startingIds.includes(row.id)" :title="transformI18n($t('pipeline.pureStart'))" @click="onStart(row)">
            <IconifyIconOffline :icon="Play" />
          </el-button>
          <el-button link type="primary" :disabled="probingIds.includes(row.id)" :title="transformI18n($t('pipeline.pureProbe'))" @click="onProbe(row)">
            <IconifyIconOffline :icon="Search" />
          </el-button>
          <el-button link type="primary" :title="transformI18n($t('pipeline.pureEdit'))" @click="onEdit(row)">
            <IconifyIconOffline :icon="Edit" />
          </el-button>
          <el-button link type="primary" :disabled="removingIds.includes(row.id)" :title="transformI18n($t('pipeline.pureRemove'))" @click="onRemove(row)">
            <IconifyIconOffline :icon="DeleteBin" />
          </el-button>
        </template>
      </el-table-column>
    </el-table>
  </div>
</template>

<style scoped>
.pipeline-page.main-content {
  display: flex;
  flex: 1;
  flex-direction: column;
  min-width: 0;
  min-height: 0;
  margin: 0;
  overflow: hidden;
  background: var(--el-bg-color);
}

.pipeline-toolbar {
  display: flex;
  flex: none;
  gap: 8px;
  align-items: center;
  padding: 8px;
  background: var(--el-bg-color);
  border-bottom: 1px solid var(--el-border-color);
}

.pipeline-toolbar :deep(.el-button) {
  margin: 0;
}

.pipeline-list {
  flex: 1;
  width: 100%;
  min-height: 0;
}

.pipeline-list :deep(.cell) {
  word-break: normal;
  white-space: nowrap;
}

.pipeline-list :deep(.el-button) {
  margin: 0;
  font-size: 16px;
}
</style>
