import { $t } from "@/plugins/i18n";

const Layout = () => import("@/layout/index.vue");

export default {
  path: "/tool",
  name: "Tool",
  component: Layout,
  redirect: "/tool/index",
  meta: {
    icon: "ri/wrench-line",
    title: $t("menus.pureTool"),
    rank: 7
  },
  children: [
    {
      path: "/tool/index",
      name: "ToolIndex",
      component: () => import("@/views/tool/index.vue"),
      meta: {
        title: $t("menus.pureTool")
      }
    }
  ]
} satisfies RouteConfigsTable;
