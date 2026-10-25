import { $t } from "@/plugins/i18n";

const Layout = () => import("@/layout/index.vue");

export default {
  path: "/stream",
  name: "Stream",
  component: Layout,
  redirect: "/stream/index",
  meta: {
    icon: "ep/video-camera",
    title: $t("menus.pureStream"),
    rank: 1
  },
  children: [
    {
      path: "/stream/index",
      name: "StreamIndex",
      component: () => import("@/views/stream/index.vue"),
      meta: {
        title: $t("menus.pureStream")
      }
    }
  ]
} satisfies RouteConfigsTable;
