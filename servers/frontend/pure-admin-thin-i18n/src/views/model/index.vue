<script setup lang="ts">
import { computed, nextTick, onMounted, onUnmounted, ref, watch } from "vue";
import Hammer from "~icons/ri/hammer-line";
import Upload from "~icons/ri/upload-2-line";
import Edit from "~icons/ri/edit-line";
import DeleteBin from "~icons/ri/delete-bin-line";
import Search from "~icons/ri/search-line";
import ListCheck from "~icons/ri/list-check";
import {
  buildModel,
  createModels,
  getModelTypes,
  type ModelTypeDefaults,
  getModel,
  getModels,
  getModelStatus,
  patchModel,
  removeModel,
  removeModels,
  uploadModel,
  type ModelDraft,
  type ModelItem
} from "@/api/model";
import SearchHighlight from "@/components/SearchHighlight/index.vue";
import {
  MODEL_BATCH_MODES,
  MODEL_CLASSES_COLUMN_ALIGNS,
  MODEL_CLASSES_COLUMN_RESIZABLE,
  MODEL_CLASSES_DIALOG_WIDTH,
  MODEL_CLASSES_ID_COLUMN_WIDTH,
  MODEL_CLASSES_ROW_HEIGHT,
  MODEL_CLASSES_TABLE_MAX_HEIGHT,
  MODEL_COLUMN_ALIGNS,
  MODEL_CREATE_COLUMN_ALIGNS,
  MODEL_CREATE_COLUMN_PERCENTS,
  MODEL_CREATE_COLUMN_RESIZABLE,
  MODEL_DIALOG_WIDTH,
  MODEL_EDIT_COLUMN_ALIGNS,
  MODEL_EDIT_COLUMN_PERCENTS,
  MODEL_EDIT_COLUMN_RESIZABLE,
  MODEL_LIST_COLUMN_PERCENTS,
  MODEL_LIST_COLUMN_RESIZABLE,
  MODEL_PRECISIONS,
  MODEL_STATUS_LABEL,
  MODEL_ROW_HEIGHT,
  MODEL_STATUS_FIRST_MS,
  MODEL_STATUS_POLL_MS
} from "@/constants/model";
import { $t, transformI18n } from "@/plugins/i18n";
import { message } from "@/utils/message";
import { columnPixels } from "@/utils/columnWidth";
import { formatLocalTime } from "@/utils/time";

defineOptions({
  name: "ModelIndex"
});

const blankRow = (): ModelDraft => ({
  name: "",
  type: "",
  batch_size: 1,
  batch_mode: "dynamic",
  precision: MODEL_PRECISIONS[0],
  max_det: null,
  conf: null,
  iou: null,
  device: 0
});

const types = ref<Record<string, ModelTypeDefaults>>({});
const keyword = ref("");
const searched = ref("");
const fileInput = ref<HTMLInputElement>();
const uploadRow = ref<{ id?: string; type?: string } | null>(null);
const items = ref<ModelItem[]>([]);
const selected = ref<ModelItem[]>([]);
const visible = ref(false);
const editing = ref(false);
const editId = ref("");
const editItem = ref<ModelItem | null>(null);
const editRow = ref<ModelDraft & { source_name: string }>({ ...blankRow(), source_name: "" });
const rows = ref<ModelDraft[]>([blankRow()]);
const picked = ref<ModelDraft[]>([]);
const classesOpen = ref(false);
const classes = ref<{ id: number; name: string }[]>([]);
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

const editRef = ref();
const editWidth = ref(0);
const editObserver = new ResizeObserver(entries => {
  editWidth.value = Math.floor(entries[0].contentRect.width);
});

const listWidths = computed(() => columnPixels(listWidth.value, MODEL_LIST_COLUMN_PERCENTS));

const createWidths = computed(() => columnPixels(createWidth.value, MODEL_CREATE_COLUMN_PERCENTS));

const editWidths = computed(() => columnPixels(editWidth.value, MODEL_EDIT_COLUMN_PERCENTS));

const observeCreate = () => {
  const el = createRef.value?.$el as HTMLElement | undefined;
  el && createObserver.observe(el);
};

const observeEdit = () => {
  const el = editRef.value?.$el as HTMLElement | undefined;
  el && editObserver.observe(el);
};

const statusTimer = ref<number | null>(null);
const statusDelays = ref<number[]>([]);
const lockedIds = ref<string[]>([]);

const opsLocked = (row: { id?: string; status?: string }) =>
  row.status === "building" || lockedIds.value.includes(row.id ?? "");

const releaseLock = (id: string) => {
  lockedIds.value = lockedIds.value.filter(item => item !== id);
};

const stopStatusPoll = () => {
  statusTimer.value !== null && window.clearInterval(statusTimer.value);
  statusTimer.value = null;
};

const pullStatus = (id: string) => {
  getModelStatus(id).then(res => {
    const row = items.value.find(entry => entry.id === id);
    row && Object.assign(row, res.data);
    res.data.status === "building" ? startStatusPoll() : releaseLock(id);
  });
};

const pollStatuses = () => {
  const ids = [...new Set([
    ...items.value.filter(item => item.status === "building").map(item => item.id),
    ...lockedIds.value
  ])];
  ids.length === 0 && stopStatusPoll();
  ids.forEach(pullStatus);
};

const startStatusPoll = () => {
  statusTimer.value === null && (statusTimer.value = window.setInterval(pollStatuses, MODEL_STATUS_POLL_MS));
};

const loadModels = () => {
  searched.value = keyword.value;
  getModels({ page: 1, page_size: 20, search: keyword.value }).then(res => {
    items.value = res.data.items;
    res.data.items.some(item => item.status === "building") && startStatusPoll();
  });
};

const done = (messageText: string) => {
  message(messageText, { type: "success" });
  loadModels();
};

const onSelectModels = (value: ModelItem[]) => {
  selected.value = value;
};

const onRemoveBatch = () => {
  removeModels(selected.value.map(row => row.id)).then(res => done(res.message));
};

const onRemoveItem = (row: ModelItem) => {
  removeModel(row.id).then(res => done(res.message));
};

const onBuild = (row: { id?: string }) => {
  const id = row.id ?? "";
  id && !lockedIds.value.includes(id) && lockedIds.value.push(id);
  id && buildModel(id).then(res => done(res.message));
  id && statusDelays.value.push(window.setTimeout(() => pullStatus(id), MODEL_STATUS_FIRST_MS));
};

const onClasses = (row: ModelItem) => {
  const value = row.classes;
  classes.value = Array.isArray(value)
    ? value.map((item, index) => ({ id: index, name: String(item) }))
    : [];
  classesOpen.value = true;
};

const onEdit = (row: ModelItem) => {
  getModel(row.id).then(res => {
    const item = res.data;
    editItem.value = item;
    editId.value = item.id;
    editRow.value = {
      name: item.name,
      type: item.type,
      batch_size: item.batch_size,
      batch_mode: item.batch_mode,
      precision: item.precision,
      max_det: item.max_det,
      conf: item.conf,
      iou: item.iou,
      device: item.device,
      source_name: item.source_name ?? ""
    };
    editing.value = true;
    loadTypes();
  });
};

const onEditUpload = () => {
  const item = editItem.value;
  item && onUpload({ ...item, type: editRow.value.type });
};

const onEditCancel = () => {
  editing.value = false;
};

const onEditConfirm = () => {
  patchModel(editId.value, editRow.value).then(res => {
    editing.value = false;
    done(res.message);
  });
};

const typeNames = () => Object.keys(types.value);

const batchModeLabel = (value: string) => (value ? value[0].toUpperCase() + value.slice(1) : value);

const precisionLabel = (value: string) => value.toUpperCase();

const statusLabel = (value: string) => MODEL_STATUS_LABEL[value as keyof typeof MODEL_STATUS_LABEL] ?? value;

const statusClass = (value: string) => ({
  built: "model-status-built",
  building: "model-status-building",
  not_built: "model-status-not-built"
}[value] ?? "");

const locked = (type: string, key: "max_det" | "conf" | "iou") => types.value[type]?.[key] == null;

const fillType = <T extends ModelDraft>(row: T, type: string): T => {
  const defaults = types.value[type];
  return defaults
    ? {
        ...row,
        type,
        batch_size: defaults.batch_size,
        batch_mode: defaults.batch_mode,
        precision: defaults.precision,
        max_det: defaults.max_det,
        conf: defaults.conf,
        iou: defaults.iou
      }
    : { ...row, type };
};

const loadTypes = () => {
  getModelTypes().then(res => {
    types.value = res.data;
    const fallback = typeNames()[0] ?? "";
    rows.value = rows.value.map(row => fillType(row, row.type || fallback));
  });
};

const onCreate = () => {
  rows.value = [blankRow()];
  picked.value = [];
  visible.value = true;
  loadTypes();
};

const onType = (row: ModelDraft, type: string) => {
  Object.assign(row, fillType(row, type));
};

const onAdd = () => {
  const row = fillType(blankRow(), typeNames()[0] ?? "");
  rows.value = [...rows.value, row];
};

const onRemove = (index: number) => {
  const next = rows.value.filter((_, itemIndex) => itemIndex !== index);
  rows.value = next.length ? next : [fillType(blankRow(), typeNames()[0] ?? "")];
};

const onSelect = (value: ModelDraft[]) => {
  picked.value = value;
};

const onRemoveSelected = () => {
  const next = rows.value.filter(row => !picked.value.includes(row));
  rows.value = next.length ? next : [fillType(blankRow(), typeNames()[0] ?? "")];
  picked.value = [];
};

const onCancel = () => {
  visible.value = false;
};

const createReady = computed(() => rows.value.every(row => row.name.trim()));

const onConfirm = () => {
  const drafts = rows.value.filter(row => row.name);
  createModels(drafts).then(res => {
    message(res.message, { type: res.success ? "success" : "warning" });
    visible.value = false;
    loadModels();
  });
};

const onUpload = (row: { id?: string; type?: string } | null) => {
  uploadRow.value = row;
  const input = fileInput.value;
  if (input) {
    input.accept = (types.value[row?.type ?? ""]?.source_suffixes ?? []).join(",");
    input.click();
  }
};

const onFile = (event: Event) => {
  const input = event.target as HTMLInputElement;
  const file = input.files?.[0] ?? null;
  const id = uploadRow.value?.id ?? "";
  input.value = "";
  uploadRow.value = null;
  id && file && uploadModel(id, file).then(res => done(res.message));
};

watch(visible, value => {
  value && nextTick(observeCreate);
});

watch(editing, value => {
  value && nextTick(observeEdit);
});

onMounted(() => {
  loadModels();
  const el = listRef.value?.$el as HTMLElement | undefined;
  el && listObserver.observe(el);
});

onUnmounted(() => {
  statusDelays.value.forEach(window.clearTimeout);
  stopStatusPoll();
  listObserver.disconnect();
  createObserver.disconnect();
  editObserver.disconnect();
});
</script>

<template>
  <div class="model-page">
    <div class="model-toolbar">
      <el-button type="primary" @click="onCreate">
        {{ transformI18n($t("model.pureCreate")) }}
      </el-button>
      <el-button type="primary" :disabled="!selected.length" @click="onRemoveBatch">
        {{ transformI18n($t("model.pureRemove")) }}
      </el-button>
      <el-input
        v-model="keyword"
        class="model-search"
        clearable
        :placeholder="transformI18n($t('model.pureSearch'))"
        @keyup.enter="loadModels"
        @clear="loadModels"
      >
        <template #prefix>
          <IconifyIconOffline :icon="Search" />
        </template>
      </el-input>
    </div>
    <el-dialog
      v-model="visible"
      class="create-dialog"
      :title="transformI18n($t('model.pureCreate'))"
      :width="MODEL_DIALOG_WIDTH"
    >
      <div class="model-toolbar">
        <el-button type="primary" @click="onRemoveSelected">
          {{ transformI18n($t("model.pureRemove")) }}
        </el-button>
      </div>
      <el-table ref="createRef" class="create-table" :data="rows" table-layout="fixed" border show-overflow-tooltip @selection-change="onSelect">
        <el-table-column type="selection" :resizable="MODEL_CREATE_COLUMN_RESIZABLE" :width="createWidths.selection" :header-align="MODEL_CREATE_COLUMN_ALIGNS.selection.header" :align="MODEL_CREATE_COLUMN_ALIGNS.selection.content" />
        <el-table-column :label="transformI18n($t('model.pureName'))" :resizable="MODEL_CREATE_COLUMN_RESIZABLE" :width="createWidths.name" :header-align="MODEL_CREATE_COLUMN_ALIGNS.name.header" :align="MODEL_CREATE_COLUMN_ALIGNS.name.content">
          <template #default="{ row }">
            <el-input v-model="row.name" />
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('model.pureType'))" :resizable="MODEL_CREATE_COLUMN_RESIZABLE" :width="createWidths.type" :header-align="MODEL_CREATE_COLUMN_ALIGNS.type.header" :align="MODEL_CREATE_COLUMN_ALIGNS.type.content">
          <template #default="{ row }">
            <el-select v-model="row.type" @change="onType(row, $event)">
              <el-option v-for="item in typeNames()" :key="item" :label="item" :value="item" />
            </el-select>
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('model.pureBatchSize'))" :resizable="MODEL_CREATE_COLUMN_RESIZABLE" :width="createWidths.batchSize" :header-align="MODEL_CREATE_COLUMN_ALIGNS.batchSize.header" :align="MODEL_CREATE_COLUMN_ALIGNS.batchSize.content">
          <template #default="{ row }">
            <el-input v-model.number="row.batch_size" />
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('model.pureBatchMode'))" :resizable="MODEL_CREATE_COLUMN_RESIZABLE" :width="createWidths.batchMode" :header-align="MODEL_CREATE_COLUMN_ALIGNS.batchMode.header" :align="MODEL_CREATE_COLUMN_ALIGNS.batchMode.content">
          <template #default="{ row }">
            <el-select v-model="row.batch_mode">
              <el-option v-for="item in MODEL_BATCH_MODES" :key="item" :label="batchModeLabel(item)" :value="item" />
            </el-select>
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('model.purePrecision'))" :resizable="MODEL_CREATE_COLUMN_RESIZABLE" :width="createWidths.precision" :header-align="MODEL_CREATE_COLUMN_ALIGNS.precision.header" :align="MODEL_CREATE_COLUMN_ALIGNS.precision.content">
          <template #default="{ row }">
            <el-select v-model="row.precision">
              <el-option v-for="item in MODEL_PRECISIONS" :key="item" :label="precisionLabel(item)" :value="item" />
            </el-select>
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('model.pureMaxDet'))" :resizable="MODEL_CREATE_COLUMN_RESIZABLE" :width="createWidths.maxDet" :header-align="MODEL_CREATE_COLUMN_ALIGNS.maxDet.header" :align="MODEL_CREATE_COLUMN_ALIGNS.maxDet.content">
          <template #default="{ row }">
            <el-input v-model.number="row.max_det" :disabled="locked(row.type, 'max_det')" />
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('model.pureConf'))" :resizable="MODEL_CREATE_COLUMN_RESIZABLE" :width="createWidths.conf" :header-align="MODEL_CREATE_COLUMN_ALIGNS.conf.header" :align="MODEL_CREATE_COLUMN_ALIGNS.conf.content">
          <template #default="{ row }">
            <el-input v-model.number="row.conf" :disabled="locked(row.type, 'conf')" />
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('model.pureIou'))" :resizable="MODEL_CREATE_COLUMN_RESIZABLE" :width="createWidths.iou" :header-align="MODEL_CREATE_COLUMN_ALIGNS.iou.header" :align="MODEL_CREATE_COLUMN_ALIGNS.iou.content">
          <template #default="{ row }">
            <el-input v-model.number="row.iou" :disabled="locked(row.type, 'iou')" />
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('model.pureOps'))" :resizable="MODEL_CREATE_COLUMN_RESIZABLE" :width="createWidths.ops" :show-overflow-tooltip="false" :header-align="MODEL_CREATE_COLUMN_ALIGNS.ops.header" :align="MODEL_CREATE_COLUMN_ALIGNS.ops.content">
          <template #default="{ $index }">
            <el-button link type="primary" :title="transformI18n($t('model.pureRemove'))" @click="onRemove($index)">
              <IconifyIconOffline :icon="DeleteBin" />
            </el-button>
          </template>
        </el-table-column>
      </el-table>
      <el-button class="add-row" @click="onAdd">+</el-button>
      <template #footer>
        <el-button @click="onCancel">
          {{ transformI18n($t("model.pureCancel")) }}
        </el-button>
        <el-button type="primary" :disabled="!createReady" @click="onConfirm">
          {{ transformI18n($t("model.pureConfirm")) }}
        </el-button>
      </template>
    </el-dialog>
    <el-dialog
      v-model="editing"
      class="create-dialog"
      :title="transformI18n($t('model.pureEdit'))"
      :width="MODEL_DIALOG_WIDTH"
    >
      <el-table ref="editRef" class="create-table" :data="[editRow]" table-layout="fixed" border show-overflow-tooltip>
        <el-table-column :label="transformI18n($t('model.pureName'))" :resizable="MODEL_EDIT_COLUMN_RESIZABLE" :width="editWidths.name" :header-align="MODEL_EDIT_COLUMN_ALIGNS.name.header" :align="MODEL_EDIT_COLUMN_ALIGNS.name.content">
          <template #default="{ row }"><el-input v-model="row.name" /></template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('model.pureSource'))" :resizable="MODEL_EDIT_COLUMN_RESIZABLE" :width="editWidths.source" :header-align="MODEL_EDIT_COLUMN_ALIGNS.source.header" :align="MODEL_EDIT_COLUMN_ALIGNS.source.content">
          <template #default="{ row }">{{ row.source_name }}</template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('model.pureType'))" :resizable="MODEL_EDIT_COLUMN_RESIZABLE" :width="editWidths.type" :header-align="MODEL_EDIT_COLUMN_ALIGNS.type.header" :align="MODEL_EDIT_COLUMN_ALIGNS.type.content">
          <template #default="{ row }">
            <el-select v-model="row.type" @change="onType(row, $event)">
              <el-option v-for="item in typeNames()" :key="item" :label="item" :value="item" />
            </el-select>
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('model.pureBatchSize'))" :resizable="MODEL_EDIT_COLUMN_RESIZABLE" :width="editWidths.batchSize" :header-align="MODEL_EDIT_COLUMN_ALIGNS.batchSize.header" :align="MODEL_EDIT_COLUMN_ALIGNS.batchSize.content">
          <template #default="{ row }"><el-input v-model.number="row.batch_size" /></template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('model.pureBatchMode'))" :resizable="MODEL_EDIT_COLUMN_RESIZABLE" :width="editWidths.batchMode" :header-align="MODEL_EDIT_COLUMN_ALIGNS.batchMode.header" :align="MODEL_EDIT_COLUMN_ALIGNS.batchMode.content">
          <template #default="{ row }">
            <el-select v-model="row.batch_mode">
              <el-option v-for="item in MODEL_BATCH_MODES" :key="item" :label="batchModeLabel(item)" :value="item" />
            </el-select>
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('model.purePrecision'))" :resizable="MODEL_EDIT_COLUMN_RESIZABLE" :width="editWidths.precision" :header-align="MODEL_EDIT_COLUMN_ALIGNS.precision.header" :align="MODEL_EDIT_COLUMN_ALIGNS.precision.content">
          <template #default="{ row }">
            <el-select v-model="row.precision">
              <el-option v-for="item in MODEL_PRECISIONS" :key="item" :label="precisionLabel(item)" :value="item" />
            </el-select>
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('model.pureMaxDet'))" :resizable="MODEL_EDIT_COLUMN_RESIZABLE" :width="editWidths.maxDet" :header-align="MODEL_EDIT_COLUMN_ALIGNS.maxDet.header" :align="MODEL_EDIT_COLUMN_ALIGNS.maxDet.content">
          <template #default="{ row }">
            <el-input v-model.number="row.max_det" :disabled="locked(row.type, 'max_det')" />
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('model.pureConf'))" :resizable="MODEL_EDIT_COLUMN_RESIZABLE" :width="editWidths.conf" :header-align="MODEL_EDIT_COLUMN_ALIGNS.conf.header" :align="MODEL_EDIT_COLUMN_ALIGNS.conf.content">
          <template #default="{ row }">
            <el-input v-model.number="row.conf" :disabled="locked(row.type, 'conf')" />
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('model.pureIou'))" :resizable="MODEL_EDIT_COLUMN_RESIZABLE" :width="editWidths.iou" :header-align="MODEL_EDIT_COLUMN_ALIGNS.iou.header" :align="MODEL_EDIT_COLUMN_ALIGNS.iou.content">
          <template #default="{ row }">
            <el-input v-model.number="row.iou" :disabled="locked(row.type, 'iou')" />
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('model.pureOps'))" :resizable="MODEL_EDIT_COLUMN_RESIZABLE" :width="editWidths.ops" :show-overflow-tooltip="false" :header-align="MODEL_EDIT_COLUMN_ALIGNS.ops.header" :align="MODEL_EDIT_COLUMN_ALIGNS.ops.content">
          <template #default>
            <el-button link type="primary" :title="transformI18n($t('model.pureUpload'))" @click="onEditUpload">
              <IconifyIconOffline :icon="Upload" />
            </el-button>
          </template>
        </el-table-column>
      </el-table>
      <template #footer>
        <el-button @click="onEditCancel">
          {{ transformI18n($t("model.pureCancel")) }}
        </el-button>
        <el-button type="primary" @click="onEditConfirm">
          {{ transformI18n($t("model.pureConfirm")) }}
        </el-button>
      </template>
    </el-dialog>
    <el-dialog v-model="classesOpen" :title="transformI18n($t('model.pureClasses'))" :width="MODEL_CLASSES_DIALOG_WIDTH">
      <el-table class="classes-table" :data="classes" border :max-height="MODEL_CLASSES_TABLE_MAX_HEIGHT" show-overflow-tooltip>
        <el-table-column :label="transformI18n($t('model.pureClassId'))" prop="id" :resizable="MODEL_CLASSES_COLUMN_RESIZABLE" :width="MODEL_CLASSES_ID_COLUMN_WIDTH" :header-align="MODEL_CLASSES_COLUMN_ALIGNS.id.header" :align="MODEL_CLASSES_COLUMN_ALIGNS.id.content" />
        <el-table-column :label="transformI18n($t('model.pureClassName'))" prop="name" :resizable="MODEL_CLASSES_COLUMN_RESIZABLE" :header-align="MODEL_CLASSES_COLUMN_ALIGNS.name.header" :align="MODEL_CLASSES_COLUMN_ALIGNS.name.content" />
      </el-table>
    </el-dialog>
    <el-table ref="listRef" class="model-list" :data="items" table-layout="fixed" border show-overflow-tooltip @selection-change="onSelectModels">
      <el-table-column type="selection" :resizable="MODEL_LIST_COLUMN_RESIZABLE" :width="listWidths.selection" :header-align="MODEL_COLUMN_ALIGNS.selection.header" :align="MODEL_COLUMN_ALIGNS.selection.content" />
      <el-table-column :label="transformI18n($t('model.pureName'))" :resizable="MODEL_LIST_COLUMN_RESIZABLE" :width="listWidths.name" :header-align="MODEL_COLUMN_ALIGNS.name.header" :align="MODEL_COLUMN_ALIGNS.name.content">
        <template #default="{ row }"><SearchHighlight :text="row.name" :query="searched" /></template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('model.pureSource'))" :resizable="MODEL_LIST_COLUMN_RESIZABLE" :width="listWidths.source" :header-align="MODEL_COLUMN_ALIGNS.source.header" :align="MODEL_COLUMN_ALIGNS.source.content">
        <template #default="{ row }"><SearchHighlight :text="row.source_name" :query="searched" /></template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('model.pureType'))" :resizable="MODEL_LIST_COLUMN_RESIZABLE" :width="listWidths.type" :header-align="MODEL_COLUMN_ALIGNS.type.header" :align="MODEL_COLUMN_ALIGNS.type.content">
        <template #default="{ row }"><SearchHighlight :text="row.type" :query="searched" /></template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('model.pureBatchSize'))" :resizable="MODEL_LIST_COLUMN_RESIZABLE" :width="listWidths.batchSize" :header-align="MODEL_COLUMN_ALIGNS.batchSize.header" :align="MODEL_COLUMN_ALIGNS.batchSize.content">
        <template #default="{ row }"><SearchHighlight :text="row.batch_size" :query="searched" /></template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('model.pureBatchMode'))" :resizable="MODEL_LIST_COLUMN_RESIZABLE" :width="listWidths.batchMode" :header-align="MODEL_COLUMN_ALIGNS.batchMode.header" :align="MODEL_COLUMN_ALIGNS.batchMode.content">
        <template #default="{ row }"><SearchHighlight :text="batchModeLabel(row.batch_mode)" :query="searched" /></template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('model.purePrecision'))" :resizable="MODEL_LIST_COLUMN_RESIZABLE" :width="listWidths.precision" :header-align="MODEL_COLUMN_ALIGNS.precision.header" :align="MODEL_COLUMN_ALIGNS.precision.content">
        <template #default="{ row }"><SearchHighlight :text="precisionLabel(row.precision)" :query="searched" /></template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('model.pureClasses'))" :resizable="MODEL_LIST_COLUMN_RESIZABLE" :width="listWidths.classes" :show-overflow-tooltip="false" :header-align="MODEL_COLUMN_ALIGNS.classes.header" :align="MODEL_COLUMN_ALIGNS.classes.content">
        <template #default="{ row }">
          <el-button link type="primary" :title="transformI18n($t('model.pureClasses'))" @click="onClasses(row)">
            <IconifyIconOffline :icon="ListCheck" />
          </el-button>
        </template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('model.pureStatus'))" :resizable="MODEL_LIST_COLUMN_RESIZABLE" :width="listWidths.status" :header-align="MODEL_COLUMN_ALIGNS.status.header" :align="MODEL_COLUMN_ALIGNS.status.content">
        <template #default="{ row }">
          <span :class="statusClass(row.status)">
            <SearchHighlight :text="statusLabel(row.status)" :query="searched" />
          </span>
        </template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('model.pureBuiltAt'))" :resizable="MODEL_LIST_COLUMN_RESIZABLE" :width="listWidths.builtAt" :header-align="MODEL_COLUMN_ALIGNS.builtAt.header" :align="MODEL_COLUMN_ALIGNS.builtAt.content">
        <template #default="{ row }"><SearchHighlight :text="formatLocalTime(row.built_at)" :query="searched" /></template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('model.pureOps'))" :resizable="MODEL_LIST_COLUMN_RESIZABLE" :width="listWidths.ops" :show-overflow-tooltip="false" :header-align="MODEL_COLUMN_ALIGNS.ops.header" :align="MODEL_COLUMN_ALIGNS.ops.content">
        <template #default="{ row }">
          <el-button link type="primary" :disabled="!row.source_path || opsLocked(row)" :title="transformI18n($t('model.pureBuild'))" @click="onBuild(row)">
            <IconifyIconOffline :icon="Hammer" />
          </el-button>
          <el-button link type="primary" :disabled="opsLocked(row)" :title="transformI18n($t('model.pureUpload'))" @click="onUpload(row)">
            <IconifyIconOffline :icon="Upload" />
          </el-button>
          <el-button link type="primary" :disabled="opsLocked(row)" :title="transformI18n($t('model.pureEdit'))" @click="onEdit(row)">
            <IconifyIconOffline :icon="Edit" />
          </el-button>
          <el-button link type="primary" :disabled="opsLocked(row)" :title="transformI18n($t('model.pureRemove'))" @click="onRemoveItem(row)">
            <IconifyIconOffline :icon="DeleteBin" />
          </el-button>
        </template>
      </el-table-column>
    </el-table>
    <input ref="fileInput" class="upload-input" type="file" @change="onFile" />
  </div>
</template>

<style scoped>
.model-page.main-content {
  display: flex;
  flex: 1;
  flex-direction: column;
  min-width: 0;
  min-height: 0;
  margin: 0;
  overflow: hidden;
  background: var(--el-bg-color);
}

.model-toolbar {
  display: flex;
  flex: none;
  gap: 8px;
  align-items: center;
  padding: 8px;
  background: var(--el-bg-color);
  border-bottom: 1px solid var(--el-border-color);
}

.model-toolbar :deep(.el-button) {
  margin: 0;
}

.model-search {
  width: 220px;
  margin-left: auto;
}

.model-list {
  flex: 1;
  width: 100%;
  min-height: 0;
}

.model-list :deep(.cell) {
  word-break: normal;
  white-space: nowrap;
}

.model-status-built {
  color: var(--el-color-success);
}

.model-status-building {
  color: var(--el-color-warning);
}

.model-status-not-built {
  color: var(--el-color-danger);
}

.model-list :deep(.el-button) {
  margin: 0;
  font-size: 16px;
}

.classes-table :deep(.el-table__cell) {
  height: v-bind('MODEL_CLASSES_ROW_HEIGHT + "px"');
  padding-top: 0;
  padding-bottom: 0;
}

.create-table :deep(.el-table__cell) {
  height: v-bind('MODEL_ROW_HEIGHT + "px"');
  padding-top: 0;
  padding-bottom: 0;
}

.create-table :deep(.cell) {
  padding: 0 4px;
  word-break: normal;
  white-space: nowrap;
}

.create-table :deep(.el-button) {
  margin: 0;
  font-size: 16px;
}

.create-table :deep(.is-center .el-input__inner),
.create-table :deep(.is-center .el-select__placeholder),
.create-table :deep(.is-center .el-select__selected-item) {
  text-align: center;
  justify-content: center;
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

.upload-input {
  display: none;
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
