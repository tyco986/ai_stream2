<script setup lang="ts">
import { inject, onBeforeUnmount, onMounted, ref } from "vue";
import Camera from "~icons/ri/camera-line";
import Close from "~icons/ri/close-line";
import FullscreenExit from "~icons/ri/fullscreen-exit-line";
import Fullscreen from "~icons/ri/fullscreen-line";
import Pause from "~icons/ri/pause-line";
import Play from "~icons/ri/play-line";
import { slotMediaLabel } from "@/constants/preview";
import { $t, transformI18n } from "@/plugins/i18n";
import { requireStreamPlayers, streamPlayersKey } from "./players";

const props = defineProps<{
  streamId: string;
  url: string;
}>();

const emit = defineEmits<{
  clear: [];
}>();

const shell = ref<HTMLElement>();
const host = ref<HTMLElement>();
const paused = ref(false);
const fullscreen = ref(false);
const width = ref(0);
const height = ref(0);
const players = requireStreamPlayers(inject(streamPlayersKey));

const videoOf = (element: HTMLElement | undefined) => element?.querySelector("video") ?? null;

const syncSize = () => {
  const video = videoOf(host.value);
  width.value = video?.videoWidth ?? 0;
  height.value = video?.videoHeight ?? 0;
};

const syncPaused = () => {
  const video = videoOf(host.value);
  paused.value = Boolean(video?.paused);
};

const bindVideo = (video: HTMLVideoElement | null) => {
  video?.addEventListener("resize", syncSize);
  video?.addEventListener("pause", syncPaused);
  video?.addEventListener("play", syncPaused);
};

const unbindVideo = (video: HTMLVideoElement | null) => {
  video?.removeEventListener("resize", syncSize);
  video?.removeEventListener("pause", syncPaused);
  video?.removeEventListener("play", syncPaused);
};

const applyPlayback = (video: HTMLVideoElement | null, pausedNow: boolean) => {
  const pending = pausedNow ? video?.play() : video?.pause();
  void pending;
};

const toggle = () => {
  const video = videoOf(host.value);
  applyPlayback(video, Boolean(video?.paused));
};

const capture = () => {
  const video = videoOf(host.value);
  const canvas = document.createElement("canvas");
  canvas.width = video?.videoWidth ?? 0;
  canvas.height = video?.videoHeight ?? 0;
  const context = canvas.getContext("2d");
  if (video && context && canvas.width) {
    context.drawImage(video, 0, 0);
    const link = document.createElement("a");
    link.href = canvas.toDataURL("image/png");
    link.download = `${props.streamId}.png`;
    link.click();
  }
};

const syncFullscreen = () => {
  fullscreen.value = document.fullscreenElement === shell.value;
};

const toggleFullscreen = () => {
  const element = shell.value;
  const pending =
    document.fullscreenElement === element ? document.exitFullscreen() : element?.requestFullscreen();
  void pending;
};

onMounted(() => {
  document.addEventListener("fullscreenchange", syncFullscreen);
  const element = host.value;
  if (element) {
    players.attach(element, props.streamId, props.url);
    bindVideo(videoOf(element));
    syncSize();
    syncPaused();
  }
});

onBeforeUnmount(() => {
  document.removeEventListener("fullscreenchange", syncFullscreen);
  const element = host.value;
  unbindVideo(videoOf(element));
  if (element) {
    players.detach(element);
  }
});
</script>

<template>
  <div ref="shell" class="preview-shell">
    <div ref="host" class="preview-host" />
    <div class="preview-controls" @click.stop>
      <div class="preview-tools">
        <button
          type="button"
          class="preview-ctl"
          :title="transformI18n($t(paused ? 'preview.purePlay' : 'preview.purePause'))"
          @click="toggle"
        >
          <IconifyIconOffline :icon="paused ? Play : Pause" />
        </button>
        <button
          type="button"
          class="preview-ctl"
          :title="transformI18n($t('preview.pureShot'))"
          @click="capture"
        >
          <IconifyIconOffline :icon="Camera" />
        </button>
        <button
          type="button"
          class="preview-ctl"
          :title="transformI18n($t('preview.pureFullscreen'))"
          @click="toggleFullscreen"
        >
          <IconifyIconOffline :icon="fullscreen ? FullscreenExit : Fullscreen" />
        </button>
        <span class="preview-label">{{ slotMediaLabel(width, height) }}</span>
      </div>
      <button
        type="button"
        class="preview-ctl"
        :title="transformI18n($t('preview.pureUnbind'))"
        @click="emit('clear')"
      >
        <IconifyIconOffline :icon="Close" />
      </button>
    </div>
  </div>
</template>

<style scoped>
.preview-shell {
  position: relative;
  width: 100%;
  height: 100%;
}

.preview-host {
  width: 100%;
  height: 100%;
}

.preview-host :deep(video) {
  display: block;
  width: 100%;
  height: 100%;
  object-fit: contain;
  background: #111;
}

.preview-controls {
  position: absolute;
  top: 0;
  right: 0;
  left: 0;
  display: flex;
  align-items: center;
  justify-content: space-between;
  height: 28px;
  padding: 0 6px;
  color: #fff;
  pointer-events: none;
  background: rgb(0 0 0 / 55%);
  opacity: 0;
}

.preview-tools {
  display: flex;
  gap: 8px;
  align-items: center;
  min-width: 0;
}

.preview-shell:hover .preview-controls {
  pointer-events: auto;
  opacity: 1;
}

.preview-label {
  overflow: hidden;
  font-size: 12px;
  white-space: nowrap;
}

.preview-ctl {
  display: flex;
  align-items: center;
  padding: 0;
  font-size: 16px;
  color: inherit;
  cursor: pointer;
  background: transparent;
  border: 0;
}
</style>
