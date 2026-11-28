<script setup lang="ts">
import { computed, nextTick, onUnmounted, ref, watch } from "vue";
import { getUploadFiles, type MediaItem } from "@/api/media";
import { getModels, type ModelItem } from "@/api/model";
import {
  createPipeline,
  getPipeline,
  getPipelineMap,
  getPipelineTypes,
  updatePipeline,
  type PipelineDetail,
  type PipelineFields,
  type PipelineScalar,
  type PipelineSpec
} from "@/api/pipeline";
import { getGies, type GieItem } from "@/api/pipelineGie";
import { getStreams, type StreamItem } from "@/api/stream";
import {
  PIPELINE_CREATE_DIALOG_WIDTH,
  PIPELINE_OPTION_PAGE_SIZE,
  PIPELINE_SOURCE_COLUMN_PERCENTS,
  PIPELINE_SOURCE_PAGE_SIZE,
  PIPELINE_TIMED_SOURCE_COLUMN_PERCENTS
} from "@/constants/pipeline";
import { $t, transformI18n } from "@/plugins/i18n";
import { columnPixels } from "@/utils/columnWidth";
import { message } from "@/utils/message";

const props = defineProps<{ pipelineId: string }>();
const emit = defineEmits<{ close: []; created: [] }>();
const open = ref(true);

type PipelineSection = PipelineFields | Record<string, PipelineFields>;

type SourceRow = {
  id: string;
  name: string;
  width: number | null;
  height: number | null;
  fps: number | null;
  path: string;
  url: string;
  analyzer: string | null;
};

const SECTION_KEYS = ["collector", "drawer", "sahi", "event_coder", "capturer"] as const;

const FIELD_LABELS: Record<string, string> = {
  collector: "pipeline.pureCollector",
  drawer: "pipeline.pureDrawer",
  sahi: "pipeline.pureSahi",
  preprocess: "pipeline.purePreprocess",
  postprocess: "pipeline.purePostprocess",
  event_coder: "pipeline.pureEventCoder",
  capturer: "pipeline.pureCapturer",
  tracker: "pipeline.pureTracker",
  class_id: "pipeline.pureClassId",
  pgie: "pipeline.purePgie",
  sgie: "pipeline.pureSgie",
  stgcnpp: "pipeline.pureStgcnpp",
  interval: "pipeline.pureInterval",
  fade_time: "pipeline.pureFadeTime",
  show_label: "pipeline.pureShowLabel",
  show_mask: "pipeline.pureShowMask",
  show_pose: "pipeline.pureShowPose",
  pose_threshold: "pipeline.purePoseThreshold",
  show_snap: "pipeline.pureShowSnap",
  slice_width: "pipeline.pureSliceWidth",
  slice_height: "pipeline.pureSliceHeight",
  overlap_width_ratio: "pipeline.pureOverlapWidthRatio",
  overlap_height_ratio: "pipeline.pureOverlapHeightRatio",
  enable_full_frame: "pipeline.pureEnableFullFrame",
  match_metric: "pipeline.pureMatchMetric",
  match_threshold: "pipeline.pureMatchThreshold",
  class_agnostic: "pipeline.pureClassAgnostic",
  enable_merge: "pipeline.pureEnableMerge",
  two_phase_nmm: "pipeline.pureTwoPhaseNmm",
  prefer_full_frame: "pipeline.purePreferFullFrame",
  max_detections: "pipeline.pureMaxDetections",
  oks_threshold: "pipeline.pureOksThreshold",
  vis_threshold: "pipeline.pureVisThreshold",
  num_keypoints: "pipeline.pureNumKeypoints",
  event_slot_map: "pipeline.pureEventSlotMap",
  length: "pipeline.pureLength",
  threshold: "pipeline.pureThreshold",
  mode: "pipeline.pureMode",
  dump_codes: "pipeline.pureDumpCodes",
  capturer_output: "pipeline.pureCapturerOutput",
  label_task: "pipeline.pureLabelTask"
};

const fieldLabel = (key: string) => {
  const path = FIELD_LABELS[key];
  const label = path ? transformI18n($t(path)) : key;
  return label;
};

const isFlat = (value: PipelineSection): value is PipelineFields =>
  Object.values(value).every(item => typeof item !== "object");

const numberStep = (value: PipelineScalar) => (typeof value === "number" && Number.isInteger(value) ? 1 : 0.1);

const collectPages = async <T>(
  load: (page: number) => Promise<{ data: { items: T[]; total: number; page_size: number } }>
) => {
  const first = await load(1);
  const count = Math.ceil(first.data.total / first.data.page_size);
  const rest = await Promise.all(
    Array.from({ length: Math.max(count - 1, 0) }, (_, index) => load(index + 2))
  );
  const rows = [first, ...rest].flatMap(result => result.data.items);
  return rows;
};

const creating = ref(false);
const createType = ref("");
const createName = ref("");
const pipelineTypes = ref<Record<string, PipelineSpec>>({});
const pipelineNames = ref<Record<string, string>>({});
const modelChoices = ref<ModelItem[]>([]);
const gieChoices = ref<GieItem[]>([]);
const sections = ref<Record<string, PipelineSection>>({});
const giePicks = ref<Record<string, string>>({});
const classId = ref<number | null>(null);
const sourceRef = ref();
const sourceWidth = ref(0);
const sourceObserver = new ResizeObserver(entries => {
  sourceWidth.value = Math.floor(entries[0].contentRect.width);
});
const sourcePage = ref(1);
const sourceTotal = ref(0);
const sourceRows = ref<SourceRow[]>([]);
const sourceSelected = ref<SourceRow[]>([]);
let sourceSyncing = false;

const boolOptions = computed(() => [
  { label: transformI18n($t("pipeline.pureOff")), value: false },
  { label: transformI18n($t("pipeline.pureOn")), value: true }
]);
const currentSpec = computed(() => pipelineTypes.value[createType.value]);
const sectionKeys = computed(() => Object.keys(sections.value));
const gieSlots = computed(() => Object.keys(currentSpec.value?.gie ?? {}));
const imageSourceWidths = computed(() => columnPixels(sourceWidth.value, PIPELINE_SOURCE_COLUMN_PERCENTS));
const timedSourceWidths = computed(() => columnPixels(sourceWidth.value, PIPELINE_TIMED_SOURCE_COLUMN_PERCENTS));
const sourceWidths = computed(() =>
  currentSpec.value?.source === "image" ? imageSourceWidths.value : timedSourceWidths.value
);
const nameTaken = computed(() => {
  const owner = pipelineNames.value[createName.value.trim()];
  const taken = Boolean(owner && owner !== props.pipelineId);
  return taken;
});
const classOptions = computed(() => {
  const gie = gieChoices.value.find(item => item.id === giePicks.value.pgie);
  const model = modelChoices.value.find(item => item.id === gie?.model_id);
  const names = Array.isArray(model?.classes) ? model.classes.map(item => String(item)) : [];
  const keys = Object.keys(gie?.class_attrs ?? {});
  const ids = keys.includes("all")
    ? names.map((_, index) => index)
    : keys.map(Number).filter(item => Number.isInteger(item) && item >= 0);
  const options = [
    { id: -1, label: transformI18n($t("pipeline.pureAll")) },
    ...ids.map(id => ({ id, label: names[id] ?? String(id) }))
  ];
  return options;
});
const createReady = computed(() => {
  const spec = currentSpec.value;
  const name = createName.value.trim();
  const gieReady = gieSlots.value.every(slot => giePicks.value[slot]);
  const trackerReady = !spec?.tracker || classId.value !== null;
  const ready = Boolean(spec && name && !nameTaken.value && gieReady && trackerReady && sourceSelected.value.length);
  return ready;
});

const mediaRow = (item: MediaItem): SourceRow => ({
  id: item.id,
  name: item.name,
  width: item.width,
  height: item.height,
  fps: item.fps,
  path: `${item.folder.replace(/\/$/, "")}/${item.name}`,
  url: "",
  analyzer: null
});

const streamRow = (item: StreamItem): SourceRow => ({
  id: item.id,
  name: item.name,
  width: item.width,
  height: item.height,
  fps: item.fps,
  path: "",
  url: item.url,
  analyzer: null
});

const sameFrame = (left: SourceRow, right: SourceRow) => {
  const size = left.width === right.width && left.height === right.height;
  const rate = currentSpec.value?.source !== "rtsp" || left.fps === right.fps;
  const matched = size && rate;
  return matched;
};

const sourceSelectable = (row: SourceRow) => {
  const sized = row.width !== null && row.height !== null;
  const rated = currentSpec.value?.source !== "rtsp" || row.fps !== null;
  const anchor = sourceSelected.value[0];
  const matched = !anchor || anchor.id === row.id || sameFrame(anchor, row);
  const allowed = sized && rated && matched;
  return allowed;
};

const gieOptions = (slot: string) => {
  const allow = currentSpec.value?.gie[slot] ?? {};
  const allowedIds = new Set(modelChoices.value.filter(model => allow[model.type]).map(model => model.id));
  const options = gieChoices.value.filter(gie => {
    const attrs = slot !== "pgie" || Object.keys(gie.class_attrs ?? {}).length > 0;
    const usable = Boolean(gie.engine_path) && allowedIds.has(gie.model_id) && attrs;
    return usable;
  });
  return options;
};

const markSources = (rows: SourceRow[]) => {
  const picked = new Set(sourceSelected.value.map(row => row.id));
  nextTick(() => {
    sourceSyncing = true;
    rows.forEach(row => picked.has(row.id) && sourceRef.value?.toggleRowSelection(row, true));
    sourceSyncing = false;
  });
};

const loadSources = () => {
  const spec = currentSpec.value;
  const media = Boolean(spec && spec.source !== "rtsp");
  const streams = spec?.source === "rtsp";
  media &&
    spec &&
    getUploadFiles({ page: sourcePage.value, page_size: PIPELINE_SOURCE_PAGE_SIZE, type: spec.source }).then(res => {
      const rows = res.data.items.map(mediaRow);
      sourceRows.value = rows;
      sourceTotal.value = res.data.total;
      markSources(rows);
    });
  streams &&
    getStreams({ page: sourcePage.value, page_size: PIPELINE_SOURCE_PAGE_SIZE }).then(res => {
      const rows = res.data.items.map(streamRow);
      sourceRows.value = rows;
      sourceTotal.value = res.data.total;
      markSources(rows);
    });
};

const applyType = () => {
  const spec = currentSpec.value;
  const next: Record<string, PipelineSection> = {};
  spec &&
    SECTION_KEYS.forEach(key => {
      const value = spec[key];
      value && (next[key] = JSON.parse(JSON.stringify(value)) as PipelineSection);
    });
  sections.value = next;
  giePicks.value = Object.fromEntries(Object.keys(spec?.gie ?? {}).map(slot => [slot, ""]));
  classId.value = null;
  sourcePage.value = 1;
  sourceSelected.value = [];
  sourceRef.value?.clearSelection();
  loadSources();
};

const onPipelineCreate = () => {
  createType.value = "";
  createName.value = "";
  sections.value = {};
  giePicks.value = {};
  classId.value = null;
  sourceRows.value = [];
  sourceSelected.value = [];
  sourceTotal.value = 0;
  open.value = true;
  Promise.all([
    getPipelineTypes(),
    getPipelineMap(),
    collectPages(page => getModels({ page, page_size: PIPELINE_OPTION_PAGE_SIZE })),
    collectPages(page => getGies({ page, page_size: PIPELINE_OPTION_PAGE_SIZE }))
  ]).then(([types, map, models, gies]) => {
    pipelineTypes.value = types.data;
    pipelineNames.value = map.data;
    modelChoices.value = models;
    gieChoices.value = gies;
    props.pipelineId &&
      getPipeline(props.pipelineId).then(res => {
        fillPipeline(res.data);
      });
  });
};

const fillPipeline = (item: PipelineDetail) => {
  const input = item.input;
  createName.value = item.name;
  createType.value = item.type;
  applyType();
  const spec = currentSpec.value;
  spec &&
    SECTION_KEYS.forEach(key => {
      const value = input[key];
      value && typeof value === "object" && (sections.value[key] = value as PipelineSection);
    });
  giePicks.value = Object.fromEntries(
    Object.keys(spec?.gie ?? {}).map(slot => [slot, typeof input[slot] === "string" ? (input[slot] as string) : ""])
  );
  const tracker = input.tracker as { class_id?: number } | null;
  classId.value = tracker && typeof tracker.class_id === "number" ? tracker.class_id : null;
  const load =
    spec?.source === "rtsp"
      ? collectPages(page => getStreams({ page, page_size: PIPELINE_SOURCE_PAGE_SIZE }))
      : spec
        ? collectPages(page => getUploadFiles({ page, page_size: PIPELINE_SOURCE_PAGE_SIZE, type: spec.source }))
        : Promise.resolve([]);
  load.then(rows => {
    const mapped = spec?.source === "rtsp" ? (rows as StreamItem[]).map(streamRow) : (rows as MediaItem[]).map(mediaRow);
    const sourceKey = spec?.source === "image" ? "images" : spec?.source === "video" ? "videos" : "streams";
    const saved = (input[sourceKey] ?? []) as { id?: string; analyzer?: string | null }[];
    const byId = new Map(mapped.map(row => [row.id, row]));
    sourceSelected.value = saved.flatMap(item => {
      const row = item.id ? byId.get(item.id) : undefined;
      const selected = row ? [{ ...row, analyzer: item.analyzer ?? null }] : [];
      return selected;
    });
    markSources(sourceRows.value);
  });
};

const onSourcePage = (page: number) => {
  sourcePage.value = page;
  loadSources();
};

const onSourceSelect = (rows: SourceRow[]) => {
  const anchor = sourceSyncing ? undefined : rows[0];
  const kept = anchor ? rows.filter(row => sameFrame(anchor, row)) : [];
  const dropped = anchor ? rows.filter(row => !kept.some(item => item.id === row.id)) : [];
  anchor &&
    (sourceSelected.value = kept.map(row => ({
      ...row,
      analyzer: sourceSelected.value.find(item => item.id === row.id)?.analyzer ?? null
    })));
  sourceSyncing = true;
  dropped.forEach(row => sourceRef.value?.toggleRowSelection(row, false));
  sourceSyncing = false;
};

const pipelineDraft = () => {
  const spec = currentSpec.value;
  const name = createName.value.trim();
  const slots = Object.keys(spec?.gie ?? {});
  const picked = Object.fromEntries(slots.map(slot => [slot, giePicks.value[slot]]));
  const sourceKeys = { image: "images", video: "videos", rtsp: "streams" };
  const sourceKey = spec ? sourceKeys[spec.source] : "streams";
  const sourceValue = sourceSelected.value.map(row => ({ id: row.id, analyzer: row.analyzer }));
  const input = {
    generator: spec?.generator,
    pipeline_name: name,
    ...picked,
    tracker: spec?.tracker ? { class_id: classId.value } : null,
    ...sections.value,
    drawer: sections.value.drawer ?? null,
    [sourceKey]: sourceValue
  };
  const draft = spec
    ? {
        name,
        type: createType.value,
        input
      }
    : null;
  return draft;
};

const onPipelineConfirm = () => {
  const draft = createReady.value ? pipelineDraft() : null;
  creating.value = Boolean(draft);
  const request = draft && props.pipelineId ? updatePipeline(props.pipelineId, draft) : draft && createPipeline(draft);
  request &&
    request
      .then(res => {
        message(res.message, { type: "success" });
        open.value = false;
        emit("created");
        emit("close");
      })
      .finally(() => {
        creating.value = false;
      });
};

watch(
  () => giePicks.value.pgie,
  () => {
    const ids = classOptions.value.map(item => item.id);
    classId.value !== null && !ids.includes(classId.value) && (classId.value = null);
  }
);

watch([open, createType], () => {
  nextTick(() => {
    const el = sourceRef.value?.$el as HTMLElement | undefined;
    el && sourceObserver.observe(el);
  });
});

watch(open, value => {
  value || emit("close");
});

onUnmounted(() => {
  sourceObserver.disconnect();
});

onPipelineCreate();
</script>

<template>
  <el-dialog
      v-model="open"
      class="create-dialog"
      align-center
      :title="transformI18n($t(props.pipelineId ? 'pipeline.pureEdit' : 'pipeline.pureCreate'))"
      :width="PIPELINE_CREATE_DIALOG_WIDTH"
    >
      <el-form class="create-form" label-width="88px" label-position="left">
        <el-form-item :label="transformI18n($t('pipeline.pureType'))">
          <el-select v-model="createType" class="create-control" @change="applyType">
            <el-option v-for="name in Object.keys(pipelineTypes)" :key="name" :label="name" :value="name" />
          </el-select>
        </el-form-item>
        <el-form-item :label="transformI18n($t('pipeline.pureName'))">
          <el-input v-model="createName" />
          <p v-if="nameTaken" class="attr-hint">{{ transformI18n($t("pipeline.pureNameTaken")) }}</p>
        </el-form-item>
        <template v-if="currentSpec">
          <el-form-item v-for="slot in gieSlots" :key="slot" :label="fieldLabel(slot)">
            <el-select v-model="giePicks[slot]" class="create-control">
              <el-option v-for="gie in gieOptions(slot)" :key="gie.id" :label="gie.name" :value="gie.id" />
            </el-select>
          </el-form-item>
          <el-form-item v-if="currentSpec.tracker" :label="fieldLabel('class_id')">
            <el-select v-model="classId" class="create-control">
              <el-option v-for="item in classOptions" :key="item.id" :label="item.label" :value="item.id" />
            </el-select>
          </el-form-item>
          <template v-for="key in sectionKeys" :key="key">
            <div class="create-title">{{ fieldLabel(key) }}</div>
            <el-table v-if="isFlat(sections[key])" :data="Object.keys(sections[key])" border size="small" :show-header="false">
              <el-table-column width="160">
                <template #default="{ row }">{{ fieldLabel(row) }}</template>
              </el-table-column>
              <el-table-column>
                <template #default="{ row }">
                  <el-segmented
                    v-if="typeof sections[key][row] === 'boolean'"
                    v-model="sections[key][row]"
                    class="create-field"
                    :options="boolOptions"
                  />
                  <el-input-number
                    v-else-if="typeof sections[key][row] === 'number'"
                    v-model="sections[key][row]"
                    class="create-field"
                    :step="numberStep(sections[key][row])"
                  />
                  <el-input v-else v-model="sections[key][row]" class="create-field" />
                </template>
              </el-table-column>
            </el-table>
            <template v-else>
              <template v-for="group in Object.keys(sections[key])" :key="group">
                <div class="create-title">{{ fieldLabel(group) }}</div>
                <el-table :data="Object.keys(sections[key][group])" border size="small" :show-header="false">
                  <el-table-column width="160">
                    <template #default="{ row }">{{ fieldLabel(row) }}</template>
                  </el-table-column>
                  <el-table-column>
                    <template #default="{ row }">
                      <el-segmented
                        v-if="typeof sections[key][group][row] === 'boolean'"
                        v-model="sections[key][group][row]"
                        class="create-field"
                        :options="boolOptions"
                      />
                      <el-input-number
                        v-else-if="typeof sections[key][group][row] === 'number'"
                        v-model="sections[key][group][row]"
                        class="create-field"
                        :step="numberStep(sections[key][group][row])"
                      />
                      <el-input v-else v-model="sections[key][group][row]" class="create-field" />
                    </template>
                  </el-table-column>
                </el-table>
              </template>
            </template>
          </template>
          <div class="create-title">{{ transformI18n($t("pipeline.pureSource")) }}</div>
          <p class="attr-hint">
            {{ transformI18n($t(currentSpec.source === "rtsp" ? "pipeline.pureStreamMatch" : "pipeline.pureSizeMatch")) }}
          </p>
          <el-table
            ref="sourceRef"
            :data="sourceRows"
            row-key="id"
            table-layout="fixed"
            border
            @selection-change="onSourceSelect"
          >
            <el-table-column type="selection" :selectable="sourceSelectable" :width="sourceWidths.selection" reserve-selection />
            <el-table-column prop="name" :label="transformI18n($t('pipeline.pureName'))" :width="sourceWidths.name" />
            <el-table-column prop="width" :label="transformI18n($t('media.pureWidth'))" :width="sourceWidths.width" />
            <el-table-column prop="height" :label="transformI18n($t('media.pureHeight'))" :width="sourceWidths.height" />
            <el-table-column
              v-if="currentSpec.source !== 'image'"
              prop="fps"
              :label="transformI18n($t('media.pureFps'))"
              :width="sourceWidths.fps"
            />
          </el-table>
          <el-pagination
            class="analyzer-pager"
            layout="prev, pager, next"
            :current-page="sourcePage"
            :page-size="PIPELINE_SOURCE_PAGE_SIZE"
            :total="sourceTotal"
            @current-change="onSourcePage"
          />
        </template>
      </el-form>
      <template #footer>
        <el-button @click="open = false">{{ transformI18n($t("pipeline.pureCancel")) }}</el-button>
        <el-button type="primary" :disabled="!createReady || creating" @click="onPipelineConfirm">
          {{ transformI18n($t("pipeline.pureConfirm")) }}
        </el-button>
      </template>
    </el-dialog>

</template>

<style scoped>
.create-form :deep(.el-form-item) {
  margin-bottom: 12px;
}

.create-control {
  width: 100%;
}

.create-field {
  width: 160px;
}

.create-field.el-segmented {
  --el-border-radius-base: var(--el-border-radius-small);
}

.create-form {
  max-height: 70vh;
  overflow: auto;
}

.create-title {
  margin: 8px 0 4px;
  font-size: 13px;
  line-height: 20px;
  color: var(--el-text-color-secondary);
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

.analyzer-pager {
  justify-content: flex-end;
  margin-top: 8px;
}
</style>
