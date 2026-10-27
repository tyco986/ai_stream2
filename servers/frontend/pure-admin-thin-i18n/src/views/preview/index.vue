<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, provide, reactive, ref, watch } from "vue";
import Brush from "~icons/ri/brush-line";
import Save from "~icons/ri/save-line";
import SaveAs from "~icons/ri/save-3-line";
import { getActivePreset } from "@/api/preview";
import type { StreamItem } from "@/api/stream";
import type { TreeNode } from "@/api/streamGroup";
import StreamGroupTree from "@/components/StreamGroupTree/index.vue";
import { LAYOUTS, SLOT_COUNT, whepUrl } from "@/constants/preview";
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

const preset = ref("");
const layout = ref("1x1");
const picked = ref<number | null>(null);
const slots = reactive<Record<number, SlotState>>(
  Object.fromEntries(Array.from({ length: SLOT_COUNT }, (_, index) => [index, emptySlot()]))
);

const liveIds = computed(() => Object.values(slots).flatMap(slot => (slot.id ? [slot.id] : [])));

watch(liveIds, ids => players.retain(ids));

const count = computed(() => Number(layout.value[0]));
const visible = computed(() => count.value * count.value);

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

onMounted(() => {
  getActivePreset().then(res => {
    preset.value = res.data.name;
    layout.value = res.data.layout;
    fillPreset(res.data.slots);
  });
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
          v-model="preset"
          class="preview-preset"
          :placeholder="transformI18n($t('preview.purePreset'))"
        />
        <el-button link :title="transformI18n($t('preview.pureSave'))">
          <IconifyIconOffline :icon="Save" />
        </el-button>
        <el-button link :title="transformI18n($t('preview.pureSaveAs'))">
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
              :class="{ picked: picked === index - 1 }"
              @click="onPick(index - 1)"
            >
              <PreviewCell
                v-if="slots[index - 1].status === 'playing'"
                :key="slots[index - 1].id"
                :stream-id="slots[index - 1].id"
                :url="slots[index - 1].url"
                @clear="clearSlot(index - 1)"
              />
            </div>
          </div>
        </div>
      </div>
    </div>
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
