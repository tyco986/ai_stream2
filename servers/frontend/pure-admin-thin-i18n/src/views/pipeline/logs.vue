<script setup lang="ts">
import { ref, watch } from "vue";
import DeleteBin from "~icons/ri/delete-bin-line";
import { getPipelineApiLogs, getPipelineGstreamerLogs, getPipelinePadLogs, removePipelineLogs } from "@/api/pipeline";
import { PIPELINE_LOG_DIALOG_WIDTH, PIPELINE_LOG_PAD_OPTIONS } from "@/constants/pipeline";
import { $t, transformI18n } from "@/plugins/i18n";
import { message } from "@/utils/message";

const SECTIONS = ["gstreamer", "api", "pad"] as const;
type LogSection = (typeof SECTIONS)[number];

const sectionLabels: Record<LogSection, string> = {
  gstreamer: "Gstreamer",
  api: "API",
  pad: "Pad"
};

const today = () => {
  const now = new Date();
  const pad = (value: number) => String(value).padStart(2, "0");
  return `${now.getFullYear()}-${pad(now.getMonth() + 1)}-${pad(now.getDate())}`;
};

const open = ref(true);
const section = ref<LogSection>("gstreamer");
const date = ref(today());
const pad = ref(PIPELINE_LOG_PAD_OPTIONS[0]);
const entries = ref<string[]>([]);
const clearing = ref(false);
const emit = defineEmits<{ close: [] }>();

const props = defineProps<{ id: string; name: string }>();

const lines = (rows: unknown[]) => rows.map(item => JSON.stringify(item));

const load = () => {
  const id = props.id;
  const day = date.value;
  const request =
    section.value === "api"
      ? getPipelineApiLogs(id, day).then(result => lines(result.data[day]?.content ?? []))
      : section.value === "pad"
        ? getPipelinePadLogs(id, pad.value, day).then(result => lines(result.data.content ?? []))
        : getPipelineGstreamerLogs(id, day).then(result => {
            const text = result.data.content ?? "";
            return text ? [text] : [];
          });
  return request.then(rows => {
    entries.value = rows;
    return rows;
  }).catch(() => {
    entries.value = [];
    return [];
  });
};

const onClear = () => {
  clearing.value = true;
  removePipelineLogs(props.id)
    .then(result => {
      message(result.message, { type: "success" });
      return load();
    })
    .finally(() => {
      clearing.value = false;
    });
};

watch([section, date, pad], () => {
  load();
}, { immediate: true });
</script>

<template>
  <el-dialog
    v-model="open"
    class="create-dialog"
    :title="`${transformI18n($t('pipeline.pureLogs'))} · ${name}`"
    :width="PIPELINE_LOG_DIALOG_WIDTH"
    @closed="emit('close')"
  >
    <div class="log-bar">
      <el-select v-model="section" class="log-section">
        <el-option v-for="item in SECTIONS" :key="item" :label="sectionLabels[item]" :value="item" />
      </el-select>
      <el-date-picker v-model="date" type="date" value-format="YYYY-MM-DD" :clearable="false" />
      <el-select v-model="pad" class="log-pad" :disabled="section !== 'pad'">
        <el-option v-for="item in PIPELINE_LOG_PAD_OPTIONS" :key="item" :label="item" :value="item" />
      </el-select>
      <el-button link type="primary" class="log-clear" :disabled="clearing" :title="transformI18n($t('pipeline.pureClearLogs'))" @click="onClear">
        <IconifyIconOffline :icon="DeleteBin" />
      </el-button>
    </div>
    <div class="log-body">
      <pre v-for="(item, index) in entries" :key="index" class="log-entry">{{ item }}</pre>
    </div>
  </el-dialog>
</template>

<style scoped>
.log-bar {
  display: flex;
  gap: 8px;
  align-items: center;
}

.log-section {
  width: 140px;
}

.log-pad {
  width: 120px;
}

.log-clear {
  margin-left: auto;
}

.log-body {
  height: 360px;
  margin: 8px 0 0;
  overflow: auto;
}

.log-entry {
  margin: 0;
  padding: 8px 0;
  white-space: pre-wrap;
}

.log-entry + .log-entry {
  border-top: 1px solid var(--el-border-color);
}
</style>
