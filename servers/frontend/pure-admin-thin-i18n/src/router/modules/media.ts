import { $t } from "@/plugins/i18n";

const Layout = () => import("@/layout/index.vue");

export default {
  path: "/media",
  name: "Media",
  component: Layout,
  redirect: "/media/index",
  meta: {
    icon: "ri/folder-image-line",
    title: $t("menus.pureMedia"),
    rank: 6
  },
  children: [
    {
      path: "/media/index",
      name: "MediaIndex",
      component: () => import("@/views/media/index.vue"),
      meta: {
        title: $t("menus.pureMedia")
      }
    }
  ]
} satisfies RouteConfigsTable;
