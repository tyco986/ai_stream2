import { defineComponent, h, ref } from "vue";
import { ElNotification } from "element-plus";
import { getEvents, type EventItem } from "@/api/event";
import { getNoticeCount, type NoticeCount } from "@/api/notice";
import {
  EVENT_ACTION_BATCH_READ,
  EVENT_ACTION_BATCH_REMOVE,
  EVENT_ACTION_BATCH_UNREAD,
  EVENT_ACTION_INSERT,
  EVENT_ACTION_READ,
  EVENT_ACTION_REMOVE,
  EVENT_ACTION_UNREAD,
  EVENT_BANNER_BATCH_MS,
  EVENT_LEVEL_ALERT,
  EVENT_LIST_PAGE_SIZE
} from "@/constants/event";
import { PATH_EVENT_WS } from "@/constants/path";
import { router } from "@/router";

export const BELL_EVENT = "event";
export const BELL_SYSTEM = "system";

type PageQuery = {
  level: string;
  page: number;
  pageSize: number;
};

type PageApply = (items: EventItem[], total: number, page: number) => void;

const RECONNECT_WAIT_MS = 1000;

type BannerEvent = {
  id: string;
  occurred_at: string;
  event_label: string;
  stream_name: string | null;
  media_name: string | null;
  pipeline_name: string | null;
  read_at: string | null;
  reader_id: string | null;
  reader_name: string | null;
};

const BANNER_TYPE = {
  [EVENT_ACTION_INSERT]: "warning",
  [EVENT_ACTION_READ]: "success",
  [EVENT_ACTION_UNREAD]: "info",
  [EVENT_ACTION_REMOVE]: "error",
  [EVENT_ACTION_BATCH_READ]: "success",
  [EVENT_ACTION_BATCH_UNREAD]: "info",
  [EVENT_ACTION_BATCH_REMOVE]: "error"
} as const;

const DETAIL_ACTION: Record<string, string> = {
  [EVENT_ACTION_BATCH_READ]: EVENT_ACTION_READ,
  [EVENT_ACTION_BATCH_UNREAD]: EVENT_ACTION_UNREAD,
  [EVENT_ACTION_BATCH_REMOVE]: EVENT_ACTION_REMOVE
};

const sourceOf = (row: BannerEvent) => row.stream_name || row.media_name || "";

const tailOf = (row: BannerEvent) => `(${sourceOf(row)}) ${row.pipeline_name ?? ""}`.trim();

const lineOf = (action: string, row: BannerEvent) => {
  const head = {
    [EVENT_ACTION_INSERT]: `${row.occurred_at}: ${row.event_label} detected.`,
    [EVENT_ACTION_READ]: `${row.occurred_at}: ${row.event_label} read by ${row.reader_name ?? ""}.`,
    [EVENT_ACTION_UNREAD]: `${row.occurred_at}: ${row.event_label} marked unread.`,
    [EVENT_ACTION_REMOVE]: `${row.occurred_at}: ${row.event_label} removed.`
  }[action];
  return `${head ?? ""} ${tailOf(row)}`.trim();
};

const summaryOf = (action: string, rows: BannerEvent[]) => {
  const count = rows.length;
  const text = {
    [EVENT_ACTION_BATCH_READ]: `${count} events read by ${rows[0]?.reader_name ?? ""}`,
    [EVENT_ACTION_BATCH_UNREAD]: `${count} events marked unread`,
    [EVENT_ACTION_BATCH_REMOVE]: `${count} events removed`
  }[action];
  return text ?? "";
};

const batchView = (action: string, rows: BannerEvent[]) =>
  defineComponent({
    setup() {
      const open = ref(false);
      return () =>
        h("div", [
          h("span", summaryOf(action, rows)),
          h(
            "button",
            {
              type: "button",
              style: "margin-left:8px;padding:0;border:0;background:none;cursor:pointer;color:inherit;",
              onClick: () => {
                open.value = !open.value;
              }
            },
            open.value ? "▼" : "▶"
          ),
          open.value
            ? h(
                "div",
                { style: "margin-top:8px;" },
                rows.map(row => h("div", { key: row.id, style: "margin-top:4px;" }, lineOf(DETAIL_ACTION[action], row)))
              )
            : null
        ]);
    }
  });

const showBanner = (action: string, event: BannerEvent | BannerEvent[]) => {
  const batch = Array.isArray(event);
  const kind = BANNER_TYPE[action as keyof typeof BANNER_TYPE];
  ElNotification({
    message: batch ? h(batchView(action, event)) : lineOf(action, event),
    type: kind,
    position: "top-right",
    showClose: true,
    ...(batch ? { duration: EVENT_BANNER_BATCH_MS } : {})
  });
};

export const noticeCount = ref<NoticeCount>({ total: 0, event: 0, system: 0 });
export const bellOpen = ref(false);
export const bellTab = ref(BELL_EVENT);
export const bellEvents = ref<EventItem[]>([]);

let ticket = 0;
let socket: WebSocket | null = null;
let stopped = false;
let started = false;
let readQuery: (() => PageQuery) | null = null;
let applyPage: PageApply | null = null;

const current = (stamp: number) => stamp === ticket;

export const nextStamp = () => ++ticket;

export const isCurrent = (stamp: number) => current(stamp);

const onEventPage = () => router.currentRoute.value.name === "EventIndex" && readQuery !== null && applyPage !== null;

const sameAlertList = (query: PageQuery) =>
  query.level === EVENT_LEVEL_ALERT && query.page === 1 && query.pageSize === EVENT_LIST_PAGE_SIZE;

const applyEvents = (stamp: number, items: EventItem[], total: number, page: number, bell: boolean, pageList: boolean) => {
  if (!current(stamp)) {
    return;
  }
  if (bell) {
    bellEvents.value = items;
  }
  if (pageList && applyPage) {
    applyPage(items, total, page);
  }
};

export const reloadEvents = () => {
  const stamp = ++ticket;
  const query = readQuery ? readQuery() : null;
  const pageList = onEventPage() && query !== null;
  const bellList = bellOpen.value && bellTab.value === BELL_EVENT;
  const shared = pageList && bellList && query !== null && sameAlertList(query);
  getNoticeCount().then(res => {
    if (current(stamp)) {
      noticeCount.value = res.data;
    }
  });
  if (shared && query) {
    getEvents({ level: query.level, page: query.page, page_size: query.pageSize }).then(res => {
      applyEvents(stamp, res.data.items, res.data.total, res.data.page, true, true);
    });
    return;
  }
  if (pageList && query) {
    getEvents({ level: query.level, page: query.page, page_size: query.pageSize }).then(res => {
      applyEvents(stamp, res.data.items, res.data.total, res.data.page, false, true);
    });
  }
  if (bellList) {
    getEvents({ level: EVENT_LEVEL_ALERT, page: 1, page_size: EVENT_LIST_PAGE_SIZE }).then(res => {
      applyEvents(stamp, res.data.items, res.data.total, res.data.page, true, false);
    });
  }
};

const onMessage = (frame: MessageEvent) => {
  const body = JSON.parse(String(frame.data)) as {
    refresh?: boolean;
    action?: string;
    event?: BannerEvent | BannerEvent[];
  };
  if (body.refresh === true) {
    reloadEvents();
  }
  if (body.action && body.event) {
    showBanner(body.action, body.event);
  }
};

const connect = () => {
  if (stopped) {
    return;
  }
  const previous = socket;
  const protocol = location.protocol === "https:" ? "wss" : "ws";
  const next = new WebSocket(`${protocol}://${location.host}${PATH_EVENT_WS}`);
  socket = next;
  previous?.close();
  next.onopen = () => reloadEvents();
  next.onmessage = onMessage;
  next.onclose = () => {
    if (socket !== next) {
      return;
    }
    socket = null;
    if (!stopped) {
      window.setTimeout(connect, RECONNECT_WAIT_MS);
    }
  };
};

const onFocus = () => reloadEvents();

export const startEventRefresh = () => {
  if (started) {
    return;
  }
  started = true;
  window.addEventListener("focus", onFocus);
  connect();
};

if (import.meta.hot) {
  import.meta.hot.dispose(() => {
    stopped = true;
    socket?.close();
    socket = null;
    window.removeEventListener("focus", onFocus);
  });
}

export const registerEventPage = (query: () => PageQuery, apply: PageApply) => {
  readQuery = query;
  applyPage = apply;
  return () => {
    readQuery = null;
    applyPage = null;
  };
};
