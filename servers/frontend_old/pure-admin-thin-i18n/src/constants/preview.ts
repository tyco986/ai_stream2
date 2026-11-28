export const PATH_PREVIEW = "/backend/preview";
export const PATH_PREVIEW_ITEM = "/backend/preview/{preset_id}";
export const PATH_PREVIEW_MAP = "/backend/preview/map";
export const PATH_PREVIEW_ACTIVE = "/backend/preview/active";

export const MODE_GRID = "grid";

export const WEBRTC_ORIGIN = "http://127.0.0.1:8889";

export const SLOT_COUNT = 16;

export const LAYOUTS = ["1x1", "2x2", "3x3", "4x4"];

export const SHOT_COOLDOWN_MS = 500;

export const PREVIEW_SAVE_DIALOG_WIDTH = "22%";

const pad = (value: number, width = 2) => String(value).padStart(width, "0");

export const shotFilename = (name: string) => {
  const date = new Date();
  const stamp = `${date.getFullYear()}${pad(date.getMonth() + 1)}${pad(date.getDate())}-${pad(date.getHours())}${pad(date.getMinutes())}${pad(date.getSeconds())}${pad(date.getMilliseconds(), 3)}`;
  return `${name}_${stamp}.png`;
};

export const whepUrl = (name: string) =>
  `${WEBRTC_ORIGIN}/${encodeURIComponent(name)}/whep`;

export const slotMediaLabel = (width: number, height: number) =>
  width && height ? `${width}X${height}` : "";
