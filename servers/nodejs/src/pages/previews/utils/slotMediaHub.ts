import { shallowRef } from 'vue'
import {
  MediamtxWhepPlayer,
  resolveWebrtcBaseUrl,
  type WhepPlayState,
} from './mediamtxWhep'

type SlotMediaEntry = {
  streamId: string
  pathName: string
  player: MediamtxWhepPlayer
  media: MediaStream | null
  status: WhepPlayState
  ready: Promise<void>
  resolveReady: () => void
}

const entries = new Map<number, SlotMediaEntry>()

/** Reactive snapshot for mirrors (focus main). */
export const slotMediaMap = shallowRef<Record<number, MediaStream | null>>({})

function publishMedia(slotIndex: number, media: MediaStream | null) {
  if (slotMediaMap.value[slotIndex] === media) {
    return
  }
  slotMediaMap.value = { ...slotMediaMap.value, [slotIndex]: media }
}

function createEntry(streamId: string, pathName: string): SlotMediaEntry {
  let resolveReady = () => undefined
  const ready = new Promise<void>((resolve) => {
    resolveReady = resolve
  })
  return {
    streamId,
    pathName,
    player: new MediamtxWhepPlayer(resolveWebrtcBaseUrl(), pathName),
    media: null,
    status: 'loading',
    ready,
    resolveReady,
  }
}

export function peekSlotMedia(
  slotIndex: number,
  streamId: string,
): MediaStream | null {
  const entry = entries.get(slotIndex)
  if (!entry || entry.streamId !== streamId || !entry.media) {
    return null
  }
  return entry.media
}

export async function bindSlotMedia(input: {
  slotIndex: number
  streamId: string
  pathName: string
  videoEl: HTMLVideoElement
}): Promise<WhepPlayState> {
  const { slotIndex, streamId, pathName, videoEl } = input
  let entry = entries.get(slotIndex)

  if (entry && entry.streamId !== streamId) {
    unbindSlotMedia(slotIndex)
    entry = undefined
  }

  if (entry && entry.streamId === streamId) {
    if (entry.media && entry.status === 'playing') {
      videoEl.srcObject = entry.media
      return 'playing'
    }
    if (entry.status === 'loading') {
      await entry.ready
      entry = entries.get(slotIndex)
      if (entry?.media && entry.streamId === streamId) {
        videoEl.srcObject = entry.media
        return entry.status
      }
      return entry?.status ?? 'failed'
    }
    if (entry.status === 'failed') {
      return 'failed'
    }
  }

  entry = createEntry(streamId, pathName)
  entries.set(slotIndex, entry)
  publishMedia(slotIndex, null)

  try {
    const media = await entry.player.play(videoEl)
    const current = entries.get(slotIndex)
    if (current !== entry) {
      return 'idle'
    }
    entry.media = media
    entry.status = 'playing'
    publishMedia(slotIndex, media)
    entry.resolveReady()
    return 'playing'
  } catch {
    const current = entries.get(slotIndex)
    if (current === entry) {
      entry.status = 'failed'
      entry.player.stop()
      entries.delete(slotIndex)
      publishMedia(slotIndex, null)
      entry.resolveReady()
    }
    return 'failed'
  }
}

export function unbindSlotMedia(slotIndex: number) {
  const entry = entries.get(slotIndex)
  if (!entry) {
    return
  }
  entry.player.stop()
  entries.delete(slotIndex)
  publishMedia(slotIndex, null)
  entry.resolveReady()
}

/** Drop hub entries that no longer match bound slot stream ids. */
export function reconcileSlotMedia(slots: (string | null)[]) {
  for (const index of [...entries.keys()]) {
    const wanted = index < slots.length ? slots[index] : null
    const entry = entries.get(index)
    if (!wanted || !entry || entry.streamId !== wanted) {
      unbindSlotMedia(index)
    }
  }
}
