<script setup lang="ts">
import { onMounted, onUnmounted, ref } from "vue";
import DeleteBin from "~icons/ri/delete-bin-line";
import Eye from "~icons/ri/eye-line";
import ListCheck from "~icons/ri/list-check";
import Mail from "~icons/ri/mail-line";
import MailOpen from "~icons/ri/mail-open-line";
import {
  getEvents,
  readEvent,
  readEvents,
  removeEvent,
  removeEvents,
  unreadEvent,
  unreadEvents,
  type EventItem
} from "@/api/event";
import {
  EVENT_ANNOTATION_DIALOG_WIDTH,
  EVENT_LEVEL_ALERT,
  EVENT_LEVEL_ALL,
  EVENT_LEVEL_LABELS,
  EVENT_LEVELS,
  EVENT_LIST_COLUMN_PERCENTS,
  EVENT_LIST_PAGE_SIZE
} from "@/constants/event";
import { PATH_EVENT_WS } from "@/constants/path";
import { $t, transformI18n } from "@/plugins/i18n";
import { message } from "@/utils/message";
import { formatLocalTime } from "@/utils/time";
import { useColumnWidths } from "@/views/pipeline/columnWidth";
import EventPreview from "./EventPreview.vue";

defineOptions({
  name: "EventIndex"
});

const { target: listRef, widths: listWidths, observe } = useColumnWidths(EVENT_LIST_COLUMN_PERCENTS);
const items = ref<EventItem[]>([]);
const selected = ref<EventItem[]>([]);
const level = ref<(typeof EVENT_LEVELS)[number]>(EVENT_LEVEL_ALERT);
const page = ref(1);
const total = ref(0);
const annotationsOpen = ref(false);
const preview = ref<InstanceType<typeof EventPreview>>();
const annotationText = ref("");
let socket: WebSocket | null = null;

const load = () => {
  getEvents({ page: page.value, page_size: EVENT_LIST_PAGE_SIZE, level: level.value }).then(res => {
    items.value = res.data.items;
    total.value = res.data.total;
    page.value = res.data.page;
  });
};

const onLevel = () => {
  page.value = 1;
  load();
};

const onPage = (next: number) => {
  page.value = next;
  load();
};

const onAnnotations = (row: EventItem) => {
  annotationText.value = JSON.stringify(row.annotations ?? {}, null, 2);
  annotationsOpen.value = true;
};

const sourceName = (row: EventItem) => {
  const name = row.stream_name || row.media_name || "";
  return name;
};

const replaceRow = (row: EventItem) => {
  items.value = items.value.map(item => (item.id === row.id ? row : item));
};

const selectedIds = () => selected.value.map(row => row.id);

const onRead = (row: EventItem) => {
  const request = row.read_at ? unreadEvent(row.id) : readEvent(row.id);
  request.then(res => {
    replaceRow(res.data);
    message(res.message, { type: "success" });
  });
};

const onRemove = (row: EventItem) => {
  removeEvent(row.id).then(res => {
    message(res.message, { type: "success" });
    page.value = items.value.length > 1 || page.value === 1 ? page.value : page.value - 1;
    load();
  });
};

const onReadBatch = () => {
  readEvents(selectedIds()).then(res => {
    const updated = new Map(res.data.updated_rows.map(row => [row.id, row]));
    items.value = items.value.map(item => updated.get(item.id) ?? item);
    message(res.message, { type: "success" });
  });
};

const onUnreadBatch = () => {
  unreadEvents(selectedIds()).then(res => {
    const updated = new Map(res.data.updated_rows.map(row => [row.id, row]));
    items.value = items.value.map(item => updated.get(item.id) ?? item);
    message(res.message, { type: "success" });
  });
};

const onRemoveBatch = () => {
  removeEvents(selectedIds()).then(res => {
    message(res.message, { type: "success" });
    const left = items.value.length - res.data.updated_count;
    page.value = left > 0 || page.value === 1 ? page.value : page.value - 1;
    load();
  });
};

const onMessage = (messageEvent: MessageEvent) => {
  const row = JSON.parse(String(messageEvent.data)) as EventItem;
  const shown = level.value === EVENT_LEVEL_ALL || row.level === level.value;
  const exists = shown && items.value.some(item => item.id === row.id);
  total.value = shown && !exists ? total.value + 1 : total.value;
  items.value =
    shown && page.value === 1 && !exists ? [row, ...items.value].slice(0, EVENT_LIST_PAGE_SIZE) : items.value;
};

onMounted(() => {
  observe();
  load();
  const protocol = location.protocol === "https:" ? "wss" : "ws";
  socket = new WebSocket(`${protocol}://${location.host}${PATH_EVENT_WS}`);
  socket.onmessage = onMessage;
});

onUnmounted(() => {
  socket?.close();
});
</script>

<template>
  <div class="event-page">
    <div class="event-toolbar">
      <el-button type="primary" :disabled="!selected.length" @click="onRemoveBatch">
        {{ transformI18n($t("event.pureRemove")) }}
      </el-button>
      <el-button type="primary" :disabled="!selected.length" @click="onReadBatch">
        {{ transformI18n($t("event.pureRead")) }}
      </el-button>
      <el-button type="primary" :disabled="!selected.length" @click="onUnreadBatch">
        {{ transformI18n($t("event.pureUnread")) }}
      </el-button>
      <span class="event-level-label">{{ transformI18n($t("event.pureLevel")) }}</span>
      <el-select v-model="level" class="event-level" @change="onLevel">
        <el-option v-for="item in EVENT_LEVELS" :key="item" :label="transformI18n($t(EVENT_LEVEL_LABELS[item]))" :value="item" />
      </el-select>
    </div>
    <el-table
      ref="listRef"
      class="event-list"
      :data="items"
      table-layout="fixed"
      border
      show-overflow-tooltip
      @selection-change="selected = $event"
    >
      <el-table-column type="selection" :width="listWidths.selection" />
      <el-table-column :label="transformI18n($t('event.pureOccurredAt'))" :width="listWidths.occurredAt">
        <template #default="{ row }">{{ formatLocalTime(row.occurred_at) }}</template>
      </el-table-column>
      <el-table-column prop="event_code" :label="transformI18n($t('event.pureEventCode'))" :width="listWidths.eventCode" />
      <el-table-column prop="event_label" :label="transformI18n($t('event.pureEventLabel'))" :width="listWidths.eventLabel" />
      <el-table-column prop="pipeline_name" :label="transformI18n($t('event.purePipeline'))" :width="listWidths.pipeline" />
      <el-table-column :label="transformI18n($t('event.pureSource'))" :width="listWidths.source">
        <template #default="{ row }">{{ sourceName(row) }}</template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('event.pureAnnotations'))" :width="listWidths.annotations" :show-overflow-tooltip="false">
        <template #default="{ row }">
          <el-button link type="primary" :title="transformI18n($t('event.pureAnnotations'))" @click="onAnnotations(row)">
            <IconifyIconOffline :icon="ListCheck" />
          </el-button>
        </template>
      </el-table-column>
      <el-table-column :label="transformI18n($t('event.pureOps'))" :width="listWidths.ops" :show-overflow-tooltip="false">
        <template #default="{ row, $index }">
          <el-button link type="primary" :title="transformI18n($t('event.purePreview'))" @click="preview?.open(items, $index)">
            <IconifyIconOffline :icon="Eye" />
          </el-button>
          <el-button
            link
            type="primary"
            :title="transformI18n($t(row.read_at ? 'event.pureUnread' : 'event.pureRead'))"
            @click="onRead(row)"
          >
            <IconifyIconOffline :icon="row.read_at ? MailOpen : Mail" />
          </el-button>
          <el-button link type="primary" :title="transformI18n($t('event.pureRemove'))" @click="onRemove(row)">
            <IconifyIconOffline :icon="DeleteBin" />
          </el-button>
        </template>
      </el-table-column>
    </el-table>
    <el-pagination
      class="event-pager"
      layout="prev, pager, next"
      :current-page="page"
      :page-size="EVENT_LIST_PAGE_SIZE"
      :total="total"
      @current-change="onPage"
    />
    <EventPreview ref="preview" />
    <el-dialog v-model="annotationsOpen" :title="transformI18n($t('event.pureAnnotations'))" :width="EVENT_ANNOTATION_DIALOG_WIDTH">
      <pre class="event-annotations">{{ annotationText }}</pre>
    </el-dialog>
  </div>
</template>

<style scoped>
.event-page.main-content {
  display: flex;
  flex: 1;
  flex-direction: column;
  min-width: 0;
  min-height: 0;
  margin: 0;
  overflow: hidden;
  background: var(--el-bg-color);
}

.event-toolbar {
  display: flex;
  flex: none;
  gap: 8px;
  align-items: center;
  padding: 8px;
  background: var(--el-bg-color);
}

.event-level-label {
  margin-left: auto;
}

.event-level {
  width: 120px;
}

.event-list {
  flex: 1;
  width: 100%;
  min-height: 0;
}

.event-list :deep(.cell) {
  word-break: normal;
  white-space: nowrap;
}

.event-pager {
  flex: none;
  justify-content: flex-end;
  padding: 8px;
}

.event-annotations {
  max-height: 420px;
  margin: 0;
  overflow: auto;
  white-space: pre-wrap;
}
</style>
