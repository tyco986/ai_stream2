<script setup lang="ts">
import { onMounted, ref } from "vue";
import FileList from "~icons/ri/file-list-3-line";
import PlayCircle from "~icons/ri/play-circle-line";
import PauseCircle from "~icons/ri/pause-circle-fill";
import DeleteBin from "~icons/ri/delete-bin-line";
import Refresh from "~icons/ri/refresh-line";
import { LOG_PAGES } from "@/constants/logger";
import { $t, transformI18n } from "@/plugins/i18n";
import {
  disableLogs,
  enableLogs,
  getLogs,
  getLogStatus,
  removeLogs
} from "@/api/logger";

const enabled = ref(false);
const visible = ref(false);
const page = ref("");
const date = ref("");
const content = ref<string[]>([]);

const today = () => {
  const now = new Date();
  const pad = (value: number) => String(value).padStart(2, "0");
  return `${now.getFullYear()}-${pad(now.getMonth() + 1)}-${pad(now.getDate())}`;
};

const loadStatus = () => {
  return getLogStatus().then(result => {
    enabled.value = result.data.enabled;
    return result;
  });
};

const open = () => {
  date.value = date.value || today();
  visible.value = true;
};

const selectPage = (name: string) => {
  page.value = name;
  return getLogs(name, date.value).then(result => {
    const value = result.data[name]?.[date.value]?.content;
    const lines = Array.isArray(value) ? value.map(item => JSON.stringify(item)) : [];
    content.value = lines;
    return result;
  });
};

const onDate = () => {
  const next = page.value ? selectPage(page.value) : Promise.resolve();
  return next;
};

const onToggle = () => {
  const request = enabled.value ? disableLogs() : enableLogs();
  return request.then(result => {
    enabled.value = result.data.enabled;
    return result;
  });
};

const onRefresh = () => {
  const next = page.value ? selectPage(page.value) : Promise.resolve();
  return next;
};

const onRemove = () => {
  const name = page.value;
  const request = name
    ? removeLogs(name).then(result => {
        content.value = [];
        return result;
      })
    : Promise.resolve();
  return request;
};

onMounted(() => {
  date.value = today();
  loadStatus();
});
</script>

<template>
  <div>
    <div
      class="w-[40px] h-[48px] flex-c cursor-pointer navbar-bg-hover"
      :class="{ 'log-on': enabled }"
      @click="open"
    >
      <IconifyIconOffline :icon="FileList" />
    </div>
    <el-dialog
      v-model="visible"
      :title="transformI18n($t('logger.pureTitle'))"
      width="880px"
      align-center
      append-to-body
    >
      <div class="log-body">
        <ul class="log-pages">
          <li
            v-for="item in LOG_PAGES"
            :key="item.page"
            :class="{ active: page === item.page }"
            @click="selectPage(item.page)"
          >
            {{ item.label }}
          </li>
        </ul>
        <div class="log-main">
          <div class="log-toolbar">
            <el-date-picker
              v-model="date"
              type="date"
              value-format="YYYY-MM-DD"
              :clearable="false"
              @change="onDate"
            />
            <span class="log-tool" :class="{ recording: enabled }" @click="onToggle">
              <IconifyIconOffline :icon="enabled ? PauseCircle : PlayCircle" />
            </span>
            <span class="log-tool" :class="{ disabled: !page }" @click="onRefresh">
              <IconifyIconOffline :icon="Refresh" />
            </span>
            <span
              class="log-tool"
              :class="{ disabled: !page }"
              @click="onRemove"
            >
              <IconifyIconOffline :icon="DeleteBin" />
            </span>
          </div>
          <div class="log-content">
            <div v-for="(line, index) in content" :key="index" class="log-line">
              {{ line }}
            </div>
          </div>
        </div>
      </div>
      <template #footer>
        <el-button @click="visible = false">Close</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<style scoped>
.log-on {
  color: #16a34a;
}

.log-body {
  display: flex;
  height: 460px;
}

.log-pages {
  width: 180px;
  margin: 0;
  padding: 0;
  overflow: auto;
  list-style: none;
  border-right: 1px solid var(--el-border-color);
}

.log-pages li {
  padding: 8px 12px;
  cursor: pointer;
}

.log-pages li.active {
  color: var(--el-color-primary);
  background: var(--el-fill-color-light);
}

.log-main {
  display: flex;
  flex: 1;
  flex-direction: column;
  min-width: 0;
}

.log-toolbar {
  display: flex;
  gap: 8px;
  align-items: center;
  padding: 0 12px 8px;
}

.log-tool {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 32px;
  height: 32px;
  font-size: 18px;
  cursor: pointer;
}

.log-tool.recording {
  color: #f56c6c;
}

.log-tool.disabled {
  cursor: default;
  opacity: 0.35;
}

.log-content {
  flex: 1;
  overflow: auto;
  font-size: 12px;
  line-height: 1.5;
}

.log-line {
  padding: 8px 12px;
  word-break: break-all;
  border-bottom: 1px solid var(--el-border-color);
}
</style>
