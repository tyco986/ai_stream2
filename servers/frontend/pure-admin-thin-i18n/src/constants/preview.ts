export const PATH_PREVIEW_ACTIVE = "/ai_stream2/backend/preview/active";

export const WEBRTC_ORIGIN = "http://127.0.0.1:8889";

export const SLOT_COUNT = 16;

export const LAYOUTS = ["1x1", "2x2", "3x3", "4x4"];

export const whepUrl = (name: string) =>
  `${WEBRTC_ORIGIN}/${encodeURIComponent(name)}/whep`;

export const slotMediaLabel = (width: number, height: number) =>
  width && height ? `${width}X${height}` : "";
