import { stagePoint, type AnalyzerBox, type AnalyzerPoint } from "@/views/pipeline/analyzerSnap";

export type AnalyzerRect = {
  name: string;
  rule: string;
  x: number;
  y: number;
  width: number;
  height: number;
};

export const imageRect = (start: AnalyzerPoint, end: AnalyzerPoint): AnalyzerRect => ({
  name: "",
  rule: "",
  x: Math.min(start.x, end.x),
  y: Math.min(start.y, end.y),
  width: Math.abs(end.x - start.x),
  height: Math.abs(end.y - start.y)
});

const rectOpposite = (rect: AnalyzerRect, key: string): AnalyzerPoint | null => {
  const corners: Record<string, AnalyzerPoint> = {
    nw: { x: rect.x + rect.width, y: rect.y + rect.height },
    ne: { x: rect.x, y: rect.y + rect.height },
    se: { x: rect.x, y: rect.y },
    sw: { x: rect.x + rect.width, y: rect.y }
  };
  return corners[key] ?? null;
};

export const shiftRect = (rect: AnalyzerRect, source: AnalyzerRect, dx: number, dy: number, size: AnalyzerPoint) => {
  const maxX = Math.max(size.x - 1 - source.width, 0);
  const maxY = Math.max(size.y - 1 - source.height, 0);
  rect.x = Math.min(Math.max(source.x + dx, 0), maxX);
  rect.y = Math.min(Math.max(source.y + dy, 0), maxY);
};

export const oppositeRectCorner = (rect: AnalyzerRect, key: string) => rectOpposite(rect, key);

export const moveRectBetween = (rect: AnalyzerRect, fixed: AnalyzerPoint, point: AnalyzerPoint) => {
  const next = imageRect(fixed, point);
  const kept = next.width && next.height ? next : null;
  kept && (rect.x = kept.x);
  kept && (rect.y = kept.y);
  kept && (rect.width = kept.width);
  kept && (rect.height = kept.height);
};

export const stageRect = (rect: AnalyzerRect, box: AnalyzerBox, size: AnalyzerPoint) => {
  const origin = stagePoint({ x: rect.x, y: rect.y }, box, size);
  const far = stagePoint({ x: rect.x + rect.width, y: rect.y + rect.height }, box, size);
  return {
    x: origin.x,
    y: origin.y,
    width: far.x - origin.x,
    height: far.y - origin.y
  };
};

const clamp = (value: number, min: number, max: number) => Math.min(Math.max(value, min), max);

export const tipOrigin = (
  pointer: AnalyzerPoint,
  canvas: AnalyzerPoint,
  tip: AnalyzerPoint,
  offset: number
): AnalyzerPoint => ({
  x: clamp(pointer.x + offset, 0, Math.max(canvas.x - tip.x, 0)),
  y: clamp(pointer.y + offset, 0, Math.max(canvas.y - tip.y, 0))
});
