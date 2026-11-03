export const PATH_PREVIEW = "/backend/preview";
export const PATH_PREVIEW_ITEM = "/backend/preview/{preset_id}";
export const PATH_PREVIEW_MAP = "/backend/preview/map";
export const PATH_PREVIEW_ACTIVE = "/backend/preview/active";

export const MODE_GRID = "grid";

export const WEBRTC_ORIGIN = "http://127.0.0.1:8889";

export const SLOT_COUNT = 16;

export const LAYOUTS = ["1x1", "2x2", "3x3", "4x4"];

export const whepUrl = (name: string) =>
  `${WEBRTC_ORIGIN}/${encodeURIComponent(name)}/whep`;

export const slotMediaLabel = (width: number, height: number) =>
  width && height ? `${width}X${height}` : "";
