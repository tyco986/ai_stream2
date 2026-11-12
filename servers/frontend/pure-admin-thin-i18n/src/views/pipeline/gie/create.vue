<script setup lang="ts">
import { computed, nextTick, ref, watch } from "vue";
import DeleteBin from "~icons/ri/delete-bin-line";
import { getModel, getModelMap } from "@/api/model";
import { createGie, getGie, updateGie, type GieDraft } from "@/api/pipelineGie";
import { GIE_ATTR_COLUMN_PERCENTS, GIE_ATTR_UNSUPPORTED_TYPES, GIE_FORM_DIALOG_WIDTH, GIE_INTERVAL_MIN } from "@/constants/pipeline";
import { $t, transformI18n } from "@/plugins/i18n";
import { message } from "@/utils/message";
import { useColumnWidths } from "@/views/pipeline/columnWidth";
import { attrMap, attrRows, emptyAttr, type AttrRow } from "@/views/pipeline/gie/attrs";

const props = defineProps<{ gieId: string }>();
const emit = defineEmits<{ close: []; saved: [] }>();

const open = ref(true);
const name = ref("");
const interval = ref(GIE_INTERVAL_MIN);
const modelId = ref("");
const modelType = ref("");
const models = ref<{ id: string; name: string }[]>([]);
const classes = ref<string[]>([]);
const modelConf = ref<number | null>(null);
const modelMaxDet = ref<number | null>(null);
const attrs = ref<AttrRow[]>([emptyAttr("all")]);
const { target: attrRef, widths: attrWidths, observe: observeAttr } = useColumnWidths(GIE_ATTR_COLUMN_PERCENTS);
const confirmable = computed(() => Boolean(name.value.trim() && modelId.value && interval.value >= GIE_INTERVAL_MIN));
const hint = computed(() => {
  const unsupported = GIE_ATTR_UNSUPPORTED_TYPES.includes(modelType.value);
  const selectedOnly = !attrs.value.some(row => row.classKey === "all");
  const messages = {
    unsupported: transformI18n($t("pipeline.pureAttrUnsupported")).replace("{type}", modelType.value),
    selected: transformI18n($t("pipeline.pureAttrSelectedOnly")),
    none: ""
  };
  const key = unsupported ? "unsupported" : selectedOnly ? "selected" : "none";
  return messages[key];
});

const defaultAttr = (classKey: string): AttrRow => ({
  classKey,
  conf: modelConf.value,
  maxDet: modelMaxDet.value,
  minW: null,
  minH: null,
  maxW: null,
  maxH: null
});

const draft = (): GieDraft => ({
  model_id: modelId.value,
  name: name.value,
  interval: interval.value,
  class_attrs: attrMap(attrs.value)
});

const onModel = (id: string, keepAttrs = false) => {
  getModel(id).then(res => {
    const item = res.data;
    modelConf.value = item.conf === null ? null : Number(item.conf);
    modelMaxDet.value = item.max_det;
    modelType.value = item.type;
    classes.value = Array.isArray(item.classes) ? item.classes.map(String) : [];
    keepAttrs || (attrs.value = [defaultAttr("all")]);
  });
};

const onAddAttr = () => {
  attrs.value = [...attrs.value, defaultAttr("")];
};

const onRemoveAttr = (index: number) => {
  const next = attrs.value.filter((_, itemIndex) => itemIndex !== index);
  attrs.value = next.length ? next : [defaultAttr("all")];
};

const classTaken = (classKey: string, row: AttrRow) => attrs.value.some(item => item !== row && item.classKey === classKey);

const onConfirm = () => {
  const request = props.gieId ? updateGie(props.gieId, draft()) : createGie(draft());
  request.then(res => {
    message(res.message, { type: "success" });
    open.value = false;
    emit("saved");
  });
};

getModelMap().then(res => {
  models.value = Object.entries(res.data).map(([label, id]) => ({ id, name: label }));
});
props.gieId &&
  getGie(props.gieId).then(res => {
    const item = res.data;
    name.value = item.name;
    interval.value = item.interval;
    modelId.value = item.model_id;
    attrs.value = attrRows(item.class_attrs);
    attrs.value.length === 0 && (attrs.value = [emptyAttr("all")]);
    item.model_id && onModel(item.model_id, true);
  });

watch(open, value => {
  value ? nextTick(observeAttr) : emit("close");
});
nextTick(observeAttr);
</script>

<template>
  <el-dialog
    v-model="open"
    append-to-body
    class="create-dialog"
    :title="transformI18n($t(gieId ? 'pipeline.pureEdit' : 'pipeline.pureCreate'))"
    :width="GIE_FORM_DIALOG_WIDTH"
  >
    <el-form label-width="88px" label-position="left">
      <el-form-item :label="transformI18n($t('pipeline.pureName'))">
        <el-input v-model="name" />
      </el-form-item>
      <el-form-item :label="transformI18n($t('pipeline.pureInterval'))">
        <el-input-number v-model="interval" :min="GIE_INTERVAL_MIN" :precision="0" :step="1" />
      </el-form-item>
      <el-form-item :label="transformI18n($t('pipeline.pureModel'))">
        <el-select v-model="modelId" class="gie-model" @change="onModel">
          <el-option v-for="item in models" :key="item.id" :label="item.name" :value="item.id" />
        </el-select>
      </el-form-item>
    </el-form>
    <el-table ref="attrRef" class="create-table" :data="attrs" table-layout="fixed" border>
      <el-table-column :label="transformI18n($t('pipeline.pureClass'))" :width="attrWidths.classKey">
        <template #default="{ row }">
          <el-select v-model="row.classKey" :disabled="!modelId">
            <el-option :label="transformI18n($t('pipeline.pureAll'))" value="all" :disabled="classTaken('all', row)" />
            <el-option
              v-for="(label, index) in classes"
              :key="index"
              :label="label"
              :value="String(index)"
              :disabled="classTaken(String(index), row)"
            />
          </el-select>
        </template>
      </el-table-column>
      <el-table-column align="center" class-name="attr-num" :label="transformI18n($t('pipeline.pureConf'))" :width="attrWidths.conf">
        <template #default="{ row }">
          <el-input-number v-model="row.conf" :disabled="!modelId" :min="modelConf ?? 0" :max="1" :step="0.01" />
        </template>
      </el-table-column>
      <el-table-column align="center" class-name="attr-num" :label="transformI18n($t('pipeline.pureMaxDet'))" :width="attrWidths.maxDet">
        <template #default="{ row }">
          <el-input-number v-model="row.maxDet" :disabled="!modelId" :min="0" :max="modelMaxDet ?? undefined" :precision="0" :step="1" />
        </template>
      </el-table-column>
      <el-table-column align="center" class-name="attr-num" :label="transformI18n($t('pipeline.pureMinW'))" :width="attrWidths.minW">
        <template #default="{ row }">
          <el-input-number v-model="row.minW" :disabled="!modelId" :min="0" :precision="0" :step="1" />
        </template>
      </el-table-column>
      <el-table-column align="center" class-name="attr-num" :label="transformI18n($t('pipeline.pureMinH'))" :width="attrWidths.minH">
        <template #default="{ row }">
          <el-input-number v-model="row.minH" :disabled="!modelId" :min="0" :precision="0" :step="1" />
        </template>
      </el-table-column>
      <el-table-column align="center" class-name="attr-num" :label="transformI18n($t('pipeline.pureMaxW'))" :width="attrWidths.maxW">
        <template #default="{ row }">
          <el-input-number v-model="row.maxW" :disabled="!modelId" :min="0" :precision="0" :step="1" />
        </template>
      </el-table-column>
      <el-table-column align="center" class-name="attr-num" :label="transformI18n($t('pipeline.pureMaxH'))" :width="attrWidths.maxH">
        <template #default="{ row }">
          <el-input-number v-model="row.maxH" :disabled="!modelId" :min="0" :precision="0" :step="1" />
        </template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('pipeline.pureOps'))" :width="attrWidths.ops" :show-overflow-tooltip="false">
        <template #default="{ $index }">
          <el-button link type="primary" :disabled="!modelId" :title="transformI18n($t('pipeline.pureRemove'))" @click="onRemoveAttr($index)">
            <IconifyIconOffline :icon="DeleteBin" />
          </el-button>
        </template>
      </el-table-column>
    </el-table>
    <el-button class="add-row" :disabled="!modelId" @click="onAddAttr">+</el-button>
    <el-tooltip :content="hint" :disabled="!hint" placement="top">
      <p class="attr-hint">{{ hint }}</p>
    </el-tooltip>
    <template #footer>
      <el-button @click="open = false">{{ transformI18n($t("pipeline.pureCancel")) }}</el-button>
      <el-button type="primary" :disabled="!confirmable" @click="onConfirm">
        {{ transformI18n($t("pipeline.pureConfirm")) }}
      </el-button>
    </template>
  </el-dialog>
</template>

<style scoped>
.gie-model,
.create-table :deep(.el-input-number),
.create-table :deep(.el-select) {
  width: 100%;
}

.create-table :deep(.cell) {
  padding: 0 4px;
  white-space: nowrap;
}

.create-table :deep(.attr-num .el-input__inner) {
  text-align: center;
}

.add-row {
  width: 100%;
  margin-top: -1px;
  border-radius: 0;
}

.attr-hint {
  height: 20px;
  margin: 6px 0 0;
  overflow: hidden;
  font-size: 12px;
  line-height: 20px;
  color: var(--el-color-danger);
  white-space: nowrap;
  text-overflow: ellipsis;
  background: transparent;
}
</style>
