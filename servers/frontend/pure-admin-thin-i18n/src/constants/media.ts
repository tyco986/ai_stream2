import { assertColumnPercents } from "@/utils/columnWidth";

export const MEDIA_UPLOAD_ACCEPT = ".jpg,.jpeg,.png,.mp4";

export const MEDIA_PAGE = 1;
export const MEDIA_PAGE_SIZE = 20;

export const MEDIA_UPLOAD_COLUMN_PERCENTS = {
  selection: 5,
  name: 27,
  width: 7,
  height: 8,
  fps: 6,
  duration: 10,
  size: 12,
  updatedAt: 12,
  ops: 13
};

assertColumnPercents(MEDIA_UPLOAD_COLUMN_PERCENTS);

export const MEDIA_SECTION_RECORDING = "recording";
export const MEDIA_SECTION_PIPELINE = "pipeline";
export const MEDIA_SECTION_UPLOAD = "upload";

export const MEDIA_SECTIONS = [
  MEDIA_SECTION_RECORDING,
  MEDIA_SECTION_PIPELINE,
  MEDIA_SECTION_UPLOAD
] as const;

export type MediaSection = (typeof MEDIA_SECTIONS)[number];
