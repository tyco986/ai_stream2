import { $t } from "@/plugins/i18n";

const Layout = () => import("@/layout/index.vue");

export default {
  path: "/event",
  name: "Event",
  component: Layout,
  redirect: "/event/index",
  meta: {
    icon: "ri/notification-3-line",
    title: $t("menus.pureEvent"),
    rank: 4
  },
  children: [
    {
      path: "/event/index",
      name: "EventIndex",
      component: () => import("@/views/event/index.vue"),
      meta: {
        title: $t("menus.pureEvent")
      }
    }
  ]
} satisfies RouteConfigsTable;
