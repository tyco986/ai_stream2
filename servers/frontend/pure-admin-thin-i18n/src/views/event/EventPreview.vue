<script setup lang="ts">
import { computed, ref, watch } from "vue";
import ArrowLeft from "~icons/ri/arrow-left-s-line";
import ArrowRight from "~icons/ri/arrow-right-s-line";
import { EVENT_KIND_VISUALIZATION, getEventContent, getEventContentInfo, type EventFileInfo, type EventItem } from "@/api/event";
import { $t, transformI18n } from "@/plugins/i18n";
import { formatLocalTime } from "@/utils/time";

defineOptions({
  name: "EventPreview"
});

const items = ref<EventItem[]>([]);
const previewOpen = ref(false);
const previewIndex = ref(0);
const previewUrl = ref("");
const previewInfo = ref<EventFileInfo | null>(null);
const previewToken = ref(0);

const previewRows = computed(() => (previewInfo.value ? [previewInfo.value] : []));

const releasePreview = () => {
  const previous = previewUrl.value;
  previewUrl.value = "";
  if (previous) {
    URL.revokeObjectURL(previous);
  }
};

const loadPreview = () => {
  const token = previewToken.value + 1;
  previewToken.value = token;
  const item = items.value[previewIndex.value];
  releasePreview();
  previewInfo.value = null;
  if (previewOpen.value && item) {
    Promise.all([getEventContentInfo(item.id, EVENT_KIND_VISUALIZATION), getEventContent(item.id, EVENT_KIND_VISUALIZATION)]).then(
      ([info, blob]) => {
        if (token === previewToken.value) {
          previewInfo.value = info.data;
          previewUrl.value = blob ? URL.createObjectURL(blob) : "";
        }
      }
    );
  }
};

const open = (list: EventItem[], index: number) => {
  items.value = list;
  previewIndex.value = index;
  previewOpen.value = true;
};

const shiftPreview = (step: number) => {
  const next = previewIndex.value + step;
  const valid = next >= 0 && next < items.value.length;
  previewIndex.value = valid ? next : previewIndex.value;
};

watch([previewOpen, previewIndex], () => {
  if (previewOpen.value) {
    loadPreview();
  } else {
    previewToken.value += 1;
    releasePreview();
    previewInfo.value = null;
  }
});

defineExpose({ open });
</script>

<template>
  <el-dialog
    v-model="previewOpen"
    class="event-preview"
    align-center
    :title="transformI18n($t('event.purePreview'))"
    append-to-body
    destroy-on-close
  >
    <el-table class="event-meta" :data="previewRows" table-layout="fixed" border>
      <el-table-column prop="width" :label="transformI18n($t('event.pureWidth'))" />
      <el-table-column prop="height" :label="transformI18n($t('event.pureHeight'))" />
      <el-table-column prop="size" :label="transformI18n($t('event.pureSize'))" />
      <el-table-column :label="transformI18n($t('event.pureCreatedAt'))">
        <template #default="{ row }">{{ formatLocalTime(row.created_at) }}</template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('event.pureUpdatedAt'))">
        <template #default="{ row }">{{ formatLocalTime(row.updated_at) }}</template>
      </el-table-column>
    </el-table>
    <div class="event-stage">
      <el-button class="event-nav event-nav-prev" circle :disabled="previewIndex <= 0" @click="shiftPreview(-1)">
        <IconifyIconOffline :icon="ArrowLeft" />
      </el-button>
      <img v-if="previewUrl" :src="previewUrl" alt="" />
      <el-button
        class="event-nav event-nav-next"
        circle
        :disabled="previewIndex >= items.length - 1"
        @click="shiftPreview(1)"
      >
        <IconifyIconOffline :icon="ArrowRight" />
      </el-button>
    </div>
  </el-dialog>
</template>

<style>
.el-overlay-dialog:has(> .event-preview) {
  align-items: center;
  justify-content: center;
}

.event-preview.el-dialog {
  --el-dialog-width: min(960px, calc(100vw - 32px), calc((100vh - 280px) * 16 / 9));
  width: var(--el-dialog-width);
  max-width: var(--el-dialog-width);
  margin: 0;
}

.event-preview .el-dialog__body {
  min-width: 0;
}

.event-preview .event-meta {
  width: 100%;
}

.event-preview .event-stage {
  position: relative;
  width: 100%;
  min-width: 0;
  aspect-ratio: 16 / 9;
  margin-top: 12px;
  overflow: hidden;
  background: #000;
}

.event-preview .event-stage img {
  position: absolute;
  inset: 0;
  width: 100%;
  height: 100%;
  object-fit: contain;
}

.event-preview .event-nav {
  position: absolute;
  top: 50%;
  z-index: 1;
  transform: translateY(-50%);
}

.event-preview .event-nav-prev {
  left: 12px;
}

.event-preview .event-nav-next {
  right: 12px;
}
</style>
