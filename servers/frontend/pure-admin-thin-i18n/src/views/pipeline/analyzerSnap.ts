export type AnalyzerPoint = { x: number; y: number };

export type AnalyzerBox = { x: number; y: number; width: number; height: number };

export type AnalyzerEdge = "left" | "right" | "top" | "bottom";

export type AnalyzerSnap = {
  point: AnalyzerPoint;
  edges: AnalyzerEdge[];
  pointIndex: number | null;
};

const clamp = (value: number, min: number, max: number) => Math.min(Math.max(value, min), max);

const insideRange = (value: number, min: number, max: number) => value >= min && value <= max;

const axisSpan = (size: number) => Math.max(size - 1, 1);

export const stagePoint = (point: AnalyzerPoint, box: AnalyzerBox, size: AnalyzerPoint): AnalyzerPoint => ({
  x: box.x + (point.x / axisSpan(size.x)) * box.width,
  y: box.y + (point.y / axisSpan(size.y)) * box.height
});

export const imagePoint = (point: AnalyzerPoint, box: AnalyzerBox, size: AnalyzerPoint): AnalyzerPoint => {
  const rawX = box.width ? ((point.x - box.x) / box.width) * axisSpan(size.x) : 0;
  const rawY = box.height ? ((point.y - box.y) / box.height) * axisSpan(size.y) : 0;
  return {
    x: clamp(Math.round(rawX), 0, Math.max(size.x - 1, 0)),
    y: clamp(Math.round(rawY), 0, Math.max(size.y - 1, 0))
  };
};

const edgePoints = (edge: AnalyzerEdge, box: AnalyzerBox, stroke: number) => {
  const inset = stroke / 2;
  const right = box.x + box.width;
  const bottom = box.y + box.height;
  const lines = {
    left: [box.x + inset, box.y, box.x + inset, bottom],
    right: [right - inset, box.y, right - inset, bottom],
    top: [box.x, box.y + inset, right, box.y + inset],
    bottom: [box.x, bottom - inset, right, bottom - inset]
  };
  return lines[edge];
};

export const snapEdgeLines = (edges: AnalyzerEdge[], box: AnalyzerBox, stroke: number) => {
  const lines: { key: AnalyzerEdge; points: number[] }[] = [];
  for (const edge of edges) lines.push({ key: edge, points: edgePoints(edge, box, stroke) });
  return lines;
};

export const snapAnalyzerPointer = (
  pointer: AnalyzerPoint | null,
  box: AnalyzerBox,
  size: AnalyzerPoint,
  points: AnalyzerPoint[],
  threshold: number
): AnalyzerSnap | null => {
  const active = Boolean(
    pointer &&
      box.width &&
      box.height &&
      size.x &&
      size.y &&
      insideRange(pointer.x, box.x - threshold, box.x + box.width + threshold) &&
      insideRange(pointer.y, box.y - threshold, box.y + box.height + threshold)
  );
  const maxX = Math.max(size.x - 1, 0);
  const maxY = Math.max(size.y - 1, 0);
  const clampedX = pointer ? clamp(pointer.x, box.x, box.x + box.width) : box.x;
  const clampedY = pointer ? clamp(pointer.y, box.y, box.y + box.height) : box.y;
  const rawX = box.width ? ((clampedX - box.x) / box.width) * size.x : 0;
  const rawY = box.height ? ((clampedY - box.y) / box.height) * size.y : 0;
  const base = {
    x: clamp(Math.floor(rawX), 0, maxX),
    y: clamp(Math.floor(rawY), 0, maxY)
  };
  const left = pointer ? Math.abs(pointer.x - box.x) : Infinity;
  const right = pointer ? Math.abs(pointer.x - (box.x + box.width)) : Infinity;
  const top = pointer ? Math.abs(pointer.y - box.y) : Infinity;
  const bottom = pointer ? Math.abs(pointer.y - (box.y + box.height)) : Infinity;
  const snapLeft = left <= threshold && left <= right;
  const snapRight = right <= threshold && right < left;
  const snapTop = top <= threshold && top <= bottom;
  const snapBottom = bottom <= threshold && bottom < top;
  const edges: AnalyzerEdge[] = [];
  snapLeft && edges.push("left");
  snapRight && edges.push("right");
  snapTop && edges.push("top");
  snapBottom && edges.push("bottom");
  const bounded = {
    x: snapLeft ? 0 : snapRight ? maxX : base.x,
    y: snapTop ? 0 : snapBottom ? maxY : base.y
  };
  const edgeDist = edges.length
    ? Math.hypot(snapLeft || snapRight ? Math.min(left, right) : 0, snapTop || snapBottom ? Math.min(top, bottom) : 0)
    : Infinity;
  let nearestIndex = -1;
  let nearestDist = Infinity;
  for (let index = 0; index < points.length; index += 1) {
    const stage = stagePoint(points[index], box, size);
    const dist = pointer ? Math.hypot(stage.x - pointer.x, stage.y - pointer.y) : Infinity;
    const closer = dist <= threshold && dist < nearestDist;
    nearestIndex = closer ? index : nearestIndex;
    nearestDist = closer ? dist : nearestDist;
  }
  const usePoint = nearestIndex >= 0 && nearestDist <= edgeDist;
  const snap = usePoint
    ? { point: points[nearestIndex], edges: [] as AnalyzerEdge[], pointIndex: nearestIndex }
    : { point: bounded, edges, pointIndex: null };
  return active ? snap : null;
};
