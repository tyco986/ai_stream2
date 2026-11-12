<script setup lang="ts">
import { nextTick, ref, watch } from "vue";
import ListCheck from "~icons/ri/list-check";
import Edit from "~icons/ri/edit-line";
import DeleteBin from "~icons/ri/delete-bin-line";
import { getAnalyzers, removeAnalyzer, removeAnalyzers, type AnalyzerItem } from "@/api/pipelineAnalyzer";
import { ANALYZER_ANNOTATION_DIALOG_WIDTH, ANALYZER_DIALOG_WIDTH, ANALYZER_LIST_COLUMN_PERCENTS } from "@/constants/pipeline";
import { $t, transformI18n } from "@/plugins/i18n";
import { message } from "@/utils/message";
import { formatLocalTime } from "@/utils/time";
import AnalyzerCreate from "@/views/pipeline/analyzer/create.vue";
import { useColumnWidths } from "@/views/pipeline/columnWidth";

const open = ref(true);
const emit = defineEmits<{ close: [] }>();
const { target: listRef, widths, observe } = useColumnWidths(ANALYZER_LIST_COLUMN_PERCENTS);
const items = ref<AnalyzerItem[]>([]);
const selected = ref<AnalyzerItem[]>([]);
const annotationsOpen = ref(false);
const annotationText = ref("");
const formOpen = ref(false);
const editRecord = ref<AnalyzerItem | null>(null);

const load = () => {
  getAnalyzers({ page: 1, page_size: 20 }).then(res => {
    items.value = res.data.items;
  });
};

const onAnnotations = (row: AnalyzerItem) => {
  annotationText.value = JSON.stringify(row.annotations ?? {}, null, 2);
  annotationsOpen.value = true;
};

const onCreate = () => {
  editRecord.value = null;
  formOpen.value = true;
};

const onEdit = (row: AnalyzerItem) => {
  editRecord.value = JSON.parse(JSON.stringify(row)) as AnalyzerItem;
  formOpen.value = true;
};

const onRemove = (row: AnalyzerItem) => {
  removeAnalyzer(row.id).then(res => {
    message(res.message, { type: "success" });
    load();
  });
};

const onRemoveBatch = () => {
  removeAnalyzers(selected.value.map(row => row.id)).then(res => {
    message(res.message, { type: "success" });
    load();
  });
};

const onSaved = () => {
  formOpen.value = false;
  load();
};

load();
watch(open, value => {
  value ? nextTick(observe) : emit("close");
});
nextTick(observe);
</script>

<template>
  <el-dialog v-model="open" class="create-dialog" :title="transformI18n($t('pipeline.pureAnalyzer'))" :width="ANALYZER_DIALOG_WIDTH">
    <div class="pipeline-toolbar">
      <el-button type="primary" @click="onCreate">{{ transformI18n($t("pipeline.pureCreate")) }}</el-button>
      <el-button type="primary" :disabled="!selected.length" @click="onRemoveBatch">
        {{ transformI18n($t("pipeline.pureRemove")) }}
      </el-button>
    </div>
    <el-table ref="listRef" class="analyzer-list" :data="items" table-layout="fixed" border show-overflow-tooltip @selection-change="selected = $event">
      <el-table-column type="selection" :width="widths.selection" />
      <el-table-column prop="name" :label="transformI18n($t('pipeline.pureName'))" :width="widths.name" />
      <el-table-column prop="media_name" :label="transformI18n($t('pipeline.pureImage'))" :width="widths.image" />
      <el-table-column prop="width" :label="transformI18n($t('media.pureWidth'))" :width="widths.width" />
      <el-table-column prop="height" :label="transformI18n($t('media.pureHeight'))" :width="widths.height" />
      <el-table-column :label="transformI18n($t('pipeline.pureAnnotations'))" :width="widths.annotations" :show-overflow-tooltip="false">
        <template #default="{ row }">
          <el-button link type="primary" :title="transformI18n($t('pipeline.pureAnnotations'))" @click="onAnnotations(row)">
            <IconifyIconOffline :icon="ListCheck" />
          </el-button>
        </template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('pipeline.pureUpdatedAt'))" :width="widths.updatedAt">
        <template #default="{ row }">{{ formatLocalTime(row.updated_at) }}</template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('pipeline.pureOps'))" :width="widths.ops" :show-overflow-tooltip="false">
        <template #default="{ row }">
          <el-button link type="primary" :title="transformI18n($t('pipeline.pureEdit'))" @click="onEdit(row)">
            <IconifyIconOffline :icon="Edit" />
          </el-button>
          <el-button link type="primary" :title="transformI18n($t('pipeline.pureRemove'))" @click="onRemove(row)">
            <IconifyIconOffline :icon="DeleteBin" />
          </el-button>
        </template>
      </el-table-column>
    </el-table>
  </el-dialog>
  <el-dialog v-model="annotationsOpen" append-to-body :title="transformI18n($t('pipeline.pureAnnotations'))" :width="ANALYZER_ANNOTATION_DIALOG_WIDTH">
    <pre class="analyzer-annotations">{{ annotationText }}</pre>
  </el-dialog>
  <AnalyzerCreate v-if="formOpen" :record="editRecord" @close="formOpen = false" @saved="onSaved" />
</template>

<style scoped>
.pipeline-toolbar {
  display: flex;
  flex: none;
  gap: 8px;
  align-items: center;
  padding: 8px;
  background: var(--el-bg-color);
  border-bottom: 1px solid var(--el-border-color);
}

.pipeline-toolbar :deep(.el-button),
.analyzer-list :deep(.el-button) {
  margin: 0;
}

.analyzer-list :deep(.cell) {
  word-break: normal;
  white-space: nowrap;
}

.analyzer-list :deep(.el-button) {
  font-size: 16px;
}

.analyzer-annotations {
  max-height: 420px;
  margin: 0;
  overflow: auto;
  font-family: monospace;
  font-size: 12px;
  line-height: 20px;
  white-space: pre;
}
</style>
