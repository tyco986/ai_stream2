<template>
  <div ref="wrapRef" class="canvas" @contextmenu.prevent>
    <v-stage
      :config="{ width: stageWidth, height: stageHeight }"
      @mousemove="onMouseMove"
      @mouseleave="onMouseLeave"
      @mousedown="onMouseDown"
    >
      <v-layer>
        <v-image v-if="konvaImage" :config="{ image: konvaImage, ...imageRect }" />
      </v-layer>

      <v-layer>
        <v-group
          v-for="ann in annotations"
          :key="ann.id"
          :config="bodyGroupConfig(ann)"
          @mousedown="onAnnotationMouseDown(ann.id, $event)"
          @dragmove="onBodyDragMove(ann.id, $event)"
          @dragend="onBodyDragEnd(ann.id, $event)"
        >
          <v-line
            v-if="ann.shape === 'rectangle' || ann.shape === 'polygon'"
            :config="closedLineConfig(ann)"
            @mouseenter="onClosedShapeEnter(ann.id)"
            @mouseleave="onClosedShapeLeave(ann.id)"
          />
          <template v-else-if="ann.shape === 'line_direction'">
            <v-line :config="lineAbConfig(ann)" />
            <v-arrow :config="arrowCdConfig(ann)" />
          </template>
          <v-arrow
            v-else-if="ann.shape === 'direction'"
            :config="directionConfig(ann)"
          />
        </v-group>

        <template v-if="pendingDraft">
          <v-line
            v-if="pendingDraft.shape === 'rectangle' || pendingDraft.shape === 'polygon'"
            :config="closedLineConfig(pendingDraft)"
          />
          <template v-else-if="pendingDraft.shape === 'line_direction'">
            <v-line :config="lineAbConfig(pendingDraft)" />
            <v-arrow :config="arrowCdConfig(pendingDraft)" />
          </template>
          <v-arrow
            v-else-if="pendingDraft.shape === 'direction'"
            :config="directionConfig(pendingDraft)"
          />
        </template>
      </v-layer>

      <v-layer>
        <v-circle
          v-for="dot in annotationDots"
          :key="dot.key"
          :config="annotationDotConfig(dot)"
          @mouseenter="onAnchorEnter(dot)"
          @mouseleave="onAnchorLeave(dot.key)"
          @dragmove="onAnchorDrag(dot.index, $event)"
          @mousedown="onAnchorMouseDown(dot.annotationId, $event)"
        />
        <v-circle
          v-for="dot in pendingClickDots"
          :key="dot.key"
          :config="vertexDotConfig(dot.x, dot.y, pendingClickColor)"
        />
        <v-rect
          v-if="draftPreview?.kind === 'rect'"
          :config="draftRectConfig"
        />
        <v-line
          v-else-if="draftPreview?.kind === 'polyline' || draftPreview?.kind === 'line'"
          :config="draftLineConfig"
        />
        <template v-else-if="draftPreview?.kind === 'arrow'">
          <v-line
            v-if="draftPreview.points.length >= 8"
            :config="draftArrowLineConfig"
          />
          <v-arrow :config="draftArrowConfig" />
        </template>
        <v-circle
          v-for="(point, index) in draftHandlePoints"
          :key="`draft-handle-${index}`"
          :config="draftHandleConfig(point, index)"
        />
      </v-layer>

      <v-layer>
        <v-line
          v-for="(line, index) in visibleCrosshairLines"
          :key="index"
          :config="{
            points: line,
            stroke: '#FF0000',
            strokeWidth: 1,
            dash: [6, 4],
            listening: false,
          }"
        />
      </v-layer>
    </v-stage>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, onUnmounted, ref, watch } from 'vue'
import type { Annotation } from '@/api/pipelines'
import {
  clampGroupDragPos,
  getClickPoints,
} from '../../composables/annotationGeometry'
import type { PendingDraft } from '../../composables/useAnalyzerCanvas'
import {
  clampStagePointToImage,
  colorForAnnotationType,
  hexToRgba,
  pixelToStage,
  pixelsToStageFlat,
  type AnnotationModeType,
  type ViewTransform,
} from '../../composables/useAnalyzerGeometry'

type ShapeSource = {
  type: AnnotationModeType | Annotation['type']
  shape: Annotation['shape']
  points: number[]
  id?: string
}

type CanvasDot = {
  key: string
  annotationId: string
  index: number
  x: number
  y: number
  color: string
  editable: boolean
}

const props = defineProps<{
  imageUrl: string
  imageWidth: number
  imageHeight: number
  annotations: Annotation[]
  selectedId: string | null
  editMode: boolean
  transform: ViewTransform
  crosshairVisible: boolean
  crosshairLines: number[][]
  draftColor: string
  draftPreview: {
    kind: 'rect' | 'polyline' | 'line' | 'arrow'
    points: number[]
    closed?: boolean
  } | null
  draftHandlePoints: { x: number; y: number }[]
  polygonCloseHighlight: boolean
  pendingDraft: PendingDraft | null
}>()

const emit = defineEmits<{
  mousemove: [event: unknown]
  mouseleave: []
  mousedown: [event: unknown]
  select: [id: string | null]
  moveSelected: [dx: number, dy: number]
  moveAnchor: [pointIndex: number, stageX: number, stageY: number]
  resize: [width: number, height: number]
}>()

const wrapRef = ref<HTMLElement | null>(null)
const stageWidth = ref(640)
const stageHeight = ref(360)
const konvaImage = ref<HTMLImageElement | null>(null)
const bodyDragOffset = ref<{ id: string; x: number; y: number } | null>(null)
const hover = ref<
  | { kind: 'shape'; id: string }
  | { kind: 'anchor'; key: string }
  | null
>(null)
let resizeObserver: ResizeObserver | null = null

const ANCHOR_RADIUS = 4
const ANCHOR_HOVER_RADIUS = 7
const ANCHOR_HIT_STROKE = 16
const CLOSED_HOVER_FILL_ALPHA = 0.28
const CLOSED_HIT_FILL = 'rgba(0,0,0,0)'

const imageRect = computed(() => ({
  x: props.transform.offsetX,
  y: props.transform.offsetY,
  width: props.imageWidth * props.transform.scale,
  height: props.imageHeight * props.transform.scale,
  listening: false,
}))

const visibleCrosshairLines = computed(() =>
  props.crosshairVisible ? props.crosshairLines : [],
)

const pendingClickColor = computed(() =>
  props.pendingDraft
    ? colorForAnnotationType(props.pendingDraft.type)
    : props.draftColor,
)

const pendingClickDots = computed(() => {
  if (!props.pendingDraft) {
    return [] as { key: string; x: number; y: number }[]
  }
  const clicks = props.pendingDraft.clickPoints
  const dots: { key: string; x: number; y: number }[] = []
  for (let i = 0; i < clicks.length; i += 2) {
    const point = pixelToStage(clicks[i], clicks[i + 1], props.transform)
    dots.push({ key: `pending-${i}`, x: point.x, y: point.y })
  }
  return dots
})

const annotationDots = computed(() => {
  const offset = bodyDragOffset.value
  const dots: CanvasDot[] = []
  for (const ann of props.annotations) {
    const clicks = getClickPoints(ann)
    const dragX = offset?.id === ann.id ? offset.x : 0
    const dragY = offset?.id === ann.id ? offset.y : 0
    const editable = props.editMode && ann.id === props.selectedId
    const color = colorForAnnotationType(ann.type)
    for (let i = 0; i < clicks.length; i += 2) {
      const point = pixelToStage(clicks[i], clicks[i + 1], props.transform)
      dots.push({
        key: `${ann.id}-${i}`,
        annotationId: ann.id,
        index: i,
        x: point.x + dragX,
        y: point.y + dragY,
        color,
        editable,
      })
    }
  }
  return dots
})

const draftStroke = computed(() => ({
  stroke: props.draftColor,
  strokeWidth: 2,
  opacity: 0.85,
  listening: false,
}))

const draftRectConfig = computed(() => {
  const preview = props.draftPreview
  if (!preview || preview.kind !== 'rect') {
    return null
  }
  return {
    x: preview.points[0],
    y: preview.points[1],
    width: preview.points[2],
    height: preview.points[3],
    ...draftStroke.value,
  }
})

const draftLineConfig = computed(() => {
  const preview = props.draftPreview
  if (!preview || (preview.kind !== 'polyline' && preview.kind !== 'line')) {
    return null
  }
  return {
    points: preview.points,
    closed: preview.closed === true,
    ...draftStroke.value,
  }
})

const draftArrowLineConfig = computed(() => {
  const preview = props.draftPreview
  if (!preview || preview.kind !== 'arrow' || preview.points.length < 8) {
    return null
  }
  return {
    points: preview.points.slice(0, 4),
    ...draftStroke.value,
  }
})

const draftArrowConfig = computed(() => {
  const preview = props.draftPreview
  if (!preview || preview.kind !== 'arrow') {
    return null
  }
  return {
    points: preview.points.length >= 8 ? preview.points.slice(4) : preview.points,
    fill: props.draftColor,
    pointerLength: 10,
    pointerWidth: 8,
    ...draftStroke.value,
  }
})

watch(
  () => props.imageUrl,
  (url) => {
    if (!url) {
      konvaImage.value = null
      return
    }
    const img = new Image()
    img.onload = () => {
      konvaImage.value = img
    }
    img.onerror = () => {
      konvaImage.value = null
    }
    img.src = url
  },
  { immediate: true },
)

watch(
  () => props.editMode,
  (editMode) => {
    if (!editMode) {
      hover.value = null
    }
  },
)

onMounted(() => {
  if (!wrapRef.value) {
    return
  }
  resizeObserver = new ResizeObserver((entries) => {
    const entry = entries[0]
    if (!entry) {
      return
    }
    const nextWidth = Math.max(1, Math.floor(entry.contentRect.width))
    const nextHeight = Math.max(1, Math.floor(entry.contentRect.height))
    if (nextWidth === stageWidth.value && nextHeight === stageHeight.value) {
      return
    }
    stageWidth.value = nextWidth
    stageHeight.value = nextHeight
    emit('resize', nextWidth, nextHeight)
  })
  resizeObserver.observe(wrapRef.value)
})

onUnmounted(() => {
  resizeObserver?.disconnect()
})

function bodyGroupConfig(ann: Annotation) {
  const selected = ann.id === props.selectedId
  return {
    /** Draw mode: group ignores hits so clicks reach Stage for new drafts. */
    listening: props.editMode,
    draggable: props.editMode && selected,
    id: ann.id,
    dragBoundFunc: (pos: { x: number; y: number }) =>
      clampGroupDragPos(
        ann.points,
        pos,
        props.transform,
        props.imageWidth,
        props.imageHeight,
      ),
  }
}

function vertexDotVisual(
  x: number,
  y: number,
  color: string,
  highlighted: boolean,
  interactive: boolean,
) {
  return {
    x,
    y,
    radius: highlighted ? ANCHOR_HOVER_RADIUS : ANCHOR_RADIUS,
    fill: color,
    stroke: highlighted ? '#FFFFFF' : 'transparent',
    strokeWidth: highlighted ? 2 : 1,
    shadowColor: highlighted ? '#FFFFFF' : undefined,
    shadowBlur: highlighted ? 8 : 0,
    shadowOpacity: highlighted ? 0.9 : 0,
    listening: interactive,
    draggable: interactive,
    hitStrokeWidth: interactive ? ANCHOR_HIT_STROKE : 0,
    dragBoundFunc: interactive
      ? (pos: { x: number; y: number }) =>
          clampStagePointToImage(
            pos.x,
            pos.y,
            props.transform,
            props.imageWidth,
            props.imageHeight,
          )
      : undefined,
  }
}

function annotationDotConfig(dot: CanvasDot) {
  const highlighted =
    dot.editable && hover.value?.kind === 'anchor' && hover.value.key === dot.key
  return vertexDotVisual(dot.x, dot.y, dot.color, highlighted, dot.editable)
}

function vertexDotConfig(x: number, y: number, color: string) {
  return vertexDotVisual(x, y, color, false, false)
}

function draftHandleConfig(point: { x: number; y: number }, index: number) {
  return vertexDotVisual(
    point.x,
    point.y,
    props.draftColor,
    index === 0 && props.polygonCloseHighlight,
    false,
  )
}

function strokeOf(source: ShapeSource) {
  return colorForAnnotationType(source.type as AnnotationModeType)
}

function widthOf(source: ShapeSource) {
  return source.id && source.id === props.selectedId ? 3 : 2
}

function closedLineFill(source: ShapeSource) {
  if (!props.editMode || !source.id) {
    return undefined
  }
  if (hover.value?.kind === 'shape' && hover.value.id === source.id) {
    return hexToRgba(strokeOf(source), CLOSED_HOVER_FILL_ALPHA)
  }
  return CLOSED_HIT_FILL
}

function closedLineConfig(source: ShapeSource) {
  const color = strokeOf(source)
  return {
    points: pixelsToStageFlat(source.points, props.transform),
    closed: true,
    stroke: color,
    strokeWidth: widthOf(source),
    fill: closedLineFill(source),
    /** Pending draft has no id; annotation hits are gated by parent group.listening. */
    listening: Boolean(source.id),
  }
}

function lineAbConfig(source: ShapeSource) {
  return {
    points: pixelsToStageFlat(source.points.slice(4, 8), props.transform),
    stroke: strokeOf(source),
    strokeWidth: widthOf(source),
    listening: Boolean(source.id),
  }
}

function arrowCdConfig(source: ShapeSource) {
  const color = strokeOf(source)
  return {
    points: pixelsToStageFlat(source.points.slice(0, 4), props.transform),
    stroke: color,
    fill: color,
    strokeWidth: widthOf(source),
    pointerLength: 10,
    pointerWidth: 8,
    listening: Boolean(source.id),
  }
}

function directionConfig(source: ShapeSource) {
  const color = strokeOf(source)
  return {
    points: pixelsToStageFlat(source.points, props.transform),
    stroke: color,
    fill: color,
    strokeWidth: widthOf(source),
    pointerLength: 10,
    pointerWidth: 8,
    listening: Boolean(source.id),
  }
}

function onMouseMove(event: unknown) {
  emit('mousemove', event)
}

function onMouseLeave() {
  hover.value = null
  emit('mouseleave')
}

function onMouseDown(event: unknown) {
  const typed = event as { target?: { getClassName?: () => string }; evt?: MouseEvent }
  if (props.editMode) {
    if (typed.evt?.button === 0) {
      const className = typed.target?.getClassName?.() ?? ''
      if (className === 'Stage' || className === 'Image' || className === 'Layer') {
        emit('select', null)
      }
    }
    return
  }
  emit('mousedown', event)
}

function onClosedShapeEnter(id: string) {
  hover.value = { kind: 'shape', id }
}

function onClosedShapeLeave(id: string) {
  if (hover.value?.kind === 'shape' && hover.value.id === id) {
    hover.value = null
  }
}

function onAnnotationMouseDown(id: string, event: { evt?: MouseEvent; cancelBubble?: boolean }) {
  if (event.evt && event.evt.button !== 0) {
    return
  }
  event.cancelBubble = true
  emit('select', id)
}

function onBodyDragMove(
  id: string,
  event: { target: { x: () => number; y: () => number } },
) {
  bodyDragOffset.value = {
    id,
    x: event.target.x(),
    y: event.target.y(),
  }
}

function onBodyDragEnd(
  id: string,
  event: {
    target: {
      x: () => number
      y: () => number
      position: (pos: { x: number; y: number }) => void
    }
  },
) {
  bodyDragOffset.value = null
  if (id !== props.selectedId) {
    return
  }
  const node = event.target
  const dx = Math.round(node.x() / props.transform.scale)
  const dy = Math.round(node.y() / props.transform.scale)
  node.position({ x: 0, y: 0 })
  if (dx === 0 && dy === 0) {
    return
  }
  emit('moveSelected', dx, dy)
}

function onAnchorEnter(dot: CanvasDot) {
  if (!dot.editable) {
    return
  }
  hover.value = { kind: 'anchor', key: dot.key }
}

function onAnchorLeave(key: string) {
  if (hover.value?.kind === 'anchor' && hover.value.key === key) {
    hover.value = null
  }
}

function onAnchorMouseDown(
  id: string,
  event: { evt?: MouseEvent; cancelBubble?: boolean },
) {
  if (event.evt && event.evt.button !== 0) {
    return
  }
  event.cancelBubble = true
  emit('select', id)
}

function onAnchorDrag(
  pointIndex: number,
  event: { target: { x: () => number; y: () => number } },
) {
  emit('moveAnchor', pointIndex, event.target.x(), event.target.y())
}
</script>

<style scoped>
.canvas {
  position: absolute;
  inset: 0;
  background: #111827;
  border: 1px solid #dcdfe6;
  overflow: hidden;
}
</style>
