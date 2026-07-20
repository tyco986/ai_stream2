export type DrawTool = 'rectangle' | 'polygon' | 'line_direction' | 'direction' | 'edit'

export type AnnotationModeType =
  | 'roi_filtering'
  | 'overcrowding'
  | 'line_crossing'
  | 'direction_detection'

export const ANNOTATION_TYPE_COLORS: Record<AnnotationModeType, string> = {
  roi_filtering: '#F5C518',
  overcrowding: '#FF8C00',
  line_crossing: '#22C55E',
  direction_detection: '#FF0000',
}

export type ViewTransform = {
  scale: number
  offsetX: number
  offsetY: number
}

export function colorForAnnotationType(type: AnnotationModeType): string {
  return ANNOTATION_TYPE_COLORS[type]
}

export function hexToRgba(hex: string, alpha: number): string {
  const r = Number.parseInt(hex.slice(1, 3), 16)
  const g = Number.parseInt(hex.slice(3, 5), 16)
  const b = Number.parseInt(hex.slice(5, 7), 16)
  return `rgba(${r}, ${g}, ${b}, ${alpha})`
}

export function computeViewTransform(
  stageWidth: number,
  stageHeight: number,
  imageWidth: number,
  imageHeight: number,
): ViewTransform {
  const safeImageWidth = Math.max(imageWidth, 1)
  const safeImageHeight = Math.max(imageHeight, 1)
  const scale = Math.min(stageWidth / safeImageWidth, stageHeight / safeImageHeight)
  return {
    scale,
    offsetX: (stageWidth - safeImageWidth * scale) / 2,
    offsetY: (stageHeight - safeImageHeight * scale) / 2,
  }
}

export function pixelToStage(
  pixelX: number,
  pixelY: number,
  transform: ViewTransform,
): { x: number; y: number } {
  return {
    x: transform.offsetX + pixelX * transform.scale,
    y: transform.offsetY + pixelY * transform.scale,
  }
}

export function clampStagePointToImage(
  stageX: number,
  stageY: number,
  transform: ViewTransform,
  imageWidth: number,
  imageHeight: number,
): { x: number; y: number } {
  const right = transform.offsetX + imageWidth * transform.scale
  const bottom = transform.offsetY + imageHeight * transform.scale
  return {
    x: Math.min(Math.max(stageX, transform.offsetX), right),
    y: Math.min(Math.max(stageY, transform.offsetY), bottom),
  }
}

export function stageToPixel(
  stageX: number,
  stageY: number,
  transform: ViewTransform,
  imageWidth: number,
  imageHeight: number,
): { x: number; y: number } {
  const clamped = clampStagePointToImage(
    stageX,
    stageY,
    transform,
    imageWidth,
    imageHeight,
  )
  return {
    x: Math.round((clamped.x - transform.offsetX) / transform.scale),
    y: Math.round((clamped.y - transform.offsetY) / transform.scale),
  }
}

export function pixelsToStageFlat(points: number[], transform: ViewTransform): number[] {
  const result: number[] = []
  for (let i = 0; i < points.length; i += 2) {
    const mapped = pixelToStage(points[i], points[i + 1], transform)
    result.push(mapped.x, mapped.y)
  }
  return result
}

export function rectCornersToPoints(
  x1: number,
  y1: number,
  x2: number,
  y2: number,
): number[] {
  const left = Math.min(x1, x2)
  const right = Math.max(x1, x2)
  const top = Math.min(y1, y2)
  const bottom = Math.max(y1, y2)
  return [left, top, right, top, right, bottom, left, bottom]
}

export function distanceStage(
  ax: number,
  ay: number,
  bx: number,
  by: number,
): number {
  const dx = ax - bx
  const dy = ay - by
  return Math.sqrt(dx * dx + dy * dy)
}

/** Stage-pixel snap distance to close a polygon on the first vertex. */
export const POLYGON_CLOSE_DISTANCE = 8

export function isPolygonCloseSnap(
  pointer: { x: number; y: number },
  draftPixelPoints: number[],
  transform: ViewTransform,
): boolean {
  if (draftPixelPoints.length < 6) {
    return false
  }
  const first = pixelToStage(draftPixelPoints[0], draftPixelPoints[1], transform)
  return distanceStage(pointer.x, pointer.y, first.x, first.y) <= POLYGON_CLOSE_DISTANCE
}

export function defaultToolForMode(mode: AnnotationModeType): DrawTool {
  const map: Record<AnnotationModeType, DrawTool> = {
    roi_filtering: 'rectangle',
    overcrowding: 'rectangle',
    line_crossing: 'line_direction',
    direction_detection: 'direction',
  }
  return map[mode]
}

export function isToolAllowed(mode: AnnotationModeType, tool: DrawTool): boolean {
  if (tool === 'edit') {
    return true
  }
  const allowed: Record<AnnotationModeType, DrawTool[]> = {
    roi_filtering: ['rectangle', 'polygon'],
    overcrowding: ['rectangle', 'polygon'],
    line_crossing: ['line_direction'],
    direction_detection: ['direction'],
  }
  return allowed[mode].includes(tool)
}

