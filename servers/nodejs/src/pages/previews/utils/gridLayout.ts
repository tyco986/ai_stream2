/** 1920x1080 and common preview streams use 16:9. */
export const SLOT_ASPECT_RATIO = 16 / 9

/** Focus list item size: 2x2's 4 slots fill the list height. */
export const FOCUS_LIST_FILL_COUNT = 4

export const SHELL_NAV_BASE_WIDTH = 200
export const GROUPS_BASE_WIDTH = 240

function fitContainGridSize(hostWidth: number, hostHeight: number, cols: number) {
  const safeCols = Math.max(cols, 1)
  const cellHeight = Math.min(
    hostHeight / safeCols,
    hostWidth / (safeCols * SLOT_ASPECT_RATIO),
  )
  const cellWidth = cellHeight * SLOT_ASPECT_RATIO
  return {
    cellWidth,
    cellHeight,
    width: cellWidth * safeCols,
    height: cellHeight * safeCols,
  }
}

/** Grid stage: 16:9 canvas; Groups panel is fixed GROUPS_BASE_WIDTH. */
export function fitPreviewChromeLayout(input: {
  shellWidth: number
  stageHeight: number
  cols: number
  sidebarMode: boolean
}) {
  const navBase = input.sidebarMode ? SHELL_NAV_BASE_WIDTH : 0
  const chromeBorders = input.sidebarMode ? 2 : 1
  const stageH = Math.max(input.stageHeight, 0)
  const fullHeight = fitContainGridSize(Number.POSITIVE_INFINITY, stageH, input.cols)
  const leftover =
    input.shellWidth - navBase - GROUPS_BASE_WIDTH - fullHeight.width - chromeBorders

  let stageWidth = fullHeight.width
  let stageHeight = fullHeight.height

  if (leftover < 0) {
    const maxMain = Math.max(
      input.shellWidth - navBase - GROUPS_BASE_WIDTH - chromeBorders,
      0,
    )
    const fitted = fitContainGridSize(maxMain, stageH, input.cols)
    stageWidth = fitted.width
    stageHeight = fitted.height
  }

  return {
    stageWidth,
    stageHeight,
  }
}

/** Fit an NxN 16:9 grid into a host box (fullscreen / constrained stage). */
export function fitStageInHost(input: {
  hostWidth: number
  hostHeight: number
  cols: number
}) {
  const fitted = fitContainGridSize(
    Math.max(input.hostWidth, 0),
    Math.max(input.hostHeight, 0),
    input.cols,
  )
  return {
    stageWidth: fitted.width,
    stageHeight: fitted.height,
  }
}

/** Matches FocusView gap between main and list. */
export const FOCUS_SPLIT_GAP = 1

/**
 * Split a grid-sized stage into focus left + right.
 * Right thumbs stay 16:9 (H/4); left takes the remainder (may letterbox).
 * Scrollbar overlays the list (no reserved gutter).
 */
export function splitFocusStage(input: {
  stageWidth: number
  stageHeight: number
  showList: boolean
}) {
  const stageH = Math.max(input.stageHeight, 0)
  const stageW = Math.max(input.stageWidth, 0)
  const itemHeight = stageH / FOCUS_LIST_FILL_COUNT
  const itemWidth = itemHeight * SLOT_ASPECT_RATIO
  const listWidth = input.showList ? itemWidth : 0
  const splitGap = input.showList ? FOCUS_SPLIT_GAP : 0
  return {
    mainWidth: Math.max(stageW - listWidth - splitGap, 0),
    mainHeight: stageH,
    listWidth,
    itemWidth,
    itemHeight,
  }
}
