<script setup lang="ts">
import { nextTick, ref, watch } from "vue";
import ListCheck from "~icons/ri/list-check";
import Edit from "~icons/ri/edit-line";
import DeleteBin from "~icons/ri/delete-bin-line";
import { getGies, removeGie, removeGies, type GieItem } from "@/api/pipelineGie";
import { GIE_ATTR_DIALOG_WIDTH, GIE_DIALOG_WIDTH, GIE_LIST_COLUMN_PERCENTS } from "@/constants/pipeline";
import { $t, transformI18n } from "@/plugins/i18n";
import { message } from "@/utils/message";
import { formatLocalTime } from "@/utils/time";
import { useColumnWidths } from "@/views/pipeline/columnWidth";
import { attrRows, type AttrRow } from "@/views/pipeline/gie/attrs";
import GieCreate from "@/views/pipeline/gie/create.vue";

const open = ref(true);
const emit = defineEmits<{ close: [] }>();
const { target: listRef, widths, observe } = useColumnWidths(GIE_LIST_COLUMN_PERCENTS);
const items = ref<GieItem[]>([]);
const selected = ref<GieItem[]>([]);
const attrsOpen = ref(false);
const attrRowsView = ref<AttrRow[]>([]);
const formOpen = ref(false);
const editId = ref("");

const load = () => {
  getGies({ page: 1, page_size: 20 }).then(res => {
    items.value = res.data.items;
  });
};

const onAttrs = (row: GieItem) => {
  attrRowsView.value = attrRows(row.class_attrs);
  attrsOpen.value = true;
};

const onCreate = () => {
  editId.value = "";
  formOpen.value = true;
};

const onEdit = (row: GieItem) => {
  editId.value = row.id;
  formOpen.value = true;
};

const onRemove = (row: GieItem) => {
  removeGie(row.id).then(res => {
    message(res.message, { type: "success" });
    load();
  });
};

const onRemoveBatch = () => {
  removeGies(selected.value.map(row => row.id)).then(res => {
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
  <el-dialog v-model="open" class="create-dialog" :title="transformI18n($t('pipeline.pureGie'))" :width="GIE_DIALOG_WIDTH">
    <div class="pipeline-toolbar">
      <el-button type="primary" @click="onCreate">{{ transformI18n($t("pipeline.pureCreate")) }}</el-button>
      <el-button type="primary" :disabled="!selected.length" @click="onRemoveBatch">
        {{ transformI18n($t("pipeline.pureRemove")) }}
      </el-button>
    </div>
    <el-table ref="listRef" class="gie-list" :data="items" row-key="id" table-layout="fixed" border show-overflow-tooltip @selection-change="selected = $event">
      <el-table-column type="selection" :width="widths.selection" />
      <el-table-column prop="name" :label="transformI18n($t('pipeline.pureName'))" :width="widths.name" />
      <el-table-column prop="model_name" :label="transformI18n($t('pipeline.pureModel'))" :width="widths.model" />
      <el-table-column :label="transformI18n($t('pipeline.pureClassAttrs'))" :width="widths.classAttrs" :show-overflow-tooltip="false">
        <template #default="{ row }">
          <el-button link type="primary" :title="transformI18n($t('pipeline.pureClassAttrs'))" @click="onAttrs(row)">
            <IconifyIconOffline :icon="ListCheck" />
          </el-button>
        </template>
      </el-table-column>
      <el-table-column prop="interval" :label="transformI18n($t('pipeline.pureInterval'))" :width="widths.interval" />
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
  <el-dialog v-model="attrsOpen" append-to-body :title="transformI18n($t('pipeline.pureClassAttrs'))" :width="GIE_ATTR_DIALOG_WIDTH">
    <el-table :data="attrRowsView" border max-height="420" show-overflow-tooltip>
      <el-table-column :label="transformI18n($t('pipeline.pureClass'))">
        <template #default="{ row }">{{ row.classKey === "all" ? transformI18n($t("pipeline.pureAll")) : row.classKey }}</template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('pipeline.pureConf'))" prop="conf" />
      <el-table-column :label="transformI18n($t('pipeline.pureMaxDet'))" prop="maxDet" />
      <el-table-column :label="transformI18n($t('pipeline.pureMinW'))" prop="minW" />
      <el-table-column :label="transformI18n($t('pipeline.pureMinH'))" prop="minH" />
      <el-table-column :label="transformI18n($t('pipeline.pureMaxW'))" prop="maxW" />
      <el-table-column :label="transformI18n($t('pipeline.pureMaxH'))" prop="maxH" />
    </el-table>
  </el-dialog>
  <GieCreate v-if="formOpen" :gie-id="editId" @close="formOpen = false" @saved="onSaved" />
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
.gie-list :deep(.el-button) {
  margin: 0;
}

.gie-list :deep(.cell) {
  word-break: normal;
  white-space: nowrap;
}

.gie-list :deep(.el-button) {
  font-size: 16px;
}
</style>
