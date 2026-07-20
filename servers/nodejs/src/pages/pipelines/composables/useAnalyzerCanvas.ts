import { computed, onMounted, onUnmounted, ref, type Ref } from 'vue'
import type { Annotation } from '@/api/pipelines'
import { pointsFromClicks } from './annotationGeometry'
import {
  clampStagePointToImage,
  computeViewTransform,
  isPolygonCloseSnap,
  isToolAllowed,
  pixelToStage,
  pixelsToStageFlat,
  stageToPixel,
  type AnnotationModeType,
  type DrawTool,
  type ViewTransform,
} from './useAnalyzerGeometry'

export type DraftPreview = {
  kind: 'rect' | 'polyline' | 'line' | 'arrow'
  points: number[]
  closed?: boolean
}

export type PendingDraft = {
  type: AnnotationModeType
  shape: Annotation['shape']
  points: number[]
  clickPoints: number[]
  popoverX: number
  popoverY: number
}

const BUTTON_LEFT = 0
const BUTTON_RIGHT = 2

export function useAnalyzerCanvas(params: {
  stageWidth: Ref<number>
  stageHeight: Ref<number>
  imageWidth: Ref<number>
  imageHeight: Ref<number>
  hasImage: Ref<boolean>
  enabled: Ref<boolean>
  tool: Ref<DrawTool>
  annotationType: Ref<AnnotationModeType>
  onDraftComplete: (draft: PendingDraft) => void
}) {
  /** Image-clamped stage cursor used for rubber-band preview; sticky while drafting. */
  const drawPointer = ref<{ x: number; y: number } | null>(null)
  const pointerInside = ref(false)
  const draftPoints = ref<number[]>([])
  const pendingDraft = ref<PendingDraft | null>(null)

  const transform = computed<ViewTransform>(() =>
    computeViewTransform(
      params.stageWidth.value,
      params.stageHeight.value,
      params.imageWidth.value,
      params.imageHeight.value,
    ),
  )

  const drafting = computed(() => draftPoints.value.length > 0)

  const canDraw = computed(
    () =>
      params.enabled.value &&
      params.hasImage.value &&
      params.tool.value !== 'edit' &&
      isToolAllowed(params.annotationType.value, params.tool.value),
  )

  const crosshairVisible = computed(
    () =>
      canDraw.value &&
      pointerInside.value &&
      drawPointer.value !== null &&
      pendingDraft.value === null,
  )

  const crosshairLines = computed(() => {
    const point = drawPointer.value
    if (!point) {
      return []
    }
    return [
      [0, point.y, params.stageWidth.value, point.y],
      [point.x, 0, point.x, params.stageHeight.value],
    ]
  })

  const draftHandlePoints = computed(() => {
    if (pendingDraft.value) {
      return [] as { x: number; y: number }[]
    }
    const tf = transform.value
    const handles = [] as { x: number; y: number }[]
    for (let i = 0; i < draftPoints.value.length; i += 2) {
      handles.push(pixelToStage(draftPoints.value[i], draftPoints.value[i + 1], tf))
    }
    return handles
  })

  const polygonCloseHighlight = computed(
    () =>
      params.tool.value === 'polygon' &&
      !pendingDraft.value &&
      drawPointer.value !== null &&
      isPolygonCloseSnap(drawPointer.value, draftPoints.value, transform.value),
  )

  const draftPreview = computed<DraftPreview | null>(() => {
    if (!canDraw.value || pendingDraft.value || !drawPointer.value) {
      return null
    }
    const tool = params.tool.value
    const points = draftPoints.value
    const mouse = drawPointer.value
    const tf = transform.value
    const vertices = points.length / 2

    if (tool === 'rectangle' && vertices === 1) {
      const a = pixelToStage(points[0], points[1], tf)
      return {
        kind: 'rect',
        points: [a.x, a.y, mouse.x - a.x, mouse.y - a.y],
      }
    }
    if (tool === 'polygon' && vertices >= 1) {
      return {
        kind: 'polyline',
        points: [...pixelsToStageFlat(points, tf), mouse.x, mouse.y],
        closed: false,
      }
    }
    if (tool === 'line_direction') {
      if (vertices === 1) {
        const a = pixelToStage(points[0], points[1], tf)
        return { kind: 'line', points: [a.x, a.y, mouse.x, mouse.y] }
      }
      if (vertices === 2) {
        return { kind: 'line', points: pixelsToStageFlat(points.slice(0, 4), tf) }
      }
      if (vertices === 3) {
        const line = pixelsToStageFlat(points.slice(0, 4), tf)
        const c = pixelToStage(points[4], points[5], tf)
        return {
          kind: 'arrow',
          points: [...line, c.x, c.y, mouse.x, mouse.y],
        }
      }
    }
    if (tool === 'direction' && vertices === 1) {
      const a = pixelToStage(points[0], points[1], tf)
      return { kind: 'arrow', points: [a.x, a.y, mouse.x, mouse.y] }
    }
    return null
  })

  function resetDraft() {
    draftPoints.value = []
    if (!pointerInside.value) {
      drawPointer.value = null
    }
  }

  function clearPending() {
    pendingDraft.value = null
  }

  function pointerFromEvent(event: unknown) {
    const typed = event as {
      target?: {
        getStage?: () => {
          getPointerPosition?: () => { x: number; y: number } | null
        } | null
      }
    }
    return typed.target?.getStage?.()?.getPointerPosition?.() ?? null
  }

  function imagePointerFromEvent(event: unknown) {
    const pos = pointerFromEvent(event)
    if (!pos) {
      return null
    }
    return clampStagePointToImage(
      pos.x,
      pos.y,
      transform.value,
      params.imageWidth.value,
      params.imageHeight.value,
    )
  }

  function mouseEventFrom(event: unknown): MouseEvent | undefined {
    return (event as { evt?: MouseEvent }).evt
  }

  function syncDrawPointer(event: unknown) {
    if (!canDraw.value || pendingDraft.value) {
      drawPointer.value = null
      return
    }
    drawPointer.value = imagePointerFromEvent(event)
  }

  function onMouseMove(event: unknown) {
    pointerInside.value = true
    syncDrawPointer(event)
  }

  function onMouseLeave() {
    pointerInside.value = false
    if (!drafting.value) {
      drawPointer.value = null
    }
  }

  function completeDraft(
    shape: Annotation['shape'],
    clickPoints: number[],
    popoverPos: { x: number; y: number },
  ) {
    const draft: PendingDraft = {
      type: params.annotationType.value,
      shape,
      points: pointsFromClicks(shape, clickPoints),
      clickPoints,
      popoverX: popoverPos.x,
      popoverY: popoverPos.y,
    }
    pendingDraft.value = draft
    resetDraft()
    params.onDraftComplete(draft)
  }

  function appendVertex(x: number, y: number) {
    draftPoints.value = [...draftPoints.value, x, y]
  }

  function placeDraftPoint(event: unknown) {
    if (!canDraw.value || pendingDraft.value) {
      return
    }
    const pos = imagePointerFromEvent(event)
    if (!pos) {
      return
    }
    pointerInside.value = true
    drawPointer.value = pos
    const pixel = stageToPixel(
      pos.x,
      pos.y,
      transform.value,
      params.imageWidth.value,
      params.imageHeight.value,
    )
    const tool = params.tool.value
    const points = draftPoints.value
    const vertices = points.length / 2

    if (tool === 'rectangle') {
      if (vertices === 0) {
        appendVertex(pixel.x, pixel.y)
        return
      }
      const dx = Math.abs(pixel.x - points[0])
      const dy = Math.abs(pixel.y - points[1])
      if (dx < 2 && dy < 2) {
        return
      }
      completeDraft('rectangle', [points[0], points[1], pixel.x, pixel.y], pos)
      return
    }

    if (tool === 'polygon') {
      if (isPolygonCloseSnap(pos, points, transform.value)) {
        completeDraft('polygon', [...points], pos)
        return
      }
      appendVertex(pixel.x, pixel.y)
      return
    }

    if (tool === 'line_direction') {
      if (vertices < 3) {
        appendVertex(pixel.x, pixel.y)
        return
      }
      completeDraft(
        'line_direction',
        [
          points[0],
          points[1],
          points[2],
          points[3],
          points[4],
          points[5],
          pixel.x,
          pixel.y,
        ],
        pos,
      )
      return
    }

    if (tool === 'direction') {
      if (vertices === 0) {
        appendVertex(pixel.x, pixel.y)
        return
      }
      completeDraft('direction', [points[0], points[1], pixel.x, pixel.y], pos)
    }
  }

  function undoLastDraftPoint() {
    if (!canDraw.value || pendingDraft.value) {
      return
    }
    if (draftPoints.value.length < 2) {
      return
    }
    draftPoints.value = draftPoints.value.slice(0, -2)
    if (!drafting.value && !pointerInside.value) {
      drawPointer.value = null
    }
  }

  function onMouseDown(event: unknown) {
    const mouse = mouseEventFrom(event)
    if (!mouse) {
      return
    }
    if (mouse.button === BUTTON_RIGHT) {
      mouse.preventDefault()
      undoLastDraftPoint()
      return
    }
    if (mouse.button === BUTTON_LEFT) {
      placeDraftPoint(event)
    }
  }

  function onKeyDown(event: KeyboardEvent) {
    if (event.key === 'Escape') {
      resetDraft()
    }
  }

  onMounted(() => {
    window.addEventListener('keydown', onKeyDown)
  })

  onUnmounted(() => {
    window.removeEventListener('keydown', onKeyDown)
  })

  return {
    transform,
    crosshairVisible,
    crosshairLines,
    draftPreview,
    draftHandlePoints,
    polygonCloseHighlight,
    pendingDraft,
    resetDraft,
    clearPending,
    onMouseMove,
    onMouseLeave,
    onMouseDown,
  }
}
