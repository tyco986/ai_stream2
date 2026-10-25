import { $t } from "@/plugins/i18n";

const Layout = () => import("@/layout/index.vue");

export default {
  path: "/service",
  name: "Service",
  component: Layout,
  redirect: "/service/index",
  meta: {
    icon: "ri/server-line",
    title: $t("menus.pureService"),
    rank: 9
  },
  children: [
    {
      path: "/service/index",
      name: "ServiceIndex",
      component: () => import("@/views/service/index.vue"),
      meta: {
        title: $t("menus.pureService")
      }
    }
  ]
} satisfies RouteConfigsTable;
