<template>
  <el-icon :size="size" :color="color">
    <component :is="iconComponent" />
  </el-icon>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import {
  Aim,
  ArrowLeft,
  ArrowRight,
  Brush,
  Check,
  Close,
  Cpu,
  Crop,
  DArrowLeft,
  DArrowRight,
  Delete,
  Document,
  DocumentAdd,
  DocumentCopy,
  Download,
  Edit,
  EditPen,
  FolderOpened,
  FullScreen,
  Grid,
  Microphone,
  Mute,
  Picture,
  Refresh,
  Setting,
  Share,
  Upload,
  VideoCamera,
  VideoPause,
  VideoPlay,
} from '@element-plus/icons-vue'

const ICON_MAP = {
  setting: Setting,
  refresh: Refresh,
  clear: Brush,
  save: DocumentCopy,
  saveAs: DocumentAdd,
  grid: Grid,
  focus: Aim,
  filter: FolderOpened,
  edit: Edit,
  editPen: EditPen,
  delete: Delete,
  document: Document,
  download: Download,
  picture: Picture,
  videoCamera: VideoCamera,
  videoPlay: VideoPlay,
  videoPause: VideoPause,
  arrowRight: ArrowRight,
  arrowLeft: ArrowLeft,
  rewind: DArrowLeft,
  forward: DArrowRight,
  fullScreen: FullScreen,
  volume: Microphone,
  mute: Mute,
  upload: Upload,
  build: Cpu,
  check: Check,
  close: Close,
  crop: Crop,
  share: Share,
} as const

export type UiIconName = keyof typeof ICON_MAP

const props = withDefaults(
  defineProps<{
    name: UiIconName
    size?: number
    color?: string
  }>(),
  {
    size: 18,
  },
)

const iconComponent = computed(() => ICON_MAP[props.name])
</script>
