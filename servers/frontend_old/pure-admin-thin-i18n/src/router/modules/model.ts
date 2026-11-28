import { $t } from "@/plugins/i18n";

const Layout = () => import("@/layout/index.vue");

export default {
  path: "/model",
  name: "Model",
  component: Layout,
  redirect: "/model/index",
  meta: {
    icon: "ri/brain-line",
    title: $t("menus.pureModel"),
    rank: 2
  },
  children: [
    {
      path: "/model/index",
      name: "ModelIndex",
      component: () => import("@/views/model/index.vue"),
      meta: {
        title: $t("menus.pureModel")
      }
    }
  ]
} satisfies RouteConfigsTable;
