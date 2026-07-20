import type { PreviewLayout } from '@/api/preview'
import { LAYOUT_SLOT_COUNT } from '@/api/preview'

const CELL = 160
const BORDER = 1
const BORDER_COLOR = '#f56c6c'
const EMPTY_FILL = '#303133'
const BOUND_FILL = '#1d3a5c'
const TEXT_COLOR = '#ffffff'

export type ShotSlotInfo = {
  id: string | null
  name: string | null
}

export function composeShot(
  layout: PreviewLayout,
  slots: ShotSlotInfo[],
): string {
  const cols = Number(layout.split('x')[0])
  const count = LAYOUT_SLOT_COUNT[layout]
  const canvas = document.createElement('canvas')
  const size = cols * CELL + (cols + 1) * BORDER
  canvas.width = size
  canvas.height = size
  const ctx = canvas.getContext('2d')
  if (!ctx) {
    return ''
  }
  ctx.fillStyle = BORDER_COLOR
  ctx.fillRect(0, 0, size, size)
  for (let index = 0; index < count; index += 1) {
    const row = Math.floor(index / cols)
    const col = index % cols
    const x = BORDER + col * (CELL + BORDER)
    const y = BORDER + row * (CELL + BORDER)
    const info = slots[index]
    ctx.fillStyle = info?.id ? BOUND_FILL : EMPTY_FILL
    ctx.fillRect(x, y, CELL, CELL)
    if (info?.name) {
      ctx.fillStyle = TEXT_COLOR
      ctx.font = '14px sans-serif'
      ctx.fillText(info.name, x + 10, y + 28, CELL - 20)
    }
  }
  return canvas.toDataURL('image/png')
}
