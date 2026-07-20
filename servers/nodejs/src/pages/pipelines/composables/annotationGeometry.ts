import type { Annotation } from '@/api/pipelines'
import { rectCornersToPoints, type ViewTransform } from './useAnalyzerGeometry'

export type AnnotationShape = Annotation['shape']

export function pointsFromClicks(shape: AnnotationShape, clicks: number[]): number[] {
  if (shape === 'rectangle' && clicks.length >= 4) {
    return rectCornersToPoints(clicks[0], clicks[1], clicks[2], clicks[3])
  }
  if (shape === 'line_direction' && clicks.length >= 8) {
    return [
      clicks[4],
      clicks[5],
      clicks[6],
      clicks[7],
      clicks[0],
      clicks[1],
      clicks[2],
      clicks[3],
    ]
  }
  return [...clicks]
}

export function clicksFromPoints(shape: AnnotationShape, points: number[]): number[] {
  if (shape === 'rectangle' && points.length >= 8) {
    return [points[0], points[1], points[4], points[5]]
  }
  if (shape === 'line_direction' && points.length >= 8) {
    return [
      points[4],
      points[5],
      points[6],
      points[7],
      points[0],
      points[1],
      points[2],
      points[3],
    ]
  }
  return [...points]
}

export function getClickPoints(item: {
  shape: AnnotationShape
  points: number[]
  click_points: number[] | null
}): number[] {
  if (item.click_points?.length) {
    return item.click_points
  }
  return clicksFromPoints(item.shape, item.points)
}

export function applyClicksToAnnotation(item: Annotation, clicks: number[]) {
  item.click_points = [...clicks]
  item.points = pointsFromClicks(item.shape, clicks)
}

export function normalizeAnnotation(row: Annotation): Annotation {
  const item: Annotation = {
    ...row,
    points: [...row.points],
    click_points: row.click_points ? [...row.click_points] : null,
  }
  applyClicksToAnnotation(item, getClickPoints(item))
  return item
}

export function translateClicks(clicks: number[], dx: number, dy: number): number[] {
  return clicks.map((value, index) => (index % 2 === 0 ? value + dx : value + dy))
}

export function moveClickVertex(
  clicks: number[],
  clickIndex: number,
  x: number,
  y: number,
): number[] {
  if (clickIndex < 0 || clickIndex + 1 >= clicks.length) {
    return clicks
  }
  const next = [...clicks]
  next[clickIndex] = x
  next[clickIndex + 1] = y
  return next
}

export function boundsOfFlat(points: number[]): {
  minX: number
  minY: number
  maxX: number
  maxY: number
} {
  let minX = Number.POSITIVE_INFINITY
  let minY = Number.POSITIVE_INFINITY
  let maxX = Number.NEGATIVE_INFINITY
  let maxY = Number.NEGATIVE_INFINITY
  for (let i = 0; i < points.length; i += 2) {
    minX = Math.min(minX, points[i])
    maxX = Math.max(maxX, points[i])
    minY = Math.min(minY, points[i + 1])
    maxY = Math.max(maxY, points[i + 1])
  }
  return { minX, minY, maxX, maxY }
}

export function clampTranslateDelta(
  points: number[],
  dx: number,
  dy: number,
  imageWidth: number,
  imageHeight: number,
): { dx: number; dy: number } {
  if (points.length < 2) {
    return { dx: 0, dy: 0 }
  }
  const { minX, minY, maxX, maxY } = boundsOfFlat(points)
  return {
    dx: Math.min(Math.max(dx, -minX), imageWidth - maxX),
    dy: Math.min(Math.max(dy, -minY), imageHeight - maxY),
  }
}

export function clampGroupDragPos(
  points: number[],
  pos: { x: number; y: number },
  transform: ViewTransform,
  imageWidth: number,
  imageHeight: number,
): { x: number; y: number } {
  const clamped = clampTranslateDelta(
    points,
    pos.x / transform.scale,
    pos.y / transform.scale,
    imageWidth,
    imageHeight,
  )
  return {
    x: clamped.dx * transform.scale,
    y: clamped.dy * transform.scale,
  }
}
