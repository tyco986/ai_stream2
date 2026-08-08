import type { PreviewLayout } from '@/api/preview/types'
import { LAYOUT_SLOT_COUNT } from '@/api/preview/types'

export function resizeSlots(
  slots: (string | null)[],
  layout: PreviewLayout,
): (string | null)[] {
  const count = LAYOUT_SLOT_COUNT[layout]
  if (slots.length === count) {
    return [...slots]
  }
  if (slots.length < count) {
    return [...slots, ...Array.from({ length: count - slots.length }, () => null)]
  }
  return slots.slice(0, count)
}
