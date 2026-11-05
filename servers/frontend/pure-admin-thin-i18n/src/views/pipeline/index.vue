<script setup lang="ts">
import { computed, nextTick, onMounted, onUnmounted, ref, toRaw, watch } from "vue";
import type { KonvaEventObject } from "konva/lib/Node";
import {
  Circle as KonvaCircle,
  Image as KonvaImage,
  Label as KonvaLabel,
  Layer,
  Line as KonvaLine,
  Rect as KonvaRect,
  Shape,
  Stage,
  Tag as KonvaTag,
  Text as KonvaText
} from "vue-konva";
import Play from "~icons/ri/play-circle-line";
import ListCheck from "~icons/ri/list-check";
import Edit from "~icons/ri/edit-line";
import DeleteBin from "~icons/ri/delete-bin-line";
import DragMove from "~icons/ri/drag-move-2-line";
import Focus from "~icons/ri/focus-3-line";
import Check from "~icons/ri/check-line";
import Close from "~icons/ri/close-line";
import Draggable from "~icons/ri/draggable";
import { getUploadFiles, getMediaContent, uploadMedia, type MediaItem } from "@/api/media";
import { getModel, getModelMap } from "@/api/model";
import {
  createGie,
  getGie,
  getGies,
  removeGie,
  removeGies,
  updateGie,
  type GieClassAttrs,
  type GieDraft,
  type GieItem
} from "@/api/pipelineGie";
import {
  createAnalyzer,
  getAnalyzers,
  removeAnalyzer,
  removeAnalyzers,
  updateAnalyzer,
  type AnalyzerItem
} from "@/api/pipelineAnalyzer";
import {
  ANALYZER_ANCHOR_FILL,
  ANALYZER_ANCHOR_RADIUS,
  ANALYZER_ANGLE_GAP,
  ANALYZER_ANGLE_LABEL_WIDTH,
  ANALYZER_ANGLE_RADIUS,
  ANALYZER_ANNOTATION_DIALOG_WIDTH,
  ANALYZER_CANVAS_HEIGHT,
  ANALYZER_CANVAS_WIDTH,
  ANALYZER_DIALOG_WIDTH,
  ANALYZER_FORM_DIALOG_WIDTH,
  ANALYZER_IMAGE_DIALOG_WIDTH,
  ANALYZER_CLASS_ID,
  ANALYZER_CROSSHAIR_DASH,
  ANALYZER_CROSSHAIR_STROKE,
  ANALYZER_DIRECTION_MODE,
  ANALYZER_DIRECTION_RULE,
  ANALYZER_DIRECTION_STROKE,
  ANALYZER_EDGE_STROKE_WIDTH,
  ANALYZER_DISPLAY_FONT_SIZE,
  ANALYZER_ENABLE,
  ANALYZER_OSD_MODE,
  ANALYZER_LINE_MODE,
  ANALYZER_LINE_RULE,
  ANALYZER_LINE_STROKE,
  ANALYZER_LINE_WIDTH,
  ANALYZER_PAN_THRESHOLD,
  ANALYZER_RECT_BORDER,
  ANALYZER_RECT_COLORS,
  ANALYZER_RECT_FONT,
  ANALYZER_RECT_FONT_SIZE,
  ANALYZER_RECT_LABEL_BG,
  ANALYZER_RECT_RULES,
  ANALYZER_OBJECT_THRESHOLD,
  ANALYZER_SHAPE_COLORS,
  ANALYZER_SHAPE_OPS_WIDTH,
  ANALYZER_SHAPE_TYPE_WIDTH,
  ANALYZER_SNAP_THRESHOLD,
  ANALYZER_TIME_THRESHOLD,
  ANALYZER_TIP_EXTEND_HEIGHT,
  ANALYZER_TIP_HEIGHT,
  ANALYZER_TIP_OFFSET,
  ANALYZER_TIP_WIDTH,
  ANALYZER_IMAGE_ACCEPT,
  ANALYZER_IMAGE_PAGE_SIZE,
  ANALYZER_IMAGE_SUFFIXES,
  ANALYZER_LIST_COLUMN_PERCENTS,
  ANALYZER_RULES,
  GIE_ATTR_COLUMN_PERCENTS,
  GIE_ATTR_DIALOG_WIDTH,
  GIE_ATTR_UNSUPPORTED_TYPES,
  GIE_DIALOG_WIDTH,
  GIE_FORM_DIALOG_WIDTH,
  GIE_LIST_COLUMN_PERCENTS,
  PIPELINE_LIST_COLUMN_PERCENTS
} from "@/constants/pipeline";
import { $t, transformI18n } from "@/plugins/i18n";
import { useUserStoreHook } from "@/store/modules/user";
import { columnPixels } from "@/utils/columnWidth";
import { message } from "@/utils/message";
import { formatLocalTime } from "@/utils/time";
import {
  apart,
  arrowWings,
  clipLine,
  lineAngle,
  moveDirPoint,
  moveLinePoint,
  projectSegment,
  segmentDistance,
  shiftDirection,
  shiftLine,
  strokeAngleArc,
  type AnalyzerDirection,
  type AnalyzerLine
} from "@/views/pipeline/analyzerLine";
import { imageRect, moveRectBetween, oppositeRectCorner, shiftRect, stageRect, tipOrigin, type AnalyzerRect } from "@/views/pipeline/analyzerRect";
import { imagePoint, snapAnalyzerPointer, snapEdgeLines, stagePoint, type AnalyzerPoint } from "@/views/pipeline/analyzerSnap";
import DetVisRtsp from "@/views/pipeline/create/DetVisRtsp.vue";

const annotationNumbers = (value: unknown) => String(value).split(";").map(Number);

const annotationGroup = (annotations: Record<string, unknown>, key: string) =>
  (annotations[key] ?? {}) as Record<string, unknown>;

const annotationFields = (group: Record<string, unknown>, prefix: string) =>
  Object.entries(group).filter(([key, value]) => key.startsWith(prefix) && typeof value === "string");

const restoredRect = (
  name: string,
  rule: string,
  value: string,
  inverseRoi: boolean,
  objectThreshold: string,
  timeThreshold: string
): AnalyzerRect => {
  const points = annotationNumbers(value);
  return {
    name,
    rule,
    inverseRoi,
    objectThreshold,
    timeThreshold,
    x: points[0],
    y: points[1],
    width: points[2] - points[0],
    height: points[5] - points[1]
  };
};

const restoredLine = (name: string, value: string, extended: boolean): AnalyzerLine => {
  const points = annotationNumbers(value);
  return {
    name,
    ax: points[0],
    ay: points[1],
    hx: points[2],
    hy: points[3],
    x1: points[4],
    y1: points[5],
    x2: points[6],
    y2: points[7],
    extended
  };
};

const restoredDirection = (name: string, value: string): AnalyzerDirection => {
  const points = annotationNumbers(value);
  return { name, x1: points[0], y1: points[1], x2: points[2], y2: points[3] };
};

type AnalyzerShapeKind = "rect" | "line" | "direction";

type AnalyzerShapeRow = {
  name: string;
  color: string;
  kind: AnalyzerShapeKind;
  mark: AnalyzerRect | AnalyzerLine | AnalyzerDirection;
};

type AttrRow = {
  classKey: string;
  conf: number | null;
  maxDet: number | null;
  minW: number | null;
  minH: number | null;
  maxW: number | null;
  maxH: number | null;
};

const emptyAttr = (classKey: string): AttrRow => ({
  classKey,
  conf: null,
  maxDet: null,
  minW: null,
  minH: null,
  maxW: null,
  maxH: null
});

const attrRows = (classAttrs: GieClassAttrs | null): AttrRow[] =>
  Object.entries(classAttrs ?? {}).map(([classKey, fields]) => ({
    classKey,
    conf: fields.conf ?? null,
    maxDet: fields.topk ?? null,
    minW: fields.detected_min_w ?? null,
    minH: fields.detected_min_h ?? null,
    maxW: fields.detected_max_w ?? null,
    maxH: fields.detected_max_h ?? null
  }));

const attrFields = (row: AttrRow) => ({
  conf: row.conf,
  topk: row.maxDet,
  detected_min_w: row.minW,
  detected_min_h: row.minH,
  detected_max_w: row.maxW,
  detected_max_h: row.maxH
});

defineOptions({
  name: "PipelineIndex"
});

const listRef = ref();
const listWidth = ref(0);
const listObserver = new ResizeObserver(entries => {
  listWidth.value = Math.floor(entries[0].contentRect.width);
});
const listWidths = computed(() => columnPixels(listWidth.value, PIPELINE_LIST_COLUMN_PERCENTS));
const items = ref<Record<string, string>[]>([]);
const pipelineCreateVisible = ref(false);
const gieVisible = ref(false);
const gieRef = ref();
const gieWidth = ref(0);
const gieObserver = new ResizeObserver(entries => {
  gieWidth.value = Math.floor(entries[0].contentRect.width);
});
const gieWidths = computed(() => columnPixels(gieWidth.value, GIE_LIST_COLUMN_PERCENTS));
const gieItems = ref<GieItem[]>([]);
const gieSelected = ref<GieItem[]>([]);
const selected = ref<Record<string, string>[]>([]);
const gieAttrsOpen = ref(false);
const gieAttrRows = ref<AttrRow[]>([]);
const gieId = ref("");
const gieCreateVisible = ref(false);
const gieName = ref("");
const gieInterval = ref(0);
const gieModel = ref("");
const gieModelType = ref("");
const gieModels = ref<{ id: string; name: string }[]>([]);
const gieClasses = ref<string[]>([]);
const modelConf = ref<number | null>(null);
const modelMaxDet = ref<number | null>(null);
const gieAttrs = ref<AttrRow[]>([emptyAttr("all")]);
const attrRef = ref();
const attrWidth = ref(0);
const attrObserver = new ResizeObserver(entries => {
  attrWidth.value = Math.floor(entries[0].contentRect.width);
});
const attrWidths = computed(() => columnPixels(attrWidth.value, GIE_ATTR_COLUMN_PERCENTS));
const gieConfirmable = computed(() => Boolean(gieName.value.trim() && gieModel.value));
const gieAttrHint = computed(() => {
  const unsupported = GIE_ATTR_UNSUPPORTED_TYPES.includes(gieModelType.value);
  const selectedOnly = !gieAttrs.value.some(row => row.classKey === "all");
  const messages = {
    unsupported: transformI18n($t("pipeline.pureAttrUnsupported")).replace("{type}", gieModelType.value),
    selected: transformI18n($t("pipeline.pureAttrSelectedOnly")),
    none: ""
  };
  const key = unsupported ? "unsupported" : selectedOnly ? "selected" : "none";
  return messages[key];
});
const analyzerVisible = ref(false);
const analyzerRef = ref();
const analyzerWidth = ref(0);
const analyzerObserver = new ResizeObserver(entries => {
  analyzerWidth.value = Math.floor(entries[0].contentRect.width);
});
const analyzerWidths = computed(() => columnPixels(analyzerWidth.value, ANALYZER_LIST_COLUMN_PERCENTS));
const analyzerAnnotationsOpen = ref(false);
const analyzerAnnotationText = ref("");
const analyzerItems = ref<AnalyzerItem[]>([]);
const analyzerSelected = ref<AnalyzerItem[]>([]);
const analyzerCreateVisible = ref(false);
const analyzerId = ref("");
const analyzerName = ref("");
const analyzerImage = ref("");
const analyzerImageName = ref("");
const analyzerImageWidth = ref(0);
const analyzerImageHeight = ref(0);
const analyzerMediaId = ref("");
const analyzerRule = ref("");
const analyzerFileRef = ref<HTMLInputElement>();
const analyzerImagesVisible = ref(false);
const analyzerImages = ref<MediaItem[]>([]);
const analyzerImagePage = ref(1);
const analyzerImageTotal = ref(0);
const analyzerPicture = ref<HTMLImageElement | null>(null);
const analyzerImageUrl = ref("");
const analyzerImageBox = ref({ x: 0, y: 0, width: 0, height: 0 });
const analyzerPointer = ref<AnalyzerPoint | null>(null);
const analyzerPoints = ref<AnalyzerPoint[]>([]);
const analyzerShapes = ref<AnalyzerShapeRow[]>([]);
const analyzerEditing = ref<AnalyzerShapeRow | null>(null);
const analyzerHandleDrag = ref<{ key: string; left: number; top: number; fixed: AnalyzerPoint | null } | null>(null);
const analyzerEditBackup = ref<AnalyzerShapeRow["mark"] | null>(null);
const analyzerRects = ref<AnalyzerRect[]>([]);
const analyzerLines = ref<AnalyzerLine[]>([]);
const analyzerLineStart = ref<AnalyzerPoint | null>(null);
const analyzerLineEnd = ref<AnalyzerPoint | null>(null);
const analyzerLineAnchor = ref<AnalyzerPoint | null>(null);
const analyzerLineHead = ref<AnalyzerPoint | null>(null);
const analyzerLineExtended = ref(false);
const analyzerDirs = ref<AnalyzerDirection[]>([]);
const analyzerDirTail = ref<AnalyzerPoint | null>(null);
const analyzerDirHead = ref<AnalyzerPoint | null>(null);
const analyzerAnchor = ref<AnalyzerPoint | null>(null);
const analyzerCorner = ref<AnalyzerPoint | null>(null);
const analyzerTip = ref<AnalyzerPoint | null>(null);
const analyzerRectName = ref("");
const analyzerNameRef = ref<HTMLInputElement | null>(null);
const analyzerInverseRoi = ref(false);
const analyzerObjectThreshold = ref(ANALYZER_OBJECT_THRESHOLD);
const analyzerTimeThreshold = ref(ANALYZER_TIME_THRESHOLD);
const analyzerStage = {
  width: ANALYZER_CANVAS_WIDTH,
  height: ANALYZER_CANVAS_HEIGHT
};
const analyzerStageStyle = {
  display: "block",
  width: `${ANALYZER_CANVAS_WIDTH}px`,
  height: `${ANALYZER_CANVAS_HEIGHT}px`,
  background: "var(--el-fill-color-light)"
};
const analyzerConfirmable = computed(() => Boolean(analyzerName.value.trim() && analyzerImage.value));
const analyzerImageSizeLabel = computed(() =>
  analyzerImageWidth.value && analyzerImageHeight.value ? `${analyzerImageWidth.value}×${analyzerImageHeight.value}` : ""
);
const analyzerImageConfig = computed(() => ({
  image: analyzerPicture.value,
  ...analyzerImageBox.value
}));
const analyzerSnap = computed(() => {
  const image = analyzerPicture.value;
  return snapAnalyzerPointer(
    analyzerPointer.value,
    analyzerImageBox.value,
    { x: image?.naturalWidth ?? 0, y: image?.naturalHeight ?? 0 },
    analyzerPoints.value,
    ANALYZER_SNAP_THRESHOLD
  );
});
const analyzerCrosshair = computed(() => {
  const snap = analyzerSnap.value;
  const image = analyzerPicture.value;
  const box = analyzerImageBox.value;
  const point =
    snap && image
      ? stagePoint(snap.point, box, { x: image.naturalWidth, y: image.naturalHeight })
      : null;
  const stroke = {
    stroke: ANALYZER_CROSSHAIR_STROKE,
    strokeWidth: 1,
    dash: ANALYZER_CROSSHAIR_DASH,
    listening: false
  };
  const gap = analyzerPanHover.value || analyzerHandleHover.value ? 12 : 0;
  const lines = point
    ? [
        { key: "h1", points: [box.x, point.y, point.x - gap, point.y] },
        { key: "h2", points: [point.x + gap, point.y, box.x + box.width, point.y] },
        { key: "v1", points: [point.x, box.y, point.x, point.y - gap] },
        { key: "v2", points: [point.x, point.y + gap, point.x, box.y + box.height] }
      ]
    : [];
  return lines.map(line => ({ ...stroke, ...line }));
});
const analyzerEdgeLines = computed(() =>
  snapEdgeLines(analyzerSnap.value?.edges ?? [], analyzerImageBox.value, ANALYZER_EDGE_STROKE_WIDTH).map(line => ({
    ...line,
    stroke: ANALYZER_CROSSHAIR_STROKE,
    strokeWidth: ANALYZER_EDGE_STROKE_WIDTH,
    listening: false
  }))
);
const analyzerRectMode = computed(() => ANALYZER_RECT_RULES.includes(analyzerRule.value));
const analyzerImageSize = computed(() => ({
  x: analyzerPicture.value?.naturalWidth ?? 0,
  y: analyzerPicture.value?.naturalHeight ?? 0
}));
const analyzerLiveRect = computed(() => {
  const anchor = analyzerAnchor.value;
  const end = analyzerCorner.value ?? analyzerSnap.value?.point ?? null;
  const rect = analyzerRectMode.value && anchor && end ? imageRect(anchor, end) : null;
  return rect && rect.width && rect.height ? rect : null;
});
const analyzerPixel = computed(() => {
  const width = analyzerImageSize.value.x;
  return width ? analyzerImageBox.value.width / width : 1;
});
const rectConfig = (rect: AnalyzerRect, dashed: boolean) => ({
  ...stageRect(rect, analyzerImageBox.value, analyzerImageSize.value),
  stroke: ANALYZER_RECT_COLORS[rect.rule],
  strokeWidth: ANALYZER_RECT_BORDER * analyzerPixel.value,
  dash: dashed ? ANALYZER_CROSSHAIR_DASH : undefined,
  listening: false
});
const analyzerRectConfigs = computed(() =>
  analyzerRects.value.map(rect => rectConfig(rect, analyzerEditing.value?.mark === rect))
);
const analyzerLabelTag = {
  fill: ANALYZER_RECT_LABEL_BG,
  pointerDirection: "none"
};
const analyzerRectLabels = computed(() =>
  analyzerRectConfigs.value.map((rect, index) => ({
    place: { x: rect.x + rect.strokeWidth / 2, y: rect.y + rect.strokeWidth / 2, listening: false },
    text: {
      text: analyzerRects.value[index]?.name ?? "",
      fontFamily: ANALYZER_RECT_FONT,
      fontSize: ANALYZER_RECT_FONT_SIZE,
      fill: ANALYZER_RECT_COLORS[analyzerRects.value[index]?.rule ?? ""]
    }
  }))
);
const analyzerLiveConfig = computed(() => {
  const rect = analyzerLiveRect.value;
  return rect ? rectConfig({ ...rect, rule: analyzerRule.value }, true) : null;
});
const analyzerLineStroke = {
  stroke: ANALYZER_LINE_STROKE,
  strokeWidth: ANALYZER_LINE_WIDTH,
  lineCap: "round",
  lineJoin: "round",
  listening: false
};
const placePoint = (point: AnalyzerPoint) => stagePoint(point, analyzerImageBox.value, analyzerImageSize.value);
const stageSegment = (start: AnalyzerPoint, end: AnalyzerPoint, dashed: boolean) => ({
  points: [start.x, start.y, end.x, end.y],
  ...analyzerLineStroke,
  dash: dashed ? ANALYZER_CROSSHAIR_DASH : undefined
});
const analyzerHover = computed(() => (analyzerLineStart.value ? (analyzerSnap.value?.point ?? null) : null));
const lineEnds = (start: AnalyzerPoint, end: AnalyzerPoint, extended: boolean): [AnalyzerPoint, AnalyzerPoint] =>
  extended ? clipLine(start, end, analyzerImageSize.value) : [start, end];
const analyzerSegments = computed(() => {
  const hover = analyzerLineEnd.value ? null : analyzerHover.value;
  const liveStart = analyzerLineStart.value;
  const liveEnd = analyzerLineEnd.value ?? hover;
  const livePair = liveStart && liveEnd && apart(liveStart, liveEnd) ? lineEnds(liveStart, liveEnd, analyzerLineExtended.value) : null;
  const live = livePair ? [stageSegment(placePoint(livePair[0]), placePoint(livePair[1]), true)] : [];
  return [
    ...analyzerLines.value.map(line => {
      const ends = lineEnds({ x: line.x1, y: line.y1 }, { x: line.x2, y: line.y2 }, line.extended);
      return stageSegment(placePoint(ends[0]), placePoint(ends[1]), analyzerEditing.value?.mark === line);
    }),
    ...live
  ];
});
const arrowSegment = (from: AnalyzerPoint, to: AnalyzerPoint, stroke: string, dashed: boolean) => ({
  points: [from.x, from.y, to.x, to.y],
  stroke,
  strokeWidth: ANALYZER_LINE_WIDTH,
  lineCap: "round",
  lineJoin: "round",
  dash: dashed ? ANALYZER_CROSSHAIR_DASH : undefined,
  listening: false
});
const arrowParts = (tail: AnalyzerPoint, head: AnalyzerPoint, stroke: string, dashed: boolean) => {
  const stageTail = placePoint(tail);
  const stageHead = placePoint(head);
  const wings = arrowWings(stageTail, stageHead);
  return [
    arrowSegment(stageTail, stageHead, stroke, dashed),
    arrowSegment(wings[0], stageHead, stroke, dashed),
    arrowSegment(wings[1], stageHead, stroke, dashed)
  ];
};
const analyzerArrows = computed(() => {
  const saved = analyzerLines.value.flatMap(line =>
    arrowParts({ x: line.ax, y: line.ay }, { x: line.hx, y: line.hy }, ANALYZER_LINE_STROKE, analyzerEditing.value?.mark === line)
  );
  const anchor = analyzerLineAnchor.value;
  const head = analyzerLineHead.value ?? analyzerHover.value;
  const live = anchor && head && apart(anchor, head) ? arrowParts(anchor, head, ANALYZER_LINE_STROKE, true) : [];
  return [...saved, ...live];
});
const analyzerAnchorDot = computed(() => {
  const start = analyzerLineStart.value;
  const end = analyzerLineEnd.value;
  const hover = analyzerHover.value;
  const locked = analyzerLineAnchor.value;
  const point = locked ?? (start && end && hover ? projectSegment(start, end, hover) : null);
  const stage = point ? placePoint(point) : null;
  return stage
    ? {
        x: stage.x,
        y: stage.y,
        radius: ANALYZER_ANCHOR_RADIUS,
        fill: ANALYZER_ANCHOR_FILL,
        stroke: ANALYZER_LINE_STROKE,
        strokeWidth: ANALYZER_LINE_WIDTH,
        listening: false
      }
    : null;
});
const analyzerAngle = computed(() => {
  const start = analyzerLineStart.value;
  const end = analyzerLineEnd.value;
  const anchor = analyzerLineAnchor.value;
  const head = anchor && !analyzerLineHead.value ? analyzerHover.value : null;
  const angle =
    start && end && anchor && head && apart(anchor, head)
      ? lineAngle(placePoint(start), placePoint(end), placePoint(anchor), placePoint(head))
      : null;
  const origin = anchor ? placePoint(anchor) : null;
  const mid = angle ? ((angle.start + angle.sweep / 2) * Math.PI) / 180 : 0;
  const distance = ANALYZER_ANGLE_RADIUS + ANALYZER_ANGLE_GAP + ANALYZER_RECT_FONT_SIZE;
  return angle && origin && angle.sweep
    ? {
        arc: {
          x: origin.x,
          y: origin.y,
          radius: ANALYZER_ANGLE_RADIUS,
          start: angle.start,
          sweep: angle.sweep,
          stroke: ANALYZER_LINE_STROKE,
          strokeWidth: ANALYZER_LINE_WIDTH,
          dash: ANALYZER_CROSSHAIR_DASH,
          sceneFunc: strokeAngleArc,
          listening: false
        },
        text: {
          x: origin.x + Math.cos(mid) * distance,
          y: origin.y + Math.sin(mid) * distance,
          width: ANALYZER_ANGLE_LABEL_WIDTH,
          offsetX: ANALYZER_ANGLE_LABEL_WIDTH / 2,
          offsetY: ANALYZER_RECT_FONT_SIZE / 2,
          align: "center",
          text: `${Math.round(angle.sweep)}°`,
          fontFamily: ANALYZER_RECT_FONT,
          fontSize: ANALYZER_RECT_FONT_SIZE,
          fill: ANALYZER_LINE_STROKE,
          listening: false
        }
      }
    : null;
});
const analyzerDirArrows = computed(() => {
  const saved = analyzerDirs.value.flatMap(dir =>
    arrowParts({ x: dir.x1, y: dir.y1 }, { x: dir.x2, y: dir.y2 }, ANALYZER_DIRECTION_STROKE, analyzerEditing.value?.mark === dir)
  );
  const tail = analyzerDirTail.value;
  const locked = analyzerDirHead.value;
  const hover = tail && !locked ? (analyzerSnap.value?.point ?? null) : null;
  const head = locked ?? hover;
  const live = tail && head && apart(tail, head) ? arrowParts(tail, head, ANALYZER_DIRECTION_STROKE, true) : [];
  return [...saved, ...live];
});
const analyzerDirLabels = computed(() =>
  analyzerDirs.value.map(dir => {
    const start = placePoint({ x: dir.x1, y: dir.y1 });
    return {
      place: { x: start.x, y: start.y, listening: false },
      text: {
        text: dir.name,
        fontFamily: ANALYZER_RECT_FONT,
        fontSize: ANALYZER_RECT_FONT_SIZE,
        fill: ANALYZER_DIRECTION_STROKE
      }
    };
  })
);
const analyzerLineLabels = computed(() =>
  analyzerLines.value.map(line => {
    const end = placePoint({ x: line.x2, y: line.y2 });
    return {
      place: { x: end.x, y: end.y, listening: false },
      text: {
        text: line.name,
        fontFamily: ANALYZER_RECT_FONT,
        fontSize: ANALYZER_RECT_FONT_SIZE,
        fill: ANALYZER_LINE_STROKE
      }
    };
  })
);
const analyzerTipStyle = computed(() => ({
  left: `${analyzerTip.value?.x ?? 0}px`,
  top: `${analyzerTip.value?.y ?? 0}px`
}));
const analyzerEditHandles = computed(() => {
  const row = analyzerEditing.value;
  const rect = row?.kind === "rect" ? (row.mark as AnalyzerRect) : null;
  const line = row?.kind === "line" ? (row.mark as AnalyzerLine) : null;
  const direction = row?.kind === "direction" ? (row.mark as AnalyzerDirection) : null;
  const rectPoints = rect
    ? [
        { key: "nw", point: { x: rect.x, y: rect.y } },
        { key: "ne", point: { x: rect.x + rect.width, y: rect.y } },
        { key: "se", point: { x: rect.x + rect.width, y: rect.y + rect.height } },
        { key: "sw", point: { x: rect.x, y: rect.y + rect.height } }
      ]
    : [];
  const linePoints = line
    ? [
        { key: "start", point: { x: line.x1, y: line.y1 } },
        { key: "end", point: { x: line.x2, y: line.y2 } },
        { key: "anchor", point: { x: line.ax, y: line.ay } },
        { key: "head", point: { x: line.hx, y: line.hy } }
      ]
    : [];
  const dirPoints = direction
    ? [
        { key: "tail", point: { x: direction.x1, y: direction.y1 } },
        { key: "head", point: { x: direction.x2, y: direction.y2 } }
      ]
    : [];
  const handles = rectPoints.length ? rectPoints : linePoints.length ? linePoints : dirPoints;
  return handles.map(handle => {
    const stage = placePoint(handle.point);
    return {
      key: handle.key,
      x: stage.x,
      y: stage.y,
      radius: ANALYZER_ANCHOR_RADIUS,
      fill: ANALYZER_ANCHOR_FILL,
      stroke: row?.color,
      strokeWidth: ANALYZER_LINE_WIDTH,
      listening: false
    };
  });
});
const analyzerPanSegments = (row: AnalyzerShapeRow) => {
  const line = row.kind === "line" ? (row.mark as AnalyzerLine) : null;
  const direction = row.kind === "direction" ? (row.mark as AnalyzerDirection) : null;
  const ends = line ? lineEnds({ x: line.x1, y: line.y1 }, { x: line.x2, y: line.y2 }, line.extended) : null;
  const tail = line ? { x: line.ax, y: line.ay } : direction ? { x: direction.x1, y: direction.y1 } : null;
  const head = line ? { x: line.hx, y: line.hy } : direction ? { x: direction.x2, y: direction.y2 } : null;
  const stageTail = tail ? placePoint(tail) : null;
  const stageHead = head ? placePoint(head) : null;
  const wings = stageTail && stageHead ? arrowWings(stageTail, stageHead) : null;
  const shaft = ends ? [placePoint(ends[0]), placePoint(ends[1])] : stageTail && stageHead ? [stageTail, stageHead] : null;
  return [
    shaft,
    stageTail && stageHead ? [stageTail, stageHead] : null,
    wings ? [wings[0], stageHead] : null,
    wings ? [wings[1], stageHead] : null
  ].filter((segment): segment is [AnalyzerPoint, AnalyzerPoint] => Boolean(segment));
};
const analyzerHandleKey = computed(() => {
  const pointer = analyzerPointer.value;
  const handle =
    pointer && analyzerEditing.value
      ? analyzerEditHandles.value.find(
          item => Math.hypot(item.x - pointer.x, item.y - pointer.y) <= ANALYZER_ANCHOR_RADIUS + ANALYZER_PAN_THRESHOLD
        )
      : null;
  return handle?.key ?? "";
});
const analyzerHandleHover = computed(() => Boolean(analyzerHandleKey.value));
const analyzerPanHover = computed(() => {
  const row = analyzerEditing.value;
  const pointer = analyzerPointer.value;
  const rect = row?.kind === "rect" ? (row.mark as AnalyzerRect) : null;
  const stage = rect ? stageRect(rect, analyzerImageBox.value, analyzerImageSize.value) : null;
  const inside = Boolean(
    pointer && stage && pointer.x >= stage.x && pointer.x <= stage.x + stage.width && pointer.y >= stage.y && pointer.y <= stage.y + stage.height
  );
  const nearLine = Boolean(
    pointer && row && row.kind !== "rect" && analyzerPanSegments(row).some(segment => segmentDistance(segment[0], segment[1], pointer) <= ANALYZER_PAN_THRESHOLD)
  );
  return Boolean(pointer && row && !analyzerHandleHover.value && (inside || nearLine));
});
const analyzerPanMask = computed(() => {
  const row = analyzerEditing.value;
  const rect = row?.kind === "rect" && analyzerPanHover.value ? (row.mark as AnalyzerRect) : null;
  const stage = rect ? stageRect(rect, analyzerImageBox.value, analyzerImageSize.value) : null;
  return stage ? { ...stage, fill: `${row?.color ?? "#000000"}33`, listening: false } : null;
});

const clearAnalyzerDraft = () => {
  analyzerAnchor.value = null;
  analyzerCorner.value = null;
  analyzerLineStart.value = null;
  analyzerLineEnd.value = null;
  analyzerLineAnchor.value = null;
  analyzerLineHead.value = null;
  analyzerLineExtended.value = false;
  analyzerDirTail.value = null;
  analyzerDirHead.value = null;
  analyzerTip.value = null;
  analyzerRectName.value = "";
};

const resetAnalyzerMarks = () => {
  analyzerShapes.value = [];
  analyzerRects.value = [];
  analyzerLines.value = [];
  analyzerDirs.value = [];
  analyzerEditing.value = null;
  analyzerEditBackup.value = null;
  clearAnalyzerDraft();
};

const clearAnalyzerImage = () => {
  analyzerImage.value = "";
  analyzerImageName.value = "";
  analyzerImageWidth.value = 0;
  analyzerImageHeight.value = 0;
  analyzerMediaId.value = "";
  analyzerPicture.value = null;
  analyzerImageUrl.value && URL.revokeObjectURL(analyzerImageUrl.value);
  analyzerImageUrl.value = "";
  resetAnalyzerMarks();
};

const editTipPoint = (row: AnalyzerShapeRow) => {
  const rect = row.kind === "rect" ? (row.mark as AnalyzerRect) : null;
  const line = row.kind === "line" ? (row.mark as AnalyzerLine) : null;
  const direction = row.kind === "direction" ? (row.mark as AnalyzerDirection) : null;
  const point = rect
    ? { x: rect.x, y: rect.y }
    : line
      ? { x: line.x2, y: line.y2 }
      : { x: direction?.x2 ?? 0, y: direction?.y2 ?? 0 };
  return placePoint(point);
};

const analyzerTipRule = () => {
  const editing = analyzerEditing.value;
  const rect = editing?.kind === "rect" ? (editing.mark as AnalyzerRect) : null;
  return rect?.rule ?? analyzerRule.value;
};

const analyzerTipExtra = () => {
  const rule = analyzerEditing.value?.kind === "line" || analyzerLineHead.value ? ANALYZER_LINE_RULE : analyzerTipRule();
  const rows = { roi: 1, overcrowding: 2, [ANALYZER_LINE_RULE]: 1 };
  return rows[rule] ?? 0;
};

const analyzerTipSize = () => ({
  x: ANALYZER_TIP_WIDTH,
  y: ANALYZER_TIP_HEIGHT + analyzerTipExtra() * ANALYZER_TIP_EXTEND_HEIGHT
});

const resetRectTip = () => {
  analyzerInverseRoi.value = false;
  analyzerObjectThreshold.value = ANALYZER_OBJECT_THRESHOLD;
  analyzerTimeThreshold.value = ANALYZER_TIME_THRESHOLD;
};

const rectTipFields = () => ({
  inverseRoi: analyzerInverseRoi.value,
  objectThreshold: analyzerObjectThreshold.value.trim(),
  timeThreshold: analyzerTimeThreshold.value.trim()
});

const openAnalyzerEditTip = (row: AnalyzerShapeRow) => {
  const line = row.kind === "line" ? (row.mark as AnalyzerLine) : null;
  const rect = row.kind === "rect" ? (row.mark as AnalyzerRect) : null;
  analyzerLineExtended.value = Boolean(line?.extended);
  analyzerInverseRoi.value = Boolean(rect?.inverseRoi);
  analyzerObjectThreshold.value = rect?.objectThreshold ?? "";
  analyzerTimeThreshold.value = rect?.timeThreshold ?? "";
  analyzerRectName.value = row.name;
  analyzerTip.value = tipOrigin(editTipPoint(row), { x: ANALYZER_CANVAS_WIDTH, y: ANALYZER_CANVAS_HEIGHT }, analyzerTipSize(), ANALYZER_TIP_OFFSET);
};

const onAnalyzerEditEnter = (event: KeyboardEvent) => {
  const enter = event.key === "Enter";
  enter && event.preventDefault();
  enter && onAnalyzerTipConfirm();
};

const onAnalyzerEditKeys = (listen: boolean) => {
  listen && window.addEventListener("keydown", onAnalyzerEditEnter);
  !listen && window.removeEventListener("keydown", onAnalyzerEditEnter);
};

const beginAnalyzerEdit = (row: AnalyzerShapeRow) => {
  const idle = !analyzerEditing.value;
  idle && (analyzerEditBackup.value = structuredClone(toRaw(row.mark)));
  idle && (analyzerEditing.value = row);
  idle && openAnalyzerEditTip(row);
  idle && onAnalyzerEditKeys(true);
};

const finishAnalyzerEdit = () => {
  onAnalyzerEditKeys(false);
  onAnalyzerHandleDragEnd();
  onAnalyzerPanEnd();
  analyzerEditing.value = null;
  analyzerEditBackup.value = null;
  analyzerLineExtended.value = false;
  closeAnalyzerTip();
};

const cancelAnalyzerEdit = () => {
  const row = analyzerEditing.value;
  const backup = analyzerEditBackup.value;
  row && backup && Object.assign(row.mark, backup);
  finishAnalyzerEdit();
};

const commitAnalyzerEdit = (name: string) => {
  const row = analyzerEditing.value;
  const rect = row?.kind === "rect" ? (row.mark as AnalyzerRect) : null;
  row && (row.name = name);
  row && (row.mark.name = name);
  rect && Object.assign(rect, rectTipFields());
  finishAnalyzerEdit();
};

const onAnalyzerHandleDragEnd = () => {
  analyzerHandleDrag.value = null;
  window.removeEventListener("mousemove", onAnalyzerHandleMove);
  window.removeEventListener("mouseup", onAnalyzerHandleDragEnd);
};

const onAnalyzerHandleMove = (event: MouseEvent) => {
  const drag = analyzerHandleDrag.value;
  const row = analyzerEditing.value;
  const point = drag
    ? imagePoint({ x: event.clientX - drag.left, y: event.clientY - drag.top }, analyzerImageBox.value, analyzerImageSize.value)
    : null;
  const rect = row?.kind === "rect" ? (row.mark as AnalyzerRect) : null;
  const line = row?.kind === "line" ? (row.mark as AnalyzerLine) : null;
  const direction = row?.kind === "direction" ? (row.mark as AnalyzerDirection) : null;
  point && (analyzerPointer.value = { x: event.clientX - (drag?.left ?? 0), y: event.clientY - (drag?.top ?? 0) });
  point && rect && drag?.fixed && moveRectBetween(rect, drag.fixed, point);
  point && line && drag && moveLinePoint(line, drag.key, point);
  point && direction && drag && moveDirPoint(direction, drag.key, point);
};

const onAnalyzerHandleDragStart = (bounds: DOMRect) => {
  const row = analyzerEditing.value;
  const key = analyzerHandleKey.value;
  const rect = row?.kind === "rect" ? (row.mark as AnalyzerRect) : null;
  const fixed = rect ? oppositeRectCorner(rect, key) : null;
  const idle = Boolean(key && row && !analyzerHandleDrag.value);
  idle && (analyzerHandleDrag.value = { key, left: bounds.left, top: bounds.top, fixed });
  idle && window.addEventListener("mousemove", onAnalyzerHandleMove);
  idle && window.addEventListener("mouseup", onAnalyzerHandleDragEnd);
};

const onAnalyzerExtend = () => {
  const row = analyzerEditing.value;
  const line = row?.kind === "line" ? (row.mark as AnalyzerLine) : null;
  analyzerLineExtended.value = !analyzerLineExtended.value;
  line && (line.extended = analyzerLineExtended.value);
};

const pushAnalyzerShape = (name: string, kind: AnalyzerShapeKind, mark: AnalyzerShapeRow["mark"]) => {
  analyzerShapes.value.push({ name, color: ANALYZER_SHAPE_COLORS[analyzerRule.value], kind, mark });
};

const removeAnalyzerShape = (row: AnalyzerShapeRow) => {
  const lists = { rect: analyzerRects, line: analyzerLines, direction: analyzerDirs };
  const list = lists[row.kind].value as AnalyzerShapeRow["mark"][];
  const markIndex = list.indexOf(row.mark);
  markIndex >= 0 && list.splice(markIndex, 1);
  const rowIndex = analyzerShapes.value.indexOf(row);
  rowIndex >= 0 && analyzerShapes.value.splice(rowIndex, 1);
};

watch(analyzerRule, clearAnalyzerDraft);

const onAnalyzerPointer = (event: KonvaEventObject<MouseEvent>) => {
  const stage = event.target.getStage();
  const point = stage ? stage.getPointerPosition() : null;
  analyzerPointer.value = analyzerPicture.value ? point : null;
};

const onAnalyzerPointerLeave = () => {
  !analyzerPan.value && (analyzerPointer.value = null);
};

const analyzerPan = ref<{ ix: number; iy: number; left: number; top: number; backup: AnalyzerShapeRow["mark"] } | null>(null);

const onAnalyzerPanMove = (event: MouseEvent) => {
  const pan = analyzerPan.value;
  const row = analyzerEditing.value;
  const point = pan
    ? imagePoint({ x: event.clientX - pan.left, y: event.clientY - pan.top }, analyzerImageBox.value, analyzerImageSize.value)
    : null;
  const rect = row?.kind === "rect" ? (row.mark as AnalyzerRect) : null;
  const line = row?.kind === "line" ? (row.mark as AnalyzerLine) : null;
  const direction = row?.kind === "direction" ? (row.mark as AnalyzerDirection) : null;
  point && rect && pan && shiftRect(rect, pan.backup as AnalyzerRect, point.x - pan.ix, point.y - pan.iy, analyzerImageSize.value);
  point && line && pan && shiftLine(line, pan.backup as AnalyzerLine, point.x - pan.ix, point.y - pan.iy, analyzerImageSize.value);
  point && direction && pan && shiftDirection(direction, pan.backup as AnalyzerDirection, point.x - pan.ix, point.y - pan.iy, analyzerImageSize.value);
};

const onAnalyzerPanEnd = () => {
  analyzerPan.value = null;
  window.removeEventListener("mousemove", onAnalyzerPanMove);
  window.removeEventListener("mouseup", onAnalyzerPanEnd);
};

const onAnalyzerPanStart = (event: KonvaEventObject<MouseEvent>) => {
  const row = analyzerEditing.value;
  const pointer = analyzerPointer.value;
  const bounds = event.target.getStage()?.container().getBoundingClientRect();
  const image = pointer ? imagePoint(pointer, analyzerImageBox.value, analyzerImageSize.value) : null;
  bounds && event.evt.button === 0 && analyzerHandleKey.value && onAnalyzerHandleDragStart(bounds);
  const ready = Boolean(!analyzerHandleKey.value && analyzerPanHover.value && row && image && bounds && event.evt.button === 0);
  ready &&
    row &&
    image &&
    bounds &&
    (analyzerPan.value = {
      ix: image.x,
      iy: image.y,
      left: bounds.left,
      top: bounds.top,
      backup: structuredClone(toRaw(row.mark))
    });
  ready && window.addEventListener("mousemove", onAnalyzerPanMove);
  ready && window.addEventListener("mouseup", onAnalyzerPanEnd);
};

const focusAnalyzerName = () => {
  analyzerNameRef.value?.focus();
};

const openAnalyzerTip = () => {
  const pointer = analyzerPointer.value;
  pointer && (analyzerTip.value = tipOrigin(pointer, { x: ANALYZER_CANVAS_WIDTH, y: ANALYZER_CANVAS_HEIGHT }, analyzerTipSize(), ANALYZER_TIP_OFFSET));
  pointer && (analyzerRectName.value = "");
  pointer && resetRectTip();
  pointer && nextTick(focusAnalyzerName);
};

const closeAnalyzerTip = () => {
  analyzerTip.value = null;
  analyzerRectName.value = "";
};

const analyzerTipDrag = ref<{ x: number; y: number; cx: number; cy: number } | null>(null);

const analyzerTipBox = () => analyzerTipSize();

const onAnalyzerTipDrag = (event: MouseEvent) => {
  const drag = analyzerTipDrag.value;
  drag &&
    (analyzerTip.value = tipOrigin(
      { x: drag.x + event.clientX - drag.cx, y: drag.y + event.clientY - drag.cy },
      { x: ANALYZER_CANVAS_WIDTH, y: ANALYZER_CANVAS_HEIGHT },
      analyzerTipBox(),
      0
    ));
};

const onAnalyzerTipDragEnd = () => {
  analyzerTipDrag.value = null;
  window.removeEventListener("mousemove", onAnalyzerTipDrag);
  window.removeEventListener("mouseup", onAnalyzerTipDragEnd);
};

const onAnalyzerTipDragStart = (event: MouseEvent) => {
  const tip = analyzerTip.value;
  event.preventDefault();
  tip && (analyzerTipDrag.value = { x: tip.x, y: tip.y, cx: event.clientX, cy: event.clientY });
  tip && window.addEventListener("mousemove", onAnalyzerTipDrag);
  tip && window.addEventListener("mouseup", onAnalyzerTipDragEnd);
};

const onRectClick = (snap: AnalyzerPoint) => {
  const anchor = analyzerAnchor.value;
  const rect = anchor ? imageRect(anchor, snap) : null;
  const second = Boolean(anchor && rect && rect.width && rect.height && analyzerPointer.value);
  second && (analyzerCorner.value = snap);
  second && openAnalyzerTip();
  !anchor && (analyzerAnchor.value = snap);
};

const onLineClick = (snap: AnalyzerPoint) => {
  const lineStart = analyzerLineStart.value;
  const lineEnd = analyzerLineEnd.value;
  const lineAnchor = analyzerLineAnchor.value;
  const projected = lineStart && lineEnd ? projectSegment(lineStart, lineEnd, snap) : null;
  const lineFourth = Boolean(lineStart && lineEnd && lineAnchor && !analyzerLineHead.value && apart(lineAnchor, snap));
  const lineThird = Boolean(projected && !lineAnchor);
  const lineSecond = Boolean(lineStart && !lineEnd && apart(lineStart, snap));
  const lineFirst = Boolean(!lineStart);
  lineFirst && (analyzerLineStart.value = snap);
  lineSecond && (analyzerLineEnd.value = snap);
  lineThird && projected && (analyzerLineAnchor.value = projected);
  lineFourth && (analyzerLineHead.value = snap);
  lineFourth && openAnalyzerTip();
};

const onDirClick = (snap: AnalyzerPoint) => {
  const tail = analyzerDirTail.value;
  const second = Boolean(tail && apart(tail, snap));
  second && (analyzerDirHead.value = snap);
  second && openAnalyzerTip();
  !tail && (analyzerDirTail.value = snap);
};

const analyzerClicks: Record<string, (snap: AnalyzerPoint) => void> = {
  ...Object.fromEntries(ANALYZER_RECT_RULES.map(rule => [rule, onRectClick])),
  [ANALYZER_LINE_RULE]: onLineClick,
  [ANALYZER_DIRECTION_RULE]: onDirClick
};

const onAnalyzerClick = (event: KonvaEventObject<MouseEvent>) => {
  const point = event.evt.button === 0 && !analyzerTip.value && !analyzerEditing.value ? (analyzerSnap.value?.point ?? null) : null;
  point && analyzerClicks[analyzerRule.value]?.(point);
};

const onRectContext = () => {
  const corner = Boolean(analyzerCorner.value);
  corner && (analyzerCorner.value = null);
  corner && closeAnalyzerTip();
  !corner && analyzerAnchor.value && (analyzerAnchor.value = null);
};

const onLineContext = () => {
  const lineHead = Boolean(analyzerLineHead.value);
  const lineAnchor = Boolean(!lineHead && analyzerLineAnchor.value);
  const lineEnd = Boolean(!lineHead && !lineAnchor && analyzerLineEnd.value);
  const lineStart = Boolean(!lineHead && !lineAnchor && !lineEnd && analyzerLineStart.value);
  lineHead && (analyzerLineHead.value = null);
  lineHead && (analyzerLineExtended.value = false);
  lineHead && closeAnalyzerTip();
  lineAnchor && (analyzerLineAnchor.value = null);
  lineEnd && (analyzerLineEnd.value = null);
  lineStart && (analyzerLineStart.value = null);
};

const onDirContext = () => {
  const head = Boolean(analyzerDirHead.value);
  head && (analyzerDirHead.value = null);
  head && closeAnalyzerTip();
  !head && analyzerDirTail.value && (analyzerDirTail.value = null);
};

const analyzerContexts: Record<string, () => void> = {
  ...Object.fromEntries(ANALYZER_RECT_RULES.map(rule => [rule, onRectContext])),
  [ANALYZER_LINE_RULE]: onLineContext,
  [ANALYZER_DIRECTION_RULE]: onDirContext
};

const onAnalyzerContext = () => {
  !analyzerEditing.value && analyzerContexts[analyzerRule.value]?.();
};

const onAnalyzerTipCancel = () => {
  const editing = Boolean(analyzerEditing.value);
  editing && cancelAnalyzerEdit();
  !editing && clearAnalyzerDraft();
};

const onRectConfirm = (name: string) => {
  const anchor = analyzerAnchor.value;
  const corner = analyzerCorner.value;
  const rect = anchor && corner ? imageRect(anchor, corner) : null;
  const saved = rect ? { ...rect, name, rule: analyzerRule.value, ...rectTipFields() } : null;
  saved && analyzerRects.value.push(saved);
  saved && pushAnalyzerShape(name, "rect", saved);
  saved && clearAnalyzerDraft();
};

const onLineConfirm = (name: string) => {
  const lineStart = analyzerLineStart.value;
  const lineEnd = analyzerLineEnd.value;
  const lineAnchor = analyzerLineAnchor.value;
  const lineHead = analyzerLineHead.value;
  const line =
    lineStart && lineEnd && lineAnchor && lineHead
      ? {
          name,
          x1: lineStart.x,
          y1: lineStart.y,
          x2: lineEnd.x,
          y2: lineEnd.y,
          ax: lineAnchor.x,
          ay: lineAnchor.y,
          hx: lineHead.x,
          hy: lineHead.y,
          extended: analyzerLineExtended.value
        }
      : null;
  line && analyzerLines.value.push(line);
  line && pushAnalyzerShape(name, "line", line);
  line && clearAnalyzerDraft();
};

const onDirConfirm = (name: string) => {
  const tail = analyzerDirTail.value;
  const head = analyzerDirHead.value;
  const direction = tail && head ? { name, x1: tail.x, y1: tail.y, x2: head.x, y2: head.y } : null;
  direction && analyzerDirs.value.push(direction);
  direction && pushAnalyzerShape(name, "direction", direction);
  direction && clearAnalyzerDraft();
};

const analyzerConfirms: Record<string, (name: string) => void> = {
  ...Object.fromEntries(ANALYZER_RECT_RULES.map(rule => [rule, onRectConfirm])),
  [ANALYZER_LINE_RULE]: onLineConfirm,
  [ANALYZER_DIRECTION_RULE]: onDirConfirm
};

const onAnalyzerTipConfirm = () => {
  const name = analyzerRectName.value.trim();
  const editing = analyzerEditing.value;
  const duplicated = analyzerShapes.value.some(row => row !== editing && row.name === name);
  name && duplicated && message(transformI18n($t("pipeline.pureNameTaken")), { type: "warning" });
  name && !duplicated && editing && commitAnalyzerEdit(name);
  name && !duplicated && !editing && analyzerConfirms[analyzerRule.value]?.(name);
};

const observeGie = () => {
  const el = gieRef.value?.$el as HTMLElement | undefined;
  el && gieObserver.observe(el);
};

const observeAttr = () => {
  const el = attrRef.value?.$el as HTMLElement | undefined;
  el && attrObserver.observe(el);
};

const defaultAttr = (classKey: string): AttrRow => ({
  classKey,
  conf: modelConf.value,
  maxDet: modelMaxDet.value,
  minW: null,
  minH: null,
  maxW: null,
  maxH: null
});

const loadGies = () => {
  getGies({ page: 1, page_size: 20 }).then(res => {
    gieItems.value = res.data.items;
  });
};

const gieDraft = (): GieDraft => {
  const classAttrs = gieAttrs.value
    .filter(row => row.classKey)
    .reduce<GieClassAttrs>((attrs, row) => {
      attrs[row.classKey] = Object.fromEntries(
        Object.entries(attrFields(row)).filter((entry): entry is [string, number] => entry[1] !== null)
      );
      return attrs;
    }, {});
  return {
    model_id: gieModel.value,
    name: gieName.value,
    interval: gieInterval.value,
    class_attrs: classAttrs
  };
};

const fillGie = (item: GieItem) => {
  gieId.value = item.id;
  gieName.value = item.name;
  gieInterval.value = item.interval;
  gieModel.value = item.model_id;
  gieModelType.value = "";
  gieClasses.value = [];
  modelConf.value = null;
  modelMaxDet.value = null;
  gieAttrs.value = attrRows(item.class_attrs);
  gieAttrs.value.length === 0 && (gieAttrs.value = [emptyAttr("all")]);
  gieCreateVisible.value = true;
  item.model_id && onGieModel(item.model_id, true);
};

const onGie = () => {
  gieVisible.value = true;
  loadGies();
  getModelMap().then(res => {
    gieModels.value = Object.entries(res.data).map(([name, id]) => ({ id, name }));
  });
};

const onGieAttrs = (row: GieItem) => {
  gieAttrRows.value = attrRows(row.class_attrs);
  gieAttrsOpen.value = true;
};

const onGieSelect = (value: GieItem[]) => {
  gieSelected.value = value;
};

const onSelect = (value: Record<string, string>[]) => {
  selected.value = value;
};

const onAnalyzerAnnotations = (row: AnalyzerItem) => {
  analyzerAnnotationText.value = JSON.stringify(row.annotations ?? {}, null, 2);
  analyzerAnnotationsOpen.value = true;
};

const onAnalyzerSelect = (value: AnalyzerItem[]) => {
  analyzerSelected.value = value;
};

const onGieCreate = () => {
  gieId.value = "";
  gieName.value = "";
  gieInterval.value = 0;
  gieModel.value = "";
  gieModelType.value = "";
  gieClasses.value = [];
  modelConf.value = null;
  modelMaxDet.value = null;
  gieAttrs.value = [emptyAttr("all")];
  gieCreateVisible.value = true;
};

const onGieEdit = (row: GieItem) => {
  getGie(row.id).then(res => fillGie(res.data));
};

const onGieRemove = (row: GieItem) => {
  removeGie(row.id).then(res => {
    message(res.message, { type: "success" });
    loadGies();
  });
};

const onGieRemoveBatch = () => {
  removeGies(gieSelected.value.map(row => row.id)).then(res => {
    message(res.message, { type: "success" });
    loadGies();
  });
};

const onGieModel = (id: string, keepAttrs = false) => {
  getModel(id).then(res => {
    const item = res.data;
    modelConf.value = item.conf;
    modelMaxDet.value = item.max_det;
    gieModelType.value = item.type;
    gieClasses.value = Array.isArray(item.classes) ? item.classes.map(String) : [];
    keepAttrs || (gieAttrs.value = [defaultAttr("all")]);
  });
};

const onAddAttr = () => {
  gieAttrs.value = [...gieAttrs.value, defaultAttr("")];
};

const onRemoveAttr = (index: number) => {
  const next = gieAttrs.value.filter((_, itemIndex) => itemIndex !== index);
  gieAttrs.value = next.length ? next : [defaultAttr("all")];
};

const classTaken = (classKey: string, row: AttrRow) =>
  gieAttrs.value.some(item => item !== row && item.classKey === classKey);

const onGieConfirm = () => {
  const request = gieId.value ? updateGie(gieId.value, gieDraft()) : createGie(gieDraft());
  request.then(res => {
    message(res.message, { type: "success" });
    gieCreateVisible.value = false;
    loadGies();
  });
};

const observeAnalyzer = () => {
  const el = analyzerRef.value?.$el as HTMLElement | undefined;
  el && analyzerObserver.observe(el);
};

watch(gieVisible, value => {
  value && nextTick(observeGie);
});

watch(analyzerVisible, value => {
  value && nextTick(observeAnalyzer);
});

const onAnalyzerPicture = (event: Event) => {
  const image = event.currentTarget as HTMLImageElement;
  const scale = Math.min(
    ANALYZER_CANVAS_WIDTH / image.naturalWidth,
    ANALYZER_CANVAS_HEIGHT / image.naturalHeight
  );
  const width = image.naturalWidth * scale;
  const height = image.naturalHeight * scale;
  analyzerPicture.value = image;
  analyzerImageBox.value = {
    x: (ANALYZER_CANVAS_WIDTH - width) / 2,
    y: (ANALYZER_CANVAS_HEIGHT - height) / 2,
    width,
    height
  };
};

const openAnalyzerPicture = (mediaId: string) => {
  analyzerMediaId.value = mediaId;
  analyzerImageUrl.value && URL.revokeObjectURL(analyzerImageUrl.value);
  getMediaContent(mediaId).then(blob => {
    const url = URL.createObjectURL(blob);
    const image = new Image();
    analyzerImageUrl.value = url;
    image.onload = onAnalyzerPicture;
    image.src = url;
  });
};

const analyzerImageLabel = () => {
  analyzerImage.value = analyzerImageName.value;
};

const showAnalyzerImage = (item: MediaItem) => {
  resetAnalyzerMarks();
  analyzerImageName.value = item.name;
  analyzerImageWidth.value = item.width ?? 0;
  analyzerImageHeight.value = item.height ?? 0;
  analyzerImageLabel();
  openAnalyzerPicture(item.id);
};

const applyAnalyzerAnnotations = (annotations: Record<string, unknown>) => {
  const roi = annotationGroup(annotations, "roi-filtering-stream");
  const crowd = annotationGroup(annotations, "overcrowding-stream");
  const line = annotationGroup(annotations, "line-crossing-stream");
  const direction = annotationGroup(annotations, "direction-detection-stream");
  const inverse = Boolean(Number(roi["inverse-roi"] ?? 0));
  const objectThreshold = String(crowd["object-threshold"] ?? "");
  const timeThreshold = String(crowd["time-threshold"] ?? "");
  const extended = Boolean(Number(line.extended ?? 0));
  const rects = [
    ...annotationFields(roi, "roi-").map(([key, value]) =>
      restoredRect(key.slice(4), "roi", value, inverse, "", "")
    ),
    ...annotationFields(crowd, "roi-").map(([key, value]) =>
      restoredRect(key.slice(4), "overcrowding", value, false, objectThreshold, timeThreshold)
    )
  ];
  const lines = annotationFields(line, "line-crossing-").map(([key, value]) =>
    restoredLine(key.slice("line-crossing-".length), value, extended)
  );
  const dirs = annotationFields(direction, "direction-").map(([key, value]) =>
    restoredDirection(key.slice("direction-".length), value)
  );
  const rows = [
    ...rects.map(mark => ({ name: mark.name, color: ANALYZER_SHAPE_COLORS[mark.rule], kind: "rect" as const, mark })),
    ...lines.map(mark => ({ name: mark.name, color: ANALYZER_SHAPE_COLORS.line, kind: "line" as const, mark })),
    ...dirs.map(mark => ({ name: mark.name, color: ANALYZER_SHAPE_COLORS.direction, kind: "direction" as const, mark }))
  ];
  analyzerRects.value = rects;
  analyzerLines.value = lines;
  analyzerDirs.value = dirs;
  analyzerShapes.value = rows;
};

const onAnalyzerCreate = () => {
  analyzerId.value = "";
  analyzerName.value = "";
  analyzerImage.value = "";
  analyzerImageName.value = "";
  analyzerImageWidth.value = 0;
  analyzerImageHeight.value = 0;
  analyzerMediaId.value = "";
  analyzerCreateVisible.value = true;
};

const onAnalyzerEdit = (row: AnalyzerItem) => {
  analyzerId.value = row.id;
  analyzerName.value = row.name;
  analyzerImageName.value = row.media_name ?? "";
  analyzerImageWidth.value = row.width ?? 0;
  analyzerImageHeight.value = row.height ?? 0;
  analyzerImageLabel();
  resetAnalyzerMarks();
  applyAnalyzerAnnotations(row.annotations ?? {});
  analyzerCreateVisible.value = true;
  row.media_id && openAnalyzerPicture(row.media_id);
};

const onAnalyzerRemove = (row: AnalyzerItem) => {
  removeAnalyzer(row.id).then(res => {
    message(res.message, { type: "success" });
    loadAnalyzers();
  });
};

const onAnalyzerRemoveBatch = () => {
  removeAnalyzers(analyzerSelected.value.map(row => row.id)).then(res => {
    message(res.message, { type: "success" });
    loadAnalyzers();
  });
};

const loadAnalyzerImages = () => {
  const userId = useUserStoreHook().userId;
  getUploadFiles({
    page: analyzerImagePage.value,
    page_size: ANALYZER_IMAGE_PAGE_SIZE,
    type: "image",
    ...(userId ? { user_id: userId } : {})
  }).then(res => {
    analyzerImages.value = res.data.items;
    analyzerImageTotal.value = res.data.total;
  });
};

const loadAnalyzers = () => {
  getAnalyzers({ page: 1, page_size: 20 }).then(res => {
    analyzerItems.value = res.data.items;
  });
};

const onAnalyzer = () => {
  analyzerVisible.value = true;
  loadAnalyzers();
};

const analyzerPointText = (values: number[]) => values.map(value => Math.round(value)).join(";");

const analyzerRectText = (rect: AnalyzerRect) =>
  analyzerPointText([
    rect.x,
    rect.y,
    rect.x + rect.width,
    rect.y,
    rect.x + rect.width,
    rect.y + rect.height,
    rect.x,
    rect.y + rect.height
  ]);

const onAnalyzerConfirm = () => {
  const rois = analyzerRects.value.filter(rect => rect.rule === "roi");
  const crowds = analyzerRects.value.filter(rect => rect.rule === "overcrowding");
  const annotations: Record<string, Record<string, string | number>> = {};
  rois.length &&
    (annotations["roi-filtering-stream"] = {
      enable: ANALYZER_ENABLE,
      "class-id": ANALYZER_CLASS_ID,
      "inverse-roi": Number(rois[0].inverseRoi),
      ...Object.fromEntries(rois.map(rect => [`roi-${rect.name}`, analyzerRectText(rect)]))
    });
  crowds.length &&
    (annotations["overcrowding-stream"] = {
      enable: ANALYZER_ENABLE,
      "class-id": ANALYZER_CLASS_ID,
      "object-threshold": Number(crowds[0].objectThreshold),
      "time-threshold": Number(crowds[0].timeThreshold),
      ...Object.fromEntries(crowds.map(rect => [`roi-${rect.name}`, analyzerRectText(rect)]))
    });
  analyzerLines.value.length &&
    (annotations["line-crossing-stream"] = {
      enable: ANALYZER_ENABLE,
      "class-id": ANALYZER_CLASS_ID,
      extended: Number(analyzerLines.value[0].extended),
      mode: ANALYZER_LINE_MODE,
      ...Object.fromEntries(
        analyzerLines.value.map(line => [
          `line-crossing-${line.name}`,
          analyzerPointText([line.ax, line.ay, line.hx, line.hy, line.x1, line.y1, line.x2, line.y2])
        ])
      )
    });
  analyzerDirs.value.length &&
    (annotations["direction-detection-stream"] = {
      enable: ANALYZER_ENABLE,
      "class-id": ANALYZER_CLASS_ID,
      mode: ANALYZER_DIRECTION_MODE,
      ...Object.fromEntries(analyzerDirs.value.map(dir => [`direction-${dir.name}`, analyzerPointText([dir.x1, dir.y1, dir.x2, dir.y2])]))
    });
  const marked = Boolean(Object.keys(annotations).length);
  marked &&
    (annotations.property = {
      enable: ANALYZER_ENABLE,
      "config-width": analyzerImageWidth.value,
      "config-height": analyzerImageHeight.value,
      "osd-mode": ANALYZER_OSD_MODE,
      "display-font-size": ANALYZER_DISPLAY_FONT_SIZE
    });
  !marked && message(transformI18n($t("pipeline.pureRuleRequired")), { type: "warning" });
  const draft = {
    name: analyzerName.value.trim(),
    media_id: analyzerMediaId.value,
    annotations
  };
  marked &&
    (analyzerId.value ? updateAnalyzer(analyzerId.value, draft) : createAnalyzer(draft)).then(res => {
      message(res.message, { type: "success" });
      analyzerCreateVisible.value = false;
      loadAnalyzers();
    });
};

const onAnalyzerUpload = () => {
  analyzerFileRef.value?.click();
};

const onAnalyzerFile = (event: Event) => {
  const input = event.target as HTMLInputElement;
  const file = input.files?.[0] ?? null;
  const suffix = file ? file.name.slice(file.name.lastIndexOf(".")).toLowerCase() : "";
  const allowed = ANALYZER_IMAGE_SUFFIXES.includes(suffix);
  const userId = useUserStoreHook().userId;
  input.value = "";
  allowed &&
    file &&
    userId &&
    uploadMedia(userId, [file]).then(res => {
      const item = res.data.updated_rows[0];
      item ? showAnalyzerImage(item) : message(res.message, { type: "error" });
    });
  file && !allowed && message(transformI18n($t("pipeline.pureImageSuffix")), { type: "warning" });
};

const onAnalyzerImages = () => {
  analyzerImagePage.value = 1;
  analyzerImagesVisible.value = true;
};

const onAnalyzerImagePage = (page: number) => {
  analyzerImagePage.value = page;
  loadAnalyzerImages();
};

const onPickAnalyzerImage = (item: MediaItem) => {
  analyzerImagesVisible.value = false;
  showAnalyzerImage(item);
};

watch(analyzerImagesVisible, value => {
  value && loadAnalyzerImages();
});

watch(analyzerRule, () => {
  clearAnalyzerDraft();
});

watch(analyzerCreateVisible, value => {
  value || resetAnalyzerMarks();
});

watch(gieCreateVisible, value => {
  value && nextTick(observeAttr);
});

onMounted(() => {
  const el = listRef.value?.$el as HTMLElement | undefined;
  el && listObserver.observe(el);
});

onUnmounted(() => {
  listObserver.disconnect();
  gieObserver.disconnect();
  analyzerObserver.disconnect();
  attrObserver.disconnect();
  analyzerImageUrl.value && URL.revokeObjectURL(analyzerImageUrl.value);
  onAnalyzerTipDragEnd();
  onAnalyzerPanEnd();
  onAnalyzerHandleDragEnd();
  onAnalyzerEditKeys(false);
});
</script>

<template>
  <div class="pipeline-page">
    <div class="pipeline-toolbar">
      <el-button type="primary" @click="pipelineCreateVisible = true">
        {{ transformI18n($t("pipeline.pureCreate")) }}
      </el-button>
      <el-button type="primary" :disabled="!selected.length">
        {{ transformI18n($t("pipeline.pureRemove")) }}
      </el-button>
      <el-button type="primary" @click="onGie">
        {{ transformI18n($t("pipeline.pureGie")) }}
      </el-button>
      <el-button type="primary" @click="onAnalyzer">
        {{ transformI18n($t("pipeline.pureAnalyzer")) }}
      </el-button>
    </div>
    <DetVisRtsp v-model="pipelineCreateVisible" />
    <el-dialog
      v-model="gieVisible"
      class="create-dialog"
      :title="transformI18n($t('pipeline.pureGie'))"
      :width="GIE_DIALOG_WIDTH"
    >
      <div class="pipeline-toolbar">
        <el-button type="primary" @click="onGieCreate">
          {{ transformI18n($t("pipeline.pureCreate")) }}
        </el-button>
        <el-button type="primary" :disabled="!gieSelected.length" @click="onGieRemoveBatch">
          {{ transformI18n($t("pipeline.pureRemove")) }}
        </el-button>
      </div>
      <el-table ref="gieRef" class="gie-list" :data="gieItems" row-key="id" table-layout="fixed" border show-overflow-tooltip @selection-change="onGieSelect">
        <el-table-column type="selection" :width="gieWidths.selection" />
        <el-table-column prop="name" :label="transformI18n($t('pipeline.pureName'))" :width="gieWidths.name" />
        <el-table-column prop="model_name" :label="transformI18n($t('pipeline.pureModel'))" :width="gieWidths.model" />
        <el-table-column :label="transformI18n($t('pipeline.pureClassAttrs'))" :width="gieWidths.classAttrs" :show-overflow-tooltip="false">
          <template #default="{ row }">
            <el-button link type="primary" :title="transformI18n($t('pipeline.pureClassAttrs'))" @click="onGieAttrs(row)">
              <IconifyIconOffline :icon="ListCheck" />
            </el-button>
          </template>
        </el-table-column>
        <el-table-column prop="interval" :label="transformI18n($t('pipeline.pureInterval'))" :width="gieWidths.interval" />
        <el-table-column :label="transformI18n($t('pipeline.pureUpdatedAt'))" :width="gieWidths.updatedAt">
          <template #default="{ row }">{{ formatLocalTime(row.updated_at) }}</template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('pipeline.pureOps'))" :width="gieWidths.ops" :show-overflow-tooltip="false">
          <template #default="{ row }">
            <el-button link type="primary" :title="transformI18n($t('pipeline.pureEdit'))" @click="onGieEdit(row)">
              <IconifyIconOffline :icon="Edit" />
            </el-button>
            <el-button link type="primary" :title="transformI18n($t('pipeline.pureRemove'))" @click="onGieRemove(row)">
              <IconifyIconOffline :icon="DeleteBin" />
            </el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-dialog>
    <el-dialog v-model="gieAttrsOpen" append-to-body :title="transformI18n($t('pipeline.pureClassAttrs'))" :width="GIE_ATTR_DIALOG_WIDTH">
      <el-table :data="gieAttrRows" border max-height="420" show-overflow-tooltip>
        <el-table-column :label="transformI18n($t('pipeline.pureClass'))">
          <template #default="{ row }">{{ row.classKey === "all" ? transformI18n($t("pipeline.pureAll")) : row.classKey }}</template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('pipeline.pureConf'))" prop="conf" />
        <el-table-column :label="transformI18n($t('pipeline.pureMaxDet'))" prop="maxDet" />
        <el-table-column :label="transformI18n($t('pipeline.pureMinW'))" prop="minW" />
        <el-table-column :label="transformI18n($t('pipeline.pureMinH'))" prop="minH" />
        <el-table-column :label="transformI18n($t('pipeline.pureMaxW'))" prop="maxW" />
        <el-table-column :label="transformI18n($t('pipeline.pureMaxH'))" prop="maxH" />
      </el-table>
    </el-dialog>
    <el-dialog
      v-model="gieCreateVisible"
      append-to-body
      class="create-dialog"
      :title="transformI18n($t(gieId ? 'pipeline.pureEdit' : 'pipeline.pureCreate'))"
      :width="GIE_FORM_DIALOG_WIDTH"
    >
      <el-form label-width="88px" label-position="left">
        <el-form-item :label="transformI18n($t('pipeline.pureName'))">
          <el-input v-model="gieName" />
        </el-form-item>
        <el-form-item :label="transformI18n($t('pipeline.pureInterval'))">
          <el-input-number v-model="gieInterval" :min="0" :precision="0" :step="1" />
        </el-form-item>
        <el-form-item :label="transformI18n($t('pipeline.pureModel'))">
          <el-select v-model="gieModel" class="gie-model" @change="onGieModel">
            <el-option v-for="item in gieModels" :key="item.id" :label="item.name" :value="item.id" />
          </el-select>
        </el-form-item>
      </el-form>
      <el-table ref="attrRef" class="create-table" :data="gieAttrs" table-layout="fixed" border>
        <el-table-column :label="transformI18n($t('pipeline.pureClass'))" :width="attrWidths.classKey">
          <template #default="{ row }">
            <el-select v-model="row.classKey" :disabled="!gieModel">
              <el-option :label="transformI18n($t('pipeline.pureAll'))" value="all" :disabled="classTaken('all', row)" />
              <el-option
                v-for="(name, index) in gieClasses"
                :key="index"
                :label="name"
                :value="String(index)"
                :disabled="classTaken(String(index), row)"
              />
            </el-select>
          </template>
        </el-table-column>
        <el-table-column align="center" class-name="attr-num" :label="transformI18n($t('pipeline.pureConf'))" :width="attrWidths.conf">
          <template #default="{ row }">
            <el-input-number v-model="row.conf" :disabled="!gieModel" :min="modelConf ?? 0" :max="1" :step="0.01" />
          </template>
        </el-table-column>
        <el-table-column align="center" class-name="attr-num" :label="transformI18n($t('pipeline.pureMaxDet'))" :width="attrWidths.maxDet">
          <template #default="{ row }">
            <el-input-number v-model="row.maxDet" :disabled="!gieModel" :min="0" :max="modelMaxDet ?? undefined" :precision="0" :step="1" />
          </template>
        </el-table-column>
        <el-table-column align="center" class-name="attr-num" :label="transformI18n($t('pipeline.pureMinW'))" :width="attrWidths.minW">
          <template #default="{ row }">
            <el-input-number v-model="row.minW" :disabled="!gieModel" :min="0" :precision="0" :step="1" />
          </template>
        </el-table-column>
        <el-table-column align="center" class-name="attr-num" :label="transformI18n($t('pipeline.pureMinH'))" :width="attrWidths.minH">
          <template #default="{ row }">
            <el-input-number v-model="row.minH" :disabled="!gieModel" :min="0" :precision="0" :step="1" />
          </template>
        </el-table-column>
        <el-table-column align="center" class-name="attr-num" :label="transformI18n($t('pipeline.pureMaxW'))" :width="attrWidths.maxW">
          <template #default="{ row }">
            <el-input-number v-model="row.maxW" :disabled="!gieModel" :min="0" :precision="0" :step="1" />
          </template>
        </el-table-column>
        <el-table-column align="center" class-name="attr-num" :label="transformI18n($t('pipeline.pureMaxH'))" :width="attrWidths.maxH">
          <template #default="{ row }">
            <el-input-number v-model="row.maxH" :disabled="!gieModel" :min="0" :precision="0" :step="1" />
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('pipeline.pureOps'))" :width="attrWidths.ops" :show-overflow-tooltip="false">
          <template #default="{ $index }">
            <el-button link type="primary" :disabled="!gieModel" :title="transformI18n($t('pipeline.pureRemove'))" @click="onRemoveAttr($index)">
              <IconifyIconOffline :icon="DeleteBin" />
            </el-button>
          </template>
        </el-table-column>
      </el-table>
      <el-button class="add-row" :disabled="!gieModel" @click="onAddAttr">+</el-button>
      <el-tooltip :content="gieAttrHint" :disabled="!gieAttrHint" placement="top">
        <p class="attr-hint">{{ gieAttrHint }}</p>
      </el-tooltip>
      <template #footer>
        <el-button @click="gieCreateVisible = false">
          {{ transformI18n($t("pipeline.pureCancel")) }}
        </el-button>
        <el-button type="primary" :disabled="!gieConfirmable" @click="onGieConfirm">
          {{ transformI18n($t("pipeline.pureConfirm")) }}
        </el-button>
      </template>
    </el-dialog>
    <el-dialog
      v-model="analyzerVisible"
      class="create-dialog"
      :title="transformI18n($t('pipeline.pureAnalyzer'))"
      :width="ANALYZER_DIALOG_WIDTH"
    >
      <div class="pipeline-toolbar">
        <el-button type="primary" @click="onAnalyzerCreate">
          {{ transformI18n($t("pipeline.pureCreate")) }}
        </el-button>
        <el-button type="primary" :disabled="!analyzerSelected.length" @click="onAnalyzerRemoveBatch">
          {{ transformI18n($t("pipeline.pureRemove")) }}
        </el-button>
      </div>
      <el-table ref="analyzerRef" class="analyzer-list" :data="analyzerItems" table-layout="fixed" border show-overflow-tooltip @selection-change="onAnalyzerSelect">
        <el-table-column type="selection" :width="analyzerWidths.selection" />
        <el-table-column prop="name" :label="transformI18n($t('pipeline.pureName'))" :width="analyzerWidths.name" />
        <el-table-column prop="media_name" :label="transformI18n($t('pipeline.pureImage'))" :width="analyzerWidths.image" />
        <el-table-column prop="width" :label="transformI18n($t('media.pureWidth'))" :width="analyzerWidths.width" />
        <el-table-column prop="height" :label="transformI18n($t('media.pureHeight'))" :width="analyzerWidths.height" />
        <el-table-column :label="transformI18n($t('pipeline.pureAnnotations'))" :width="analyzerWidths.annotations" :show-overflow-tooltip="false">
          <template #default="{ row }">
            <el-button link type="primary" :title="transformI18n($t('pipeline.pureAnnotations'))" @click="onAnalyzerAnnotations(row)">
              <IconifyIconOffline :icon="ListCheck" />
            </el-button>
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('pipeline.pureUpdatedAt'))" :width="analyzerWidths.updatedAt">
          <template #default="{ row }">{{ formatLocalTime(row.updated_at) }}</template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('pipeline.pureOps'))" :width="analyzerWidths.ops" :show-overflow-tooltip="false">
          <template #default="{ row }">
            <el-button link type="primary" :title="transformI18n($t('pipeline.pureEdit'))" @click="onAnalyzerEdit(row)">
              <IconifyIconOffline :icon="Edit" />
            </el-button>
            <el-button link type="primary" :title="transformI18n($t('pipeline.pureRemove'))" @click="onAnalyzerRemove(row)">
              <IconifyIconOffline :icon="DeleteBin" />
            </el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-dialog>
    <el-dialog v-model="analyzerAnnotationsOpen" append-to-body :title="transformI18n($t('pipeline.pureAnnotations'))" :width="ANALYZER_ANNOTATION_DIALOG_WIDTH">
      <pre class="analyzer-annotations">{{ analyzerAnnotationText }}</pre>
    </el-dialog>
    <el-dialog
      v-model="analyzerCreateVisible"
      align-center
      append-to-body
      class="create-dialog"
      :title="transformI18n($t(analyzerId ? 'pipeline.pureEdit' : 'pipeline.pureCreate'))"
      :width="ANALYZER_FORM_DIALOG_WIDTH"
    >
      <el-form label-width="88px" label-position="left">
        <el-form-item :label="transformI18n($t('pipeline.pureName'))">
          <el-input v-model="analyzerName" />
        </el-form-item>
        <el-form-item :label="transformI18n($t('pipeline.pureImage'))">
          <el-input
            v-model="analyzerImage"
            readonly
            :placeholder="transformI18n($t('pipeline.pureSelectImage'))"
            @click="onAnalyzerImages"
          >
            <template #suffix>
              <span v-if="analyzerImageSizeLabel" class="analyzer-image-size">{{ analyzerImageSizeLabel }}</span>
              <button v-if="analyzerImage" type="button" class="analyzer-image-clear" @click.stop="clearAnalyzerImage">
                <IconifyIconOffline :icon="Close" />
              </button>
            </template>
            <template #append>
              <el-button @click.stop="onAnalyzerUpload">
                {{ transformI18n($t("model.pureUpload")) }}
              </el-button>
            </template>
          </el-input>
          <input
            ref="analyzerFileRef"
            class="analyzer-file"
            type="file"
            :accept="ANALYZER_IMAGE_ACCEPT"
            @change="onAnalyzerFile"
          />
        </el-form-item>
      </el-form>
      <div class="analyzer-rules">
        <button
          v-for="rule in ANALYZER_RULES"
          :key="rule.key"
          type="button"
          class="analyzer-rule"
          :class="{ active: analyzerRule === rule.key }"
          :disabled="Boolean(analyzerEditing)"
          @click="analyzerRule = rule.key"
        >
          <span class="analyzer-swatch" :style="{ background: ANALYZER_SHAPE_COLORS[rule.key] }" />
          {{ transformI18n($t(rule.label)) }}
        </button>
      </div>
      <div class="analyzer-work">
      <div class="analyzer-stage" @contextmenu.prevent>
        <Stage
          class="analyzer-canvas"
          :class="{ ready: analyzerPicture }"
          :config="analyzerStage"
          :style="analyzerStageStyle"
          @mousemove="onAnalyzerPointer"
          @mouseleave="onAnalyzerPointerLeave"
          @mousedown="onAnalyzerPanStart"
          @click="onAnalyzerClick"
          @contextmenu="onAnalyzerContext"
        >
          <Layer>
            <KonvaImage v-if="analyzerPicture" :config="analyzerImageConfig" />
            <KonvaRect v-for="(rect, index) in analyzerRectConfigs" :key="index" :config="rect" />
            <KonvaRect v-if="analyzerPanMask" :config="analyzerPanMask" />
            <KonvaLine v-for="(line, index) in analyzerSegments" :key="`segment-${index}`" :config="line" />
            <KonvaLine v-for="(line, index) in analyzerArrows" :key="`arrow-${index}`" :config="line" />
            <KonvaLine v-for="(line, index) in analyzerDirArrows" :key="`dir-${index}`" :config="line" />
            <KonvaLabel v-for="(label, index) in analyzerDirLabels" :key="`dir-label-${index}`" :config="label.place">
              <KonvaTag :config="analyzerLabelTag" />
              <KonvaText :config="label.text" />
            </KonvaLabel>
            <KonvaCircle v-if="analyzerAnchorDot" :config="analyzerAnchorDot" />
            <Shape v-if="analyzerAngle" :config="analyzerAngle.arc" />
            <KonvaText v-if="analyzerAngle" :config="analyzerAngle.text" />
            <KonvaLabel v-for="(label, index) in analyzerLineLabels" :key="`line-label-${index}`" :config="label.place">
              <KonvaTag :config="analyzerLabelTag" />
              <KonvaText :config="label.text" />
            </KonvaLabel>
            <KonvaLabel v-for="(label, index) in analyzerRectLabels" :key="`label-${index}`" :config="label.place">
              <KonvaTag :config="analyzerLabelTag" />
              <KonvaText :config="label.text" />
            </KonvaLabel>
            <KonvaRect v-if="analyzerLiveConfig" :config="analyzerLiveConfig" />
            <KonvaLine v-for="line in analyzerCrosshair" :key="line.key" :config="line" />
            <KonvaLine v-for="line in analyzerEdgeLines" :key="line.key" :config="line" />
            <KonvaCircle v-for="handle in analyzerEditHandles" :key="handle.key" :config="handle" />
          </Layer>
        </Stage>
        <div v-if="(analyzerPanHover || analyzerHandleHover) && analyzerPointer" class="analyzer-pan" :style="{ left: `${analyzerPointer.x}px`, top: `${analyzerPointer.y}px` }">
          <IconifyIconOffline :icon="analyzerHandleHover ? Focus : DragMove" />
        </div>
        <div v-if="analyzerTip" class="analyzer-tip" :style="analyzerTipStyle" @mousedown.stop @click.stop>
          <div class="analyzer-tip-row">
            <button type="button" class="analyzer-tip-drag" @mousedown="onAnalyzerTipDragStart">
              <IconifyIconOffline :icon="Draggable" />
            </button>
            <input ref="analyzerNameRef" v-model="analyzerRectName" class="analyzer-tip-name" @keydown.enter.prevent="onAnalyzerTipConfirm" />
            <button type="button" class="analyzer-tip-btn" :disabled="!analyzerRectName.trim()" @click="onAnalyzerTipConfirm">
              <IconifyIconOffline :icon="Check" />
            </button>
            <button type="button" class="analyzer-tip-btn" @click="onAnalyzerTipCancel">
              <IconifyIconOffline :icon="Close" />
            </button>
          </div>
          <button
            v-if="analyzerTipRule() === 'roi'"
            type="button"
            class="analyzer-tip-extend"
            :class="{ active: analyzerInverseRoi }"
            @click="analyzerInverseRoi = !analyzerInverseRoi"
          >
            {{ transformI18n($t("pipeline.pureInverseRoi")) }}
          </button>
          <label v-if="analyzerTipRule() === 'overcrowding'" class="analyzer-tip-field">
            <span>{{ transformI18n($t("pipeline.pureObjectThreshold")) }}</span>
            <input v-model="analyzerObjectThreshold" @keydown.enter.prevent="onAnalyzerTipConfirm" />
          </label>
          <label v-if="analyzerTipRule() === 'overcrowding'" class="analyzer-tip-field">
            <span>{{ transformI18n($t("pipeline.pureTimeThreshold")) }}</span>
            <input v-model="analyzerTimeThreshold" @keydown.enter.prevent="onAnalyzerTipConfirm" />
          </label>
          <button
            v-if="analyzerLineHead || analyzerEditing?.kind === 'line'"
            type="button"
            class="analyzer-tip-extend"
            :class="{ active: analyzerLineExtended }"
            @click="onAnalyzerExtend"
          >
            {{ transformI18n($t("pipeline.pureExtendEdge")) }}
          </button>
        </div>
      </div>
      <el-table
        class="analyzer-shapes"
        :data="analyzerShapes"
        :height="ANALYZER_CANVAS_HEIGHT + 2"
        table-layout="fixed"
        border
      >
        <el-table-column :label="transformI18n($t('pipeline.pureName'))" prop="name" />
        <el-table-column align="center" :label="transformI18n($t('pipeline.pureType'))" :width="ANALYZER_SHAPE_TYPE_WIDTH">
          <template #default="{ row }">
            <span class="analyzer-swatch" :style="{ background: row.color }" />
          </template>
        </el-table-column>
        <el-table-column :label="transformI18n($t('pipeline.pureOps'))" :width="ANALYZER_SHAPE_OPS_WIDTH" :show-overflow-tooltip="false">
          <template #default="{ row }">
            <el-button link type="primary" :disabled="Boolean(analyzerEditing)" :title="transformI18n($t('pipeline.pureEdit'))" @click="beginAnalyzerEdit(row)">
              <IconifyIconOffline :icon="Edit" />
            </el-button>
            <el-button link type="primary" :disabled="Boolean(analyzerEditing)" :title="transformI18n($t('pipeline.pureRemove'))" @click="removeAnalyzerShape(row)">
              <IconifyIconOffline :icon="DeleteBin" />
            </el-button>
          </template>
        </el-table-column>
      </el-table>
      </div>
      <template #footer>
        <el-button @click="analyzerCreateVisible = false">
          {{ transformI18n($t("pipeline.pureCancel")) }}
        </el-button>
        <el-button type="primary" :disabled="!analyzerConfirmable" @click="onAnalyzerConfirm">
          {{ transformI18n($t("pipeline.pureConfirm")) }}
        </el-button>
      </template>
    </el-dialog>
    <el-dialog
      v-model="analyzerImagesVisible"
      align-center
      append-to-body
      class="create-dialog"
      :title="transformI18n($t('pipeline.pureImage'))"
      :width="ANALYZER_IMAGE_DIALOG_WIDTH"
    >
      <el-table :data="analyzerImages" table-layout="fixed" border show-overflow-tooltip @row-click="onPickAnalyzerImage">
        <el-table-column :label="transformI18n($t('pipeline.pureName'))" prop="name" />
        <el-table-column :label="transformI18n($t('media.pureWidth'))" prop="width" width="96" />
        <el-table-column :label="transformI18n($t('media.pureHeight'))" prop="height" width="96" />
        <el-table-column :label="transformI18n($t('pipeline.pureUpdatedAt'))">
          <template #default="{ row }">
            {{ formatLocalTime(row.updated_at) }}
          </template>
        </el-table-column>
      </el-table>
      <el-pagination
        class="analyzer-pager"
        layout="prev, pager, next"
        :current-page="analyzerImagePage"
        :page-size="ANALYZER_IMAGE_PAGE_SIZE"
        :total="analyzerImageTotal"
        @current-change="onAnalyzerImagePage"
      />
    </el-dialog>
    <el-table ref="listRef" class="pipeline-list" :data="items" table-layout="fixed" border show-overflow-tooltip @selection-change="onSelect">
      <el-table-column type="selection" :width="listWidths.selection" />
      <el-table-column :label="transformI18n($t('pipeline.pureName'))" :width="listWidths.name" />
      <el-table-column :label="transformI18n($t('pipeline.pureType'))" :width="listWidths.type" />
      <el-table-column :label="transformI18n($t('pipeline.pureStatus'))" :width="listWidths.status" />
      <el-table-column :label="transformI18n($t('pipeline.pureGie'))" :width="listWidths.gie" />
      <el-table-column :label="transformI18n($t('pipeline.pureAnalyzer'))" :width="listWidths.analyzer" />
      <el-table-column :label="transformI18n($t('pipeline.pureOps'))" :width="listWidths.ops" :show-overflow-tooltip="false">
        <template #default>
          <el-button link type="primary" :title="transformI18n($t('pipeline.pureStart'))">
            <IconifyIconOffline :icon="Play" />
          </el-button>
          <el-button link type="primary" :title="transformI18n($t('pipeline.pureEdit'))">
            <IconifyIconOffline :icon="Edit" />
          </el-button>
          <el-button link type="primary" :title="transformI18n($t('pipeline.pureRemove'))">
            <IconifyIconOffline :icon="DeleteBin" />
          </el-button>
        </template>
      </el-table-column>
    </el-table>
  </div>
</template>

<style scoped>
.pipeline-page.main-content {
  display: flex;
  flex: 1;
  flex-direction: column;
  min-width: 0;
  min-height: 0;
  margin: 0;
  overflow: hidden;
  background: var(--el-bg-color);
}

.pipeline-toolbar {
  display: flex;
  flex: none;
  gap: 8px;
  align-items: center;
  padding: 8px;
  background: var(--el-bg-color);
  border-bottom: 1px solid var(--el-border-color);
}

.pipeline-toolbar :deep(.el-button) {
  margin: 0;
}

.pipeline-list {
  flex: 1;
  width: 100%;
  min-height: 0;
}

.pipeline-list :deep(.cell) {
  word-break: normal;
  white-space: nowrap;
}

.pipeline-list :deep(.el-button) {
  margin: 0;
  font-size: 16px;
}

.analyzer-list :deep(.cell),
.gie-list :deep(.cell) {
  word-break: normal;
  white-space: nowrap;
}

.analyzer-list :deep(.el-button),
.gie-list :deep(.el-button) {
  margin: 0;
  font-size: 16px;
}

.gie-model,
.create-table :deep(.el-input-number),
.create-table :deep(.el-select) {
  width: 100%;
}

.create-table :deep(.cell) {
  padding: 0 4px;
  white-space: nowrap;
}

.create-table :deep(.attr-num .el-input__inner) {
  text-align: center;
}

.add-row {
  width: 100%;
  margin-top: -1px;
  border-radius: 0;
}

.attr-hint {
  height: 20px;
  margin: 6px 0 0;
  overflow: hidden;
  font-size: 12px;
  line-height: 20px;
  color: var(--el-color-danger);
  white-space: nowrap;
  text-overflow: ellipsis;
  background: transparent;
}

.analyzer-rules {
  display: flex;
}

.analyzer-rule {
  display: inline-flex;
  flex: 1;
  gap: 8px;
  align-items: center;
  justify-content: center;
  margin: 0;
  padding: 8px 12px;
  color: var(--el-text-color-regular);
  cursor: pointer;
  background: var(--el-bg-color);
  border: 1px solid var(--el-border-color);
  border-radius: 0;
}

.analyzer-rule + .analyzer-rule {
  margin-left: -1px;
}

.analyzer-rule.active {
  z-index: 1;
  color: var(--el-color-primary);
  border-color: var(--el-color-primary);
}

.analyzer-rule:disabled {
  cursor: default;
  opacity: 0.5;
}

.analyzer-work {
  display: flex;
  gap: 12px;
  align-items: flex-start;
  margin-top: 8px;
}

.analyzer-stage {
  position: relative;
  flex: none;
  width: fit-content;
  border: 1px solid var(--el-border-color);
}

.analyzer-shapes {
  flex: 1;
  min-width: 0;
}

.analyzer-swatch {
  display: inline-block;
  width: 14px;
  height: 14px;
  vertical-align: middle;
  border: 1px solid #000;
}

html.dark .analyzer-swatch {
  border-color: #fff;
}

.analyzer-canvas.ready {
  cursor: crosshair;
}

.analyzer-pan {
  position: absolute;
  z-index: 2;
  display: flex;
  pointer-events: none;
  color: var(--el-color-primary);
  font-size: 18px;
  transform: translate(-50%, -50%);
}

.analyzer-tip {
  position: absolute;
  z-index: 2;
  display: flex;
  flex-direction: column;
  width: 200px;
  background: var(--el-bg-color);
  border: 1px solid var(--el-border-color);
}

.analyzer-tip-row {
  display: flex;
  height: 32px;
}

.analyzer-tip-field {
  display: flex;
  height: 32px;
  border-top: 1px solid var(--el-border-color);
}

.analyzer-tip-field span {
  display: flex;
  align-items: center;
  width: 72px;
  padding: 0 8px;
  font-size: 12px;
  color: var(--el-text-color-regular);
  border-right: 1px solid var(--el-border-color);
}

.analyzer-tip-field input {
  flex: 1;
  min-width: 0;
  padding: 0 8px;
  color: var(--el-text-color-regular);
  background: transparent;
  border: 0;
  outline: none;
}

.analyzer-tip-extend {
  height: 32px;
  padding: 0;
  font-size: 12px;
  color: var(--el-text-color-regular);
  cursor: pointer;
  background: transparent;
  border: 0;
  border-top: 1px solid var(--el-border-color);
}

.analyzer-tip-extend.active {
  color: var(--el-color-white);
  background: var(--el-color-primary);
}

.analyzer-tip-drag {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 32px;
  padding: 0;
  font-size: 16px;
  color: var(--el-text-color-regular);
  cursor: grab;
  background: transparent;
  border: 0;
  border-right: 1px solid var(--el-border-color);
}

.analyzer-tip-name {
  flex: 1;
  min-width: 0;
  padding: 0 8px;
  color: var(--el-text-color-regular);
  background: transparent;
  border: 0;
  outline: none;
}

.analyzer-tip-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 32px;
  padding: 0;
  font-size: 16px;
  color: var(--el-color-primary);
  cursor: pointer;
  background: transparent;
  border: 0;
  border-left: 1px solid var(--el-border-color);
}

.analyzer-tip-btn:disabled {
  color: var(--el-text-color-disabled);
  cursor: default;
}

.analyzer-annotations {
  max-height: 420px;
  margin: 0;
  overflow: auto;
  font-family: monospace;
  font-size: 12px;
  line-height: 20px;
  white-space: pre;
}

.analyzer-file {
  display: none;
}

.analyzer-image-size {
  padding: 0 6px;
  font-size: 12px;
  line-height: 20px;
  color: var(--el-text-color-secondary);
  background: var(--el-fill-color);
  border-radius: 4px;
}

.analyzer-image-clear {
  display: inline-flex;
  padding: 0;
  margin-left: 4px;
  font-size: 14px;
  color: var(--el-text-color-placeholder);
  cursor: pointer;
  background: transparent;
  border: 0;
}

.analyzer-pager {
  justify-content: flex-end;
  margin-top: 8px;
}
</style>

<style>
.create-dialog.el-dialog {
  padding: 12px;
}

.create-dialog .el-dialog__header {
  padding-bottom: 8px;
}

.create-dialog .el-dialog__body {
  padding: 0;
}
</style>
