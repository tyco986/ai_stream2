<script setup lang="ts">
import { computed, nextTick, onMounted, onUnmounted, ref, watch } from "vue";
import Play from "~icons/ri/play-circle-line";
import Edit from "~icons/ri/edit-line";
import DeleteBin from "~icons/ri/delete-bin-line";
import { getModel, getModelMap } from "@/api/model";
import { createGie, type GieClassAttrs } from "@/api/pipelineGie";
import {
  ANALYZER_LIST_COLUMN_PERCENTS,
  GIE_ATTR_COLUMN_PERCENTS,
  GIE_LIST_COLUMN_PERCENTS,
  PIPELINE_LIST_COLUMN_PERCENTS
} from "@/constants/pipeline";
import { $t, transformI18n } from "@/plugins/i18n";
import { columnPixels } from "@/utils/columnWidth";
import { message } from "@/utils/message";

type AttrRow = {
  classKey: string;
  conf: number | null;
  maxDet: number | null;
  minW: number | null;
  minH: number | null;
  maxW: number | null;
  maxH: number | null;
};

const emptyAttr = (classKey: string): AttrRow => ({
  classKey,
  conf: null,
  maxDet: null,
  minW: null,
  minH: null,
  maxW: null,
  maxH: null
});

const attrFields = (row: AttrRow) => ({
  conf: row.conf,
  topk: row.maxDet,
  detected_min_w: row.minW,
  detected_min_h: row.minH,
  detected_max_w: row.maxW,
  detected_max_h: row.maxH
});

defineOptions({
  name: "PipelineIndex"
});

const listRef = ref();
const listWidth = ref(0);
const listObserver = new ResizeObserver(entries => {
  listWidth.value = Math.floor(entries[0].contentRect.width);
});
const listWidths = computed(() => columnPixels(listWidth.value, PIPELINE_LIST_COLUMN_PERCENTS));
const items = ref<Record<string, string>[]>([]);
const gieVisible = ref(false);
const gieRef = ref();
const gieWidth = ref(0);
const gieObserver = new ResizeObserver(entries => {
  gieWidth.value = Math.floor(entries[0].contentRect.width);
});
const gieWidths = computed(() => columnPixels(gieWidth.value, GIE_LIST_COLUMN_PERCENTS));
const gieItems = ref<Record<string, string>[]>([]);
const gieCreateVisible = ref(false);
const gieName = ref("");
const gieInterval = ref(0);
const gieModel = ref("");
const gieModels = ref<{ id: string; name: string }[]>([]);
const gieClasses = ref<string[]>([]);
const modelConf = ref<number | null>(null);
const modelMaxDet = ref<number | null>(null);
const gieAttrs = ref<AttrRow[]>([emptyAttr("all")]);
const attrRef = ref();
const attrWidth = ref(0);
const attrObserver = new ResizeObserver(entries => {
  attrWidth.value = Math.floor(entries[0].contentRect.width);
});
const attrWidths = computed(() => columnPixels(attrWidth.value, GIE_ATTR_COLUMN_PERCENTS));
const gieConfirmable = computed(() => Boolean(gieName.value.trim() && gieModel.value));
const analyzerVisible = ref(false);
const analyzerRef = ref();
const analyzerWidth = ref(0);
const analyzerObserver = new ResizeObserver(entries => {
  analyzerWidth.value = Math.floor(entries[0].contentRect.width);
});
const analyzerWidths = computed(() => columnPixels(analyzerWidth.value, ANALYZER_LIST_COLUMN_PERCENTS));
const analyzerItems = ref<Record<string, string>[]>([]);

const observeGie = () => {
  const el = gieRef.value?.$el as HTMLElement | undefined;
  el && gieObserver.observe(el);
};

const observeAttr = () => {
  const el = attrRef.value?.$el as HTMLElement | undefined;
  el && attrObserver.observe(el);
};

const defaultAttr = (classKey: string): AttrRow => ({
  classKey,
  conf: modelConf.value,
  maxDet: modelMaxDet.value,
  minW: null,
  minH: null,
  maxW: null,
  maxH: null
});

const onGie = () => {
  gieVisible.value = true;
  getModelMap().then(res => {
    gieModels.value = Object.entries(res.data).map(([name, id]) => ({ id, name }));
  });
};

const onGieCreate = () => {
  gieName.value = "";
  gieInterval.value = 0;
  gieModel.value = "";
  gieClasses.value = [];
  modelConf.value = null;
  modelMaxDet.value = null;
  gieAttrs.value = [emptyAttr("all")];
  gieCreateVisible.value = true;
};

const onGieModel = (id: string) => {
  getModel(id).then(res => {
    const item = res.data;
    modelConf.value = item.conf;
    modelMaxDet.value = item.max_det;
    gieClasses.value = Array.isArray(item.classes) ? item.classes.map(String) : [];
    gieAttrs.value = [defaultAttr("all")];
  });
};

const onAddAttr = () => {
  gieAttrs.value = [...gieAttrs.value, defaultAttr("")];
};

const onRemoveAttr = (index: number) => {
  const next = gieAttrs.value.filter((_, itemIndex) => itemIndex !== index);
  gieAttrs.value = next.length ? next : [defaultAttr("all")];
};

const classTaken = (classKey: string, row: AttrRow) =>
  gieAttrs.value.some(item => item !== row && item.classKey === classKey);

const onGieConfirm = () => {
  const classAttrs = gieAttrs.value
    .filter(row => row.classKey)
    .reduce<GieClassAttrs>((attrs, row) => {
      attrs[row.classKey] = Object.fromEntries(
        Object.entries(attrFields(row)).filter((entry): entry is [string, number] => entry[1] !== null)
      );
      return attrs;
    }, {});
  createGie({
    model_id: gieModel.value,
    name: gieName.value,
    interval: gieInterval.value,
    class_attrs: classAttrs
  }).then(res => {
    message(res.message, { type: "success" });
    gieCreateVisible.value = false;
  });
};

const observeAnalyzer = () => {
  const el = analyzerRef.value?.$el as HTMLElement | undefined;
  el && analyzerObserver.observe(el);
};

watch(gieVisible, value => {
  value && nextTick(observeGie);
});

watch(analyzerVisible, value => {
  value && nextTick(observeAnalyzer);
});

watch(gieCreateVisible, value => {
  value && nextTick(observeAttr);
});

onMounted(() => {
  const el = listRef.value?.$el as HTMLElement | undefined;
  el && listObserver.observe(el);
});

onUnmounted(() => {
  listObserver.disconnect();
  gieObserver.disconnect();
  analyzerObserver.disconnect();
  attrObserver.disconnect();
});
</script>

<template>
  <div class="pipeline-page">
    <div class="pipeline-toolbar">
      <el-button type="primary">
        {{ transformI18n($t("pipeline.pureCreate")) }}
      </el-button>
      <el-button type="primary">
        {{ transformI18n($t("pipeline.pureRemove")) }}
      </el-button>
      <el-button type="primary" @click="onGie">
        {{ transformI18n($t("pipeline.pureGie")) }}
      </el-button>
      <el-button type="primary" @click="analyzerVisible = true">
        {{ transformI18n($t("pipeline.pureAnalyzer")) }}
      </el-button>
    </div>
    <el-dialog
      v-model="gieVisible"
      class="create-dialog"
      :title="transformI18n($t('pipeline.pureGie'))"
      width="92%"
    >
      <div class="pipeline-toolbar">
        <el-button type="primary" @click="onGieCreate">
          {{ transformI18n($t("pipeline.pureCreate")) }}
        </el-button>
        <el-button type="primary">
          {{ transformI18n($t("pipeline.pureRemove")) }}
        </el-button>
      </div>
      <el-table ref="gieRef" class="gie-list" :data="gieItems" table-layout="fixed" border show-overflow-tooltip>
        <el-table-column type="selection" :width="gieWidths.selection" />
        <el-table-column :label="transformI18n($t('pipeline.pureName'))" :width="gieWidths.name" />
        <el-table-column :label="transformI18n($t('pipeline.pureModel'))" :width="gieWidths.model" />
        <el-table-column :label="transformI18n($t('pipeline.pureClassAttrs'))" :width="gieWidths.classAttrs" />
        <el-table-column :label="transformI18n($t('pipeline.pureInterval'))" :width="gieWidths.interval" />
        <el-table-column :label="transformI18n($t('pipeline.pureUpdatedAt'))" :width="gieWidths.updatedAt" />
        <el-table-column :label="transformI18n($t('pipeline.pureOps'))" :width="gieWidths.ops" :show-overflow-tooltip="false">
          <template #default>
            <el-button link type="primary" :title="transformI18n($t('pipeline.pureEdit'))">
              <IconifyIconOffline :icon="Edit" />
            </el-button>
            <el-button link type="primary" :title="transformI18n($t('pipeline.pureRemove'))">
              <IconifyIconOffline :icon="DeleteBin" />
            </el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-dialog>
    <el-dialog
      v-model="gieCreateVisible"
      append-to-body
      class="create-dialog"
      :title="transformI18n($t('pipeline.pureCreate'))"
      width="70%"
    >
      <el-form label-width="88px" label-position="left">
        <el-form-item :label="transformI18n($t('pipeline.pureName'))">
          <el-input v-model="gieName" />
        </el-form-item>
        <el-form-item :label="transformI18n($t('pipeline.pureInterval'))">
          <el-input-number v-model="gieInterval" :min="0" :precision="0" :step="1" />
        </el-form-item>
        <el-form-item :label="transformI18n($t('pipeline.pureModel'))">
          <el-select v-model="gieModel" class="gie-model" @change="onGieModel">
            <el-option v-for="item in gieModels" :key="item.id" :label="item.name" :value="item.id" />
          </el-select>
        </el-form-item>
      </el-form>
      <el-table ref="attrRef" class="create-table" :data="gieAttrs" table-layout="fixed" border>
        <el-table-column :label="transformI18n($t('pipeline.pureClass'))" :width="attrWidths.classKey">
          <template #default="{ row }">
            <el-select v-model="row.classKey" :disabled="!gieModel">
              <el-option :label="transformI18n($t('pipeline.pureAll'))" value="all" :disabled="classTaken('all', row)" />
              <el-option
                v-for="(name, index) in gieClasses"
                :key="index"
                :label="name"
                :value="String(index)"
                :disabled="classTaken(String(index), row)"
              />
            </el-select>
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('pipeline.pureConf'))" :width="attrWidths.conf">
          <template #default="{ row }">
            <el-input-number v-model="row.conf" :disabled="!gieModel" :min="modelConf ?? 0" :max="1" :step="0.01" />
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('pipeline.pureMaxDet'))" :width="attrWidths.maxDet">
          <template #default="{ row }">
            <el-input-number v-model="row.maxDet" :disabled="!gieModel" :min="0" :max="modelMaxDet ?? undefined" :precision="0" :step="1" />
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('pipeline.pureMinW'))" :width="attrWidths.minW">
          <template #default="{ row }">
            <el-input-number v-model="row.minW" :disabled="!gieModel" :min="0" :precision="0" :step="1" />
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('pipeline.pureMinH'))" :width="attrWidths.minH">
          <template #default="{ row }">
            <el-input-number v-model="row.minH" :disabled="!gieModel" :min="0" :precision="0" :step="1" />
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('pipeline.pureMaxW'))" :width="attrWidths.maxW">
          <template #default="{ row }">
            <el-input-number v-model="row.maxW" :disabled="!gieModel" :min="0" :precision="0" :step="1" />
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('pipeline.pureMaxH'))" :width="attrWidths.maxH">
          <template #default="{ row }">
            <el-input-number v-model="row.maxH" :disabled="!gieModel" :min="0" :precision="0" :step="1" />
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('pipeline.pureOps'))" :width="attrWidths.ops" :show-overflow-tooltip="false">
          <template #default="{ $index }">
            <el-button link type="primary" :disabled="!gieModel" :title="transformI18n($t('pipeline.pureRemove'))" @click="onRemoveAttr($index)">
              <IconifyIconOffline :icon="DeleteBin" />
            </el-button>
          </template>
        </el-table-column>
      </el-table>
      <el-button class="add-row" :disabled="!gieModel" @click="onAddAttr">+</el-button>
      <template #footer>
        <el-button @click="gieCreateVisible = false">
          {{ transformI18n($t("pipeline.pureCancel")) }}
        </el-button>
        <el-button type="primary" :disabled="!gieConfirmable" @click="onGieConfirm">
          {{ transformI18n($t("pipeline.pureConfirm")) }}
        </el-button>
      </template>
    </el-dialog>
    <el-dialog
      v-model="analyzerVisible"
      class="create-dialog"
      :title="transformI18n($t('pipeline.pureAnalyzer'))"
      width="92%"
    >
      <div class="pipeline-toolbar">
        <el-button type="primary">
          {{ transformI18n($t("pipeline.pureCreate")) }}
        </el-button>
        <el-button type="primary">
          {{ transformI18n($t("pipeline.pureRemove")) }}
        </el-button>
      </div>
      <el-table ref="analyzerRef" class="analyzer-list" :data="analyzerItems" table-layout="fixed" border show-overflow-tooltip>
        <el-table-column type="selection" :width="analyzerWidths.selection" />
        <el-table-column :label="transformI18n($t('pipeline.pureName'))" :width="analyzerWidths.name" />
        <el-table-column :label="transformI18n($t('pipeline.pureAnnotations'))" :width="analyzerWidths.annotations" />
        <el-table-column :label="transformI18n($t('pipeline.pureUpdatedAt'))" :width="analyzerWidths.updatedAt" />
        <el-table-column :label="transformI18n($t('pipeline.pureOps'))" :width="analyzerWidths.ops" :show-overflow-tooltip="false">
          <template #default>
            <el-button link type="primary" :title="transformI18n($t('pipeline.pureEdit'))">
              <IconifyIconOffline :icon="Edit" />
            </el-button>
            <el-button link type="primary" :title="transformI18n($t('pipeline.pureRemove'))">
              <IconifyIconOffline :icon="DeleteBin" />
            </el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-dialog>
    <el-table ref="listRef" class="pipeline-list" :data="items" table-layout="fixed" border show-overflow-tooltip>
      <el-table-column type="selection" :width="listWidths.selection" />
      <el-table-column :label="transformI18n($t('pipeline.pureName'))" :width="listWidths.name" />
      <el-table-column :label="transformI18n($t('pipeline.pureType'))" :width="listWidths.type" />
      <el-table-column :label="transformI18n($t('pipeline.pureStatus'))" :width="listWidths.status" />
      <el-table-column :label="transformI18n($t('pipeline.pureGie'))" :width="listWidths.gie" />
      <el-table-column :label="transformI18n($t('pipeline.pureAnalyzer'))" :width="listWidths.analyzer" />
      <el-table-column :label="transformI18n($t('pipeline.pureOps'))" :width="listWidths.ops" :show-overflow-tooltip="false">
        <template #default>
          <el-button link type="primary" :title="transformI18n($t('pipeline.pureStart'))">
            <IconifyIconOffline :icon="Play" />
          </el-button>
          <el-button link type="primary" :title="transformI18n($t('pipeline.pureEdit'))">
            <IconifyIconOffline :icon="Edit" />
          </el-button>
          <el-button link type="primary" :title="transformI18n($t('pipeline.pureRemove'))">
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

.analyzer-list :deep(.cell),
.gie-list :deep(.cell) {
  word-break: normal;
  white-space: nowrap;
}

.analyzer-list :deep(.el-button),
.gie-list :deep(.el-button) {
  margin: 0;
  font-size: 16px;
}

.gie-model,
.create-table :deep(.el-input-number),
.create-table :deep(.el-select) {
  width: 100%;
}

.create-table :deep(.cell) {
  padding: 0 4px;
  white-space: nowrap;
}

.create-table :deep(.el-input-number .el-input__wrapper) {
  padding-left: 8px;
  padding-right: 32px;
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
</style>
