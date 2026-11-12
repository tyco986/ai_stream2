import type { GieClassAttrs } from "@/api/pipelineGie";

export type AttrRow = {
  classKey: string;
  conf: number | null;
  maxDet: number | null;
  minW: number | null;
  minH: number | null;
  maxW: number | null;
  maxH: number | null;
};

export const emptyAttr = (classKey: string): AttrRow => ({
  classKey,
  conf: null,
  maxDet: null,
  minW: null,
  minH: null,
  maxW: null,
  maxH: null
});

const attrNumber = (value: unknown) => {
  const number = Number(value);
  const ready = value !== null && value !== undefined && value !== "" && Number.isFinite(number);
  return ready ? number : null;
};

export const attrRows = (classAttrs: GieClassAttrs | null): AttrRow[] =>
  Object.entries(classAttrs ?? {}).map(([classKey, fields]) => ({
    classKey,
    conf: attrNumber(fields.conf),
    maxDet: attrNumber(fields.topk),
    minW: attrNumber(fields.detected_min_w),
    minH: attrNumber(fields.detected_min_h),
    maxW: attrNumber(fields.detected_max_w),
    maxH: attrNumber(fields.detected_max_h)
  }));

export const attrFields = (row: AttrRow) => ({
  conf: attrNumber(row.conf),
  topk: attrNumber(row.maxDet),
  detected_min_w: attrNumber(row.minW),
  detected_min_h: attrNumber(row.minH),
  detected_max_w: attrNumber(row.maxW),
  detected_max_h: attrNumber(row.maxH)
});

export const attrMap = (rows: AttrRow[]): GieClassAttrs =>
  Object.fromEntries(
    rows
      .filter(row => row.classKey)
      .map(row => [
        row.classKey,
        Object.fromEntries(Object.entries(attrFields(row)).filter((entry): entry is [string, number] => entry[1] !== null))
      ])
  );
