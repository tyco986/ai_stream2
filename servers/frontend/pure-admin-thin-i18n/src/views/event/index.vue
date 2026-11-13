<script setup lang="ts">
import { onMounted, onUnmounted, ref } from "vue";
import { getEvents, type EventAnnotations, type EventItem } from "@/api/event";
import { EVENT_LIST_COLUMN_PERCENTS, EVENT_LIST_PAGE_SIZE } from "@/constants/event";
import { PATH_EVENT_WS } from "@/constants/path";
import { $t, transformI18n } from "@/plugins/i18n";
import { formatLocalTime } from "@/utils/time";
import { useColumnWidths } from "@/views/pipeline/columnWidth";

defineOptions({
  name: "EventIndex"
});

const { target: listRef, widths: listWidths, observe } = useColumnWidths(EVENT_LIST_COLUMN_PERCENTS);
const items = ref<EventItem[]>([]);
let socket: WebSocket | null = null;

const shown = (value: string | number | undefined | null) => {
  const text = value === undefined || value === null || value === "" ? "" : String(value);
  return text;
};

const annotationText = (annotations: EventAnnotations) => {
  const text = [shown(annotations?.num), shown(annotations?.object)].join(" ").trim();
  return text;
};

const onMessage = (message: MessageEvent) => {
  const row = JSON.parse(String(message.data)) as EventItem;
  items.value = items.value.some(item => item.id === row.id) ? items.value : [row, ...items.value];
};

onMounted(() => {
  observe();
  getEvents({ page: 1, page_size: EVENT_LIST_PAGE_SIZE }).then(res => {
    items.value = res.data.items;
    const protocol = location.protocol === "https:" ? "wss" : "ws";
    socket = new WebSocket(`${protocol}://${location.host}${PATH_EVENT_WS}`);
    socket.onmessage = onMessage;
  });
});

onUnmounted(() => {
  socket?.close();
});
</script>

<template>
  <div class="event-page">
    <el-table ref="listRef" class="event-list" :data="items" table-layout="fixed" border show-overflow-tooltip>
      <el-table-column :label="transformI18n($t('event.pureOccurredAt'))" :width="listWidths.occurredAt">
        <template #default="{ row }">{{ formatLocalTime(row.occurred_at) }}</template>
      </el-table-column>
      <el-table-column prop="event_code" :label="transformI18n($t('event.pureEventCode'))" :width="listWidths.eventCode" />
      <el-table-column prop="event_label" :label="transformI18n($t('event.pureEventLabel'))" :width="listWidths.eventLabel" />
      <el-table-column :label="transformI18n($t('event.pureAnnotations'))" :width="listWidths.annotations">
        <template #default="{ row }">{{ annotationText(row.annotations) }}</template>
      </el-table-column>
    </el-table>
  </div>
</template>

<style scoped>
.event-page {
  display: flex;
  flex: 1;
  flex-direction: column;
  min-width: 0;
  min-height: 0;
  margin: 0;
  overflow: hidden;
  background: var(--el-bg-color);
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
</style>
