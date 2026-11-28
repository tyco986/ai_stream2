import { $t } from "@/plugins/i18n";

const Layout = () => import("@/layout/index.vue");

export default {
  path: "/preview",
  name: "Preview",
  component: Layout,
  redirect: "/preview/index",
  meta: {
    icon: "ri/eye-line",
    title: $t("menus.purePreview"),
    rank: 5
  },
  children: [
    {
      path: "/preview/index",
      name: "PreviewIndex",
      component: () => import("@/views/preview/index.vue"),
      meta: {
        title: $t("menus.purePreview")
      }
    }
  ]
} satisfies RouteConfigsTable;
