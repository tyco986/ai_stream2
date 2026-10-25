// 这里存放本地图标，在 src/layout/index.vue 文件中加载，避免在首启动加载
import { getSvgInfo } from "@pureadmin/utils";
import { addIcon } from "@iconify/vue/dist/offline";

// https://icon-sets.iconify.design/ep/?keyword=ep
import EpHomeFilled from "~icons/ep/home-filled?raw";
import EpVideoCamera from "~icons/ep/video-camera?raw";

// https://icon-sets.iconify.design/ri/?keyword=ri
import RiSearchLine from "~icons/ri/search-line?raw";
import RiWrenchLine from "~icons/ri/wrench-line?raw";
import RiMovie2Line from "~icons/ri/movie-2-line?raw";
import RiFolderImageLine from "~icons/ri/folder-image-line?raw";
import RiEyeLine from "~icons/ri/eye-line?raw";
import RiBrainLine from "~icons/ri/brain-line?raw";
import RiFlowChart from "~icons/ri/flow-chart?raw";
import RiNotification3Line from "~icons/ri/notification-3-line?raw";
import RiServerLine from "~icons/ri/server-line?raw";

const icons = [
  // Element Plus Icon: https://github.com/element-plus/element-plus-icons
  ["ep/home-filled", EpHomeFilled],
  ["ep/video-camera", EpVideoCamera],
  // Remix Icon: https://github.com/Remix-Design/RemixIcon
  ["ri/search-line", RiSearchLine],
  ["ri/wrench-line", RiWrenchLine],
  ["ri/movie-2-line", RiMovie2Line],
  ["ri/folder-image-line", RiFolderImageLine],
  ["ri/eye-line", RiEyeLine],
  ["ri/brain-line", RiBrainLine],
  ["ri/flow-chart", RiFlowChart],
  ["ri/notification-3-line", RiNotification3Line],
  ["ri/server-line", RiServerLine]
];

// 本地菜单图标，后端在路由的 icon 中返回对应的图标字符串并且前端在此处使用 addIcon 添加即可渲染菜单图标
icons.forEach(([name, icon]) => {
  addIcon(name as string, getSvgInfo(icon as string));
});
