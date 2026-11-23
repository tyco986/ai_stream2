<script setup lang="ts">
import { computed, onMounted, ref } from "vue";
import BellIcon from "~icons/ep/bell";
import Mail from "~icons/ri/mail-line";
import MailOpen from "~icons/ri/mail-open-line";
import { readEvent, unreadEvent, type EventItem } from "@/api/event";
import { EVENT_LEVEL_ALERT } from "@/constants/event";
import { $t, transformI18n } from "@/plugins/i18n";
import { message } from "@/utils/message";
import {
  BELL_EVENT,
  BELL_SYSTEM,
  bellEvents,
  bellOpen,
  bellTab,
  noticeCount,
  reloadEvents,
  startEventRefresh
} from "@/views/event/refresh";

const historyOpen = ref(false);

const unread = computed(() => bellEvents.value.filter(row => row.read_at == null && row.level === EVENT_LEVEL_ALERT));
const history = computed(() => bellEvents.value.filter(row => row.read_at != null));

const textOf = (row: EventItem) => {
  const sourceName = row.stream_id ? row.stream_name ?? "" : row.media_name ?? "";
  const pipelineName = row.pipeline_name ?? "";
  const place = pipelineName && sourceName ? `, ${pipelineName}-${sourceName}` : "";
  return `${row.occurred_at}: ${row.event_label} detected${place}`;
};

const onVisible = (visible: boolean) => {
  bellOpen.value = visible;
  if (visible) {
    historyOpen.value = false;
    bellTab.value = BELL_EVENT;
    reloadEvents();
  }
};

const onTab = (name: string | number) => {
  bellTab.value = String(name);
  if (bellTab.value === BELL_EVENT) {
    reloadEvents();
  }
};

const onToggle = (row: EventItem) => {
  const request = row.read_at ? unreadEvent(row.id) : readEvent(row.id);
  request.then(res => {
    message(res.message, { type: "success" });
  });
};

onMounted(() => {
  startEventRefresh();
});
</script>

<template>
  <el-dropdown trigger="click" placement="bottom-end" :hide-on-click="false" @visible-change="onVisible">
    <span :class="['dropdown-badge', 'navbar-bg-hover', 'select-none', noticeCount.total !== 0 && 'mr-[10px]']">
      <el-badge :value="noticeCount.total === 0 ? '' : noticeCount.total" :max="99">
        <span class="header-notice-icon">
          <IconifyIconOffline :icon="BellIcon" />
        </span>
      </el-badge>
    </span>
    <template #dropdown>
      <el-dropdown-menu>
        <el-tabs v-model="bellTab" :stretch="true" class="dropdown-tabs" @tab-change="onTab">
          <el-tab-pane :label="`${transformI18n($t('event.pureBellEvent'))} ${noticeCount.event}`" :name="BELL_EVENT">
            <el-scrollbar max-height="330px">
              <div class="notice-list">
                <el-empty v-if="unread.length === 0" :description="transformI18n($t('status.pureNoMessage'))" :image-size="60" />
                <div v-for="row in unread" :key="row.id" class="notice-row">
                  <span>{{ textOf(row) }}</span>
                  <button type="button" class="notice-mail" @click.stop="onToggle(row)">
                    <IconifyIconOffline :icon="Mail" />
                  </button>
                </div>
                <button type="button" class="notice-history" @click.stop="historyOpen = !historyOpen">
                  {{ historyOpen ? "▼" : "▶" }} {{ transformI18n($t("event.pureHistory")) }}
                </button>
                <template v-if="historyOpen">
                  <div v-for="row in history" :key="row.id" class="notice-row">
                    <span>{{ textOf(row) }}</span>
                    <button type="button" class="notice-mail" @click.stop="onToggle(row)">
                      <IconifyIconOffline :icon="MailOpen" />
                    </button>
                  </div>
                </template>
              </div>
            </el-scrollbar>
          </el-tab-pane>
          <el-tab-pane :label="`${transformI18n($t('event.pureBellSystem'))} ${noticeCount.system}`" :name="BELL_SYSTEM">
            <el-empty :description="transformI18n($t('status.pureNoMessage'))" :image-size="60" />
          </el-tab-pane>
        </el-tabs>
      </el-dropdown-menu>
    </template>
  </el-dropdown>
</template>

<style lang="scss" scoped>
.dropdown-badge {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 40px;
  height: 48px;
  cursor: pointer;

  .header-notice-icon {
    font-size: 18px;
  }
}

.dropdown-tabs {
  width: 360px;

  :deep(.el-tabs__header) {
    margin: 0;
  }

  :deep(.el-tabs__nav-wrap)::after {
    height: 1px;
  }
}

.notice-list {
  padding: 8px 16px 12px;
}

.notice-row {
  display: flex;
  gap: 12px;
  align-items: flex-start;
  justify-content: space-between;
  padding: 8px 0;
  font-size: 13px;
  line-height: 1.4;
}

.notice-mail,
.notice-history {
  padding: 0;
  color: inherit;
  cursor: pointer;
  background: none;
  border: 0;
}

.notice-mail {
  flex: none;
  font-size: 16px;
}

.notice-history {
  margin-top: 8px;
  font-size: 13px;
}
</style>
