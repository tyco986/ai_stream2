import type { Ref } from 'vue'
import type { Annotation } from '@/api/pipelines'
import {
  applyClicksToAnnotation,
  clampTranslateDelta,
  getClickPoints,
  moveClickVertex,
  translateClicks,
} from './annotationGeometry'
import { stageToPixel, type ViewTransform } from './useAnalyzerGeometry'

export function useAnnotationEdit(params: {
  annotations: Ref<Annotation[]>
  selectedId: Ref<string | null>
  transform: Ref<ViewTransform>
  imageWidth: Ref<number>
  imageHeight: Ref<number>
}) {
  function selectedAnnotation(): Annotation | null {
    const id = params.selectedId.value
    let item: Annotation | null = null
    if (id) {
      item = params.annotations.value.find((ann) => ann.id === id) ?? null
    }
    return item
  }

  function selectAnnotation(id: string | null) {
    params.selectedId.value = id
  }

  function applyDeltaToSelected(dx: number, dy: number) {
    const item = selectedAnnotation()
    let nextDx = dx
    let nextDy = dy
    if (!item || (nextDx === 0 && nextDy === 0)) {
      return
    }
    const clamped = clampTranslateDelta(
      item.points,
      nextDx,
      nextDy,
      params.imageWidth.value,
      params.imageHeight.value,
    )
    nextDx = clamped.dx
    nextDy = clamped.dy
    if (nextDx === 0 && nextDy === 0) {
      return
    }
    applyClicksToAnnotation(item, translateClicks(getClickPoints(item), nextDx, nextDy))
  }

  function moveAnchor(clickIndex: number, stageX: number, stageY: number) {
    const item = selectedAnnotation()
    if (!item) {
      return
    }
    const pixel = stageToPixel(
      stageX,
      stageY,
      params.transform.value,
      params.imageWidth.value,
      params.imageHeight.value,
    )
    applyClicksToAnnotation(
      item,
      moveClickVertex(getClickPoints(item), clickIndex, pixel.x, pixel.y),
    )
  }

  return {
    selectAnnotation,
    applyDeltaToSelected,
    moveAnchor,
  }
}
