import type { RecordingSegment } from '@/api/recordings'

export type TimeRange = {
  startMs: number
  endMs: number
}

export function parseWallClock(iso: string): number {
  return new Date(iso).getTime()
}

export function dateStartMs(date: string): number {
  return new Date(`${date}T00:00:00`).getTime()
}

export function dateEndMs(date: string): number {
  return new Date(`${date}T23:59:59`).getTime()
}

export function formatClock(ms: number): string {
  const d = new Date(ms)
  const hh = String(d.getHours()).padStart(2, '0')
  const mm = String(d.getMinutes()).padStart(2, '0')
  const ss = String(d.getSeconds()).padStart(2, '0')
  return `${hh}:${mm}:${ss}`
}

export function clampToDay(ms: number, date: string): number {
  const start = dateStartMs(date)
  const end = dateEndMs(date)
  let result = ms
  if (result < start) {
    result = start
  }
  if (result > end) {
    result = end
  }
  return result
}

export function findSegment(
  segments: RecordingSegment[],
  tMs: number,
): RecordingSegment | null {
  const hit = segments.find((seg) => {
    const start = parseWallClock(seg.start)
    const end = parseWallClock(seg.end)
    return start <= tMs && tMs < end
  })
  return hit ?? null
}

export function segmentOffsetSec(segment: RecordingSegment, tMs: number): number {
  return Math.max(0, (tMs - parseWallClock(segment.start)) / 1000)
}

/** Intersect multiple segment lists into wall-clock ranges present in every list. */
export function intersectSegmentRanges(
  lists: RecordingSegment[][],
): TimeRange[] {
  if (lists.length === 0) {
    return []
  }
  let ranges: TimeRange[] = lists[0].map((seg) => ({
    startMs: parseWallClock(seg.start),
    endMs: parseWallClock(seg.end),
  }))
  for (let i = 1; i < lists.length; i += 1) {
    const next: TimeRange[] = []
    const other = lists[i].map((seg) => ({
      startMs: parseWallClock(seg.start),
      endMs: parseWallClock(seg.end),
    }))
    for (const a of ranges) {
      for (const b of other) {
        const startMs = Math.max(a.startMs, b.startMs)
        const endMs = Math.min(a.endMs, b.endMs)
        if (startMs < endMs) {
          next.push({ startMs, endMs })
        }
      }
    }
    ranges = next
  }
  return ranges.sort((a, b) => a.startMs - b.startMs)
}

export function firstIntersectionStart(
  lists: RecordingSegment[][],
  date: string,
): number {
  const ranges = intersectSegmentRanges(lists)
  if (ranges.length > 0) {
    return ranges[0].startMs
  }
  return dateStartMs(date)
}

export function todayIso(): string {
  const now = new Date()
  const y = now.getFullYear()
  const m = String(now.getMonth() + 1).padStart(2, '0')
  const d = String(now.getDate()).padStart(2, '0')
  return `${y}-${m}-${d}`
}

export function monthIso(date = new Date()): string {
  const y = date.getFullYear()
  const m = String(date.getMonth() + 1).padStart(2, '0')
  return `${y}-${m}`
}

export function shiftMonth(month: string, delta: number): string {
  const [y, m] = month.split('-').map(Number)
  const d = new Date(y, m - 1 + delta, 1)
  return monthIso(d)
}
