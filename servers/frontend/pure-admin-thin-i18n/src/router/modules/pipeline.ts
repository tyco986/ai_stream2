import { $t } from "@/plugins/i18n";

const Layout = () => import("@/layout/index.vue");

export default {
  path: "/pipeline",
  name: "Pipeline",
  component: Layout,
  redirect: "/pipeline/index",
  meta: {
    icon: "ri/flow-chart",
    title: $t("menus.purePipeline"),
    rank: 3
  },
  children: [
    {
      path: "/pipeline/index",
      name: "PipelineIndex",
      component: () => import("@/views/pipeline/index.vue"),
      meta: {
        title: $t("menus.purePipeline")
      }
    }
  ]
} satisfies RouteConfigsTable;
