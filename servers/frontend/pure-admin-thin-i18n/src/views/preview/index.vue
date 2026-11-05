<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, provide, reactive, ref, watch } from "vue";
import Brush from "~icons/ri/brush-line";
import Save from "~icons/ri/save-line";
import SaveAs from "~icons/ri/save-3-line";
import { createPreset, getActivePreset, getPreset, getPresetMap, updatePreset } from "@/api/preview";
import type { Preset, PresetDraft } from "@/api/preview";
import type { StreamItem } from "@/api/stream";
import type { TreeNode } from "@/api/streamGroup";
import StreamGroupTree from "@/components/StreamGroupTree/index.vue";
import { LAYOUTS, MODE_GRID, PREVIEW_SAVE_DIALOG_WIDTH, SLOT_COUNT, whepUrl } from "@/constants/preview";
import { $t, transformI18n } from "@/plugins/i18n";
import PreviewCell from "./cell.vue";
import { StreamPlayers, streamPlayersKey } from "./players";

defineOptions({
  name: "PreviewIndex"
});

type SlotState = {
  url: string;
  name: string;
  id: string;
  status: "pausing" | "playing";
};

const emptySlot = (): SlotState => ({
  url: "",
  name: "",
  id: "",
  status: "pausing"
});

const players = new StreamPlayers();
provide(streamPlayersKey, players);

const presetId = ref("");
const preset = ref("");
const saveAsVisible = ref(false);
const saveAsName = ref("");
const presetMap = ref<Record<string, string>>({});
const mode = ref(MODE_GRID);
const saved = ref("");
const layout = ref("1x1");
const picked = ref<number | null>(null);
const slots = reactive<Record<number, SlotState>>(
  Object.fromEntries(Array.from({ length: SLOT_COUNT }, (_, index) => [index, emptySlot()]))
);

const liveIds = computed(() => Object.values(slots).flatMap(slot => (slot.id ? [slot.id] : [])));

watch(liveIds, ids => players.retain(ids));

const count = computed(() => Number(layout.value[0]));
const visible = computed(() => count.value * count.value);
const fingerprint = computed(() =>
  [layout.value, ...Array.from({ length: SLOT_COUNT }, (_, index) => slots[index].id)].join("\n")
);
const optionLabel = (name: string) =>
  name + (name === preset.value && saved.value && saved.value !== fingerprint.value ? " *" : "");

const clearSlot = (index: number) => {
  slots[index] = emptySlot();
};

const assign = (index: number, stream: { id: string; name: string }) => {
  slots[index] = {
    url: whepUrl(stream.name),
    name: stream.name,
    id: stream.id,
    status: "playing"
  };
};

const slotIndex = (id: string) => {
  const found = Object.keys(slots).find(key => slots[Number(key)].id === id);
  return found === undefined ? -1 : Number(found);
};

const place = (stream: { id: string; name: string }) => {
  const bound = slotIndex(stream.id);
  const empty = Array.from({ length: visible.value }, (_, index) => index).find(
    index => slots[index].id === ""
  );
  const target = picked.value ?? (bound >= 0 ? bound : empty ?? -1);
  const clearOnly = picked.value === null && bound >= 0;
  if (bound >= 0 && bound !== target) {
    clearSlot(bound);
  }
  if (clearOnly) {
    clearSlot(bound);
  }
  if (target >= 0 && !clearOnly) {
    assign(target, stream);
  }
};

const onSelect = (node: TreeNode) => {
  if (node.type === "stream") {
    place(node);
  }
};

const onPick = (index: number) => {
  picked.value = picked.value === index ? null : index;
};

const onClear = () => {
  Object.keys(slots).forEach(key => clearSlot(Number(key)));
  picked.value = null;
};

const slotCount = () => Number(layout.value[0]) ** 2;

const draft = (name: string): PresetDraft => ({
  name,
  layout: layout.value,
  mode: mode.value,
  active: true,
  slots: Object.fromEntries(
    Array.from({ length: slotCount() }, (_, index) => [String(index + 1), slots[index].id || null])
  )
});

const applySaved = (data: Preset) => {
  Array.from({ length: SLOT_COUNT - slotCount() }, (_, index) => clearSlot(slotCount() + index));
  presetId.value = data.id;
  preset.value = data.name;
  mode.value = data.mode;
  saved.value = fingerprint.value;
};

const onSave = () => {
  updatePreset(presetId.value, draft(preset.value)).then(res => applySaved(res.data));
};

const loadMap = () => {
  getPresetMap().then(res => {
    presetMap.value = res.data;
  });
};

const onSaveAs = () => {
  saveAsName.value = preset.value;
  saveAsVisible.value = true;
};

const onSaveAsConfirm = () => {
  const name = saveAsName.value.trim();
  if (name) {
    createPreset(draft(name)).then(res => {
      applySaved(res.data);
      loadMap();
      saveAsVisible.value = false;
    });
  }
};

const fillPreset = (items: Record<string, StreamItem | null>) => {
  Object.keys(slots).forEach(key => clearSlot(Number(key)));
  Object.entries(items).forEach(([key, stream]) => {
    const index = Number(key) - 1;
    if (stream && index >= 0 && index < SLOT_COUNT) {
      assign(index, stream);
    }
  });
};

onBeforeUnmount(() => players.retain([]));

const show = (data: Preset) => {
  presetId.value = data.id;
  preset.value = data.name;
  mode.value = data.mode;
  layout.value = data.layout;
  fillPreset(data.slots);
  saved.value = fingerprint.value;
};

const onPreset = (id: string) => {
  getPreset(id).then(res => show(res.data));
};

onMounted(() => {
  getActivePreset().then(res => show(res.data));
  loadMap();
});
</script>

<template>
  <div class="preview-page">
    <aside class="preview-tree">
      <StreamGroupTree @select="onSelect" />
    </aside>
    <div class="preview-main">
      <div class="preview-toolbar">
        <el-select
          :model-value="presetId"
          class="preview-preset"
          :placeholder="transformI18n($t('preview.purePreset'))"
          @change="onPreset"
        >
          <el-option v-for="(id, name) in presetMap" :key="id" :label="optionLabel(name)" :value="id" />
        </el-select>
        <el-button link :title="transformI18n($t('preview.pureSave'))" @click="onSave">
          <IconifyIconOffline :icon="Save" />
        </el-button>
        <el-button link :title="transformI18n($t('preview.pureSaveAs'))" @click="onSaveAs">
          <IconifyIconOffline :icon="SaveAs" />
        </el-button>
        <el-select v-model="layout" class="preview-layout">
          <el-option v-for="item in LAYOUTS" :key="item" :label="item" :value="item" />
        </el-select>
        <el-button link :title="transformI18n($t('preview.pureClear'))" @click="onClear">
          <IconifyIconOffline :icon="Brush" />
        </el-button>
      </div>
      <div class="preview-stage-wrap">
        <div class="preview-stage">
          <div
            class="preview-grid"
            :style="{
              gridTemplateColumns: `repeat(${count}, minmax(0, 1fr))`,
              gridTemplateRows: `repeat(${count}, minmax(0, 1fr))`
            }"
          >
            <div
              v-for="index in visible"
              :key="index - 1"
              class="preview-cell"
              :class="{ picked: picked === index - 1, empty: !slots[index - 1].id }"
              @click="onPick(index - 1)"
            >
              <span v-if="!slots[index - 1].id" class="preview-index">{{ index }}</span>
              <PreviewCell
                :key="slots[index - 1].id || `empty-${index}`"
                :stream-id="slots[index - 1].id"
                :stream-name="slots[index - 1].name"
                :url="slots[index - 1].url"
                @clear="clearSlot(index - 1)"
              />
            </div>
          </div>
        </div>
      </div>
    </div>
    <el-dialog v-model="saveAsVisible" :title="transformI18n($t('preview.pureSaveAs'))" :width="PREVIEW_SAVE_DIALOG_WIDTH">
      <el-form label-width="72px" label-position="left" @submit.prevent="onSaveAsConfirm">
        <el-form-item :label="transformI18n($t('preview.pureName'))">
          <el-input v-model="saveAsName" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="saveAsVisible = false">
          {{ transformI18n($t("stream.pureCancel")) }}
        </el-button>
        <el-button type="primary" @click="onSaveAsConfirm">
          {{ transformI18n($t("stream.pureConfirm")) }}
        </el-button>
      </template>
    </el-dialog>
  </div>
</template>

<style scoped>
.preview-page.main-content {
  display: flex;
  flex: 1;
  min-width: 0;
  min-height: 0;
  margin: 0;
}

.preview-tree {
  flex: 0 0 240px;
  align-self: stretch;
  overflow: auto;
  background: var(--el-bg-color);
  border-right: 1px solid var(--el-border-color);
}

.preview-main {
  display: flex;
  flex: 1;
  flex-direction: column;
  min-width: 0;
  min-height: 0;
  overflow: hidden;
  background: var(--el-bg-color);
}

.preview-toolbar {
  display: flex;
  flex: none;
  gap: 8px;
  align-items: center;
  padding: 8px;
  border-bottom: 1px solid var(--el-border-color);
}

.preview-toolbar :deep(.el-button) {
  margin: 0;
  font-size: 18px;
}

.preview-preset {
  width: 180px;
}

.preview-layout {
  width: 88px;
}

.preview-stage-wrap {
  display: flex;
  flex: 1;
  align-items: center;
  justify-content: center;
  min-width: 0;
  min-height: 0;
  overflow: hidden;
  background: var(--el-fill-color-darker);
  container-type: size;
}

.preview-stage {
  width: min(100cqw, calc(100cqh * 16 / 9));
  aspect-ratio: 16 / 9;
}

.preview-grid {
  display: grid;
  gap: 1px;
  width: 100%;
  height: 100%;
}

.preview-cell {
  position: relative;
  min-width: 0;
  min-height: 0;
  overflow: hidden;
  background: #111;
  container-type: size;
}

.preview-cell.empty {
  background: #3a3a3a;
}

.preview-index {
  position: absolute;
  inset: 0;
  z-index: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 28cqmin;
  line-height: 1;
  color: rgb(255 255 255 / 45%);
  pointer-events: none;
}

.preview-cell.picked::after {
  position: absolute;
  inset: 0;
  z-index: 2;
  pointer-events: none;
  content: "";
  box-shadow: inset 0 0 0 2px var(--el-color-primary);
}
</style>
