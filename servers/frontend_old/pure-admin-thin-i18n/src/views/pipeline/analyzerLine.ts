import type { Context } from "konva/lib/Context";
import type { Shape } from "konva/lib/Shape";
import { ANALYZER_ARROW_HEAD, ANALYZER_ARROW_RADIAN } from "@/constants/pipeline";
import type { AnalyzerPoint } from "@/views/pipeline/analyzerSnap";

export type AnalyzerDirection = {
  name: string;
  x1: number;
  y1: number;
  x2: number;
  y2: number;
};

export type AnalyzerLine = {
  name: string;
  x1: number;
  y1: number;
  x2: number;
  y2: number;
  ax: number;
  ay: number;
  hx: number;
  hy: number;
  extended: boolean;
};

const fullTurn = 360;

const span = (from: number, to: number) => {
  const sweep = (((to - from) % fullTurn) + fullTurn) % fullTurn;
  return sweep > 180 ? sweep - fullTurn : sweep;
};

const clampShift = (points: AnalyzerPoint[], dx: number, dy: number, size: AnalyzerPoint) => {
  const maxX = Math.max(size.x - 1, 0);
  const maxY = Math.max(size.y - 1, 0);
  return {
    x: points.reduce((value, point) => Math.min(Math.max(value, -point.x), maxX - point.x), dx),
    y: points.reduce((value, point) => Math.min(Math.max(value, -point.y), maxY - point.y), dy)
  };
};

export const shiftLine = (line: AnalyzerLine, source: AnalyzerLine, dx: number, dy: number, size: AnalyzerPoint) => {
  const shift = clampShift(
    [
      { x: source.x1, y: source.y1 },
      { x: source.x2, y: source.y2 },
      { x: source.ax, y: source.ay },
      { x: source.hx, y: source.hy }
    ],
    dx,
    dy,
    size
  );
  line.x1 = source.x1 + shift.x;
  line.y1 = source.y1 + shift.y;
  line.x2 = source.x2 + shift.x;
  line.y2 = source.y2 + shift.y;
  line.ax = source.ax + shift.x;
  line.ay = source.ay + shift.y;
  line.hx = source.hx + shift.x;
  line.hy = source.hy + shift.y;
};

export const shiftDirection = (direction: AnalyzerDirection, source: AnalyzerDirection, dx: number, dy: number, size: AnalyzerPoint) => {
  const shift = clampShift(
    [
      { x: source.x1, y: source.y1 },
      { x: source.x2, y: source.y2 }
    ],
    dx,
    dy,
    size
  );
  direction.x1 = source.x1 + shift.x;
  direction.y1 = source.y1 + shift.y;
  direction.x2 = source.x2 + shift.x;
  direction.y2 = source.y2 + shift.y;
};

export const segmentDistance = (start: AnalyzerPoint, end: AnalyzerPoint, point: AnalyzerPoint) => {
  const nearest = projectSegment(start, end, point);
  return Math.hypot(point.x - nearest.x, point.y - nearest.y);
};

export const projectSegment = (start: AnalyzerPoint, end: AnalyzerPoint, point: AnalyzerPoint): AnalyzerPoint => {
  const vx = end.x - start.x;
  const vy = end.y - start.y;
  const length = vx * vx + vy * vy;
  const raw = length ? ((point.x - start.x) * vx + (point.y - start.y) * vy) / length : 0;
  const t = Math.min(Math.max(raw, 0), 1);
  return { x: start.x + t * vx, y: start.y + t * vy };
};

const placeAnchor = (line: AnalyzerLine, point: AnalyzerPoint) => {
  const projected = projectSegment({ x: line.x1, y: line.y1 }, { x: line.x2, y: line.y2 }, point);
  line.ax = projected.x;
  line.ay = projected.y;
};

const setLineStart = (line: AnalyzerLine, point: AnalyzerPoint) => {
  line.x1 = point.x;
  line.y1 = point.y;
  placeAnchor(line, { x: line.ax, y: line.ay });
};

const setLineEnd = (line: AnalyzerLine, point: AnalyzerPoint) => {
  line.x2 = point.x;
  line.y2 = point.y;
  placeAnchor(line, { x: line.ax, y: line.ay });
};

const setLineAnchor = (line: AnalyzerLine, point: AnalyzerPoint) => {
  placeAnchor(line, point);
};

const setLineHead = (line: AnalyzerLine, point: AnalyzerPoint) => {
  line.hx = point.x;
  line.hy = point.y;
};

const linePointMoves: Record<string, (line: AnalyzerLine, point: AnalyzerPoint) => void> = {
  start: setLineStart,
  end: setLineEnd,
  anchor: setLineAnchor,
  head: setLineHead
};

const setDirTail = (direction: AnalyzerDirection, point: AnalyzerPoint) => {
  direction.x1 = point.x;
  direction.y1 = point.y;
};

const setDirHead = (direction: AnalyzerDirection, point: AnalyzerPoint) => {
  direction.x2 = point.x;
  direction.y2 = point.y;
};

const dirPointMoves: Record<string, (direction: AnalyzerDirection, point: AnalyzerPoint) => void> = {
  tail: setDirTail,
  head: setDirHead
};

export const moveLinePoint = (line: AnalyzerLine, key: string, point: AnalyzerPoint) => {
  linePointMoves[key]?.(line, point);
};

export const moveDirPoint = (direction: AnalyzerDirection, key: string, point: AnalyzerPoint) => {
  dirPointMoves[key]?.(direction, point);
};

export const apart = (start: AnalyzerPoint, end: AnalyzerPoint) => start.x !== end.x || start.y !== end.y;

export const clipLine = (start: AnalyzerPoint, end: AnalyzerPoint, size: AnalyzerPoint): [AnalyzerPoint, AnalyzerPoint] => {
  const dx = end.x - start.x;
  const dy = end.y - start.y;
  const maxX = Math.max(size.x - 1, 0);
  const maxY = Math.max(size.y - 1, 0);
  const candidates = [
    dx ? (0 - start.x) / dx : null,
    dx ? (maxX - start.x) / dx : null,
    dy ? (0 - start.y) / dy : null,
    dy ? (maxY - start.y) / dy : null
  ];
  const hits: { t: number; x: number; y: number }[] = [];
  for (const t of candidates) {
    const x = t === null ? 0 : start.x + t * dx;
    const y = t === null ? 0 : start.y + t * dy;
    const inside = t !== null && x >= -1e-4 && x <= maxX + 1e-4 && y >= -1e-4 && y <= maxY + 1e-4;
    inside && hits.push({ t, x: Math.min(Math.max(x, 0), maxX), y: Math.min(Math.max(y, 0), maxY) });
  }
  let near = hits[0];
  let far = hits[0];
  for (const hit of hits) {
    near = hit.t < near.t ? hit : near;
    far = hit.t > far.t ? hit : far;
  }
  return near && far && apart(near, far) ? [near, far] : [start, end];
};

export const arrowWings = (tail: AnalyzerPoint, head: AnalyzerPoint): [AnalyzerPoint, AnalyzerPoint] => {
  const cos = Math.cos(ANALYZER_ARROW_RADIAN);
  const sin = Math.sin(ANALYZER_ARROW_RADIAN);
  const dx = tail.x - head.x;
  const dy = tail.y - head.y;
  return [
    {
      x: head.x + ANALYZER_ARROW_HEAD * (dx * cos + dy * sin),
      y: head.y + ANALYZER_ARROW_HEAD * (dy * cos - dx * sin)
    },
    {
      x: head.x + ANALYZER_ARROW_HEAD * (dx * cos - dy * sin),
      y: head.y + ANALYZER_ARROW_HEAD * (dy * cos + dx * sin)
    }
  ];
};

export const lineAngle = (lineStart: AnalyzerPoint, lineEnd: AnalyzerPoint, tail: AnalyzerPoint, head: AnalyzerPoint) => {
  const line = (Math.atan2(lineEnd.y - lineStart.y, lineEnd.x - lineStart.x) * 180) / Math.PI;
  const arrow = (Math.atan2(head.y - tail.y, head.x - tail.x) * 180) / Math.PI;
  const forward = span(line, arrow);
  const back = span(line + 180, arrow);
  const sweep = Math.abs(forward) <= Math.abs(back) ? forward : back;
  const base = Math.abs(forward) <= Math.abs(back) ? line : line + 180;
  const start = sweep >= 0 ? base : base + sweep;
  return { start: ((start % fullTurn) + fullTurn) % fullTurn, sweep: Math.abs(sweep) };
};

export const strokeAngleArc = (context: Context, shape: Shape) => {
  const radius = shape.getAttr("radius") as number;
  const start = ((shape.getAttr("start") as number) * Math.PI) / 180;
  const sweep = ((shape.getAttr("sweep") as number) * Math.PI) / 180;
  context.beginPath();
  context.arc(0, 0, radius, start, start + sweep, false);
  context.strokeShape(shape);
};
